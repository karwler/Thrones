#include "engine/world.h"
#include <ctime>

// FAVORING STATE

FavorState::FavorState() :
	piece(nullptr),
	use(false)
{}

// RECORD

Record::Record(Piece* actor, Piece* swapper, Action action) :
	actor(actor),
	swapper(swapper),
	action(action)
{}

void Record::update(Piece* doActor, Piece* didSwap, Action didActions) {
	actor = doActor;
	swapper = didSwap ? didSwap : swapper;
	action |= didActions;
}

void Record::updateProtectionColors(bool on) const {
	if (actor)
		updateProtectionColor(actor, on);
	if (swapper)
		updateProtectionColor(swapper, on);
}

// GAME

const array<uint16 (*)(uint16, uint16), Game::adjacentStraight.size()> Game::adjacentStraight = {
	[](uint16 id, uint16 lim) -> uint16 { return id / lim ? id - lim : UINT16_MAX; },			// up
	[](uint16 id, uint16 lim) -> uint16 { return id % lim ? id - 1 : UINT16_MAX; },				// left
	[](uint16 id, uint16 lim) -> uint16 { return id % lim != lim - 1 ? id + 1 : UINT16_MAX; },	// right
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 ? id + lim : UINT16_MAX; }	// down
};

const array<uint16 (*)(uint16, uint16), Game::adjacentFull.size()> Game::adjacentFull = {
	[](uint16 id, uint16 lim) -> uint16 { return id / lim && id % lim ? id - lim - 1 : UINT16_MAX; },						// left up
	[](uint16 id, uint16 lim) -> uint16 { return id / lim ? id - lim : UINT16_MAX; },										// up
	[](uint16 id, uint16 lim) -> uint16 { return id / lim && id % lim != lim - 1  ? id - lim + 1 : UINT16_MAX; },			// right up
	[](uint16 id, uint16 lim) -> uint16 { return id % lim ? id - 1 : UINT16_MAX; },											// left
	[](uint16 id, uint16 lim) -> uint16 { return id % lim != lim - 1 ? id + 1 : UINT16_MAX; },								// right
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 && id % lim ? id + lim - 1 : UINT16_MAX; },			// left down
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 ? id + lim : UINT16_MAX; },							// down
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 && id % lim != lim - 1 ? id + lim + 1 : UINT16_MAX; }	// right down
};

Game::Game() :
	randGen(createRandomEngine()),
	randDist(0, Com::Config::randomLimit - 1),
	ground(vec3(Com::Config::boardWidth / 2.f, -6.f, Com::Config::boardWidth / 2.f), vec3(0.f), vec3(100.f, 1.f, 100.f), World::scene()->mesh("ground"), World::scene()->material("ground"), World::scene()->texture("grass")),
	board(vec3(Com::Config::boardWidth / 2.f, 0.f, 0.f), vec3(0.f), vec3(1.f), World::scene()->mesh("table"), World::scene()->material("board"), World::scene()->texture("rock")),
	bgrid(vec3(0.f), vec3(0.f), vec3(1.f), &gridat, World::scene()->material("grid"), World::scene()->blank()),
	screen(vec3(Com::Config::boardWidth / 2.f, screenYUp, 0.f), vec3(0.f), vec3(1.f), World::scene()->mesh("screen"), World::scene()->material("screen"), World::scene()->texture("wall")),
	ffpad(gtop(svec2(INT16_MIN), 0.001f), vec3(0.f), vec3(config.objectSize, 1.f, config.objectSize), World::scene()->mesh("outline"), World::scene()->material("grid"), World::scene()->blank(), nullptr, false, false),	// show indicates if favor is being used and pos informs tile type
	tiles(config),
	pieces(config)
{}

vector<Object*> Game::initObjects() {
	tiles.update(config);
	pieces.update(config);
	setBgrid();
	screen.setPos(vec3(screen.getPos().x, screen.getPos().y, -(config.objectSize / 2.f)));

	// prepare objects for setup
	setTiles(tiles.ene(), -int16(config.homeSize.y), false);
	setMidTiles();
	setTiles(tiles.own(), 1, true);
	setPieces(pieces.own(), PI, World::scene()->material("ally"));
	setPieces(pieces.ene(), 0.f, World::scene()->material("enemy"));

	// collect array of references to all objects
	array<Object*, 4> others = { &ground, &board, &bgrid, &screen };
	sizet oi = others.size();
	vector<Object*> objs(others.size() + config.boardSize + 1 + config.piecesSize);
	std::copy(others.begin(), others.end(), objs.begin());
	setObjectAddrs(tiles.begin(), config.boardSize, objs, oi);
	objs[oi++] = &ffpad;
	setObjectAddrs(pieces.begin(), config.piecesSize, objs, oi);
	return objs;
}

