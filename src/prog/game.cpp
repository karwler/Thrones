#include "engine/world.h"

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
	updateProtectionColor(actor, on);
	updateProtectionColor(swapper, on);
}

void Record::updateProtectionColor(Piece* pce, bool on) const {
	if (pce)
		pce->diffuseFactor = pieceProtected(pce) && on ? 0.5f : 1.f;
}

bool Record::isProtectedMember(Piece* pce) const {
	return pce && (actor == pce || swapper == pce) && pieceProtected(pce);
}

// GAME

const vec2 Game::screenPosUp(Com::Config::boardWidth / 2.f, 1.7f);
const vec2 Game::screenPosDown(Com::Config::boardWidth / 2.f, -1.8f);

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
	randGen(Com::createRandomEngine()),
	randDist(0, Com::Config::randomLimit - 1),
	gridat({}, { vec3() }, { vec3(0.f, 1.f, 0.f) }, {}),
	bgrid(vec3(0.f), vec3(0.f), vec3(1.f), &gridat, World::scene()->material("gray"), nullptr, Object::INFO_SHOW | Object::INFO_LINES),
	board(vec3(Com::Config::boardWidth / 2.f, -BoardObject::upperPoz, 0.f), vec3(0.f), vec3(Com::Config::boardWidth + 1.f, 1.f, Com::Config::boardWidth + 1.f), World::scene()->blueprint("plane"), World::scene()->material("dark brown"), nullptr, Object::INFO_FILL),
	screen(vec3(screenPosUp.x, screenPosUp.y, 0.f), vec3(90.f, 0.f, 0.f), vec3(Com::Config::boardWidth + 0.5f, 1.f, 3.4f), World::scene()->blueprint("plane"), World::scene()->material("light brown"), nullptr, Object::INFO_FILL),
	tiles(config),
	pieces(config)
{
	World::scene()->effects = {
		Object(gtop(INT16_MIN, BoardObject::upperPoz / 2.f), vec3(0.f), vec3(config.objectSize, 1.f, config.objectSize), World::scene()->blueprint("outline"), World::scene()->material("gold"), nullptr, Object::INFO_TEXTURE)	// INFO_SHOW indicates if favor is being used and pos informs tile type
	};
}

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
		firstTurn = myTurn = data[0];
		World::program()->eventOpenSetup();
		break;
	case Com::Code::tiles:
		for (uint16 i = 0; i < config.numTiles; i++)
			tiles.ene(i)->setType(decompressTile(data, i));
		for (uint16 i = 0; i < config.homeWidth; i++)
			static_cast<ProgSetup*>(World::state())->rcvMidBuffer[i] = decompressTile(data, config.numTiles + i);
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
		pieces[*reinterpret_cast<const uint16*>(data)].disable();
		break;
	case Com::Code::breach:
		(tiles.begin() + *reinterpret_cast<const uint16*>(data + 1))->setBreached(data[0]);
		break;
	case Com::Code::record:
		eneRec = Record(&pieces[*reinterpret_cast<const uint16*>(data + 1)], &pieces[*reinterpret_cast<const uint16*>(data + 3)], Record::Action(data[0]));
		ownRec = Record();
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
	screen.pos.z = -(config.objectSize / 2.f);

	// prepare objects for setup
	setTiles(tiles.ene(), -int16(config.homeHeight), Object::INFO_TEXTURE);
	setMidTiles();
	setTiles(tiles.own(), 1, Object::INFO_TEXTURE | Object::INFO_RAYCAST);
	setPieces(pieces.own(), &Program::eventMovePiece, World::scene()->material("ally"), Object::INFO_TEXTURE);
	setPieces(pieces.ene(), nullptr, World::scene()->material("enemy"), Object::INFO_TEXTURE);

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
	setTilesInteract(tiles.begin(), config.boardSize, Tile::Interactivity::ignore);
	setOwnPiecesInteract(false);
}

