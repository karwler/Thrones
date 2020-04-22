#include "engine/world.h"
#include <regex>
#ifdef WEBUTILS
#include <curl/curl.h>
#endif

// PROGRAM

Program::Program() :
	info(INF_NONE),
	ftimeMode(FrameTime::none),
	proc(nullptr),
	ftimeSleep(ftimeUpdateDelay)
{}

Program::~Program() {
	if (proc)
		SDL_DetachThread(proc);
}

void Program::start() {
	World::scene()->setObjects(game.board.initObjects(Com::Config(), false, World::sets(), World::scene()));	// doesn't need to be here but I like having the game board in the background
	eventOpenMainMenu();

#ifdef EMSCRIPTEN
	if (!(World::sets()->versionLookup.first.empty() || World::sets()->versionLookup.second.empty())) {
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		std::copy_n("GET", 4, attr.requestMethod);
		attr.userData = new char[World::sets()->versionLookup.second.length()+1];
		std::copy_n(World::sets()->versionLookup.second.c_str(), World::sets()->versionLookup.second.length() + 1, static_cast<char*>(attr.userData));
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.onsuccess = fetchVersionSucceed;
		attr.onerror = fetchVersionFail;
		emscripten_fetch(&attr, World::sets()->versionLookup.first.c_str());
	}
#elif defined(WEBUTILS)
	if (!(World::sets()->versionLookup.first.empty() || World::sets()->versionLookup.second.empty()))
		proc = SDL_CreateThread(fetchVersion, "", new pairStr(World::sets()->versionLookup));
#endif
}

void Program::eventUser(const SDL_UserEvent& user) {
	switch (UserCode(user.code)) {
	case UserCode::versionFetch: {
#ifdef WEBUTILS
		int rc;
		SDL_WaitThread(proc, &rc);
		proc = nullptr;
#endif
		if (user.data1) {
			string* ver = static_cast<string*>(user.data1);
			if (latestVersion = std::move(*ver); ProgMenu* pm = dynamic_cast<ProgMenu*>(state.get()))
				pm->versionNotif->setText(latestVersion);
			delete ver;
		} else
#ifdef WEBUTILS
			std::cerr << "version fetch failed: " << curl_easy_strerror(CURLcode(rc)) << std::endl;
#else
			std::cerr << "version fetch failed" << std::endl;
#endif
		break; }
	default:
		std::cerr << "unknown user event code: " << user.code << std::endl;
	}
}

void Program::tick(float dSec) {
	if (netcp)
		netcp->tick();

	if (ftimeMode != FrameTime::none)
		if (ftimeSleep -= dSec; ftimeSleep <= 0.f) {
			ftimeSleep = ftimeUpdateDelay;
			ProgState::Text txt = state->makeFpsText(dSec);
			Overlay* box = static_cast<Overlay*>(state->getFpsText()->getParent());
			box->relSize = txt.length;
			box->onResize();
			state->getFpsText()->setText(std::move(txt.text));
		}
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
	World::sets()->address = Settings::defaultAddress;
	static_cast<LabelEdit*>(but)->setText(World::sets()->address);
	eventSaveSettings();
}

void Program::eventUpdatePort(Button* but) {
	World::sets()->port = static_cast<LabelEdit*>(but)->getText();
	eventSaveSettings();
}

void Program::eventResetPort(Button* but) {
	World::sets()->port = Com::defaultPort;
	static_cast<LabelEdit*>(but)->setText(World::sets()->port);
	eventSaveSettings();
}

// LOBBY MENU

void Program::eventOpenLobby(const uint8* data) {
	vector<pair<string, bool>> rooms(*data++);
	for (auto& [name, full] : rooms) {
		full = *data++;
		name = Com::readName(data);
		data += name.length() + 1;
	}
	info &= ~INF_HOST;
	netcp->setTickproc(&Netcp::tickLobby);
	setState(new ProgLobby(std::move(rooms)));
}

void Program::eventHostRoomInput(Button*) {
	World::scene()->setPopup(state->createPopupInput("Name:", &Program::eventHostRoomRequest, Com::roomNameLimit));
}

