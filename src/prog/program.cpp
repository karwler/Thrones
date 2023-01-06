#include "board.h"
#include "netcp.h"
#include "progs.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include "utils/layouts.h"
#ifdef UPDATE_CHECK
#include <regex>
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#endif
#endif

// WEB FETCH PROC

#ifdef UPDATE_CHECK
WebFetchProc::WebFetchProc(string link, string rver) :
	url(std::move(link)),
	regex(std::move(rver))
{}

#ifndef __EMSCRIPTEN__
WebFetchProc::~WebFetchProc() {
	if (proc) {
		SDL_AtomicSet(&arun, 0);
		SDL_WaitThread(proc, nullptr);
	}
}
#endif

bool WebFetchProc::start() {
#ifdef __EMSCRIPTEN__
	constexpr char request[] = "GET";
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	std::copy_n(request, sizeof(request), attr.requestMethod);
	attr.userData = this;
	attr.onsuccess = fetchVersionSucceed;
	attr.onerror = fetchVersionFail;
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	emscripten_fetch(&attr, url.c_str());
	return true;
#else
	SDL_AtomicSet(&arun, 1);
	proc = SDL_CreateThread(fetchVersion, "", this);
	return proc;
#endif
}

#ifdef __EMSCRIPTEN__
void WebFetchProc::fetchVersionSucceed(emscripten_fetch_t* fetch) {
	static_cast<WebFetchProc*>(fetch->userData)->pushFetchedVersion(string(static_cast<const char*>(fetch->data), fetch->numBytes));
	emscripten_fetch_close(fetch);
}

void WebFetchProc::fetchVersionFail(emscripten_fetch_t* fetch) {
	WebFetchProc* wfp = static_cast<WebFetchProc*>(fetch->userData);
	wfp->error = fetch->statusText;
	pushEvent(UserCode::versionFetch, wfp);
	emscripten_fetch_close(fetch);
}

const string& WebFetchProc::finish(string& pver) {
	pver = std::move(progVersion);
	return error;
}
#else
const string& WebFetchProc::finish(string& lver, string& pver) {
	SDL_WaitThread(proc, nullptr);
	proc = nullptr;
	lver = std::move(libVersion);
	pver = std::move(progVersion);
	return error;
}

int WebFetchProc::fetchVersion(void* data) {
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);

	WebFetchProc* wfp = static_cast<WebFetchProc*>(data);
	CURL* curl = nullptr;
	void (*easy_cleanup)(CURL*) = nullptr;
#ifdef _WIN32
	HMODULE lib = LoadLibraryW(L"libcurl.dll");
	FARPROC (WINAPI* const libsym)(HMODULE, const char*) = GetProcAddress;
	string (* const liberror)() = lastErrorMessage;
	BOOL (WINAPI* const libclose)(HMODULE) = FreeLibrary;
#else
	void* lib = dlopen("libcurl.so", RTLD_NOW);
	void* (* const libsym)(void*, const char*) = dlsym;
	char* (* const liberror)() = dlerror;
	int (* const libclose)(void*) = dlclose;
	FILE* dnull = fopen("/dev/null", "w");
#endif
	int rc = EXIT_SUCCESS;

	try {
		if (!lib)
			throw std::runtime_error(liberror());
		CURL* (*easy_init)() = reinterpret_cast<CURL* (*)()>(libsym(lib, "curl_easy_init"));
		if (!easy_init)
			throw std::runtime_error(liberror());
		if (easy_cleanup = reinterpret_cast<void (*)(CURL*)>(libsym(lib, "curl_easy_cleanup")); !easy_cleanup)
			throw std::runtime_error(liberror());
		CURLcode(*easy_setopt)(CURL*, CURLoption, ...) = reinterpret_cast<CURLcode(*)(CURL*, CURLoption, ...)>(libsym(lib, "curl_easy_setopt"));
		if (!easy_setopt)
			throw std::runtime_error(liberror());
		CURLcode(*easy_perform)(CURL*) = reinterpret_cast<CURLcode(*)(CURL*)>(libsym(lib, "curl_easy_perform"));
		if (!easy_perform)
			throw std::runtime_error(liberror());
		const char* (*easy_strerror)(CURLcode) = reinterpret_cast<const char* (*)(CURLcode)>(libsym(lib, "curl_easy_strerror"));

		if (curl_version_info_data* (*version_info)(CURLversion) = reinterpret_cast<curl_version_info_data* (*)(CURLversion)>(libsym(lib, "curl_version_info"))) {
			const curl_version_info_data* cinf = version_info(CURLVERSION_NOW);
			string clver;
			if (cinf->features & CURL_VERSION_IPV6)
				clver += " IPv6,";
			if (cinf->features & CURL_VERSION_SSL)
				clver += " SSL,";
			if (cinf->features & CURL_VERSION_LIBZ)
				clver += " libz,";
#if CURL_AT_LEAST_VERSION(7, 33, 0)
			if (cinf->features & CURL_VERSION_HTTP2)
				clver += " HTTP2,";
#endif
#if CURL_AT_LEAST_VERSION(7, 66, 0)
			if (cinf->features & CURL_VERSION_HTTP3)
				clver += " HTTP3,";
#endif
#if CURL_AT_LEAST_VERSION(7, 72, 0)
			if (cinf->features & CURL_VERSION_UNICODE)
				clver += " Unicode,";
#endif
			wfp->libVersion = clver.empty() ? cinf->version : cinf->version + " with"s + string_view(clver).substr(0, clver.length() - 1);
		}

		if (SDL_AtomicGet(&wfp->arun)) {
			if (curl = easy_init(); !curl)
				throw std::runtime_error("couldn't initialize curl");
			string html;
			easy_setopt(curl, CURLOPT_URL, wfp->url.data());
			easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);
			easy_setopt(curl, CURLOPT_NOSIGNAL, 1l);
			easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeText);
			easy_setopt(curl, CURLOPT_WRITEDATA, &html);
			easy_setopt(curl, CURLOPT_NOPROGRESS, 0l);
			easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, checkProgress);
			easy_setopt(curl, CURLOPT_XFERINFODATA, wfp);
#ifndef _WIN32
			easy_setopt(curl, CURLOPT_STDERR, dnull);	// curl tries to output some bullshit
#endif
			if (CURLcode code = easy_perform(curl); code != CURLE_OK)
				throw std::runtime_error(easy_strerror ? easy_strerror(code) : "couldn't fetch page");
			wfp->pushFetchedVersion(html);
		}
	} catch (const std::runtime_error& err) {
		wfp->error = err.what();
		rc = EXIT_FAILURE;
		pushEvent(UserCode::versionFetch, wfp);
	}
#ifndef _WIN32
	if (dnull)
		fclose(dnull);
#endif
	if (curl && easy_cleanup)	// check for easy_cleanup is only here to shut MSVC up
		easy_cleanup(curl);
	if (lib)
		libclose(lib);
	return rc;
}

sizet WebFetchProc::writeText(char* ptr, sizet size, sizet nmemb, void* userdata) {
	sizet len = size * nmemb;
	static_cast<string*>(userdata)->append(ptr, len);
	return len;
}

int WebFetchProc::checkProgress(void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
	return SDL_AtomicGet(&static_cast<WebFetchProc*>(clientp)->arun) ? CURL_PROGRESSFUNC_CONTINUE : -1;
}
#endif

void WebFetchProc::pushFetchedVersion(const string& html) {
	if (std::smatch sm; std::regex_search(html, sm, std::regex(regex, std::regex_constants::ECMAScript | std::regex_constants::icase)) && sm.size() >= 2) {
		string latest = sm[1];
		uvec4 cur = toVec<uvec4>(Com::commonVersion), web = toVec<uvec4>(latest);
		glm::length_t i = 0;
		for (; i < uvec4::length() - 1 && cur[i] == web[i]; ++i);
		if (cur[i] < web[i])
			progVersion = "New version " + latest + " available";
	}
	pushEvent(UserCode::versionFetch, this);
}
#endif

