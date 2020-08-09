#include "program.h"
#include "engine/scene.h"

Board::Board(const Scene* scene) :
	ground(vec3(Com::Config::boardWidth / 2.f, -6.f, Com::Config::boardWidth / 2.f), vec3(0.f), vec3(1.f), scene->mesh("ground"), scene->material("ground"), scene->texture("grass")),
	board(vec3(Com::Config::boardWidth / 2.f, 0.f, Com::Config::boardWidth / 2.f), vec3(0.f), vec3(1.f), scene->mesh("table"), scene->material("board"), scene->texture("rock")),
	bgrid(vec3(0.f), vec3(0.f), vec3(1.f), &gridat, scene->material("grid"), scene->texture()),
	screen(vec3(Com::Config::boardWidth / 2.f, screenYUp, Com::Config::boardWidth / 2.f), vec3(0.f), vec3(1.f), scene->mesh("screen"), scene->material("screen"), scene->texture("wall")),
	tileTops{
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), 0.f, 0.f, scene->mesh("plane"), scene->material("tile"), scene->texture(TileTop(TileTop::ownFarm).name()), false, 1.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), 0.f, 0.f, scene->mesh("plane"), scene->material("tile"), scene->texture(TileTop(TileTop::ownCity).name()), false, 1.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), PI, 0.f, scene->mesh("plane"), scene->material("tile"), scene->texture(TileTop(TileTop::eneFarm).name()), false, 1.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), PI, 0.f, scene->mesh("plane"), scene->material("tile"), scene->texture(TileTop(TileTop::eneCity).name()), false, 1.f)
	},
	pxpad(gtop(svec2(UINT16_MAX), 0.001f), vec3(0.f), vec3(0.f), scene->mesh("outline"), scene->material("red"), scene->texture(), false, false)	// show indicates if destruction pad is being used
{}

void Board::updateConfigValues() {
	boardHeight = config.homeSize.y * 2 + 1;
	objectSize = Com::Config::boardWidth / float(std::max(config.homeSize.x, boardHeight));
	tilesOffset = (Com::Config::boardWidth - objectSize * vec2(config.homeSize.x, boardHeight)) / 2.f;
	bobOffset = objectSize / 2.f + tilesOffset;
	boardBounds = vec4(tilesOffset.x, tilesOffset.y, tilesOffset.x + objectSize * float(config.homeSize.x), tilesOffset.y + objectSize * float(boardHeight));
}

vector<Object*> Board::initObjects(const Com::Config& cfg, bool regular, const Settings* sets, const Scene* scene) {
	config = cfg;
	updateConfigValues();
	tiles.update(config);
	pieces.update(config, regular);
	setBgrid();
	screen.setPos(vec3(screen.getPos().x, screen.getPos().y, Com::Config::boardWidth / 2.f - objectSize / 2.f));
	for (BoardObject& it : tileTops) {
		it.setPos(gtop(svec2(UINT16_MAX)));
		it.setScl(vec3(objectSize));
		it.show = false;
	}
	pxpad.setScl(vec3(objectSize));

	// prepare objects for setup
	setTiles(tiles.ene(), 0, false);
	setMidTiles();
	setTiles(tiles.own(), config.homeSize.y + 1, true);
	setPieces(pieces.own(), PI, scene->material(Settings::colorNames[uint8(sets->colorAlly)]));
	setPieces(pieces.ene(), 0.f, scene->material(Settings::colorNames[uint8(sets->colorEnemy)]));

	// collect array of references to all objects
	initlist<Object*> others = { &ground, &board, &bgrid, &screen };
	vector<Object*> objs(others.size() + tiles.getSize() + tileTops.size() + 1 + pieces.getSize());
	vector<Object*>::iterator oi = std::copy(others.begin(), others.end(), objs.begin());
	setObjectAddrs(tiles.begin(), tiles.getSize(), oi);
	setObjectAddrs(tileTops.begin(), tileTops.size(), oi);
	*oi++ = &pxpad;
	setObjectAddrs(pieces.begin(), pieces.getSize(), oi);
	return objs;
}

template <class S>
void Board::setObjectAddrs(S src, sizet size, vector<Object*>::iterator& dst) {
	for (sizet i = 0; i < size; i++)
		*dst++ = &src[i];
}