void Game::uninitObjects() {
	setTilesInteract(tiles.begin(), config.boardSize, Tile::Interact::ignore);
	disableOwnPiecesInteract(false);
}

void Game::prepareMatch() {
	// set interactivity and reassign callback events
	for (Tile* it = tiles.ene(); it != tiles.mid(); it++)
		it->show = true;
	for (Piece* it = pieces.ene(); it != pieces.end(); it++)
		it->show = pieceOnBoard(it);
	ownRec = eneRec = Record();

	// check for favor
	favorCount = 0;
	favorTotal = config.favorLimit;
	for (Piece* throne = getOwnPieces(Com::Piece::throne); throne != pieces.ene() && favorCount < favorTotal; throne++)
		if (svec2 pos = ptog(throne->getPos()); getTile(pos)->getType() == Com::Tile::fortress) {
			throne->lastFortress = posToId(pos);
			favorCount++;
		}

	// rearange middle tiles
	vector<Com::Tile> mid(config.homeSize.x);
	for (uint16 i = 0; i < config.homeSize.x; i++)
		mid[i] = tiles.mid(i)->getType();
	rearangeMiddle(mid.data(), World::state<ProgSetup>()->rcvMidBuffer.data());
	for (uint16 i = 0; i < config.homeSize.x; i++)
		tiles.mid(i)->setType(mid[i]);
}

void Game::rearangeMiddle(Com::Tile* mid, Com::Tile* buf) {
	if (!myTurn)
		std::swap(mid, buf);
	bool fwd = myTurn == config.shiftLeft;
	for (uint16 x, i = fwd ? 0 : config.homeSize.x - 1, fm = btom<uint16>(fwd), rm = btom<uint16>(!fwd); i < config.homeSize.x; i += fm)
		if (buf[i] != Com::Tile::empty) {
			if (mid[i] == Com::Tile::empty)
				mid[i] = buf[i];
			else {
				if (config.shiftNear) {
					for (x = i + rm; x < config.homeSize.x && mid[x] != Com::Tile::empty; x += rm);
					if (x >= config.homeSize.x)
						for (x = i + fm; x < config.homeSize.x && mid[x] != Com::Tile::empty; x += fm);
				} else
					for (x = fwd ? 0 : config.homeSize.x - 1; x < config.homeSize.x && mid[x] != Com::Tile::empty; x += fm);
				mid[x] = buf[i];
			}
		}
	std::replace(mid, mid + config.homeSize.x, Com::Tile::empty, Com::Tile::fortress);
	if (!myTurn)
		std::copy(mid, mid + config.homeSize.x, buf);
}

void Game::checkOwnTiles() const {
	uint16 empties = 0;
	for (int16 y = 1; y <= config.homeSize.y; y++) {
		// collect information and check if the fortress isn't touching a border
		uint8 cnt[uint8(Com::Tile::fortress)] = { 0, 0, 0, 0 };
		for (int16 x = 0; x < config.homeSize.x; x++) {
			if (Com::Tile type = tiles.mid(y * config.homeSize.x + x)->getType(); type < Com::Tile::fortress)
				cnt[uint8(type)]++;
			else if (svec2 pos(x, y); outRange(pos, svec2(1), svec2(config.homeSize.x - 2, config.homeSize.y - 1)))
				throw firstUpper(Com::tileNames[uint8(Com::Tile::fortress)]) + " at " + toStr(pos, "|") + " not allowed";
			else
				empties++;
		}
		// check diversity in each row
		for (uint8 i = 0; i < uint8(Com::Tile::fortress); i++)
			if (!cnt[i] && config.tileAmounts[i])
				throw firstUpper(Com::tileNames[i]) + " missing in row " + toStr(y);
	}
	if (empties > config.tileAmounts[uint8(Com::Tile::fortress)])
		throw string("Not all tiles were placed");
}

void Game::checkMidTiles() const {
	// collect information
	uint8 cnt[uint8(Com::Tile::fortress)] = { 0, 0, 0, 0 };
	for (uint16 i = 0; i < config.homeSize.x; i++)
		if (Com::Tile type = tiles.mid(i)->getType(); type < Com::Tile::fortress)
			cnt[uint8(type)]++;
	// check if all pieces except for dragon were placed
	for (uint8 i = 0; i < uint8(Com::Tile::fortress); i++)
		if (!cnt[i])
			throw firstUpper(Com::tileNames[i]) + " wasn't placed";
}

void Game::checkOwnPieces() const {
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (!it->show && it->getType() != Com::Piece::dragon)
			throw firstUpper(Com::pieceNames[uint8(it->getType())]) + " wasn't placed";
}

