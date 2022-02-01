#pragma once

#include "game.h"
#include "guiGen.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#elif !defined(__ANDROID__)
#include <curl/curl.h>
#endif

#ifndef __ANDROID__
class WebFetchProc {
private:
	string url;
	string regex;
#ifndef __EMSCRIPTEN__
	string libVersion;
#endif
	string progVersion;
	string error;
#ifndef __EMSCRIPTEN__
	SDL_Thread* proc = nullptr;
	SDL_atomic_t arun = { 0 };
#endif

public:
	WebFetchProc(string link, string rver);
#ifndef __EMSCRIPTEN__
	~WebFetchProc();
#endif

	bool start();
#ifdef __EMSCRIPTEN__
	const string& finish(string& pver);
#else
	const string& finish(string& lver, string& pver);
#endif
private:
#ifdef __EMSCRIPTEN__
	static void fetchVersionSucceed(emscripten_fetch_t* fetch);
	static void fetchVersionFail(emscripten_fetch_t* fetch);
#else
	static int fetchVersion(void* data);
	static sizet writeText(char* ptr, sizet size, sizet nmemb, void* userdata);
	static int checkProgress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
#endif
	void pushFetchedVersion(const string& html);
};
#endif

// handles the front-end
class Program {
public:
	enum Info : uint8 {
		INF_NONE = 0,
		INF_HOST = 1,			// is host of a room
		INF_UNIQ = 2,			// is using or connected to single NetcpHost session
		INF_GUEST_WAITING = 4	// shall only be set if INF_HOST is set
	};

	enum class FrameTime : uint8 {
		none,
		frames,
		seconds
	};

	Info info = INF_NONE;
	FrameTime ftimeMode = FrameTime::none;
private:
	ProgState* state = nullptr;
	Netcp* netcp = nullptr;
	Game game;
	GuiGen gui;
#ifndef __ANDROID__
	uptr<WebFetchProc> wfproc;
#ifndef __EMSCRIPTEN__
	string curlVersion = "not found";
#endif
#endif
	string chatName;
	string latestVersion;
	float ftimeSleep = ftimeUpdateDelay;

	static constexpr float ftimeUpdateDelay = 0.5f;
	static constexpr float transAnimTime = 0.5f;
	static constexpr float pieceYDown = -2.f;

public:
	~Program();

	void start();
	void eventUser(const SDL_UserEvent& user);
	void tick(float dSec);

	// main menu
	void eventOpenMainMenu(Button* but = nullptr);
	void eventConnectServer(Button* but = nullptr);
	void eventConnectCancel(Button* but = nullptr);
	void eventUpdateAddress(Button* but);
	void eventResetAddress(Button* but);
	void eventUpdatePort(Button* but);
	void eventResetPort(Button* but);
	void eventUpdatePlayerName(Button* but);
	void eventResetPlayerName(Button* but);

	// lobby menu
	void eventConnLobby(const uint8* data);
	void eventOpenLobby(const uint8* data, const char* message = nullptr);
	void eventHostRoomInput(Button* but = nullptr);
	void eventHostRoomRequest(Button* but = nullptr);
	void eventHostRoomReceive(const uint8* data);
	void eventJoinRoomRequest(Button* but);
	void eventJoinRoomReceive(const uint8* data);
	void eventSendMessage(Button* but);
	void eventRecvMessage(const uint8* data);
	void eventExitLobby(Button* but = nullptr);

