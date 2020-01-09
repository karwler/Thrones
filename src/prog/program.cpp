#include "engine/world.h"

// PROGRAM

Program::Program() :
	info(INF_NONE)
{
	World::window()->writeLog("starting");
}

void Program::start() {
	World::scene()->setObjects(game.initObjects(Com::Config()));	// doesn't need to be here but I like having the game board in the background
	eventOpenMainMenu();
}

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	info &= ~INF_HOST;
	setState(new ProgMenu);
}

void Program::eventConnectServer(Button*) {
	connect(true, "Connecting...");
}

void Program::eventConnectCancel(Button*) {
	disconnect();
	World::scene()->setPopup(nullptr);
}

void Program::eventUpdateAddress(Button* but) {
	World::sets()->address = static_cast<LabelEdit*>(but)->getText();
	eventSaveSettings();
}

void Program::eventResetAddress(Button* but) {
	World::sets()->address = Settings::loopback;
	static_cast<LabelEdit*>(but)->setText(World::sets()->address);
	eventSaveSettings();
}

void Program::eventUpdatePort(Button* but) {
	World::sets()->port = uint16(sstoul(static_cast<LabelEdit*>(but)->getText()));
	static_cast<LabelEdit*>(but)->setText(toStr(World::sets()->port));
	eventSaveSettings();
}

void Program::eventResetPort(Button* but) {
	World::sets()->port = Com::defaultPort;
	static_cast<LabelEdit*>(but)->setText(toStr(World::sets()->port));
	eventSaveSettings();
}

// LOBBY MENU

void Program::eventOpenLobby(uint8* data) {
	vector<pair<string, bool>> rooms(*data++);
	for (pair<string, bool>& it : rooms) {
		it.second = *data++;
		it.first = Com::readName(data);
		data += it.first.length() + 1;
	}
	info &= ~INF_HOST;
	netcp->setCncproc(&Netcp::cprocLobby);
	setState(new ProgLobby(std::move(rooms)));
}

void Program::eventHostRoomInput(Button*) {
	World::scene()->setPopup(state->createPopupInput("Name:", &Program::eventHostRoomRequest, Com::roomNameLimit));
}