// PROGRAM

Program::~Program() {
	delete netcp;
	delete state;
}

void Program::start() {
	game.board->initObjects(false, false);	// doesn't need to be here but I like having the game board in the background
	gui.initSizes();	// redundant because it happens again in eventOpenMainMenu but makeTitleBar needs it
	gui.makeTitleBar();
	eventOpenMainMenu();
#if defined(UPDATE_CHECK) && !defined(__EMSCRIPTEN__)
	if (!(World::sets()->versionLookupUrl.empty() || World::sets()->versionLookupRegex.empty()))
		if (wfproc = std::make_unique<WebFetchProc>(World::sets()->versionLookupUrl, World::sets()->versionLookupRegex); !wfproc->start())
			wfproc.reset();
#endif
}

void Program::eventUser(const SDL_UserEvent& user) {
	switch (UserCode(user.code)) {
#ifdef UPDATE_CHECK
	case UserCode::versionFetch:
		if (WebFetchProc* wfp = static_cast<WebFetchProc*>(user.data1)) {
#ifdef __EMSCRIPTEN__
			if (const string& error = wfp->finish(latestVersion); error.empty()) {
#else
			if (const string& error = wfp->finish(curlVersion, latestVersion); error.empty()) {
#endif
				if (ProgMenu* pm = dynamic_cast<ProgMenu*>(state))
					pm->versionNotif->setText(latestVersion);
			} else
				logError("version fetch failed: ", error);
			wfproc.reset();
		}
		break;
#endif
	default:
		throw std::runtime_error("Invalid user event code: " + toStr(user.code));
	}
}

void Program::tick(float dSec) {
	try {
		if (netcp)
			netcp->tick();
	} catch (const Com::Error& err) {
		dynamic_cast<ProgGame*>(state) ? showGameError(err) : showLobbyError(err);
	}

	if (ftimeMode != FrameTime::none)
		if (ftimeSleep -= dSec; ftimeSleep <= 0.f) {
			ftimeSleep = ftimeUpdateDelay;
			state->getFpsText()->setText(gui.makeFpsText(dSec));
			state->getFpsText()->findFirstInstLayout()->onResize(nullptr);
		}
}

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	info &= ~INF_HOST;
	setState<ProgMenu>();
}