void Game::prepareMatch() {
	// set interactivity and reassign callback events
	for (Tile& it : tiles)
		it.setRaycast(myTurn);	// interactivity is already reset during setup phase
	for (Piece* it = pieces.own(); it != pieces.ene(); it++) {
		it->setRaycast(myTurn);
		it->hgcall = &Program::eventFavorStart;
		it->ulcall = &Program::eventMove;
		it->urcall = it->canFire() ? &Program::eventFire : nullptr;
	}
	for (Piece* it = pieces.ene(); it != pieces.end(); it++) {
		it->mode |= Object::INFO_SHOW;
		it->setRaycast(myTurn);
	}

	// check for favor
	Piece* throne = getOwnPieces(Com::Piece::throne);
	if (vec2s pos = ptog(throne->pos); getTile(pos)->getType() == Com::Tile::fortress) {
		throne->lastFortress = posToId(pos);
		favorCount = config.favorLimit > 0;
	} else
		favorCount = 0;
	favorTotal = config.favorLimit;

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
	std::replace(mid, mid + config.homeWidth, Com::Tile::empty, Com::Tile::fortress);
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
			if (!cnt[i] && config.tileAmounts[i])
				throw firstUpper(Com::tileNames[i]) + " missing in row " + to_string(y);
	}
	if (empties > config.tileAmounts[uint8(Com::Tile::fortress)])
		throw string("Not all tiles were placed");
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

vector<uint16> Game::countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt) {
	for (uint16 i = 0; i < num; i++)
		if (uint8(tiles[i].getType()) < cnt.size())
			cnt[uint8(tiles[i].getType())]--;
	return cnt;
}

vector<uint16> Game::countOwnPieces() const {
	vector<uint16> cnt(config.pieceAmounts.begin(), config.pieceAmounts.end());
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (inRange(ptog(it->pos), vec2s(0, 1), vec2s(config.homeWidth - 1, config.homeHeight)))
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
		World::scene()->effects[0].mode &= ~Object::INFO_SHOW;
	else if (Tile* til = getTile(ptog(favorState.piece->pos)); til->getType() < Com::Tile::fortress) {
		World::scene()->effects[0].pos = vec3(favorState.piece->pos.x, World::scene()->effects[0].pos.y, favorState.piece->pos.z);
		World::scene()->effects[0].mode |= Object::INFO_SHOW;
	}
}

Com::Tile Game::pollFavor() {
	if (World::scene()->effects[0].mode & Object::INFO_SHOW) {
		favorState.piece = nullptr;
		updateFavorState();
		return getTile(ptog(World::scene()->effects[0].pos))->getType();
	}
	return Com::Tile::empty;
}

void Game::useFavor() {
	favorCount--;
	favorTotal--;
}

void Game::pieceMove(Piece* piece, vec2s pos, Piece* occupant) {
	// check attack, favor, move and survival conditions
	if (ownRec.action & Record::ACT_MOVE) {
		static_cast<ProgMatch*>(World::state())->message->setText("You've already moved");
		return;
	}
	vec2s spos = ptog(piece->pos);
	bool attacking = occupant && !isOwnPiece(occupant);
	Com::Tile favored = pollFavor();
	if (!checkMove(piece, spos, occupant, pos, attacking, favored))
		return;
	if (!survivalCheck(piece, spos, pos, attacking, occupant && isOwnPiece(occupant), favored)) {
		failSurvivalCheck(piece->getType() != Com::Piece::throne ? piece : nullptr);
		return;
	}

	// handle movement
	if (attacking) {	// kill or breach
		if (Tile* til = getTile(pos); piece->getType() != Com::Piece::throne && til->isUnbreachedFortress())	// breach fortress
			updateFortress(til, true);
		else {			// execute attack
			removePiece(occupant);
			placePiece(piece, pos);
		}
		ownRec.update(piece, nullptr, Record::ACT_MOVE | Record::ACT_ATCK);
	} else if (occupant) {	// switch ally
		placePiece(occupant, spos);
		placePiece(piece, pos);
		ownRec.update(piece, occupant, Record::ACT_MOVE | Record::ACT_SWAP);
	} else {				// regular move
		if (Tile* til = getTile(spos); til->isBreachedFortress())
			updateFortress(til, false);	// restore fortress
		placePiece(piece, pos);
		ownRec.update(piece, nullptr, Record::ACT_MOVE);
	}
	concludeAction();
}