void Program::eventHostRoomRequest(Button*) {
	try {
		ProgLobby* pl = static_cast<ProgLobby*>(state.get());
		if (pl->roomsMaxed())
			throw "Server full";
		const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText();
		if (pl->roomTaken(name))
			throw "Name taken";
		sendRoomName(Com::Code::rnew, name);
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to create room: ") + err.message, &Program::eventClosePopup));
	} catch (const char* err) {
		World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Program::eventHostRoomReceive(uint8* data) {
	if (*data) {
		info = (info | INF_HOST) & ~INF_GUEST_WAITING;
		eventOpenRoom();
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to host room", &Program::eventClosePopup));
}

void Program::eventJoinRoomRequest(Button* but) {
	try {
		sendRoomName(Com::Code::join, static_cast<Label*>(but)->getText());
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to join room: ") + err.message, &Program::eventClosePopup));
	}
}

void Program::eventJoinRoomReceive(uint8* data) {
	if (*data) {
		game.recvConfig(data + 1);
		eventOpenRoom();
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to join room", &Program::eventClosePopup));
}

void Program::sendRoomName(Com::Code code, const string& name) {
	vector<uint8> data(Com::dataHeadSize + 1 + name.length());
	data[0] = uint8(code);
	SDLNet_Write16(uint16(data.size()), data.data() + 1);
	data[Com::dataHeadSize] = uint8(name.length());
	std::copy(name.begin(), name.end(), data.data() + Com::dataHeadSize + 1);
	netcp->sendData(data);
}

void Program::eventStartGame(Button*) {
	try {
		game.sendStart();
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to start game: ") + err.message, &Program::eventClosePopup));
	}
}

void Program::eventExitLobby(Button*) {
	disconnect();
	eventOpenMainMenu();
}

// ROOM MENU

void Program::eventOpenRoom(Button*) {
	netcp->setCncproc(&Netcp::cprocLobby);
	setState(new ProgRoom);
}

void Program::eventOpenHostMenu(Button*) {
	info |= INF_HOST | INF_UNIQ;
	setState(new ProgRoom);
}

void Program::eventHostServer(Button*) {
	FileSys::saveConfigs(static_cast<ProgRoom*>(state.get())->confs);
	connect(false, "Waiting for player...");
}

void Program::eventSwitchConfig(Button* but) {
	setSaveConfig(static_cast<SwitchBox*>(but)->getText(), false);
	World::scene()->resetLayouts();
}

void Program::eventConfigDelete(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state.get());
	vector<string> names = sortNames(ph->confs);
	vector<string>::iterator nit = std::find(names.begin(), names.end(), curConfig);

	ph->confs.erase(curConfig);
	setSaveConfig(ph->confs.find(nit + 1 != names.end() ? *(nit + 1) : *(nit - 1))->first);
	World::scene()->resetLayouts();
}

void Program::eventConfigCopyInput(Button*) {
	World::scene()->setPopup(state->createPopupInput("Name:", &Program::eventConfigCopy));
}

void Program::eventConfigCopy(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state.get());
	if (const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText(); ph->confs.find(name) == ph->confs.end()) {
		setSaveConfig(ph->confs.emplace(name, ph->confs[curConfig]).first->first);
		World::scene()->resetLayouts();
	} else
		World::scene()->setPopup(state->createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventConfigNewInput(Button*) {
	World::scene()->setPopup(state->createPopupInput("Name:", &Program::eventConfigNew));
}

void Program::eventConfigNew(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state.get());
	if (const string& name = static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText(); ph->confs.find(name) == ph->confs.end()) {
		setSaveConfig(ph->confs.emplace(name, Com::Config()).first->first);
		World::scene()->resetLayouts();
	} else
		World::scene()->setPopup(state->createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventUpdateConfig(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = ph->confs[curConfig];
	svec2 newHome = glm::clamp(svec2(uint16(sstol(ph->wio.width->getText())), uint16(sstol(ph->wio.height->getText()))), Com::Config::minHomeSize, Com::Config::maxHomeSize);
	setConfigAmounts(cfg.tileAmounts.data(), ph->wio.tiles.data(), Com::tileMax - 1, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scaleTiles);
	setConfigAmounts(cfg.middleAmounts.data(), ph->wio.middles.data(), Com::tileMax - 1, cfg.homeSize.x, newHome.x, World::sets()->scaleTiles);
	setConfigAmounts(cfg.pieceAmounts.data(), ph->wio.pieces.data(), Com::pieceMax, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scalePieces);
	cfg.homeSize = newHome;

	cfg.survivalPass = uint8(sstol(ph->wio.survivalLE->getText()));
	cfg.survivalMode = Com::Config::Survival(ph->wio.survivalMode->getCurOpt());
	cfg.favorLimit = ph->wio.favorLimit->on;
	cfg.favorMax = uint8(sstol(ph->wio.favorMax->getText()));
	cfg.dragonDist = uint8(sstol(ph->wio.dragonDist->getText()));
	cfg.dragonSingle = ph->wio.dragonSingle->on;
	cfg.dragonDiag = ph->wio.dragonDiag->on;
	cfg.winFortress = uint16(sstol(ph->wio.winFortress->getText()));
	cfg.winThrone = uint16(sstol(ph->wio.winThrone->getText()));
	cfg.readCapturers(ph->wio.capturers->getText());
	cfg.shiftLeft = ph->wio.shiftLeft->on;
	cfg.shiftNear = ph->wio.shiftNear->on;

	cfg.checkValues();
	ph->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::eventUpdateReset(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	pr->confs[curConfig] = Com::Config();
	pr->updateConfigWidgets(pr->confs[curConfig]);
	postConfigUpdate();
}

void Program::postConfigUpdate() {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	try {
		if ((info & (INF_UNIQ | INF_GUEST_WAITING)) == INF_GUEST_WAITING)	// only send if is host on remote server with guest
			game.sendConfig();
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to send config data: ") + err.message, &Program::eventClosePopup));
	}
	FileSys::saveConfigs(pr->confs);
}

void Program::setSaveConfig(const string& name, bool save) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	curConfig = name;
	if (pr->confs[name].checkValues(); save)
		FileSys::saveConfigs(pr->confs);
}

void Program::setConfigAmounts(uint16* amts, LabelEdit** wgts, uint8 acnt, uint16 oarea, uint16 narea, bool scale) {
	for (uint8 i = 0; i < acnt; i++)
		amts[i] = scale && narea != oarea ? uint16(std::round(float(amts[i]) * float(narea) / float(oarea))) : uint16(sstoul(wgts[i]->getText()));
}

void Program::eventTileSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = pr->confs[curConfig];
	updateAmtSlider(cfg.tileAmounts.data(), pr->wio.tiles.data(), Com::pieceMax, static_cast<Slider*>(but));
	pr->updateAmtSliders(cfg.tileAmounts.data(), pr->wio.tiles.data(), Com::tileMax - 1, cfg.homeSize.y, cfg.countFreeTiles());
	pr->wio.tileFortress->setText(ProgState::tileFortressString(cfg));
}

void Program::eventMiddleSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = pr->confs[curConfig];
	updateAmtSlider(cfg.middleAmounts.data(), pr->wio.middles.data(), Com::pieceMax, static_cast<Slider*>(but));
	pr->updateAmtSliders(cfg.middleAmounts.data(), pr->wio.middles.data(), Com::tileMax - 1, 0, cfg.countFreeMiddles());
	pr->wio.middleFortress->setText(ProgState::middleFortressString(cfg));
}

void Program::eventPieceSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = pr->confs[curConfig];
	updateAmtSlider(cfg.pieceAmounts.data(), pr->wio.pieces.data(), Com::pieceMax, static_cast<Slider*>(but));
	pr->updateAmtSliders(cfg.pieceAmounts.data(), pr->wio.pieces.data(), Com::pieceMax, 0, cfg.countFreePieces());
	pr->wio.pieceTotal->setText(ProgState::pieceTotalString(cfg));
}

void Program::eventKickPlayer(Button*) {
	try {
		netcp->sendData(Com::Code::kick);
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventClosePopup));
	}
}

