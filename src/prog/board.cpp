#include "board.h"
#include "program.h"
#include "engine/scene.h"
#include <glm/gtc/matrix_inverse.hpp>

Board::Board(Scene* sceneSys, Settings* settings) :
	screen(vec3(Config::boardWidth / 2.f, screenYUp, Config::boardWidth / 2.f)),
	tileTops{
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), 0.f, 0.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), 0.f, 0.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), glm::pi<float>(), 0.f),
		BoardObject(gtop(svec2(UINT16_MAX), 0.0005f), glm::pi<float>(), 0.f)
	},
	pxpad(gtop(svec2(UINT16_MAX), 0.001f), vec3(0.f), vec3(0.f)),	// show indicates if destruction pad is being used
	scene(sceneSys),
	sets(settings)
{
	Mesh* meshGround = scene->mesh("ground");
	Mesh* meshTable = scene->mesh("table");
	Mesh* meshScreen = scene->mesh("screen");
	Mesh* meshPlane = scene->mesh("plane");
	Mesh* meshOutline = scene->mesh("outline");
	meshGround->allocate(1);
	meshTable->allocate(1);
	meshScreen->allocate(1);
	meshPlane->allocate(tileTops.size(), true);
	meshOutline->allocate(1);

	Object::init(meshGround, 0, vec3(Config::boardWidth / 2.f, -6.f, Config::boardWidth / 2.f), vec3(0.f), vec3(1.f), scene->material("ground"), scene->objTex("grass"));
	Object::init(meshTable, 0, vec3(Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f), vec3(0.f), vec3(1.f), scene->material("board"), scene->objTex("rock"));
	screen.init(meshScreen, 0, scene->material("screen"), scene->objTex("wall"));
	for (uint8 i = 0; i < tileTops.size(); ++i)
		tileTops[i].init(meshPlane, i, scene->material("tile"), scene->objTex(TileTop(i).name()), false);
	pxpad.init(meshOutline, 0, scene->material("red"), scene->objTex(), false);

	meshGround->updateInstanceData();
	meshTable->updateInstanceData();
	meshScreen->updateInstanceData();
	meshPlane->updateInstanceData();
	meshPlane->updateInstanceDataTop();
	meshOutline->updateInstanceData();
}

uint16 Board::initConfig(const Config& cfg) {
	uint16 piecePicksLeft = 0;
	if (config = cfg; (config.opts & Config::setPieceBattle) && config.setPieceBattleNum < config.countPieces()) {
		ownPieceAmts.fill(0);
		piecePicksLeft = config.setPieceBattleNum;
		if (config.winThrone) {
			ownPieceAmts[uint8(PieceType::throne)] += config.winThrone;
			piecePicksLeft -= config.winThrone;
		} else {
			uint16 caps = config.winFortress;
			for (uint8 i = uint8(PieceType::throne); i < pieceLim && caps; --i)
				if (config.capturers & (1 << i)) {
					uint16 diff = std::min(caps, uint16(config.pieceAmounts[i] - ownPieceAmts[i]));
					ownPieceAmts[i] += diff;
					piecePicksLeft -= diff;
					caps -= diff;
				}
		}
	} else
		ownPieceAmts = config.pieceAmounts;
	return piecePicksLeft;
}