void Board::setTiles(Tile* tils, uint16 yofs, bool show) {
	sizet id = 0;
	for (uint16 y = yofs; y < yofs + config.homeSize.y; y++)
		for (uint16 x = 0; x < config.homeSize.x; x++)
			tils[id++] = Tile(gtop(svec2(x, y)), objectSize, show);
}

void Board::setMidTiles() {
	for (uint16 i = 0; i < config.homeSize.x; i++)
		*tiles.mid(i) = Tile(gtop(svec2(i, config.homeSize.y)), objectSize, true);
	if ((config.opts & (Com::Config::victoryPoints | Com::Config::victoryPointsEquidistant)) == (Com::Config::victoryPoints | Com::Config::victoryPointsEquidistant)) {
		uint16 forts = config.homeSize.x - config.countMiddles() * 2;
		for (uint16 i = (config.homeSize.x - forts) / 2; i < (config.homeSize.x + forts) / 2; i++)
			tiles.mid(i)->setType(Com::Tile::fortress);
	}
}

void Board::setPieces(Piece* pces, float rot, const Material* matl) {
	vec3 pos = gtop(svec2(UINT16_MAX));
	for (uint16 i = 0; i < pieces.getNum(); i++)
		pces[i] = Piece(pos, rot, objectSize, matl);
}

void Board::initOwnPieces() {
	uint8 t = 0;
	for (uint16 i = 0, c = 0; i < pieces.getNum(); i++, c++) {
		for (; c >= ownPieceAmts[t]; t++, c = 0);
		pieces.own(i)->setType(Com::Piece(t));
	}
}

void Board::setBgrid() {
	gridat.free();
	vector<Vertex> verts((config.homeSize.x + boardHeight + 2) * 2);
	vector<uint16> elems(verts.size());

	for (sizet i = 0; i < elems.size(); i++)
		elems[i] = uint16(i);
	uint16 i = 0;
	for (uint16 x = 0; x <= config.homeSize.x; x++)
		for (uint16 y : { uint16(0), boardHeight })
			verts[i++] = Vertex(gtop(svec2(x, y), -0.018f) + vec3(-(objectSize / 2.f), 0.f, -(objectSize / 2.f)), Camera::up, vec2(0.f));
	for (uint16 y = 0; y <= boardHeight; y++)
		for (uint16 x : { uint16(0), config.homeSize.x })
			verts[i++] = Vertex(gtop(svec2(x, y), -0.018f) + vec3(-(objectSize / 2.f), 0.f, -(objectSize / 2.f)), Camera::up, vec2(0.f));
	gridat = Mesh(verts, elems, GL_LINES);
}

void Board::uninitObjects() {
	setTilesInteract(tiles.begin(), tiles.getSize(), Tile::Interact::ignore);
	disableOwnPiecesInteract(false);
}

uint16 Board::countAvailableFavors() {
	uint16 flim = config.favorLimit * 4;
	uint16 availFF = 0;
	for (Piece* throne = getOwnPieces(Com::Piece::throne); throne != pieces.ene(); throne++)
		if (uint16 pos = posToId(ptog(throne->getPos())); tiles[pos].getType() == Com::Tile::fortress)
			if (throne->lastFortress = pos; availFF < flim)
				availFF++;
	return availFF;
}

void Board::prepareMatch(bool myTurn, Com::Tile* buf) {
	// set interactivity and reassign callback events
	for (Tile* it = tiles.ene(); it != tiles.mid(); it++)
		it->show = true;
	for (Piece* it = pieces.ene(); it != pieces.end(); it++)
		it->show = pieceOnBoard(it);
	for (Object& it : tileTops)
		it.show = false;

	// rearrange middle tiles
	vector<Com::Tile> mid(config.homeSize.x);
	for (uint16 i = 0; i < config.homeSize.x; i++)
		mid[i] = tiles.mid(i)->getType();
	rearangeMiddle(mid.data(), buf, myTurn);
	for (uint16 i = 0; i < config.homeSize.x; i++)
		tiles.mid(i)->setType(mid[i]);
}