void Program::updateAmtSlider(uint16* amts, LabelEdit** wgts, uint8 cnt, Slider* sld) {
	LabelEdit* le = static_cast<LabelEdit*>(sld->getParent()->getWidget(sld->getID() + 1));
	amts[sizet(std::find(wgts, wgts + cnt, le) - wgts)] = uint16(sld->getVal());
	le->setText(toStr(uint16(sld->getVal())));
}

void Program::eventPlayerHello(bool onJoin) {
	static_cast<ProgRoom*>(state.get())->updateStartButton();
	game.sendConfig(onJoin);
}

void Program::eventExitRoom(Button*) {
	try {
		netcp->sendData(Com::Code::leave);
	} catch (const NetError& err) {
		disconnect();
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventOpenMainMenu));
	}
}

void Program::eventSendMessage(Button* but) {
	const string& msg = static_cast<LabelEdit*>(but)->getOldText();
	static_cast<TextBox*>(but->getParent()->getWidget(but->getID() - 1))->addLine("< " + msg);
	vector<uint8> data(Com::dataHeadSize + msg.length());
	data[0] = uint8(Com::Code::message);
	SDLNet_Write16(uint16(data.size()), data.data() + 1);
	std::copy(msg.begin(), msg.end(), data.data() + Com::dataHeadSize);
	netcp->sendData(data);
}

void Program::eventRecvMessage(uint8* data) {
	if (string msg = Com::readText(data); state->getChat())
		state->getChat()->addLine("> " + msg);
	else
		std::cout << "net message: " << msg << std::endl;
}

