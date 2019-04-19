#include "engine/world.h"

// DATA BATCH

DataBatch::DataBatch() :
	size(0)
{}

void DataBatch::push(NetCode code, const vector<uint8>& info) {
	push(code);
	memcpy(data + size, info.data(), info.size());
	size += uint8(info.size());
}

bool DataBatch::send(TCPsocket sock) {
	if (SDLNet_TCP_Send(sock, data, size) == size) {
		size = 0;
		return true;
	}
	return false;
}

// RECORD

Game::Record::Record(Piece* piece, bool attack, bool swap) :
	piece(piece),
	attack(attack),
	swap(swap)
{}

// GAME

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
	if (socket && SDLNet_CheckSockets(socks, 0) > 0 && SDLNet_SocketReady(socket)) {
		if (int len = SDLNet_TCP_Recv(socket, recvb, Server::bufSiz); len > 0)
			for (int i = 0; i < len; i += (this->*conn)(recvb + i));
		else
			disconnectMessage("Connection lost");
	}
}

uint8 Game::connectionWait(const uint8* data) {
	switch (NetCode(*data)) {
	case NetCode::full:
		disconnectMessage("Server full");
		return 1;
	case NetCode::setup:
		conn = &Game::connectionSetup;
		myTurn = data[1];
		World::program()->eventOpenSetup();
		return 2;
	default:
		printInvalidCode(*data);
	}
	return 1;
}

uint8 Game::connectionSetup(const uint8* data) {
	switch (NetCode(*data)) {
	case NetCode::ready: {
		uint8 bi = 1;
		for (uint8 i = 0; i < numTiles; i++, bi++)
			tiles.ene[i].setType(Tile::Type(data[bi]));
		for (int8 i = 0; i < boardSize; i++, bi++)
			static_cast<ProgSetup*>(World::state())->rcvMidBuffer[sizet(i)] = Tile::Type(data[bi]);
		for (uint8 i = 0; i < numPieces; i++, bi += 2)
			pieces.ene[i].setPos(*reinterpret_cast<const vec2b*>(data + bi));

		conn = &Game::connectionMatch;
		if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->stage == ProgSetup::Stage::ready)
			World::program()->eventOpenMatch();
		else {
			ps->enemyReady = true;
			ps->message->setText("Player ready");
			if (ps->stage < ProgSetup::Stage::pieces)
				World::scene()->setPopup(ProgState::createPopupMessage("Hurry the fuck up!", &Program::eventClosePopup));
		}
		return bi; }
	default:
		printInvalidCode(*data);
	}
	return 1;
}

