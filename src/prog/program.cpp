#include "netcp.h"
#include "progs.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include <iostream>
#include <regex>
#ifdef WEBUTILS
#include <curl/curl.h>
#endif

// PROGRAM

Program::Program() :
	info(INF_NONE),
	ftimeMode(FrameTime::none),
#ifdef WEBUTILS
	proc(nullptr),
#endif
	ftimeSleep(ftimeUpdateDelay)
{}

Program::~Program() {
#ifdef WEBUTILS
	if (proc)
		SDL_DetachThread(proc);
#endif
}

void Program::start() {
	gui.resize();
	game.board.initObjects(Config(), false, World::sets(), World::scene());	// doesn't need to be here but I like having the game board in the background
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
			latestVersion = std::move(*ver);
			if (ProgMenu* pm = dynamic_cast<ProgMenu*>(state.get()))
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
	try {
		if (netcp)
			netcp->tick();
	} catch (const Com::Error& err) {
		dynamic_cast<ProgGame*>(state.get()) ? showGameError(err) : showLobbyError(err);
	}

	if (ftimeMode != FrameTime::none)
		if (ftimeSleep -= dSec; ftimeSleep <= 0.f) {
			ftimeSleep = ftimeUpdateDelay;
			GuiGen::Text txt = gui.makeFpsText(dSec);
			static_cast<Overlay*>(state->getFpsText()->getParent())->setSize(txt.length);
			state->getFpsText()->setText(std::move(txt.text));
		}
}

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	info &= ~INF_HOST;
	setState<ProgMenu>();
}

void Program::eventConnectServer(Button*) {
	connect(true, "Connecting...");
}

