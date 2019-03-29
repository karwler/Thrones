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
		pieces.ene[rcvBuf[1]].setPos(*reinterpret_cast<vec2b*>(rcvBuf + 2));
		break;
	case NetCode::kill: {
		array<Piece, numPieces>& pcs = rcvBuf[1] ? pieces.ene : pieces.own;
		pcs[rcvBuf[2]].disable();
		break; }
	case NetCode::fortress:
		(tiles.begin() + rcvBuf[1])->ruined = rcvBuf[2];
		break;
	default:
		printInvalidCode();
	}
}

Piece* Game::findPiece(vec2b pos) {
	Piece* pce = std::find_if(pieces.begin(), pieces.end(), [pos](const Piece& it) -> bool { return it.getPos() == pos; });
	return pce != pieces.end() ? pce : nullptr;
}

vector<Object*> Game::initObjects() {
	// prepare objects for setup
	setTiles(tiles.ene, -homeHeight, nullptr, Object::INFO_LINES);
	setMidTiles();
	setTiles(tiles.own, 1, &Program::eventClearTile, Object::INFO_LINES | Object::INFO_RAYCAST);
	setPieces(pieces.own, &Program::eventClearPiece, Object::INFO_TEXTURE);
	setPieces(pieces.ene, nullptr, Object::INFO_TEXTURE);
	setScreen();
	setBoard();

	// collect array of references to all objects
	sizet oi = 0;
	vector<Object*> objs(tiles.size() + pieces.size() + 2);
	setObjectAddrs(tiles.begin(), tiles.size(), objs, oi);
	setObjectAddrs(pieces.begin(), pieces.size(), objs, oi);
	objs[oi] = &screen;
	objs[oi+1] = &board;
	return objs;
}

void Game::prepareMatch() {
	// make everything interactable
	setOwnTilesInteract(true);
	setMidTilesInteract(true);
	setTilesInteract(tiles.ene.data(), tiles.ene.size(), true);
	setOwnPiecesInteract(true);
	setPiecesInteract(pieces.ene, true);

	// rearange middle tiles
	if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->amFirst) {
		for (sizet i = 0; i < tiles.mid.size(); i++)
			if ((tiles.mid[i].getType() != Tile::Type::empty) && (ps->rcvMidBuffer[i] != Tile::Type::empty))
				std::find_if(tiles.mid.begin(), tiles.mid.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(ps->rcvMidBuffer[i]);
	} else {
		for (sizet i = 0; i < tiles.mid.size(); i++)
			if (ps->rcvMidBuffer[i] != Tile::Type::empty) {
				if (tiles.mid[i].getType() != Tile::Type::empty)
					tiles.mid[sizet(std::find(ps->rcvMidBuffer.begin(), ps->rcvMidBuffer.end(), Tile::Type::empty) - ps->rcvMidBuffer.begin())].setType(tiles.mid[i].getType());
				tiles.mid[i].setType(ps->rcvMidBuffer[i]);
			}
	}
	std::find_if(tiles.mid.begin(), tiles.mid.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(Tile::Type::fortress);
}

string Game::checkOwnTiles() const {
	uint8 empties = 0;
	for (int8 y = 1; y <= homeHeight; y++) {
		// collect information and check if the fortress isn't touching a border
		uint8 cnt[sizet(Tile::Type::fortress)] = {0, 0, 0, 0};
		for (int8 x = 0; x < boardSize; x++) {
			if (Tile::Type type = tiles.mid[sizet(y * boardSize + x)].getType(); type < Tile::Type::fortress)
				cnt[sizet(type)]++;
			else if (vec2b pos(x, y); outRange(pos, vec2b(1, 2), vec2b(boardSize - 2, homeHeight - 1)))
				return firstUpper(Tile::names[uint8(Tile::Type::fortress)]) + " at " + pos.toString('|') + " not allowed";
			else
				empties++;
		}
		// check diversity in each row
		for (uint8 i = 0; i < sizet(Tile::Type::fortress); i++)
			if (!cnt[i])
				return firstUpper(Tile::names[i]) + " missing in row " + to_string(i);
	}
	return empties == 1 ? emptyStr : "Not all tiles were placed";
}

string Game::checkMidTiles() const {
	// collect information
	uint8 cnt[sizet(Tile::Type::fortress)] = {0, 0, 0, 0};
	for (int8 i = 0; i < boardSize; i++)
		if (Tile::Type type = tiles.mid[sizet(i)].getType(); type < Tile::Type::fortress)
			cnt[sizet(type)]++;
	// check if all pieces except for dragon were placed
	for (uint8 i = 0; i < sizet(Tile::Type::fortress); i++)
		if (cnt[i] != 1)
			return firstUpper(Tile::names[i]) + " wasn't placed";
	return emptyStr;
}

string Game::checkOwnPieces() const {
	for (const Piece& it : pieces.own)
		if (outRange(it.getPos(), vec2b(0, boardSize), vec2b(1, homeHeight)) && it.getType() != Piece::Type::dragon)
			return firstUpper(Piece::names[uint8(it.getType())]) + " wasn't placed";
	return emptyStr;
}

void Game::fillInFortress() {
	if (array<Tile, numTiles>::iterator fit = std::find_if(tiles.own.begin(), tiles.own.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; }); fit != tiles.own.end())
		fit->setType(Tile::Type::fortress);
}

