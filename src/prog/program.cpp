#include "engine/world.h"
#include <cassert>

void Program::start() {
	if (const string& addr = World::getOpt(World::argAddress); !addr.empty())
		World::sets()->address = addr;
	else if (World::hasFlag(World::argAddress))
		World::sets()->address = Settings::loopback;

	if (const string& port = World::getOpt(World::argPort); !port.empty())
		World::sets()->port = uint16(sstoul(port));
	else if (World::hasFlag(World::argPort))
		World::sets()->port = Com::defaultPort;

	World::scene()->setObjects(game.initObjects());	// doesn't need to be here but I like having the game board in the background
	eventOpenMainMenu();
	if (World::hasFlag(World::argConnect))
		eventConnectServer(nullptr);
}

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	game.uninitObjects();	// either do this or reset objects
	setState(new ProgMenu);
}

void Program::eventConnectServer(Button*) {
	game.connect();
}

void Program::eventConnectCancel(Button*) {
	game.disconnect();
	World::scene()->setPopup(nullptr);
}

void Program::eventUpdateAddress(Button* but) {
	World::sets()->address = static_cast<LabelEdit*>(but)->getText();
}

void Program::eventUpdatePort(Button* but) {
	World::sets()->port = uint16(sstoul(static_cast<LabelEdit*>(but)->getText()));
}

// HOST MENU

void Program::eventOpenHostMenu(Button*) {
	setState(new ProgHost);
}

void Program::eventHostServer(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	Com::saveConfs(ph->confs, Com::defaultConfigFile);
	game.conhost(ph->confs[ph->curConf]);
}

void Program::eventSwitchConfig(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->curConf = static_cast<SwitchBox*>(but)->getCurOpt();
	ph->confs[ph->curConf].checkValues();
	World::scene()->resetLayouts();
}

void Program::eventUpdateWidth(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].homeWidth = uint16(sstol(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].homeWidth));
}

void Program::eventUpdateHeight(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].homeHeight = uint16(sstol(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].homeHeight));
}

void Program::eventUpdateTile(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	uint8 id = strToEnum<uint8>(Com::tileNames, static_cast<Label*>(le->getParent()->getWidget(0))->getText());
	ph->confs[ph->curConf].tileAmounts[id] = uint16(sstoul(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].tileAmounts[id]));
}

void Program::eventUpdatePiece(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	uint8 id = strToEnum<uint8>(Com::tileNames, static_cast<Label*>(le->getParent()->getWidget(0))->getText());
	ph->confs[ph->curConf].pieceAmounts[id] = uint16(sstoul(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].pieceAmounts[id]));
}

void Program::eventUpdateWinThrone(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].winThrone = uint16(sstol(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].winThrone));
}

void Program::eventUpdateWinFortress(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].winFortress = uint16(sstol(le->getText()));
	ph->confs[ph->curConf].checkValues();
	le->setText(to_string(ph->confs[ph->curConf].winFortress));
}

void Program::eventUpdateCapturers(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].readCapturers(le->getText());
	ph->confs[ph->curConf].checkValues();
	le->setText(ph->confs[ph->curConf].capturersString());
}

void Program::eventUpdateShiftLeft(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].shiftLeft = static_cast<CheckBox*>(but)->on;
}

void Program::eventUpdateShiftNear(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf].shiftNear = static_cast<CheckBox*>(but)->on;
}

void Program::eventUpdateReset(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf] = Com::Config(ph->confs[ph->curConf].name);
	World::scene()->resetLayouts();
}

void Program::eventExitHost(Button*) {
	Com::saveConfs(static_cast<ProgHost*>(state.get())->confs, Com::defaultConfigFile);
	eventOpenMainMenu();
}

// GAME SETUP

void Program::eventOpenSetup() {
#ifdef NDEBUG
	World::scene()->setObjects(game.initObjects());
#else
	World::scene()->setObjects(World::hasFlag(World::argSetup) ? game.initDummyObjects() : game.initObjects());
#endif
	setState(new ProgSetup);
	static_cast<ProgSetup*>(state.get())->setStage(ProgSetup::Stage::tiles);
}

void Program::eventPlaceTileC(BoardObject* obj) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (uint8 tid = ps->getSelected(); ps->getCount(tid))
		placeTile(static_cast<Tile*>(obj), tid);
}

void Program::eventPlaceTileD(Button* but) {
	if (Tile* tile = dynamic_cast<Tile*>(World::scene()->select))
		placeTile(tile, uint8(but->getID() - 1));
}

void Program::placeTile(Tile* tile, uint8 type) {
	// remove any piece that may already be there
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (tile->getType() < Com::Tile::fortress)
		ps->incdecIcon(uint8(tile->getType()), true, true);

	// place the tile
	tile->setType(Com::Tile(type));
	tile->setCalls(Tile::Interactivity::tiling);
	ps->incdecIcon(type, false, true);
}

