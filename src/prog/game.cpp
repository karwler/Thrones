#include "engine/world.h"

// RECORD

Game::Record::Record(Piece* piece, bool attack, bool swap) :
	piece(piece),
	attack(attack),
	swap(swap)
{}

// GAME

const vec3 Game::screenPosUp(0.f, 1.3f, -BoardObject::halfSize);
const vec3 Game::screenPosDown(0.f, -1.4f, -BoardObject::halfSize);

Game::Game() :
	gridat({}, { vec3() }, { vec3(0.f, 1.f, 0.f) }, {}),
	bgrid(vec3(0.f), vec3(0.f), vec3(1.f), &gridat, nullptr, Tile::colors[uint8(Com::Tile::empty)], Object::INFO_SHOW | Object::INFO_LINES),
	board(vec3(0.f, -BoardObject::upperPoz, 0.f), vec3(0.f), vec3(11.f, 1.f, 11.f), World::scene()->blueprint(Scene::bprRect), nullptr, vec4(0.166f, 0.068f, 0.019f, 1.f), Object::INFO_FILL),
	screen(screenPosUp, vec3(90.f, 0.f, 0.f), vec3(9.4f, 1.f, 2.6f), World::scene()->blueprint(Scene::bprRect), nullptr, vec4(0.2f, 0.13f, 0.062f, 1.f), Object::INFO_FILL),
	tiles(config),
	pieces(config)
{}

void Game::conhost(const Com::Config& cfg) {
	config = cfg;	// don't mess with the tiles or pieces before initObjects() gets called
	connect(new NetcpHost);
}

void Game::tick() {
	if (netcp)
		netcp->tick();
}

void Game::connect(Netcp* net) {
	try {
		netcp.reset(net);
		netcp->connect();
		World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventConnectCancel, "Cancel"));
	} catch (const char* err) {
		netcp.reset();
		World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Game::disconnect(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventExitGame));
}

void Game::processCode(Com::Code code, const uint8* data) {
	switch (code) {
	case Com::Code::full:
		disconnect("Server full");
		break;
	case Com::Code::setup:
		config.fromComData(data);
		World::program()->eventOpenSetup();
		break;
	case Com::Code::tiles:
		for (uint16 i = 0; i < config.numTiles; i++)
			tiles.ene(i)->setType(decompressTile(data, i));
		static_cast<ProgSetup*>(World::state())->rcvMidBuffer.resize(config.homeWidth);
		for (uint16 i = 0; i < config.homeWidth; i++)
			static_cast<ProgSetup*>(World::state())->rcvMidBuffer[i] = decompressTile(data + config.numTiles / 2, i);	// TODO: this is inaccurate
		break;
	case Com::Code::pieces:
		for (uint16 i = 0; i < config.numPieces; i++)
			pieces.ene(i)->pos = gtop(idToPos(reinterpret_cast<const uint16*>(data)[i]), BoardObject::upperPoz);

		if (ProgSetup* ps = static_cast<ProgSetup*>(World::state()); ps->getStage() == ProgSetup::Stage::ready)
			World::program()->eventOpenMatch();
		else {
			ps->enemyReady = true;
			ps->message->setText(ps->getStage() == ProgSetup::Stage::pieces ? "Opponent ready" : "Hurry the fuck up!");
		}
		break;
	case Com::Code::move:
		pieces.ene(reinterpret_cast<const uint16*>(data)[0])->pos = gtop(idToPos(reinterpret_cast<const uint16*>(data)[1]), BoardObject::upperPoz);
		break;
	case Com::Code::kill:
		if (data[0])
			pieces.ene(*reinterpret_cast<const uint16*>(data + 1))->disable();
		else
			pieces.own(*reinterpret_cast<const uint16*>(data + 1))->disable();
		break;
	case Com::Code::breach:
		(tiles.begin() + *reinterpret_cast<const uint16*>(data + 1))->setBreached(data[0]);
		break;
	case Com::Code::favor:
		(tiles.begin() + *reinterpret_cast<const uint16*>(data + 1))->favored = Tile::Favor(data[0]);
		break;
	case Com::Code::record:
		record = Record(pieces.ene(*reinterpret_cast<const uint16*>(data)), data[2], data[3]);
		myTurn = true;
		prepareTurn();
		break;
	case Com::Code::win:
		disconnect(data[0] ? messageLoose : messageWin);
		break;
	default:
		std::cerr << "invalid net code " << uint(code) << std::endl;
	}
}

