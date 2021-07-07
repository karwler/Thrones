#include "engine/audioSys.h"
#include "board.h"
#include "program.h"
#include "netcp.h"
#include "progs.h"
#include "utils/widgets.h"
#include <ctime>
#include <iostream>

// GAME

Game::Game(AudioSys* audioSys, Program* program, const Scene* scene) :
	board(new Board(scene)),
	audio(audioSys),
	prog(program),
	randGen(createRandomEngine()),
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

void Game::finishFavor(Favor next, Favor previous) {
	if (previous != Favor::none) {
		if (lastFavorUsed) {
			--favorsCount[uint8(previous)];
			anyFavorUsed = true;
			lastFavorUsed = false;
		}
		ProgMatch* pm = prog->getState<ProgMatch>();
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
	Favor favor = prog->getState<ProgMatch>()->favorIconSelect();
	Action action = move ? occupant ? ACT_SWAP : ACT_MOVE : ACT_ATCK;
	if (pos == dst)
		throw string();

	checkActionRecord(piece, occupant, action, favor);
	switch (action) {
	case ACT_MOVE:
		if (!board->collectMoveTiles(piece, eneRec, favor).count(board->posToId(dst)))
			throw string("Can't move there");
		placePiece(piece, dst);
		break;
	case ACT_SWAP:
		if (board->isEnemyPiece(occupant) && piece->getType() != PieceType::warhorse && favor != Favor::deceive)
			throw string("Piece can't switch with an enemy");
		if (favor != Favor::assault && favor != Favor::deceive && piece->getType() == PieceType::warhorse && board->isEnemyPiece(occupant) && (board->config.opts & Config::terrainRules)) {
			if (occupant->getType() == PieceType::spearmen)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch with an enemy " + pieceNames[uint8(occupant->getType())];
			if (dtil->getType() == TileType::water)
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch onto " + tileNames[uint8(dtil->getType())];
			if (dtil->isUnbreachedFortress())
				throw firstUpper(pieceNames[uint8(piece->getType())]) + " can't switch onto a not breached " + tileNames[uint8(dtil->getType())];
		}
		if (!board->collectMoveTiles(piece, eneRec, favor, true).count(board->posToId(dst)))
			throw string("Can't move there");
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
			throw string("Can't move there");
		doEngage(piece, pos, dst, occupant, dtil, action);
	}
	if (board->getPxpad()->show)
		changeTile(stil, TileType::plains);
	if (concludeAction(piece, action, favor) && availableFF)
		prog->getGui()->openPopupFavorPick(availableFF);
	if (audio)
		audio->play("move");
}

void Game::pieceFire(Piece* killer, svec2 dst, Piece* victim) {
	svec2 pos = board->ptog(killer->getPos());
	Tile* stil = board->getTile(pos);
	Tile* dtil = board->getTile(dst);
	Favor favor = prog->getState<ProgMatch>()->favorIconSelect();
	if (pos == dst)
		throw string();

	checkActionRecord(killer, victim, ACT_FIRE, favor);
	checkKiller(killer, victim, dtil, false);
	if (board->config.opts & Config::terrainRules) {
		if (stil->getType() == TileType::forest || stil->getType() == TileType::water)
			throw "Can't fire from " + string(stil->getType() == TileType::forest ? "a " : "") + tileNames[uint8(stil->getType())];
		if (dtil->getType() == TileType::forest && killer->getType() != PieceType::trebuchet)
			throw firstUpper(pieceNames[uint8(killer->getType())]) + " can't fire at a " + tileNames[uint8(dtil->getType())];
		if (dtil->getType() == TileType::mountain)
			throw string("Can't fire at a ") + tileNames[uint8(dtil->getType())];
	}
	if (!board->collectEngageTiles(killer).count(board->posToId(dst)))
		throw string("Can't fire there");
	if (board->config.opts & Config::terrainRules)
		for (svec2 m = deltaSingle(ivec2(dst) - ivec2(pos)), i = pos + m; i != dst; i += m)
			if (TileType type = board->getTile(i)->getType(); type == TileType::mountain)
				throw string("Can't fire over ") + tileNames[uint8(type)] + 's';

	doEngage(killer, pos, dst, victim, dtil, ACT_FIRE);
	if (concludeAction(killer, ACT_FIRE, favor) && availableFF)
		prog->getGui()->openPopupFavorPick(availableFF);
	if (audio)
		audio->play("ammo");
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
	bool done = termin != ACT_NONE && std::none_of(favorsCount.begin(), favorsCount.end(), [](uint16 cnt) -> bool { return cnt; }) && !prog->getState<ProgMatch>()->getUnplacedDragons();
	if (((done || (termin & (ACT_AF | ACT_SPAWN))) && !(board->config.opts & Config::homefront)) || eneRec.info == Record::battleFail) {
		if (checkPointsWin())
			return false;
		endTurn();
	} else {
		ProgMatch* pm = prog->getState<ProgMatch>();
		pm->updateIcons();
		pm->message->setText("Your turn");	// in case it got changed during the turn
		board->setFavorInteracts(pm->favorIconSelect(), ownRec);
		board->setPxpadPos(nullptr);
	}
	return true;
}

void Game::placeDragon(Piece* dragon, svec2 pos, Piece* occupant) {
	ProgMatch* pm = prog->getState<ProgMatch>();
	if (Tile* til = board->getTile(pos); board->isHomeTile(til) && til->getType() == TileType::fortress) {
		dragon->setInteractivity(dragon->show, false, &Program::eventPieceStart, &Program::eventMove, &Program::eventEngage);
		pm->decreaseDragonIcon();
		if (occupant)
			removePiece(occupant);
		placePiece(dragon, pos);
		concludeAction(dragon, ACT_SPAWN, Favor::none);
	} else
		pm->updateIcons();	// reset selected
}

void Game::establishTile(Piece* throne, bool reinit) {
	ProgMatch* pm = prog->getState<ProgMatch>();
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
	prog->getState<ProgMatch>()->updateIcons();
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
		throw string("Only moving is allowed");

	switch (favor) {
	case Favor::hasten:
		if (action != ACT_MOVE)
			throw firstUpper(favorNames[uint8(favor)]) + " is limited to moving";
		break;
	case Favor::assault:
		if (!(action & ACT_MS))
			throw firstUpper(favorNames[uint8(favor)]) + " is limited to moving and switching";
		if (ownRec.actors.count(occupant))
			throw "Can't switch with a non-" + string(favorNames[uint8(Favor::assault)]) + " piece";
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
					throw string("A piece has already moved and switched");
				if (it != ownRec.actors.end() && it->second)
					throw string("Piece can't move anymore");
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
					throw string("Piece can't switch anymore");
			}
		} else if (!ownRec.actors.empty())
			throw string("A piece has already acted");
	}
	if (favor != Favor::assault && action == ACT_SWAP && ownRec.assault.count(occupant))
		throw string("Can't switch with an ") + favorNames[uint8(Favor::assault)] + "piece";
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
			throw string("Piece is protected during this turn");
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
			if (dst -= deltaSingle(ivec2(dst) - ivec2(pos)); dst != pos && board->findOccupant(dst))
				throw string("No space beside ") + tileNames[uint8(TileType::fortress)];

		if ((!victim || board->isEnemyPiece(victim)) && randDist(randGen) >= board->config.battlePass) {
			if (eneRec.addProtect(killer, false); action == ACT_ATCK) {
				ownRec.info = Record::battleFail;
				ownRec.lastAct = pair(killer, ACT_ATCK);
				endTurn();
			}
			throw string("Battle lost");
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
	prog->getState<ProgMatch>()->updateVictoryPoints(vpOwn, vpEne);
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
	prog->finishMatch(win);
}