void Board::rearangeMiddle(Com::Tile* mid, Com::Tile* buf, bool myTurn) {
	for (uint16 i = 0; i < config.homeSize.x; i++)
		if (mid[i] == Com::Tile::empty && buf[i] != Com::Tile::empty) {
			mid[i] = buf[i];
			buf[i] = Com::Tile::empty;
		}

	for (uint16 i = myTurn ? 0 : config.homeSize.x - 1, fm = btom<uint16>(myTurn); i < config.homeSize.x; i += fm)
		if (mid[i] < Com::Tile::fortress && buf[i] < Com::Tile::fortress) {
			Com::Tile val = mid[i];
			mid[i] = Com::Tile::empty;
			uint16 a = findEmptyMiddle(mid, i, uint16(-1)), b = findEmptyMiddle(mid, i, 1);
			if (a == b)
				(myTurn ? a : b) = i;
			mid[a] = val;
			mid[b] = buf[i];
		}
	std::replace(mid, mid + config.homeSize.x, Com::Tile::empty, Com::Tile::fortress);
}

uint16 Board::findEmptyMiddle(Com::Tile* mid, uint16 i, uint16 m) {
	for (i += m; i < config.homeSize.x && mid[i] != Com::Tile::empty; i += m);
	if (i >= config.homeSize.x)
		for (i = m <= INT16_MAX ? 0 : config.homeSize.x - 1; i < config.homeSize.x && mid[i] != Com::Tile::empty; i += m);
	return i;
}

void Board::prepareTurn(bool myTurn, bool xmov, bool fcont, Record& orec, Record& erec) {
	if (!(xmov || fcont) && orec.info != Record::battleFail) {
		for (Piece& it : pieces)
			it.alphaFactor = 1.f;
		for (auto& [pce, prt] : (myTurn ? erec : orec).protects)
			pce->alphaFactor = BoardObject::noEngageAlpha;

		// restore fortresses
		if (!(config.opts & Com::Config::homefront))
			for (Tile& til : tiles)
				if (til.isBreachedFortress())
					if (!findPiece(pieces.begin(), pieces.end(), ptog(til.getPos())))
						til.setBreached(false);
	}

	if (!myTurn) {
		for (Piece& it : pieces)
			it.setInteractivity(false, orec.info == Record::battleFail, nullptr, nullptr, nullptr);
	} else if (xmov) {
		for (Piece& it : pieces)
			it.setInteractivity(it.show, true, nullptr, nullptr, nullptr);
		erec.lastAct.first->setInteractivity(true, false, &Program::eventPieceStart, &Program::eventMove, nullptr );
	} else {
		for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			it->setInteractivity(it->show, false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		for (Piece* it = pieces.ene(); it != pieces.end(); it++)
			it->setInteractivity(it->show, false, nullptr, nullptr, nullptr);
	}
}

void Board::setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim) {
	for (uint16 i = 0; i < num; i++)
		tiles[i].setInteractivity(lvl, dim);
}

void Board::setMidTilesInteract(Tile::Interact lvl, bool dim) {
	for (Tile* it = tiles.mid(); it != tiles.own(); it++)
		it->setInteractivity(it->getType() != Com::Tile::fortress ? lvl : Tile::Interact::ignore, dim || it->getType() == Com::Tile::fortress);
}

