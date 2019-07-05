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

void Program::eventConfigDeleteInput(Button*) {
	World::scene()->setPopup(ProgState::createPopupChoice("Are you sure?", &Program::eventConfigDelete, &Program::eventClosePopup));
}

void Program::eventConfigDelete(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs.erase(ph->confs.begin() + pdift(ph->curConf));
	if (ph->curConf >= ph->confs.size())
		ph->curConf--;
	World::scene()->resetLayouts();
}

void Program::eventConfigCopyInput(Button*) {
	World::scene()->setPopup(ProgState::createPopupInput("Name:", &Program::eventConfigCopy));
}

void Program::eventConfigCopy(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText();
	if (std::find_if(ph->confs.begin(), ph->confs.end(), [name](const Com::Config& it) -> bool { return it.name == name; }) == ph->confs.end()) {
		Com::Config cfg = ph->confs[ph->curConf];
		cfg.name = name;
		ph->confs.push_back(cfg);
		ph->curConf = ph->confs.size() - 1;
		World::scene()->resetLayouts();
	} else
		World::scene()->setPopup(ProgState::createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventConfigNewInput(Button*) {
	World::scene()->setPopup(ProgState::createPopupInput("Name:", &Program::eventConfigNew));
}

void Program::eventConfigNew(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText();
	if (std::find_if(ph->confs.begin(), ph->confs.end(), [name](const Com::Config& it) -> bool { return it.name == name; }) == ph->confs.end()) {
		ph->confs.emplace_back(name);
		ph->curConf = ph->confs.size() - 1;
		World::scene()->resetLayouts();
	} else
		World::scene()->setPopup(ProgState::createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventUpdateSurvivalSL(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	uint8 prc = static_cast<Slider*>(but)->getVal();
	ph->confs[ph->curConf].survivalPass = prc;
	ph->inSurvivalLE->setText(to_string(prc) + '%');
}

void Program::eventUpdateConfig(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	Com::Config& cfg = ph->confs[ph->curConf];

	cfg.homeWidth = uint16(sstol(ph->inWidth->getText()));
	cfg.homeHeight = uint16(sstol(ph->inHeight->getText()));
	cfg.survivalPass = uint8(sstoul(ph->inSurvivalLE->getText()));
	cfg.favorLimit = uint8(sstoul(ph->inFavors->getText()));
	cfg.dragonDist = uint8(sstoul(ph->inDragonDist->getText()));
	cfg.dragonDiag = ph->inDragonDiag->on;
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		cfg.tileAmounts[i] = uint16(sstoul(ph->inTiles[i]->getText()));
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		cfg.middleAmounts[i] = uint16(sstoul(ph->inMiddles[i]->getText()));
	for (uint8 i = 0; i < Com::pieceMax; i++)
		cfg.pieceAmounts[i] = uint16(sstoul(ph->inPieces[i]->getText()));
	cfg.winFortress = uint16(sstol(ph->inWinFortress->getText()));
	cfg.winThrone = uint16(sstol(ph->inWinThrone->getText()));
	cfg.readCapturers(ph->inCapturers->getText());
	cfg.shiftLeft = ph->inShiftLeft->on;
	cfg.shiftNear = ph->inShiftNear->on;
	
	cfg.checkValues();
	updateConfigWidgets(ph, cfg);
}

void Program::updateConfigWidgets(ProgHost* ph, const Com::Config& cfg) {
	ph->inWidth->setText(to_string(cfg.homeWidth));
	ph->inHeight->setText(to_string(cfg.homeHeight));
	ph->inSurvivalSL->setVal(cfg.survivalPass);
	ph->inSurvivalLE->setText(to_string(cfg.survivalPass) + '%');
	ph->inFavors->setText(to_string(cfg.favorLimit));
	ph->inDragonDist->setText(to_string(cfg.dragonDist));
	ph->inDragonDiag->on = cfg.dragonDiag;
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		ph->inTiles[i]->setText(to_string(cfg.tileAmounts[i]));
	ph->outTileFortress->setText(ProgHost::tileFortressString(cfg));
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		ph->inMiddles[i]->setText(to_string(cfg.middleAmounts[i]));
	ph->outMiddleFortress->setText(ProgHost::middleFortressString(cfg));
	for (uint8 i = 0; i < Com::pieceMax; i++)
		ph->inPieces[i]->setText(to_string(cfg.pieceAmounts[i]));
	ph->outPieceTotal->setText(ProgHost::pieceTotalString(cfg));
	ph->inWinFortress->setText(to_string(cfg.winFortress));
	ph->inWinThrone->setText(to_string(cfg.winThrone));
	ph->inCapturers->setText(cfg.capturersString());
	ph->inShiftLeft->on = cfg.shiftLeft;
	ph->inShiftNear->on = cfg.shiftNear;
}

void Program::eventUpdateReset(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->confs[ph->curConf] = Com::Config(ph->confs[ph->curConf].name);
	updateConfigWidgets(ph, ph->confs[ph->curConf]);
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

	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setStage(ProgSetup::Stage::tiles);
	ps->rcvMidBuffer.resize(game.getConfig().homeWidth);
}

void Program::eventPlaceTileH() {
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->select)) {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		if (uint8 tid = ps->getSelected(); ps->getCount(tid))
			placeTile(til, tid);
	}
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
	tile->setInteractivity(Tile::Interactivity::interact);
	ps->incdecIcon(type, false, true);
}

void Program::eventPlacePieceH() {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce)) {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		if (uint8 tid = ps->getSelected(); ps->getCount(tid))
			placePiece(pos, tid, pce);
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
		dst->setInteractivity(Tile::Interactivity::interact);
		src->setType(desType);
		src->setInteractivity(Tile::Interactivity::interact);
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

void Program::eventClearTile() {
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->select); til && til->getType() != Com::Tile::empty) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true, true);
		til->setType(Com::Tile::empty);
		til->setInteractivity(Tile::Interactivity::interact);
	}
}