void Game::pieceFire(Piece* killer, vec2s pos, Piece* piece) {
	if (ownRec.action & Record::ACT_FIRE) {
		static_cast<ProgMatch*>(World::state())->message->setText("You've already fired");
		return;
	}
	vec2s kpos = ptog(killer->pos);
	Com::Tile favored = pollFavor();
	if (!checkFire(killer, kpos, piece, pos))	// check rules
		return;
	if (!survivalCheck(killer, kpos, pos, false, false, favored)) {
		failSurvivalCheck(nullptr);
		return;
	}

	if (Tile* til = getTile(pos); til->isUnbreachedFortress()) {	// breach fortress
		updateFortress(til, true);
		ownRec.update(killer, nullptr, Record::ACT_FIRE);
	} else {	// regular fire
		if (til->isBreachedFortress())
			updateFortress(til, false);	// restore fortress
		removePiece(piece);
		ownRec.update(killer, nullptr, Record::ACT_FIRE);
	}
	concludeAction();
}

void Game::concludeAction() {
	if (!checkWin()) {
		if (!config.multistage || !ownRec.actor->canFire() || (ownRec.action & (Record::ACT_MOVE | Record::ACT_FIRE)) == (Record::ACT_MOVE | Record::ACT_FIRE) || ((ownRec.action & Record::ACT_MOVE) && firstTurn))
			endTurn();
		else {
			setOwnPiecesInteract(false, true);
			ownRec.actor->setRaycast(true);
			static_cast<ProgMatch*>(World::state())->message->setText("");
		}
	}
}

void Game::placeDragon(vec2s pos, Piece* occupant) {
	if (Tile* til = getTile(pos); til->getType() == Com::Tile::fortress && isHomeTile(til)) {		// tile needs to be a home fortress
		static_cast<ProgMatch*>(World::state())->deleteDragonIcon();
		if (occupant)
			removePiece(occupant);
		placePiece(getOwnPieces(Com::Piece::dragon), pos);
		checkWin();
	}
}

void Game::setBgrid() {
	uint16 i = 0;
	uint16 boardHeight = config.homeHeight * 2 + 1;

	gridat.verts.resize((config.homeWidth + boardHeight + 2) * 2);
	for (int16 x = 0; x <= config.homeWidth; x++)
		for (int16 y = -int16(config.homeHeight); y <= config.homeHeight + 1; y += boardHeight)
			gridat.verts[i++] = gtop(vec2s(x, y), BoardObject::upperPoz) + vec3(-(config.objectSize / 2.f), 0.f, -(config.objectSize / 2.f));
	for (int16 y = -int16(config.homeHeight); y <= config.homeHeight + 1; y++)
		for (int16 x = 0; x <= config.homeWidth; x += config.homeWidth)
			gridat.verts[i++] = gtop(vec2s(x, y), BoardObject::upperPoz) + vec3(-(config.objectSize / 2.f), 0.f, -(config.objectSize / 2.f));

	gridat.elems.resize(gridat.verts.size());
	for (i = 0; i < gridat.elems.size(); i++)
		gridat.elems[i] = Vertex(i, 0, 0);
}

void Game::setTiles(Tile* tiles, int16 yofs, Object::Info mode) {
	sizet id = 0;
	for (int16 y = yofs, yend = config.homeHeight + yofs; y < yend; y++)
		for (int16 x = 0; x < config.homeWidth; x++)
			tiles[id++] = Tile(gtop(vec2s(x, y), 0.f), config.objectSize, Com::Tile::empty, nullptr, nullptr, nullptr, mode);
}

void Game::setMidTiles() {
	for (int8 i = 0; i < config.homeWidth; i++)
		*tiles.mid(i) = Tile(gtop(vec2s(i, 0), 0.f), config.objectSize, Com::Tile::empty, nullptr, &Program::eventMoveTile, nullptr, Object::INFO_TEXTURE);
}