void Board::initObjects(bool regular, bool basicInitPieces, bool initAllPieces) {
	boardHeight = config.homeSize.y * 2 + 1;
	objectSize = Config::boardWidth / float(std::max(config.homeSize.x, boardHeight));
	tilesOffset = (Config::boardWidth - objectSize * vec2(config.homeSize.x, boardHeight)) / 2.f;
	bobOffset = objectSize / 2.f + tilesOffset;
	boardBounds = vec4(tilesOffset.x, tilesOffset.y, tilesOffset.x + objectSize * float(config.homeSize.x), tilesOffset.y + objectSize * float(boardHeight));
	setBgrid();
	screen.setPos(vec3(screen.getPos().x, screen.getPos().y, Config::boardWidth / 2.f - objectSize / 2.f));
	for (BoardObject& it : tileTops) {
		it.setTrans(gtop(svec2(UINT16_MAX)), vec3(objectSize));
		it.setShow(false);
	}
	pxpad.setScl(vec3(objectSize));

	// prepare objects for setup
	tiles.update(config);
	const Material* matl = scene->material("empty");
	uvec2 tex = scene->objTex(tileNames[uint8(TileType::empty)]);
	Mesh* mesh = scene->mesh("tile");
	mesh->allocate(tiles.getSize(), true);

	setTiles(0, 0, mesh, matl, tex);
	setTiles(tiles.getHome(), config.homeSize.y, mesh, matl, tex);
	setTiles(tiles.getExtra(), config.homeSize.y + 1, mesh, matl, tex);
	mesh->updateInstanceData();
	mesh->updateInstanceDataTop();
	setMidFortressTiles();

	if (basicInitPieces) {
		pieces.update(config, regular);
		setPieces(pieces.own(), glm::pi<float>());
		setPieces(pieces.ene(), 0.f);

		Mesh* meshes[pieceNames.size()];
		for (uint8 i = 0; i < pieceNames.size(); ++i) {
			meshes[i] = scene->mesh(pieceNames[i]);
			meshes[i]->allocate(!initAllPieces ? ownPieceAmts[i] : ownPieceAmts[i] + enePieceAmts[i], true);
		}

		matl = scene->material(Settings::colorNames[uint8(sets->colorAlly)]);
		tex = scene->objTex("metal");
		uint8 t = 0;
		for (uint16 i = 0, c = 0; i < pieces.getNum(); ++i, ++c) {
			for (; c >= ownPieceAmts[t]; ++t, c = 0);
			pieces.own(i)->init(meshes[t], c, matl, tex, false, PieceType(t));
		}
		if (initAllPieces)
			initEnePieces(meshes);
		for (Mesh* it : meshes) {
			it->updateInstanceData();
			it->updateInstanceDataTop();
		}
	}
}

void Board::initEnePieces(Mesh** meshes) {
	const Material* matl = scene->material(Settings::colorNames[uint8(sets->colorEnemy)]);
	uvec2 tex = scene->objTex("metal");
	uint8 t = 0;
	for (uint16 i = 0, c = 0; i < pieces.getNum(); ++i, ++c) {
		for (; c >= enePieceAmts[t]; ++t, c = 0);
		pieces.ene(i)->init(meshes[t], enePieceAmts[t] + c, matl, tex, true, PieceType(t));
	}
}

void Board::setTiles(uint16 id, uint16 yofs, Mesh* mesh, const Material* matl, uvec2 tex) {
	for (uint16 y = yofs; y < yofs + config.homeSize.y; ++y)
		for (uint16 x = 0; x < config.homeSize.x; ++x, ++id) {
			tiles[id] = Tile(gtop(svec2(x, y)), objectSize);
			tiles[id].init(mesh, id, matl, tex, false, 0.f);
		}
}

void Board::setMidFortressTiles() {
	if ((config.opts & (Config::victoryPoints | Config::victoryPointsEquidistant)) == (Config::victoryPoints | Config::victoryPointsEquidistant)) {
		uint16 forts = config.homeSize.x - config.countMiddles() * 2;
		for (uint16 i = (config.homeSize.x - forts) / 2; i < (config.homeSize.x + forts) / 2; ++i)
			tiles.mid(i)->setType(TileType::fortress);
	}
}

void Board::setPieces(Piece* pces, float rot) {
	vec3 pos = gtop(svec2(UINT16_MAX));
	for (uint16 i = 0; i < pieces.getNum(); ++i)
		pces[i] = Piece(pos, rot, objectSize);
}

void Board::setBgrid() {
	Mesh* mesh = scene->mesh("grid");
	mesh->allocate(config.homeSize.x - 1 + boardHeight - 1);
	const Material* matl = scene->material("grid");
	uvec2 tex = scene->objTex("grid");

	uint i = 0;
	for (uint x = 1; x < config.homeSize.x; ++x, ++i)
		Object::init(mesh, i, vec3(float(x) * objectSize + tilesOffset.x, 0.f, 0.f), vec3(0.f), vec3(objectSize, 1.f, Config::boardWidth), matl, tex);
	for (uint y = 1; y < boardHeight; ++y, ++i)
		Object::init(mesh, i, vec3(0.f, 0.f, float(y) * objectSize + tilesOffset.y), vec3(0.f, glm::half_pi<float>(), 0.f), vec3(objectSize, 1.f, Config::boardWidth), matl, tex);
	mesh->updateInstanceData();
}