vector<Object*> Game::initObjects() {
	tiles.update(config);
	pieces.update(config);
	setBgrid();

	// prepare objects for setup
	setTiles(tiles.ene(), -int16(config.homeHeight), nullptr, Object::INFO_TEXTURE);
	setMidTiles();
	setTiles(tiles.own(), 1, &Program::eventPlaceTileC, Object::INFO_TEXTURE | Object::INFO_RAYCAST);
	setPieces(pieces.own(), &Program::eventClearPiece, &Program::eventMovePiece, Object::INFO_TEXTURE, Object::defaultColor);
	setPieces(pieces.ene(), nullptr, nullptr, Object::INFO_TEXTURE, Piece::enemyColor);

	// collect array of references to all objects
	initializer_list<Object*> others = { &board, &bgrid, &screen };
	sizet oi = others.size();
	vector<Object*> objs(others.size() + config.boardSize + config.piecesSize);
	std::copy(others.begin(), others.end(), objs.begin());
	setObjectAddrs(tiles.begin(), config.boardSize, objs, oi);
	setObjectAddrs(pieces.begin(), config.piecesSize, objs, oi);
	return objs;
}

void Game::uninitObjects() {
	setTilesInteract(tiles.begin(), config.boardSize, false);
	setPiecesInteract(pieces.begin(), config.piecesSize, false);
}

void Game::prepareMatch() {
	// set interactivity and reassign callback events
	for (Tile& it : tiles) {
		it.setModeByInteract(myTurn);
		it.setClcall(nullptr);
		it.setCrcall(nullptr);
		it.setUlcall(nullptr);
	}
	for (Piece* it = pieces.own(); it != pieces.ene(); it++) {
		it->setModeByInteract(myTurn);
		it->setClcall(nullptr);
		it->setCrcall(nullptr);
		it->setUlcall(&Program::eventMove);
		it->setUrcall(it->canFire() ? &Program::eventFire : nullptr);
	}
	for (Piece* it = pieces.ene(); it != pieces.end(); it++) {
		it->mode |= Object::INFO_SHOW;
		it->setModeByInteract(myTurn);
	}

	// check for favor
	Piece* throne = getOwnPieces(Com::Piece::throne);
	if (vec2s pos = ptog(throne->pos); getTile(pos)->getType() == Com::Tile::fortress) {
		throne->lastFortress = posToId(pos);
		favorCount = favorTotal = 1;
	} else
		favorCount = favorTotal = 0;

	// rearange middle tiles
	vector<Com::Tile> mid(config.homeWidth);
	for (uint16 i = 0; i < config.homeWidth; i++)
		mid[i] = tiles.mid(i)->getType();
	rearangeMiddle(mid.data(), static_cast<ProgSetup*>(World::state())->rcvMidBuffer.data());
	for (uint16 i = 0; i < config.homeWidth; i++)
		tiles.mid(i)->setType(mid[i]);
}

