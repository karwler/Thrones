#include "engine/world.h"
#include <cassert>

// PROGRAM

Program::Program() :
	state(new ProgState)	// necessary as a placeholder to prevent nullptr exceptions
{
	World::window()->writeLog("starting");
}

void Program::start() {
	if (const char* addr = World::args.getOpt(World::argAddress); addr && *addr)
		World::sets()->address = addr;
	else if (World::args.hasFlag(World::argAddress))
		World::sets()->address = Settings::loopback;

	if (const char* port = World::args.getOpt(World::argPort); port && *port)
		World::sets()->port = uint16(sstoul(port));
	else if (World::args.hasFlag(World::argPort))
		World::sets()->port = Com::defaultPort;

	World::scene()->setObjects(game.initObjects());	// doesn't need to be here but I like having the game board in the background
	eventOpenMainMenu();
	if (World::args.hasFlag(World::argConnect))
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
	Com::Config::save(ph->confs);
	game.conhost(ph->curConf->second);
}

void Program::eventSwitchConfig(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	ph->curConf = ph->confs.find(static_cast<SwitchBox*>(but)->getText());
	ph->curConf->second.checkValues();
	World::scene()->resetLayouts();
}

void Program::eventConfigDeleteInput(Button*) {
	World::scene()->setPopup(ProgState::createPopupChoice("Are you sure?", &Program::eventConfigDelete, &Program::eventClosePopup));
}

void Program::eventConfigDelete(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	vector<string> names = sortNames(ph->confs);
	vector<string>::iterator nit = std::find(names.begin(), names.end(), ph->curConf->first);

	ph->confs.erase(ph->curConf);
	ph->curConf = ph->confs.find(nit + 1 != names.end() ? *(nit + 1) : *(nit - 1));
	Com::Config::save(ph->confs);
	World::scene()->resetLayouts();
}

void Program::eventConfigCopyInput(Button*) {
	World::scene()->setPopup(ProgState::createPopupInput("Name:", &Program::eventConfigCopy));
}