void Game::takeOutFortress() {
	if (array<Tile, numTiles>::iterator fit = std::find_if(tiles.own.begin(), tiles.own.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::fortress; }); fit != tiles.own.end())
		fit->setType(Tile::Type::empty);
}

void Game::movePiece(Piece* piece, vec2b pos, Piece* occupant) {
	vec2b spos = piece->getPos();
	vector<Tile*> moves = collectMoveTiles(piece);	// TODO: consider blocking pieces
	if (vector<Tile*>::iterator it = std::find_if(moves.begin(), moves.end(), [pos](Tile* til) -> bool { return til->getPos() == pos; }); it == moves.end() || spos == pos)
		return;

	if (occupant) {
		if (occupant < pieces.ene.data())
			killPiece(occupant);			// TODO: attack conditions
		else if (survivalCheck(piece, pos))
			placePiece(occupant, spos);		// if swapping with ally and survival check passed
		else
			killPiece(occupant);			// swapped piece failed check
	}
	if (survivalCheck(piece, spos)) {
		if (Tile* til = getTile(pos); til->getType() == Tile::Type::fortress)
			updateFortress(til, false);		// restore possibly ruined fortress
		placePiece(piece, pos);
	} else
		killPiece(piece);		// piece failed check
}

void Game::attackPiece(Piece* piece, Piece* occupant) {
	if (sizet(occupant - pieces.own.data()) < pieces.size())	// no friendly fire
		return;

	vec2b pos = occupant->getPos();
	vector<Tile*> attacks = collectAttackTiles(piece);
	if (vector<Tile*>::iterator it = std::find_if(attacks.begin(), attacks.end(), [pos](Tile* til) -> bool { return til->getPos() == pos; }); it != attacks.end()) {
		if ((*it)->getType() == Tile::Type::fortress)
			updateFortress(*it, true);
		killPiece(occupant);	// TODO: attack conditions
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

void Game::setTiles(array<Tile, numTiles>& tiles, int8 yofs, OCall call, Object::Info mode) {
	sizet id = 0;
	for (int8 y = yofs, yend = homeHeight + yofs; y < yend; y++)
		for (int8 x = 0; x < boardSize; x++)
			tiles[id++] = Tile(vec2b(x, y), Tile::Type::empty, call, nullptr, nullptr, mode);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < boardSize; i++)
		tiles.mid[sizet(i)] = Tile(vec2b(i, 0), Tile::Type::empty, &Program::eventClearTile, nullptr, nullptr, Object::INFO_LINES);
}

void Game::setPieces(array<Piece, numPieces>& pieces, OCall call, Object::Info mode) {
	sizet id = 0;
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::ranger, call, nullptr, nullptr, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::spearman, call, nullptr, nullptr, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::crossbowman, call, nullptr, nullptr, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::catapult, call, nullptr, nullptr, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::trebuchet, call, nullptr, nullptr, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::lancer, call, nullptr, nullptr, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::warhorse, call, nullptr, nullptr, mode);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::elephant, call, nullptr, nullptr, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::dragon, call, nullptr, nullptr, mode);
	pieces[id++] = Piece(vec2b(INT8_MIN), Piece::Type::throne, call, nullptr, nullptr, mode);
}

