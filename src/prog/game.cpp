#include "engine/world.h"

Game::Game() :
	socks(nullptr),
	socket(nullptr),
	conn(nullptr),
	randGen(createRandomEngine()),
	randDist(0, 1)
{}

Game::~Game() {
	disconnect();
}

bool Game::connect() {
	IPaddress address;
	if (SDLNet_ResolveHost(&address, World::sets()->address.c_str(), World::sets()->port))
		return connectFail();
	if (!(socket = SDLNet_TCP_Open(&address)))
		return connectFail();

	socks = SDLNet_AllocSocketSet(1);
	SDLNet_TCP_AddSocket(socks, socket);
	conn = &Game::connectionWait;

	World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventConnectCancel, "Cancel"));
	return true;
}

void Game::disconnect() {
	if (socket) {
		SDLNet_TCP_DelSocket(socks, socket);
		SDLNet_TCP_Close(socket);
		SDLNet_FreeSocketSet(socks);
		socket = nullptr;
		socks = nullptr;
	}
}

void Game::tick() {
	if (socket && SDLNet_CheckSockets(socks, 0) > 0 && SDLNet_SocketReady(socket))
		SDLNet_TCP_Recv(socket, rcvBuf, sizeof(rcvBuf)) > 0 ? (this->*conn)() : disconnectMessage("Connection lost");
}

vector<Object*> Game::initObjects() {
	// prepare objects for setup
	setTiles(tiles, 1, &Program::eventClearTile, Object::INFO_LINES | Object::INFO_RAYCAST);
	setTiles(enemyTiles, -homeHeight, nullptr, Object::INFO_LINES);
	setMidTiles();
	setPieces(pieces, &Program::eventClearPiece, Object::INFO_TEXTURE);
	setPieces(enemyPieces, nullptr, Object::INFO_TEXTURE);
	setScreen();
	setBoard();

	// collect array of references to all objects
	sizet oi = 0;
	vector<Object*> objs(tiles.size() + enemyTiles.size() + midTiles.size() + pieces.size() + enemyPieces.size() + 2);
	setObjectAddrs(tiles, objs, oi);
	setObjectAddrs(enemyTiles, objs, oi);
	setObjectAddrs(midTiles, objs, oi);
	setObjectAddrs(pieces, objs, oi);
	setObjectAddrs(enemyPieces, objs, oi);
	objs[oi] = &screen;
	objs[oi+1] = &board;
	return objs;
}

Piece* Game::findPiece(vec2b pos) {
	array<Piece, numPieces>::iterator pce = std::find_if(pieces.begin(), pieces.end(), [pos](const Piece& it) -> bool { return it.getPos() == pos; });
	return pce != pieces.end() ? &*pce : nullptr;
}

void Game::connectionWait() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::full:
		disconnectMessage("Server full");
		break;
	case NetCode::setup:
		conn = &Game::connectionSetup;
		World::program()->eventOpenSetup();
		static_cast<ProgSetup*>(World::state())->amFirst = rcvBuf[1];
		break;
	default:
		printInvalidCode();
	}
}

void Game::connectionSetup() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::ready:
		receiveSetup();
		conn = &Game::connectionMatch;

		if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->stage == ProgSetup::Stage::ready)
			World::program()->eventOpenMatch();
		else {
			ps->enemyReady = true;
			ps->message->setText("Player ready");
			if (ps->stage < ProgSetup::Stage::pieces)
				World::scene()->setPopup(ProgState::createPopupMessage("Hurry the fuck up!", &Program::eventClosePopup));
		}
		break;
	default:
		printInvalidCode();
	}
}

void Game::connectionMatch() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::move:
		// TODO: the move
		break;
	default:
		printInvalidCode();
	}
}