void Program::eventConfigCopy(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText();
	if (ph->confs.find(name) == ph->confs.end()) {
		ph->curConf = ph->confs.emplace(name, ph->curConf->second).first;
		Com::Config::save(ph->confs);
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
	if (ph->confs.find(name) == ph->confs.end()) {
		ph->curConf = ph->confs.emplace(name, Com::Config()).first;
		Com::Config::save(ph->confs);
		World::scene()->resetLayouts();
	} else
		World::scene()->setPopup(ProgState::createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventUpdateSurvivalSL(Button* but) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	uint8 prc = uint8(static_cast<Slider*>(but)->getVal());
	ph->curConf->second.survivalPass = prc;
	ph->wio.survivalLE->setText(toStr(prc) + '%');
	Com::Config::save(ph->confs);
}

void Program::eventUpdateConfig(Button*) {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	Com::Config& cfg = ph->curConf->second;

	cfg.homeWidth = uint16(sstol(ph->wio.width->getText()));
	cfg.homeHeight = uint16(sstol(ph->wio.height->getText()));
	cfg.survivalPass = uint8(sstoul(ph->wio.survivalLE->getText()));
	cfg.favorLimit = uint8(sstoul(ph->wio.favors->getText()));
	cfg.dragonDist = uint8(sstoul(ph->wio.dragonDist->getText()));
	cfg.dragonDiag = ph->wio.dragonDiag->on;
	cfg.multistage = ph->wio.multistage->on;
	cfg.survivalKill = ph->wio.survivalKill->on;
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		cfg.tileAmounts[i] = uint16(sstoul(ph->wio.tiles[i]->getText()));
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		cfg.middleAmounts[i] = uint16(sstoul(ph->wio.middles[i]->getText()));
	for (uint8 i = 0; i < Com::pieceMax; i++)
		cfg.pieceAmounts[i] = uint16(sstoul(ph->wio.pieces[i]->getText()));
	cfg.winFortress = uint16(sstol(ph->wio.winFortress->getText()));
	cfg.winThrone = uint16(sstol(ph->wio.winThrone->getText()));
	cfg.readCapturers(ph->wio.capturers->getText());
	cfg.shiftLeft = ph->wio.shiftLeft->on;
	cfg.shiftNear = ph->wio.shiftNear->on;
	
	cfg.checkValues();
	updateConfigWidgets();
}

void Program::updateConfigWidgets() {
	ProgHost* ph = static_cast<ProgHost*>(state.get());
	Com::Config& cfg = ph->curConf->second;

	ph->wio.width->setText(toStr(cfg.homeWidth));
	ph->wio.height->setText(toStr(cfg.homeHeight));
	ph->wio.survivalSL->setVal(cfg.survivalPass);
	ph->wio.survivalLE->setText(toStr(cfg.survivalPass) + '%');
	ph->wio.favors->setText(toStr(cfg.favorLimit));
	ph->wio.dragonDist->setText(toStr(cfg.dragonDist));
	ph->wio.dragonDiag->on = cfg.dragonDiag;
	ph->wio.multistage->on = cfg.multistage;
	ph->wio.survivalKill->on = cfg.survivalKill;
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		ph->wio.tiles[i]->setText(toStr(cfg.tileAmounts[i]));
	ph->wio.tileFortress->setText(ProgState::tileFortressString(cfg));
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		ph->wio.middles[i]->setText(toStr(cfg.middleAmounts[i]));
	ph->wio.middleFortress->setText(ProgState::middleFortressString(cfg));
	for (uint8 i = 0; i < Com::pieceMax; i++)
		ph->wio.pieces[i]->setText(toStr(cfg.pieceAmounts[i]));
	ph->wio.pieceTotal->setText(ProgState::pieceTotalString(cfg));
	ph->wio.winFortress->setText(toStr(cfg.winFortress));
	ph->wio.winThrone->setText(toStr(cfg.winThrone));
	ph->wio.capturers->setText(cfg.capturersString());
	ph->wio.shiftLeft->on = cfg.shiftLeft;
	ph->wio.shiftNear->on = cfg.shiftNear;
	Com::Config::save(ph->confs);
}

void Program::eventUpdateReset(Button*) {
	static_cast<ProgHost*>(state.get())->curConf->second = Com::Config();
	updateConfigWidgets();
}

void Program::eventShowConfig(Button*) {
	World::scene()->setPopup(ProgState::createPopupConfig(game.getConfig()));
}

// GAME SETUP

void Program::eventOpenSetup() {
#ifdef NDEBUG
	World::scene()->setObjects(game.initObjects());
#else
	World::scene()->setObjects(World::args.hasFlag(World::argSetup) ? game.initDummyObjects() : game.initObjects());
#endif
	setState(new ProgSetup);

	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setStage(ProgSetup::Stage::tiles);
	ps->rcvMidBuffer.resize(game.getConfig().homeWidth);
}

void Program::eventIconSelect(Button* but) {
	static_cast<ProgSetup*>(state.get())->setSelected(uint8(but->getID() - 1));
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
	std::find_if(pieces, pieces + game.getConfig().pieceAmounts[type], [](Piece& it) -> bool { return !it.active(); })->enable(pos);
	ps->incdecIcon(type, false, false);
}

void Program::eventMoveTile(BoardObject* obj, uint8) {
	// switch types with destination tile
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->select)) {
		Com::Tile desType = dst->getType();
		dst->setType(src->getType());
		dst->setInteractivity(Tile::Interactivity::interact);
		src->setType(desType);
		src->setInteractivity(Tile::Interactivity::interact);
	}
}