void Game::setTilesInteract(Tile* tiles, sizet num, bool on) {
	for (sizet i = 0; i < num; i++)
		tiles[i].mode = on ? tiles[i].mode | Object::INFO_RAYCAST : tiles[i].mode & ~Object::INFO_RAYCAST;
}

void Game::setPiecesInteract(array<Piece, Game::numPieces> pieces, bool on) {
	for (Piece& it : pieces)
		it.mode = on ? it.mode | Object::INFO_SHOW | Object::INFO_RAYCAST : it.mode & ~(Object::INFO_SHOW | Object::INFO_RAYCAST);
}

void Game::receiveSetup() {
	sizet bi = 1;
	for (sizet i = 0; i < numTiles; i++, bi++)
		tiles.ene[i].setType(Tile::Type(rcvBuf[bi]));
	for (int8 i = 0; i < boardSize; i++, bi++)
		static_cast<ProgSetup*>(World::state())->rcvMidBuffer[sizet(i)] = Tile::Type(rcvBuf[bi]);
	for (sizet i = 0; i < numPieces; i++, bi += 2)
		pieces.ene[i].setPos(*reinterpret_cast<vec2b*>(rcvBuf + bi));
}

void Game::sendSetup() {
	uint8 buff[1 + boardSize + numTiles + numPieces * 2] = {uint8(NetCode::ready)};	// size should be 121
	for (sizet i = 0; i < boardSize + numTiles; i++)
		buff[boardSize + numTiles - i] = uint8(tiles.mid[i].getType());
	for (sizet i = 0, bi = boardSize + numTiles + 1; i < numPieces; i++, bi += 2)
		*reinterpret_cast<vec2b*>(buff + bi) = pieces.own[i].getPos();
	sendData(buff, sizeof(buff));
}

bool Game::sendData(const uint8* data, int size) {
	if (SDLNet_TCP_Send(socket, data, size) != size) {
		disconnectMessage(SDLNet_GetError());
		return false;
	}
	return true;
}

vector<Tile*> Game::collectMoveTiles(Piece* piece) {
	vec2b pos = piece->getPos();
	if (piece->getType() == Piece::Type::spearman)
		return getTile(pos)->getType() == Tile::Type::water ? collectTilesByType(pos) : collectTilesBySingle(pos);	// TODO: single or any water
	if (piece->getType() == Piece::Type::lancer)
		return getTile(pos)->getType() == Tile::Type::plains && true ? collectTilesByType(pos) : collectTilesBySingle(pos);	// TOOD: single or any plains if no attack
	if (piece->getType() == Piece::Type::dragon)
		return collectTilesByArea(pos, 4);
	return collectTilesBySingle(pos);
}

vector<Tile*> Game::collectTilesBySingle(vec2b pos) {
	vector<Tile*> ret;
	int8 xs = pos.x != 0 ? pos.x - 1 : 0;
	int8 xe = pos.x != boardSize - 1 ? pos.x + 2 : boardSize;
	for (int8 y = pos.y != -homeHeight ? pos.y - 1 : -homeHeight, ye = pos.y != homeHeight ? pos.y + 1 : homeHeight, ofs = y * boardSize; y <= ye; y++) {
		for (int8 x = xs; x < xe; x++)
			ret.push_back(tiles.begin() + ofs + x);
	}
	return ret;
}

vector<Tile*> Game::collectTilesByArea(vec2b pos, int8 dist) {
	vector<Tile*> ret;	// TODO: count steps
	int8 xs = pos.x - dist >= 0 ? pos.x - dist : 0;
	int8 xe = pos.x + dist < boardSize ? pos.x + dist + 1 : boardSize;
	for (int8 y = pos.y - dist >= -homeHeight ? pos.y - dist : -homeHeight, ye = pos.y + dist <= homeHeight ? pos.y + dist : homeHeight, ofs = y * boardSize; y <= ye; y++)
		for (int8 x = xs; x < xe; x++)
			ret.push_back(tiles.begin() + ofs + x);
	return ret;
}