void Game::rearangeMiddle(Com::Tile* mid, Com::Tile* buf) {
	if (!myTurn)
		std::swap(mid, buf);
	bool fwd = myTurn == config.shiftLeft;
	for (uint16 x, i = fwd ? 0 : config.homeWidth - 1, fm = btom<uint16>(fwd), rm = btom<uint16>(!fwd); i < config.homeWidth; i += fm)
		if (buf[i] != Com::Tile::empty) {
			if (mid[i] == Com::Tile::empty)
				mid[i] = buf[i];
			else {
				if (config.shiftNear) {
					for (x = i + rm; x < config.homeWidth && mid[x] != Com::Tile::empty; x += rm);
					if (x >= config.homeWidth)
						for (x = i + fm; x < config.homeWidth && mid[x] != Com::Tile::empty; x += fm);
				} else
					for (x = fwd ? 0 : config.homeWidth - 1; x < config.homeWidth && mid[x] != Com::Tile::empty; x += fm);
				mid[x] = buf[i];
			}
		}
	*std::find(mid, mid + config.homeWidth, Com::Tile::empty) = Com::Tile::fortress;
	if (!myTurn)
		std::copy(mid, mid + config.homeWidth, buf);
}

void Game::checkOwnTiles() const {
	uint16 empties = 0;
	for (int16 y = 1; y <= config.homeHeight; y++) {
		// collect information and check if the fortress isn't touching a border
		uint8 cnt[uint8(Com::Tile::fortress)] = { 0, 0, 0, 0 };
		for (int16 x = 0; x < config.homeWidth; x++) {
			if (Com::Tile type = tiles.mid(y * config.homeWidth + x)->getType(); type < Com::Tile::fortress)
				cnt[uint8(type)]++;
			else if (vec2s pos(x, y); outRange(pos, vec2s(1), vec2s(config.homeWidth - 2, config.homeHeight - 1)))
				throw firstUpper(Com::tileNames[uint8(Com::Tile::fortress)]) + " at " + pos.toString("|") + " not allowed";
			else
				empties++;
		}
		// check diversity in each row
		for (uint8 i = 0; i < uint8(Com::Tile::fortress); i++)
			if (!cnt[i])
				throw firstUpper(Com::tileNames[i]) + " missing in row " + to_string(y);
	}
	if (empties > 1)
		throw "Not all tiles were placed";
}

void Game::checkMidTiles() const {
	// collect information
	uint8 cnt[uint8(Com::Tile::fortress)] = { 0, 0, 0, 0 };
	for (uint16 i = 0; i < config.homeWidth; i++)
		if (Com::Tile type = tiles.mid(i)->getType(); type < Com::Tile::fortress)
			cnt[uint8(type)]++;
	// check if all pieces except for dragon were placed
	for (uint8 i = 0; i < uint8(Com::Tile::fortress); i++)
		if (!cnt[i])
			throw firstUpper(Com::tileNames[i]) + " wasn't placed";
}

void Game::checkOwnPieces() const {
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (!it->active() && it->getType() != Com::Piece::dragon)
			throw firstUpper(Com::pieceNames[uint8(it->getType())]) + " wasn't placed";
}

vector<uint8> Game::countTiles(const Tile* tiles, uint16 num, vector<uint8> cnt) {
	for (uint16 i = 0; i < num; i++)
		if (uint8(tiles[i].getType()) < cnt.size())
			cnt[uint8(tiles[i].getType())]--;
	return cnt;
}

vector<uint8> Game::countOwnPieces() const {
	vector<uint8> cnt(config.pieceAmounts.begin(), config.pieceAmounts.end());
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (inRange(ptog(it->pos), vec2s(0, 1), vec2s(config.homeWidth - 1, config.homeHeight)))
			cnt[uint8(it->getType())]--;
	return cnt;
}

void Game::fillInFortress() {
	if (Tile* fit = std::find_if(tiles.own(), tiles.end(), [](const Tile& it) -> bool { return it.getType() == Com::Tile::empty; }); fit != tiles.end())
		fit->setType(Com::Tile::fortress);
}

void Game::takeOutFortress() {
	if (Tile* fit = std::find_if(tiles.own(), tiles.end(), [](const Tile& it) -> bool { return it.getType() == Com::Tile::fortress; }); fit != tiles.end())
		fit->setType(Com::Tile::empty);
}