void Game::surrender() {
	sendb.pushHead(Com::Code::record, Com::dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16) * 2));
	sendb.push(uint8(Record::loose));
	sendb.push({ uint16(UINT16_MAX), uint16(0) });
	prog->getNetcp()->sendData(sendb);
	prog->finishMatch(Record::loose);
}

void Game::prepareTurn(bool fcont) {
	bool xmov = eneRec.info == Record::battleFail;	// should only occur when myTurn is true
	Board::setTilesInteract(board->getTiles().begin(), board->getTiles().getSize(), Tile::Interact(myTurn));
	board->prepareTurn(myTurn, xmov, fcont, ownRec, eneRec);
	if (myTurn && !fcont)
		anyFavorUsed = lastFavorUsed = miscActionTaken = false;

	ProgMatch* pm = prog->getState<ProgMatch>();
	pm->updateIcons(fcont);
	pm->message->setText(myTurn ? xmov ? "Opponent lost a battle" : "Your turn" : "Opponent's turn");
}

void Game::endTurn() {
	sendb.pushHead(Com::Code::record, Com::dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16) * (2 + ownRec.protects.size())));	// 2 for last actor and protects size
	sendb.push(uint8(ownRec.info | (eneRec.info & Record::battleFail)));
	sendb.push({ board->inversePieceId(ownRec.lastAct.first), uint16(ownRec.protects.size()) });
	for (auto& [pce, prt] : ownRec.protects)
		sendb.push(uint16((board->inversePieceId(pce) & 0x7FFF) | (uint16(prt) << 15)));
	prog->getNetcp()->sendData(sendb);

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
	prog->getState<ProgMatch>()->updateVictoryPoints(vpOwn, vpEne);
	if (eneRec.info == Record::win || eneRec.info == Record::loose || eneRec.info == Record::tie)
		prog->finishMatch(eneRec.info == Record::win ? Record::loose : eneRec.info == Record::loose ? Record::win : Record::tie);
	else {
		myTurn = true;
		prepareTurn(fcont);
		if (board->getPxpad()->show)
			prog->getGui()->openPopupChoice("Destroy forest at " + toStr(board->ptog(board->getPxpad()->getPos()), "|") + '?', &Program::eventKillDestroy, &Program::eventCancelDestroy);
	}
	return prog->getNetcp();
}