void Program::eventChatOpen(Button*) {
	static_cast<LabelEdit*>(World::scene()->getOverlay()->getWidget(1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
}

void Program::eventChatClose(Button*) {
	static_cast<LabelEdit*>(World::scene()->getOverlay()->getWidget(1))->cancel();
}

void Program::eventToggleChat(Button*) {
	if (state->getChat()) {
		if (World::scene()->getOverlay())
			World::scene()->getOverlay()->setOn(!World::scene()->getOverlay()->getOn());
		else
			state->toggleChatEmbedShow();
	}
}

void Program::eventHideChat(Button*) {
	state->hideChatEmbed();
}

// GAME SETUP

void Program::eventOpenSetup() {
	const Com::Config& cfg = info & INF_HOST ? static_cast<ProgRoom*>(state.get())->confs[curConfig] : game.getConfig();
#ifdef NDEBUG
	World::scene()->setObjects(game.initObjects(cfg));
#else
	World::scene()->setObjects(World::args.hasFlag(World::argSetup) ? game.initDummyObjects(cfg) : game.initObjects(cfg));
#endif
	netcp->setCncproc(&Netcp::cprocGame);
	info &= ~INF_GUEST_WAITING;

	string txt = state->getChat() ? state->getChat()->moveText() : "";
	setState(new ProgSetup);
	state->getChat()->setText(std::move(txt));

	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setStage(ProgSetup::Stage::tiles);
	ps->rcvMidBuffer.resize(game.getConfig().homeSize.x);
}

void Program::eventOpenSetup(Button*) {
	World::scene()->setObjects(game.initObjects(static_cast<ProgRoom*>(state.get())->confs[curConfig]));
	setState(new ProgSetup);
	static_cast<ProgSetup*>(state.get())->setStage(ProgSetup::Stage::tiles);
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
	tile->setInteractivity(Tile::Interact::interact);
	ps->incdecIcon(type, false, true);
}

void Program::eventPlacePieceH() {
	svec2 pos;
	if (Piece* pce; pickBob(pos, pce)) {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		if (uint8 tid = ps->getSelected(); ps->getCount(tid))
			placePiece(pos, tid, pce);
	}
}

void Program::eventPlacePieceD(Button* but) {
	svec2 pos;
	if (Piece* pce; pickBob(pos, pce))
		placePiece(pos, uint8(but->getID() - 1), pce);
}

void Program::placePiece(svec2 pos, uint8 type, Piece* occupant) {
	// remove any piece that may be occupying that tile already
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (occupant) {
		occupant->updatePos();
		ps->incdecIcon(uint8(occupant->getType()), true, false);
	}

	// find the first not placed piece of the specified type and place it if it exists
	Piece* pieces = game.getOwnPieces(Com::Piece(type));
	std::find_if(pieces, pieces + game.getConfig().pieceAmounts[type], [](Piece& it) -> bool { return !it.show; })->updatePos(pos, true);
	ps->incdecIcon(type, false, false);
}

void Program::eventMoveTile(BoardObject* obj, uint8) {
	// switch types with destination tile
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->select)) {
		Com::Tile desType = dst->getType();
		dst->setType(src->getType());
		dst->setInteractivity(Tile::Interact::interact);
		src->setType(desType);
		src->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventMovePiece(BoardObject* obj, uint8) {
	// get new position and set or swap positions with possibly already occupying piece
	svec2 pos;
	if (Piece* dst; pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->setPos(game.gtop(pos));
		World::scene()->updateSelect();
	}
}

void Program::eventClearTile() {
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->select); til && til->getType() != Com::Tile::empty) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true, true);
		til->setType(Com::Tile::empty);
		til->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventClearPiece() {
	svec2 pos;
	Piece* pce;
	if (pickBob(pos, pce); pce) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(pce->getType()), true, false);
		pce->updatePos();
	}
}