void Program::eventConnectServer(Button*) {
	chatName = static_cast<ProgMenu*>(state)->pname->getText();
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

void Program::eventUpdatePlayerName(Button* but) {
	World::sets()->playerName = static_cast<LabelEdit*>(but)->getText();
	eventSaveSettings();
}

void Program::eventResetPlayerName(Button* but) {
	World::sets()->playerName.clear();
	static_cast<LabelEdit*>(but)->setText(string());
	eventSaveSettings();
}

// LOBBY MENU

void Program::eventConnLobby(const uint8* data) {
	eventOpenLobby(data + sizeof(uint16));
	if (uint16 nresp = Com::read16(data)) {
		bool acceptName = chatName.empty();
		chatName = toStr<16>(nresp);
		if (!acceptName)
			gui.openPopupChoice("Name taken. Are you okay with " + chatName + '?', &Program::eventClosePopup, &Program::eventExitLobby);
	}
}

void Program::eventOpenLobby(const uint8* data, const char* message) {
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
	gui.openPopupInput("Name:", chatName + "'s room", &Program::eventHostRoomRequest, Com::roomNameLimit);
}

void Program::eventHostRoomRequest(Button*) {
	try {
#ifdef __EMSCRIPTEN__
		if (!FileSys::canRead())
			throw "Waiting for files to sync";
#endif
		const string& name = World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
		if (static_cast<ProgLobby*>(state)->hasRoom(name))
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
#ifdef __EMSCRIPTEN__
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
		setState<ProgRoom>(game.board->config.fromComData(data + 1));
	else
		gui.openPopupMessage("Failed to join room", &Program::eventClosePopup);
}

void Program::sendRoomName(Com::Code code, const string& name) {
	vector<uint8> data(Com::dataHeadSize + 1 + name.length());
	data[0] = uint8(code);
	Com::write16(data.data() + 1, data.size());
	data[Com::dataHeadSize] = name.length();
	std::copy(name.begin(), name.end(), data.data() + Com::dataHeadSize + 1);
	netcp->sendData(data);
}

void Program::eventSendMessage(Button* but) {
	Com::Code code = dynamic_cast<ProgLobby*>(state) ? Com::Code::glmessage : Com::Code::message;
	bool inGame = dynamic_cast<ProgGame*>(state);
	const string& msg = static_cast<LabelEdit*>(but)->getOldText();
	string fullMsg = chatName + GuiGen::chatPrefix + trim(msg);
	but->getParent()->getWidget<TextBox>(but->getIndex() - 1)->addLine(fullMsg);
	try {
		if (code == Com::Code::glmessage || inGame || (info & (INF_HOST | INF_GUEST_WAITING)) != INF_HOST) {
			vector<uint8> data(Com::dataHeadSize + fullMsg.length());
			data[0] = uint8(code);
			Com::write16(data.data() + 1, data.size());
			std::copy(fullMsg.begin(), fullMsg.end(), data.begin() + Com::dataHeadSize);
			netcp->sendData(data);
		}
#ifndef NDEBUG
		if (World::args.hasFlag(Settings::argConsole) && dynamic_cast<ProgMatch*>(state) && !msg.empty() && msg[0] == '/')
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
		} else if (!state->getChat()->getParent()->sizeValid())
			state->showNotification(true);
	} else
		logInfo("unseen net message: ", msg);
}

void Program::eventExitLobby(Button*) {
	disconnect();
	eventOpenMainMenu();
}

void Program::showLobbyError(const Com::Error& err) {
	delete netcp;
	netcp = nullptr;
	gui.openPopupMessage(err.what(), &Program::eventOpenMainMenu);
}

// ROOM MENU

void Program::eventOpenHostMenu(Button*) {
#ifdef __EMSCRIPTEN__
	if (!FileSys::canRead())
		return gui.openPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
	if (ProgMenu* pm = dynamic_cast<ProgMenu*>(state))
		chatName = pm->pname->getText();
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
	FileSys::saveConfigs(static_cast<ProgRoom*>(state)->confs);
	connect(false, "Waiting for player...");
}

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
void Program::eventOpenPopupRecords(Button*) {
	gui.openPopupRecords();
}
#endif

void Program::eventDelRecord(Button* but) {
	if (const string& file = static_cast<Label*>(but)->getText(); !remove(file.c_str()))
		static_cast<InstLayout*>(but->getParent())->deleteWidgets(but->getIndex());
	else
		gui.openPopupMessage("Failed to delete file", &Program::eventClosePopup);
}

void Program::eventSwitchConfig(uint, const string& str) {
	setSaveConfig(str, false);
}

void Program::eventConfigDelete(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	vector<string> names = sortNames(pr->confs);
	vector<string>::iterator nit = std::find(names.begin(), names.end(), pr->configName->getText());
	pr->confs.erase(pr->configName->getText());
	setSaveConfig(*(nit + 1 != names.end() ? nit + 1 : nit - 1));
}

void Program::eventConfigCopyInput(Button*) {
	gui.openPopupInput("Name:", static_cast<ProgRoom*>(state)->configName->getText(), &Program::eventConfigCopy, Config::maxNameLength);
}

void Program::eventConfigCopy(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
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
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	const string& name = World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
	pr->confs.insert_or_assign(name, Config());
	setSaveConfig(name);
	eventClosePopup();
}

void Program::setSaveConfig(const string& name, bool save) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
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

void Program::eventSetConfigRecord(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	pr->confs[pr->configName->getText()].record = static_cast<CheckBox*>(but)->getOn();
	FileSys::saveConfigs(pr->confs);
}

void Program::eventSetConfigRecordName(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	pr->confs[pr->configName->getText()].recordName = static_cast<LabelEdit*>(but)->getText();
	FileSys::saveConfigs(pr->confs);
}

void Program::eventUpdateConfig(Button*) {
	ProgRoom* ph = static_cast<ProgRoom*>(state);
	Config& cfg = ph->confs[ph->configName->getText()];
	svec2 newHome = glm::clamp(svec2(toNum<int>(ph->wio.width->getText()), toNum<int>(ph->wio.height->getText())), Config::minHomeSize, Config::maxHomeSize);
	setConfigAmounts(cfg.tileAmounts.data(), ph->wio.tiles.data(), tileLim, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scaleTiles);
	setConfigAmounts(cfg.middleAmounts.data(), ph->wio.middles.data(), tileLim, cfg.homeSize.x, newHome.x, World::sets()->scaleTiles);
	setConfigAmounts(cfg.pieceAmounts.data(), ph->wio.pieces.data(), pieceLim, cfg.homeSize.x * cfg.homeSize.y, newHome.x * newHome.y, World::sets()->scalePieces);
	cfg.homeSize = newHome;
	cfg.opts = ph->wio.victoryPoints->getOn() ? cfg.opts | Config::victoryPoints : cfg.opts & ~Config::victoryPoints;
	cfg.victoryPointsNum = toNum<uint16>(ph->wio.victoryPointsNum->getText());
	cfg.opts = ph->wio.vpEquidistant->getOn() ? cfg.opts | Config::victoryPointsEquidistant : cfg.opts & ~Config::victoryPointsEquidistant;
	cfg.opts = ph->wio.ports->getOn() ? cfg.opts | Config::ports : cfg.opts & ~Config::ports;
	cfg.opts = ph->wio.rowBalancing->getOn() ? cfg.opts | Config::rowBalancing : cfg.opts & ~Config::rowBalancing;
	cfg.opts = ph->wio.homefront->getOn() ? cfg.opts | Config::homefront : cfg.opts & ~Config::homefront;
	cfg.opts = ph->wio.setPieceBattle->getOn() ? cfg.opts | Config::setPieceBattle : cfg.opts & ~Config::setPieceBattle;
	cfg.setPieceBattleNum = toNum<int>(ph->wio.setPieceBattleNum->getText());
	cfg.battlePass = toNum<int8>(ph->wio.battleLE->getText());
	cfg.opts = ph->wio.favorTotal->getOn() ? cfg.opts | Config::favorTotal : cfg.opts & ~Config::favorTotal;
	cfg.favorLimit = toNum<int>(ph->wio.favorLimit->getText());
	cfg.opts = ph->wio.firstTurnEngage->getOn() ? cfg.opts | Config::firstTurnEngage : cfg.opts & ~Config::firstTurnEngage;
	cfg.opts = ph->wio.terrainRules->getOn() ? cfg.opts | Config::terrainRules : cfg.opts & ~Config::terrainRules;
	cfg.opts = ph->wio.dragonLate->getOn() ? cfg.opts | Config::dragonLate : cfg.opts & ~Config::dragonLate;
	cfg.opts = ph->wio.dragonStraight->getOn() ? cfg.opts | Config::dragonStraight : cfg.opts & ~Config::dragonStraight;
	cfg.winThrone = toNum<int>(ph->wio.winThrone->getText());
	cfg.winFortress = toNum<int>(ph->wio.winFortress->getText());
	cfg.capturers = 0;
	for (uint8 i = 0; i < pieceLim; ++i)
		cfg.capturers |= uint16(ph->wio.capturers[i]->getSelected()) << i;
	cfg.checkValues();
	ph->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::eventUpdateConfigI(Button* but) {
	Icon* ico = static_cast<Icon*>(but);
	ico->setSelected(!ico->getSelected());
	eventUpdateConfig();
}

void Program::eventUpdateConfigV(Button* but) {
	if (World::sets()->autoVictoryPoints) {
		ProgRoom* ph = static_cast<ProgRoom*>(state);
		Config& cfg = ph->confs[ph->configName->getText()];
		uint16 tils = cfg.countTiles(), mids = cfg.countMiddles();
		const char* num;
		if (static_cast<CheckBox*>(but)->getOn()) {
			if (uint16 must = cfg.homeSize.x * cfg.homeSize.y; tils < must)
				Config::ceilAmounts(tils, must, cfg.tileAmounts.data(), cfg.tileAmounts.size() - 1);
			if (uint16 must = (cfg.homeSize.x - cfg.homeSize.x / 3) / 2; mids > must)
				Config::floorAmounts(mids, cfg.middleAmounts.data(), must, cfg.middleAmounts.size() - 1);
			num = "0";
		} else {
			if (uint16 must = cfg.homeSize.x * cfg.homeSize.y - 1; tils > must)
				Config::floorAmounts(tils, cfg.tileAmounts.data(), must, cfg.tileAmounts.size() - 1);
			if (uint16 must = (cfg.homeSize.x - cfg.homeSize.x / 9) / 2; mids < must)
				Config::ceilAmounts(mids, must, cfg.middleAmounts.data(), cfg.middleAmounts.size() - 1);
			num = "1";
		}
		ProgRoom::setAmtSliders(cfg, cfg.tileAmounts.data(), ph->wio.tiles.data(), ph->wio.tileFortress, tileLim, cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, &Config::countFreeTiles, &GuiGen::tileFortressString);
		ProgRoom::setAmtSliders(cfg, cfg.middleAmounts.data(), ph->wio.middles.data(), ph->wio.middleFortress, tileLim, 0, &Config::countFreeMiddles, &GuiGen::middleFortressString);
		ph->wio.winThrone->setText(num);
		ph->wio.winFortress->setText(num);
	}
	eventUpdateConfig();
}

void Program::eventUpdateReset(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	Config& cfg = pr->confs[pr->configName->getText()];
	cfg = Config();
	pr->updateConfigWidgets(cfg);
	postConfigUpdate();
}

void Program::postConfigUpdate() {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
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
		amts[i] = scale && narea != oarea ? uint16(std::round(float(amts[i]) * float(narea) / float(oarea))) : toNum<uint16>(wgts[i]->getText());
}

void Program::eventTileSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.tileAmounts.data(), pr->wio.tiles.data(), pr->wio.tileFortress, tileLim, cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, &Config::countFreeTiles, GuiGen::tileFortressString);
}

void Program::eventMiddleSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.middleAmounts.data(), pr->wio.middles.data(), pr->wio.middleFortress, tileLim, 0, &Config::countFreeMiddles, GuiGen::middleFortressString);
}

void Program::eventPieceSliderUpdate(Button* but) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	Config& cfg = pr->confs[pr->configName->getText()];
	updateAmtSliders(static_cast<Slider*>(but), cfg, cfg.pieceAmounts.data(), pr->wio.pieces.data(), pr->wio.pieceTotal, pieceLim, 0, &Config::countFreePieces, GuiGen::pieceTotalString);
}

void Program::eventRecvConfig(const uint8* data) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	pr->configName->set({ game.board->config.fromComData(data) }, 0);
	pr->updateConfigWidgets(game.board->config);
}