void Game::sendConfig(bool onJoin) {
	uint ofs;
	ProgRoom* pr = prog->getState<ProgRoom>();
	Config& cfg = pr->confs[pr->configName->getText()];
	if (onJoin) {
		ofs = sendb.allocate(Com::Code::cnjoin, Com::dataHeadSize + 1 + cfg.dataSize(pr->configName->getText()));
		ofs = sendb.write(uint8(true), ofs);
	} else
		ofs = sendb.allocate(Com::Code::config, Com::dataHeadSize + cfg.dataSize(pr->configName->getText()));
	cfg.toComData(&sendb[ofs], pr->configName->getText());
	prog->getNetcp()->sendData(sendb);
}

void Game::sendStart() {
	myTurn = uint8(std::uniform_int_distribution<uint>(0, 1)(randGen));
	ProgRoom* pr = prog->getState<ProgRoom>();
	Config& cfg = pr->confs[pr->configName->getText()];
	uint ofs = sendb.allocate(Com::Code::start, Com::dataHeadSize + cfg.dataSize(pr->configName->getText()));
	ofs = sendb.write(uint8(!myTurn), ofs);
	cfg.toComData(&sendb[ofs], pr->configName->getText());
	prog->getNetcp()->sendData(sendb);
	prog->eventOpenSetup(pr->configName->getText());
}

void Game::recvStart(const uint8* data) {
	myTurn = data[0];
	prog->eventOpenSetup(board->config.fromComData(data + 1));
}

