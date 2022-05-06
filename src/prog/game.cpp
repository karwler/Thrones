#include "board.h"
#include "netcp.h"
#include "progs.h"
#include "engine/audioSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include <ctime>

Game::Game() :
	board(new Board(World::scene(), World::sets())),
	randGen(Com::generateRandomSeed()),
	randDist(0, Config::randomLimit - 1)
{}

Game::~Game() {
	delete board;
}

void Game::finishSetup() {
	firstTurn = true;
	ownRec = eneRec = Record();
	std::fill(favorsCount.begin(), favorsCount.end(), 0);
	std::fill(favorsLeft.begin(), favorsLeft.end(), board->config.favorLimit);
	availableFF = board->countAvailableFavors();
	vpOwn = vpEne = 0;
}

void Game::prepareMatch(TileType* buf) {
	board->prepareMatch(myTurn, buf);
	if (board->config.record) {
		try {
			recWriter = std::make_unique<RecordWriter>(World::state<ProgGame>()->configName, board);
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
	}
}

void Game::finishFavor(Favor next, Favor previous) {
	if (previous != Favor::none) {
		if (lastFavorUsed) {
			--favorsCount[uint8(previous)];
			anyFavorUsed = true;
			lastFavorUsed = false;
		}
		ProgMatch* pm = World::state<ProgMatch>();
		if (pm->updateFavorIcon(previous, true); next == Favor::none)
			pm->selectFavorIcon(next);
	}
}

void Game::setNoEngage(Piece* piece) {
	ownRec.addProtect(piece, true);
	concludeAction(nullptr, ACT_NONE, Favor::conspire);
}

void Game::pieceMove(Piece* piece, svec2 dst, Piece* occupant, bool move) {
	svec2 pos = board->ptog(piece->getPos());
	Tile* stil = board->getTile(pos);
	Tile* dtil = board->getTile(dst);
	Favor favor = World::state<ProgMatch>()->favorIconSelect();
	Action action = move ? occupant ? ACT_SWAP : ACT_MOVE : ACT_ATCK;
	if (pos == dst)
		throw string();

	checkActionRecord(piece, occupant, action, favor);
	switch (action) {
	case ACT_MOVE:
		if (!board->collectMoveTiles(piece, eneRec, favor).count(board->posToId(dst)))
			throw "Can't move there"s;
		placePiece(piece, dst);
		break;
	case ACT_SWAP:
		if (board->isEnemyPiece(occupant) && piece->getType() != PieceType::warhorse && favor != Favor::deceive)
			throw "Piece can't switch with an enemy"s;
		if (favor != Favor::assault && favor != Favor::deceive && piece->getType() == PieceType::warhorse && board->isEnemyPiece(occupant) && (board->config.opts & Config::terrainRules)) {
			if (occupant->getType() == PieceType::spearmen)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch with an enemy " + pieceNames[uint8(occupant->getType())];
			if (dtil->getType() == TileType::water)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch onto " + tileNames[uint8(dtil->getType())];
			if (dtil->isUnbreachedFortress())
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch onto a not breached " + tileNames[uint8(dtil->getType())];
		}
		if (!board->collectMoveTiles(piece, eneRec, favor, true).count(board->posToId(dst)))
			throw "Can't move there"s;
		placePiece(occupant, pos);
		placePiece(piece, dst);
		break;
	case ACT_ATCK:
		checkKiller(piece, occupant, dtil, true);
		if (piece->getType() != PieceType::throne && (board->config.opts & Config::terrainRules)) {
			if (stil->getType() == TileType::mountain && piece->getType() != PieceType::rangers && piece->getType() != PieceType::dragon)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't attack from a " + tileNames[uint8(stil->getType())];
			if (dtil->getType() == TileType::forest && stil->getType() != TileType::forest && piece->getType() >= PieceType::lancer && piece->getType() <= PieceType::elephant)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " must be on a " + tileNames[uint8(dtil->getType())] + " to attack onto one";
			if (dtil->getType() == TileType::forest && piece->getType() == PieceType::dragon)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't attack onto a " + tileNames[uint8(dtil->getType())];
			if (dtil->getType() == TileType::water && piece->getType() != PieceType::spearmen && piece->getType() != PieceType::dragon)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't attack onto " + tileNames[uint8(dtil->getType())];
		}
		if (!board->collectEngageTiles(piece).count(board->posToId(dst)))
			throw "Can't move there"s;
		doEngage(piece, pos, dst, occupant, dtil, action);
	}
	if (board->getPxpad()->getShow())
		changeTile(stil, TileType::plains);
	if (concludeAction(piece, action, favor) && availableFF)
		World::pgui()->openPopupFavorPick(availableFF);
	if (World::audio())
		World::audio()->play("move");
}

void Game::pieceFire(Piece* killer, svec2 dst, Piece* victim) {
	svec2 pos = board->ptog(killer->getPos());
	Tile* stil = board->getTile(pos);
	Tile* dtil = board->getTile(dst);
	Favor favor = World::state<ProgMatch>()->favorIconSelect();
	if (pos == dst)
		throw string();

	checkActionRecord(killer, victim, ACT_FIRE, favor);
	checkKiller(killer, victim, dtil, false);
	if (board->config.opts & Config::terrainRules) {
		if (stil->getType() == TileType::forest || stil->getType() == TileType::water)
			throw "Can't fire from "s + (stil->getType() == TileType::forest ? "a " : "") + tileNames[uint8(stil->getType())];
		if (dtil->getType() == TileType::forest && killer->getType() != PieceType::trebuchet)
			throw firstUpper(pieceNames[uint8(killer->getType())]) + " can't fire at a " + tileNames[uint8(dtil->getType())];
		if (dtil->getType() == TileType::mountain)
			throw "Can't fire at a "s + tileNames[uint8(dtil->getType())];
	}
	if (!board->collectEngageTiles(killer).count(board->posToId(dst)))
		throw "Can't fire there"s;
	if (board->config.opts & Config::terrainRules)
		for (svec2 m = deltaSingle(ivec2(dst) - ivec2(pos)), i = pos + m; i != dst; i += m)
			if (TileType type = board->getTile(i)->getType(); type == TileType::mountain)
				throw "Can't fire over "s + tileNames[uint8(type)] + 's';

	doEngage(killer, pos, dst, victim, dtil, ACT_FIRE);
	if (concludeAction(killer, ACT_FIRE, favor) && availableFF)
		World::pgui()->openPopupFavorPick(availableFF);
	if (World::audio())
		World::audio()->play("ammo");
}

bool Game::concludeAction(Piece* piece, Action action, Favor favor) {
	if (favor == Favor::none)
		ownRec.update(piece, action);
	else if (lastFavorUsed = true; favor == Favor::assault) {
		if (ownRec.update(piece, action, false); (ownRec.assault[piece] & ACT_MS) == ACT_MS)
			finishFavor(Favor::none, favor);
	} else if (finishFavor(Favor::none, favor); favor == Favor::hasten)
		ownRec.update(piece, ACT_NONE);
	if (checkWin())
		return false;

	Action termin = ownRec.actionsExhausted();
	bool done = termin != ACT_NONE && std::none_of(favorsCount.begin(), favorsCount.end(), [](uint16 cnt) -> bool { return cnt; }) && !World::state<ProgMatch>()->getUnplacedDragons();
	if (((done || (termin & (ACT_AF | ACT_SPAWN))) && !(board->config.opts & Config::homefront)) || eneRec.info == Record::battleFail) {
		if (checkPointsWin())
			return false;
		endTurn();
	} else {
		ProgMatch* pm = World::state<ProgMatch>();
		pm->updateIcons();
		pm->message->setText("Your turn");	// in case it got changed during the turn
		board->setFavorInteracts(pm->favorIconSelect(), ownRec);
		board->setPxpadPos(nullptr);
	}
	return true;
}

void Game::placeDragon(Piece* dragon, svec2 pos, Piece* occupant) {
	ProgMatch* pm = World::state<ProgMatch>();
	if (Tile* til = board->getTile(pos); board->isHomeTile(til) && til->getType() == TileType::fortress) {
		dragon->setInteractivity(dragon->getShow(), false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		pm->decreaseDragonIcon();
		if (occupant)
			removePiece(occupant);
		placePiece(dragon, pos);
		concludeAction(dragon, ACT_SPAWN, Favor::none);
	} else
		pm->updateIcons();	// reset selected
}

void Game::establishTile(Piece* throne, bool reinit) {
	ProgMatch* pm = World::state<ProgMatch>();
	auto [tile, top] = board->checkTileEstablishable(throne);
	if (top == TileTop::ownCity)
		pm->destroyEstablishIcon();
	if (reinit)
		board->restorePiecesInteract(ownRec);

	changeTile(tile, tile->getType(), top);
	miscActionTaken = true;
	pm->updateIcons();
}

void Game::rebuildTile(Piece* throne, bool reinit) {
	if (reinit)
		board->restorePiecesInteract(ownRec);
	breachTile(board->getTile(board->ptog(throne->getPos())), false);
	miscActionTaken = true;
	World::state<ProgMatch>()->updateIcons();
}

void Game::spawnPiece(PieceType type, Tile* tile, bool reinit) {
	if (Piece* occ = board->findOccupant(tile))
		removePiece(occ);
	Piece* pce = board->findSpawnablePiece(type);
	if (reinit)
		board->resetTilesAfterSpawn();
	placePiece(pce, board->idToPos(board->tileId(tile)));
	concludeAction(pce, ACT_SPAWN, Favor::none);	// should reset pieces and side icons
}

void Game::checkActionRecord(Piece* piece, Piece* occupant, Action action, Favor favor) {
	if (eneRec.info == Record::battleFail && action != ACT_MOVE)
		throw "Only moving is allowed"s;

	switch (favor) {
	case Favor::hasten:
		if (action != ACT_MOVE)
			throw firstUpper(favorNames[uint8(favor)]) + " is limited to moving";
		break;
	case Favor::assault:
		if (!(action & ACT_MS))
			throw firstUpper(favorNames[uint8(favor)]) + " is limited to moving and switching";
		if (ownRec.actors.count(occupant))
			throw "Can't switch with a non-"s + favorNames[uint8(Favor::assault)] + " piece";
		if (umap<Piece*, Action>::iterator it = ownRec.assault.find(piece); it != ownRec.assault.end())
			if (Action act = it->second & ~(action == ACT_MOVE ? ACT_SWAP : ACT_MOVE))
				throw actionRecordMsg(act, true);
		break;
	case Favor::deceive:
		if (action != ACT_SWAP || board->isOwnPiece(occupant))
			throw firstUpper(favorNames[uint8(favor)]) + " is limited to switching enemy pieces";
		break;
	case Favor::none:
		if (action == ACT_MOVE && eneRec.info != Record::battleFail) {
			umap<Piece*, Action>::iterator it = ownRec.actors.find(piece);
			if (ownRec.actors.size() >= (it != ownRec.actors.end() ? 3 : 2))
				throw toStr(ownRec.actors.size()) + " non-" + favorNames[uint8(Favor::assault)] + " pieces have already acted";
			if (it != ownRec.actors.end())
				if (Action act = it->second & ~ACT_SWAP)
					throw actionRecordMsg(act, true);
			if (umap<Piece*, Action>::iterator oth = std::find_if(ownRec.actors.begin(), ownRec.actors.end(), [piece](const pair<Piece*, Action>& pa) -> bool { return pa.first != piece; }); oth != ownRec.actors.end()) {
				if (Action act = oth->second & ~ACT_MS)
					throw actionRecordMsg(act, false);
				if ((oth->second & ACT_MS) == ACT_MS)
					throw "A piece has already moved and switched"s;
				if (it != ownRec.actors.end() && it->second)
					throw "Piece can't move anymore"s;
			}
		} else if (action == ACT_SWAP) {
			umap<Piece*, Action>::iterator it = ownRec.actors.find(piece);
			if (ownRec.actors.size() >= (it != ownRec.actors.end() ? 3 : 2))
				throw toStr(ownRec.actors.size()) + " non-" + favorNames[uint8(Favor::assault)] + " pieces have already acted";
			if (it != ownRec.actors.end())
				if (Action act = it->second & ~(it->first->getType() != PieceType::warhorse ? ACT_MOVE : ACT_MS))
					throw actionRecordMsg(act, true);
			if (umap<Piece*, Action>::iterator oth = std::find_if(ownRec.actors.begin(), ownRec.actors.end(), [piece](const pair<Piece*, Action>& pa) -> bool { return pa.first != piece; }); oth != ownRec.actors.end()) {
				if (Action act = oth->second & ~ACT_MOVE)
					throw actionRecordMsg(act, false);
				if (it != ownRec.actors.end() && (it->first->getType() != PieceType::warhorse ? it->second : it->second & ~ACT_SWAP))
					throw "Piece can't switch anymore"s;
			}
		} else if (!ownRec.actors.empty())
			throw "A piece has already acted"s;
	}
	if (favor != Favor::assault && action == ACT_SWAP && ownRec.assault.count(occupant))
		throw "Can't switch with an "s + favorNames[uint8(Favor::assault)] + "piece";
}

string Game::actionRecordMsg(Action action, bool self) {
	string pref = self ? "Piece has already " : "A piece has already ";
	if (action & ACT_MOVE)
		return pref + "moved";
	if (action & ACT_SWAP)
		return pref + "switched";
	if (action & ACT_ATCK)
		return pref + "attacked";
	if (action & ACT_FIRE)
		return pref + "fired";
	if (action & ACT_SPAWN)
		return pref + "spawned";
	return pref + "acted";
}

void Game::checkKiller(Piece* killer, Piece* victim, Tile* dtil, bool attack) {
	string action = attack ? "attack" : "fire";
	if (firstTurn && !(board->config.opts & Config::firstTurnEngage))
		throw "Can't " + action + " during the first turn";
	if (anyFavorUsed)
		throw "Can't " + action + " after a fate's favor";
	if (eneRec.protects.count(killer))
		throw "Piece can't " + action + " during this turn";
	if (!ownRec.actors.empty())
		throw "Piece can't " + action;

	if (victim) {
		umap<Piece*, bool>::iterator protect = eneRec.protects.find(victim);
		if (protect != eneRec.protects.end() && (protect->second || killer->getType() != PieceType::throne))
			throw "Piece is protected during this turn"s;
		if (victim->getType() == PieceType::elephant && dtil->getType() == TileType::plains && killer->getType() != PieceType::dragon && killer->getType() != PieceType::throne && (board->config.opts & Config::terrainRules))
			throw firstUpper(pieceNames[uint8(killer->getType())]) + " can't attack an " + pieceNames[uint8(victim->getType())] + " on " + tileNames[uint8(dtil->getType())];
	} else if (!(board->config.opts & Config::homefront) || (dtil->getType() != TileType::fortress && !board->findTileTop(dtil).isFarm()) || dtil->getBreached())
		throw "Can't " + (attack ? action : action + " at") + " nothing";
}

void Game::doEngage(Piece* killer, svec2 pos, svec2 dst, Piece* victim, Tile* dtil, Action action) {
	if (killer->getType() == PieceType::warhorse)
		ownRec.addProtect(killer, false);

	if (dtil->isUnbreachedFortress() && killer->getType() != PieceType::throne) {
		if (killer->getType() == PieceType::dragon)
			if (dst -= deltaSingle(ivec2(dst) - ivec2(pos)); dst != pos && board->findPiece(dst))
				throw "No space beside "s + tileNames[uint8(TileType::fortress)];

		if ((!victim || board->isEnemyPiece(victim)) && randDist(randGen) >= board->config.battlePass) {
			if (eneRec.addProtect(killer, false); action == ACT_ATCK) {
				ownRec.info = Record::battleFail;
				ownRec.lastAct = pair(killer, ACT_ATCK);
				endTurn();
			}
			throw "Battle lost"s;
		}
		if (breachTile(dtil); killer->getType() == PieceType::dragon)
			placePiece(killer, dst);
	} else {
		if (board->findTileTop(dtil).isFarm() && !dtil->getBreached())
			breachTile(dtil);
		if (victim)
			removePiece(victim);
		if (action == ACT_ATCK)
			placePiece(killer, dst);
	}
}

bool Game::checkPointsWin() {
	Record::Info win = board->countVictoryPoints(vpOwn, vpEne, eneRec);
	World::state<ProgMatch>()->updateVictoryPoints(vpOwn, vpEne);
	if (win != Record::none) {
		doWin(win);
		return true;
	}
	return false;
}

bool Game::checkWin() {
	if (board->checkThroneWin(board->getPieces().own(), board->ownPieceAmts) || board->checkFortressWin(board->getTiles().own(), board->getPieces().ene(), board->enePieceAmts)) {
		doWin(Record::loose);
		return true;
	}
	if (board->checkThroneWin(board->getPieces().ene(), board->enePieceAmts) || board->checkFortressWin(board->getTiles().ene(), board->getPieces().own(), board->ownPieceAmts)) {
		doWin(Record::win);
		return true;
	}
	return false;
}

void Game::doWin(Record::Info win) {
	ownRec.info = win;
	endTurn();
	capRec(win);
	World::program()->finishMatch(win);
}

void Game::surrender() {
	sendb.pushHead(Com::Code::record, Com::dataHeadSize + sizeof(uint8) + sizeof(uint16) * 2);
	sendb.push(uint8(Record::loose));
	sendb.push({ uint16(UINT16_MAX), 0_us });
	World::netcp()->sendData(sendb);
	capRec(Record::loose);
	World::program()->finishMatch(Record::loose);
}

void Game::prepareTurn(bool fcont) {
	bool xmov = eneRec.info == Record::battleFail;	// should only occur when myTurn is true
	Board::setTilesInteract(board->getTiles().begin(), board->getTiles().getSize(), Tile::Interact(myTurn));
	board->prepareTurn(myTurn, xmov, fcont, ownRec, eneRec);
	if (myTurn && !fcont)
		anyFavorUsed = lastFavorUsed = miscActionTaken = false;

	ProgMatch* pm = World::state<ProgMatch>();
	pm->updateIcons(fcont);
	pm->message->setText(myTurn ? xmov ? "Opponent lost a battle" : "Your turn" : "Opponent's turn");
}

void Game::endTurn() {
	sendb.pushHead(Com::Code::record, Com::dataHeadSize + sizeof(uint8) + sizeof(uint16) * (2 + ownRec.protects.size()));	// 2 for last actor and protects size
	sendb.push(uint8(ownRec.info | (eneRec.info & Record::battleFail)));
	sendb.push({ board->inversePieceId(ownRec.lastAct.first), uint16(ownRec.protects.size()) });
	for (auto& [pce, prt] : ownRec.protects)
		sendb.push(uint16((board->inversePieceId(pce) & 0x7FFF) | (uint16(prt) << 15)));
	World::netcp()->sendData(sendb);

	firstTurn = myTurn = false;
	board->setPxpadPos(nullptr);
	prepareTurn(false);
}

bool Game::recvRecord(const uint8* data) {
	Record::Info info = Record::Info(*data);
	bool fcont = ownRec.info == Record::battleFail && info == Record::battleFail;
	if (fcont)
		ownRec.info = eneRec.info = Record::none;	// response to failed attack, meaning keep old records when continuing a turn but battleFail no longer needed
	else {
		uint16 ai = Com::read16(++data);
		uint16 ptCnt = Com::read16(data += sizeof(uint16));
		umap<Piece*, bool> protect(ptCnt);
		while (ptCnt--) {
			uint16 id = Com::read16(data += sizeof(uint16));
			protect.emplace(&board->getPieces()[id & 0x7FFF], id & 0x8000);
		}
		eneRec = Record(pair(ai < board->getPieces().getSize() ? &board->getPieces()[ai] : nullptr, ACT_NONE), std::move(protect), info);
		ownRec = Record();
	}

	board->countVictoryPoints(vpOwn, vpEne, eneRec);
	World::state<ProgMatch>()->updateVictoryPoints(vpOwn, vpEne);
	if (eneRec.info == Record::win || eneRec.info == Record::loose || eneRec.info == Record::tie)
		World::program()->finishMatch(eneRec.info == Record::win ? Record::loose : eneRec.info == Record::loose ? Record::win : Record::tie);
	else {
		myTurn = true;
		prepareTurn(fcont);
		if (board->getPxpad()->getShow())
			World::pgui()->openPopupChoice("Destroy forest at " + toStr(board->ptog(board->getPxpad()->getPos()), "|") + '?', &Program::eventKillDestroy, &Program::eventCancelDestroy);
	}
	return World::netcp();
}

void Game::sendConfig(bool onJoin) {
	uint ofs;
	ProgRoom* pr = World::state<ProgRoom>();
	Config& cfg = pr->confs[pr->configName->getText()];
	if (onJoin) {
		ofs = sendb.allocate(Com::Code::cnjoin, Com::dataHeadSize + 1 + cfg.dataSize(pr->configName->getText().length()));
		ofs = sendb.write(uint8(true), ofs);
	} else
		ofs = sendb.allocate(Com::Code::config, Com::dataHeadSize + cfg.dataSize(pr->configName->getText().length()));
	cfg.toComData(&sendb[ofs], pr->configName->getText());
	World::netcp()->sendData(sendb);
}

void Game::sendStart() {
	myTurn = std::uniform_int_distribution<uint>(0, 1)(randGen);
	ProgRoom* pr = World::state<ProgRoom>();
	Config& cfg = pr->confs[pr->configName->getText()];
	uint ofs = sendb.allocate(Com::Code::start, Com::dataHeadSize + cfg.dataSize(pr->configName->getText().length()));
	ofs = sendb.write(uint8(!myTurn), ofs);
	cfg.toComData(&sendb[ofs], pr->configName->getText());
	World::netcp()->sendData(sendb);
	World::program()->eventOpenSetup(pr->configName->getText());
}

void Game::recvStart(const uint8* data) {
	myTurn = data[0];
	World::program()->eventOpenSetup(board->config.fromComData(data + 1));
}

void Game::sendSetup() {
	uint tcnt = board->tileCompressionSize();
	uint ofs = sendb.allocate(Com::Code::setup, Com::dataHeadSize + tcnt + pieceLim * sizeof(uint16) + board->getPieces().getNum() * sizeof(uint16));
	std::fill_n(&sendb[ofs], tcnt, 0);
	for (uint16 i = 0; i < board->getTiles().getExtra(); ++i)
		sendb[i / 2 + ofs] |= board->compressTile(i);
	ofs += tcnt;

	for (uint8 i = 0; i < pieceLim; ofs = sendb.write(board->ownPieceAmts[i++], ofs));
	for (uint16 i = 0; i < board->getPieces().getNum(); ++i)
		sendb.write(board->pieceOnBoard(board->getPieces().own(i)) ? board->invertId(board->posToId(board->ptog(board->getPieces().own(i)->getPos()))) : uint16(UINT16_MAX), i * sizeof(uint16) + ofs);
	World::netcp()->sendData(sendb);
}

void Game::recvSetup(const uint8* data) {
	// set tiles and pieces
	for (uint16 i = 0; i < board->getTiles().getHome(); ++i)
		board->getTiles()[i].setType(board->decompressTile(data, i));
	for (uint16 i = 0; i < board->config.homeSize.x; ++i)
		World::state<ProgSetup>()->rcvMidBuffer[i] = board->decompressTile(data, board->getTiles().getHome() + i);

	uint16 ofs = board->tileCompressionSize();
	for (uint8 i = 0; i < pieceLim; ++i, ofs += sizeof(uint16))
		board->enePieceAmts[i] = Com::read16(data + ofs);

	Mesh* meshes[pieceNames.size()];
	for (uint8 i = 0; i < pieceNames.size(); ++i) {
		meshes[i] = World::scene()->mesh(pieceNames[i]);
		meshes[i]->allocate(board->ownPieceAmts[i] + board->enePieceAmts[i], true);
	}
	board->initEnePieces(meshes);
	for (Mesh* it : meshes)
		it->updateInstanceData();
	for (uint16 i = 0; i < board->getPieces().getNum(); ++i) {
		uint16 id = Com::read16(data + i * sizeof(uint16) + ofs);
		board->getPieces().ene(i)->setPos(board->gtop(id < board->getTiles().getHome() ? board->idToPos(id) : svec2(UINT16_MAX)));
	}

	// document thrones' fortresses
	for (Piece* throne = board->getPieces(board->getPieces().ene(), board->enePieceAmts, PieceType::throne); throne != board->getPieces().end(); ++throne)
		if (uint16 pos = board->posToId(board->ptog(throne->getPos())); board->getTiles()[pos].getType() == TileType::fortress)
			throne->lastFortress = pos;

	// finish up
	if (ProgSetup* ps = World::state<ProgSetup>(); ps->getStage() == ProgSetup::Stage::ready)
		World::program()->eventOpenMatch();
	else {
		ps->enemyReady = true;
		ps->message->setText(ps->getStage() >= ProgSetup::Stage::pieces ? "Opponent ready" : "Hurry the fuck up!");
	}
}

void Game::recvMove(const uint8* data) {
	Piece& pce = board->getPieces()[Com::read16(data)];
	uint16 id = Com::read16(data + sizeof(uint16));
	svec2 pos = board->idToPos(id);
	capRec(&pce, pos);
	pce.updatePos(pos);
	if (pce.getType() == PieceType::throne && board->getTiles()[id].getType() == TileType::fortress)
		pce.lastFortress = id;
}

void Game::placePiece(Piece* piece, svec2 pos) {
	if (uint16 fid = board->posToId(pos); piece->getType() == PieceType::throne && board->getTiles()[fid].getType() == TileType::fortress && piece->lastFortress != fid)
		if (piece->lastFortress = fid; availableFF < std::accumulate(favorsLeft.begin(), favorsLeft.end(), 0_us))
			++availableFF;

	capRec(piece, pos);
	piece->updatePos(pos, true);
	sendb.pushHead(Com::Code::move);
	sendb.push({ board->inversePieceId(piece), board->invertId(board->posToId(pos)) });
	World::netcp()->sendData(sendb);
}

void Game::recvKill(const uint8* data) {
	Piece* pce = &board->getPieces()[Com::read16(data)];
	if (board->isOwnPiece(pce))
		board->setPxpadPos(pce);
	capRec(pce);
	pce->updatePos();
}

void Game::recvBreach(const uint8* data) {
	uint16 id = Com::read16(data);
	bool yes = data[sizeof(uint16)];
	capRec(&board->getTiles()[id], yes);
	board->getTiles()[id].setBreached(yes);
}

void Game::removePiece(Piece* piece) {
	capRec(piece);
	piece->updatePos();
	sendb.pushHead(Com::Code::kill);
	sendb.push(board->inversePieceId(piece));
	World::netcp()->sendData(sendb);
}

void Game::breachTile(Tile* tile, bool yes) {
	capRec(tile, yes);
	tile->setBreached(yes);
	sendb.pushHead(Com::Code::breach);
	sendb.push(board->inverseTileId(tile));
	sendb.push(uint8(yes));
	World::netcp()->sendData(sendb);
}

void Game::recvTile(const uint8* data) {
	uint16 id = Com::read16(data);
	TileType type = TileType(data[sizeof(uint16)] & 0xF);
	capRec(&board->getTiles()[id], type);
	if (board->getTiles()[id].setType(type); board->getTiles()[id].getType() == TileType::fortress)
		if (Piece* pce = board->findPiece(board->idToPos(id)); pce && pce->getType() == PieceType::throne)
			pce->lastFortress = id;
	if (TileTop top = TileTop(data[sizeof(uint16)] >> 4); top != TileTop::none) {
		capRec(top, &board->getTiles()[id]);
		board->setTileTop(top, &board->getTiles()[id]);
	}
}

void Game::changeTile(Tile* tile, TileType type, TileTop top) {
	capRec(tile, type);
	tile->setType(type);
	if (top != TileTop::none) {
		capRec(top, tile);
		board->setTileTop(top, tile);
	}
	sendb.pushHead(Com::Code::tile);
	sendb.push(board->inverseTileId(tile));
	sendb.push(uint8(uint8(type) | (top.invert() << 4)));
	World::netcp()->sendData(sendb);
}

void Game::capRec(Piece* piece, svec2 pos) {
	if (recWriter)
		recWriter->piece(board->pieceId(piece), board->ptog(piece->getPos()), pos);
}

void Game::capRec(Tile* tile, TileType type) {
	if (recWriter)
		recWriter->tile(board->tileId(tile), tile->getType(), type);
}

void Game::capRec(Tile* tile, bool breach) {
	if (recWriter)
		recWriter->breach(board->tileId(tile), tile->getBreached(), breach);
}

void Game::capRec(TileTop top, Tile* tile) {
	if (recWriter)
		recWriter->top(top, board->ptog(board->getTileTop(top)->getPos()), board->ptog(tile->getPos()));
}

void Game::capRec(Record::Info info) {
	if (recWriter)
		recWriter->finish(info);
	recWriter.reset();
}

#ifndef NDEBUG
void Game::processCommand(const char* cmd) {
	if (string_view key = readWord(cmd); !SDL_strcasecmp(key.data(), "m")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPieceId(cmd), true);
	} else if (!SDL_strcasecmp(key.data(), "move")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPiecePos(cmd), true);
	} else if (!SDL_strcasecmp(key.data(), "s")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPieceId(cmd), false);
	} else if (!SDL_strcasecmp(key.data(), "swap")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPiecePos(cmd), false);
	} else if (!SDL_strcasecmp(key.data(), "switch")) {
		while (*cmd)
			if (Piece* a = readCommandPieceId(cmd), *b = readCommandPieceId(cmd); a && b) {
				svec2 pos = board->ptog(b->getPos());
				placePiece(b, board->ptog(a->getPos()));
				placePiece(a, pos);
			}
	} else if (!SDL_strcasecmp(key.data(), "k")) {
		while (*cmd)
			if (Piece* pce = readCommandPieceId(cmd))
				removePiece(pce);
	} else if (!SDL_strcasecmp(key.data(), "kill")) {
		while (*cmd)
			if (Piece* pce = readCommandPiecePos(cmd))
				removePiece(pce);
	} else if (!SDL_strcasecmp(key.data(), "killall")) {
		while (*cmd) {
			bool own = readCommandPref(cmd);
			if (PieceType type = strToEnum<PieceType>(pieceNames, readWord(cmd)); type <= PieceType::throne) {
				Piece* pces = own ? board->getOwnPieces(type) : board->getEnePieces(type);
				for (uint16 i = 0; i < board->config.pieceAmounts[uint8(type)]; ++i)
					removePiece(pces + i);
			}
		}
	} else if (!SDL_strcasecmp(key.data(), "c")) {
		while (*cmd)
			if (auto [id, name] = readCommandTileId(cmd); id < board->getTiles().getSize())
				readCommandChange(id, name.data());
	} else if (!SDL_strcasecmp(key.data(), "change")) {
		while (*cmd)
			if (auto [pos, name] = readCommandTilePos(cmd); inRange(pos, svec2(0), board->boardLimit()))
				readCommandChange(board->posToId(pos), name.data());
	} else if (!SDL_strcasecmp(key.data(), "changeall")) {
		while (*cmd) {
			svec2 pos = readCommandVec(cmd), end = readCommandVec(cmd);
			for (svec2::length_type i = 0; i < svec2::length(); ++i)
				if (pos[i] > end[i])
					std::swap(pos[i], end[i]);
			if (TileType type = strToEnum<TileType>(tileNames, readWord(cmd)); inRange(pos, svec2(0), board->boardLimit()) && type < TileType::empty)
				for (end = glm::min(end, board->boardLimit() - svec2(1)); pos.y <= end.y; ++pos.y)
					for (uint16 x = pos.x; x <= end.x; ++x) {
						Tile* tile = board->getTile(svec2(x, pos.y));
						changeTile(tile, type, board->findTileTop(tile));
					}
		}
	} else if (!SDL_strcasecmp(key.data(), "b")) {
		while (*cmd)
			if (auto [id, yes] = readCommandTileId(cmd); id < board->getTiles().getSize())
				breachTile(&board->getTiles()[id], toBool(yes));
	} else if (!SDL_strcasecmp(key.data(), "breach"))
		while (*cmd)
			if (auto [pos, yes] = readCommandTilePos(cmd); inRange(pos, svec2(0), board->boardLimit()))
				breachTile(board->getTile(pos), toBool(yes));
}