void Game::pieceMove(Piece* piece, vec2s pos, Piece* occupant) {
	// check attack, move and survival conditions
	vec2s spos = ptog(piece->pos);
	bool attacking = occupant && !isOwnPiece(occupant);
	if (!checkMove(piece, spos, occupant, pos, attacking))
		return;
	if (!survivalCheck(piece, spos, pos, attacking)) {
		failSurvivalCheck(piece->getType() != Com::Piece::throne ? piece : nullptr);
		return;
	}

	// handle movement
	if (attacking) {	// kill or breach
		if (Tile* til = getTile(pos); piece->getType() != Com::Piece::throne && til->isUnbreachedFortress())	// breach fortress
			updateFortress(til, true);
		else {	// execute attack
			removePiece(occupant);
			placePiece(piece, pos);
		}
		record = Record(piece, true, false);
	} else if (occupant) {	// switch ally
		placePiece(occupant, spos);
		placePiece(piece, pos);
		record = Record(piece, false, true);
	} else {				// regular move
		if (Tile* til = getTile(spos); til->isBreachedFortress())
			updateFortress(til, false);	// restore fortress
		placePiece(piece, pos);
		record = Record(piece, false, false);
	}
	if (!checkWin())
		endTurn();
}

void Game::pieceFire(Piece* killer, vec2s pos, Piece* piece) {
	vec2s kpos = ptog(killer->pos);
	if (!checkFire(killer, kpos, piece, pos))	// check rules
		return;
	if (!survivalCheck(killer, kpos, pos, false)) {
		failSurvivalCheck(nullptr);
		return;
	}

	if (Tile* til = getTile(pos); til->isUnbreachedFortress()) {	// breach fortress
		updateFortress(til, true);
		record = Record(killer, true, false);
		endTurn();
	} else {	// regular fire
		if (til->isBreachedFortress())
			updateFortress(til, false);	// restore fortress
		removePiece(piece);
		record = Record(killer, true, false);
		if (!checkWin())
			endTurn();
	}
}

void Game::placeFavor(vec2s pos) {
	if (Tile* til = getTile(pos); til->getType() < Com::Tile::fortress && til->favored == Tile::Favor::none) {
		if (ProgMatch* pm = static_cast<ProgMatch*>(World::state()); --favorCount || favorTotal < favorLimit)
			pm->setIconOn(false, favorCount);
		else
			pm->deleteIcon(true);
		updateFavored(til, true);
	}
}

void Game::placeDragon(vec2s pos, Piece* occupant) {
	if (Tile* til = getTile(pos); til->getType() == Com::Tile::fortress && isHomeTile(til)) {		// tile needs to be a home fortress
		static_cast<ProgMatch*>(World::state())->deleteIcon(false);
		if (occupant)
			removePiece(occupant);
		placePiece(getOwnPieces(Com::Piece::dragon), pos);
		checkWin();
	}
}

void Game::setBgrid() {
	sizet i = 0;
	gridat.verts.resize((config.homeWidth + 1) * 4);
	for (int16 y = -int16(config.homeHeight); y <= config.homeHeight + 1; y += config.homeWidth)
		for (int16 x = 0; x <= config.homeWidth; x++)
			gridat.verts[i++] = gtop(vec2s(x, y), BoardObject::upperPoz) + vec3(-BoardObject::halfSize, 0.f, -BoardObject::halfSize);
	for (int16 x = 0; x <= config.homeWidth; x += config.homeWidth)
		for (int16 y = -int16(config.homeHeight); y <= config.homeHeight + 1; y++)
			gridat.verts[i++] = gtop(vec2s(x, y), BoardObject::upperPoz) + vec3(-BoardObject::halfSize, 0.f, -BoardObject::halfSize);

	i = 0;
	gridat.elems.resize((config.homeWidth + 1) * 4);
	for (uint16 ofs = 0; ofs <= uint16(gridat.verts.size()) / 2; ofs += uint16(gridat.verts.size()) / 2)
		for (uint16 c = 0; c <= config.homeWidth; c++) {
			gridat.elems[i++] = Vertex(ofs + c, 0, 0);
			gridat.elems[i++] = Vertex(ofs + c + config.homeWidth + 1, 0, 0);
		}
}