void Program::updateAmtSliders(Slider* sl, Config& cfg, uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Config::*counter)() const, string (*totstr)(const Config&)) {
	LabelEdit* le = sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1);
	amts[std::find(wgts, wgts + cnt, le) - wgts] = sl->getValue();
	le->setText(toStr(sl->getValue()));
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
	static_cast<ProgRoom*>(state)->setStartConfig();
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
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	game.sendConfig(onJoin);
	pr->updateStartButton();
	pr->toggleChatEmbedShow(true);
}

void Program::eventRoomPlayerLeft() {
	if (info & INF_HOST) {
		info &= ~Program::INF_GUEST_WAITING;
		static_cast<ProgRoom*>(state)->updateStartButton();
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
	if (chatName.empty())
		chatName = "0";
	game.sendStart();
}

void Program::eventStartUnique(const uint8* data) {
	info |= INF_UNIQ;
	if (chatName.empty())
		chatName = "1";
	game.recvStart(data);
}

void Program::eventOpenSetup(string configName) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	Config cfg = info & INF_HOST ? pr->confs[pr->configName->getText()] : game.board->config;

	netcp->setTickproc(&Netcp::tickGame);
	info &= ~INF_GUEST_WAITING;
	setStateWithChat<ProgSetup>(std::move(configName));

	ProgSetup* ps = static_cast<ProgSetup*>(state);
	ps->rcvMidBuffer.resize(game.board->config.homeSize.x);
	if (ps->piecePicksLeft = game.board->initConfig(cfg); ps->piecePicksLeft)
		gui.openPopupPiecePicker(ps->piecePicksLeft);
	else
		initSetupObjects(true);
}

void Program::eventOpenSetup(Button*) {
	ProgRoom* pr = static_cast<ProgRoom*>(state);
	game.board->config = pr->confs[pr->configName->getText()];
	game.board->ownPieceAmts = game.board->config.pieceAmounts;
	setState<ProgSetup>(pr->configName->getText());
	initSetupObjects(false);
}

void Program::initSetupObjects(bool regular) {
#ifdef NDEBUG
	game.board->initObjects(regular);
#else
	regular && World::args.hasFlag(Settings::argSetup) ? game.board->initDummyObjects() : game.board->initObjects(regular);
#endif
	static_cast<ProgSetup*>(state)->setStage(ProgSetup::Stage::tiles);
}

void Program::eventIconSelect(Button* but) {
	state->eventSetSelected(but->getIndex() - 1);
}

void Program::eventPlaceTile() {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	uint8 type = ps->getSelected();
	if (Tile* tile = dynamic_cast<Tile*>(World::scene()->getSelect()); ps->getCount(type) && tile) {
		if (tile->getType() < TileType::fortress)
			ps->incdecIcon(uint8(tile->getType()), true);

		tile->setType(TileType(type));
		tile->setInteractivity(Tile::Interact::interact);
		ps->incdecIcon(type, false);
	}
}

void Program::eventPlacePiece() {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	uint8 type = ps->getSelected();
	if (auto [bob, occupant, pos] = pickBob(); ps->getCount(type) && bob) {
		if (occupant) {
			occupant->updatePos();
			ps->incdecIcon(uint8(occupant->getType()), true);
		}

		Piece* pieces = game.board->getOwnPieces(PieceType(type));
		std::find_if(pieces, pieces + game.board->ownPieceAmts[type], [](Piece& it) -> bool { return !it.getShow(); })->updatePos(pos, true);
		ps->incdecIcon(type, false);
	}
}

void Program::eventMoveTile(BoardObject* obj, uint8) {
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->getSelect())) {
		TileType desType = dst->getType();
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
	if (Tile* til = dynamic_cast<Tile*>(World::scene()->getSelect()); til && til->getType() != TileType::empty) {
		static_cast<ProgSetup*>(state)->incdecIcon(uint8(til->getType()), true);
		til->setType(TileType::empty);
		til->setInteractivity(Tile::Interact::interact);
	}
}

void Program::eventClearPiece() {
	if (auto [bob, pce, pos] = pickBob(); pce) {
		static_cast<ProgSetup*>(state)->incdecIcon(uint8(pce->getType()), true);
		pce->updatePos();
	}
}

void Program::eventSetupNext(Button*) {
	try {
		ProgSetup* ps = static_cast<ProgSetup*>(state);
		if (netcp)		// only run checks in an actual game
			switch (ps->getStage()) {
			case ProgSetup::Stage::tiles:
				game.board->checkOwnTiles();
				break;
			case ProgSetup::Stage::middles:
				game.board->checkMidTiles();
				break;
			case ProgSetup::Stage::pieces:
				game.board->checkOwnPieces();
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
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.board->takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventOpenSetupSave(Button*) {
	openPopupSaveLoad(true);
}

void Program::eventOpenSetupLoad(Button*) {
	openPopupSaveLoad(false);
}

void Program::openPopupSaveLoad(bool save) {
#ifdef __EMSCRIPTEN__
	if (!FileSys::canRead())
		return gui.openPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	ps->setups = FileSys::loadSetups();
	gui.openPopupSaveLoad(ps->setups, save);
}

void Program::eventSetupPickPiece(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	uint8 type = but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex();
	if (++game.board->ownPieceAmts[type]; --ps->piecePicksLeft) {
		but->getParent()->getParent()->getParent()->getParent()->getWidget<Label>(0)->setText(GuiGen::msgPickPiece + toStr(ps->piecePicksLeft) + ')');
		if (game.board->config.pieceAmounts[type] == game.board->ownPieceAmts[type]) {
			Icon* ico = static_cast<Icon*>(but);
			ico->setDim(GuiGen::defaultDim);
			ico->lcall = nullptr;
		}
	} else {
		initSetupObjects(true);
		eventClosePopup();
	}
}

void Program::eventSetupNew(Button* but) {
	const string& name = (dynamic_cast<LabelEdit*>(but) ? static_cast<LabelEdit*>(but) : but->getParent()->getWidget<LabelEdit>(but->getIndex() - 1))->getText();
	populateSetup(static_cast<ProgSetup*>(state)->setups.insert_or_assign(name, Setup()).first->second);
}

void Program::eventSetupSave(Button* but) {
	umap<string, Setup>& setups = static_cast<ProgSetup*>(state)->setups;
	Setup& stp = setups.find(static_cast<Label*>(but)->getText())->second;
	stp.clear();
	populateSetup(stp);
}

void Program::populateSetup(Setup& setup) {
	for (Tile* it = game.board->getTiles().own(); it != game.board->getTiles().end(); ++it)
		if (it->getType() < TileType::fortress) {
			svec2 pos = game.board->ptog(it->getPos());
			setup.tiles.emplace_back(svec2(pos.x, pos.y - game.board->config.homeSize.y - 1), it->getType());
		}
	for (Tile* it = game.board->getTiles().mid(); it != game.board->getTiles().own(); ++it)
		if (it->getType() < TileType::fortress)
			setup.mids.emplace_back(it - game.board->getTiles().mid(), it->getType());
	for (Piece* it = game.board->getPieces().own(); it != game.board->getPieces().ene(); ++it)
		if (game.board->pieceOnBoard(it)) {
			svec2 pos = game.board->ptog(it->getPos());
			setup.pieces.emplace_back(svec2(pos.x, pos.y - game.board->config.homeSize.y - 1), it->getType());
		}
	FileSys::saveSetups(static_cast<ProgSetup*>(state)->setups);
	eventClosePopup();
}

void Program::eventSetupLoad(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	Setup& stp = ps->setups.find(static_cast<Label*>(but)->getText())->second;
	if (!stp.tiles.empty()) {
		for (Tile* it = game.board->getTiles().own(); it != game.board->getTiles().end(); ++it)
			it->setType(TileType::empty);

		array<uint16, tileLim> cnt = game.board->config.tileAmounts;
		for (auto [pos, type] : stp.tiles)
			if (pos.x < game.board->config.homeSize.x && pos.y < game.board->config.homeSize.y && cnt[uint8(type)]) {
				--cnt[uint8(type)];
				game.board->getTile(svec2(pos.x, pos.y + game.board->config.homeSize.y + 1))->setType(type);
			}
	}
	if (!stp.mids.empty()) {
		for (Tile* it = game.board->getTiles().mid(); it != game.board->getTiles().own(); ++it)
			it->setType(TileType::empty);

		array<uint16, tileLim> cnt = game.board->config.middleAmounts;
		for (auto [pos, type] : stp.mids)
			if (pos < game.board->config.homeSize.x && cnt[uint8(type)]) {
				--cnt[uint8(type)];
				game.board->getTiles().mid(pos)->setType(type);
			}
	}
	if (!stp.pieces.empty()) {
		for (Piece* it = game.board->getPieces().own(); it != game.board->getPieces().ene(); ++it)
			it->updatePos();

		array<uint16, pieceLim> cnt = game.board->ownPieceAmts;
		for (auto [pos, type] : stp.pieces)
			if (pos.x < game.board->config.homeSize.x && pos.y < game.board->config.homeSize.y && cnt[uint8(type)]) {
				--cnt[uint8(type)];
				Piece* pieces = game.board->getOwnPieces(type);
				std::find_if(pieces, pieces + game.board->ownPieceAmts[uint8(type)], [](Piece& pce) -> bool { return !pce.getShow(); })->updatePos(svec2(pos.x, pos.y + game.board->config.homeSize.y + 1), true);
			}
	}
	ps->setStage(ProgSetup::Stage::tiles);
}

void Program::eventSetupDelete(Button* but) {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	ps->setups.erase(static_cast<Label*>(but)->getText());
	FileSys::saveSetups(ps->setups);
	static_cast<InstLayout*>(but->getParent())->deleteWidgets(but->getIndex());
}

void Program::eventShowConfig(Button*) {
	if (!World::scene()->getPopup())
		gui.openPopupConfig(static_cast<ProgGame*>(state)->configName, game.board->config, state->mainScrollContent, dynamic_cast<ProgMatch*>(state));
}

void Program::eventSwitchGameButtons(Button*) {
	ProgGame* pg = static_cast<ProgGame*>(state);
	pg->bswapIcon->setSelected(!pg->bswapIcon->getSelected());
}

// GAME MATCH

void Program::eventOpenMatch() {
	ProgSetup* ps = static_cast<ProgSetup*>(state);
	game.prepareMatch(ps->rcvMidBuffer.data());
	setStateWithChat<ProgMatch>(std::move(ps->configName));
	game.prepareTurn(false);
	playGameStartAnimations();
}

void Program::playGameStartAnimations() const {
	World::scene()->addAnimation(Animation(game.board->getScreen(), std::queue<Keyframe>({ Keyframe(transAnimTime, vec3(game.board->getScreen()->getPos().x, Board::screenYDown, game.board->getScreen()->getPos().z)), Keyframe(0.f, std::nullopt, std::nullopt, vec3(0.f)) })));
#ifndef OPENVR
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(transAnimTime, Camera::posMatch, std::nullopt, Camera::latMatch) })));
	World::scene()->getCamera()->pmax = Camera::pmaxMatch;
	World::scene()->getCamera()->ymax = Camera::ymaxMatch;