void Game::setScreen() {
	vector<Vertex> verts = {
		Vertex(vec3(-4.7f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(0.f, 1.f)),
		Vertex(vec3(-4.7f, 2.6f, 0.f), vec3(0.f, 0.f, 1.f), vec2(0.f, 0.f)),
		Vertex(vec3(4.7f, 2.6f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f, 0.f)),
		Vertex(vec3(4.7f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f, 1.f))
	};
	screen = Object(vec3(0.f, 0.f, -0.5f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, {120, 120, 120, 255}, Object::INFO_FILL);
}

void Game::setBoard() {
	vector<Vertex> verts = {
		Vertex(vec3(-5.5f, 0.f, 5.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 1.f)),
		Vertex(vec3(-5.5f, 0.f, -5.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 0.f)),
		Vertex(vec3(5.5f, 0.f, -5.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 0.f)),
		Vertex(vec3(5.5f, 0.f, 5.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 1.f))
	};
	board = Object(vec3(0.f, -0.01f, 0.f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, {30, 5, 5, 255}, Object::INFO_FILL);
}

void Game::setTilesInteract(Tile* tiles, sizet num, bool on) {
	for (sizet i = 0; i < num; i++)
		tiles[i].mode = on ? tiles[i].mode | Object::INFO_RAYCAST : tiles[i].mode & ~Object::INFO_RAYCAST;
}

void Game::setPiecesInteract(array<Piece, Game::numPieces> pieces, bool on) {
	for (Piece& it : pieces)
		it.mode = on ? it.mode | Object::INFO_SHOW | Object::INFO_RAYCAST : it.mode & ~Object::INFO_SHOW & ~Object::INFO_RAYCAST;
}

void Game::prepareMatch() {
	// make everything interactable
	setOwnTilesInteract(true);
	setMidTilesInteract(true);
	setTilesInteract(enemyTiles.data(), enemyTiles.size(), true);
	setOwnPiecesInteract(true);
	setPiecesInteract(enemyPieces, true);

	// rearange middle tiles
	if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->amFirst) {
		for (sizet i = 0; i < midTiles.size(); i++)
			if ((midTiles[i].getType() != Tile::Type::empty) && (ps->rcvMidBuffer[i] != Tile::Type::empty))
				std::find_if(midTiles.begin(), midTiles.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(ps->rcvMidBuffer[i]);
	} else {
		for (sizet i = 0; i < midTiles.size(); i++)
			if (ps->rcvMidBuffer[i] != Tile::Type::empty) {
				if (midTiles[i].getType() != Tile::Type::empty)
					midTiles[sizet(std::find(ps->rcvMidBuffer.begin(), ps->rcvMidBuffer.end(), Tile::Type::empty) - ps->rcvMidBuffer.begin())].setType(midTiles[i].getType());
				midTiles[i].setType(ps->rcvMidBuffer[i]);
			}
	}
	std::find_if(midTiles.begin(), midTiles.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(Tile::Type::fortress);
}

string Game::checkOwnTiles() const {
	uint8 empties = 0;
	for (int8 y = 1; y <= homeHeight; y++) {
		// collect information and check if the fortress isn't touching a border
		uint8 cnt[sizet(Tile::Type::fortress)] = {0, 0, 0, 0};
		for (int8 x = 0; x < boardSize.x; x++) {
			if (Tile::Type type = tiles[sizet(y * boardSize.x + x)].getType(); type < Tile::Type::fortress)
				cnt[sizet(type)]++;
			else if (vec2b pos(x, y); outRange(pos, vec2b(1, 2), vec2b(boardSize.x - 2, homeHeight - 1)))
				return firstUpper(Tile::names[uint8(Tile::Type::fortress)]) + " at " + pos.toString('|') + " not allowed";
			else
				empties++;
		}
		// check diversity in each row
		for (uint8 i = 0; i < sizet(Tile::Type::fortress); i++)
			if (!cnt[i])
				return firstUpper(Tile::names[i]) + " missing in row " + to_string(i);
	}
	return empties == 1 ? "" : "Not all tiles were placed";
}

string Game::checkMidTiles() const {
	// collect information
	uint8 cnt[sizet(Tile::Type::fortress)] = {0, 0, 0, 0};
	for (int8 i = 0; i < boardSize.x; i++)
		if (Tile::Type type = midTiles[sizet(i)].getType(); type < Tile::Type::fortress)
			cnt[sizet(type)]++;
	// check if all pieces except for dragon were placed
	for (uint8 i = 0; i < sizet(Tile::Type::fortress); i++)
		if (cnt[i] != 1)
			return firstUpper(Tile::names[i]) + " wasn't placed";
	return "";
}

string Game::checkOwnPieces() const {
	for (const Piece& it : pieces)
		if (outRange(it.getPos(), vec2b(0, boardSize.x), vec2b(1, homeHeight)) && it.getType() != Piece::Type::dragon)
			return firstUpper(Piece::names[uint8(it.getType())]) + " wasn't placed";
	return "";
}

void Game::fillInFortress() {
	if (array<Tile, numTiles>::iterator fit = std::find_if(tiles.begin(), tiles.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; }); fit != tiles.end())
		fit->setType(Tile::Type::fortress);
}

void Game::takeOutFortress() {
	if (array<Tile, numTiles>::iterator fit = std::find_if(tiles.begin(), tiles.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::fortress; }); fit != tiles.end())
		fit->setType(Tile::Type::empty);
}

void Game::killPiece(Piece* piece) {
	piece->setPos(INT8_MIN);
	piece->mode &= ~Object::INFO_SHOW & ~Object::INFO_RAYCAST;
	// TODO: send piece update
}

void Game::setTiles(array<Tile, numTiles>& tiles, int8 yofs, OCall call, Object::Info mode) {
	sizet id = 0;
	for (int8 y = yofs, yend = homeHeight + yofs; y < yend; y++)
		for (int8 x = 0; x < boardSize.x; x++)
			tiles[id++] = Tile(vec2b(x, y), Tile::Type::empty, call, mode);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < boardSize.x; i++)
		midTiles[sizet(i)] = Tile(vec2b(i, 0), Tile::Type::empty, &Program::eventClearTile, Object::INFO_LINES);
}

void Game::setPieces(array<Piece, numPieces>& pieces, OCall call, Object::Info mode) {
	sizet id = 0;
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::ranger, call, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::spearman, call, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::crossbowman, call, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::catapult, call, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::trebuchet, call, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::lancer, call, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::warhorse, call, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::elephant, call, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::dragon, call, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::throne, call, mode);
}