	// room menu
	void eventOpenHostMenu(Button* but = nullptr);
	void eventStartGame(Button* but = nullptr);
	void eventHostServer(Button* but = nullptr);
	void eventSwitchConfig(uint id, const string& str);
	void eventConfigDelete(Button* but = nullptr);
	void eventConfigCopyInput(Button* but = nullptr);
	void eventConfigCopy(Button* but = nullptr);
	void eventConfigNewInput(Button* but = nullptr);
	void eventConfigNew(Button* but = nullptr);
	void eventUpdateConfig(Button* but = nullptr);
	void eventUpdateConfigI(Button* but);
	void eventUpdateConfigV(Button* but);
	void eventUpdateReset(Button* but);
	void eventTileSliderUpdate(Button* but);
	void eventMiddleSliderUpdate(Button* but);
	void eventPieceSliderUpdate(Button* but);
	void eventRecvConfig(const uint8* data);
	void eventTransferHost(Button* but = nullptr);
	void eventRecvHost(bool hasGuest);
	void eventKickPlayer(Button* but = nullptr);
	void eventPlayerHello(bool onJoin);
	void eventRoomPlayerLeft();
	void eventExitRoom(Button* but = nullptr);
	void eventChatOpen(Button* but = nullptr);
	void eventChatClose(Button* but = nullptr);
	void eventToggleChat(Button* but = nullptr);
	void eventCloseChat(Button* but = nullptr);
	void eventHideChat(Button* but = nullptr);
	void eventFocusChatLabel(Button* but);

	// game setup
	void eventStartUnique();
	void eventStartUnique(const uint8* data);
	void eventOpenSetup(string configName);
	void eventOpenSetup(Button* but);
	void eventIconSelect(Button* but);
	void eventPlaceTile();
	void eventPlacePiece();
	void eventMoveTile(BoardObject* obj, uint8 mBut);
	void eventMovePiece(BoardObject* obj, uint8 mBut);
	void eventClearTile();
	void eventClearPiece();
	void eventSetupNext(Button* but = nullptr);
	void eventSetupBack(Button* but = nullptr);
	void eventOpenSetupSave(Button* but = nullptr);
	void eventOpenSetupLoad(Button* but = nullptr);
	void eventSetupPickPiece(Button* but);
	void eventSetupNew(Button* but);
	void eventSetupSave(Button* but);
	void eventSetupLoad(Button* but);
	void eventSetupDelete(Button* but);
	void eventShowConfig(Button* but = nullptr);
	void eventSwitchGameButtons(Button* but = nullptr);

	// game match
	void eventOpenMatch();
	void eventEndTurn(Button* but = nullptr);
	void eventPickFavor(Button* but = nullptr);
	void eventSelectFavor(Button* but = nullptr);
	void eventSwitchDestroy(Button* but = nullptr);
	void eventKillDestroy(Button* but = nullptr);
	void eventCancelDestroy(Button* but = nullptr);
	void eventClickPlaceDragon(Button* but);
	void eventHoldPlaceDragon(Button* but);
	void eventPlaceDragon(BoardObject* obj, uint8 mBut = 0);
	void eventEstablish(Button* but = nullptr);
	void eventEstablish(BoardObject* obj, uint8 mBut = 0);
	void eventRebuildTile(Button* but = nullptr);
	void eventRebuildTile(BoardObject* obj, uint8 mBut = 0);
	void eventOpenSpawner(Button* but = nullptr);
	void eventSpawnPiece(Button* but);
	void eventSpawnPiece(BoardObject* obj, uint8 mBut = 0);
	void eventClosePopupResetIcons(Button* but = nullptr);
	void eventPieceStart(BoardObject* obj, uint8 mBut = 0);
	void eventMove(BoardObject* obj, uint8 mBut);
	void eventEngage(BoardObject* obj, uint8 mBut);
	void eventPieceNoEngage(BoardObject* obj, uint8 mBut);
	void eventAbortGame(Button* but = nullptr);
	void eventSurrender(Button* but = nullptr);
	void uninitGame();
	void finishMatch(Record::Info win);
	void eventPostFinishMatch(Button* but = nullptr);
	void eventGamePlayerLeft();

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventShowSettings(Button* but = nullptr);
	void eventOpenShowSettings(Button* but = nullptr);
	void eventSetDisplay(Button* but);
	void eventSetScreen(uint id, const string& str);
	void eventSetWindowSize(uint id, const string& str);
	void eventSetWindowMode(uint id, const string& str);
	void eventSetVsync(uint id, const string& str);
	void eventSetSamples(uint id, const string& str);
	void eventSetTexturesScaleSL(Button* but);
	void eventSetTextureScaleLE(Button* but);
	void eventUpdateShadowResSL(Button* but);
	void eventSetShadowResSL(Button* but);
	void eventSetShadowResLE(Button* but);
	void eventSetSoftShadows(Button* but);
	void eventSetSsao(Button* but);
	void eventSetBloom(Button* but);
	void eventSetGammaSL(Button* but);
	void eventSetGammaLE(Button* but);
	void eventSetVolumeSL(Button* but);
	void eventSetVolumeLE(Button* but);
	void eventSetColorAlly(uint id, const string& str);
	void eventSetColorEnemy(uint id, const string& str);
	void eventSetScaleTiles(Button* but);
	void eventSetScalePieces(Button* but);
	void eventSetAutoVictoryPoints(Button* but);
	void eventSetTooltips(Button* but);
	void eventSetChatLineLimitSL(Button* but);
	void eventSetChatLineLimitLE(Button* but);
	void eventSetDeadzoneSL(Button* but);
	void eventSetDeadzoneLE(Button* but);
	void eventSetResolveFamily(uint id, const string& str);
	void eventSetFont(uint id, const string& str);
	void eventSetFontHinting(uint id, const string& str);
	void eventSetInvertWheel(Button* but);
	void eventAddKeyBinding(Button* but);
	void eventSetNewBinding(Button* but);
	void eventDelKeyBinding(Button* but);
	void eventResetSettings(Button* but);
	void eventSaveSettings(Button* but = nullptr);
	void eventOpenInfo(Button* but = nullptr);
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	void eventOpenRules(Button* but = nullptr);
	void eventOpenDocs(Button* but = nullptr);
#endif

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventCloseScrollingPopup(Button* but = nullptr);
	void eventMinimize(Button* but = nullptr);
	void eventExit(Button* but = nullptr);
	void eventSLUpdateLE(Button* but);
	void eventPrcSliderUpdate(Button* but);
	void eventClearLabel(Button* but);
	void disconnect();
	void eventCycleFrameCounter();

