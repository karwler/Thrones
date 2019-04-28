#include "engine/world.h"

// DATA BATCH

void DataBatch::push(Com::Code code, const vector<uint8>& info) {
	push(code);
	memcpy(data + size, info.data(), info.size());
	size += uint8(info.size());	// there should probably be some exception handling here, but I don't feel like it
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
		if (int len = SDLNet_TCP_Recv(socket, recvb, Com::recvSize); len > 0)
			for (int i = 0; i < len; i += (this->*conn)(recvb + i));
		else
			disconnectMessage("Connection lost");
	}
}

uint8 Game::connectionWait(const uint8* data) {
	switch (Com::Code(*data)) {
	case Com::Code::full:
		disconnectMessage("Server full");
		return 1;
	case Com::Code::setup:
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
	switch (Com::Code(*data)) {
	case Com::Code::ready: {
		uint8 bi = 1;
		for (uint8 i = 0; i < Com::numTiles; i++, bi++)
			tiles.ene[i].setType(Tile::Type(data[bi]));
		for (int8 i = 0; i < Com::boardLength; i++, bi++)
			static_cast<ProgSetup*>(World::state())->rcvMidBuffer[sizet(i)] = Tile::Type(data[bi]);
		for (uint8 i = 0; i < Com::numPieces; i++, bi += 2)
			pieces.ene[i].setPos(*reinterpret_cast<const vec2b*>(data + bi));

		conn = &Game::connectionMatch;
		if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->getStage() == ProgSetup::Stage::ready)
			World::program()->eventOpenMatch();
		else {
			ps->enemyReady = true;
			ps->message->setText(ps->getStage() == ProgSetup::Stage::pieces ? "Opponent ready" : "Hurry the fuck up!");
		}
		return bi; }
	default:
		printInvalidCode(*data);
	}
	return 1;
}