void Game::sendSetup() {
	uint tcnt = board->tileCompressionSize();
	uint ofs = sendb.allocate(Com::Code::setup, uint16(Com::dataHeadSize + tcnt + pieceLim * sizeof(uint16) + board->getPieces().getNum() * sizeof(uint16)));
	std::fill_n(&sendb[ofs], tcnt, 0);
	for (uint16 i = 0; i < board->getTiles().getExtra(); ++i)
		sendb[i/2+ofs] |= board->compressTile(i);
	ofs += tcnt;

	for (uint8 i = 0; i < pieceLim; ofs = sendb.write(board->ownPieceAmts[i++], ofs));
	for (uint16 i = 0; i < board->getPieces().getNum(); ++i)
		sendb.write(board->pieceOnBoard(board->getPieces().own(i)) ? board->invertId(board->posToId(board->ptog(board->getPieces().own(i)->getPos()))) : uint16(UINT16_MAX), i * sizeof(uint16) + ofs);
	prog->getNetcp()->sendData(sendb);
}

void Game::recvSetup(const uint8* data) {
	// set tiles and pieces
	for (uint16 i = 0; i < board->getTiles().getHome(); ++i)
		board->getTiles()[i].setType(board->decompressTile(data, i));
	for (uint16 i = 0; i < board->config.homeSize.x; ++i)
		prog->getState<ProgSetup>()->rcvMidBuffer[i] = board->decompressTile(data, board->getTiles().getHome() + i);

	uint16 ofs = board->tileCompressionSize();
	for (uint8 i = 0; i < pieceLim; ++i, ofs += sizeof(uint16))
		board->enePieceAmts[i] = Com::read16(data + ofs);
	uint8 t = 0;
	for (uint16 i = 0, c = 0; i < board->getPieces().getNum(); ++i, ++c) {
		uint16 id = Com::read16(data + i * sizeof(uint16) + ofs);
		for (; c >= board->enePieceAmts[t]; ++t, c = 0);
		board->getPieces().ene(i)->setType(PieceType(t));
		board->getPieces().ene(i)->setPos(board->gtop(id < board->getTiles().getHome() ? board->idToPos(id) : svec2(UINT16_MAX)));
	}

	// document thrones' fortresses
	for (Piece* throne = board->getPieces(board->getPieces().ene(), board->enePieceAmts, PieceType::throne); throne != board->getPieces().end(); ++throne)
		if (uint16 pos = board->posToId(board->ptog(throne->getPos())); board->getTiles()[pos].getType() == TileType::fortress)
			throne->lastFortress = pos;

	// finish up
	if (ProgSetup* ps = prog->getState<ProgSetup>(); ps->getStage() == ProgSetup::Stage::ready)
		prog->eventOpenMatch();
	else {
		ps->enemyReady = true;
		ps->message->setText(ps->getStage() >= ProgSetup::Stage::pieces ? "Opponent ready" : "Hurry the fuck up!");
	}
}

void Game::recvMove(const uint8* data) {
	Piece& pce = board->getPieces()[Com::read16(data)];
	uint16 pos = Com::read16(data + sizeof(uint16));
	if (pce.updatePos(board->idToPos(pos)); pce.getType() == PieceType::throne && board->getTiles()[pos].getType() == TileType::fortress)
		pce.lastFortress = pos;
}

void Game::placePiece(Piece* piece, svec2 pos) {
	if (uint16 fid = board->posToId(pos); piece->getType() == PieceType::throne && board->getTiles()[fid].getType() == TileType::fortress && piece->lastFortress != fid)
		if (piece->lastFortress = fid; availableFF < std::accumulate(favorsLeft.begin(), favorsLeft.end(), uint16(0)))
			++availableFF;

	piece->updatePos(pos, true);
	sendb.pushHead(Com::Code::move);
	sendb.push({ board->inversePieceId(piece), board->invertId(board->posToId(pos)) });
	prog->getNetcp()->sendData(sendb);
}