uint8 Game::connectionMatch(const uint8* data) {
	switch (NetCode(*data)) {
	case NetCode::move:
		pieces.ene[data[1]].setPos(*reinterpret_cast<const vec2b*>(data + 2));
		return 4;
	case NetCode::kill: {
		(data[1] ? pieces.ene : pieces.own)[data[2]].disable();
		return 3; }
	case NetCode::ruin:
		(tiles.begin() + data[1])->ruined = data[2];
		return 3;
	case NetCode::record:
		record = Record(pieces.ene.data() + data[1], data[2], data[3]);
		myTurn = !myTurn;
		prepareTurn();
		return 4;
	case NetCode::win:
		disconnectMessage("You lose.");
		return 1;
	default:
		printInvalidCode(*data);
	}
	return 1;
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
	prepareTurn();

	// rearange middle tiles
	if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); myTurn) {
		for (sizet i = 0; i < tiles.mid.size(); i++)
			if ((tiles.mid[i].getType() != Tile::Type::empty) && (ps->rcvMidBuffer[i] != Tile::Type::empty))
				std::find_if(tiles.mid.begin(), tiles.mid.end(), [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(ps->rcvMidBuffer[i]);
	} else
		for (sizet i = 0; i < tiles.mid.size(); i++)
			if (ps->rcvMidBuffer[i] != Tile::Type::empty) {
				if (tiles.mid[i].getType() != Tile::Type::empty)
					tiles.mid[sizet(std::find(ps->rcvMidBuffer.begin(), ps->rcvMidBuffer.end(), Tile::Type::empty) - ps->rcvMidBuffer.begin())].setType(tiles.mid[i].getType());
				tiles.mid[i].setType(ps->rcvMidBuffer[i]);
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

bool Game::movePiece(Piece* piece, vec2b pos, Piece* occupant) {
	bool attacking = occupant >= pieces.ene.data();
	if (attacking ? !checkAttack(piece, occupant) : occupant && occupant->getType() == piece->getType())
		return false;

	vec2b spos = piece->getPos();
	vector<Tile*> moves = collectMoveTiles(piece, attacking);	// TODO: consider blocking pieces
	if (vector<Tile*>::iterator it = std::find_if(moves.begin(), moves.end(), [pos](Tile* til) -> bool { return til->getPos() == pos; }); it == moves.end() || spos == pos)
		return false;

	if (!survivalCheck(piece, spos)) {
		removePiece(piece);
		sendData();
		return false;
	}

	// handle movement
	if (occupant && !attacking) {
		placePiece(occupant, spos);	// switch ally
		updateRecord(piece, false, true);
	} else {
		if (attacking) {
			removePiece(occupant);	// execute attack
			updateRecord(piece, true, false);
		}
		if (Tile* til = getTile(spos); til->getType() == Tile::Type::fortress && til->ruined)
			updateFortress(til, false);	// restore fortress
	}
	placePiece(piece, pos);

	if (Tile* til = getTile(pos); (attacking && occupant->getType() == Piece::Type::throne) || (piece->getType() == Piece::Type::throne && til->getType() == Tile::Type::fortress && til < tiles.mid.data())) {
		sendb.push(NetCode::win);
		sendData();
		disconnectMessage("You win.");
	} else
		sendData();
	return true;
}

bool Game::attackPiece(Piece* killer, vec2b pos, Piece* piece) {
	vec2b kpos = killer->getPos();
	vector<Tile*> attacks = collectAttackTiles(killer);
	if (std::find_if(attacks.begin(), attacks.end(), [pos](Tile* til) -> bool { return til->getPos() == pos; }) == attacks.end() || kpos == pos)
		return false;

	// specific behaviour for firing on fortress
	bool succ = survivalCheck(killer, kpos);
	if (Tile* til = getTile(pos); til->getType() == Tile::Type::fortress && !til->ruined) {	// TODO: separate check for adjacent pieces
		if (succ) {
			updateFortress(til, true);
			updateRecord(killer, true, false);
		} else
			removePiece(killer);

		sendData();
		return succ;
	}
	// no piece, no friendly fire and unmet conditions
	if (!piece || piece < pieces.ene.data() || !checkAttack(killer, piece))
		return false;

	if (succ) {
		removePiece(piece);
		updateRecord(killer, true, false);
	} else
		removePiece(killer);
	sendData();
	return succ;
}

bool Game::placeDragon(vec2b pos, Piece* occupant) {
	if (getTile(pos)->getType() != Tile::Type::fortress)	// tile needs to be a fortress
		return false;

	// kill any piece that might be occupying the tile
	if (occupant)
		removePiece(occupant);

	// place the dragon
	Piece* maid = getOwnPieces(Piece::Type::dragon);
	maid->mode |= Object::INFO_SHOW;
	placePiece(maid, pos);
	return true;
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

void Game::prepareTurn() {
	setOwnTilesInteract(myTurn);
	setMidTilesInteract(myTurn);
	setTilesInteract(tiles.ene.data(), tiles.ene.size(), myTurn);
	setOwnPiecesInteract(myTurn);
	setPiecesInteract(pieces.ene, myTurn);
}

Piece* Game::getOwnPieces(Piece::Type type) {
	Piece* it = pieces.own.data();
	for (uint8 t = 0; t < uint8(type); t++)
		it += Piece::amounts[t];
	return it;
}

vector<Tile*> Game::collectMoveTiles(Piece* piece, bool attacking) {
	vec2b pos = piece->getPos();
	if (piece->getType() == Piece::Type::spearman)
		return getTile(pos)->getType() == Tile::Type::water ? mergeTiles(collectTilesByType(pos), collectTilesBySingle(pos)) : collectTilesBySingle(pos);
	if (piece->getType() == Piece::Type::lancer)
		return getTile(pos)->getType() == Tile::Type::plains && attacking ? collectTilesBySingle(pos) : mergeTiles(collectTilesByType(pos), collectTilesBySingle(pos));
	if (piece->getType() == Piece::Type::dragon)
		return collectTilesByArea(pos, 4);
	return collectTilesBySingle(pos);
}

vector<Tile*> Game::collectTilesBySingle(vec2b pos) {
	vector<Tile*> ret;
	int8 xs = pos.x != 0 ? pos.x - 1 : 0;
	int8 xe = pos.x != boardSize - 1 ? pos.x + 2 : boardSize;
	for (int8 y = pos.y != -homeHeight ? pos.y - 1 : -homeHeight, ye = pos.y != homeHeight ? pos.y + 1 : homeHeight, ofs = y * boardSize; y <= ye; y++)
		for (int8 x = xs; x < xe; x++)
			ret.push_back(tiles.begin() + ofs + x);
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

bool Game::checkAttack(Piece* killer, Piece* victim) {
	Tile* dtil = getTile(victim->getPos());
	if (killer->getType() != Piece::Type::throne && dtil->getType() == Tile::Type::fortress && !dtil->ruined)
		return false;
	if (victim->getType() == Piece::Type::ranger && dtil->getType() == Tile::Type::mountain)
		return false;
	if (killer->getType() == Piece::Type::crossbowman && (dtil->getType() == Tile::Type::forest || dtil->getType() == Tile::Type::mountain))
		return false;
	if (killer->getType() == Piece::Type::catapult && dtil->getType() == Tile::Type::forest)
		return false;
	if (victim->getType() == Piece::Type::warhorse && victim == record.piece && (record.attack || record.swap))
		return false;
	if (killer->getType() != Piece::Type::dragon) {
		if (victim->getType() == Piece::Type::elephant && dtil->getType() == Tile::Type::plains)
			return false;
	} else if (dtil->getType() == Tile::Type::forest)
		return false;
	return true;
}

void Game::sendSetup() {
	sendb.size = 1 + boardSize + numTiles + numPieces * sizeof(vec2b);
	sendb.data[0] = uint8(NetCode::ready);
	for (sizet i = 0; i < boardSize + numTiles; i++)
		sendb.data[boardSize + numTiles - i] = uint8(tiles.mid[i].getType());
	for (sizet i = 0, e = boardSize + numTiles + 1; i < numPieces; i++)
		*reinterpret_cast<vec2b*>(sendb.data + e + i * sizeof(vec2b)) = pieces.own[i].getPos();
	sendData();
}

void Game::sendData() {
	if (!sendb.send(socket))
		disconnectMessage(SDLNet_GetError());
}

void Game::placePiece(Piece* piece, vec2b pos) {
	piece->setPos(pos);
	sendb.push(NetCode::move, {uint8(piece - pieces.own.data()), uint8(boardSize - pos.x - 1), uint8(-pos.y)});	// rotate position 180
}

void Game::removePiece(Piece* piece) {
	piece->disable();
	bool mine = piece < pieces.ene.data();
	sendb.push(NetCode::kill, {uint8(mine), uint8(piece - (mine ? pieces.own.data() : pieces.ene.data()))});	// index doesn't need to be modified
}

void Game::updateFortress(Tile* fort, bool ruined) {
	fort->ruined = ruined;
	sendb.push(NetCode::ruin, {uint8(tiles.end() - fort - 1), ruined});	// invert index of fort
}

void Game::updateRecord(Piece* piece, bool attack, bool swap) {
	record = Record(piece, attack, swap);
	sendb.push(NetCode::record, {uint8(piece - pieces.own.data()), attack, swap});
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}

vector<Tile*> Game::mergeTiles(vector<Tile*> a, const vector<Tile*>& b) {
	pdift osiz = pdift(a.size());
	for (Tile* it : b)
		if (vector<Tile*>::iterator end = a.begin() + osiz; std::find(a.begin(), end, it) == end)
			a.push_back(it);
	return a;
}