vector<uint16> Game::countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt) {
	for (uint16 i = 0; i < num; i++)
		if (uint8(tiles[i].getType()) < cnt.size())
			cnt[uint8(tiles[i].getType())]--;
	return cnt;
}

vector<uint16> Game::countOwnPieces() const {
	vector<uint16> cnt(config.pieceAmounts.begin(), config.pieceAmounts.end());
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (pieceOnHome(it))
			cnt[uint8(it->getType())]--;
	return cnt;
}

void Game::fillInFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); it++)
		if (it->getType() == Com::Tile::empty)
			it->setType(Com::Tile::fortress);
}

void Game::takeOutFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); it++)
		if (it->getType() == Com::Tile::fortress)
			it->setType(Com::Tile::empty);
}

void Game::updateFavorState() {
	if (!favorState.piece || !favorState.use || !favorCount)
		ffpad.show = false;
	else if (Tile* til = getTile(ptog(favorState.piece->getPos())); til->getType() < Com::Tile::fortress) {
		ffpad.setPos(vec3(favorState.piece->getPos().x, ffpad.getPos().y, favorState.piece->getPos().z));
		ffpad.show = true;
	}
}

Com::Tile Game::pollFavor() {
	Com::Tile favor = checkFavor();
	if (ffpad.show) {
		favorState = FavorState();
		updateFavorState();
	}
	return favor;
}

void Game::useFavor() {
	favorCount--;
	favorTotal--;
}

void Game::pieceMove(Piece* piece, svec2 pos, Piece* occupant, bool forceSwitch) {
	// check attack, favor, move and survival conditions
	svec2 spos = ptog(piece->getPos());
	Tile *stil = getTile(spos), *dtil = getTile(pos);
	Action action = occupant ? isOwnPiece(occupant) || forceSwitch ? ACT_SWAP : ACT_ATCK : ACT_MOVE;
	Com::Tile favored = pollFavor();
	if (checkMove(piece, spos, occupant, pos, action, favored); !survivalCheck(piece, stil, dtil, action, favored)) {
		failSurvivalCheck(piece, action);
		return;
	}

	// handle movement
	bool end = true;
	switch (action) {
	case ACT_MOVE:	// regular move
		if (stil->isBreachedFortress())
			updateFortress(stil, false);
		placePiece(piece, pos);
		ownRec.update(piece, nullptr, action);
		break;
	case ACT_SWAP:	// switch ally or enemy
		placePiece(occupant, spos);
		placePiece(piece, pos);
		ownRec.update(piece, occupant, ACT_MOVE | action);
		if (piece->getType() == Com::Piece::warhorse)
			end = false;
		break;
	case ACT_ATCK: {	// kill or breach
		bool okFort = dtil->isUnbreachedFortress();
		if (okFort)
			updateFortress(dtil, true);
		if (!okFort || piece->getType() == Com::Piece::throne) {
			removePiece(occupant);
			placePiece(piece, pos);
		}
		ownRec.update(piece, nullptr, ACT_MOVE | action); }
	}
	World::play("move");
	concludeAction(end);
}

void Game::pieceFire(Piece* killer, svec2 pos, Piece* piece) {
	svec2 kpos = ptog(killer->getPos());
	Tile* dtil = getTile(pos);
	Com::Tile favored = pollFavor();
	if (checkFire(killer, kpos, piece, pos); !survivalCheck(killer, getTile(kpos), dtil, ACT_FIRE, favored)) {	// occupant must be guaranteed not to be an ally
		failSurvivalCheck(killer, ACT_FIRE);
		return;
	}

	if (dtil->isUnbreachedFortress())	// breach fortress
		updateFortress(dtil, true);
	else {								// regular fire
		if (dtil->isBreachedFortress())
			updateFortress(dtil, false);	// restore fortress
		removePiece(piece);
	}
	ownRec.update(killer, nullptr, ACT_FIRE);
	World::play("ammo");
	concludeAction(true);
}

void Game::concludeAction(bool end) {
	if (!checkWin()) {
		if (end && (!config.multistage || !ownRec.actor->firingDistance() || (ownRec.action & (ACT_MOVE | ACT_FIRE)) == (ACT_MOVE | ACT_FIRE) || ((ownRec.action & ACT_MOVE) && firstTurn)))
			endTurn();
		else {
			disableOwnPiecesInteract(true, true);
			setPieceInteract(ownRec.actor, true, false, &Program::eventFavorStart, & Program::eventMove, ownRec.actor->firingDistance() ? &Program::eventFire : &Program::eventMove);
			World::state<ProgMatch>()->message->setText("Your turn");
			World::netcp()->sendData(sendb);
		}
	}
}