void Program::eventSetupNext(Button*) {
	try {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		if (netcp)		// only run checks in an actual game
			switch (ps->getStage()) {
			case ProgSetup::Stage::tiles:
				game.checkOwnTiles();
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
		World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to send setup: ") + err.message, &Program::eventAbortGame));
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventShowWaitPopup(Button*) {
	World::scene()->setPopup(state->createPopupMessage("Waiting for player...", &Program::eventAbortGame, "Exit"));
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
		World::scene()->setPopup(state->createPopupMessage("Name taken", &Program::eventClosePopup));
}

void Program::eventSetupSave(Button* but) {
	umap<string, Setup>& setups = static_cast<ProgSetup*>(state.get())->setups;
	Setup& stp = setups.find(static_cast<Label*>(but)->getText())->second;
	stp.clear();
	popuplateSetup(stp);
}

void Program::popuplateSetup(Setup& setup) {
	for (Tile* it = game.getTiles().own(); it != game.getTiles().end(); it++)
		if (it->getType() < Com::Tile::fortress) {
			svec2 pos = game.ptog(it->getPos());
			setup.tiles.emplace(svec2(pos.x, pos.y - game.getConfig().homeSize.y - 1), it->getType());
		}
	for (Tile* it = game.getTiles().mid(); it != game.getTiles().own(); it++)
		if (it->getType() < Com::Tile::fortress)
			setup.mids.emplace(uint16(it - game.getTiles().mid()), it->getType());
	for (Piece* it = game.getPieces().own(); it != game.getPieces().ene(); it++)
		if (game.pieceOnBoard(it)) {
			svec2 pos = game.ptog(it->getPos());
			setup.pieces.emplace(svec2(pos.x, pos.y - game.getConfig().homeSize.y - 1), it->getType());
		}
	FileSys::saveSetups(static_cast<ProgSetup*>(state.get())->setups);
	World::scene()->setPopup(nullptr);
}

void Program::eventSetupLoad(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	Setup& stp = ps->setups.find(static_cast<Label*>(but)->getText())->second;
	if (!stp.tiles.empty()) {
		for (Tile* it = game.getTiles().own(); it != game.getTiles().end(); it++)
			it->setType(Com::Tile::empty);

		vector<uint16> cnt(game.getConfig().tileAmounts.begin(), game.getConfig().tileAmounts.end() - 1);
		for (const pair<svec2, Com::Tile>& it : stp.tiles)
			if (it.first.x < game.getConfig().homeSize.x && it.first.y < game.getConfig().homeSize.y && cnt[uint8(it.second)]) {
				cnt[uint8(it.second)]--;
				game.getTile(svec2(it.first.x, it.first.y + game.getConfig().homeSize.y + 1))->setType(it.second);
			}
	}
	if (!stp.mids.empty()) {
		for (Tile* it = game.getTiles().mid(); it != game.getTiles().own(); it++)
			it->setType(Com::Tile::empty);

		vector<uint16> cnt(game.getConfig().middleAmounts.begin(), game.getConfig().middleAmounts.end());
		for (const pair<uint16, Com::Tile>& it : stp.mids)
			if (it.first < game.getConfig().homeSize.x && cnt[uint8(it.second)]) {
				cnt[uint8(it.second)]--;
				game.getTiles().mid(it.first)->setType(it.second);
			}
	}
	if (!stp.pieces.empty()) {
		for (Piece* it = game.getPieces().own(); it != game.getPieces().ene(); it++)
			it->updatePos();

		vector<uint16> cnt(game.getConfig().pieceAmounts.begin(), game.getConfig().pieceAmounts.end());
		for (const pair<svec2, Com::Piece>& it : stp.pieces)
			if (it.first.x < game.getConfig().homeSize.x && it.first.y < game.getConfig().homeSize.y && cnt[uint8(it.second)]) {
				cnt[uint8(it.second)]--;
				Piece* pieces = game.getOwnPieces(it.second);
				std::find_if(pieces, pieces + game.getConfig().pieceAmounts[uint8(it.second)], [](Piece& pce) -> bool { return !pce.show; })->updatePos(svec2(it.first.x, it.first.y + game.getConfig().homeSize.y + 1), true);
			}
	}
	ps->setStage(ProgSetup::Stage::tiles);
}

void Program::eventSetupDelete(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setups.erase(static_cast<Label*>(but)->getText());
	FileSys::saveSetups(ps->setups);
	but->getParent()->deleteWidget(but->getID());
}

void Program::eventShowConfig(Button*) {
	World::scene()->setPopup(state->createPopupConfig(game.getConfig()));
}

void Program::eventSwitchSetupButtons(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setDeleteLock(!ps->getDeleteLock());
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.prepareMatch();
	string txt = state->getChat()->moveText();
	setState(new ProgMatch);
	state->getChat()->setText(std::move(txt));
	game.prepareTurn();
	World::scene()->addAnimation(Animation(game.getScreen(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.getScreen()->getPos().x, Game::screenYDown, game.getScreen()->getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(0.f)) })));
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, quat(), Camera::latMatch) })));
	World::scene()->getCamera()->pmax = Camera::pmaxMatch;
	World::scene()->getCamera()->ymax = Camera::ymaxMatch;
}