void Board::uninitObjects() {
	setTilesInteract(tiles.begin(), tiles.getSize(), Tile::Interact::ignore);
	disableOwnPiecesInteract(false);
}

uint16 Board::countAvailableFavors() {
	uint16 flim = config.favorLimit * 4;
	uint16 availFF = 0;
	for (Piece* throne = getOwnPieces(PieceType::throne); throne != pieces.ene(); ++throne)
		if (uint16 pos = posToId(ptog(throne->getPos())); tiles[pos].getType() == TileType::fortress)
			if (throne->lastFortress = pos; availFF < flim)
				++availFF;
	return availFF;
}

void Board::prepareMatch(bool myTurn, TileType* buf) {
	for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
		it->setShow(pieceOnBoard(it));
	for (BoardObject& it : tileTops)
		it.setShow(false);

	// rearrange middle tiles
	vector<TileType> mid(config.homeSize.x);
	for (uint16 i = 0; i < config.homeSize.x; ++i) {
		if (tiles.mid(i)->getType() == TileType::empty && buf[i] != TileType::empty) {
			mid[i] = buf[i];
			buf[i] = TileType::empty;
		} else
			mid[i] = tiles.mid(i)->getType();
	}
	for (uint16 i = myTurn ? 0 : config.homeSize.x - 1, fm = btom<uint16>(myTurn); i < config.homeSize.x; i += fm)
		if (mid[i] < TileType::fortress && buf[i] < TileType::fortress) {
			TileType val = mid[i];
			mid[i] = TileType::empty;
			uint16 a = findEmptyMiddle(mid, i, -1), b = findEmptyMiddle(mid, i, 1);
			if (a == b)
				(myTurn ? a : b) = i;
			mid[a] = val;
			mid[b] = buf[i];
		}
	for (uint16 i = 0; i < config.homeSize.x; ++i)
		tiles.mid(i)->setType(mid[i] != TileType::empty ? mid[i] : TileType::fortress);
}

uint16 Board::findEmptyMiddle(const vector<TileType>& mid, uint16 i, int16 m) const {
	for (i += m; i < config.homeSize.x && mid[i] != TileType::empty; i += m);
	if (i >= config.homeSize.x)
		for (i = m > 0 ? 0 : config.homeSize.x - 1; i < config.homeSize.x && mid[i] != TileType::empty; i += m);
	return i;
}

void Board::prepareTurn(bool myTurn, bool xmov, bool fcont, Record& orec, Record& erec) {
	if (!(xmov || fcont) && orec.info != Record::battleFail) {
		for (Piece& it : pieces)
			it.setAlphaFactor(1.f);
		for (auto& [pce, prt] : (myTurn ? erec : orec).protects)
			pce->setAlphaFactor(BoardObject::noEngageAlpha);

		// restore fortresses
		if (!(config.opts & Config::homefront))
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
			it.setInteractivity(it.getShow(), true, nullptr, nullptr, nullptr);
		erec.lastAct.first->setInteractivity(true, false, &Program::eventPieceStart, &Program::eventMove, nullptr );
	} else {
		for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
			it->setInteractivity(it->getShow(), false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
			it->setInteractivity(it->getShow(), false, nullptr, nullptr, nullptr);
	}
}

void Board::setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim) {
	for (uint16 i = 0; i < num; ++i)
		tiles[i].setInteractivity(lvl, dim);
}

void Board::setMidTilesInteract(Tile::Interact lvl, bool dim) {
	for (Tile* it = tiles.mid(); it != tiles.own(); ++it)
		it->setInteractivity(it->getType() != TileType::fortress ? lvl : Tile::Interact::ignore, dim || it->getType() == TileType::fortress);
}