void Game::placeDragon(svec2 pos, Piece* occupant) {
	if (Tile* til = getTile(pos); isHomeTile(til) && (til->getType() == Com::Tile::fortress || (til->getType() == Com::Tile::mountain && !occupant))) {		// tile needs to be a home fortress or unoccupied homeland mountain
		World::state<ProgMatch>()->decreaseDragonIcon();
		if (occupant)
			removePiece(occupant);
		for (Piece* pce = getOwnPieces(Com::Piece::dragon); pce->getType() == Com::Piece::dragon; pce++)
			if (!pieceOnBoard(pce)) {
				placePiece(pce, pos);
				break;
			}
		checkWin();
	}
}

void Game::setBgrid() {
	gridat.free();
	uint16 boardHeight = config.homeSize.y * 2 + 1;
	vector<Vertex> verts((config.homeSize.x + boardHeight + 2) * 2);
	vector<uint16> elems(verts.size());

	for (uint16 i = 0; i < elems.size(); i++)
		elems[i] = i;
	uint16 i = 0;
	for (int16 x = 0; x <= config.homeSize.x; x++)
		for (int16 y = -int16(config.homeSize.y); y <= config.homeSize.y + 1; y += boardHeight)
			verts[i++] = Vertex(gtop(svec2(x, y), -0.018f) + vec3(-(config.objectSize / 2.f), 0.f, -(config.objectSize / 2.f)), Camera::up, vec2(0.f));
	for (int16 y = -int16(config.homeSize.y); y <= config.homeSize.y + 1; y++)
		for (int16 x = 0; x <= config.homeSize.x; x += config.homeSize.x)
			verts[i++] = Vertex(gtop(svec2(x, y), -0.018f) + vec3(-(config.objectSize / 2.f), 0.f, -(config.objectSize / 2.f)), Camera::up, vec2(0.f));
	gridat = GMesh(verts, elems, GL_LINES);
}

void Game::setTiles(Tile* tiles, int16 yofs, bool inter) {
	sizet id = 0;
	for (int16 y = yofs, yend = config.homeSize.y + yofs; y < yend; y++)
		for (int16 x = 0; x < config.homeSize.x; x++)
			tiles[id++] = Tile(gtop(svec2(x, y)), config.objectSize, Com::Tile::empty, nullptr, nullptr, nullptr, inter, inter);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < config.homeSize.x; i++)
		*tiles.mid(i) = Tile(gtop(svec2(i, 0)), config.objectSize, Com::Tile::empty, nullptr, nullptr, nullptr, false, false);
}

void Game::setPieces(Piece* pieces, float rot, const Material* matl) {
	for (uint16 i = 0, t = 0, c = 0; i < config.numPieces; i++) {
		pieces[i] = Piece(gtop(svec2(INT16_MIN)), rot, config.objectSize, Com::Piece(t), nullptr, nullptr, nullptr, matl, false, false);
		if (++c >= config.pieceAmounts[t]) {
			c = 0;
			t++;
		}
	}
}

void Game::setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim) {
	for (uint16 i = 0; i < num; i++)
		tiles[i].setInteractivity(lvl, dim);
}

void Game::setPieceInteract(Piece* piece, bool on, bool dim, GCall hgcall, GCall ulcall, GCall urcall) {
	piece->setRaycast(on, dim);
	piece->hgcall = hgcall;
	piece->ulcall = ulcall;
	piece->urcall = urcall;
}