void Board::setOwnPiecesVisible(bool on) {
	for (Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (it->setInteractivity(on, false, nullptr, on ? &Program::eventMovePiece : nullptr, nullptr); pieceOnHome(it))
			it->setActive(on);
}

void Board::disableOwnPiecesInteract(bool rigid, bool dim) {
	for (Piece* it = pieces.own(); it != pieces.ene(); it++)
		it->setInteractivity(rigid, dim, nullptr, nullptr, nullptr);
}

void Board::restorePiecesInteract(const Record& orec) {
	if (orec.actionsExhausted()) {	// for when there's FFs
		for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
	} else if (orec.actors.size() == 1 && (orec.actors.begin()->second & ACT_MS) == ACT_MS) {	// for moving and switching a warhorse
		for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
		orec.actors.begin()->first->setInteractivity(orec.actors.begin()->first->show, false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
	} else {	// business as usual
		if (orec.actors.size() >= 2) {	// for pieces with more than a total of two actions
			for (Piece* it = pieces.own(); it != pieces.ene(); it++)
				it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->show, false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		} else for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			it->setInteractivity(it->show, false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		for (auto [pce, act] : orec.assault)
			pce->setInteractivity(pce->show, true, nullptr, nullptr, nullptr);
	}
	for (Piece* it = pieces.ene(); it != pieces.end(); it++)
		it->setInteractivity(it->show, false, nullptr, nullptr, nullptr);
}

vector<uint16> Board::countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt) {
	for (uint16 i = 0; i < num; i++)
		if (uint8(tiles[i].getType()) < cnt.size())
			cnt[uint8(tiles[i].getType())]--;
	return cnt;
}

void Board::checkOwnTiles() const {
	uint16 fort = 0;
	for (uint16 y = config.homeSize.y + 1; y < boardHeight; y++) {
		uint8 cnt[Com::tileLim] = { 0, 0, 0, 0 };	// collect information and check if the fortress isn't touching a border
		for (uint16 x = 0; x < config.homeSize.x; x++) {
			if (Com::Tile type = tiles[y * config.homeSize.x + x].getType(); type < Com::Tile::fortress)
				cnt[uint8(type)]++;
			else if (fort++; outRange(svec2(x, y), svec2(1, config.homeSize.y + 1), boardLimit() - svec2(1)))
				throw firstUpper(Com::tileNames[uint8(Com::Tile::fortress)]) + " at " + toStr(x) + '|' + toStr(y - config.homeSize.y - 1) + " not allowed";
		}

		if (config.opts & Com::Config::rowBalancing)	// check diversity in each row
			for (uint8 i = 0; i < Com::tileLim; i++)
				if (!cnt[i] && config.tileAmounts[i])
					throw firstUpper(Com::tileNames[i]) + " missing in row " + toStr(y - config.homeSize.y - 1);
	}
	if (fort != config.countFreeTiles())
		throw string("Not all tiles were placed");
}

void Board::checkMidTiles() const {
	// collect information
	uint8 cnt[Com::tileLim] = { 0, 0, 0, 0 };
	for (uint16 i = 0; i < config.homeSize.x; i++)
		if (Com::Tile type = tiles.mid(i)->getType(); type < Com::Tile::fortress)
			cnt[uint8(type)]++;
	// check if all tiles were placed
	for (uint8 i = 0; i < Com::tileLim; i++)
		if (cnt[i] < config.middleAmounts[i])
			throw firstUpper(Com::tileNames[i]) + " wasn't placed";
}

void Board::checkOwnPieces() const {
	uint16 forts = config.countFreeTiles();
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (!it->show && !(it->getType() == Com::Piece::dragon && (config.opts & Com::Config::dragonLate) && forts))
			throw firstUpper(Com::pieceNames[uint8(it->getType())]) + " wasn't placed";
}

vector<uint16> Board::countOwnPieces() const {
	vector<uint16> cnt(ownPieceAmts.begin(), ownPieceAmts.end());
	for (const Piece* it = pieces.own(); it != pieces.ene(); it++)
		if (pieceOnHome(it))
			cnt[uint8(it->getType())]--;
	return cnt;
}

Piece* Board::getPieces(Piece* beg, const array<uint16, Com::pieceMax>& amts, Com::Piece type) {
	for (uint8 t = 0; t < uint8(type); t++)
		beg += amts[t];
	return beg;
}

Piece* Board::findPiece(Piece* beg, Piece* end, svec2 pos) {
	Piece* pce = std::find_if(beg, end, [this, pos](const Piece& it) -> bool { return ptog(it.getPos()) == pos; });
	return pce != pieces.end() ? pce : nullptr;
}

BoardObject* Board::findObject(const vec3& isct) {
	if (isct.x >= boardBounds.x && isct.x < boardBounds.z && isct.z >= boardBounds.y && isct.z < boardBounds.a) {
		svec2 pp = ptog(isct);
		for (Piece& it : pieces)
			if (it.rigid && ptog(it.getPos()) == pp)
				return &it;
		if (uint16 id = posToId(pp); id < tiles.getSize())
			if (Tile& it = tiles[id]; it.rigid)
				return &it;
	}
	return nullptr;
}

void Board::collectTilesByStraight(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*)) {
	tcol.insert(pos);
	for (uint16 (*mov)(uint16, svec2) : adjacentFull) {
		uint16 p = pos;
		for (uint16 i = 0; i < dlim; i++)
			if (uint16 ni = mov(p, boardLimit()); ni < tiles.getSize())
				if (tcol.insert(p = ni); !stepable(ni, this))
					break;
	}
}

void Board::collectTilesByArea(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*)) {
	vector<uint16> dist = Dijkstra::travelDist(pos, dlim, boardLimit(), stepable, adjacentFull.data(), uint8(adjacentFull.size()), this);
	for (uint16 i = 0; i < tiles.getSize(); i++)
		if (dist[i] <= dlim)
			tcol.insert(i);
}