void Game::setPieces(Piece* pieces, OCall ucall, const Material* mat, Object::Info mode) {
	for (uint16 i = 0, t = 0, c = 0; i < config.numPieces; i++) {
		pieces[i] = Piece(gtop(INT16_MIN, BoardObject::upperPoz), config.objectSize, Com::Piece(t), nullptr, ucall, nullptr, mat, mode);
		if (++c >= config.pieceAmounts[t]) {
			c = 0;
			t++;
		}
	}
}

void Game::setTilesInteract(Tile* tiles, uint16 num, Tile::Interactivity lvl, bool dim) {
	for (uint16 i = 0; i < num; i++)
		tiles[i].setInteractivity(lvl, dim);
}

void Game::setPiecesInteract(Piece* pieces, uint16 num, bool on, bool dim) {
	for (uint16 i = 0; i < num; i++)
		pieces[i].setRaycast(on, dim);
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

bool Game::checkMove(Piece* piece, vec2s pos, Piece* occupant, vec2s dst, bool attacking, Com::Tile favor) {
	if (pos == dst) {
		static_cast<ProgMatch*>(World::state())->message->setText("Can't swap with yourself");
		return false;
	}
	Tile* dtil = getTile(dst);
	if (attacking) {
		if (firstTurn) {
			static_cast<ProgMatch*>(World::state())->message->setText("Can't attack on first turn");
			return false;
		}
		if (dtil->isUnbreachedFortress())
			return checkMoveBySingle(pos, dst, favor);
		else if (!checkAttack(piece, occupant, dtil))
			return false;
	} else if (occupant) {
		if (piece->getType() == occupant->getType()) {
			static_cast<ProgMatch*>(World::state())->message->setText("Can't swap with a piece of the same type");
			return false;
		}
		if (favor == Com::Tile::forest) {
			useFavor();
			return true;
		}
		return checkMoveBySingle(pos, dst, favor);
	}

	switch (piece->getType()) {
	case Com::Piece::spearman:
		return dtil->getType() == Com::Tile::water ? checkMoveByType(pos, dst, favor) : checkMoveBySingle(pos, dst, favor);
	case Com::Piece::lancer:
		return dtil->getType() == Com::Tile::plains && !attacking ? checkMoveByType(pos, dst, favor) : checkMoveBySingle(pos, dst, favor);
	case Com::Piece::dragon:
		return !attacking ? checkMoveByArea(pos, dst, favor, config.dragonDist, spaceAvailible, config.dragonDiag ? adjacentFull.data() : adjacentStraight.data(), uint8(config.dragonDiag ? adjacentFull.size() : adjacentStraight.size())) : checkMoveBySingle(pos, dst, favor);
	}
	return checkMoveBySingle(pos, dst, favor);
}

bool Game::checkMoveByArea(vec2s pos, vec2s dst, Com::Tile favor, uint16 dlim, bool(*stepable)(uint16), uint16(*const* vmov)(uint16, uint16), uint8 movSize) {
	if (favor == Com::Tile::plains) {
		useFavor();
		dlim++;
	}
	if (Dijkstra::travelDist(posToId(pos), dlim, config.homeWidth, config.boardSize, stepable, vmov, movSize)[posToId(dst)] > dlim) {
		static_cast<ProgMatch*>(World::state())->message->setText("Can't move there");
		return false;
	}
	return true;
}

bool Game::spaceAvailible(uint16 pos) {
	Piece* occ = World::game()->findPiece(World::game()->idToPos(pos));
	return !occ || World::game()->isOwnPiece(occ) || (occ->getType() != Com::Piece::dragon && !occ->canFire());
}

bool Game::checkMoveByType(vec2s pos, vec2s dst, Com::Tile favor) {
	vector<bool> visited(config.boardSize, false);
	return checkAdjacentTilesByType(posToId(pos), posToId(dst), visited, getTile(pos)->getType()) || checkMoveBySingle(pos, dst, favor);
}

bool Game::checkAdjacentTilesByType(uint16 pos, uint16 dst, vector<bool>& visited, Com::Tile type) const {
	visited[pos] = true;
	for (uint16 (*mov)(uint16, uint16) : adjacentFull)
		if (uint16 ni = mov(pos, config.homeWidth); ni < config.boardSize && tiles[ni].getType() == type && !visited[ni] && checkAdjacentTilesByType(ni, dst, visited, type))
			return true;
	return false;
}

bool Game::checkFire(Piece* killer, vec2s pos, Piece* victim, vec2s dst) {
	if (!victim) {
		static_cast<ProgMatch*>(World::state())->message->setText("Can't fire at nobody");
		return false;
	}
	if (firstTurn) {
		static_cast<ProgMatch*>(World::state())->message->setText("Can't fire on first turn");
		return false;
	}
	if (pos == dst) {
		static_cast<ProgMatch*>(World::state())->message->setText("Suicide is not yet implemented");
		return false;
	}
	if (isOwnPiece(victim)) {
		static_cast<ProgMatch*>(World::state())->message->setText("Can't fire at an ally");
		return false;
	}
	if (Tile* dtil = getTile(dst); dtil->isPenetrable()) {	// check rules of directly firing at a piece
		if (!checkAttack(killer, victim, dtil))
			return false;
		if (victim->getType() == Com::Piece::ranger && dtil->getType() == Com::Tile::mountain) {
			static_cast<ProgMatch*>(World::state())->message->setText("Can't fire at a " + Com::pieceNames[uint8(victim->getType())] + " in the " + Com::tileNames[uint8(dtil->getType())] + 's');
			return false;
		}
		if (killer->getType() == Com::Piece::crossbowman && (dtil->getType() == Com::Tile::forest || dtil->getType() == Com::Tile::mountain)) {
			static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't fire at " + (dtil->getType() == Com::Tile::forest ? "a ": "") + Com::tileNames[uint8(dtil->getType())] + (dtil->getType() == Com::Tile::mountain ? "s" : ""));
			return false;
		}
		if (killer->getType() == Com::Piece::catapult && dtil->getType() == Com::Tile::forest) {
			static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't fire at a " + Com::tileNames[uint8(dtil->getType())] + 's');
			return false;
		}
	}
	if (int16 dist = int16(killer->getType()) - int16(Com::Piece::crossbowman) + 1; !checkTilesByDistance(pos, dst, dist)) {
		static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can only fire at a distance of " + to_string(dist));
		return false;
	}
	return true;
}

bool Game::checkTilesByDistance(vec2s pos, vec2s dst, int16 dist) {
	vec2s dlt = dst - pos;
	return (!dlt.x || std::abs(dlt.x) == dist) && (!dlt.y || std::abs(dlt.y) == dist);
}

bool Game::checkAttack(Piece* killer, Piece* victim, Tile* dtil) {
	if (killer->getType() == Com::Piece::throne)
		return true;
	if (eneRec.isProtectedMember(victim)) {
		static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(victim->getType())]) + " is protected");
		return false;
	}
	if (killer->getType() == Com::Piece::dragon && dtil->getType() == Com::Tile::forest) {
		static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't attack a " + Com::tileNames[uint8(dtil->getType())]);
		return false;
	}
	if (victim->getType() == Com::Piece::elephant && dtil->getType() == Com::Tile::plains && killer->getType() != Com::Piece::dragon) {
		static_cast<ProgMatch*>(World::state())->message->setText(firstUpper(Com::pieceNames[uint8(killer->getType())]) + " can't attack an " + Com::pieceNames[uint8(victim->getType())] + " on " + Com::tileNames[uint8(dtil->getType())]);
		return false;
	}
	return true;
}