void Game::setOwnPiecesVisible(bool on, bool event) {
	for (Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (setPieceInteract(it, on, false, nullptr, on && event ? &Program::eventMovePiece : nullptr, nullptr); pieceOnHome(it))
			it->setActive(on);
}

void Game::disableOwnPiecesInteract(bool rigid, bool dim) {
	for (Piece* it = pieces.own(); it != pieces.ene(); it++)
		setPieceInteract(it, rigid, dim, nullptr, nullptr, nullptr);
}

Piece* Game::getPieces(Piece* pieces, Com::Piece type) {
	for (uint8 t = 0; t < uint8(type); t++)
		pieces += config.pieceAmounts[t];
	return pieces;
}

Piece* Game::findPiece(svec2 pos) {
	Piece* pce = std::find_if(pieces.begin(), pieces.end(), [this, pos](const Piece& it) -> bool { return ptog(it.getPos()) == pos; });
	return pce != pieces.end() ? pce : nullptr;
}

void Game::checkMove(Piece* piece, svec2 pos, Piece* occupant, svec2 dst, Action action, Com::Tile favor) {
	if ((ownRec.action & ACT_MOVE) && piece->getType() != Com::Piece::warhorse)
		throw string("You've already moved");
	if (pos == dst)
		throw string();

	switch (Tile* dtil = getTile(dst); action) {
	case ACT_MOVE:
		switch (piece->getType()) {
		case Com::Piece::spearman:
			checkMoveDestination(pos, dst, favor, getTile(pos)->getType() == Com::Tile::water && dtil->getType() == Com::Tile::water ? &Game::collectTilesByType : &Game::collectTilesBySingle);
			break;
		case Com::Piece::lancer:
			checkMoveDestination(pos, dst, favor, getTile(pos)->getType() == Com::Tile::plains && dtil->getType() == Com::Tile::plains ? &Game::collectTilesByType : &Game::collectTilesBySingle);
			break;
		case Com::Piece::dragon:
			checkMoveDestination(pos, dst, favor, &Game::collectTilesByArea, config.dragonDist, spaceAvailible, config.dragonDiag ? adjacentFull.data() : adjacentStraight.data(), uint8(config.dragonDiag ? adjacentFull.size() : adjacentStraight.size()));
			break;
		default:
			checkMoveDestination(pos, dst, favor, &Game::collectTilesBySingle);
		}
		break;
	case ACT_SWAP:
		if (piece->getType() == occupant->getType())
			throw string("Can't swap with a piece of the same type");
		if (favor == Com::Tile::forest)
			useFavor();
		else
			checkMoveDestination(pos, dst, favor, &Game::collectTilesBySingle);
		break;
	case ACT_ATCK:
		if (firstTurn)
			throw string("Can't attack on first turn");
		if (piece->getType() != Com::Piece::throne) {
			checkAttack(piece, occupant, dtil);
			if (piece->getType() == Com::Piece::dragon && dtil->getType() == Com::Tile::forest)
				throw firstUpper(Com::pieceNames[uint8(piece->getType())]) + " can't attack a " + Com::tileNames[uint8(dtil->getType())];
		}
		checkMoveDestination(pos, dst, favor, &Game::collectTilesBySingle);
	}
}

template <class F, class... A>
void Game::checkMoveDestination(svec2 pos, svec2 dst, Com::Tile favor, F check, A... args) {
	bool favorUsed = false;
	if (!(this->*check)(pos, favor, favorUsed, args...).count(posToId(dst)))
		throw string("Can't move there");
	if (favorUsed)
		useFavor();
}

uset<uint16> Game::collectTilesByArea(svec2 pos, Com::Tile favor, bool& favorUsed, uint16 dlim, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, uint16), uint8 movSize) {
	if (favorUsed = favor == Com::Tile::plains)
		dlim++;

	uset<uint16> tcol;
	vector<uint16> dist = Dijkstra::travelDist(posToId(pos), dlim, config.homeSize.x, config.boardSize, stepable, vmov, movSize);
	for (uint16 i = 0; i < config.boardSize; i++)
		if (dist[i] <= dlim)
			tcol.insert(i);
	return tcol;
}

uset<uint16> Game::collectTilesByType(svec2 pos, Com::Tile favor, bool& favorUsed) {
	uset<uint16> tcol;
	collectAdjacentTilesByType(tcol, posToId(pos), getTile(pos)->getType());
	uset<uint16> sing = collectTilesBySingle(pos, favor, favorUsed);
	tcol.insert(sing.begin(), sing.end());
	return tcol;
}

void Game::collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, Com::Tile type) const {
	tcol.insert(pos);
	for (uint16 (*mov)(uint16, uint16) : adjacentFull)
		if (uint16 ni = mov(pos, config.homeSize.x); ni < config.boardSize && tiles[ni].getType() == type && !tcol.count(ni))
			collectAdjacentTilesByType(tcol, ni, type);
}

bool Game::spaceAvailible(uint16 pos) {
	Piece* occ = World::game()->findPiece(World::game()->idToPos(pos));
	return !occ || World::game()->isOwnPiece(occ) || (occ->getType() != Com::Piece::dragon && !occ->firingDistance());
}

void Game::highlightMoveTiles(Piece* pce) {
	for (Tile& it : tiles)
		it.setEmission(it.getEmission() & ~BoardObject::EMI_HIGH);
	if (!pce)
		return;

	svec2 pos = ptog(pce->getPos());
	Com::Tile src = getTile(pos)->getType();
	Com::Tile favor = checkFavor();
	uset<uint16> tcol;
	if (favor == Com::Tile::forest)
		for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			if (svec2 tp = ptog(it->getPos()); tp != pos && getTile(tp)->getType() == Com::Tile::forest)
				tcol.insert(posToId(tp));

	switch (bool fu; pce->getType()) {
	case Com::Piece::spearman:
		tcol = src == Com::Tile::water ? collectTilesByType(pos, favor, fu) : collectTilesBySingle(pos, favor, fu);
		break;
	case Com::Piece::lancer:
		tcol = src == Com::Tile::plains ? collectTilesByType(pos, favor, fu) : collectTilesBySingle(pos, favor, fu);
		break;
	case Com::Piece::dragon:
		tcol = collectTilesByArea(pos, favor, fu, config.dragonDist, spaceAvailible, config.dragonDiag ? adjacentFull.data() : adjacentStraight.data(), uint8(config.dragonDiag ? adjacentFull.size() : adjacentStraight.size()));
		break;
	default:
		tcol = collectTilesBySingle(pos, favor, fu);
	}
	for (uint16 id : tcol)
		tiles[id].setEmission(tiles[id].getEmission() | BoardObject::EMI_HIGH);
}