void Program::eventConnectCancel(Button*) {
	disconnect();
	eventClosePopup();
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

void Program::eventOpenLobby(const uint8* data, const char* message) {
	chatPrefix = toStr(Com::read64(data)) + ": ";
	data += sizeof(uint64);

	vector<pair<string, bool>> rooms(Com::read16(data));
	data += sizeof(uint16);
	for (auto& [name, open] : rooms) {
		open = *data & 0x80;
		name = Com::readName(data, 0x7F);
		data += name.length() + 1;
	}

	info &= ~(INF_HOST | INF_UNIQ | INF_GUEST_WAITING);
	netcp->setTickproc(&Netcp::tickLobby);
	setState<ProgLobby>(std::move(rooms));
	if (message)
		gui.openPopupMessage(message, &Program::eventClosePopup);
}

void Program::eventHostRoomInput(Button*) {
	gui.openPopupInput("Name:", string(), &Program::eventHostRoomRequest, Com::roomNameLimit);
}

void Program::eventHostRoomRequest(Button*) {
	try {
#ifdef EMSCRIPTEN
		if (!FileSys::canRead())
			throw "Waiting for files to sync";
#endif
		const string& name = World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
		if (static_cast<ProgLobby*>(state.get())->hasRoom(name))
			throw "Name taken";
		sendRoomName(Com::Code::rnew, name);
	} catch (const char* err) {
		gui.openPopupMessage(err, &Program::eventClosePopup);
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventHostRoomReceive(const uint8* data) {
	if (Com::CncrnewCode code = Com::CncrnewCode(*data); code == Com::CncrnewCode::ok) {
		info = (info | INF_HOST) & ~INF_GUEST_WAITING;
		setState<ProgRoom>();
	} else
		gui.openPopupMessage(code == Com::CncrnewCode::full ? "Server full" : code == Com::CncrnewCode::taken ? "Name taken" : "Name too long", &Program::eventClosePopup);
}

void Program::eventJoinRoomRequest(Button* but) {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead())
		return gui.openPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
	try {
		sendRoomName(Com::Code::join, static_cast<Label*>(but)->getText());
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventJoinRoomReceive(const uint8* data) {
	if (*data)
		setState<ProgRoom>(game.board.config.fromComData(data + 1));
	else
		gui.openPopupMessage("Failed to join room", &Program::eventClosePopup);
}

void Program::sendRoomName(Com::Code code, const string& name) {
	vector<uint8> data(Com::dataHeadSize + 1 + name.length());
	data[0] = uint8(code);
	Com::write16(data.data() + 1, uint16(data.size()));
	data[Com::dataHeadSize] = uint8(name.length());
	std::copy(name.begin(), name.end(), data.data() + Com::dataHeadSize + 1);
	netcp->sendData(data);
}

void Program::eventSendMessage(Button* but) {
	Com::Code code = dynamic_cast<ProgLobby*>(state.get()) ? Com::Code::glmessage : Com::Code::message;
	bool inGame = dynamic_cast<ProgGame*>(state.get());
	const string& msg = static_cast<LabelEdit*>(but)->getOldText();
	but->getParent()->getWidget<TextBox>(but->getIndex() - 1)->addLine(chatPrefix + msg);
	try {
		if (code == Com::Code::glmessage || inGame || (info & (INF_HOST | INF_GUEST_WAITING)) != INF_HOST) {
			vector<uint8> data(Com::dataHeadSize + chatPrefix.length() + msg.length());
			data[0] = uint8(code);
			Com::write16(data.data() + 1, uint16(data.size()));
			std::copy(chatPrefix.begin(), chatPrefix.end(), data.begin() + Com::dataHeadSize);
			std::copy(msg.begin(), msg.end(), data.begin() + Com::dataHeadSize + chatPrefix.length());
			netcp->sendData(data);
		}
#ifdef DEBUG
		if (World::args.hasFlag(Settings::argConsole) && dynamic_cast<ProgMatch*>(state.get()) && !msg.empty() && msg[0] == '/')
			game.processCommand(msg.c_str() + 1);
#endif
	} catch (const Com::Error& err) {
		inGame ? showGameError(err) : showLobbyError(err);
	}
}

void Program::eventRecvMessage(const uint8* data) {
	if (string msg = Com::readText(data); state->getChat()) {
		state->getChat()->addLine(msg);
		if (Overlay* lay = dynamic_cast<Overlay*>(state->getChat()->getParent())) {
			if (!lay->getShow())
				state->showNotification(true);
		} else if (!state->getChat()->getParent()->getSize().pix)
			state->showNotification(true);
	} else
		std::cout << "net message from " << msg << std::endl;
}

void Program::eventExitLobby(Button*) {
	disconnect();
	eventOpenMainMenu();
}

void Program::showLobbyError(const Com::Error& err) {
	netcp.reset();
	gui.openPopupMessage(err.what(), &Program::eventOpenMainMenu);
}

// ROOM MENU

void Program::eventOpenHostMenu(Button*) {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead())
		return gui.openPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
	info |= INF_HOST | INF_UNIQ;
	setState<ProgRoom>();
}

void Program::eventStartGame(Button*) {
	try {
		game.sendStart();
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventHostServer(Button*) {
	FileSys::saveConfigs(static_cast<ProgRoom*>(state.get())->confs);
	connect(false, "Waiting for player...");
}

void Program::eventSwitchConfig(sizet, const string& str) {
	setSaveConfig(str, false);
}

void Program::eventConfigDelete(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	vector<string> names = sortNames(pr->confs);
	vector<string>::iterator nit = std::find(names.begin(), names.end(), pr->configName->getText());
	pr->confs.erase(pr->configName->getText());
	setSaveConfig(*(nit + 1 != names.end() ? nit + 1 : nit - 1));
}

void Program::eventConfigCopyInput(Button*) {
	gui.openPopupInput("Name:", static_cast<ProgRoom*>(state.get())->configName->getText(), &Program::eventConfigCopy, Config::maxNameLength);
}

void Program::eventConfigCopy(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	const string& name = World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
	pr->confs.insert_or_assign(name, pr->confs[pr->configName->getText()]);
	if (info & INF_HOST)
		setSaveConfig(name);
	else
		FileSys::saveConfigs(pr->confs);
	eventClosePopup();
}

void Program::eventConfigNewInput(Button*) {
	gui.openPopupInput("Name:", string(), &Program::eventConfigNew, Config::maxNameLength);
}

void Program::eventConfigNew(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	const string& name = World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
	pr->confs.insert_or_assign(name, Config());
	setSaveConfig(name);
	eventClosePopup();
}

void Program::setSaveConfig(const string& name, bool save) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Config& cfg = pr->confs[name];
	cfg.checkValues();
	if (save)
		FileSys::saveConfigs(pr->confs);
	World::sets()->lastConfig = name;
	eventSaveSettings();

	pr->configName->set(sortNames(pr->confs), name);
	pr->updateDelButton();
	pr->updateConfigWidgets(cfg);
}

void Program::eventUpdateConfig(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state.get());
	Config& cfg = ph->confs[ph->configName->getText()];
	svec2 newHome = glm::clamp(svec2(uint16(sstol(ph->wio.width->getText())), uint16(sstol(ph->wio.height->getText()))), Config::minHomeSize, Config::maxHomeSize);
	setConfigAmounts(cfg.tileAmounts.data(), ph->wio.tiles.data(), Tile::lim, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scaleTiles);
	setConfigAmounts(cfg.middleAmounts.data(), ph->wio.middles.data(), Tile::lim, cfg.homeSize.x, newHome.x, World::sets()->scaleTiles);
	setConfigAmounts(cfg.pieceAmounts.data(), ph->wio.pieces.data(), Piece::lim, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scalePieces);
	cfg.homeSize = newHome;
	cfg.opts = ph->wio.victoryPoints->on ? cfg.opts | Config::victoryPoints : cfg.opts & ~Config::victoryPoints;
	cfg.victoryPointsNum = uint16(sstol(ph->wio.victoryPointsNum->getText()));
	cfg.opts = ph->wio.vpEquidistant->on ? cfg.opts | Config::victoryPointsEquidistant : cfg.opts & ~Config::victoryPointsEquidistant;
	cfg.opts = ph->wio.ports->on ? cfg.opts | Config::ports : cfg.opts & ~Config::ports;
	cfg.opts = ph->wio.rowBalancing->on ? cfg.opts | Config::rowBalancing : cfg.opts & ~Config::rowBalancing;
	cfg.opts = ph->wio.homefront->on ? cfg.opts | Config::homefront : cfg.opts & ~Config::homefront;
	cfg.opts = ph->wio.setPieceBattle->on ? cfg.opts | Config::setPieceBattle : cfg.opts & ~Config::setPieceBattle;
	cfg.setPieceBattleNum = uint16(sstol(ph->wio.setPieceBattleNum->getText()));
	cfg.battlePass = uint8(sstol(ph->wio.battleLE->getText()));
	cfg.opts = ph->wio.favorTotal->on ? cfg.opts | Config::favorTotal : cfg.opts & ~Config::favorTotal;
	cfg.favorLimit = uint16(sstol(ph->wio.favorLimit->getText()));
	cfg.opts = ph->wio.firstTurnEngage->on ? cfg.opts | Config::firstTurnEngage : cfg.opts & ~Config::firstTurnEngage;
	cfg.opts = ph->wio.terrainRules->on ? cfg.opts | Config::terrainRules : cfg.opts & ~Config::terrainRules;
	cfg.opts = ph->wio.dragonLate->on ? cfg.opts | Config::dragonLate : cfg.opts & ~Config::dragonLate;
	cfg.opts = ph->wio.dragonStraight->on ? cfg.opts | Config::dragonStraight : cfg.opts & ~Config::dragonStraight;
	cfg.winThrone = uint16(sstol(ph->wio.winThrone->getText()));
	cfg.winFortress = uint16(sstol(ph->wio.winFortress->getText()));
	cfg.capturers = 0;
	for (uint8 i = 0; i < Piece::lim; ++i)
		cfg.capturers |= uint16(ph->wio.capturers[i]->selected) << i;
	cfg.checkValues();
	ph->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::eventUpdateConfigI(Button* but) {
	static_cast<Icon*>(but)->selected = !static_cast<Icon*>(but)->selected;
	eventUpdateConfig();
}

void Program::eventUpdateConfigV(Button* but) {
	if (World::sets()->autoVictoryPoints) {
		ProgRoom* ph = static_cast<ProgRoom*>(state.get());
		Config& cfg = ph->confs[ph->configName->getText()];
		uint16 tils = cfg.countTiles(), mids = cfg.countMiddles();
		const char* num;
		if (static_cast<CheckBox*>(but)->on) {
			if (uint16 must = cfg.homeSize.x * cfg.homeSize.y; tils < must)
				Config::ceilAmounts(tils, must, cfg.tileAmounts.data(), uint8(cfg.tileAmounts.size() - 1));
			if (uint16 must = (cfg.homeSize.x - cfg.homeSize.x / 3) / 2; mids > must)
				Config::floorAmounts(mids, cfg.middleAmounts.data(), must, uint8(cfg.middleAmounts.size() - 1));
			num = "0";
		} else {
			if (uint16 must = cfg.homeSize.x * cfg.homeSize.y - 1; tils > must)
				Config::floorAmounts(tils, cfg.tileAmounts.data(), must, uint8(cfg.tileAmounts.size() - 1));
			if (uint16 must = (cfg.homeSize.x - cfg.homeSize.x / 9) / 2; mids < must)
				Config::ceilAmounts(mids, must, cfg.middleAmounts.data(), uint8(cfg.middleAmounts.size() - 1));
			num = "1";
		}
		ProgRoom::setAmtSliders(cfg, cfg.tileAmounts.data(), ph->wio.tiles.data(), ph->wio.tileFortress, Tile::lim, cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, &Config::countFreeTiles, &GuiGen::tileFortressString);
		ProgRoom::setAmtSliders(cfg, cfg.middleAmounts.data(), ph->wio.middles.data(), ph->wio.middleFortress, Tile::lim, 0, &Config::countFreeMiddles, &GuiGen::middleFortressString);
		ph->wio.winThrone->setText(num);
		ph->wio.winFortress->setText(num);
	}
	eventUpdateConfig();
}

void Program::eventUpdateReset(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Config& cfg = pr->confs[pr->configName->getText()];
	cfg = Config();
	pr->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::postConfigUpdate() {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	try {
		if ((info & (INF_UNIQ | INF_GUEST_WAITING)) == INF_GUEST_WAITING)	// only send if is host on remote server with guest
			game.sendConfig();
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
	FileSys::saveConfigs(pr->confs);
}

void Program::setConfigAmounts(uint16* amts, LabelEdit** wgts, uint8 acnt, uint16 oarea, uint16 narea, bool scale) {
	for (uint8 i = 0; i < acnt; ++i)
		amts[i] = scale && narea != oarea ? uint16(std::round(float(amts[i]) * float(narea) / float(oarea))) : uint16(sstoul(wgts[i]->getText()));
}

void Program::eventTileSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.tileAmounts.data(), pr->wio.tiles.data(), pr->wio.tileFortress, Tile::lim, cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, &Config::countFreeTiles, GuiGen::tileFortressString);
}

void Program::eventMiddleSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.middleAmounts.data(), pr->wio.middles.data(), pr->wio.middleFortress, Tile::lim, 0, &Config::countFreeMiddles, GuiGen::middleFortressString);
}

void Program::eventPieceSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.pieceAmounts.data(), pr->wio.pieces.data(), pr->wio.pieceTotal, Piece::lim, 0, &Config::countFreePieces, GuiGen::pieceTotalString);
}