#endif
}

void Program::eventEndTurn(Button*) {
	try {
		game.finishFavor(Favor::none, static_cast<ProgMatch*>(state)->favorIconSelect());	// in case assault FF has been used
		if (!game.checkPointsWin())
			game.endTurn();
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventPickFavor(Button* but) {
	uint8 fid = but->getIndex() - 1;
	if (++game.favorsCount[fid]; game.board->config.opts & Config::favorTotal)
		--game.favorsLeft[fid];

	if (--game.availableFF) {
		if (but->getParent()->getParent()->getWidget<Label>(0)->setText(GuiGen::msgFavorPick + " ("s + toStr(game.availableFF) + ')'); !game.favorsLeft[fid]) {
			but->setDim(GuiGen::defaultDim);
			but->lcall = nullptr;
		}
	} else if (eventClosePopup(); ProgMatch* pm = dynamic_cast<ProgMatch*>(state))
		pm->updateIcons();
	else
		eventSetupNext();
}

void Program::eventSelectFavor(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state);
	pm->setIcons(Favor(std::find(pm->getFavorIcons().begin(), pm->getFavorIcons().end(), but) - pm->getFavorIcons().begin()));
}

void Program::eventSwitchDestroy(Button*) {
	state->eventDestroyToggle();
}

void Program::eventKillDestroy(Button*) {
	try {
		game.changeTile(game.board->getTile(game.board->ptog(game.board->getPxpad()->getPos())), TileType::plains);
		eventCancelDestroy();
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventCancelDestroy(Button*) {
	game.board->setPxpadPos(nullptr);
	eventClosePopup();
}

void Program::eventClickPlaceDragon(Button* but) {
	static_cast<Icon*>(but)->setSelected(true);
	Tile* til = game.board->getTiles().own();
	for (; til != game.board->getTiles().end() && til->getType() != TileType::fortress; ++til);
	Piece* pce = game.board->findOccupant(til);
	World::scene()->updateSelect(pce ? static_cast<BoardObject*>(pce) : static_cast<BoardObject*>(til));
	getUnplacedDragon()->onHold(ivec2(0), SDL_BUTTON_LEFT);	// like startKeyDrag but with a different position
	state->setObjectDragPos(vec2(til->getPos().x, til->getPos().z));
}

void Program::eventHoldPlaceDragon(Button* but) {
	static_cast<Icon*>(but)->setSelected(true);
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
	Piece* pce = game.board->getOwnPieces(PieceType::dragon);
	for (; pce->getType() == PieceType::dragon && pce->getShow(); ++pce);
	pce->hgcall = nullptr;
	pce->ulcall = pce->urcall = &Program::eventPlaceDragon;
	return pce;
}

void Program::eventEstablish(Button* but) {
	ProgMatch* pm = static_cast<ProgMatch*>(state);
	if (!pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		return;

	switch (std::count_if(game.board->getOwnPieces(PieceType::throne), game.board->getPieces().ene(), [](Piece& it) -> bool { return it.getShow(); })) {
	case 0:
		gui.openPopupMessage("No "s + pieceNames[uint8(PieceType::throne)] + " for establishing", &Program::eventClosePopupResetIcons);
		break;
	case 1:
		try {
			game.establishTile(std::find_if(game.board->getOwnPieces(PieceType::throne), game.board->getPieces().ene(), [](Piece& it) -> bool { return it.getShow(); }), false);
		} catch (const string& err) {
			gui.openPopupMessage(err, &Program::eventClosePopup);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
		break;
	default:
		game.board->selectEstablishers();
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
	ProgMatch* pm = static_cast<ProgMatch*>(state);
	if (!pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		return;

	switch (std::count_if(game.board->getOwnPieces(PieceType::throne), game.board->getPieces().ene(), [this](Piece& it) -> bool { return game.board->tileRebuildable(&it); })) {
	case 0:
		gui.openPopupMessage("Can't rebuild", &Program::eventClosePopupResetIcons);
		break;
	case 1:
		try {
			game.rebuildTile(std::find_if(game.board->getOwnPieces(PieceType::throne), game.board->getPieces().ene(), [this](Piece& it) -> bool { return game.board->tileRebuildable(&it); }), false);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
		break;
	default:
		game.board->selectRebuilders();
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
	ProgMatch* pm = static_cast<ProgMatch*>(state);
	if (pm->selectHomefrontIcon(static_cast<Icon*>(but)))
		gui.openPopupSpawner();
}

void Program::eventSpawnPiece(Button* but) {
	PieceType type = PieceType(but->getParent()->getIndex() * but->getParent()->getWidgets().size() + but->getIndex());
	uint forts = std::count_if(game.board->getTiles().own(), game.board->getTiles().end(), [](const Tile& it) -> bool { return it.isUnbreachedFortress(); });
	if (type == PieceType::lancer || type == PieceType::rangers || type == PieceType::spearmen || type == PieceType::catapult || type == PieceType::elephant || forts == 1) {
		try {
			game.spawnPiece(type, game.board->findSpawnableTile(type), false);
		} catch (const Com::Error& err) {
			showGameError(err);
		}
	} else {
		static_cast<ProgMatch*>(state)->spawning = type;
		game.board->selectSpawners();
	}
	eventClosePopup();
}

void Program::eventSpawnPiece(BoardObject* obj, uint8) {
	try {
		game.spawnPiece(static_cast<ProgMatch*>(state)->spawning, static_cast<Tile*>(obj), true);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventClosePopupResetIcons(Button*) {
	static_cast<ProgMatch*>(state)->updateIcons();
	eventClosePopup();
}

void Program::eventPieceStart(BoardObject* obj, uint8 mBut) {
	ProgMatch* pm = static_cast<ProgMatch*>(state);
	Piece* pce = static_cast<Piece*>(obj);
	game.board->setPxpadPos(pm->getDestroyIcon()->getSelected() ? pce : nullptr);
	mBut == SDL_BUTTON_LEFT ? game.board->highlightMoveTiles(pce, game.getEneRec(), pm->favorIconSelect()) : game.board->highlightEngageTiles(pce);
}

void Program::eventMove(BoardObject* obj, uint8) {
	game.board->highlightMoveTiles(nullptr, game.getEneRec(), static_cast<ProgMatch*>(state)->favorIconSelect());
	try {
		if (auto [bob, pce, pos] = pickBob(); bob)
			game.pieceMove(static_cast<Piece*>(obj), pos, pce, true);
	} catch (const string& err) {
		if (game.board->setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state)->message->setText(err);
	} catch (const Com::Error& err) {
		showGameError(err);
	}
}

void Program::eventEngage(BoardObject* obj, uint8) {
	game.board->highlightEngageTiles(nullptr);
	try {
		if (auto [bob, pce, pos] = pickBob(); bob) {
			if (Piece* actor = static_cast<Piece*>(obj); actor->firingArea().first)
				game.pieceFire(actor, pos, pce);
			else
				game.pieceMove(actor, pos, pce, false);
		}
	} catch (const string& err) {
		if (game.board->setPxpadPos(nullptr); !err.empty())
			static_cast<ProgGame*>(state)->message->setText(err);
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
	game.board->uninitObjects();
	if (dynamic_cast<ProgMatch*>(state)) {
		World::scene()->addAnimation(Animation(game.board->getScreen(), std::queue<Keyframe>({ Keyframe(0.f, std::nullopt, std::nullopt, vec3(1.f)), Keyframe(transAnimTime, vec3(game.board->getScreen()->getPos().x, Board::screenYUp, game.board->getScreen()->getPos().z)) })));
#ifndef OPENVR
		World::scene()->addAnimation(Animation(World::scene()->getCamera(), std::queue<Keyframe>({ Keyframe(transAnimTime, Camera::posSetup, std::nullopt, Camera::latSetup) })));
		for (Piece& it : game.board->getPieces())
			if (it.getPos().z <= game.board->getScreen()->getPos().z && it.getPos().z >= Config::boardWidth / -2.f)
				World::scene()->addAnimation(Animation(&it, std::queue<Keyframe>({ Keyframe(transAnimTime, vec3(it.getPos().x, pieceYDown, it.getPos().z)), Keyframe(0.f, std::nullopt, std::nullopt, vec3(0.f)) })));
		World::scene()->getCamera()->pmax = Camera::pmaxSetup;
		World::scene()->getCamera()->ymax = Camera::ymaxSetup;
#endif
	}
}

void Program::finishMatch(Record::Info win) {
	if (info & INF_UNIQ)
		disconnect();
	gui.openPopupMessage(winMessage(win), &Program::eventPostFinishMatch);
}

string Program::winMessage(Record::Info win) {
	switch (win) {
	case Record::win:
		return "You win";
	case Record::loose:
		return "You lose";
	case Record::tie:
		return "You tied";
	}
	return "Invalid finish";
}

void Program::eventPostFinishMatch(Button*) {
	uninitGame();
	if (info & INF_UNIQ)
		info & INF_HOST ? eventOpenHostMenu() : eventOpenMainMenu();
	else try {
		netcp->setTickproc(&Netcp::tickLobby);
		if (info & INF_HOST) {
#ifdef __EMSCRIPTEN__
			if (!FileSys::canRead())
				return eventExitRoom();
#endif
			setStateWithChat<ProgRoom>();
		} else {
			netcp->sendData(Com::Code::hello);
			setStateWithChat<ProgRoom>(std::move(static_cast<ProgGame*>(state)->configName));
		}
		if (info & INF_GUEST_WAITING)
			eventPlayerHello(false);
	} catch (const Com::Error& err) {
		showLobbyError(err);
	}
}

void Program::eventGamePlayerLeft() {
#ifdef __EMSCRIPTEN__
	if (!FileSys::canRead())
		return showGameError(Com::Error("Player left"));
#endif
	uninitGame();
	bool host = info & INF_HOST;
	info = (info | INF_HOST) & ~INF_GUEST_WAITING;
	netcp->setTickproc(&Netcp::tickLobby);
	host ? setStateWithChat<ProgRoom>() : setStateWithChat<ProgRoom>(std::move(static_cast<ProgGame*>(state)->configName));
	gui.openPopupMessage(host ? "Player left" : "Host left", &Program::eventClosePopup);
}

void Program::showGameError(const Com::Error& err) {
	delete netcp;
	netcp = nullptr;
	uninitGame();
	gui.openPopupMessage(err.what(), (info & (INF_HOST | INF_UNIQ)) == (INF_HOST | INF_UNIQ) ? &Program::eventOpenHostMenu : &Program::eventOpenMainMenu);
}

// GAME RECORD

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
void Program::eventOpenRecord(Button* but) {
	try {
		RecConfig cfg;
		RecordReader rr(static_cast<Label*>(but)->getText(), cfg);
		setState<ProgRecord>(std::move(rr), std::move(cfg.name));
		game.board->config = cfg;
		game.board->ownPieceAmts = cfg.ownPieceAmts;
		game.board->enePieceAmts = cfg.enePieceAmts;
		game.board->initObjects(true);
		for (sizet i = 0; i < cfg.tiles.size(); ++i)
			game.board->getTiles()[i].setType(cfg.tiles[i]);
		for (sizet i = 0; i < cfg.pieces.size(); ++i) {
			Piece& pce = game.board->getPieces()[i];
			pce.setPos(game.board->gtop(cfg.pieces[i]));
			if (!game.board->pieceOnBoard(&pce))
				pce.setShow(false);
		}
		playGameStartAnimations();
	} catch (const std::runtime_error& err) {
		gui.openPopupMessage(err.what(), &Program::eventClosePopup);
	}
}

void Program::eventRecordPrevAction(Button*) {
	executeRecordAction(static_cast<ProgRecord*>(state)->reader.prevAction());
}

void Program::eventRecordNextAction(Button*) {
	executeRecordAction(static_cast<ProgRecord*>(state)->reader.nextAction());
}

void Program::executeRecordAction(const RecAction& action) {
	ProgRecord* pr = static_cast<ProgRecord*>(state);
	pr->setButtons(action.prev, action.next);
	switch (action.type) {
	case RecAction::piece:
		game.board->getPieces()[action.aid].setPos(game.board->gtop(action.loc));
		break;
	case RecAction::tile:
		game.board->getTiles()[action.aid].setType(TileType(action.loc.x));
		break;
	case RecAction::breach:
		game.board->getTiles()[action.aid].setBreached(action.loc.x);
		break;
	case RecAction::top:
		game.board->setTileTop(action.aid, inRange(action.loc, svec2(0), game.board->boardLimit()) ? game.board->getTile(action.loc) : nullptr);
		break;
	case RecAction::finish:
		pr->message->setText(winMessage(Record::Info(action.aid)));
		break;
	}
	if (!action.next && action.type != RecAction::finish)
		pr->message->setText(winMessage(Record::Info::none));
}
#endif

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState<ProgSettings>();
}

void Program::eventShowSettings(Button*) {
	if (!World::scene()->getPopup()) {
		if (!(dynamic_cast<ProgMenu*>(state) || dynamic_cast<ProgInfo*>(state)))
			gui.openPopupSettings(state->mainScrollContent, state->keyBindingsStart);
		else
			eventOpenSettings();
	}
}

void Program::eventSetDisplay(Button* but) {
	if (uint8 disp = toNum<uint8>(static_cast<LabelEdit*>(but)->getText()); disp != World::sets()->display) {
		World::sets()->display = disp;
		World::window()->setScreen();
		eventSaveSettings();
	}
}

void Program::eventSetScreen(uint id, const string&) {
	if (Settings::Screen screen = Settings::Screen(id); screen != World::sets()->screen) {
		World::sets()->screen = screen;
		World::window()->setScreen();
		eventSaveSettings();
	}
}

void Program::eventSetWindowSize(uint, const string& str) {
	if (ivec2 wsize = toVec<ivec2, uint>(str); wsize != World::sets()->size) {
		World::sets()->size = wsize;
		World::window()->setScreen();
		eventSaveSettings();
	}
}

void Program::eventSetWindowMode(uint, const string& str) {
	if (SDL_DisplayMode mode = GuiGen::fstrToDisp(str); !memcmp(&mode, &World::sets()->mode, sizeof(mode))) {
		World::sets()->mode = mode;
		World::window()->setScreen();
		eventSaveSettings();
	}
}

void Program::eventSetSamples(uint id, const string&) {
	if (Settings::AntiAliasing aa = Settings::AntiAliasing(id); aa != World::sets()->antiAliasing) {
		World::sets()->antiAliasing = aa;
		World::window()->setMultisampling();
		World::scene()->resetFrames();
		eventSaveSettings();
	}
}

void Program::eventSetAnisotropy(uint id, const string&) {
	if (Settings::Anisotropy af = Settings::Anisotropy(id); af != World::sets()->anisotropy) {
		World::sets()->anisotropy = af;
		World::window()->setSamplers();
		eventSaveSettings();
	}
}

void Program::eventSetTexturesScaleSL(Button* but) {
	if (uint8 tscale = static_cast<Slider*>(but)->getValue(); tscale != World::sets()->texScale) {
		World::sets()->texScale = tscale;
		World::scene()->reloadTextures();
		but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->texScale) + '%');
		eventSaveSettings();
	}
}

void Program::eventSetTextureScaleLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (uint8 tscale = std::clamp(toNum<uint8>(le->getText()), 1_ub, 100_ub); tscale != World::sets()->texScale) {
		World::sets()->texScale = tscale;
		World::scene()->reloadTextures();
		le->setText(toStr(World::sets()->texScale) + '%');
		but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(World::sets()->texScale);
		eventSaveSettings();
	}
}

void Program::eventUpdateShadowResSL(Button* but) {
	int val = static_cast<Slider*>(but)->getValue();
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(val >= 0 ? uint16(std::pow(2, val)) : 0));
}

void Program::eventSetShadowResSL(Button* but) {
	int val = static_cast<Slider*>(but)->getValue();
	if (uint16 sres = val >= 0 ? uint16(std::pow(2, val)) : 0; sres != World::sets()->shadowRes) {
		setShadowRes(sres);
		but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->shadowRes));
	}
}

void Program::eventSetShadowResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (uint16 sres = std::min(toNum<uint16>(le->getText()), uint16(1 << Settings::shadowBitMax)); sres != World::sets()->shadowRes) {
		setShadowRes(sres);
		le->setText(toStr(World::sets()->shadowRes));
		but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1);
	}
}

void Program::setShadowRes(uint16 newRes) {
	World::sets()->shadowRes = newRes;
	World::scene()->resetShadows();
	eventSaveSettings();
}

void Program::eventSetSoftShadows(Button* but) {
	World::sets()->softShadows = static_cast<CheckBox*>(but)->getOn();
	World::window()->setShaderOptions();
	eventSaveSettings();
}

void Program::eventSetSsao(Button* but) {
	World::sets()->ssao = static_cast<CheckBox*>(but)->getOn();
	finishFramebufferSettings();
}

void Program::eventSetBloom(Button* but) {
	World::sets()->bloom = static_cast<CheckBox*>(but)->getOn();
	finishFramebufferSettings();
}

void Program::eventSetSsr(Button* but) {
	World::sets()->ssr = static_cast<CheckBox*>(but)->getOn();
	finishFramebufferSettings();
}

void Program::finishFramebufferSettings() {
	World::window()->setShaderOptions();
	World::scene()->resetFrames();
	eventSaveSettings();
}

void Program::eventSetVsync(Button* but) {
	World::sets()->vsync = static_cast<CheckBox*>(but)->getOn();
	World::window()->setSwapInterval();
	eventSaveSettings();
}

void Program::eventSetGammaSL(Button* but) {
	if (float gamma = float(static_cast<Slider*>(but)->getValue()) / GuiGen::gammaStepFactor; gamma != World::sets()->gamma) {
		World::window()->setGamma(gamma);
		but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->gamma));
	}
}

void Program::eventSetGammaLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (float gamma = toNum<float>(le->getText()); gamma != World::sets()->gamma) {
		World::window()->setGamma(gamma);
		le->setText(toStr(World::sets()->gamma));
		but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(int(World::sets()->gamma * GuiGen::gammaStepFactor));
		eventSaveSettings();
	}
}