void Game::checkFire(Piece* killer, svec2 pos, Piece* victim, svec2 dst) {
	if (ownRec.action & ACT_FIRE)
		throw string("You've already fired");
	if (firstTurn)
		throw string("Can't fire on first turn");
	if (pos == dst)
		throw string();
	if (!victim)
		throw string("Can't fire at nobody");
	if (isOwnPiece(victim))
		throw string("Can't fire at an ally");

	Tile* dtil = getTile(dst);
	checkAttack(killer, victim, dtil);
	if (victim->getType() == Com::Piece::ranger && dtil->getType() == Com::Tile::mountain)
		throw "Can't fire at a " + Com::pieceNames[uint8(victim->getType())] + " in the " + Com::tileNames[uint8(dtil->getType())] + 's';
	if ((killer->getType() == Com::Piece::crossbowman || killer->getType() == Com::Piece::catapult) && dtil->getType() == Com::Tile::forest)
		throw firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't fire at a " + Com::tileNames[uint8(dtil->getType())];
	if (uint8 dist = killer->firingDistance(); !collectTilesByDistance(pos, dist).count(posToId(dst)))
		throw firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can only fire at a distance of " + toStr(dist);
}

uset<uint16> Game::collectTilesByDistance(svec2 pos, int16 dist) {
	uset<uint16> tcol;
	for (svec2 it : { pos - dist, pos - svec2(0, dist), pos + svec2(dist, -dist), pos - svec2(dist, 0), pos + svec2(dist, 0), pos + svec2(-dist, dist), pos + svec2(0, dist), pos + dist })
		if (inRange(it, svec2(0, -int16(config.homeSize.y)), svec2(config.homeSize.x - 1, config.homeSize.y)))
			tcol.insert(posToId(it));
	return tcol;
}

void Game::highlightFireTiles(Piece* pce) {
	for (Tile& it : tiles)
		it.setEmission(it.getEmission() & ~BoardObject::EMI_HIGH);
	if (!pce)
		return;
	if (uint8 dist = pce->firingDistance())
		for (uint16 id : collectTilesByDistance(ptog(pce->getPos()), dist))
			tiles[id].setEmission(tiles[id].getEmission() | BoardObject::EMI_HIGH);
}

void Game::checkAttack(Piece* killer, Piece* victim, Tile* dtil) const {
	if (eneRec.isProtectedMember(victim))
		throw firstUpper(Com::pieceNames[uint8(victim->getType())]) + " is protected";
	if (victim->getType() == Com::Piece::elephant && dtil->getType() == Com::Tile::plains && killer->getType() != Com::Piece::dragon)
		throw firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't attack an " + Com::pieceNames[uint8(victim->getType())] + " on " + Com::tileNames[uint8(dtil->getType())];
}

bool Game::survivalCheck(Piece* piece, Tile* stil, Tile* dtil, Action action, Com::Tile favor) {
	Com::Tile src = action != ACT_FIRE ? stil->getType() : Com::Tile::empty;						// don't check src when firing (only check dst)
	Com::Tile dst = action == ACT_SWAP || action == ACT_FIRE ? dtil->getType() : Com::Tile::empty;	// check dst when switching/firing (in addition to src when switching)
	if (!(dtil->isUnbreachedFortress() && (action == ACT_ATCK || action == ACT_FIRE))) {	// always run survival check when attacking a fortress
		if ((src != Com::Tile::mountain && src != Com::Tile::water && dst != Com::Tile::mountain && dst != Com::Tile::water) || piece->getType() == Com::Piece::dragon)
			return true;
		if ((piece->getType() == Com::Piece::ranger && src == Com::Tile::mountain && dst != Com::Tile::water) || (piece->getType() == Com::Piece::spearman && src == Com::Tile::water && dst != Com::Tile::mountain))
			return true;
	}
	if (favor == Com::Tile::mountain || (action == ACT_SWAP && favor == Com::Tile::water)) {
		useFavor();
		return true;
	}
	return randDist(randGen) < config.survivalPass;
}