void Program::eventEndTurn(Button*) {
	game.endTurn();
}

void Program::eventPlaceDragon(Button*) {
	svec2 pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(pos, pce);
}

void Program::eventSwitchFavor(Button*) {
	state->eventFavorize(static_cast<ProgMatch*>(state.get())->favorIconSelect() == FavorAct::off ? FavorAct::on : FavorAct::off);
}

void Program::eventSwitchFavorNow(Button*) {
	state->eventFavorize(static_cast<ProgMatch*>(state.get())->favorIconSelect() == FavorAct::now ? FavorAct::on : FavorAct::now);
}

void Program::eventFavorStart(BoardObject* obj, uint8) {
	Piece* pce = static_cast<Piece*>(obj);
	game.setFfpadPos(false, game.ptog(pce->getPos()));
	pce->getDrawTopSelf() ? game.highlightMoveTiles(pce) : game.highlightFireTiles(pce);
}

void Program::eventMove(BoardObject* obj, uint8 mBut) {
	if (game.checkFavor() == Com::Tile::fortress)
		static_cast<ProgMatch*>(state.get())->selectFavorIcon(FavorAct::off);
	game.highlightMoveTiles(nullptr);
	try {
		svec2 pos;
		if (Piece* mover = static_cast<Piece*>(obj), *pce; pickBob(pos, pce))
			game.pieceMove(mover, pos, pce, mover->getType() == Com::Piece::warhorse && mBut == SDL_BUTTON_LEFT);
	} catch (const string& err) {
		if (game.setFfpadPos(); !err.empty())
			static_cast<ProgMatch*>(state.get())->message->setText(err);
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventAbortGame));
	}
}

void Program::eventFire(BoardObject* obj, uint8) {
	if (game.checkFavor() != Com::Tile::empty)
		static_cast<ProgMatch*>(state.get())->selectFavorIcon(FavorAct::off);
	game.highlightFireTiles(nullptr);
	try {
		svec2 pos;
		if (Piece* pce; pickBob(pos, pce))
			game.pieceFire(static_cast<Piece*>(obj), pos, pce);
	} catch (const string& err) {
		if (game.setFfpadPos(); !err.empty())
			static_cast<ProgMatch*>(state.get())->message->setText(err);
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventAbortGame));
	}
}

void Program::eventAbortGame(Button*) {
	if (uninitGame(); info & INF_UNIQ) {
		disconnect();
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	} else try {
		netcp->sendData(Com::Code::leave);
	} catch (const NetError& err) {
		disconnect();
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventOpenMainMenu));
	}
}

void Program::uninitGame() {
	if (game.uninitObjects(); dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.getScreen(), std::queue<Keyframe>({ Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(1.f)), Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.getScreen()->getPos().x, Game::screenYUp, game.getScreen()->getPos().z)) })));
		World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posSetup, quat(), Camera::latSetup) })));
		for (Piece& it : game.getPieces())
			if (it.getPos().z <= game.getScreen()->getPos().z && it.getPos().z >= Com::Config::boardWidth / -2.f)
				World::scene()->addAnimation(Animation(&it, std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(it.getPos().x, -2.f, it.getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(0.f)) })));
		World::scene()->getCamera()->pmax = Camera::pmaxSetup;
		World::scene()->getCamera()->ymax = Camera::ymaxSetup;
	}
}