	ProgState* getState();
	template <class T> T* getState();
	Netcp* getNetcp();
	Game* getGame();
	GuiGen* getGui();
	const string& getChatName() const;
	const string& getLatestVersion() const;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	const string& getCurlVersion() const;
#endif

private:
	void sendRoomName(Com::Code code, const string& name);
	void showLobbyError(const Com::Error& err);
	void showGameError(const Com::Error& err);
	void postConfigUpdate();
	static void setConfigAmounts(uint16* amts, LabelEdit** wgts, uint8 acnt, uint16 oarea, uint16 narea, bool scale);
	static void updateAmtSliders(Slider* sl, Config& cfg, uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Config::*counter)() const, string (*totstr)(const Config&));
	void setSaveConfig(const string& name, bool save = true);
	void openPopupSaveLoad(bool save);
	void initSetupObjects(bool regular);
	void popuplateSetup(Setup& setup);
	Piece* getUnplacedDragon();
	void setShadowRes(uint16 newRes);
	void setColorPieces(Settings::Color clr, Piece* pos, Piece* end);
	template <class T> void setStandardSlider(Slider* sl, T& val);
	template <class T> void setStandardSlider(LabelEdit* le, T& val);

	void connect(bool client, const char* msg);
	template <class T, class... A> void setState(A&&... args);
	template <class T, class... A> void setStateWithChat(A&&... args);
	void resetLayoutsWithChat();
	tuple<BoardObject*, Piece*, svec2> pickBob() const;	// returns selected object, occupant, position
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	static void openDoc(const char* file);
	static void openDocNative(const string& path);
#endif
};

inline ProgState* Program::getState() {
	return state;
}

template <class T>
T* Program::getState() {
	return static_cast<T*>(state);
}

inline Netcp* Program::getNetcp() {
	return netcp;
}

inline Game* Program::getGame() {
	return &game;
}

inline GuiGen* Program::getGui() {
	return &gui;
}

inline const string& Program::getChatName() const {
	return chatName;
}

inline const string& Program::getLatestVersion() const {
	return latestVersion;
}

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
inline const string& Program::getCurlVersion() const {
	return curlVersion;
}
#endif