void Game::failSurvivalCheck(Piece* piece, Action action) {
	if (piece->getType() != Com::Piece::throne && (action == ACT_ATCK || config.survivalKill)) {
		removePiece(piece);
		World::netcp()->sendData(sendb);
	} else
		endTurn();
	World::state<ProgMatch>()->message->setText("Failed survival check");
}

bool Game::checkWin() {
	if (checkThroneWin(getOwnPieces(Com::Piece::throne))) {	// no need to check if enemy throne has occupied own fortress
		doWin(false);
		return true;
	}
	if (checkThroneWin(getPieces(pieces.ene(), Com::Piece::throne)) || checkFortressWin()) {
		doWin(true);
		return true;
	}
	return false;
}

bool Game::checkThroneWin(Piece* thrones) {
	if (uint16 c = config.winThrone)
		for (uint16 i = 0; i < config.pieceAmounts[uint8(Com::Piece::throne)]; i++)
			if (!thrones[i].show && !--c)
				return true;
	return false;
}

bool Game::checkFortressWin() {
	if (uint16 cnt = config.winFortress)											// if there's a fortress quota
		for (Tile* tit = tiles.ene(); tit != tiles.mid(); tit++)					// iterate enemy tiles
			if (Piece* pit = pieces.own(); tit->getType() == Com::Tile::fortress)	// if tile is an enemy fortress
				for (uint8 pi = 0; pi < Com::pieceMax; pit += config.pieceAmounts[pi++])	// iterate piece types
					if (config.capturers[pi])										// if the piece type is a capturer
						for (uint16 i = 0; i < config.pieceAmounts[pi]; i++)		// iterate pieces of that type
							if (tileId(tit) == posToId(ptog(pit[i].getPos())) && !--cnt)	// if such a piece is on the fortress
								return true;										// decrement fortress counter and win if 0
	return false;
}

void Game::doWin(bool win) {
	ownRec.action |= win ? ACT_WIN : ACT_LOOSE;
	endTurn();
	World::program()->finishMatch(win);
}

void Game::prepareTurn() {
	setTilesInteract(tiles.begin(), config.boardSize, Tile::Interact(myTurn));
	for (Piece* it = pieces.own(); it != pieces.ene(); it++)
		setPieceInteract(it, myTurn && pieceOnBoard(it), false, &Program::eventFavorStart, & Program::eventMove, it->firingDistance() ? &Program::eventFire : &Program::eventMove);
	for (Piece* it = pieces.ene(); it != pieces.end(); it++)
		setPieceInteract(it, myTurn && pieceOnBoard(it), false, nullptr, nullptr, nullptr);
	(myTurn ? eneRec : ownRec).updateProtectionColors(true);

	ProgMatch* pm = World::state<ProgMatch>();
	pm->message->setText(myTurn ? "Your turn" : "Opponent's turn");
	pm->updateFavorIcon(myTurn, favorCount, favorTotal);
	pm->updateTurnIcon(myTurn);
	pm->setDragonIcon(myTurn);
}

void Game::endTurn() {
	firstTurn = myTurn = false;
	prepareTurn();

	sendb.pushHead(Com::Code::record);
	sendb.push(uint8(ownRec.action));
	sendb.push(vector<uint16>{ inversePieceId(ownRec.actor), inversePieceId(ownRec.swapper) });
	World::netcp()->sendData(sendb);
}

void Game::recvRecord(uint8* data) {
	uint16 ai = SDLNet_Read16(data + 1), si = SDLNet_Read16(data + 3);
	eneRec = Record(ai < config.piecesSize ? &pieces[ai] : nullptr, si < config.piecesSize ? &pieces[si] : nullptr, Action(data[0]));
	ownRec = Record();
	if (eneRec.action & ACT_MOVE)
		World::play("move");
	else if (eneRec.action & ACT_FIRE)
		World::play("ammo");
	myTurn = true;
	prepareTurn();

	if (eneRec.action & (ACT_WIN | ACT_LOOSE))
		World::program()->finishMatch(eneRec.action & ACT_LOOSE);
}

void Game::sendConfig(bool onJoin) {
	Com::Code code = onJoin ? Com::Code::cnjoin : Com::Code::config;
	sendb.size = Com::codeSizes[uint8(code)];
	sendb.writeHead(code, sendb.size);
	config.toComData(sendb.data + Com::dataHeadSize);
	World::netcp()->sendData(sendb);
}

void Game::sendStart() {
	std::default_random_engine randGen = createRandomEngine();
	sendb.size = Com::codeSizes[uint8(Com::Code::start)];
	uint16 ofs = sendb.writeHead(Com::Code::start, sendb.size);
	sendb.data[ofs] = uint8(std::uniform_int_distribution<uint>(0, 1)(randGen));
	config.toComData(sendb.data + ofs + 1);
	World::netcp()->sendData(sendb);

	firstTurn = myTurn = !sendb.data[ofs];		// basically simulate recvConfig but without reinitializing config
	World::program()->eventOpenSetup();
}