void Program::eventHostRoomRequest(Button*) {
	try {
#ifdef EMSCRIPTEN
		if (!FileSys::canRead())
			throw "Waiting for files to sync";
#endif
		sendRoomName(Com::Code::rnew, static_cast<LabelEdit*>(World::scene()->getPopup()->getWidget(1))->getText());
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to create room: ") + err.message, &Program::eventClosePopup));
	} catch (const char* err) {
		World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Program::eventHostRoomReceive(const uint8* data) {
	string err = Com::readText(data);
	if (err.empty()) {
		info = (info | INF_HOST) & ~INF_GUEST_WAITING;
		setState(new ProgRoom(FileSys::loadConfigs()));
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to host room: " + err, &Program::eventClosePopup));
}

void Program::eventJoinRoomRequest(Button* but) {
	try {
		sendRoomName(Com::Code::join, static_cast<Label*>(but)->getText());
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to join room: ") + err.message, &Program::eventClosePopup));
	}
}

void Program::eventJoinRoomReceive(const uint8* data) {
	if (*data) {
		game.recvConfig(data + 1);
		setState(new ProgRoom);
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to join room", &Program::eventClosePopup));
}

void Program::sendRoomName(Com::Code code, const string& name) {
	vector<uint8> data(Com::dataHeadSize + 1 + name.length());
	data[0] = uint8(code);
	Com::write16(data.data() + 1, uint16(data.size()));
	data[Com::dataHeadSize] = uint8(name.length());
	std::copy(name.begin(), name.end(), data.data() + Com::dataHeadSize + 1);
	netcp->sendData(data);
}

void Program::eventStartGame(Button*) {
	try {
		game.sendStart();
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to start game: ") + err.message, &Program::eventClosePopup));
	}
}

void Program::eventExitLobby(Button*) {
	disconnect();
	eventOpenMainMenu();
}

// ROOM MENU

void Program::eventOpenHostMenu(Button*) {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead()) {
		World::scene()->setPopup(state->createPopupMessage("Waiting for files to sync", &Program::eventClosePopup));
		return;
	}
#endif
	info |= INF_HOST | INF_UNIQ;
	setState(new ProgRoom(FileSys::loadConfigs()));
}

void Program::eventHostServer(Button*) {
	FileSys::saveConfigs(static_cast<ProgRoom*>(state.get())->confs);
	connect(false, "Waiting for player...");
}

void Program::eventSwitchConfig(sizet, const string& str) {
	setSaveConfig(str, false);
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
	setConfigAmounts(cfg.tileAmounts.data(), ph->wio.tiles.data(), Com::tileLim, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scaleTiles);
	setConfigAmounts(cfg.middleAmounts.data(), ph->wio.middles.data(), Com::tileLim, cfg.homeSize.x, newHome.x, World::sets()->scaleTiles);
	setConfigAmounts(cfg.pieceAmounts.data(), ph->wio.pieces.data(), Com::pieceMax, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scalePieces);
	cfg.homeSize = newHome;
	//cfg.gameType = Com::Config::GameType(ph->wio.gameType->getCurOpt());
	cfg.ports = ph->wio.ports->on;
	cfg.rowBalancing = ph->wio.rowBalancing->on;
	cfg.setPieceOn = ph->wio.setPieceOn->on;
	cfg.setPieceNum = uint16(sstol(ph->wio.setPieceNum->getText()));
	cfg.battlePass = uint8(sstol(ph->wio.battleLE->getText()));
	cfg.favorTotal = ph->wio.favorTotal->on;
	cfg.favorLimit = uint8(sstol(ph->wio.favorLimit->getText()));
	cfg.dragonLate = ph->wio.dragonLate->on;
	cfg.dragonStraight = ph->wio.dragonStraight->on;
	cfg.winFortress = uint16(sstol(ph->wio.winFortress->getText()));
	cfg.winThrone = uint16(sstol(ph->wio.winThrone->getText()));
	cfg.readCapturers(ph->wio.capturers->getText());
	cfg.checkValues();
	ph->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::eventUpdateConfigC(sizet, const string&) {
	eventUpdateConfig();
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
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to send configuration data: ") + err.message, &Program::eventClosePopup));
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
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.tileAmounts.data(), pr->wio.tiles.data(), pr->wio.tileFortress, Com::tileLim, cfg.homeSize.y, &Com::Config::countFreeTiles, ProgState::tileFortressString);
}

void Program::eventMiddleSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = pr->confs[curConfig];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.middleAmounts.data(), pr->wio.middles.data(), pr->wio.middleFortress, Com::tileLim, 0, &Com::Config::countFreeMiddles, ProgState::middleFortressString);
}

void Program::eventPieceSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Com::Config& cfg = pr->confs[curConfig];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.pieceAmounts.data(), pr->wio.pieces.data(), pr->wio.pieceTotal, Com::pieceMax, 0, &Com::Config::countFreePieces, ProgState::pieceTotalString);
}

void Program::updateAmtSliders(Slider* sl, Com::Config& cfg, uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Com::Config::*counter)() const, string (*totstr)(const Com::Config&)) {
	LabelEdit* le = static_cast<LabelEdit*>(sl->getParent()->getWidget(sl->getIndex() + 1));
	amts[sizet(std::find(wgts, wgts + cnt, le) - wgts)] = uint16(sl->getVal());
	le->setText(toStr(uint16(sl->getVal())));
	ProgRoom::updateAmtSliders(amts, wgts, cnt, min, (cfg.*counter)());
	total->setText(totstr(cfg));
}

void Program::eventKickPlayer(Button*) {
	try {
		netcp->sendData(Com::Code::kick);
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventClosePopup));
	}
}

void Program::eventPlayerHello(bool onJoin) {
	static_cast<ProgRoom*>(state.get())->updateStartButton();
	game.sendConfig(onJoin);
}

void Program::eventExitRoom(Button*) {
	try {
		netcp->sendData(Com::Code::leave);
	} catch (const Com::Error& err) {
		netcp.reset();
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventOpenMainMenu));
	}
}

void Program::eventSendMessage(Button* but) {
	const string& msg = static_cast<LabelEdit*>(but)->getOldText();
	static_cast<TextBox*>(but->getParent()->getWidget(but->getIndex() - 1))->addLine("> " + msg);
	vector<uint8> data(Com::dataHeadSize + msg.length());
	data[0] = uint8(Com::Code::message);
	Com::write16(data.data() + 1, uint16(data.size()));
	std::copy(msg.begin(), msg.end(), data.data() + Com::dataHeadSize);
	netcp->sendData(data);
}

void Program::eventRecvMessage(const uint8* data) {
	if (string msg = Com::readText(data); state->getChat()) {
		state->getChat()->addLine("< " + msg);
		if (Overlay* lay = dynamic_cast<Overlay*>(state->getChat()->getParent())) {
			if (!lay->getShow())
				state->getNotification()->setShow(true);
		} else if (!state->getChat()->getParent()->relSize.pix)
			state->getNotification()->setShow(true);
	} else
		std::cout << "net message: " << msg << std::endl;
}