void Game::setTiles(Tile* tiles, int16 yofs, OCall lcall, Object::Info mode) {
	sizet id = 0;
	for (int16 y = yofs, yend = config.homeHeight + yofs; y < yend; y++)
		for (int16 x = 0; x < config.homeWidth; x++)
			tiles[id++] = Tile(gtop(vec2s(x, y), 0.f), Com::Tile::empty, lcall, nullptr, nullptr, nullptr, mode);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < config.homeWidth; i++)
		*tiles.mid(i) = Tile(gtop(vec2s(i, 0), 0.f), Com::Tile::empty, &Program::eventPlaceTileC, &Program::eventClearTile, &Program::eventMoveTile, nullptr, Object::INFO_TEXTURE);
}

void Game::setPieces(Piece* pieces, OCall rcall, OCall ucall, Object::Info mode, const vec4& color) {
	for (uint16 i = 0, t = 0, c = 0; i < config.numPieces; i++) {
		pieces[i] = Piece(gtop(INT16_MIN, BoardObject::upperPoz), Com::Piece(t), nullptr, rcall, ucall, nullptr, mode, color);
		if (++c >= config.pieceAmounts[t]) {
			c = 0;
			t++;
		}
	}
}

void Game::setOwnTilesInteract(Tile::Interactivity lvl) {
	for (Tile* it = tiles.own(); it != tiles.end(); it++)
		it->setCalls(lvl);
}

void Game::setTilesInteract(Tile* tiles, uint16 num, bool on) {
	for (uint16 i = 0; i < num; i++)
		tiles[i].setModeByInteract(on);
}

void Game::setPiecesInteract(Piece* pieces, uint16 num, bool on) {
	for (uint16 i = 0; i < num; i++)
		pieces[i].setModeByInteract(on);
}

void Game::setPiecesVisible(Piece* pieces, bool on) {
	for (uint16 i = 0; i < config.numPieces; i++)
		pieces[i].setModeByOn(on);
}

Piece* Game::getPieces(Piece* pieces, Com::Piece type) {
	for (uint8 t = 0; t < uint8(type); t++)
		pieces += config.pieceAmounts[t];
	return pieces;
}

Piece* Game::findPiece(vec2s pos) {
	Piece* pce = std::find_if(pieces.begin(), pieces.end(), [this, pos](const Piece& it) -> bool { return ptog(it.pos) == pos; });
	return pce != pieces.end() ? pce : nullptr;
}

bool Game::checkMove(Piece* piece, vec2s pos, Piece* occupant, vec2s dst, bool attacking) {
	if (pos == dst)
		return false;

	Tile* dtil = getTile(pos);
	if (attacking) {
		if (!checkAttack(piece, occupant, dtil) || firstTurn)
			return false;
		if (dtil->isUnbreachedFortress())
			return checkMoveBySingle(pos, dst);
	} else if (occupant)
		return piece->getType() != occupant->getType() ? checkMoveBySingle(pos, dst) : false;

	switch (piece->getType()) {
	case Com::Piece::spearman:
		return dtil->getType() == Com::Tile::water ? checkMoveByType(pos, dst) : checkMoveBySingle(pos, dst);
	case Com::Piece::lancer:
		return dtil->getType() == Com::Tile::plains && !attacking ? checkMoveByType(pos, dst) : checkMoveBySingle(pos, dst);
	case Com::Piece::dragon:
		return !attacking ? Dijkstra::travelDist(posToId(pos), config.homeWidth, config.boardSize, spaceAvailible)[posToId(dst)] <= 4 : checkMoveBySingle(pos, dst);	// 4 being the max distance of a dragon
	}
	return checkMoveBySingle(pos, dst);
}