void Game::recvKill(const uint8* data) {
	Piece* pce = &board->getPieces()[Com::read16(data)];
	if (board->isOwnPiece(pce))
		board->setPxpadPos(pce);
	pce->updatePos();
}

void Game::recvBreach(const uint8* data) {
	board->getTiles()[Com::read16(data)].setBreached(data[sizeof(uint16)]);
}

void Game::removePiece(Piece* piece) {
	piece->updatePos();
	sendb.pushHead(Com::Code::kill);
	sendb.push(board->inversePieceId(piece));
	prog->getNetcp()->sendData(sendb);
}

void Game::breachTile(Tile* tile, bool yes) {
	tile->setBreached(yes);
	sendb.pushHead(Com::Code::breach);
	sendb.push(board->inverseTileId(tile));
	sendb.push(uint8(yes));
	prog->getNetcp()->sendData(sendb);
}

void Game::recvTile(const uint8* data) {
	uint16 pos = Com::read16(data);
	if (board->getTiles()[pos].setType(TileType(data[sizeof(uint16)] & 0xF)); board->getTiles()[pos].getType() == TileType::fortress)
		if (Piece* pce = board->findOccupant(board->idToPos(pos)); pce && pce->getType() == PieceType::throne)
			pce->lastFortress = pos;
	if (TileTop top = TileTop(data[sizeof(uint16)] >> 4); top != TileTop::none)
		board->setTileTop(top, &board->getTiles()[pos]);
}

void Game::changeTile(Tile* tile, TileType type, TileTop top) {
	if (tile->setType(type); top != TileTop::none)
		board->setTileTop(top, tile);
	sendb.pushHead(Com::Code::tile);
	sendb.push(board->inverseTileId(tile));
	sendb.push(uint8(uint8(type) | (top.invert() << 4)));
	prog->getNetcp()->sendData(sendb);
}

std::default_random_engine Game::createRandomEngine() {
	std::default_random_engine randGen;
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(time(nullptr)));
		std::cerr << "failed to use random_device" << std::endl;
	}
	return randGen;
}

#ifdef DEBUG
void Game::processCommand(const char* cmd) {
	if (string key = readWord(cmd); !SDL_strcasecmp(key.c_str(), "m")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPieceId(cmd), true);
	} else if (!SDL_strcasecmp(key.c_str(), "move")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPiecePos(cmd), true);
	} else if (!SDL_strcasecmp(key.c_str(), "s")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPieceId(cmd), false);
	} else if (!SDL_strcasecmp(key.c_str(), "swap")) {
		while (*cmd)
			readCommandPieceMove(cmd, readCommandPiecePos(cmd), false);
	} else if (!SDL_strcasecmp(key.c_str(), "switch")) {
		while (*cmd)
			if (Piece* a = readCommandPieceId(cmd), *b = readCommandPieceId(cmd); a && b) {
				svec2 pos = board->ptog(b->getPos());
				placePiece(b, board->ptog(a->getPos()));
				placePiece(a, pos);
			}
	} else if (!SDL_strcasecmp(key.c_str(), "k")) {
		while (*cmd)
			if (Piece* pce = readCommandPieceId(cmd))
				removePiece(pce);
	} else if (!SDL_strcasecmp(key.c_str(), "kill")) {
		while (*cmd)
			if (Piece* pce = readCommandPiecePos(cmd))
				removePiece(pce);
	} else if (!SDL_strcasecmp(key.c_str(), "killall")) {
		while (*cmd) {
			bool own = readCommandPref(cmd);
			if (PieceType type = strToEnum<PieceType>(pieceNames, readWord(cmd)); type <= PieceType::throne) {
				Piece* pces = own ? board->getOwnPieces(type) : board->getEnePieces(type);
				for (uint16 i = 0; i < board->config.pieceAmounts[uint8(type)]; ++i)
					removePiece(pces + i);
			}
		}
	} else if (!SDL_strcasecmp(key.c_str(), "c")) {
		while (*cmd)
			if (auto [id, name] = readCommandTileId(cmd); id < board->getTiles().getSize())
				readCommandChange(id, name.c_str());
	} else if (!SDL_strcasecmp(key.c_str(), "change")) {
		while (*cmd)
			if (auto [pos, name] = readCommandTilePos(cmd); inRange(pos, svec2(0), board->boardLimit()))
				readCommandChange(board->posToId(pos), name.c_str());
	} else if (!SDL_strcasecmp(key.c_str(), "changeall")) {
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
	} else if (!SDL_strcasecmp(key.c_str(), "b")) {
		while (*cmd)
			if (auto [id, yes] = readCommandTileId(cmd); id < board->getTiles().getSize())
				breachTile(&board->getTiles()[id], stob(yes));
	} else if (!SDL_strcasecmp(key.c_str(), "breach"))
		while (*cmd)
			if (auto [pos, yes] = readCommandTilePos(cmd); inRange(pos, svec2(0), board->boardLimit()))
				breachTile(board->getTile(pos), stob(yes));
}