void Program::eventChatOpen(Button*) {
	static_cast<LabelEdit*>(state->getChat()->getParent()->getWidget(state->getChat()->getIndex() + 1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
}

void Program::eventChatClose(Button*) {
	static_cast<LabelEdit*>(state->getChat()->getParent()->getWidget(state->getChat()->getIndex() + 1))->cancel();
}

void Program::eventToggleChat(Button*) {
	if (state->getChat()) {
		if (Overlay* lay = dynamic_cast<Overlay*>(state->getChat()->getParent())) {
			lay->setShow(!lay->getShow());
			state->getNotification()->setShow(false);
		} else
			state->toggleChatEmbedShow();
	}
}

void Program::eventCloseChat(Button*) {
	static_cast<Overlay*>(state->getChat()->getParent())->setShow(false);
}

void Program::eventHideChat(Button*) {
	state->hideChatEmbed();
}

void Program::eventFocusChatLabel(Button* but) {
	static_cast<Label*>(but->getParent()->getWidget(but->getIndex() + 1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
}

// GAME SETUP

void Program::eventOpenSetup() {
	const Com::Config& cfg = info & INF_HOST ? static_cast<ProgRoom*>(state.get())->confs[curConfig] : game.board.config;
#ifdef NDEBUG
	World::scene()->setObjects(game.board.initObjects(cfg, true, World::sets(), World::scene()));
#else
	World::scene()->setObjects(World::args.hasFlag(Settings::argSetup) ? game.board.initDummyObjects(cfg, World::sets(), World::scene()) : game.board.initObjects(cfg, true, World::sets(), World::scene()));
#endif
	netcp->setTickproc(&Netcp::tickGame);
	info &= ~INF_GUEST_WAITING;

	string txt = state->getChat() ? state->getChat()->moveText() : string();
	setState(new ProgSetup);
	state->getChat()->setText(std::move(txt));

	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setStage(ProgSetup::Stage::tiles);
	ps->rcvMidBuffer.resize(game.board.config.homeSize.x);
	ps->piecePicksLeft = 0;
	if (game.board.config.setPieceOn && game.board.config.setPieceNum < game.board.config.countPieces()) {
		game.board.ownPieceAmts.fill(0);
		ps->piecePicksLeft = game.board.config.setPieceNum;
		if (game.board.config.winThrone) {
			game.board.ownPieceAmts[uint8(Com::Piece::throne)] += game.board.config.winThrone;
			ps->piecePicksLeft -= game.board.config.winThrone;
		} else {
			uint16 caps = game.board.config.winFortress;
			for (uint8 i = uint(Com::Piece::throne); i < Com::pieceMax && caps; i--)
				if (game.board.config.capturers[i]) {
					uint16 diff = std::min(caps, uint16(game.board.config.pieceAmounts[i] - game.board.ownPieceAmts[i]));
					game.board.ownPieceAmts[i] += diff;
					ps->piecePicksLeft -= diff;
					caps -= diff;
				}
		}
	} else
		game.board.ownPieceAmts = game.board.config.pieceAmounts;

	if (ps->piecePicksLeft)
		World::scene()->setPopup(ps->createPopupPiecePicker());
	else
		game.board.initOwnPieces();
}

void Program::eventOpenSetup(Button*) {
	World::scene()->setObjects(game.board.initObjects(static_cast<ProgRoom*>(state.get())->confs[curConfig], false, World::sets(), World::scene()));
	game.board.ownPieceAmts = game.board.config.pieceAmounts;
	game.board.initOwnPieces();
	setState(new ProgSetup);
	static_cast<ProgSetup*>(state.get())->setStage(ProgSetup::Stage::tiles);
}

void Program::eventIconSelect(Button* but) {
	static_cast<ProgSetup*>(state.get())->setSelected(uint8(but->getIndex() - 1));
}

void Program::eventPlaceTile() {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = ps->getSelected();
	if (Tile* tile = dynamic_cast<Tile*>(World::scene()->getSelect()); ps->getCount(type) && tile) {
		if (tile->getType() < Com::Tile::fortress)
			ps->incdecIcon(uint8(tile->getType()), true);

		tile->setType(Com::Tile(type));
		tile->setInteractivity(Tile::Interact::interact);
		ps->incdecIcon(type, false);
	}
}

void Program::eventPlacePiece() {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = ps->getSelected();
	svec2 pos;
	if (Piece* occupant; ps->getCount(type) && pickBob(pos, occupant)) {
		if (occupant) {
			occupant->updatePos();
			ps->incdecIcon(uint8(occupant->getType()), true);
		}

		Piece* pieces = game.board.getOwnPieces(Com::Piece(type));
		std::find_if(pieces, pieces + game.board.ownPieceAmts[type], [](Piece& it) -> bool { return !it.show; })->updatePos(pos, true);
		ps->incdecIcon(type, false);
	}
}

void Program::eventMoveTile(BoardObject* obj, uint8) {
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->getSelect())) {
		Com::Tile desType = dst->getType();
		dst->setType(src->getType());
		dst->setInteractivity(Tile::Interact::interact);
		src->setType(desType);
		src->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventMovePiece(BoardObject* obj, uint8) {
	svec2 pos;
	if (Piece* dst; pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->updatePos(pos);
	}
}

void Program::eventClearTile() {
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->getSelect()); til && til->getType() != Com::Tile::empty) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true);
		til->setType(Com::Tile::empty);
		til->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventClearPiece() {
	svec2 pos;
	Piece* pce;
	if (pickBob(pos, pce); pce) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(pce->getType()), true);
		pce->updatePos();
	}
}