uint8 Game::connectionMatch(const uint8* data) {
	switch (Com::Code(*data)) {
	case Com::Code::move:
		pieces.ene[data[1]].setPos(*reinterpret_cast<const vec2b*>(data + 2));
		return 4;
	case Com::Code::kill: {
		(data[1] ? pieces.ene : pieces.own)[data[2]].disable();
		return 3; }
	case Com::Code::ruin:
		(tiles.begin() + data[1])->ruined = data[2];
		return 3;
	case Com::Code::record:
		record = Record(pieces.ene + data[1], data[2], data[3]);
		myTurn = true;
		prepareTurn();
		return 4;
	case Com::Code::win:
		disconnectMessage("You lose");
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
	setTiles(tiles.ene, -Com::homeHeight, nullptr, Object::INFO_LINES);
	setMidTiles();
	setTiles(tiles.own, 1, &Program::eventPlaceTileC, Object::INFO_LINES | Object::INFO_RAYCAST);
	setPieces(pieces.own, &Program::eventClearPiece, &Program::eventMovePiece, Object::INFO_TEXTURE);
	setPieces(pieces.ene, nullptr, nullptr, Object::INFO_TEXTURE);
	setScreen();
	setBoard();

	// collect array of references to all objects
	sizet oi = 0;
	vector<Object*> objs(Com::boardSize + Com::piecesSize + 2);
	setObjectAddrs(tiles.begin(), Com::boardSize, objs, oi);
	setObjectAddrs(pieces.begin(), Com::piecesSize, objs, oi);
	objs[oi] = &screen;
	objs[oi+1] = &board;
	return objs;
}

void Game::prepareMatch() {
	firstTurn = myTurn;

	// set interactivity and reassign callback events
	for (Tile& it : tiles) {
		it.mode = getTileInfoInteract(it.mode, myTurn);
		it.setClcall(nullptr);
		it.setUlcall(nullptr);
	}
	for (Piece& it : pieces.own) {
		it.mode = getPieceInfoInteract(it.mode, myTurn);
		it.setClcall(nullptr);
		it.setUlcall(&Program::eventMove);
		it.setUrcall(&Program::eventAttack);
	}
	setPiecesInteract(pieces.ene, myTurn);

	// rearange middle tiles
	if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); myTurn) {
		for (uint8 i = 0; i < Com::boardLength; i++)
			if ((tiles.mid[i].getType() != Tile::Type::empty) && (ps->rcvMidBuffer[i] != Tile::Type::empty))
				std::find_if(tiles.mid, tiles.mid + Com::boardLength, [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(ps->rcvMidBuffer[i]);
	} else
		for (uint8 i = 0; i < Com::boardLength; i++)
			if (ps->rcvMidBuffer[i] != Tile::Type::empty) {
				if (tiles.mid[i].getType() != Tile::Type::empty)
					tiles.mid[sizet(std::find(ps->rcvMidBuffer.begin(), ps->rcvMidBuffer.end(), Tile::Type::empty) - ps->rcvMidBuffer.begin())].setType(tiles.mid[i].getType());
				tiles.mid[i].setType(ps->rcvMidBuffer[i]);
			}
	std::find_if(tiles.mid, tiles.mid + Com::boardLength, [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; })->setType(Tile::Type::fortress);
}

string Game::checkOwnTiles() const {
	uint8 empties = 0;
	for (int8 y = 1; y <= Com::homeHeight; y++) {
		// collect information and check if the fortress isn't touching a border
		uint8 cnt[uint8(Tile::Type::fortress)] = { 0, 0, 0, 0 };
		for (int8 x = 0; x < Com::boardLength; x++) {
			if (Tile::Type type = tiles.mid[uint8(y * Com::boardLength + x)].getType(); type < Tile::Type::fortress)
				cnt[uint8(type)]++;
			else if (vec2b pos(x, y); outRange(pos, vec2b(1), vec2b(Com::boardLength - 2, Com::homeHeight - 1)))
				return firstUpper(Tile::names[uint8(Tile::Type::fortress)]) + " at " + pos.toString('|') + " not allowed";
			else
				empties++;
		}
		// check diversity in each row
		for (uint8 i = 0; i < uint8(Tile::Type::fortress); i++)
			if (!cnt[i])
				return firstUpper(Tile::names[i]) + " missing in row " + to_string(y);
	}
	return empties == 1 ? emptyStr : "Not all tiles were placed";
}

string Game::checkMidTiles() const {
	// collect information
	uint8 cnt[sizet(Tile::Type::fortress)] = { 0, 0, 0, 0 };
	for (int8 i = 0; i < Com::boardLength; i++)
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
		if (outRange(it.getPos(), vec2b(0, Com::boardLength), vec2b(1, Com::homeHeight)) && it.getType() != Piece::Type::dragon)
			return firstUpper(Piece::names[uint8(it.getType())]) + " wasn't placed";
	return emptyStr;
}

void Game::fillInFortress() {
	if (Tile* fit = std::find_if(tiles.own, tiles.own + Com::numTiles, [](const Tile& it) -> bool { return it.getType() == Tile::Type::empty; }); fit != tiles.own + Com::numTiles)
		fit->setType(Tile::Type::fortress);
}

void Game::takeOutFortress() {
	if (Tile* fit = std::find_if(tiles.own, tiles.own + Com::numTiles, [](const Tile& it) -> bool { return it.getType() == Tile::Type::fortress; }); fit != tiles.own + Com::numTiles)
		fit->setType(Tile::Type::empty);
}

void Game::pieceMove(Piece* piece, vec2b pos, Piece* occupant) {
	// check attack, move and survival conditions
	vec2b spos = piece->getPos();
	bool attacking = occupant && !isOwnPiece(occupant);
	if (!checkMove(piece, spos, occupant, pos, attacking))
		return;

	if (!survivalCheck(piece, spos)) {
		removePiece(piece);
		sendData();
		return;
	}

	// handle movement
	if (attacking) {		// kill and/or ruin
		Tile* til = getTile(pos);
		bool ruin = til->getType() == Tile::Type::fortress && !til->ruined;
		if (ruin)	// ruin fortress
			updateFortress(til, true);

		if (!ruin || piece->getType() == Piece::Type::throne) {	// execute attack
			removePiece(occupant);
			record = Record(piece, true, false);
		}
	} else if (occupant) {	// switch ally
		placePiece(occupant, spos);
		record = Record(piece, false, true);
	} else {				// regular move
		if (Tile* til = getTile(spos); til->getType() == Tile::Type::fortress && til->ruined)
			updateFortress(til, false);	// restore fortress
		record = Record(piece, false, false);
	}
	placePiece(piece, pos);

	if (!tryWin(piece, occupant, getTile(pos)))
		endTurn();
}

void Game::pieceFire(Piece* killer, vec2b pos, Piece* piece) {
	vec2b kpos = killer->getPos();
	if (!checkFire(killer, kpos, piece, pos))	// check rules
		return;

	bool succ = survivalCheck(killer, kpos);
	if (Tile* til = getTile(pos); til->getType() == Tile::Type::fortress && !til->ruined) {	// specific behaviour for firing on fortress
		if (succ) {
			updateFortress(til, true);
			record = Record(killer, true, false);
			endTurn();
		} else {
			removePiece(killer);
			sendData();
		}
	} else if (piece && !isOwnPiece(piece)) {	// regular behaviour
		if (succ) {
			removePiece(piece);
			record = Record(killer, true, false);
			if (!tryWin(killer, piece, nullptr))
				endTurn();
		} else {
			removePiece(killer);
			sendData();
		}
	}
}

void Game::placeDragon(vec2b pos, Piece* occupant) {
	// tile needs to be a home fortress
	if (Tile* til = getTile(pos); til->getType() != Tile::Type::fortress || !isHomeTile(til))
		return;

	// get rid of button
	ProgMatch* pm = static_cast<ProgMatch*>(World::state());
	pm->dragonIcon->getParent()->deleteWidget(pm->dragonIcon->getID());

	// kill any piece that might be occupying the tile
	if (occupant)
		removePiece(occupant);

	// place the dragon
	Piece* maid = getOwnPieces(Piece::Type::dragon);
	maid->mode |= Object::INFO_SHOW;
	placePiece(maid, pos);
	endTurn();
}

void Game::setScreen() {
	vector<Vertex> verts = {
		Vertex(vec3(-4.7f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(0.f, 1.f)),
		Vertex(vec3(-4.7f, 2.6f, 0.f), vec3(0.f, 0.f, 1.f), vec2(0.f, 0.f)),
		Vertex(vec3(4.7f, 2.6f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f, 0.f)),
		Vertex(vec3(4.7f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f, 1.f))
	};
	screen = Object(vec3(0.f, 0.f, -0.5f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, { 0.2f, 0.13f, 0.062f, 1.f }, Object::INFO_FILL);
}

void Game::setBoard() {
	vector<Vertex> verts = {
		Vertex(vec3(-5.5f, 0.f, 5.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 1.f)),
		Vertex(vec3(-5.5f, 0.f, -5.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 0.f)),
		Vertex(vec3(5.5f, 0.f, -5.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 0.f)),
		Vertex(vec3(5.5f, 0.f, 5.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 1.f))
	};
	board = Object(vec3(0.f, -0.01f, 0.f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, { 0.166f, 0.068f, 0.019f, 1.f }, Object::INFO_FILL);
}

void Game::setTiles(Tile* tiles, int8 yofs, OCall lcall, Object::Info mode) {
	sizet id = 0;
	for (int8 y = yofs, yend = Com::homeHeight + yofs; y < yend; y++)
		for (int8 x = 0; x < Com::boardLength; x++)
			tiles[id++] = Tile(vec2b(x, y), Tile::Type::empty, lcall, nullptr, nullptr, nullptr, mode);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < Com::boardLength; i++)
		tiles.mid[sizet(i)] = Tile(vec2b(i, 0), Tile::Type::empty, &Program::eventPlaceTileC, &Program::eventClearTile, &Program::eventMoveTile, nullptr, Object::INFO_LINES);
}

void Game::setPieces(Piece* pieces, OCall rcall, OCall ucall, Object::Info mode) {
	for (uint8 i = 0, t = 0, c = 0; i < Com::numPieces; i++) {
		pieces[i] = Piece(INT8_MIN, Piece::Type(t), nullptr, rcall, ucall, nullptr, mode);
		if (++c >= Piece::amounts[t]) {
			c = 0;
			t++;
		}
	}
}

void Game::setTilesInteract(Tile* tiles, sizet num, bool on) {
	for (sizet i = 0; i < num; i++)
		tiles[i].mode = getTileInfoInteract(tiles[i].mode, on);
}

void Game::setPiecesInteract(Piece* pieces, bool on) {
	for (uint8 i = 0; i < Com::numPieces; i++)
		pieces[i].mode = getPieceInfoInteract(pieces[i].mode, on);
}

Piece* Game::getOwnPieces(Piece::Type type) {
	Piece* it = pieces.own;
	for (uint8 t = 0; t < uint8(type); t++)
		it += Piece::amounts[t];
	return it;
}

bool Game::checkMove(Piece* piece, vec2b pos, Piece* occupant, vec2b dst, bool attacking) {
	if (pos == dst)
		return false;

	Tile* dtil = getTile(pos);
	if (attacking) {
		if (!checkAttack(piece, occupant, dtil) || firstTurn)
			return false;
		if (dtil->getType() == Tile::Type::fortress && !dtil->ruined)
			return checkMoveBySingle(pos, dst);
	} else if (occupant && piece->getType() == occupant->getType())
		return false;

	if (piece->getType() == Piece::Type::spearman && dtil->getType() == Tile::Type::water)
		return checkMoveByType(pos, dst);
	if (piece->getType() == Piece::Type::lancer && dtil->getType() == Tile::Type::plains && !attacking)
		return checkMoveByType(pos, dst);
	if (piece->getType() == Piece::Type::dragon && !attacking)
		return checkMoveByArea(piece, pos, dst, 4);
	return checkMoveBySingle(pos, dst);
}

bool Game::checkMoveBySingle(vec2b pos, vec2b dst) {
	uint8 pi = posToGid(pos), di = posToGid(dst);
	for (uint8 (*mov)(uint8) : Com::adjacentFull)
		if (uint8 ni = mov(pi); ni < Com::boardSize && ni == di)
			return true;
	return false;
}

bool Game::checkMoveByArea(Piece* piece, vec2b pos, vec2b dst, uint dist) {
	int cost = Astar(spaceAvailible, piece).travelDist(posToGid(pos), posToGid(dst));
	return uint(cost) <= dist && cost != -1;
}

bool Game::spaceAvailible(uint8 pos, void* data) {
	Piece* occ = World::game()->findPiece(gidToPos(pos));
	return !occ || static_cast<Piece*>(data)->getType() != Piece::Type::dragon || (occ->getType() != Piece::Type::dragon && occ->getType() != Piece::Type::crossbowman && occ->getType() != Piece::Type::catapult && occ->getType() != Piece::Type::trebuchet);
}

bool Game::checkMoveByType(vec2b pos, vec2b dst) {
	bool visited[Com::boardSize];
	memset(visited, 0, Com::boardSize * sizeof(bool));
	return checkAdjacentTilesByType(posToGid(pos), posToGid(dst), visited, getTile(pos)->getType()) || checkMoveBySingle(pos, dst);
}

bool Game::checkAdjacentTilesByType(uint8 pos, uint8 dst, bool* visited, Tile::Type type) {
	visited[pos] = true;
	if (pos == dst)
		return true;

	for (uint8 (*mov)(uint8) : Com::adjacentStraight)
		if (uint8 ni = mov(pos); ni < Com::boardSize && tiles[ni].getType() == type && !visited[ni] && checkAdjacentTilesByType(ni, dst, visited, type))
			return true;
	return false;
}

bool Game::checkFire(Piece* killer, vec2b pos, Piece* victim, vec2b dst) {
	if (pos == dst || firstTurn)
		return false;
	if (Tile* dtil = getTile(dst); dtil->getType() != Tile::Type::fortress) {
		if (!checkAttack(killer, victim, dtil))
			return false;
		if (victim->getType() == Piece::Type::ranger && dtil->getType() == Tile::Type::mountain)
			return false;
		if (killer->getType() == Piece::Type::crossbowman && (dtil->getType() == Tile::Type::forest || dtil->getType() == Tile::Type::mountain))
			return false;
		if (killer->getType() == Piece::Type::catapult && dtil->getType() == Tile::Type::forest)
			return false;
	}
	switch (killer->getType()) {
	case Piece::Type::crossbowman:
		return checkTilesByDistance(pos, dst, 1);
	case Piece::Type::catapult:
		return checkTilesByDistance(pos, dst, 2);
	case Piece::Type::trebuchet:
		return checkTilesByDistance(pos, dst, 3);
	}
	return false;
}

bool Game::checkTilesByDistance(vec2b pos, vec2b dst, int8 dist) {
	if (dst.x == pos.x)
		return dst.y == pos.y - dist || dst.y == pos.y + dist;
	if (dst.y == pos.y)
		return dst.x == pos.x - dist || dst.x == pos.x + dist;
	return false;
}

bool Game::checkAttack(Piece* killer, Piece* victim, Tile* dtil) {
	if (dtil->getType() == Tile::Type::fortress && !dtil->ruined)
		return true;
	if (!victim)
		return false;
	if (killer->getType() == Piece::Type::throne)
		return true;

	if (victim->getType() == Piece::Type::warhorse && victim == record.piece && (record.attack || record.swap))
		return false;
	if (victim->getType() == Piece::Type::elephant && dtil->getType() == Tile::Type::plains && killer->getType() != Piece::Type::dragon)
		return false;
	if (killer->getType() == Piece::Type::dragon && dtil->getType() == Tile::Type::forest)
		return false;
	return true;
}

bool Game::tryWin(Piece* piece, Piece* victim, Tile* dest) {
	if ((victim && !isOwnPiece(victim) && victim->getType() == Piece::Type::throne) || (dest && piece->getType() == Piece::Type::throne && dest->getType() == Tile::Type::fortress && dest < tiles.mid)) {
		sendb.push(Com::Code::win);
		endTurn();
		disconnectMessage("You win");
		return true;
	}
	return false;
}

void Game::prepareTurn() {
	setOwnTilesInteract(myTurn);
	setMidTilesInteract(myTurn);
	setTilesInteract(tiles.ene, Com::numTiles, myTurn);
	setOwnPiecesInteract(myTurn);
	setPiecesInteract(pieces.ene, myTurn);
}

void Game::endTurn() {
	myTurn = false;
	prepareTurn();

	sendb.push(Com::Code::record, { uint8(record.piece - pieces.own), record.attack, record.swap });
	sendData();
}

void Game::sendSetup() {
	sendb.size = 1 + Com::boardLength + Com::numTiles + Com::numPieces * sizeof(vec2b);
	sendb.data[0] = uint8(Com::Code::ready);
	for (sizet i = 0; i < Com::boardLength + Com::numTiles; i++)
		sendb.data[Com::boardLength + Com::numTiles - i] = uint8(tiles.mid[i].getType());
	for (sizet i = 0, e = Com::boardLength + Com::numTiles + 1; i < Com::numPieces; i++)
		*reinterpret_cast<vec2b*>(sendb.data + e + i * sizeof(vec2b)) = pieces.own[i].getPos();
	sendData();
}

void Game::sendData() {
	if (SDLNet_TCP_Send(socket, sendb.data, sendb.size) == sendb.size)
		sendb.size = 0;
	else 
		disconnectMessage(SDLNet_GetError());
}

void Game::placePiece(Piece* piece, vec2b pos) {
	piece->setPos(pos);
	sendb.push(Com::Code::move, { uint8(piece - pieces.own), uint8(Com::boardLength - pos.x - 1), uint8(-pos.y) });	// rotate position 180
}

void Game::removePiece(Piece* piece) {
	piece->disable();
	bool mine = isOwnPiece(piece);
	sendb.push(Com::Code::kill, { uint8(mine), uint8(piece - (mine ? pieces.own : pieces.ene)) });	// index doesn't need to be modified
}

void Game::updateFortress(Tile* fort, bool ruined) {
	fort->ruined = ruined;
	sendb.push(Com::Code::ruin, { uint8(tiles.end() - fort - 1), ruined });	// invert index of fort
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventExitGame));
}