Piece* Game::readCommandPieceId(const char*& cmd) {
	auto [own, id] = readCommandMnum(cmd, { '-' });
	if (own)
		return id < board->getPieces().getSize() ? board->getPieces().own() + id : nullptr;
	return id < board->getPieces().getNum() ? board->getPieces().ene() + id : nullptr;
}

inline Piece* Game::readCommandPiecePos(const char*& cmd) {
	return board->findPiece(readCommandVec(cmd) % board->boardLimit());
}

void Game::readCommandPieceMove(const char*& cmd, Piece* pce, bool killOccupant) {
	auto [xm, xv] = readCommandMnum(cmd);
	auto [ym, yv] = readCommandMnum(cmd);
	if (pce) {
		svec2 pos = board->ptog(pce->getPos());
		pos.x = !xm ? pos.x + xv : xm == 1 ? pos.x - xv : xv;
		pos.y = !ym ? pos.y + yv : ym == 1 ? pos.y - yv : yv;
		if (Piece* occ = board->findPiece(pos))
			killOccupant ? removePiece(occ) : placePiece(occ, board->ptog(pce->getPos()));
		placePiece(pce, pos);
	}
}

pair<uint16, string_view> Game::readCommandTileId(const char*& cmd) {
	uint16 id = readNumber<uint16>(cmd);
	return pair(id, readWord(cmd));
}