bool Game::survivalCheck(Piece* piece, vec2s spos, vec2s dpos, bool attacking, bool switching, Com::Tile favor) {
	Com::Tile src = getTile(spos)->getType();
	Com::Tile dst = attacking ? getTile(dpos)->getType() : Com::Tile::empty;	// dst only relevant when attacking
	if ((src != Com::Tile::mountain && src != Com::Tile::water && dst != Com::Tile::mountain && dst != Com::Tile::water) || piece->getType() == Com::Piece::dragon)
		return true;
	if ((piece->getType() == Com::Piece::ranger && src == Com::Tile::mountain && dst != Com::Tile::water) || (piece->getType() == Com::Piece::spearman && src == Com::Tile::water && dst != Com::Tile::mountain))
		return true;
	if ((switching && favor == Com::Tile::water) || favor == Com::Tile::mountain) {
		useFavor();
		return true;
	}
	return randDist(randGen) < config.survivalPass;
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
	if (uint16 cnt = config.winFortress)											// if there's a fortress quota
		for (Tile* tit = tiles.ene(); tit != tiles.mid(); tit++)					// iterate enemy tiles
			if (Piece* pit = pieces.own(); tit->getType() == Com::Tile::fortress)	// if tile is an enemy fortress
				for (uint8 pi = 0; pi < Com::pieceMax; pit += config.pieceAmounts[pi++])	// iterate piece types
					if (config.capturers[pi])										// if the piece type is a capturer
						for (uint16 i = 0; i < config.pieceAmounts[pi]; i++)		// iterate pieces of that type
							if (tileId(tit) == posToId(ptog(pit[i].pos)) && !--cnt)	// if such a piece is on the fortress
								return true;										// decrement fortress counter and win if 0
	return false;
}