Piece* Game::readCommandPieceId(const char*& cmd) {
	auto [own, id] = readCommandMnum(cmd, { '-' });
	if (own)
		return id < board->getPieces().getSize() ? board->getPieces().own() + id : nullptr;
	return id < board->getPieces().getNum() ? board->getPieces().ene() + id : nullptr;
}

inline Piece* Game::readCommandPiecePos(const char*& cmd) {
	return board->findOccupant(readCommandVec(cmd) % board->boardLimit());
}

void Game::readCommandPieceMove(const char*& cmd, Piece* pce, bool killOccupant) {
	auto [xm, xv] = readCommandMnum(cmd);
	auto [ym, yv] = readCommandMnum(cmd);
	if (pce) {
		svec2 pos = board->ptog(pce->getPos());
		pos.x = !xm ? pos.x + xv : xm == 1 ? pos.x - xv : xv;
		pos.y = !ym ? pos.y + yv : ym == 1 ? pos.y - yv : yv;
		if (Piece* occ = board->findOccupant(pos))
			killOccupant ? removePiece(occ) : placePiece(occ, board->ptog(pce->getPos()));
		placePiece(pce, pos);
	}
}

pair<uint16, string> Game::readCommandTileId(const char*& cmd) {
	uint16 id = readNumber<uint16>(cmd, strtoul, 0);
	return pair(id, readWord(cmd));
}

pair<svec2, string> Game::readCommandTilePos(const char*& cmd) {
	svec2 pos = readCommandVec(cmd);
	return pair(pos, readWord(cmd));
}

void Game::readCommandChange(uint16 id, const char* name) {
	bool own = readCommandPref(name);
	if (TileType type = strToEnum<TileType>(tileNames, name); type < TileType::empty)
		changeTile(&board->getTiles()[id], type, board->findTileTop(&board->getTiles()[id]));
	else if (TileTop top = strToEnum<TileTop>(TileTop::names, name); top <= TileTop::ownCity)
		changeTile(&board->getTiles()[id], board->getTiles()[id].getType(), own ? top.type : top.invert());
}

svec2 Game::readCommandVec(const char*& cmd) {
	uint16 x = readNumber<uint16>(cmd, strtoul, 0);
	return svec2(x, readNumber<uint16>(cmd, strtoul, 0));
}

pair<uint8, uint16> Game::readCommandMnum(const char*& cmd, initlist<char> pref) {
	uint8 p = readCommandPref(cmd, pref);
	return pair(p, readNumber<uint16>(cmd, strtoul, 0));
}

uint8 Game::readCommandPref(const char*& cmd, initlist<char> chars) {
	for (; isSpace(*cmd); ++cmd);
	initlist<char>::iterator pos = std::find(chars.begin(), chars.end(), *cmd);
	if (pos != chars.end())
		++cmd;
	return pos - chars.begin();
}
#endif