void Program::eventSetFovSL(Button* but) {
	if (double fov = double(static_cast<Slider*>(but)->getValue()) / GuiGen::fovStepFactor; fov != World::sets()->fov) {
		World::sets()->fov = std::clamp(fov, Settings::fovLimit.x, Settings::fovLimit.y);
#ifndef OPENVR
		World::scene()->getCamera()->setFov(float(glm::radians(World::sets()->fov)), World::window()->getScreenView());
#endif
		but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->fov));
	}
}

void Program::eventSetFovLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (double fov = toNum<double>(le->getText()); fov != World::sets()->fov) {
		World::sets()->fov = std::clamp(fov, Settings::fovLimit.x, Settings::fovLimit.y);
#ifndef OPENVR
		World::scene()->getCamera()->setFov(float(glm::radians(World::sets()->fov)), World::window()->getScreenView());
#endif
		le->setText(toStr(World::sets()->fov));
		but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(int(World::sets()->fov * GuiGen::fovStepFactor));
		eventSaveSettings();
	}
}

void Program::eventSetVolumeSL(Button* but) {
	setStandardSlider(static_cast<Slider*>(but), World::sets()->avolume);
}

void Program::eventSetVolumeLE(Button* but) {
	setStandardSlider(static_cast<LabelEdit*>(but), World::sets()->avolume);
}