pair<svec2, string_view> Game::readCommandTilePos(const char*& cmd) {
	svec2 pos = readCommandVec(cmd);
	return pair(pos, readWord(cmd));
}

void Game::readCommandChange(uint16 id, const char* name) {
	bool own = readCommandPref(name);
	if (TileType type = strToEnum<TileType>(tileNames, name); type < TileType::empty)
		changeTile(&board->getTiles()[id], type, board->findTileTop(&board->getTiles()[id]));
	else if (TileTop top = strToEnum<TileTop::Type>(TileTop::names, name); top <= TileTop::ownCity)
		changeTile(&board->getTiles()[id], board->getTiles()[id].getType(), own ? top.type : top.invert());
}

svec2 Game::readCommandVec(const char*& cmd) {
	uint16 x = readNumber<uint16>(cmd);
	return svec2(x, readNumber<uint16>(cmd));
}

pair<uint8, uint16> Game::readCommandMnum(const char*& cmd, initlist<char> pref) {
	uint8 p = readCommandPref(cmd, pref);
	return pair(p, readNumber<uint16>(cmd));
}

uint8 Game::readCommandPref(const char*& cmd, initlist<char> chars) {
	for (; isSpace(*cmd); ++cmd);
	initlist<char>::iterator pos = std::find(chars.begin(), chars.end(), *cmd);
	if (pos != chars.end())
		++cmd;
	return pos - chars.begin();
}
#endif