void Board::collectTilesByType(uset<uint16>& tcol, uint16 pos, bool (*stepable)(uint16, void*)) {
	collectAdjacentTilesByType(tcol, pos, tiles[pos].getType(), stepable);
	collectTilesBySingle(tcol, pos);
}

void Board::collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, Com::Tile type, bool (*stepable)(uint16, void*)) {
	tcol.insert(pos);
	for (uint16 (*mov)(uint16, svec2) : adjacentFull)
		if (uint16 ni = mov(pos, boardLimit()); ni < tiles.getSize() && tiles[ni].getType() == type && !tcol.count(ni) && stepable(ni, this))
			collectAdjacentTilesByType(tcol, ni, type, stepable);
}

void Board::collectTilesByPorts(uset<uint16>& tcol, uint16 pos) {
	if (svec2 p = idToPos(pos); (config.opts & Com::Config::ports) && tiles[pos].getType() == Com::Tile::water && (!p.x || p.x == config.homeSize.x - 1 || !p.y || p.y == boardHeight - 1)) {
		for (uint16 b : { 0, tiles.getSize() - config.homeSize.x })
			for (uint16 i = 0; i < config.homeSize.x; i++)
				if (uint16 id = b + i; tiles[id].getType() == Com::Tile::water)
					tcol.insert(id);
		for (uint16 b = config.homeSize.x; b < tiles.getSize() - config.homeSize.x; b += config.homeSize.x)
			for (uint16 i : { 0, config.homeSize.x - 1 })
				if (uint16 id = b + i; tiles[id].getType() == Com::Tile::water)
					tcol.insert(id);
	}
}

void Board::collectTilesForLancer(uset<uint16>& tcol, uint16 pos) {
	collectTilesByType(tcol, pos, spaceAvailableAny);
	collectTilesByArea(tcol, pos, lancerDist, spaceAvailableGround);
}

void Board::collectTilesByDistance(uset<uint16>& tcol, svec2 pos, pair<uint8, uint8> dist) {
	for (svec2 mov : { svec2(-1, -1), svec2(0, -1), svec2(1, -1), svec2(1, 0), svec2(1, 1), svec2(0, 1), svec2(-1, 1), svec2(-1, 0) }) {
		uint16 i = dist.first;
		for (svec2 dst = pos + mov * i; i <= dist.second && inRange(dst, svec2(0), boardLimit()); i++, dst += mov)
			tcol.insert(posToId(dst));
	}
}

bool Board::spaceAvailableAny(uint16, void*) {
	return true;
}

bool Board::spaceAvailableGround(uint16 pos, void* board) {
	return static_cast<Board*>(board)->tiles[pos].getType() != Com::Tile::water;
}

bool Board::spaceAvailableDragon(uint16 pos, void* board) {
	Board* self = static_cast<Board*>(board);
	Piece* occ = self->findPiece(self->pieces.begin(), self->pieces.end(), self->idToPos(pos));
	return !occ || self->isOwnPiece(occ) || (occ->getType() != Com::Piece::dragon && !occ->firingArea().first);
}

void Board::highlightMoveTiles(Piece* pce, const Record& erec, Favor favor) {
	for (Tile& it : tiles)
		it.setEmission(it.getEmission() & ~BoardObject::EMI_HIGH);
	if (pce)
		for (uint16 id : collectMoveTiles(pce, erec, favor))
			tiles[id].setEmission(tiles[id].getEmission() | BoardObject::EMI_HIGH);
}

void Board::highlightEngageTiles(Piece* pce) {
	for (Tile& it : tiles)
		it.setEmission(it.getEmission() & ~BoardObject::EMI_HIGH);
	if (pce)
		for (uint16 id : collectEngageTiles(pce))
			tiles[id].setEmission(tiles[id].getEmission() | BoardObject::EMI_HIGH);
}

uset<uint16> Board::collectMoveTiles(const Piece* piece, const Record& erec, Favor favor, bool single) {
	uset<uint16> tcol;
	uint16 pos = posToId(ptog(piece->getPos()));
	if (collectTilesByPorts(tcol, pos); favor == Favor::hasten || erec.info == Record::battleFail || single)
		collectTilesBySingle(tcol, pos);
	else if (piece->getType() == Com::Piece::spearmen && tiles[pos].getType() == Com::Tile::water)
		collectTilesByType(tcol, pos, spaceAvailableAny);
	else if (piece->getType() == Com::Piece::lancer && tiles[pos].getType() == Com::Tile::plains)
		collectTilesForLancer(tcol, pos);
	else if (piece->getType() == Com::Piece::dragon)
		(this->*(config.opts & Com::Config::dragonStraight ? &Board::collectTilesByStraight : &Board::collectTilesByArea))(tcol, pos, dragonDist, spaceAvailableDragon);
	else
		collectTilesBySingle(tcol, pos);
	return tcol;
}