void Program::finishMatch(bool win) {
	if (info & INF_UNIQ)
		disconnect();
	World::scene()->setPopup(state->createPopupMessage(win ? "You win" : "You lose", &Program::eventPostFinishMatch));
}

void Program::eventPostFinishMatch(Button*) {
	if (uninitGame(); info & INF_UNIQ)
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	else try {
		if (!(info & INF_HOST))
			netcp->sendData(Com::Code::hello);
		string txt = state->getChat()->moveText();
		eventOpenRoom();
		state->getChat()->setText(std::move(txt));
		if (info & INF_GUEST_WAITING)
			eventPlayerHello(false);
	} catch (const NetError& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventAbortGame));
	}
}

void Program::eventPostDisconnectGame(Button*) {
	uninitGame();
	(info & (INF_HOST | INF_UNIQ)) == (INF_HOST | INF_UNIQ) ? eventOpenHostMenu() : eventOpenMainMenu();
}

void Program::eventHostLeft(uint8* data) {
	uninitGame();
	eventOpenLobby(data);
	World::scene()->setPopup(World::state()->createPopupMessage("Host left", &Program::eventClosePopup));
}

void Program::eventPlayerLeft() {
	uninitGame();
	info &= ~INF_GUEST_WAITING;
	eventOpenRoom();
	World::scene()->setPopup(state->createPopupMessage("Player left", &Program::eventClosePopup));
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState(new ProgSettings);
}

void Program::eventApplySettings(Button*) {
	ProgSettings* ps = static_cast<ProgSettings*>(state.get());
	World::window()->setScreen(uint8(sstoul(ps->display->getText())), Settings::Screen(ps->screen->getCurOpt()), stoiv<ivec2>(ps->winSize->getText().c_str(), strtoul), ps->currentMode());
	ps->display->setText(toStr(World::sets()->display));
	ps->screen->setCurOpt(uint8(World::sets()->screen));
	ps->winSize->setText(toStr(World::sets()->size, ProgSettings::rv2iSeparator));
	ps->dspMode->setText(ProgSettings::dispToFstr(World::sets()->mode));
	eventSaveSettings();
}

void Program::eventSetVsync(Button* but) {
	World::window()->setVsync(Settings::VSync(static_cast<SwitchBox*>(but)->getCurOpt() - 1));
	eventSaveSettings();
}

void Program::eventSetSamples(Button* but) {
	World::sets()->msamples = uint8(sstoul(static_cast<SwitchBox*>(but)->getText()));
}

void Program::eventSetTexturesScaleSL(Button* but) {
	World::sets()->texScale = uint8(static_cast<Slider*>(but)->getVal());
	World::scene()->reloadTextures();
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->texScale) + '%');
	eventSaveSettings();
}

void Program::eventSetTextureScaleLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->texScale = uint8(std::clamp(sstoul(le->getText()), 1ul, 100ul));
	World::scene()->reloadTextures();
	le->setText(toStr(World::sets()->texScale) + '%');
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(int(World::sets()->texScale));
	eventSaveSettings();
}

void Program::eventSetShadowResSL(Button* but) {
	int val = static_cast<Slider*>(but)->getVal();
	setShadowRes(val >= 0 ? uint16(std::pow(2, val)) : 0);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->shadowRes));
	eventSaveSettings();
}

void Program::eventSetShadowResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	setShadowRes(uint16(std::clamp(sstoul(le->getText()), 0ul, ulong(Settings::shadowResMax))));
	le->setText(toStr(World::sets()->shadowRes));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1);
	eventSaveSettings();
}

void Program::setShadowRes(uint16 newRes) {
	bool reload = bool(World::sets()->shadowRes) != bool(newRes);
	World::sets()->shadowRes = newRes;
	if (World::scene()->resetShadows(); reload)
		World::scene()->reloadShader();
}