void Board::setOwnPiecesVisible(bool on) {
	for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
		if (it->setInteractivity(on, false, nullptr, on ? &Program::eventMovePiece : nullptr, nullptr); pieceOnHome(it))
			it->setActive(on);
}

void Board::disableOwnPiecesInteract(bool rigid, bool dim) {
	for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
		it->setInteractivity(rigid, dim, nullptr, nullptr, nullptr);
}

void Board::restorePiecesInteract(const Record& orec) {
	if (orec.actionsExhausted()) {	// for when there's FFs
		for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
			it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
	} else if (orec.actors.size() == 1 && (orec.actors.begin()->second & ACT_MS) == ACT_MS) {	// for moving and switching a warhorse
		for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
			it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
		orec.actors.begin()->first->setInteractivity(orec.actors.begin()->first->getShow(), false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
	} else {	// business as usual
		if (orec.actors.size() >= 2) {	// for pieces with more than a total of two actions
			for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
				it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->getShow(), false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		} else for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
			it->setInteractivity(it->getShow(), false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		for (auto [pce, act] : orec.assault)
			pce->setInteractivity(pce->getShow(), true, nullptr, nullptr, nullptr);
	}
	for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
		it->setInteractivity(it->getShow(), false, nullptr, nullptr, nullptr);
}

vector<uint16> Board::countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt) {
	for (uint16 i = 0; i < num; ++i)
		if (uint8(tiles[i].getType()) < cnt.size())
			--cnt[uint8(tiles[i].getType())];
	return cnt;
}

void Board::checkOwnTiles() const {
	uint16 fort = 0;
	for (uint16 y = config.homeSize.y + 1; y < boardHeight; ++y) {
		uint8 cnt[tileLim] = { 0, 0, 0, 0 };	// collect information and check if the fortress isn't touching a border
		for (uint16 x = 0; x < config.homeSize.x; ++x) {
			if (TileType type = tiles[y * config.homeSize.x + x].getType(); type < TileType::fortress)
				++cnt[uint8(type)];
			else if (++fort; outRange(svec2(x, y), svec2(1, config.homeSize.y + 1), boardLimit() - svec2(1)))
				throw firstUpper(tileNames[uint8(TileType::fortress)]) + " at " + toStr(x) + '|' + toStr(y - config.homeSize.y - 1) + " not allowed";
		}

		if (config.opts & Config::rowBalancing)	// check diversity in each row
			for (uint8 i = 0; i < tileLim; ++i)
				if (!cnt[i] && config.tileAmounts[i])
					throw firstUpper(tileNames[i]) + " missing in row " + toStr(y - config.homeSize.y - 1);
	}
	if (fort != config.countFreeTiles())
		throw string("Not all tiles were placed");
}

void Board::checkMidTiles() const {
	// collect information
	uint8 cnt[tileLim] = { 0, 0, 0, 0 };
	for (uint16 i = 0; i < config.homeSize.x; ++i)
		if (TileType type = tiles.mid(i)->getType(); type < TileType::fortress)
			++cnt[uint8(type)];
	// check if all tiles were placed
	for (uint8 i = 0; i < tileLim; ++i)
		if (cnt[i] < config.middleAmounts[i])
			throw firstUpper(tileNames[i]) + " wasn't placed";
}

void Board::checkOwnPieces() const {
	uint16 forts = config.countFreeTiles();
	for (const Piece* it = pieces.own(); it != pieces.ene(); ++it)
		if (!it->getShow() && !(it->getType() == PieceType::dragon && (config.opts & Config::dragonLate) && forts))
			throw firstUpper(pieceNames[uint8(it->getType())]) + " wasn't placed";
}

vector<uint16> Board::countOwnPieces() const {
	vector<uint16> cnt(ownPieceAmts.begin(), ownPieceAmts.end());
	for (const Piece* it = pieces.own(); it != pieces.ene(); ++it)
		if (pieceOnHome(it))
			--cnt[uint8(it->getType())];
	return cnt;
}

Piece* Board::getPieces(Piece* beg, const array<uint16, pieceLim>& amts, PieceType type) {
	for (uint8 t = 0; t < uint8(type); ++t)
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
	for (uint16 (*const mov)(uint16, svec2) : adjacentIndex) {
		uint16 p = pos;
		for (uint16 i = 0; i < dlim; ++i)
			if (uint16 ni = mov(p, boardLimit()); ni < tiles.getSize())
				if (tcol.insert(p = ni); !stepable(ni, this))
					break;
	}
}

void Board::collectTilesByArea(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*)) {
	vector<uint16> dist = Dijkstra::travelDist(pos, dlim, boardLimit(), stepable, this);
	for (uint16 i = 0; i < tiles.getSize(); ++i)
		if (dist[i] <= dlim)
			tcol.insert(i);
}