void Program::eventSetupNext(Button*) {
	try {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		if (netcp)		// only run checks in an actual game
			switch (ps->getStage()) {
			case ProgSetup::Stage::tiles:
				game.board.checkOwnTiles();
				break;
			case ProgSetup::Stage::middles:
				game.board.checkMidTiles();
				break;
			case ProgSetup::Stage::pieces:
				game.board.checkOwnPieces();
			}
		switch (ps->setStage(ps->getStage() + 1); ps->getStage()) {
		case ProgSetup::Stage::preparation:
			game.finishSetup();
			if (game.availableFF)
				World::scene()->setPopup(ps->createPopupFavorPick(game.availableFF));
			else
				eventSetupNext();
			break;
		case ProgSetup::Stage::ready:
			game.sendSetup();
			if (ps->enemyReady)
				eventOpenMatch();
			else
				ps->message->setText("Waiting for player...");
		}
	} catch (const string& err) {
		World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(string("Failed to send setup: ") + err.message, &Program::eventAbortGame));
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.board.takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventOpenSetupSave(Button*) {
	World::scene()->setPopup(static_cast<ProgSetup*>(state.get())->createPopupSaveLoad(true));
}

void Program::eventOpenSetupLoad(Button*) {
	World::scene()->setPopup(static_cast<ProgSetup*>(state.get())->createPopupSaveLoad(false));
}

void Program::eventSetupPickPiece(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = uint8(but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex());
	if (game.board.ownPieceAmts[type]++; --ps->piecePicksLeft) {
		static_cast<Label*>(but->getParent()->getParent()->getParent()->getParent()->getWidget(0))->setText(ProgSetup::msgPickPiece + toStr(ps->piecePicksLeft) + ')');
		if (game.board.config.pieceAmounts[type] == game.board.ownPieceAmts[type]) {
			Icon* ico = static_cast<Icon*>(but);
			ico->setDim(ProgState::defaultDim);
			ico->lcall = nullptr;
		}
	} else {
		game.board.initOwnPieces();
		eventClosePopup();
	}
}

void Program::eventSetupNew(Button* but) {
	const string& name = static_cast<LabelEdit*>(dynamic_cast<LabelEdit*>(but) ? but : but->getParent()->getWidget(but->getIndex() - 1))->getText();
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
	for (Tile* it = game.board.getTiles().own(); it != game.board.getTiles().end(); it++)
		if (it->getType() < Com::Tile::fortress) {
			svec2 pos = game.board.ptog(it->getPos());
			setup.tiles.emplace_back(svec2(pos.x, pos.y - game.board.config.homeSize.y - 1), it->getType());
		}
	for (Tile* it = game.board.getTiles().mid(); it != game.board.getTiles().own(); it++)
		if (it->getType() < Com::Tile::fortress)
			setup.mids.emplace_back(uint16(it - game.board.getTiles().mid()), it->getType());
	for (Piece* it = game.board.getPieces().own(); it != game.board.getPieces().ene(); it++)
		if (game.board.pieceOnBoard(it)) {
			svec2 pos = game.board.ptog(it->getPos());
			setup.pieces.emplace_back(svec2(pos.x, pos.y - game.board.config.homeSize.y - 1), it->getType());
		}
	FileSys::saveSetups(static_cast<ProgSetup*>(state.get())->setups);
	World::scene()->setPopup(nullptr);
}

void Program::eventSetupLoad(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	Setup& stp = ps->setups.find(static_cast<Label*>(but)->getText())->second;
	if (!stp.tiles.empty()) {
		for (Tile* it = game.board.getTiles().own(); it != game.board.getTiles().end(); it++)
			it->setType(Com::Tile::empty);

		array<uint16, Com::tileLim> cnt = game.board.config.tileAmounts;
		for (auto& [pos, type] : stp.tiles)
			if (pos.x < game.board.config.homeSize.x && pos.y < game.board.config.homeSize.y && cnt[uint8(type)]) {
				cnt[uint8(type)]--;
				game.board.getTile(svec2(pos.x, pos.y + game.board.config.homeSize.y + 1))->setType(type);
			}
	}
	if (!stp.mids.empty()) {
		for (Tile* it = game.board.getTiles().mid(); it != game.board.getTiles().own(); it++)
			it->setType(Com::Tile::empty);

		array<uint16, Com::tileLim> cnt = game.board.config.middleAmounts;
		for (auto& [pos, type] : stp.mids)
			if (pos < game.board.config.homeSize.x && cnt[uint8(type)]) {
				cnt[uint8(type)]--;
				game.board.getTiles().mid(pos)->setType(type);
			}
	}
	if (!stp.pieces.empty()) {
		for (Piece* it = game.board.getPieces().own(); it != game.board.getPieces().ene(); it++)
			it->updatePos();

		array<uint16, Com::pieceMax> cnt = game.board.ownPieceAmts;
		for (auto& [pos, type] : stp.pieces)
			if (pos.x < game.board.config.homeSize.x && pos.y < game.board.config.homeSize.y && cnt[uint8(type)]) {
				cnt[uint8(type)]--;
				Piece* pieces = game.board.getOwnPieces(type);
				std::find_if(pieces, pieces + game.board.ownPieceAmts[uint8(type)], [](Piece& pce) -> bool { return !pce.show; })->updatePos(svec2(pos.x, pos.y + game.board.config.homeSize.y + 1), true);
			}
	}
	ps->setStage(ProgSetup::Stage::tiles);
}

void Program::eventSetupDelete(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setups.erase(static_cast<Label*>(but)->getText());
	FileSys::saveSetups(ps->setups);
	but->getParent()->deleteWidget(but->getIndex());
}

void Program::eventShowConfig(Button*) {
	World::scene()->setPopup(state->createPopupConfig(game.board.config, static_cast<ProgGame*>(state.get())->configList));
}

void Program::eventCloseConfigList(Button*) {
	static_cast<ProgGame*>(state.get())->configList = nullptr;
	eventClosePopup();
}

void Program::eventSwitchGameButtons(Button*) {
	ProgGame* pg = static_cast<ProgGame*>(state.get());
	pg->bswapIcon->selected = !pg->bswapIcon->selected;
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.board.prepareMatch(game.getMyTurn(), static_cast<ProgSetup*>(state.get())->rcvMidBuffer.data());
	string txt = state->getChat()->moveText();
	setState(new ProgMatch);
	state->getChat()->setText(std::move(txt));
	game.prepareTurn(false);
	World::scene()->addAnimation(Animation(game.board.getScreen(), std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.board.getScreen()->getPos().x, Board::screenYDown, game.board.getScreen()->getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(0.f)) })));
	World::scene()->addAnimation(Animation(&World::scene()->camera, std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, quat(), Camera::latMatch) })));
	World::scene()->camera.pmax = Camera::pmaxMatch;
	World::scene()->camera.ymax = Camera::ymaxMatch;
}