bool Game::checkMoveBySingle(vec2s pos, vec2s dst) {
	uint16 pi = posToId(pos), di = posToId(dst);
	for (uint16 (*mov)(uint16, uint16) : Com::adjacentFull)
		if (uint16 ni = mov(pi, config.homeWidth); ni < config.boardSize && ni == di)
			return true;
	return false;
}

bool Game::checkMoveByArea(vec2s pos, vec2s dst, int16 dist) {
	vec2s lo = clampLow(pos - dist, vec2s(0, -int16(config.homeHeight)));
	vec2s hi = clampHigh(pos + dist, vec2s(config.homeWidth - 1, config.homeHeight));
	return dst.x >= lo.x && dst.x <= hi.x && dst.y >= lo.y && dst.y <= hi.y;
}

bool Game::spaceAvailible(uint16 pos) {
	Piece* occ = World::game()->findPiece(World::game()->idToPos(pos));
	return !occ || World::game()->isOwnPiece(occ) || (occ->getType() != Com::Piece::dragon && !occ->canFire());
}

bool Game::checkMoveByType(vec2s pos, vec2s dst) {
	vector<bool> visited(config.boardSize, 0);
	return checkAdjacentTilesByType(posToId(pos), posToId(dst), visited, getTile(pos)->getType()) || checkMoveBySingle(pos, dst);
}

bool Game::checkAdjacentTilesByType(uint16 pos, uint16 dst, vector<bool>& visited, Com::Tile type) const {
	visited[pos] = true;
	if (pos == dst)
		return true;

	for (uint16 (*mov)(uint16, uint16) : Com::adjacentFull)
		if (uint16 ni = mov(pos, config.homeWidth); ni < config.boardSize && tiles[ni].getType() == type && !visited[ni] && checkAdjacentTilesByType(ni, dst, visited, type))
			return true;
	return false;
}

bool Game::checkFire(Piece* killer, vec2s pos, Piece* victim, vec2s dst) {
	if (!killer->canFire() || pos == dst || firstTurn)
		return false;
	if (Tile* dtil = getTile(dst); dtil->getType() != Com::Tile::fortress || dtil->isBreachedFortress()) {
		if (!checkAttack(killer, victim, dtil))
			return false;
		if (victim->getType() == Com::Piece::ranger && dtil->getType() == Com::Tile::mountain)
			return false;
		if (killer->getType() == Com::Piece::crossbowman && (dtil->getType() == Com::Tile::forest || dtil->getType() == Com::Tile::mountain))
			return false;
		if (killer->getType() == Com::Piece::catapult && dtil->getType() == Com::Tile::forest)
			return false;
	}
	return checkTilesByDistance(pos, dst, int16(killer->getType()) - int16(Com::Piece::crossbowman) + 1);
}

bool Game::checkTilesByDistance(vec2s pos, vec2s dst, int16 dist) {
	if (dst.x == pos.x)
		return dst.y == pos.y - dist || dst.y == pos.y + dist;
	if (dst.y == pos.y)
		return dst.x == pos.x - dist || dst.x == pos.x + dist;
	return false;
}

bool Game::checkAttack(Piece* killer, Piece* victim, Tile* dtil) {
	if (dtil->isUnbreachedFortress())
		return true;
	if (!victim || isOwnPiece(victim))
		return false;
	if (killer->getType() == Com::Piece::throne)
		return true;

	switch (victim->getType()) {
	case Com::Piece::warhorse:
		return victim != record.piece || !(record.attack || record.swap);
	case Com::Piece::elephant:
		return dtil->getType() != Com::Tile::plains || killer->getType() == Com::Piece::dragon;
	case Com::Piece::dragon:
		return dtil->getType() != Com::Tile::forest;
	}
	return true;
}