void Board::collectTilesByType(uset<uint16>& tcol, uint16 pos, bool (*stepable)(uint16, void*)) {
	collectAdjacentTilesByType(tcol, pos, tiles[pos].getType(), stepable);
	collectTilesBySingle(tcol, pos);
}

void Board::collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, TileType type, bool (*stepable)(uint16, void*)) {
	tcol.insert(pos);
	for (uint16 (*const mov)(uint16, svec2) : adjacentIndex)
		if (uint16 ni = mov(pos, boardLimit()); ni < tiles.getSize() && tiles[ni].getType() == type && !tcol.count(ni) && stepable(ni, this))
			collectAdjacentTilesByType(tcol, ni, type, stepable);
}

void Board::collectTilesByPorts(uset<uint16>& tcol, uint16 pos) {
	if (svec2 p = idToPos(pos); (config.opts & Config::ports) && tiles[pos].getType() == TileType::water && (!p.x || p.x == config.homeSize.x - 1 || !p.y || p.y == boardHeight - 1)) {
		for (uint16 b : { 0, tiles.getSize() - config.homeSize.x })
			for (uint16 i = 0; i < config.homeSize.x; ++i)
				if (uint16 id = b + i; tiles[id].getType() == TileType::water)
					tcol.insert(id);
		for (uint16 b = config.homeSize.x; b < tiles.getSize() - config.homeSize.x; b += config.homeSize.x)
			for (uint16 i : { 0, config.homeSize.x - 1 })
				if (uint16 id = b + i; tiles[id].getType() == TileType::water)
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
		for (svec2 dst = pos + mov * i; i <= dist.second && inRange(dst, svec2(0), boardLimit()); ++i, dst += mov)
			tcol.insert(posToId(dst));
	}
}

bool Board::spaceAvailableAny(uint16, void*) {
	return true;
}

bool Board::spaceAvailableGround(uint16 pos, void* board) {
	return static_cast<Board*>(board)->tiles[pos].getType() != TileType::water;
}

bool Board::spaceAvailableDragon(uint16 pos, void* board) {
	Board* self = static_cast<Board*>(board);
	Piece* occ = self->findPiece(self->pieces.begin(), self->pieces.end(), self->idToPos(pos));
	return !occ || self->isOwnPiece(occ) || (occ->getType() != PieceType::dragon && !occ->firingArea().first);
}

void Board::highlightMoveTiles(const Piece* pce, const Record& erec, Favor favor) {
	for (Tile& it : tiles)
		it.setEmission(it.getEmission() & ~BoardObject::EMI_HIGH);
	if (pce)
		for (uint16 id : collectMoveTiles(pce, erec, favor))
			tiles[id].setEmission(tiles[id].getEmission() | BoardObject::EMI_HIGH);
}

void Board::highlightEngageTiles(const Piece* pce) {
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
	else if (piece->getType() == PieceType::spearmen && tiles[pos].getType() == TileType::water)
		collectTilesByType(tcol, pos, spaceAvailableAny);
	else if (piece->getType() == PieceType::lancer && tiles[pos].getType() == TileType::plains)
		collectTilesForLancer(tcol, pos);
	else if (piece->getType() == PieceType::dragon)
		(this->*(config.opts & Config::dragonStraight ? &Board::collectTilesByStraight : &Board::collectTilesByArea))(tcol, pos, dragonDist, spaceAvailableDragon);
	else
		collectTilesBySingle(tcol, pos);
	return tcol;
}