vector<Tile*> Game::collectTilesByType(vec2b pos) {
	// mark all tiles as not visited yet
	bool* visited = new bool[tiles.size()];
	memset(visited, 0, tiles.size() * sizeof(bool));

	// collect tiles of same type connected to the one at pos
	vector<Tile*> ret(tiles.size());
	Tile** end = ret.data();
	appAdjacentTilesByType(uint8(pos.y * boardSize + pos.x) + numTiles, visited, end, getTile(pos)->getType());

	// cleanup
	delete[] visited;
	ret.resize(sizet(end - ret.data()));
	vector<Tile*>::iterator tend = ret.end();

	// add possible additional movement
	for (Tile* it : collectTilesBySingle(pos))
		if (std::find(ret.begin(), tend, it) == tend)
			ret.push_back(it);
	return ret;
}

void Game::appAdjacentTilesByType(uint8 pos, bool* visits, Tile**& out, Tile::Type type) {
	visits[pos] = true;
	*out++ = tiles.begin() + pos;

	if (uint8 next = pos - boardSize; next < tiles.size() && (tiles.begin() + next)->getType() == type)
		appAdjacentTilesByType(next, visits, out, type);
	if (uint8 next = pos - 1; pos % boardSize && (tiles.begin() + next)->getType() == type)
		appAdjacentTilesByType(next, visits, out, type);
	if (uint8 next = pos + boardSize; next < tiles.size() && (tiles.begin() + next)->getType() == type)
		appAdjacentTilesByType(next, visits, out, type);
	if (uint8 next = pos + 1; !(next % boardSize) && (tiles.begin() + next)->getType() == type)
		appAdjacentTilesByType(next, visits, out, type);
}

vector<Tile*> Game::collectAttackTiles(Piece* piece) {
	if (piece->getType() == Piece::Type::crossbowman)
		return collectTilesByDistance(piece->getPos(), 1);
	if (piece->getType() == Piece::Type::catapult)
		return collectTilesByDistance(piece->getPos(), 2);
	if (piece->getType() == Piece::Type::trebuchet)
		return collectTilesByDistance(piece->getPos(), 3);
	// TODO: dragon and throne
	return {};
}

vector<Tile*> Game::collectTilesByDistance(vec2b pos, int8 dist) {
	bool ul = pos.x - dist >= 0;
	bool ur = pos.x + dist < boardSize;
	int8 lft = ul ? pos.x - dist : 0;
	int8 rgt = ur ? pos.x + dist : boardSize - 1;
	int8 top, bot;
	vector<Tile*> ret;

	if (pos.y - dist >= -homeHeight) {	// top line
		top = pos.y - dist;
		int8 ofs = top * boardSize;
		ret.insert(ret.end(), ret.begin() + ofs + lft, ret.begin() + ofs + rgt + 1);
	} else
		top = -homeHeight;
	top++;

	if (pos.y + dist <= homeHeight) {	// bottom line
		bot = pos.y + dist;
		int8 ofs = bot * boardSize;
		ret.insert(ret.end(), ret.begin() + ofs + lft, ret.begin() + ofs + rgt + 1);
	} else
		bot = homeHeight;
	bot--;

	if (ul)		// left column excluding top- and bottom-most cell
		for (int8 y = top; y <= bot; y++)
			ret.push_back(tiles.begin() + y * boardSize + lft);
	if (ur)		// right column excluding top- and bottom-most cell
		for (int8 y = top; y <= bot; y++)
			ret.push_back(tiles.begin() + y * boardSize + rgt);
	return ret;
}

void Game::placePiece(Piece* piece, vec2b pos) {
	piece->setPos(pos);
	uint8 buff[4] = {uint8(NetCode::kill), uint8(piece - pieces.own.data()), uint8(boardSize - pos.x - 1), uint8(-pos.y)};	// rotate position 180
	sendData(buff, sizeof(buff));
}

void Game::killPiece(Piece* piece) {
	piece->disable();
	bool mine = piece < pieces.ene.data();
	uint8 buff[3] = {uint8(NetCode::kill), uint8(mine), uint8(piece - (mine ? pieces.own.data() : pieces.ene.data()))};	// index doesn't need to be modified
	sendData(buff, sizeof(buff));
}

void Game::updateFortress(Tile* fort, bool ruined) {
	fort->ruined = ruined;
	uint8 buff[3] = {uint8(NetCode::fortress), uint8(tiles.end() - fort - 1), ruined};	// invert index of fort
	sendData(buff, sizeof(buff));
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}