bool Game::survivalCheck(Piece* piece, vec2s spos, vec2s dpos, bool attacking) {
	Com::Tile src = getTile(spos)->getType();
	Com::Tile dst = attacking ? getTile(dpos)->getType() : Com::Tile::empty;	// dst only relevant when attacking
	if ((src != Com::Tile::mountain && src != Com::Tile::water && dst != Com::Tile::mountain && dst != Com::Tile::water) || piece->getType() == Com::Piece::dragon)
		return true;
	if ((piece->getType() == Com::Piece::ranger && src != Com::Tile::water && dst == Com::Tile::water) || (piece->getType() == Com::Piece::spearman && src != Com::Tile::mountain && dst == Com::Tile::mountain))
		return true;
	return netcp->random();
}

void Game::failSurvivalCheck(Piece* piece) {
	if (piece) {
		removePiece(piece);
		if (!checkWin()) {
			netcp->sendData();
			static_cast<ProgMatch*>(World::state())->message->setText("Failed survival check");
		}
	} else {
		endTurn();
		static_cast<ProgMatch*>(World::state())->message->setText("Failed action");
	}
}

bool Game::checkWin() {
	if (checkThroneWin(getOwnPieces(Com::Piece::throne)))	// no need to check if enemy throne has occupied own fortress
		return doWin(false);
	return checkThroneWin(getPieces(pieces.ene(), Com::Piece::throne)) || checkFortressWin() ? doWin(true) : false;
}

bool Game::checkThroneWin(Piece* thrones) {
	if (uint16 c = config.winThrone)
		for (uint16 i = 0; i < config.pieceAmounts[uint8(Com::Piece::throne)]; i++)
			if (!thrones[i].active() && !--c)
				return true;
	return false;
}

bool Game::checkFortressWin() {
	if (uint16 cnt = config.winFortress)
		for (Tile* tit = tiles.ene(); tit != tiles.mid(); tit++)
			if (Piece* pit = pieces.own(); tit->getType() == Com::Tile::fortress)
				for (uint8 pi = 0; pi < config.capturers.size(); pit += config.pieceAmounts[pi++])	// kill me
					if (config.capturers[pi])
						for (uint16 i = 0; i < config.pieceAmounts[pi]; i++)
							if (tileId(tit) == posToId(ptog(pit[i].pos)) && !--cnt)
								return true;
	return false;
}

bool Game::doWin(bool win) {
	netcp->push(vector<uint8>({ uint8(Com::Code::win), uint8(win) }));
	endTurn();
	disconnect(win ? messageWin : messageLoose);
	return true;
}

void Game::prepareTurn() {
	setOwnTilesInteract(myTurn ? Tile::Interactivity::raycast : Tile::Interactivity::none);
	setMidTilesInteract(myTurn);
	setTilesInteract(tiles.ene(), config.numTiles, myTurn);
	setPiecesInteract(pieces.begin(), config.piecesSize, myTurn);

	ProgMatch* pm = static_cast<ProgMatch*>(World::state());
	pm->message->setText(myTurn ? messageTurnGo : messageTurnWait);
	pm->setIconOn(myTurn, favorCount);
	pm->setIconOn(myTurn);
}

void Game::endTurn() {
	firstTurn = myTurn = false;
	prepareTurn();

	netcp->push(Com::Code::record);
	netcp->push(uint16(record.piece - pieces.own()));
	netcp->push(vector<uint8>({ uint8(record.attack), uint8(record.swap) }));
	netcp->sendData();
}