void Game::sendSetup() const {
	uint8 buff[1 + numTiles + boardSize.x + numPieces * 2] = {uint8(NetCode::ready)};	// size should be 121
	sizet bi = 1;
	bufferizeTilesType(tiles.data(), tiles.size(), buff, bi);
	bufferizeTilesType(midTiles.data(), midTiles.size(), buff, bi);
	for (const Piece& it : pieces)
		*reinterpret_cast<vec2b*>(buff) = it.getPos();

	SDLNet_TCP_Send(socket, buff, sizeof(buff));
}

void Game::bufferizeTilesType(const Tile* tiles, sizet num, uint8* dst, sizet& di) {
	for (sizet i = 0; i < num; i++)
		dst[di] = uint8(tiles[i].getType());
	std::reverse(dst + di, dst + di + num);
	di += num;
}

void Game::receiveSetup() {
	sizet bi = 1;
	for (sizet i = 0; i < enemyTiles.size(); i++, bi++)
		enemyTiles[i].setType(Tile::Type(rcvBuf[bi]));
	for (int i = 0; i < boardSize.x; i++, bi++)
		static_cast<ProgSetup*>(World::state())->rcvMidBuffer[i] = Tile::Type(rcvBuf[bi]);
	for (sizet i = 0; i < enemyPieces.size(); i++, bi += 2)
		enemyPieces[i].setPos(*reinterpret_cast<vec2b*>(rcvBuf + bi));
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}