uset<uint16> Board::collectEngageTiles(const Piece* piece) {
	uset<uint16> tcol;
	if (pair<uint8, uint8> farea = piece->firingArea(); farea.first)
		collectTilesByDistance(tcol, ptog(piece->getPos()), piece->firingArea());
	else if (piece->getType() == PieceType::dragon)
		collectTilesByStraight(tcol, posToId(ptog(piece->getPos())), dragonDist, spaceAvailableDragon);
	else
		collectTilesBySingle(tcol, posToId(ptog(piece->getPos())));
	return tcol;
}

void Board::fillInFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); ++it)
		if (it->getType() == TileType::empty)
			it->setType(TileType::fortress);
}

void Board::takeOutFortress() {
	for (Tile* it = tiles.own(); it != tiles.end(); ++it)
		if (it->getType() == TileType::fortress)
			it->setType(TileType::empty);
}

void Board::setFavorInteracts(Favor favor, const Record& orec) {
	switch (favor) {
	case Favor::hasten: {
		if (orec.actors.size() >= 2 || std::any_of(orec.actors.begin(), orec.actors.end(), [](const pair<const Piece*, Action>& it) -> bool { return it.second & ACT_SWAP; })) {
			for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
				it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->getShow(), false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		} else {
			for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
				it->setInteractivity(it->getShow(), false, &Program::eventPieceStart, &Program::eventMove, nullptr);
			for (auto [pce, act] : orec.assault)
				pce->setInteractivity(pce->getShow(), true, nullptr, nullptr, nullptr);
		}
		for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
			it->setInteractivity(it->getShow(), false, nullptr, nullptr, nullptr);
		break; }
	case Favor::assault:
		if (umap<Piece*, Action>::const_iterator pa = orec.assault.find(orec.lastAss.first); pa == orec.assault.end()) {
			for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
				it->setInteractivity(it->getShow(), false, &Program::eventPieceStart, &Program::eventMove, nullptr);
			for (auto [pce, act] : orec.actors)
				pce->setInteractivity(pce->getShow(), true, nullptr, nullptr, nullptr);
			for (auto [pce, act] : orec.assault)
				if ((act & ACT_MS) == ACT_MS)
					pce->setInteractivity(pce->getShow(), true, nullptr, nullptr, nullptr);
		} else {
			for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
				it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
			pa->first->setInteractivity(pa->first->getShow(), false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		}
		for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
			it->setInteractivity(it->getShow(), false, nullptr, nullptr, nullptr);
		break;
	case Favor::conspire:
		for (Piece& it : pieces) {
			umap<Piece*, bool>::const_iterator prot = orec.protects.find(&it);
			bool dim = prot != orec.protects.end() && prot->second;
			it.setInteractivity(it.getShow(), dim, !dim ? &Program::eventPieceNoEngage : nullptr, nullptr, nullptr);
		}
		break;
	case Favor::deceive:
		for (Piece* it = pieces.own(); it != pieces.ene(); ++it)
			it->setInteractivity(it->getShow(), true, nullptr, nullptr, nullptr);
		for (Piece* it = pieces.ene(); it != pieces.end(); ++it)
			it->setInteractivity(it->getShow(), false, &Program::eventPieceStart, &Program::eventMove, nullptr);
		break;
	case Favor::none:
		restorePiecesInteract(orec);
	}
}

void Board::setPxpadPos(const Piece* piece) {
	if (pxpad.setShow(piece && piece->getType() == PieceType::rangers && getTile(ptog(piece->getPos()))->getType() == TileType::forest); pxpad.getShow())
		pxpad.setPos(vec3(piece->getPos().x, pxpad.getPos().y, piece->getPos().z));
}

TileTop Board::findTileTop(const Tile* tile) {
	svec2 pos = ptog(tile->getPos());
	return std::find_if(tileTops.begin(), tileTops.end(), [this, pos](const Object& it) -> bool { return ptog(it.getPos()) == pos; }) - tileTops.begin();
}

void Board::setTileTop(TileTop top, const Tile* tile) {
	if (tile) {
		tileTops[top].setPos(vec3(tile->getPos().x, tileTops[top].getPos().y, tile->getPos().z));
		tileTops[top].setShow(true);
	} else {
		tileTops[top].setPos(gtop(svec2(UINT16_MAX)));
		tileTops[top].setShow(false);
	}
}

void Board::selectEstablishers() {
	for (Piece& it : pieces)
		it.setInteractivity(false, true, nullptr, nullptr, nullptr);
	for (Piece* it = getOwnPieces(PieceType::throne); it != pieces.ene(); ++it)
		if (it->getShow())
			it->setInteractivity(true, false, &Program::eventEstablish, nullptr, nullptr);
}

pair<Tile*, TileTop> Board::checkTileEstablishable(const Piece* throne) {
	svec2 pos = ptog(throne->getPos());
	if (config.opts & Config::terrainRules)
		for (Tile& it : tiles)
			if (TileTop top = findTileTop(&it); (it.getType() == TileType::fortress && (&it < tiles.mid() || &it >= tiles.own())) || top != TileTop::none)
				if (svec2 dp = glm::abs(ivec2(ptog(it.getPos())) - ivec2(pos)); dp.x < 3 && dp.y < 3)
					throw "Tile is too close to a " + string(top == TileTop::none ? tileNames[uint8(it.getType())] : top.name());
	return pair(getTile(pos), tileTops[TileTop::ownFarm].getShow() ? TileTop::ownCity : TileTop::ownFarm);
}

bool Board::tileRebuildable(const Piece* throne) {
	if (throne->getShow())
		if (Tile* til = getTile(ptog(throne->getPos())); (til->getType() == TileType::fortress || findTileTop(til) == TileTop::ownFarm) && til->getBreached())
			return true;
	return false;
}

void Board::selectRebuilders() {
	for (Piece& it : pieces)
		it.setInteractivity(false, true, nullptr, nullptr, nullptr);
	for (Piece* it = getOwnPieces(PieceType::throne); it != pieces.ene(); ++it)
		if (tileRebuildable(it))
			it->setInteractivity(true, false, &Program::eventRebuildTile, nullptr, nullptr);
}

bool Board::pieceSpawnable(PieceType type) {
	switch (type) {
	case PieceType::rangers: case PieceType::lancer:
		if (!tileTops[TileTop::ownFarm].getShow())
			return false;
		if (Tile* til = getTileBot(TileTop::ownFarm); til->getBreached() || findOccupant(til))
			return false;
		break;
	case PieceType::spearmen: case PieceType::catapult: case PieceType::elephant:
		if (!tileTops[TileTop::ownCity].getShow() || findOccupant(getTileBot(TileTop::ownCity)))
			return false;
		break;
	case PieceType::crossbowmen: case PieceType::trebuchet: case PieceType::warhorse:
		if (std::none_of(tiles.own(), tiles.end(), [](const Tile& dt) -> bool { return dt.isUnbreachedFortress(); }))
			return false;
		break;
	default:
		return false;
	}

	for (Piece* it = getOwnPieces(type); it->getType() == type; ++it)
		if (!it->getShow())
			return true;
	return false;
}

void Board::selectSpawners() {
	for (Tile* it = tiles.ene(); it != tiles.own(); ++it)
		it->setEmission(it->getEmission() | BoardObject::EMI_DIM);
	for (Tile* it = tiles.own(); it != tiles.end(); ++it) {
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

Tile* Board::findSpawnableTile(PieceType type) {
	switch (type) {
	case PieceType::rangers: case PieceType::lancer:
		return getTileBot(TileTop::ownFarm);
	case PieceType::spearmen: case PieceType::catapult: case PieceType::elephant:
		return getTileBot(TileTop::ownCity);
	case PieceType::crossbowmen: case PieceType::trebuchet: case PieceType::warhorse:
		return std::find_if(tiles.own(), tiles.end(), [](const Tile& it) -> bool { return it.isUnbreachedFortress(); });
	}
	return nullptr;
}

Piece* Board::findSpawnablePiece(PieceType type) {
	for (Piece* it = getOwnPieces(type); it->getType() == type; ++it)
		if (!it->getShow())
			return it;
	return nullptr;
}

void Board::resetTilesAfterSpawn() {
	for (Tile& ti : tiles) {
		ti.hgcall = nullptr;
		ti.setEmission(ti.getEmission() & ~BoardObject::EMI_DIM);
	}
	for (BoardObject& tt : tileTops)
		tt.setEmission(tt.getEmission() & ~BoardObject::EMI_DIM);
}

bool Board::checkThroneWin(Piece* pcs, const array<uint16, pieceLim>& amts) {
	if (uint16 c = config.winThrone) {
		pcs = getPieces(pcs, amts, PieceType::throne);
		for (uint16 i = 0; i < amts[uint8(PieceType::throne)]; ++i)
			if (!pcs[i].getShow() && !--c)
				return true;
	} else
		return std::none_of(pcs, pcs + pieces.getNum(), [](Piece& it) -> bool { return it.getShow(); });	// check if all pieces dead
	return false;
}

bool Board::checkFortressWin(const Tile* tit, const Piece* pit, const array<uint16, pieceLim>& amts) const {
	if (uint16 cnt = config.winFortress)									// if there's a fortress quota
		for (const Tile* tend = tit + tiles.getHome(); tit != tend; ++tit)	// iterate homeland tiles
			if (tit->getType() == TileType::fortress)						// if tile is an enemy fortress
				for (uint8 pi = 0; pi < pieceLim; pit += amts[pi++])		// iterate piece types
					if (config.capturers & (1 << pi))						// if the piece type is a capturer
						for (uint16 i = 0; i < amts[pi]; ++i)				// iterate board.getPieces() of that type
							if (tileId(tit) == posToId(ptog(pit[i].getPos())) && !--cnt)	// if such a piece is on the fortress
								return true;								// decrement fortress counter and win if 0
	return false;
}

Record::Info Board::countVictoryPoints(uint16& own, uint16& ene, const Record& erec) {
	if ((config.opts & Config::victoryPoints) && erec.info != Record::battleFail) {
		for (Tile* it = tiles.mid(); it != tiles.own(); ++it)
			if (it->getType() == TileType::fortress)
				if (Piece* pce = findOccupant(it))
					isOwnPiece(pce) ? ++own : ++ene;
		if (own >= config.victoryPointsNum || ene >= config.victoryPointsNum)
			return own > ene ? Record::win : own < ene ? Record::loose : Record::tie;
	}
	return Record::none;
}

void Board::updateTileInstances(Tile* til, Mesh* old) {
	Mesh::Instance ins = old->erase(til->meshIndex);
	for (Tile& it : tiles)
		if (it.getMesh() == old && it.meshIndex > til->meshIndex)
			--it.meshIndex;
	til->meshIndex = til->getMesh()->insert(ins);
	old->updateInstanceData();
	til->getMesh()->updateInstanceData();
}

#ifndef NDEBUG
void Board::initDummyObjects() {
	initObjects(true);

	uint16 t = 0;
	vector<uint16> amts(config.tileAmounts.begin(), config.tileAmounts.end());
	uint16 fort = config.countFreeTiles();
	for (uint16 x = 0; x < config.homeSize.x; ++x)
		for (uint16 y = config.homeSize.y + 1; y < boardHeight; ++y) {
			if (!fort || outRange(svec2(x, y), svec2(1, config.homeSize.y + 1), boardLimit() - svec2(1))) {
				for (; !amts[t]; ++t);
				tiles[y * config.homeSize.x + x].setType(TileType(t));
				--amts[t];
			} else
				--fort;
		}

	t = 0;
	amts.assign(config.middleAmounts.begin(), config.middleAmounts.end());
	for (sizet i = 0; t < amts.size(); ++i) {
		for (; t < amts.size() && !amts[t]; ++t);
		if (t < amts.size() && tiles.mid(pdift(i))->getType() == TileType::empty) {
			tiles.mid(pdift(i))->setType(TileType(t));
			--amts[t];
		}
	}

	for (uint16 i = 0; i < pieces.getNum(); ++i)
		pieces.own(i)->setPos(gtop(svec2(i % config.homeSize.x, config.homeSize.y + 1 + i / config.homeSize.x)));
}
#endif