void Game::recvStart(uint8* data) {
	firstTurn = myTurn = data[0];
	config.fromComData(data + 1);
	World::program()->eventOpenSetup();
}

void Game::sendSetup() {
	// send tiles
	sendb.size = config.tileCompressionSize();
	uint16 ofs = sendb.writeHead(Com::Code::tiles, sendb.size);
	std::fill_n(sendb.data + ofs, sendb.size, 0);
	for (uint16 i = 0; i < config.extraSize; i++)
		sendb.data[i/2+ofs] |= compressTile(config.extraSize - i - 1);
	World::netcp()->sendData(sendb);

	// send pieces
	sendb.size = config.pieceDataSize();
	ofs = sendb.writeHead(Com::Code::pieces, sendb.size);
	for (uint16 i = 0; i < config.numPieces; i++)
		SDLNet_Write16(pieceOnBoard(pieces.own(i)) ? invertId(posToId(ptog(pieces.own(i)->getPos()))) : UINT16_MAX, sendb.data + i * sizeof(uint16) + ofs);
	World::netcp()->sendData(sendb);
}

void Game::recvTiles(uint8* data) {
	for (uint16 i = 0; i < config.numTiles; i++)
		tiles.ene(i)->setTypeSilent(decompressTile(data, i));
	for (uint16 i = 0; i < config.homeSize.x; i++)
		World::state<ProgSetup>()->rcvMidBuffer[i] = decompressTile(data, config.numTiles + i);
}

void Game::recvPieces(uint8* data) {
	for (uint16 i = 0; i < config.numPieces; i++) {
		uint16 id = SDLNet_Read16(data + i * sizeof(uint16));
		pieces.ene(i)->setPos(gtop(id < config.numTiles ? idToPos(id) : svec2(INT16_MIN)));
	}

	if (ProgSetup* ps = World::state<ProgSetup>(); ps->getStage() == ProgSetup::Stage::ready)
		World::program()->eventOpenMatch();
	else {
		ps->enemyReady = true;
		ps->message->setText(ps->getStage() == ProgSetup::Stage::pieces ? "Opponent ready" : "Hurry the fuck up!");
	}
}

void Game::placePiece(Piece* piece, svec2 pos) {
	if (piece->getType() == Com::Piece::throne && getTile(pos)->getType() == Com::Tile::fortress && favorCount < favorTotal)
		if (uint16 fid = posToId(pos); piece->lastFortress != fid) {
			piece->lastFortress = fid;
			favorCount++;
		}
	piece->updatePos(pos, true);
	sendb.pushHead(Com::Code::move);
	sendb.push(vector<uint16>{ inversePieceId(piece), invertId(posToId(pos)) });
}

void Game::removePiece(Piece* piece) {
	piece->updatePos();
	sendb.pushHead(Com::Code::kill);
	sendb.push(inversePieceId(piece));
}

void Game::updateFortress(Tile* fort, bool breached) {
	fort->setBreached(breached);
	sendb.pushHead(Com::Code::breach);
	sendb.push(uint8(breached));
	sendb.push(inverseTileId(fort));
}

std::default_random_engine Game::createRandomEngine() {
	std::default_random_engine randGen;
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(time(nullptr)));
	}
	return randGen;
}
#ifdef DEBUG
vector<Object*> Game::initDummyObjects() {
	vector<Object*> objs = initObjects();

	uint8 t = 0;
	vector<uint16> amts(config.tileAmounts.begin(), config.tileAmounts.end());
	for (int16 x = 0; x < config.homeSize.x; x++)
		for (int16 y = 0; y < config.homeSize.y; y++) {
			if (outRange(svec2(x, y), svec2(1, 0), svec2(config.homeSize.x - 2, config.homeSize.y - 2)) || !amts[uint8(Com::Tile::fortress)]) {
				for (; !amts[t]; t++);
				tiles.own(y * config.homeSize.x + x)->setType(Com::Tile(t));
				amts[t]--;
			} else
				amts.back()--;
		}

	t = 0;
	amts.assign(config.middleAmounts.begin(), config.middleAmounts.end());
	for (uint16 i = 0; t < amts.size(); i++) {
		for (; t < amts.size() && !amts[t]; t++);
		if (t < amts.size()) {
			tiles.mid(i)->setType(Com::Tile(t));
			amts[t]--;
		}
	}

	for (uint16 i = 0; i < config.numPieces; i++)
		pieces.own(i)->setPos(gtop(svec2(i % config.homeSize.x, 1 + i / config.homeSize.x)));
	return objs;
}
#endif