void Program::eventSetColorAlly(uint id, const string&) {
	if (Settings::Color color = Settings::Color(id); color != World::sets()->colorAlly) {
		World::sets()->colorAlly = color;
		World::scene()->setPieceIcons();
		setColorPieces(color, game.board->getPieces().own(), game.board->getPieces().ene());
	}
}

void Program::eventSetColorEnemy(uint id, const string&) {
	if (Settings::Color color = Settings::Color(id); color != World::sets()->colorEnemy) {
		World::sets()->colorEnemy = color;
		setColorPieces(color, game.board->getPieces().ene(), game.board->getPieces().end());
	}
}

void Program::setColorPieces(Settings::Color clr, Piece* pos, Piece* end) {
	for (; pos != end; ++pos)
		pos->setMaterial(World::scene()->material(Settings::colorNames[uint8(clr)]));
	eventSaveSettings();
}

void Program::eventSetScaleTiles(Button* but) {
	World::sets()->scaleTiles = static_cast<CheckBox*>(but)->getOn();
	eventSaveSettings();
}

void Program::eventSetScalePieces(Button* but) {
	World::sets()->scalePieces = static_cast<CheckBox*>(but)->getOn();
	eventSaveSettings();
}

void Program::eventSetAutoVictoryPoints(Button* but) {
	World::sets()->autoVictoryPoints = static_cast<CheckBox*>(but)->getOn();
	eventSaveSettings();
}