void Program::eventEndTurn(Button*) {
	game.finishFavor(Favor::none, static_cast<ProgMatch*>(state.get())->favorIconSelect());	// in case assault FF has been used
	game.endTurn();
}

void Program::eventPickFavor(Button* but) {
	uint8 fid = uint8(but->getIndex() - 1);
	if (game.favorsCount[fid]++; game.board.config.favorTotal)
		game.favorsLeft[fid]--;

	if (--game.availableFF) {
		if (static_cast<Label*>(but->getParent()->getParent()->getWidget(0))->setText(ProgMatch::msgFavorPick + string(" (") + toStr(game.availableFF) + ')'); !game.favorsLeft[fid]) {
			but->setDim(ProgState::defaultDim);
			but->lcall = nullptr;
		}
	} else if (dynamic_cast<ProgMatch*>(state.get()))
		eventClosePopup();
	else
		eventSetupNext();
}

void Program::eventSelectFavor(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	pm->eventNumpress(uint8(std::find(pm->favorIcons.begin(), pm->favorIcons.end(), but) - pm->favorIcons.begin()));
}

void Program::eventSwitchDestroy(Button*) {
	state->eventDestroy(Switch::toggle);
}

void Program::eventKillDestroy(Button*) {
	game.changeTile(game.board.getTile(game.board.ptog(game.board.getPxpad()->getPos())), Com::Tile::plains);
	eventCancelDestroy();
}

void Program::eventCancelDestroy(Button*) {
	game.board.setPxpadPos(nullptr);
	eventClosePopup();
}

void Program::eventClickPlaceDragon(Button* but) {
	static_cast<Icon*>(but)->selected = true;
	Tile* til = game.board.getTiles().own();
	for (; til != game.board.getTiles().end() && til->getType() != Com::Tile::fortress; til++);
	Piece* pce = game.board.findOccupant(til);
	World::scene()->updateSelect(pce ? static_cast<BoardObject*>(pce) : static_cast<BoardObject*>(til));
	getUnplacedDragon()->onHold(ivec2(0), SDL_BUTTON_LEFT);	// like startKeyDrag but with a different position
	World::state()->objectDragPos = vec2(til->getPos().x, til->getPos().z);
}

void Program::eventHoldPlaceDragon(Button* but) {
	static_cast<Icon*>(but)->selected = true;
	Piece* pce = getUnplacedDragon();
	World::scene()->delegateStamp(pce);
	pce->startKeyDrag(SDL_BUTTON_LEFT);
}

void Program::eventPlaceDragon(BoardObject* obj, uint8) {
	svec2 pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(static_cast<Piece*>(obj), pos, pce);
}

Piece* Program::getUnplacedDragon() {
	Piece* pce = game.board.getOwnPieces(Com::Piece::dragon);
	for (; pce->getType() == Com::Piece::dragon && pce->show; pce++);
	pce->hgcall = nullptr;
	pce->ulcall = pce->urcall = &Program::eventPlaceDragon;
	return pce;
}

void Program::eventEstablishB(Button*) {
	switch (std::count_if(game.board.getOwnPieces(Com::Piece::throne), game.board.getPieces().ene(), [](Piece& it) -> bool { return it.show; })) {
	case 0:
		World::scene()->setPopup(state->createPopupMessage("No " + string(Com::pieceNames[uint8(Com::Piece::throne)]) + " for establishing", &Program::eventClosePopup));
		break;
	case 1:
		try {
			game.establishTile(std::find_if(game.board.getOwnPieces(Com::Piece::throne), game.board.getPieces().ene(), [](Piece& it) -> bool { return it.show; }), false);
		} catch (const string& err) {
			World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
		}
		break;
	default:
		game.board.selectEstablishable();
	}
}