void Program::eventPlacePieceC(BoardObject* obj) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (uint8 tid = ps->getSelected(); ps->getCount(tid)) {
		vec2s pos = game.ptog(obj->pos);
		placePiece(pos, tid, extractPiece(obj, pos));
	}
}

void Program::eventPlacePieceD(Button* but) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		placePiece(pos, uint8(but->getID() - 1), pce);
}

void Program::placePiece(vec2s pos, uint8 type, Piece* occupant) {
	// remove any piece that may be occupying that tile already
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (occupant) {
		occupant->disable();
		ps->incdecIcon(uint8(occupant->getType()), true, false);
	}

	// find the first not placed piece of the specified type and place it if it exists
	Piece* pieces = game.getOwnPieces(Com::Piece(type));
	for (uint8 i = 0; i < game.getConfig().pieceAmounts[type]; i++)
		if (!pieces[i].active()) {
			pieces[i].enable(pos);
			ps->incdecIcon(type, false, false);
			break;
		}
}

void Program::eventMoveTile(BoardObject* obj) {
	// switch types with destination tile
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->select)) {
		Com::Tile desType = dst->getType();
		dst->setType(src->getType());
		dst->setCalls(Tile::Interactivity::tiling);
		src->setType(desType);
		src->setCalls(Tile::Interactivity::tiling);
	}
}

void Program::eventMovePiece(BoardObject* obj) {
	// get new position and set or swap positions with possibly already occupying piece
	vec2s pos;
	if (Piece* dst; pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->pos = src->pos;
		src->pos = game.gtop(pos, BoardObject::upperPoz);
	}
}

void Program::eventClearTile(BoardObject* obj) {
	Tile* til = static_cast<Tile*>(obj);

	static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true, true);
	til->setType(Com::Tile::empty);
	til->setCalls(Tile::Interactivity::tiling);
}

void Program::eventClearPiece(BoardObject* obj) {
	if (Piece* pce = extractPiece(obj, game.ptog(obj->pos))) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(pce->getType()), true, false);
		pce->disable();
	}
}

void Program::eventSetupNext(Button*) {
	try {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		switch (ps->getStage()) {
		case ProgSetup::Stage::tiles:
			game.checkOwnTiles();
			game.fillInFortress();
			break;
		case ProgSetup::Stage::middles:
			game.checkMidTiles();
			break;
		case ProgSetup::Stage::pieces:
			game.checkOwnPieces();
		}
		if (ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) + 1))) {
			game.sendSetup();
			ps->enemyReady ? eventOpenMatch() : eventShowWaitPopup();
		}
	} catch (const string& err) {
		World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventShowWaitPopup(Button*) {
	World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventExitGame, "Exit"));
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.prepareMatch();
	setState(new ProgMatch);
	World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, Game::screenPosDown) })));
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, Camera::latMatch) })));
}

void Program::eventPlaceFavor(Button*) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeFavor(pos);
}

void Program::eventPlaceDragon(Button*) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(pos, pce);
}

void Program::eventMove(BoardObject* obj) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.pieceMove(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventFire(BoardObject* obj) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.pieceFire(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventExitGame(Button*) {
	if (dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, Game::screenPosUp) })));
		World::scene()->addAnimation(Animation(World::scene()->getCamera(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posSetup, Camera::latSetup) })));
	}
	game.disconnect();
	eventOpenMainMenu();
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState(new ProgSettings);
}

void Program::eventApplySettings(Button*) {
	ProgSettings* ps = static_cast<ProgSettings*>(state.get());
	World::winSys()->setScreen(Settings::Screen(ps->screen->getCurOpt()), vec2i::get(ps->winSize->getText(), strtoul, 0), strToDisp(ps->dspMode->getText()), uint8(sstoul(ps->msample->getText())));
}

void Program::eventSetVsync(Button* but) {
	World::winSys()->setVsync(Settings::VSync(static_cast<SwitchBox*>(but)->getCurOpt() - 1));
}

void Program::eventSetSmooth(Button* but) {
	World::winSys()->setSmooth(Settings::Smooth(static_cast<SwitchBox*>(but)->getCurOpt()));
}

void Program::eventResetSettings(Button*) {
	World::winSys()->resetSettings();
}

void Program::eventOpenInfo(Button*) {
	setState(new ProgInfo);
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button*) {
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state.reset(newState);
	World::scene()->resetLayouts();
}

BoardObject* Program::pickBob(vec2s& pos, Piece*& pce) {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->select);
	if (bob) {
		pos = game.ptog(bob->pos);
		pce = extractPiece(bob, pos);
	}
	return bob;
}