uset<uint16> Board::collectEngageTiles(const Piece* piece) {
	uset<uint16> tcol;
	if (pair<uint8, uint8> farea = piece->firingArea(); farea.first)
		collectTilesByDistance(tcol, ptog(piece->getPos()), piece->firingArea());
	else if (piece->getType() == Com::Piece::dragon)
		collectTilesByStraight(tcol, posToId(ptog(piece->getPos())), dragonDist, spaceAvailableDragon);
	else
		collectTilesBySingle(tcol, posToId(ptog(piece->getPos())));
	return tcol;
}

void Board::fillInFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); it++)
		if (it->getType() == Com::Tile::empty)
			it->setType(Com::Tile::fortress);
}

void Board::takeOutFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); it++)
		if (it->getType() == Com::Tile::fortress)
			it->setType(Com::Tile::empty);
}

void Board::setFavorInteracts(Favor favor, const Record& orec) {
	switch (favor) {
	case Favor::hasten: {
		if (orec.actors.size() >= 2 || std::any_of(orec.actors.begin(), orec.actors.end(), [](const pair<const Piece*, Action>& it) -> bool { return it.second & ACT_SWAP; })) {
			for (Piece* it = pieces.own(); it != pieces.ene(); it++)
				it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->show, false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		} else {
			for (Piece* it = pieces.own(); it != pieces.ene(); it++)
				it->setInteractivity(it->show, false, &Program::eventPieceStart, &Program::eventMove, nullptr);
			for (auto [pce, act] : orec.assault)
				pce->setInteractivity(pce->show, true, nullptr, nullptr, nullptr);
		}
		for (Piece* it = pieces.ene(); it != pieces.end(); it++)
			it->setInteractivity(it->show, false, nullptr, nullptr, nullptr);
		break; }
	case Favor::assault:
		if (umap<Piece*, Action>::const_iterator pa = orec.assault.find(orec.lastAss.first); pa == orec.assault.end()) {
			for (Piece* it = pieces.own(); it != pieces.ene(); it++)
				it->setInteractivity(it->show, false, &Program::eventPieceStart, &Program::eventMove, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->show, true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.assault)
				if ((act & ACT_MS) == ACT_MS)
					pce->setInteractivity(pce->show, true, nullptr, nullptr, nullptr);
		} else {
			for (Piece* it = pieces.own(); it != pieces.ene(); it++)
				it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
			pa->first->setInteractivity(pa->first->show, false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		}
		for (Piece* it = pieces.ene(); it != pieces.end(); it++)
			it->setInteractivity(it->show, false, nullptr, nullptr, nullptr);
		break;
	case Favor::conspire:
		for (Piece& it : pieces) {
			umap<Piece*, bool>::const_iterator prot = orec.protects.find(&it);
			bool dim = prot != orec.protects.end() && prot->second;
			it.setInteractivity(it.show, dim, !dim ? &Program::eventPieceNoEngage : nullptr, nullptr, nullptr);
		}
		break;
	case Favor::deceive:
		for (Piece* it = pieces.own(); it != pieces.ene(); it++)
			it->setInteractivity(it->show, true, nullptr, nullptr, nullptr);
		for (Piece* it = pieces.ene(); it != pieces.end(); it++)
			it->setInteractivity(it->show, false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		break;
	case Favor::none:
		restorePiecesInteract(orec);
	}
}

void Board::setPxpadPos(const Piece* piece) {
	if (pxpad.show = piece && piece->getType() == Com::Piece::rangers && getTile(ptog(piece->getPos()))->getType() == Com::Tile::forest)
		pxpad.setPos(vec3(piece->getPos().x, pxpad.getPos().y, piece->getPos().z));
}

TileTop Board::findTileTop(const Tile* tile) {
	svec2 pos = ptog(tile->getPos());
	return std::find_if(tileTops.begin(), tileTops.end(), [this, pos](const Object& it) -> bool { return ptog(it.getPos()) == pos; }) - tileTops.begin();
}

void Board::setTileTop(TileTop top, const Tile* tile) {
	tileTops[top].setPos(vec3(tile->getPos().x, tileTops[top].getPos().y, tile->getPos().z));
	tileTops[top].show = true;
}

void Board::selectEstablishable() {
	for (Piece& it : pieces)
		it.setInteractivity(false, true, nullptr, nullptr, nullptr);
	for (Piece* it = getOwnPieces(Com::Piece::throne); it != pieces.ene(); it++)
		if (it->show)
			it->setInteractivity(true, false, &Program::eventEstablish, nullptr, nullptr);
}

bool Board::tileRebuildable(Piece* throne) {
	if (throne->show)
		if (Tile* til = getTile(ptog(throne->getPos())); (til->getType() == Com::Tile::fortress || findTileTop(til) == TileTop::ownFarm) && til->getBreached())
			return true;
	return false;
}

void Board::selectRebuildable() {
	for (Piece& it : pieces)
		it.setInteractivity(false, true, nullptr, nullptr, nullptr);
	for (Piece* it = getOwnPieces(Com::Piece::throne); it != pieces.ene(); it++)
		if (tileRebuildable(it))
			it->setInteractivity(true, false, &Program::eventRebuildTile, nullptr, nullptr);
}

Tile* Board::establishTile(Piece* throne, bool reinit, const Record& orec, ProgMatch* match) {
	svec2 pos = ptog(throne->getPos());
	for (Tile& it : tiles)
		if (TileTop top = findTileTop(&it); (it.getType() == Com::Tile::fortress && (&it < tiles.mid() || &it >= tiles.own())) || top != TileTop::none)
			if (svec2 dp = glm::abs(ivec2(ptog(it.getPos())) - ivec2(pos)); dp.x < 3 && dp.y < 3)
				throw "Tile is too close to a " + string(top == TileTop::none ? Com::tileNames[uint8(it.getType())] : top.name());

	Tile* tile = getTile(pos);
	if (tileTops[TileTop::ownFarm].show) {
		setTileTop(TileTop::ownCity, tile);
		match->destroyEstablishIcon();
	} else
		setTileTop(TileTop::ownFarm, tile);
	if (reinit)
		restorePiecesInteract(orec);
	return tile;
}

bool Board::pieceSpawnable(Com::Piece type) {
	switch (type) {
	case Com::Piece::rangers: case Com::Piece::lancer:
		if (!tileTops[TileTop::ownFarm].show)
			return false;
		if (Tile* til = getTileBot(TileTop::ownFarm); til->getBreached() || findOccupant(til))
			return false;
		break;
	case Com::Piece::spearmen: case Com::Piece::catapult: case Com::Piece::elephant:
		if (!tileTops[TileTop::ownCity].show || findOccupant(getTileBot(TileTop::ownCity)))
			return false;
		break;
	case Com::Piece::crossbowmen: case Com::Piece::trebuchet: case Com::Piece::warhorse:
		if (std::none_of(tiles.own(), tiles.end(), [](const Tile& dt) -> bool { return dt.isUnbreachedFortress(); }))
			return false;
		break;
	default:
		return false;
	}

	for (Piece* it = getOwnPieces(type); it->getType() == type; it++)
		if (!it->show)
			return true;
	return false;
}

void Board::selectSpawners() {
	for (Tile* it = tiles.ene(); it != tiles.own(); it++)
		it->setEmission(it->getEmission() | BoardObject::EMI_DIM);
	for (Tile* it = tiles.own(); it != tiles.end(); it++) {
		if (!it->isUnbreachedFortress())
			it->setEmission(it->getEmission() | BoardObject::EMI_DIM);
		else
			it->hgcall = &Program::eventSpawnPiece;
	}
	for (BoardObject& it : tileTops)
		it.setEmission(it.getEmission() | BoardObject::EMI_DIM);
	for (Piece& it : pieces)
		it.setInteractivity(false, true, nullptr, nullptr, nullptr);
}

Tile* Board::findSpawnableTile(Com::Piece type) {
	switch (type) {
	case Com::Piece::rangers: case Com::Piece::lancer:
		return getTileBot(TileTop::ownFarm);
	case Com::Piece::spearmen: case Com::Piece::catapult: case Com::Piece::elephant:
		return getTileBot(TileTop::ownCity);
	case Com::Piece::crossbowmen: case Com::Piece::trebuchet: case Com::Piece::warhorse:
		return std::find_if(tiles.own(), tiles.end(), [](const Tile& it) -> bool { return it.isUnbreachedFortress(); });
	}
	return nullptr;
}

Piece* Board::findSpawnablePiece(Com::Piece type, bool reinit) {
	for (Piece* it = getOwnPieces(type); it->getType() == type; it++)
		if (!it->show) {
			if (reinit) {	// pieces should be reinitialized later in prepareTurn because this should be the only action in a turn
				for (Tile& ti : tiles) {
					ti.hgcall = nullptr;
					ti.setEmission(ti.getEmission() & ~BoardObject::EMI_DIM);
				}
				for (BoardObject& tt : tileTops)
					tt.setEmission(tt.getEmission() & ~BoardObject::EMI_DIM);
			}
			return it;
		}
	return nullptr;
}

bool Board::checkThroneWin(Piece* pcs, const array<uint16, Com::pieceMax>& amts) {
	if (uint16 c = config.winThrone) {
		pcs = getPieces(pcs, amts, Com::Piece::throne);
		for (uint16 i = 0; i < amts[uint8(Com::Piece::throne)]; i++)
			if (!pcs[i].show && !--c)
				return true;
	} else
		return std::none_of(pcs, pcs + pieces.getNum(), [](Piece& it) -> bool { return it.show; });	// check if all pieces dead
	return false;
}

bool Board::checkFortressWin() {
	if (uint16 cnt = config.winFortress)											// if there's a fortress quota
		for (Tile* tit = tiles.ene(); tit != tiles.mid(); tit++)					// iterate enemy tiles
			if (Piece* pit = pieces.own(); tit->getType() == Com::Tile::fortress)	// if tile is an enemy fortress
				for (uint8 pi = 0; pi < Com::pieceMax; pit += ownPieceAmts[pi++])	// iterate piece types
					if (config.capturers & (1 << pi))								// if the piece type is a capturer
						for (uint16 i = 0; i < ownPieceAmts[pi]; i++)				// iterate board.getPieces() of that type
							if (tileId(tit) == posToId(ptog(pit[i].getPos())) && !--cnt)	// if such a piece is on the fortress
								return true;										// decrement fortress counter and win if 0
	return false;
}

Record::Info Board::countVictoryPoints(uint16& own, uint16& ene, const Record& erec) {
	if ((config.opts & Com::Config::victoryPoints) && erec.info != Record::battleFail) {
		for (Tile* it = tiles.mid(); it != tiles.own(); it++)
			if (it->getType() == Com::Tile::fortress)
				if (Piece* pce = findOccupant(it))
					(isOwnPiece(pce) ? own : ene)++;
		if (own >= config.victoryPointsNum || ene >= config.victoryPointsNum)
			return own > ene ? Record::win : own < ene ? Record::loose : Record::tie;
	}
	return Record::none;
}

#ifdef DEBUG
vector<Object*> Board::initDummyObjects(const Com::Config& cfg, const Settings* sets, const Scene* scene) {
	vector<Object*> objs = initObjects(cfg, true, sets, scene);

	uint16 t = 0;
	vector<uint16> amts(config.tileAmounts.begin(), config.tileAmounts.end());
	uint16 fort = config.countFreeTiles();
	for (uint16 x = 0; x < config.homeSize.x; x++)
		for (uint16 y = config.homeSize.y + 1; y < boardHeight; y++) {
			if (!fort || outRange(svec2(x, y), svec2(1, config.homeSize.y + 1), boardLimit() - svec2(1))) {
				for (; !amts[t]; t++);
				tiles[y * config.homeSize.x + x].setType(Com::Tile(t));
				amts[t]--;
			} else
				fort--;
		}

	t = 0;
	amts.assign(config.middleAmounts.begin(), config.middleAmounts.end());
	for (sizet i = 0; t < amts.size(); i++) {
		for (; t < amts.size() && !amts[t]; t++);
		if (t < amts.size() && tiles.mid(i)->getType() == Com::Tile::empty) {
			tiles.mid(i)->setType(Com::Tile(t));
			amts[t]--;
		}
	}

	for (uint16 i = 0; i < pieces.getNum(); i++)
		pieces.own(i)->setPos(gtop(svec2(i % config.homeSize.x, config.homeSize.y + 1 + i / config.homeSize.x)));
	return objs;
}
#endif