void Program::eventEstablishP(BoardObject* obj, uint8) {
	try {
		game.establishTile(static_cast<Piece*>(obj), true);
	} catch (const string& err) {
		World::scene()->setPopup(state->createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Program::eventRebuildTileB(Button*) {
	switch (std::count_if(game.board.getOwnPieces(Com::Piece::throne), game.board.getPieces().ene(), [this](Piece& it) -> bool { return game.board.tileRebuildable(&it); })) {
	case 0:
		World::scene()->setPopup(state->createPopupMessage("No " + string(Com::pieceNames[uint8(Com::Piece::throne)]) + " for rebuilding", &Program::eventClosePopup));
		break;
	case 1:
		game.rebuildTile(std::find_if(game.board.getOwnPieces(Com::Piece::throne), game.board.getPieces().ene(), [this](Piece& it) -> bool { return game.board.tileRebuildable(&it); }), false);
		break;
	default:
		game.board.selectRebuildable();
	}
}

void Program::eventRebuildTileP(BoardObject* obj, uint8) {
	game.rebuildTile(static_cast<Piece*>(obj), true);
}

void Program::eventOpenSpawner(Button*) {
	World::scene()->setPopup(static_cast<ProgMatch*>(state.get())->createPopupSpawner());
}

void Program::eventSpawnPieceB(Button* but) {
	Com::Piece type = Com::Piece(but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex());
	uint forts = uint(std::count_if(game.board.getTiles().own(), game.board.getTiles().end(), [this](Tile& it) -> bool { return it.isUnbreachedFortress() && !game.board.findOccupant(&it); }));
	if (type == Com::Piece::lancer || type == Com::Piece::rangers || type == Com::Piece::spearmen || type == Com::Piece::catapult || type == Com::Piece::elephant || forts == 1)
		game.spawnPiece(type, nullptr, false);
	else {
		static_cast<ProgMatch*>(state.get())->spawning = type;
		game.board.selectSpawners();
	}
}

void Program::eventSpawnPieceT(BoardObject* obj, uint8) {
	game.spawnPiece(static_cast<ProgMatch*>(state.get())->spawning, static_cast<Tile*>(obj), true);
}

void Program::eventPieceStart(BoardObject* obj, uint8 mBut) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	Piece* pce = static_cast<Piece*>(obj);
	game.board.setPxpadPos(pm->destroyIcon->selected ? pce : nullptr);
	mBut == SDL_BUTTON_LEFT ? game.board.highlightMoveTiles(pce, game.getEneRec(), pm->favorIconSelect()) : game.board.highlightEngageTiles(pce);
}

void Program::eventMove(BoardObject* obj, uint8) {
	game.board.highlightMoveTiles(nullptr, game.getEneRec(), static_cast<ProgMatch*>(state.get())->favorIconSelect());
	try {
		svec2 pos;
		if (Piece* mover = static_cast<Piece*>(obj), *pce; pickBob(pos, pce))
			game.pieceMove(mover, pos, pce, true);
	} catch (const string& err) {
		if (game.board.setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state.get())->message->setText(err);
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventAbortGame));
	}
}

void Program::eventEngage(BoardObject* obj, uint8) {
	game.board.highlightEngageTiles(nullptr);
	try {
		svec2 pos;
		if (Piece* pce; pickBob(pos, pce)) {
			if (Piece* actor = static_cast<Piece*>(obj); actor->firingArea().first)
				game.pieceFire(actor, pos, pce);
			else
				game.pieceMove(actor, pos, pce, false);
		}
	} catch (const string& err) {
		if (game.board.setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state.get())->message->setText(err);
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventAbortGame));
	}
}

void Program::eventPieceNoEngage(BoardObject* obj, uint8) {
	try {
		game.setNoEngage(static_cast<Piece*>(obj));
		static_cast<ProgMatch*>(state.get())->updateTurnIcon(true);
	} catch (const string& err) {
		static_cast<ProgGame*>(state.get())->message->setText(err);
	}
}

void Program::eventAbortGame(Button*) {
	uninitGame();
	if (info & INF_UNIQ) {
		disconnect();
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	} else
		eventExitRoom();
}

void Program::eventSurrender(Button*) {
	game.surrender();
}

void Program::uninitGame() {
	game.board.uninitObjects();
	if (dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.board.getScreen(), std::queue<Keyframe>({ Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(1.f)), Keyframe(0.5f, Keyframe::CHG_POS, vec3(game.board.getScreen()->getPos().x, Board::screenYUp, game.board.getScreen()->getPos().z)) })));
		World::scene()->addAnimation(Animation(&World::scene()->camera, std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posSetup, quat(), Camera::latSetup) })));
		for (Piece& it : game.board.getPieces())
			if (it.getPos().z <= game.board.getScreen()->getPos().z && it.getPos().z >= Com::Config::boardWidth / -2.f)
				World::scene()->addAnimation(Animation(&it, std::queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, vec3(it.getPos().x, -2.f, it.getPos().z)), Keyframe(0.f, Keyframe::CHG_SCL, vec3(), quat(), vec3(0.f)) })));
		World::scene()->camera.pmax = Camera::pmaxSetup;
		World::scene()->camera.ymax = Camera::ymaxSetup;
	}
}

void Program::finishMatch(bool win) {
	if (info & INF_UNIQ)
		disconnect();
	World::scene()->setPopup(state->createPopupMessage(win ? "You win" : "You lose", &Program::eventPostFinishMatch));
}

void Program::eventPostFinishMatch(Button*) {
	uninitGame();
	if (info & INF_UNIQ)
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	else try {
		string txt = state->getChat()->moveText();
		netcp->setTickproc(&Netcp::tickLobby);
		if (info & INF_HOST) {
#ifdef EMSCRIPTEN
			if (!FileSys::canRead()) {
				eventExitRoom();
				return;
			}
#endif
			setState(new ProgRoom(FileSys::loadConfigs()));
		} else {
			netcp->sendData(Com::Code::hello);
			setState(new ProgRoom);
		}
		state->getChat()->setText(std::move(txt));
		if (info & INF_GUEST_WAITING)
			eventPlayerHello(false);
	} catch (const Com::Error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventExitRoom));
	}
}

void Program::eventPostDisconnectGame(Button*) {
	uninitGame();
	(info & (INF_HOST | INF_UNIQ)) == (INF_HOST | INF_UNIQ) ? eventOpenHostMenu() : eventOpenMainMenu();
}

void Program::eventHostLeft(const uint8* data) {
	uninitGame();
	eventOpenLobby(data);
	World::scene()->setPopup(World::state()->createPopupMessage("Host left", &Program::eventClosePopup));
}

void Program::eventPlayerLeft() {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead()) {
		World::scene()->setPopup(state->createPopupMessage("Player left", &Program::eventAbortGame));
		return;
	}