void Program::eventMovePiece(BoardObject* obj, uint8) {
	// get new position and set or swap positions with possibly already occupying piece
	vec2s pos;
	if (Piece* dst; pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->setPos(game.gtop(pos));
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

void Program::eventOpenSetupSave(Button*) {
	World::scene()->setPopup(static_cast<ProgSetup*>(state.get())->createPopupSaveLoad(true));
}

void Program::eventOpenSetupLoad(Button*) {
	World::scene()->setPopup(static_cast<ProgSetup*>(state.get())->createPopupSaveLoad(false));
}

void Program::eventSetupNew(Button* but) {
	const string& name = static_cast<LabelEdit*>(dynamic_cast<LabelEdit*>(but) ? but : but->getParent()->getWidget(but->getID() - 1))->getText();
	if (umap<string, Setup>& setups = static_cast<ProgSetup*>(state.get())->setups; setups.find(name) == setups.end())
		popuplateSetup(setups.emplace(name, Setup()).first->second);
	else
		World::scene()->setPopup(ProgState::createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventSetupSave(Button* but) {
	umap<string, Setup>& setups = static_cast<ProgSetup*>(state.get())->setups;
	Setup& stp = setups.find(static_cast<Label*>(but)->getText())->second;
	stp.clear();
	popuplateSetup(stp);
}

void Program::popuplateSetup(Setup& setup) {
	for (Tile* it = game.getTiles().own(); it != game.getTiles().end(); it++)
		if (it->getType() < Com::Tile::fortress)
			setup.tiles.emplace(game.ptog(it->getPos()), it->getType());
	for (Tile* it = game.getTiles().mid(); it != game.getTiles().own(); it++)
		if (it->getType() < Com::Tile::fortress)
			setup.mids.emplace(uint16(it - game.getTiles().mid()), it->getType());
	for (Piece* it = game.getPieces().own(); it != game.getPieces().ene(); it++)
		if (vec2s pos = game.ptog(it->getPos()); pos.hasNot(INT16_MIN))
			setup.pieces.emplace(pos, it->getType());

	FileSys::saveSetups(static_cast<ProgSetup*>(state.get())->setups);
	World::scene()->setPopup(nullptr);
}

void Program::eventSetupLoad(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	Setup& stp = ps->setups.find(static_cast<Label*>(but)->getText())->second;
	for (Tile* it = game.getTiles().mid(); it != game.getTiles().end(); it++)
		it->setType(Com::Tile::empty);
	for (Piece* it = game.getPieces().own(); it != game.getPieces().ene(); it++)
		it->disable();

	vector<uint16> cnt(game.getConfig().tileAmounts.begin(), game.getConfig().tileAmounts.end() - 1);
	for (const pair<const vec2s, Com::Tile>& it : stp.tiles)
		if (inRange(it.first, vec2s(0, 1), vec2s(game.getConfig().homeWidth - 1, game.getConfig().homeHeight)) && cnt[uint8(it.second)]) {
			cnt[uint8(it.second)]--;
			game.getTile(it.first)->setType(it.second);
		}
	cnt.assign(game.getConfig().middleAmounts.begin(), game.getConfig().middleAmounts.end());
	for (const pair<const uint16, Com::Tile>& it : stp.mids)
		if (it.first < game.getConfig().homeWidth && cnt[uint8(it.second)]) {
			cnt[uint8(it.second)]--;
			game.getTiles().mid(it.first)->setType(it.second);
		}
	cnt.assign(game.getConfig().pieceAmounts.begin(), game.getConfig().pieceAmounts.end());
	for (const pair<const vec2s, Com::Piece>& it : stp.pieces)
		if (inRange(it.first, vec2s(0, 1), vec2s(game.getConfig().homeWidth - 1, game.getConfig().homeHeight)) && cnt[uint8(it.second)]) {
			cnt[uint8(it.second)]--;
			Piece* pieces = game.getOwnPieces(it.second);
			std::find_if(pieces, pieces + game.getConfig().pieceAmounts[uint8(it.second)], [](Piece& pce) -> bool { return !pce.active(); })->enable(it.first);
		}
	ps->setStage(ProgSetup::Stage::tiles);
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.prepareMatch();
	setState(new ProgMatch);
	World::scene()->addAnimation(Animation(game.getScreen(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.getScreen()->getPos().x, Game::screenYDown, game.getScreen()->getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), vec3(), vec3(0.f)) })));
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, Camera::latMatch) })));
	World::scene()->getCamera()->pmax = Camera::pmaxMatch;
	World::scene()->getCamera()->ymax = Camera::ymaxMatch;
}