void Program::eventSetSoftShadows(Button* but) {
	World::sets()->softShadows = static_cast<CheckBox*>(but)->on;
	World::scene()->reloadShader();
	eventSaveSettings();
}

void Program::eventSetGammaSL(Button* but) {
	World::window()->setGamma(float(static_cast<Slider*>(but)->getVal()) / ProgSettings::gammaStepFactor);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->gamma));
	eventSaveSettings();
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::window()->setGamma(sstof(le->getText()));
	le->setText(toStr(World::sets()->gamma));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(int(World::sets()->gamma * ProgSettings::gammaStepFactor));
	eventSaveSettings();
}

void Program::eventSetVolumeSL(Button* but) {
	World::sets()->avolume = uint8(static_cast<Slider*>(but)->getVal());
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->avolume));
	eventSaveSettings();
}

void Program::eventSetVolumeLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->avolume = uint8(std::clamp(sstoul(le->getText()), 0ul, ulong(SDL_MIX_MAXVOLUME)));
	le->setText(toStr(World::sets()->avolume));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(World::sets()->avolume);
	eventSaveSettings();
}

void Program::eventSetScaleTiles(Button* but) {
	World::sets()->scaleTiles = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetScalePieces(Button* but) {
	World::sets()->scalePieces = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetChatLineLimitSL(Button* but) {
	World::sets()->chatLines = uint16(static_cast<Slider*>(but)->getVal());
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(World::sets()->chatLines));
	eventSaveSettings();
}

void Program::eventSetChatLineLimitLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->chatLines = uint16(std::clamp(sstoul(le->getText()), 0ul, ulong(Settings::chatLinesMax)));
	le->setText(toStr(World::sets()->chatLines));
	static_cast<Slider*>(but->getParent()->getWidget(but->getID() - 1))->setVal(World::sets()->chatLines);
	eventSaveSettings();
}

void Program::eventSetFontRegular(Button* but) {
	World::window()->reloadFont(static_cast<CheckBox*>(but)->on);
	World::scene()->resetLayouts();
	eventSaveSettings();
}

void Program::eventResetSettings(Button*) {
	World::window()->resetSettings();
}

void Program::eventSaveSettings(Button*) {
	FileSys::saveSettings(World::sets());
}

void Program::eventOpenInfo(Button*) {
	setState(new ProgInfo);
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventCloseOverlay(Button*) {
	World::scene()->getOverlay()->setOn(false);
}

void Program::eventExit(Button*) {
	World::window()->close();
}

void Program::eventSBNext(Button* but) {
	static_cast<SwitchBox*>(but->getParent()->getWidget(but->getID() - 1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
}

void Program::eventSBPrev(Button* but) {
	static_cast<SwitchBox*>(but->getParent()->getWidget(but->getID() + 1))->onClick(ivec2(0), SDL_BUTTON_RIGHT);
}

void Program::eventSLUpdateLE(Button* but) {
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(static_cast<Slider*>(but)->getVal()));
}

void Program::eventPrcSliderUpdate(Button* but) {
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getID() + 1))->setText(toStr(static_cast<Slider*>(but)->getVal()) + '%');
}

void Program::connect(bool client, const char* msg) {
	try {
		netcp.reset(client ? new Netcp : new NetcpHost);
		netcp->connect();
		World::scene()->setPopup(state->createPopupMessage(msg, &Program::eventConnectCancel, "Cancel"));
	} catch (const NetError& err) {
		if (netcp)
			disconnect();
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventClosePopup));
	}
}

void Program::disconnect() {
	if (netcp) {
		netcp->disconnect();
		netcp.reset();
	}
}

void Program::setState(ProgState* newState) {
	state.reset(newState);
	World::scene()->resetLayouts();
}

BoardObject* Program::pickBob(svec2& pos, Piece*& pce) {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->select);
	pos = bob ? game.ptog(bob->getPos()) : svec2(UINT16_MAX);
	pce = dynamic_cast<Piece*>(bob);
	return bob;
}