void Program::eventClearPiece() {
	vec2s pos;
	Piece* pce;
	if (pickBob(pos, pce); pce) {
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
	World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(Game::screenPosDown.x, Game::screenPosDown.y, game.getScreen()->pos.z)) })));
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, Camera::latMatch) })));
}

void Program::eventEndTurn(Button*) {
	game.endTurn();
}

void Program::eventPlaceDragon(Button*) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(pos, pce);
}

void Program::eventFavorStart(BoardObject* obj) {
	game.favorState.piece = static_cast<Piece*>(obj);
	game.updateFavorState();
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
		World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(Game::screenPosUp.x, Game::screenPosUp.y, game.getScreen()->pos.z)) })));
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
	World::winSys()->setScreen(Settings::Screen(ps->screen->getCurOpt()), vec2i::get(ps->winSize->getText(), strtoul, 0), ps->currentMode(), uint8(sstoul(ps->msample->getText())));
}

void Program::eventSetVsync(Button* but) {
	World::winSys()->setVsync(Settings::VSync(static_cast<SwitchBox*>(but)->getCurOpt() - 1));
}

void Program::eventSetSmooth(Button* but) {
	World::winSys()->setSmooth(Settings::Smooth(static_cast<SwitchBox*>(but)->getCurOpt()));
}

void Program::eventSetGammaSL(Button* but) {
	World::winSys()->setGamma(float(static_cast<Slider*>(but)->getVal()) / ProgSettings::gammaStepFactor);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(trimZero(to_string(World::sets()->gamma)));
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::winSys()->setGamma(sstof(le->getText()));
	le->setText(trimZero(to_string(World::sets()->gamma)));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(int(World::sets()->gamma * ProgSettings::gammaStepFactor));
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

void Program::eventSBNext(Button* but) {
	static_cast<SwitchBox*>(but->getParent()->getWidget(but->getID() - 1))->onClick(0, SDL_BUTTON_LEFT);
}

void Program::eventSBPrev(Button* but) {
	static_cast<SwitchBox*>(but->getParent()->getWidget(but->getID() + 1))->onClick(0, SDL_BUTTON_RIGHT);
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
	} else {
		pos = INT16_MIN;
		pce = nullptr;
	}
	return bob;
}