bool Game::doWin(bool win) {
	netcp->push(vector<uint8>({ uint8(Com::Code::win), uint8(win) }));
	endTurn();
	disconnect(win ? messageWin : messageLoose);
	return true;
}

void Game::prepareTurn() {
	setTilesInteract(tiles.begin(), config.boardSize, Tile::Interactivity(myTurn));
	setPiecesInteract(pieces.begin(), config.piecesSize, myTurn);
	(myTurn ? eneRec : ownRec).updateProtectionColors(true);

	ProgMatch* pm = static_cast<ProgMatch*>(World::state());
	pm->message->setText(myTurn ? messageTurnGo : messageTurnWait);
	pm->updateFavorIcon(myTurn, favorCount, favorTotal);
	pm->updateTurnIcon(myTurn);
	pm->setDragonIcon(myTurn);
}

void Game::endTurn() {
	firstTurn = myTurn = false;
	prepareTurn();

	netcp->push(vector<uint8>({ uint8(Com::Code::record), uint8(ownRec.action) }));
	netcp->push(vector<uint16>({ inversePieceId(ownRec.actor), inversePieceId(ownRec.swapper) }));
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
	if (piece->getType() == Com::Piece::throne && getTile(pos)->getType() == Com::Tile::fortress && favorCount < favorTotal)
		if (uint16 fid = posToId(pos); piece->lastFortress != fid) {
			piece->lastFortress = fid;
			favorCount++;
		}
	piece->pos = gtop(pos, BoardObject::upperPoz);
	netcp->push(Com::Code::move);
	netcp->push(vector<uint16>({ uint16(piece - pieces.own()), invertId(posToId(pos)) }));
}

void Game::removePiece(Piece* piece) {
	piece->disable();
	netcp->push(Com::Code::kill);
	netcp->push(inversePieceId(piece));
}

void Game::updateFortress(Tile* fort, bool breached) {
	fort->setBreached(breached);
	netcp->push(vector<uint8>({ uint8(Com::Code::breach), uint8(breached) }));
	netcp->push(inverseTileId(fort));
}
#ifdef DEBUG
vector<Object*> Game::initDummyObjects() {
	vector<Object*> objs = initObjects();

	uint8 t = 0;
	vector<uint16> amts(config.tileAmounts.begin(), config.tileAmounts.end());
	for (int16 x = 0; x < config.homeWidth; x++)
		for (int16 y = 0; y < config.homeHeight; y++) {
			if (outRange(vec2s(x, y), vec2s(1, 0), vec2s(config.homeWidth - 2, config.homeHeight - 2)) || !amts[uint8(Com::Tile::fortress)]) {
				for (; !amts[t]; t++);
				tiles.own(y * config.homeWidth + x)->setType(Com::Tile(t));
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
		pieces.own(i)->pos = gtop(vec2s(i % config.homeWidth, 1 + i / config.homeWidth), BoardObject::upperPoz);
	return objs;
}
#endif