void Program::eventSetTooltips(Button* but) {
	World::sets()->tooltips = static_cast<CheckBox*>(but)->getOn();
	World::scene()->updateTooltips();
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

void Program::eventSetResolveFamily(uint id, const string&) {
	if (Settings::Family family = Settings::Family(id); family != World::sets()->resolveFamily) {
		World::sets()->resolveFamily = family;
		eventSaveSettings();
	}
}

void Program::eventSetFont(uint, const string& str) {
	if (string font = World::fonts()->init(str, World::sets()->hinting); font != World::sets()->font) {
		World::sets()->font = std::move(font);
		World::scene()->onInternalResize();
		eventSaveSettings();
	}
}

void Program::eventSetFontHinting(uint id, const string&) {
	if (Settings::Hinting hint = Settings::Hinting(id); hint != World::sets()->hinting) {
		World::sets()->hinting = hint;
		World::fonts()->setHinting(hint);
		World::scene()->onInternalResize();
		eventSaveSettings();
	}
}

void Program::eventSetInvertWheel(Button* but) {
	World::sets()->invertWheel = static_cast<CheckBox*>(but)->getOn();
	eventSaveSettings();
}

void Program::eventAddKeyBinding(Button* but) {
	gui.openPopupKeyGetter(Binding::Type(but->getParent()->getParent()->getIndex() - state->keyBindingsStart));
}

void Program::eventSetNewBinding(Button* but) {
	KeyGetter* kg = static_cast<KeyGetter*>(but);
	InstLayout* lin = state->mainScrollContent->getWidget<InstLayout>(state->keyBindingsStart + sizet(kg->bind));
	Widget* newKg = gui.createKeyGetter(kg->accept, kg->bind, kg->kid, lin->getWidget<InstLayout>(0)->getWidget<Label>(0)->getText());
	lin->getWidget<InstLayout>(uint8(kg->accept) + 1)->insertWidgets(kg->kid, &newKg);
	lin->setSize(gui.keyGetLineSize(kg->bind));
	eventClosePopup();
	eventSaveSettings();
}

void Program::eventDelKeyBinding(Button* but) {
	KeyGetter* kg = static_cast<KeyGetter*>(but);
	switch (kg->accept) {
	case Binding::Accept::keyboard:
		World::input()->delBindingK(kg->bind, kg->kid);
		break;
	case Binding::Accept::joystick:
		World::input()->delBindingJ(kg->bind, kg->kid);
		break;
	case Binding::Accept::gamepad:
		World::input()->delBindingG(kg->bind, kg->kid);
	}
	static_cast<InstLayout*>(but->getParent())->deleteWidgets(but->getIndex());
	but->getParent()->getParent()->setSize(gui.keyGetLineSize(kg->bind));
	eventSaveSettings();
}

template <class T>
void Program::setStandardSlider(Slider* sl, T& val) {
	if (T nv = T(sl->getValue()); nv != val) {
		val = nv;
		sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1)->setText(toStr(val));
		eventSaveSettings();
	}
}

template <class T>
void Program::setStandardSlider(LabelEdit* le, T& val) {
	Slider* sl = le->getParent()->getWidget<Slider>(le->getIndex() - 1);
	if (T nv = T(std::clamp(toNum<int>(le->getText()), sl->getMin(), sl->getMax())); nv != val) {
		val = nv;
		le->setText(toStr(val));
		sl->setVal(val);
		eventSaveSettings();
	}
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

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
void Program::eventOpenRules(Button*) {
	openDoc(FileSys::docPath() + FileSys::fileRules);
}

void Program::eventOpenDocs(Button*) {
	openDoc(FileSys::docPath() + FileSys::fileDocs);
}

void Program::eventOpenFiles(Button*) {
	openDoc(FileSys::getDirConfig());
}

void Program::openDoc(const string& file) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
	if (SDL_OpenURL(("file://" + file).c_str()))
		openDocNative(file);
#else
	openDocNative(file);
#endif
}

void Program::openDocNative(const string& path) {
#ifdef _WIN32
	if (iptrt rc = iptrt(ShellExecuteW(nullptr, L"open", sstow(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL)); rc <= 32)
#elif defined(__APPLE__)
	if (int rc = system(("open " + path).c_str()))
#else
	if (int rc = system(("xdg-open " + path).c_str()))
#endif
		logError("failed to open file \"", path, "\" with code ", rc);
}
#endif

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->popPopup();
}

void Program::eventCloseScrollingPopup(Button*) {
	state->mainScrollContent = nullptr;
	eventClosePopup();
}

void Program::eventMinimize(Button*) {
	SDL_MinimizeWindow(World::window()->getWindow());
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
	if (netcp)	// shouldn't happen, but just in case
		return;
	try {
		netcp = client ? new Netcp(this) : new NetcpHost(this);
		netcp->connect(World::sets());
		gui.openPopupMessage(msg, &Program::eventConnectCancel, "Cancel");
	} catch (const Com::Error& err) {
		delete netcp;
		netcp = nullptr;
		gui.openPopupMessage(err.what(), &Program::eventClosePopup);
	}
}

void Program::disconnect() {
	if (netcp) {
		netcp->disconnect();
		delete netcp;
		netcp = nullptr;
	}
}

void Program::eventCycleFrameCounter() {
	Overlay* box = static_cast<Overlay*>(state->getFpsText()->findFirstInstLayout());
	if (ftimeMode = ftimeMode < FrameTime::seconds ? ftimeMode + 1 : FrameTime::none; ftimeMode == FrameTime::none)
		box->setShow(false);
	else {
		box->setShow(true);
		state->getFpsText()->setText(gui.makeFpsText(World::window()->getDeltaSec()));
		box->onResize(nullptr);
		ftimeSleep = ftimeUpdateDelay;
	}
}

template <class T, class... A>
void Program::setState(A&&... args) {
	delete state;
	state = new T(std::forward<A>(args)...);
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

tuple<BoardObject*, Piece*, svec2> Program::pickBob() const {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getSelect());
	return tuple(bob, dynamic_cast<Piece*>(bob), bob ? game.board->ptog(bob->getPos()) : svec2(UINT16_MAX));
}