void Program::eventRecvConfig(const uint8* data) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	pr->configName->set({ game.board.config.fromComData(data) }, 0);
	pr->updateConfigWidgets(game.board.config);
}

void Program::updateAmtSliders(Slider* sl, Config& cfg, uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Config::*counter)() const, string (*totstr)(const Config&)) {
	LabelEdit* le = sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1);
	amts[sizet(std::find(wgts, wgts + cnt, le) - wgts)] = uint16(sl->getValue());
	le->setText(toStr(uint16(sl->getValue())));
	ProgRoom::updateAmtSliders(amts, wgts, cnt, min, (cfg.*counter)());
	total->setText(totstr(cfg));
}

void Program::eventTransferHost(Button*) {
	try {
		netcp->sendData(Com::Code::thost);
		info &= ~(INF_HOST | INF_GUEST_WAITING);
		resetLayoutsWithChat();
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventRecvHost(bool hasGuest) {
	info = hasGuest ? info | INF_HOST | INF_GUEST_WAITING : (info | INF_HOST) & ~INF_GUEST_WAITING;
	static_cast<ProgRoom*>(state.get())->setStartConfig();
	resetLayoutsWithChat();
}

void Program::eventKickPlayer(Button*) {
	try {
		netcp->sendData(Com::Code::kick);
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventPlayerHello(bool onJoin) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	game.sendConfig(onJoin);
	pr->updateStartButton();
	pr->toggleChatEmbedShow(true);
}

void Program::eventRoomPlayerLeft() {
	if (info & INF_HOST) {
		World::program()->info &= ~Program::INF_GUEST_WAITING;
		World::state<ProgRoom>()->updateStartButton();
	} else
		eventRecvHost(false);
}

void Program::eventExitRoom(Button*) {
	try {
		netcp->sendData(Com::Code::leave);
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventChatOpen(Button*) {
	LabelEdit* le = state->getChat()->getParent()->getWidget<LabelEdit>(state->getChat()->getIndex() + 1);
	le->onClick(le->position(), SDL_BUTTON_LEFT);
}

void Program::eventChatClose(Button*) {
	state->getChat()->getParent()->getWidget<LabelEdit>(state->getChat()->getIndex() + 1)->cancel();
}

void Program::eventToggleChat(Button*) {
	if (state->getChat()) {
		if (Overlay* lay = dynamic_cast<Overlay*>(state->getChat()->getParent())) {
			lay->setShow(!lay->getShow());
			state->showNotification(false);
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
	Label* lb = but->getParent()->getWidget<Label>(but->getIndex() + 1);
	lb->onClick(lb->position(), SDL_BUTTON_LEFT);
}

// GAME SETUP

void Program::eventStartUnique() {
	chatPrefix = "0: ";
	game.sendStart();
}

void Program::eventStartUnique(const uint8* data) {
	info |= INF_UNIQ;
	chatPrefix = "1: ";
	game.recvStart(data);
}

void Program::eventOpenSetup(string configName) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	const Config& cfg = info & INF_HOST ? pr->confs[pr->configName->getText()] : game.board.config;
#ifdef NDEBUG
	game.board.initObjects(cfg, true, World::sets(), World::scene());
#else
	World::args.hasFlag(Settings::argSetup) ? game.board.initDummyObjects(cfg, World::sets(), World::scene()) : game.board.initObjects(cfg, true, World::sets(), World::scene());
#endif
	netcp->setTickproc(&Netcp::tickGame);
	info &= ~INF_GUEST_WAITING;
	setStateWithChat<ProgSetup>(std::move(configName));

	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setStage(ProgSetup::Stage::tiles);
	ps->rcvMidBuffer.resize(game.board.config.homeSize.x);
	ps->piecePicksLeft = 0;
	if ((game.board.config.opts & Config::setPieceBattle) && game.board.config.setPieceBattleNum < game.board.config.countPieces()) {
		game.board.ownPieceAmts.fill(0);
		ps->piecePicksLeft = game.board.config.setPieceBattleNum;
		if (game.board.config.winThrone) {
			game.board.ownPieceAmts[Piece::throne] += game.board.config.winThrone;
			ps->piecePicksLeft -= game.board.config.winThrone;
		} else {
			uint16 caps = game.board.config.winFortress;
			for (uint8 i = Piece::throne; i < Piece::lim && caps; --i)
				if (game.board.config.capturers & (1 << i)) {
					uint16 diff = std::min(caps, uint16(game.board.config.pieceAmounts[i] - game.board.ownPieceAmts[i]));
					game.board.ownPieceAmts[i] += diff;
					ps->piecePicksLeft -= diff;
					caps -= diff;
				}
		}
	} else
		game.board.ownPieceAmts = game.board.config.pieceAmounts;

	if (ps->piecePicksLeft)
		gui.openPopupPiecePicker(ps->piecePicksLeft);
	else
		game.board.initOwnPieces();
}

void Program::eventOpenSetup(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state.get());
	game.board.initObjects(pr->confs[pr->configName->getText()], false, World::sets(), World::scene());
	game.board.ownPieceAmts = game.board.config.pieceAmounts;
	game.board.initOwnPieces();
	setState<ProgSetup>(pr->configName->getText());
	static_cast<ProgSetup*>(state.get())->setStage(ProgSetup::Stage::tiles);
}

void Program::eventIconSelect(Button* but) {
	state->eventSetSelected(uint8(but->getIndex() - 1));
}

void Program::eventPlaceTile() {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = ps->getSelected();
	if (Tile* tile = dynamic_cast<Tile*>(World::scene()->getSelect()); ps->getCount(type) && tile) {
		if (tile->getType() < Tile::fortress)
			ps->incdecIcon(uint8(tile->getType()), true);

		tile->setType(Tile::Type(type));
		tile->setInteractivity(Tile::Interact::interact);
		ps->incdecIcon(type, false);
	}
}

void Program::eventPlacePiece() {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = ps->getSelected();
	if (auto [bob, occupant, pos] = pickBob(); ps->getCount(type) && bob) {
		if (occupant) {
			occupant->updatePos();
			ps->incdecIcon(uint8(occupant->getType()), true);
		}

		Piece* pieces = game.board.getOwnPieces(Piece::Type(type));
		std::find_if(pieces, pieces + game.board.ownPieceAmts[type], [](Piece& it) -> bool { return !it.show; })->updatePos(pos, true);
		ps->incdecIcon(type, false);
	}
}

void Program::eventMoveTile(BoardObject* obj, uint8) {
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->getSelect())) {
		Tile::Type desType = dst->getType();
		dst->setType(src->getType());
		dst->setInteractivity(Tile::Interact::interact);
		src->setType(desType);
		src->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventMovePiece(BoardObject* obj, uint8) {
	if (auto [bob, dst, pos] = pickBob(); bob) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->updatePos(pos);
	}
}

void Program::eventClearTile() {
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->getSelect()); til && til->getType() != Tile::empty) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true);
		til->setType(Tile::empty);
		til->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventClearPiece() {
	if (auto [bob, pce, pos] = pickBob(); pce) {
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
				gui.openPopupFavorPick(game.availableFF);
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
		gui.openPopupMessage(err, &Program::eventClosePopup);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.board.takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventOpenSetupSave(Button*) {
	openPopupSaveLoad(true);
}

void Program::eventOpenSetupLoad(Button*) {
	openPopupSaveLoad(false);
}

void Program::openPopupSaveLoad(bool save) {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead())
		return gui.openPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	ps->setups = FileSys::loadSetups();
	gui.openPopupSaveLoad(ps->setups, save);
}

void Program::eventSetupPickPiece(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	uint8 type = uint8(but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex());
	if (++game.board.ownPieceAmts[type]; --ps->piecePicksLeft) {
		but->getParent()->getParent()->getParent()->getParent()->getWidget<Label>(0)->setText(GuiGen::msgPickPiece + toStr(ps->piecePicksLeft) + ')');
		if (game.board.config.pieceAmounts[type] == game.board.ownPieceAmts[type]) {
			Icon* ico = static_cast<Icon*>(but);
			ico->setDim(GuiGen::defaultDim);
			ico->lcall = nullptr;
		}
	} else {
		game.board.initOwnPieces();
		eventClosePopup();
	}
}

void Program::eventSetupNew(Button* but) {
	const string& name = (dynamic_cast<LabelEdit*>(but) ? static_cast<LabelEdit*>(but) : but->getParent()->getWidget<LabelEdit>(but->getIndex() - 1))->getText();
	popuplateSetup(static_cast<ProgSetup*>(state.get())->setups.insert_or_assign(name, Setup()).first->second);
}

void Program::eventSetupSave(Button* but) {
	umap<string, Setup>& setups = static_cast<ProgSetup*>(state.get())->setups;
	Setup& stp = setups.find(static_cast<Label*>(but)->getText())->second;
	stp.clear();
	popuplateSetup(stp);
}

void Program::popuplateSetup(Setup& setup) {
	for (Tile* it = game.board.getTiles().own(); it != game.board.getTiles().end(); ++it)
		if (it->getType() < Tile::fortress) {
			svec2 pos = game.board.ptog(it->getPos());
			setup.tiles.emplace_back(svec2(pos.x, pos.y - game.board.config.homeSize.y - 1), it->getType());
		}
	for (Tile* it = game.board.getTiles().mid(); it != game.board.getTiles().own(); ++it)
		if (it->getType() < Tile::fortress)
			setup.mids.emplace_back(uint16(it - game.board.getTiles().mid()), it->getType());
	for (Piece* it = game.board.getPieces().own(); it != game.board.getPieces().ene(); ++it)
		if (game.board.pieceOnBoard(it)) {
			svec2 pos = game.board.ptog(it->getPos());
			setup.pieces.emplace_back(svec2(pos.x, pos.y - game.board.config.homeSize.y - 1), it->getType());
		}
	FileSys::saveSetups(static_cast<ProgSetup*>(state.get())->setups);
	eventClosePopup();
}

void Program::eventSetupLoad(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	Setup& stp = ps->setups.find(static_cast<Label*>(but)->getText())->second;
	if (!stp.tiles.empty()) {
		for (Tile* it = game.board.getTiles().own(); it != game.board.getTiles().end(); ++it)
			it->setType(Tile::empty);

		array<uint16, Tile::lim> cnt = game.board.config.tileAmounts;
		for (auto [pos, type] : stp.tiles)
			if (pos.x < game.board.config.homeSize.x && pos.y < game.board.config.homeSize.y && cnt[uint8(type)]) {
				--cnt[uint8(type)];
				game.board.getTile(svec2(pos.x, pos.y + game.board.config.homeSize.y + 1))->setType(type);
			}
	}
	if (!stp.mids.empty()) {
		for (Tile* it = game.board.getTiles().mid(); it != game.board.getTiles().own(); ++it)
			it->setType(Tile::empty);

		array<uint16, Tile::lim> cnt = game.board.config.middleAmounts;
		for (auto [pos, type] : stp.mids)
			if (pos < game.board.config.homeSize.x && cnt[uint8(type)]) {
				--cnt[uint8(type)];
				game.board.getTiles().mid(pos)->setType(type);
			}
	}
	if (!stp.pieces.empty()) {
		for (Piece* it = game.board.getPieces().own(); it != game.board.getPieces().ene(); ++it)
			it->updatePos();

		array<uint16, Piece::lim> cnt = game.board.ownPieceAmts;
		for (auto [pos, type] : stp.pieces)
			if (pos.x < game.board.config.homeSize.x && pos.y < game.board.config.homeSize.y && cnt[uint8(type)]) {
				--cnt[uint8(type)];
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
	ProgGame* pg = static_cast<ProgGame*>(state.get());
	gui.openPopupConfig(pg->configName, game.board.config, pg->configList, dynamic_cast<ProgMatch*>(pg));
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
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	game.board.prepareMatch(game.getMyTurn(), ps->rcvMidBuffer.data());
	setStateWithChat<ProgMatch>(std::move(ps->configName));
	game.prepareTurn(false);
	World::scene()->addAnimation(Animation(game.board.getScreen(), std::queue<Keyframe>({ Keyframe(transAnimTime, vec3(game.board.getScreen()->getPos().x, Board::screenYDown, game.board.getScreen()->getPos().z)), Keyframe(0.f, std::nullopt, std::nullopt, vec3(0.f)) })));
	World::scene()->addAnimation(Animation(&World::scene()->camera, std::queue<Keyframe>({ Keyframe(transAnimTime, Camera::posMatch, std::nullopt, Camera::latMatch) })));
	World::scene()->camera.pmax = Camera::pmaxMatch;
	World::scene()->camera.ymax = Camera::ymaxMatch;
}

void Program::eventEndTurn(Button*) {
	try {
		game.finishFavor(Favor::none, static_cast<ProgMatch*>(state.get())->favorIconSelect());	// in case assault FF has been used
		if (!game.checkPointsWin())
			game.endTurn();
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventPickFavor(Button* but) {
	uint8 fid = uint8(but->getIndex() - 1);
	if (++game.favorsCount[fid]; game.board.config.opts & Config::favorTotal)
		--game.favorsLeft[fid];

	if (--game.availableFF) {
		if (but->getParent()->getParent()->getWidget<Label>(0)->setText(GuiGen::msgFavorPick + string(" (") + toStr(game.availableFF) + ')'); !game.favorsLeft[fid]) {
			but->setDim(GuiGen::defaultDim);
			but->lcall = nullptr;
		}
	} else if (eventClosePopup(); ProgMatch* pm = dynamic_cast<ProgMatch*>(state.get()))
		pm->updateIcons();
	else
		eventSetupNext();
}

void Program::eventSelectFavor(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	pm->setIcons(Favor(std::find(pm->getFavorIcons().begin(), pm->getFavorIcons().end(), but) - pm->getFavorIcons().begin()));
}

void Program::eventSwitchDestroy(Button*) {
	state->eventDestroyToggle();
}

void Program::eventKillDestroy(Button*) {
	try {
		game.changeTile(game.board.getTile(game.board.ptog(game.board.getPxpad()->getPos())), Tile::plains);
		eventCancelDestroy();
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventCancelDestroy(Button*) {
	game.board.setPxpadPos(nullptr);
	eventClosePopup();
}

void Program::eventClickPlaceDragon(Button* but) {
	static_cast<Icon*>(but)->selected = true;
	Tile* til = game.board.getTiles().own();
	for (; til != game.board.getTiles().end() && til->getType() != Tile::fortress; ++til);
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
	try {
		if (auto [bob, pce, pos] = pickBob(); bob)
			game.placeDragon(static_cast<Piece*>(obj), pos, pce);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

Piece* Program::getUnplacedDragon() {
	Piece* pce = game.board.getOwnPieces(Piece::dragon);
	for (; pce->getType() == Piece::dragon && pce->show; ++pce);
	pce->hgcall = nullptr;
	pce->ulcall = pce->urcall = &Program::eventPlaceDragon;
	return pce;
}

void Program::eventEstablish(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	if (!pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		return;

	switch (std::count_if(game.board.getOwnPieces(Piece::throne), game.board.getPieces().ene(), [](Piece& it) -> bool { return it.show; })) {
	case 0:
		gui.openPopupMessage("No " + string(Piece::names[Piece::throne]) + " for establishing", &Program::eventClosePopupResetIcons);
		break;
	case 1:
		try {
			game.establishTile(std::find_if(game.board.getOwnPieces(Piece::throne), game.board.getPieces().ene(), [](Piece& it) -> bool { return it.show; }), false);
		} catch (const string& err) {
			gui.openPopupMessage(err, &Program::eventClosePopup);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
		break;
	default:
		game.board.selectEstablishers();
	}
}

void Program::eventEstablish(BoardObject* obj, uint8) {
	try {
		game.establishTile(static_cast<Piece*>(obj), true);
	} catch (const string& err) {
		gui.openPopupMessage(err, &Program::eventClosePopup);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventRebuildTile(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	if (!pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		return;

	switch (std::count_if(game.board.getOwnPieces(Piece::throne), game.board.getPieces().ene(), [this](Piece& it) -> bool { return game.board.tileRebuildable(&it); })) {
	case 0:
		gui.openPopupMessage("Can't rebuild", &Program::eventClosePopupResetIcons);
		break;
	case 1:
		try {
			game.rebuildTile(std::find_if(game.board.getOwnPieces(Piece::throne), game.board.getPieces().ene(), [this](Piece& it) -> bool { return game.board.tileRebuildable(&it); }), false);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
		break;
	default:
		game.board.selectRebuilders();
	}
}

void Program::eventRebuildTile(BoardObject* obj, uint8) {
	try {
		game.rebuildTile(static_cast<Piece*>(obj), true);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventOpenSpawner(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	if (pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		gui.openPopupSpawner();
}

void Program::eventSpawnPiece(Button* but) {
	Piece::Type type = Piece::Type(but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex());
	uint forts = uint(std::count_if(game.board.getTiles().own(), game.board.getTiles().end(), [](const Tile& it) -> bool { return it.isUnbreachedFortress(); }));
	if (type == Piece::lancer || type == Piece::rangers || type == Piece::spearmen || type == Piece::catapult || type == Piece::elephant || forts == 1) {
		try {
			game.spawnPiece(type, game.board.findSpawnableTile(type), false);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
	} else {
		static_cast<ProgMatch*>(state.get())->spawning = type;
		game.board.selectSpawners();
	}
	eventClosePopup();
}

void Program::eventSpawnPiece(BoardObject* obj, uint8) {
	try {
		game.spawnPiece(static_cast<ProgMatch*>(state.get())->spawning, static_cast<Tile*>(obj), true);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventClosePopupResetIcons(Button*) {
	static_cast<ProgMatch*>(state.get())->updateIcons();
	eventClosePopup();
}

void Program::eventPieceStart(BoardObject* obj, uint8 mBut) {
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	Piece* pce = static_cast<Piece*>(obj);
	game.board.setPxpadPos(pm->getDestroyIcon()->selected ? pce : nullptr);
	mBut == SDL_BUTTON_LEFT ? game.board.highlightMoveTiles(pce, game.getEneRec(), pm->favorIconSelect()) : game.board.highlightEngageTiles(pce);
}

void Program::eventMove(BoardObject* obj, uint8) {
	game.board.highlightMoveTiles(nullptr, game.getEneRec(), static_cast<ProgMatch*>(state.get())->favorIconSelect());
	try {
		if (auto [bob, pce, pos] = pickBob(); bob)
			game.pieceMove(static_cast<Piece*>(obj), pos, pce, true);
	} catch (const string& err) {
		if (game.board.setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state.get())->message->setText(err);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventEngage(BoardObject* obj, uint8) {
	game.board.highlightEngageTiles(nullptr);
	try {
		if (auto [bob, pce, pos] = pickBob(); bob) {
			if (Piece* actor = static_cast<Piece*>(obj); actor->firingArea().first)
				game.pieceFire(actor, pos, pce);
			else
				game.pieceMove(actor, pos, pce, false);
		}
	} catch (const string& err) {
		if (game.board.setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state.get())->message->setText(err);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventPieceNoEngage(BoardObject* obj, uint8) {
	game.setNoEngage(static_cast<Piece*>(obj));
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
	try {
		game.surrender();
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::uninitGame() {
	game.board.uninitObjects();
	if (dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.board.getScreen(), std::queue<Keyframe>({ Keyframe(0.f, std::nullopt, std::nullopt, vec3(1.f)), Keyframe(transAnimTime, vec3(game.board.getScreen()->getPos().x, Board::screenYUp, game.board.getScreen()->getPos().z)) })));
		World::scene()->addAnimation(Animation(&World::scene()->camera, std::queue<Keyframe>({ Keyframe(transAnimTime, Camera::posSetup, std::nullopt, Camera::latSetup) })));
		for (Piece& it : game.board.getPieces())
			if (it.getPos().z <= game.board.getScreen()->getPos().z && it.getPos().z >= Config::boardWidth / -2.f)
				World::scene()->addAnimation(Animation(&it, std::queue<Keyframe>({ Keyframe(transAnimTime, vec3(it.getPos().x, pieceYDown, it.getPos().z)), Keyframe(0.f, std::nullopt, std::nullopt, vec3(0.f)) })));
		World::scene()->camera.pmax = Camera::pmaxSetup;
		World::scene()->camera.ymax = Camera::ymaxSetup;
	}
}

void Program::finishMatch(Record::Info win) {
	if (info & INF_UNIQ)
		disconnect();
	gui.openPopupMessage(win == Record::win ? "You win" : win == Record::loose ? "You lose" : "You tied", &Program::eventPostFinishMatch);
}

void Program::eventPostFinishMatch(Button*) {
	uninitGame();
	if (info & INF_UNIQ)
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	else try {
		netcp->setTickproc(&Netcp::tickLobby);
		if (info & INF_HOST) {
#ifdef EMSCRIPTEN
			if (!FileSys::canRead())
				return eventExitRoom();
#endif
			setStateWithChat<ProgRoom>();
		} else {
			netcp->sendData(Com::Code::hello);
			setStateWithChat<ProgRoom>(std::move(static_cast<ProgGame*>(state.get())->configName));
		}
		if (info & INF_GUEST_WAITING)
			eventPlayerHello(false);
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventGamePlayerLeft() {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead())
		return showGameError(Com::Error("Player left"));
#endif
	uninitGame();
	bool host = info & INF_HOST;
	info = (info | INF_HOST) & ~INF_GUEST_WAITING;
	netcp->setTickproc(&Netcp::tickLobby);
	host ? setStateWithChat<ProgRoom>() : setStateWithChat<ProgRoom>(std::move(static_cast<ProgGame*>(state.get())->configName));
	gui.openPopupMessage(host ? "Player left" : "Host left", &Program::eventClosePopup);
}

void Program::showGameError(const Com::Error& err) {
	netcp.reset();
	uninitGame();
	gui.openPopupMessage(err.what(), (info & (INF_HOST | INF_UNIQ)) == (INF_HOST | INF_UNIQ) ? &Program::eventOpenHostMenu : &Program::eventOpenMainMenu);
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState<ProgSettings>();
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
	World::sets()->mode = gui.fstrToDisp(static_cast<ProgSettings*>(state.get())->getPixelformats(), str);
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
	World::sets()->texScale = uint8(static_cast<Slider*>(but)->getValue());
	World::scene()->reloadTextures();
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->texScale) + '%');
	eventSaveSettings();
}

void Program::eventSetTextureScaleLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->texScale = uint8(std::clamp(sstoul(le->getText()), 1ul, 100ul));
	World::scene()->reloadTextures();
	le->setText(toStr(World::sets()->texScale) + '%');
	but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(int(World::sets()->texScale));
	eventSaveSettings();
}

void Program::eventSetShadowResSL(Button* but) {
	int val = static_cast<Slider*>(but)->getValue();
	setShadowRes(val >= 0 ? uint16(std::pow(2, val)) : 0);
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->shadowRes));
	eventSaveSettings();
}

void Program::eventSetShadowResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	setShadowRes(uint16(std::min(sstoul(le->getText()), ulong(Settings::shadowResMax))));
	le->setText(toStr(World::sets()->shadowRes));
	but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1);
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
	World::window()->setGamma(float(static_cast<Slider*>(but)->getValue()) / GuiGen::gammaStepFactor);
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->gamma));
	eventSaveSettings();
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::window()->setGamma(sstof(le->getText()));
	le->setText(toStr(World::sets()->gamma));
	but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(int(World::sets()->gamma * GuiGen::gammaStepFactor));
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
	for (; pos != end; ++pos)
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

void Program::eventSetAutoVictoryPoints(Button* but) {
	World::sets()->autoVictoryPoints = static_cast<CheckBox*>(but)->on;
	eventSaveSettings();
}

void Program::eventSetTooltips(Button* but) {
	World::sets()->tooltips = static_cast<CheckBox*>(but)->on;
	resetLayoutsWithChat();
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
	World::sets()->resolveFamily = Settings::Family(id);
	eventSaveSettings();
}

void Program::eventSetFontRegular(Button* but) {
	World::window()->reloadFont(static_cast<CheckBox*>(but)->on);
	resetLayoutsWithChat();
	eventSaveSettings();
}

void Program::eventSetInvertWheel(Button* but) {
	World::sets()->invertWheel = static_cast<CheckBox*>(but)->on;
	eventSaveSettings();
}

void Program::eventAddKeyBinding(Button* but) {
	gui.openPopupKeyGetter(Binding::Type(but->getParent()->getParent()->getIndex() - static_cast<ProgSettings*>(state.get())->bindingsStart));
}

void Program::eventSetNewBinding(Button* but) {
	ProgSettings* ps = static_cast<ProgSettings*>(state.get());
	KeyGetter* kg = static_cast<KeyGetter*>(but);
	Layout* lin = ps->content->getWidget<Layout>(ps->bindingsStart + sizet(kg->bind));
	Layout* lst = lin->getWidget<Layout>(uint8(kg->accept) + 1);
	lst->insertWidget(kg->kid, gui.createKeyGetter(kg->accept, kg->bind, kg->kid, lin->getWidget<Layout>(0)->getWidget<Label>(0)));
	lin->setSize(gui.keyGetLineSize(kg->bind));
	eventClosePopup();
	eventSaveSettings();
}

void Program::eventDelKeyBinding(Button* but) {
	KeyGetter* kg = static_cast<KeyGetter*>(but);
	switch (kg->accept) {
	case KeyGetter::Accept::keyboard:
		World::input()->delBindingK(kg->bind, kg->kid);
		break;
	case KeyGetter::Accept::joystick:
		World::input()->delBindingJ(kg->bind, kg->kid);
		break;
	case KeyGetter::Accept::gamepad:
		World::input()->delBindingG(kg->bind, kg->kid);
	}
	but->getParent()->deleteWidget(but->getIndex());
	but->getParent()->getParent()->setSize(gui.keyGetLineSize(kg->bind));
	eventSaveSettings();
}

template <class T>
void Program::setStandardSlider(Slider* sl, T& val) {
	val = T(sl->getValue());
	sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1)->setText(toStr(val));
	eventSaveSettings();
}

template <class T>
void Program::setStandardSlider(LabelEdit* le, T& val) {
	Slider* sl = le->getParent()->getWidget<Slider>(le->getIndex() - 1);
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
	FileSys::saveSettings(*World::sets(), World::input());
}

void Program::eventOpenInfo(Button*) {
	setState<ProgInfo>();
}

#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
void Program::eventOpenRules(Button*) {
	openDoc(FileSys::fileRules);
}

void Program::eventOpenDocs(Button*) {
	openDoc(FileSys::fileDocs);
}

void Program::openDoc(const char* file) const {
	string path = FileSys::docPath(file);
#ifdef _WIN32
	if (iptrt rc = iptrt(ShellExecuteW(nullptr, L"open", stow(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL)); rc <= 32)
#elif defined(__APPLE__)
	if (int rc = system(("open " + path).c_str()))
#else
	if (int rc = system(("xdg-open " + path).c_str()))
#endif
		std::cerr << "failed to open file \"" << path << "\" with code " << rc << std::endl;
}
#endif

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button*) {
	World::window()->close();
}

void Program::eventSLUpdateLE(Button* but) {
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(static_cast<Slider*>(but)->getValue()));
}

void Program::eventPrcSliderUpdate(Button* but) {
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(static_cast<Slider*>(but)->getValue()) + '%');
}

void Program::eventClearLabel(Button* but) {
	static_cast<Label*>(but)->setText(string());
}

void Program::connect(bool client, const char* msg) {
	try {
		netcp = client ? std::make_unique<Netcp>() : std::make_unique<NetcpHost>();
		netcp->connect();
		gui.openPopupMessage(msg, &Program::eventConnectCancel, "Cancel");
	} catch (const Com::Error& err) {
		netcp.reset();
		gui.openPopupMessage(err.what(), &Program::eventClosePopup);
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
		GuiGen::Text txt = gui.makeFpsText(World::window()->getDeltaSec());
		box->setSize(txt.length);
		box->setShow(true);
		state->getFpsText()->setText(std::move(txt.text));
		ftimeSleep = ftimeUpdateDelay;
	}
}

template <class T, class... A>
void Program::setState(A&&... args) {
	state = std::make_unique<T>(std::forward<A>(args)...);
	World::scene()->resetLayouts();
	ftimeSleep = ftimeUpdateDelay;	// because the frame time counter gets updated in resetLayouts
}

template<class T, class... A>
void Program::setStateWithChat(A&&... args) {
	auto [text, notif, show] = state->chatState();
	setState<T>(std::forward<A>(args)...);
	state->chatState(std::move(text), notif, false);
}

void Program::resetLayoutsWithChat() {
	auto [text, notif, show] = state->chatState();
	World::scene()->resetLayouts();
	state->chatState(std::move(text), notif, show);
	ftimeSleep = ftimeUpdateDelay;
}

tuple<BoardObject*, Piece*, svec2> Program::pickBob() {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getSelect());
	return tuple(bob, dynamic_cast<Piece*>(bob), bob ? game.board.ptog(bob->getPos()) : svec2(UINT16_MAX));
}

#ifdef EMSCRIPTEN
void Program::fetchVersionSucceed(emscripten_fetch_t* fetch) {
	pushFetchedVersion(string(static_cast<const char*>(fetch->data), fetch->numBytes), static_cast<char*>(fetch->userData));
	delete[] static_cast<char*>(fetch->userData);
	emscripten_fetch_close(fetch);
}

void Program::fetchVersionFail(emscripten_fetch_t* fetch) {
	pushEvent(UserCode::versionFetch);
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
		pushEvent(UserCode::versionFetch);
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
	if (std::smatch sm; std::regex_search(html, sm, std::regex(rver, std::regex_constants::ECMAScript | std::regex_constants::icase)) && sm.size() >= 2) {
		string latest = sm[1];
		uvec4 cur = stoiv<uvec4>(Com::commonVersion, strtoul), web = stoiv<uvec4>(latest.c_str(), strtoul);
		glm::length_t i = 0;
		for (; i < uvec4::length() - 1 && cur[i] == web[i]; ++i);
		pushEvent(UserCode::versionFetch, cur[i] >= web[i] ? new string() : new string("New version " + latest + " available"));
	} else
		pushEvent(UserCode::versionFetch);
}