void Game::sendSetup() {
	// send tiles
	netcp->sendSize = config.tileCompressionEnd();
	std::fill_n(netcp->sendb, netcp->sendSize, 0);
	netcp->sendb[0] = uint8(Com::Code::tiles);
	for (uint16 i = 0; i < config.extraSize; i++) {
		uint16 e = config.extraSize - i - 1;
		netcp->sendb[i/2+1] |= uint8(tiles.mid(e)->getType()) << (e % 2 * 4);
	}
	netcp->sendData();

	// send pieces
	netcp->sendSize = config.dataSize(Com::Code::pieces);
	netcp->sendb[0] = uint8(Com::Code::pieces);
	for (uint16 i = 0; i < config.numPieces; i++)
		reinterpret_cast<uint16*>(netcp->sendb + 1)[i] = invertId(posToId(ptog(pieces.own(i)->pos)));
	netcp->sendData();
}

void Game::placePiece(Piece* piece, vec2s pos) {
	if (piece->getType() == Com::Piece::throne && getTile(pos)->getType() == Com::Tile::fortress && favorTotal < favorLimit)
		if (uint16 fid = posToId(pos); piece->lastFortress != fid) {
			piece->lastFortress = fid;
			favorCount++;
			favorTotal++;
		}
	piece->pos = gtop(pos, BoardObject::upperPoz);
	netcp->push(Com::Code::move);
	netcp->push(vector<uint16>({ uint16(piece - pieces.own()), invertId(posToId(pos)) }));
}

void Game::removePiece(Piece* piece) {
	piece->disable();
	bool mine = isOwnPiece(piece);
	netcp->push(vector<uint8>({ uint8(Com::Code::kill), uint8(mine) }));
	netcp->push(uint16(piece - (mine ? pieces.own() : pieces.ene())));
}

void Game::updateFortress(Tile* fort, bool breached) {
	fort->setBreached(breached);
	netcp->push(vector<uint8>({ uint8(Com::Code::breach), uint8(breached) }));
	netcp->push(inverseTileId(fort));
}

void Game::updateFavored(Tile* tile, bool favored) {
	tile->favored = Tile::Favor(favored);
	netcp->push(vector<uint8>({ uint8(Com::Code::favor), uint8(favored ? Tile::Favor::enemy : Tile::Favor::none) }));
	netcp->push(inverseTileId(tile));
}
#ifdef DEBUG
vector<Object*> Game::initDummyObjects() {
	vector<Object*> objs = initObjects();

	// columns 0 p, 2 f, 4 m, 6 w
	for (int16 x = 0; x < 8; x += 2)
		for (int16 y = 1; y <= config.homeHeight; y++)
			getTile(vec2s(x, y))->setType(Com::Tile(x / 2));

	// columns 1 f, 3 fftp
	for (int16 y = 1; y <= config.homeHeight; y++)
		getTile(vec2s(1, y))->setType(Com::Tile::forest);
	for (int16 y = 1; y <= 2; y++)
		getTile(vec2s(3, y))->setType(Com::Tile::forest);
	getTile(vec2s(3, 4))->setType(Com::Tile::plains);

	// columns 5 pm, 7 pw, 8 p
	getTile(vec2s(5, 1))->setType(Com::Tile::plains);
	for (int16 y = 2; y <= config.homeHeight; y++)
		getTile(vec2s(5, y))->setType(Com::Tile::mountain);
	getTile(vec2s(7, 1))->setType(Com::Tile::plains);
	for (int16 y = 2; y <= config.homeHeight; y++)
		getTile(vec2s(7, y))->setType(Com::Tile::water);
	for (int16 y = 1; y <= config.homeHeight; y++)
		getTile(vec2s(8, y))->setType(Com::Tile::plains);

	// middle 0 p, 2 f, 4 m, 6 w
	for (int16 i = 0; i < 8; i += 2)
		getTile(vec2s(i, 0))->setType(Com::Tile(i / 2));

	// pieces
	for (uint16 i = 0; i < config.numPieces * 2; i += 2)
		pieces.own(i/2)->pos = gtop(vec2s(i % config.homeWidth, 1 + i / config.homeWidth), BoardObject::upperPoz);
	return objs;
}
#endif