void Program::eventEndTurn(Button*) {
	game.endTurn();
}

void Program::eventPlaceDragon(Button*) {
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(pos, pce);
}

void Program::eventFavorStart(BoardObject* obj, uint8 mBut) {
	game.favorState.piece = static_cast<Piece*>(obj);
	game.updateFavorState();
	mBut == SDL_BUTTON_LEFT || game.favorState.piece->getType() == Com::Piece::warhorse ? game.highlightMoveTiles(game.favorState.piece) : game.highlightFireTiles(game.favorState.piece);
}

void Program::eventMove(BoardObject* obj, uint8 mBut) {
	game.highlightMoveTiles(nullptr);
	vec2s pos;
	if (Piece *mover = static_cast<Piece*>(obj), *pce; pickBob(pos, pce))
		game.pieceMove(mover, pos, pce, mover->getType() == Com::Piece::warhorse && mBut == SDL_BUTTON_LEFT);
}

void Program::eventFire(BoardObject* obj, uint8) {
	game.highlightFireTiles(nullptr);
	vec2s pos;
	if (Piece* pce; pickBob(pos, pce))
		game.pieceFire(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventExitGame(Button*) {
	if (dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.getScreen(), std::queue<Keyframe>({ Keyframe(0.f, Keyframe::CHG_SCL, vec3(), vec3(), vec3(1.f)), Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.getScreen()->getPos().x, Game::screenYUp, game.getScreen()->getPos().z)) })));
		World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posSetup, Camera::latSetup) })));
		for (Piece& it : game.getPieces())
			if (it.getPos().z <= game.getScreen()->getPos().z && it.getPos().z >= Com::Config::boardWidth / -2.f)
				World::scene()->addAnimation(Animation(&it, std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(it.getPos().x, -2.f, it.getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), vec3(), vec3(0.f)) })));
		World::scene()->getCamera()->pmax = Camera::pmaxSetup;
		World::scene()->getCamera()->ymax = Camera::ymaxSetup;
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
	World::window()->setScreen(uint8(sstoul(ps->display->getText())), Settings::Screen(ps->screen->getCurOpt()), vec2i::get(ps->winSize->getText(), strtoul, 0), ps->currentMode());
	ps->display->setText(toStr(World::sets()->display));
	ps->screen->setCurOpt(uint8(World::sets()->screen));
	ps->winSize->setText(World::sets()->size.toString(ProgSettings::rv2iSeparator));
	ps->dspMode->setText(ProgSettings::dispToFstr(World::sets()->mode));
}

void Program::eventSetVsync(Button* but) {
	World::window()->setVsync(Settings::VSync(static_cast<SwitchBox*>(but)->getCurOpt() - 1));
}

void Program::eventSetSamples(Button* but) {
	World::sets()->msamples = uint8(sstoul(static_cast<SwitchBox*>(but)->getText()));
}

void Program::eventSetGammaSL(Button* but) {
	World::window()->setGamma(float(static_cast<Slider*>(but)->getVal()) / ProgSettings::gammaStepFactor);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->gamma));
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::window()->setGamma(sstof(le->getText()));
	le->setText(toStr(World::sets()->gamma));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(int(World::sets()->gamma * ProgSettings::gammaStepFactor));
}

void Program::eventSetVolumeSL(Button* but) {
	World::sets()->avolume = uint8(static_cast<Slider*>(but)->getVal());
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->avolume));
}

void Program::eventSetVolumeLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->avolume = clampHigh(uint8(sstoul(le->getText())), uint8(SDL_MIX_MAXVOLUME));
	le->setText(toStr(World::sets()->avolume));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(World::sets()->avolume);
}

void Program::eventResetSettings(Button*) {
	World::window()->resetSettings();
}

void Program::eventOpenInfo(Button*) {
	setState(new ProgInfo);
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button*) {
	World::window()->close();
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
	pos = bob ? game.ptog(bob->getPos()) : INT16_MIN;
	pce = bob ? extractPiece(bob, pos) : nullptr;
	return bob;
}