#endif
	uninitGame();
	info &= ~INF_GUEST_WAITING;
	netcp->setTickproc(&Netcp::tickLobby);
	setState(new ProgRoom(FileSys::loadConfigs()));
	World::scene()->setPopup(state->createPopupMessage("Player left", &Program::eventClosePopup));
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState(new ProgSettings);
}

void Program::eventSetDisplay(Button* but) {
	World::sets()->display = uint8(sstoul(static_cast<LabelEdit*>(but)->getText()));
	World::window()->setScreen();
	eventSaveSettings();
}

void Program::eventSetScreen(sizet id, const string&) {
	World::sets()->screen = Settings::Screen(id);
	World::window()->setScreen();
	eventSaveSettings();
}

void Program::eventSetWindowSize(sizet, const string& str) {
	World::sets()->size = stoiv<ivec2>(str.c_str(), strtoul);
	World::window()->setScreen();
	eventSaveSettings();
}

void Program::eventSetWindowMode(sizet, const string& str) {
	World::sets()->mode = static_cast<ProgSettings*>(state.get())->fstrToDisp(str);
	World::window()->setScreen();
	eventSaveSettings();
}

void Program::eventSetVsync(sizet id, const string&) {
	World::window()->setVsync(Settings::VSync(id - 1));
	eventSaveSettings();
}

void Program::eventSetSamples(sizet, const string& str) {
	World::sets()->msamples = uint8(sstoul(str));
	eventSaveSettings();
}

void Program::eventSetTexturesScaleSL(Button* but) {
	World::sets()->texScale = uint8(static_cast<Slider*>(but)->getVal());
	World::scene()->reloadTextures();
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getIndex() + 1))->setText(toStr(World::sets()->texScale) + '%');
	eventSaveSettings();
}

void Program::eventSetTextureScaleLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->texScale = uint8(std::clamp(sstoul(le->getText()), 1ul, 100ul));
	World::scene()->reloadTextures();
	le->setText(toStr(World::sets()->texScale) + '%');
	static_cast<Slider*>(but->getParent()->getWidget(but->getIndex() - 1))->setVal(int(World::sets()->texScale));
	eventSaveSettings();
}

void Program::eventSetShadowResSL(Button* but) {
	int val = static_cast<Slider*>(but)->getVal();
	setShadowRes(val >= 0 ? uint16(std::pow(2, val)) : 0);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getIndex() + 1))->setText(toStr(World::sets()->shadowRes));
	eventSaveSettings();
}

void Program::eventSetShadowResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	setShadowRes(uint16(std::min(sstoul(le->getText()), ulong(Settings::shadowResMax))));
	le->setText(toStr(World::sets()->shadowRes));
	static_cast<Slider*>(but->getParent()->getWidget(but->getIndex() - 1))->setVal(World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1);
	eventSaveSettings();
}

void Program::setShadowRes(uint16 newRes) {
	bool reload = bool(World::sets()->shadowRes) != bool(newRes);
	World::sets()->shadowRes = newRes;
	if (World::scene()->resetShadows(); reload)
		World::scene()->reloadShader();
	eventSaveSettings();
}

void Program::eventSetSoftShadows(Button* but) {
	World::sets()->softShadows = static_cast<CheckBox*>(but)->on;
	World::scene()->reloadShader();
	eventSaveSettings();
}

void Program::eventSetGammaSL(Button* but) {
	World::window()->setGamma(float(static_cast<Slider*>(but)->getVal()) / ProgSettings::gammaStepFactor);
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getIndex() + 1))->setText(toStr(World::sets()->gamma));
	eventSaveSettings();
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::window()->setGamma(sstof(le->getText()));
	le->setText(toStr(World::sets()->gamma));
	static_cast<Slider*>(but->getParent()->getWidget(but->getIndex() - 1))->setVal(int(World::sets()->gamma * ProgSettings::gammaStepFactor));
	eventSaveSettings();
}

void Program::eventSetVolumeSL(Button* but) {
	setStandardSlider(static_cast<Slider*>(but), World::sets()->avolume);
}

void Program::eventSetVolumeLE(Button* but) {
	setStandardSlider(static_cast<LabelEdit*>(but), World::sets()->avolume);
}

void Program::eventSetColorAlly(sizet id, const string&) {
	World::sets()->colorAlly = Settings::Color(id);
	setColorPieces(World::sets()->colorAlly, game.board.getPieces().own(), game.board.getPieces().ene());
}

void Program::eventSetColorEnemy(sizet id, const string&) {
	World::sets()->colorEnemy = Settings::Color(id);
	setColorPieces(World::sets()->colorEnemy, game.board.getPieces().ene(), game.board.getPieces().end());
}

void Program::setColorPieces(Settings::Color clr, Piece* pos, Piece* end) {
	for (; pos != end; pos++)
		pos->matl = World::scene()->material(Settings::colorNames[uint8(clr)]);
	eventSaveSettings();
}

void Program::eventSetScaleTiles(Button* but) {
	World::sets()->scaleTiles = static_cast<CheckBox*>(but)->on;
	eventSaveSettings();
}

void Program::eventSetScalePieces(Button* but) {
	World::sets()->scalePieces = static_cast<CheckBox*>(but)->on;
	eventSaveSettings();
}

void Program::eventSetTooltips(Button* but) {
	World::sets()->tooltips = static_cast<CheckBox*>(but)->on;
	World::scene()->resetLayouts();
	eventSaveSettings();
}

void Program::eventSetChatLineLimitSL(Button* but) {
	setStandardSlider(static_cast<Slider*>(but), World::sets()->chatLines);
}

void Program::eventSetChatLineLimitLE(Button* but) {
	setStandardSlider(static_cast<LabelEdit*>(but), World::sets()->chatLines);
}

void Program::eventSetDeadzoneSL(Button* but) {
	setStandardSlider(static_cast<Slider*>(but), World::sets()->deadzone);
}

void Program::eventSetDeadzoneLE(Button* but) {
	setStandardSlider(static_cast<LabelEdit*>(but), World::sets()->deadzone);
}

void Program::eventSetResolveFamily(sizet id, const string&) {
	World::sets()->resolveFamily = Com::Family(id);
	eventSaveSettings();
}

void Program::eventSetFontRegular(Button* but) {
	World::window()->reloadFont(static_cast<CheckBox*>(but)->on);
	World::scene()->resetLayouts();
	eventSaveSettings();
}

template <class T>
void Program::setStandardSlider(Slider* sl, T& val) {
	val = T(sl->getVal());
	static_cast<LabelEdit*>(sl->getParent()->getWidget(sl->getIndex() + 1))->setText(toStr(val));
	eventSaveSettings();
}

template <class T>
void Program::setStandardSlider(LabelEdit* le, T& val) {
	Slider* sl = static_cast<Slider*>(le->getParent()->getWidget(le->getIndex() - 1));
	val = T(std::clamp(sstol(le->getText()), long(sl->getMin()), long(sl->getMax())));
	le->setText(toStr(val));
	sl->setVal(val);
	eventSaveSettings();
}

void Program::eventResetSettings(Button*) {
	World::window()->resetSettings();
	eventSaveSettings();
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

void Program::eventExit(Button*) {
	World::window()->close();
}

void Program::eventSLUpdateLE(Button* but) {
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getIndex() + 1))->setText(toStr(static_cast<Slider*>(but)->getVal()));
}

void Program::eventPrcSliderUpdate(Button* but) {
	static_cast<LabelEdit*>(but->getParent()->getWidget(but->getIndex() + 1))->setText(toStr(static_cast<Slider*>(but)->getVal()) + '%');
}

void Program::eventClearLabel(Button* but) {
	static_cast<Label*>(but)->setText(string());
}

void Program::connect(bool client, const char* msg) {
	try {
		netcp.reset(client ? new Netcp : new NetcpHost);
		netcp->connect();
		World::scene()->setPopup(state->createPopupMessage(msg, &Program::eventConnectCancel, "Cancel"));
	} catch (const Com::Error& err) {
		netcp.reset();
		World::scene()->setPopup(state->createPopupMessage(err.message, &Program::eventClosePopup));
	}
}

void Program::disconnect() {
	if (netcp) {
		netcp->disconnect();
		netcp.reset();
	}
}

void Program::eventCycleFrameCounter() {
	Overlay* box = static_cast<Overlay*>(state->getFpsText()->getParent());
	if (ftimeMode = ftimeMode < FrameTime::seconds ? ftimeMode + 1 : FrameTime::none; ftimeMode == FrameTime::none)
		box->setShow(false);
	else {
		ProgState::Text txt = state->makeFpsText(World::window()->getDeltaSec());
		box->setShow(true);
		box->relSize = txt.length;
		box->onResize();
		state->getFpsText()->setText(std::move(txt.text));
		ftimeSleep = ftimeUpdateDelay;
	}
}

void Program::setState(ProgState* newState) {
	state.reset(newState);
	World::scene()->resetLayouts();
	ftimeSleep = ftimeUpdateDelay;	// because the frame time counter gets updated in resetLayouts
}

BoardObject* Program::pickBob(svec2& pos, Piece*& pce) {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getSelect());
	pos = bob ? game.board.ptog(bob->getPos()) : svec2(UINT16_MAX);
	pce = dynamic_cast<Piece*>(bob);
	return bob;
}

#ifdef EMSCRIPTEN
void Program::fetchVersionSucceed(emscripten_fetch_t* fetch) {
	pushFetchedVersion(string(static_cast<const char*>(fetch->data), fetch->numBytes), static_cast<char*>(fetch->userData));
	delete[] static_cast<char*>(fetch->userData);
	emscripten_fetch_close(fetch);
}

void Program::fetchVersionFail(emscripten_fetch_t* fetch) {
	pushEvent(UserCode::versionFetch, nullptr);
	delete[] static_cast<char*>(fetch->userData);
	emscripten_fetch_close(fetch);
}
#elif defined(WEBUTILS)
int Program::fetchVersion(void* data) {
	pairStr* ver = static_cast<pairStr*>(data);
	CURL* curl = curl_easy_init();
	if (!curl) {
		pushEvent(UserCode::versionFetch);
		delete ver;
		return CURLE_FAILED_INIT;
	}

	string html;
	curl_easy_setopt(curl, CURLOPT_URL, ver->first.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeText);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
	CURLcode code = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (code == CURLE_OK)
		pushFetchedVersion(html, ver->second);
	else
		pushEvent(UserCode::versionFetch, nullptr);
	delete ver;
	return code;
}

sizet Program::writeText(char* ptr, sizet size, sizet nmemb, void* userdata) {
	sizet len = size * nmemb;
	static_cast<string*>(userdata)->append(ptr, len);
	return len;
}
#endif
void Program::pushFetchedVersion(const string& html, const string& rver) {
	if (std::smatch sm; std::regex_search(html, sm, std::regex(rver)) && sm.size() >= 2) {
		string latest = sm[1];
		uvec4 cur = stoiv<uvec4>(Com::commonVersion, strtoul), web = stoiv<uvec4>(latest.c_str(), strtoul);
		glm::length_t i = 0;
		for (; i < uvec4::length() - 1 && cur[i] == web[i]; i++);
		pushEvent(UserCode::versionFetch, cur[i] >= web[i] ? new string() : new string("New version " + latest + " available"));
	} else
		pushEvent(UserCode::versionFetch, nullptr);
}
