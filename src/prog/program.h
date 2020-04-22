#pragma once

#include "game.h"
#include "progs.h"

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

	Info info;
	FrameTime ftimeMode;
	string curConfig;
private:
	uptr<ProgState> state;
	uptr<Netcp> netcp;
	Game game;
	SDL_Thread* proc;
	string latestVersion;
	float ftimeSleep;

	static constexpr float ftimeUpdateDelay = 0.5f;

public:
	Program();
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

	// lobby menu
	void eventOpenLobby(const uint8* data);
	void eventHostRoomInput(Button* but = nullptr);
	void eventHostRoomRequest(Button* but = nullptr);
	void eventHostRoomReceive(const uint8* data);
	void eventJoinRoomRequest(Button* but);
	void eventJoinRoomReceive(const uint8* data);
	void eventStartGame(Button* but = nullptr);
	void eventExitLobby(Button* but = nullptr);

	// room menu
	void eventOpenHostMenu(Button* but = nullptr);
	void eventHostServer(Button* but = nullptr);
	void eventSwitchConfig(sizet id, const string& str);
	void eventConfigDelete(Button* but = nullptr);
	void eventConfigCopyInput(Button* but = nullptr);
	void eventConfigCopy(Button* but = nullptr);
	void eventConfigNewInput(Button* but = nullptr);
	void eventConfigNew(Button* but = nullptr);
	void eventUpdateConfig(Button* but = nullptr);
	void eventUpdateConfigC(sizet id, const string& str);
	void eventUpdateReset(Button* but);
	void eventTileSliderUpdate(Button* but);
	void eventMiddleSliderUpdate(Button* but);
	void eventPieceSliderUpdate(Button* but);
	void eventKickPlayer(Button* but = nullptr);
	void eventPlayerHello(bool onJoin);
	void eventExitRoom(Button* but = nullptr);
	void eventSendMessage(Button* but);
	void eventRecvMessage(const uint8* data);
	void eventChatOpen(Button* but = nullptr);
	void eventChatClose(Button* but = nullptr);
	void eventToggleChat(Button* but = nullptr);
	void eventCloseChat(Button* but = nullptr);
	void eventHideChat(Button* but = nullptr);
	void eventFocusChatLabel(Button* but);

	// game setup
	void eventOpenSetup();
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
	void eventCloseConfigList(Button* but = nullptr);
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
	void eventEstablishB(Button* but = nullptr);
	void eventEstablishP(BoardObject* obj, uint8 mBut = 0);
	void eventRebuildTileB(Button* but = nullptr);
	void eventRebuildTileP(BoardObject* obj, uint8 mBut = 0);
	void eventOpenSpawner(Button* but = nullptr);
	void eventSpawnPieceB(Button* but);
	void eventSpawnPieceT(BoardObject* obj, uint8 mBut = 0);
	void eventPieceStart(BoardObject* obj, uint8 mBut = 0);
	void eventMove(BoardObject* obj, uint8 mBut);
	void eventEngage(BoardObject* obj, uint8 mBut);
	void eventPieceNoEngage(BoardObject* obj, uint8 mBut);
	void eventAbortGame(Button* but = nullptr);
	void eventSurrender(Button* but = nullptr);
	void uninitGame();
	void finishMatch(bool win);
	void eventPostFinishMatch(Button* but = nullptr);
	void eventPostDisconnectGame(Button* but = nullptr);
	void eventHostLeft(const uint8* data);
	void eventPlayerLeft();

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventSetDisplay(Button* but);
	void eventSetScreen(sizet id, const string& str);
	void eventSetWindowSize(sizet id, const string& str);
	void eventSetWindowMode(sizet id, const string& str);
	void eventSetVsync(sizet id, const string& str);
	void eventSetSamples(sizet id, const string& str);
	void eventSetTexturesScaleSL(Button* but);
	void eventSetTextureScaleLE(Button* but);
	void eventSetShadowResSL(Button* but);
	void eventSetShadowResLE(Button* but);
	void eventSetSoftShadows(Button* but);
	void eventSetGammaSL(Button* but);
	void eventSetGammaLE(Button* but);
	void eventSetVolumeSL(Button* but);
	void eventSetVolumeLE(Button* but);
	void eventSetColorAlly(sizet id, const string& str);
	void eventSetColorEnemy(sizet id, const string& str);
	void eventSetScaleTiles(Button* but);
	void eventSetScalePieces(Button* but);
	void eventSetTooltips(Button* but);
	void eventSetChatLineLimitSL(Button* but);
	void eventSetChatLineLimitLE(Button* but);
	void eventSetDeadzoneSL(Button* but);
	void eventSetDeadzoneLE(Button* but);
	void eventSetResolveFamily(sizet id, const string& str);
	void eventSetFontRegular(Button* but);
	void eventResetSettings(Button* but);
	void eventSaveSettings(Button* but = nullptr);
	void eventOpenInfo(Button* but = nullptr);

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventExit(Button* but = nullptr);
	void eventSLUpdateLE(Button* but);
	void eventPrcSliderUpdate(Button* but);
	void eventClearLabel(Button* but);
	void disconnect();
	void eventCycleFrameCounter();

	ProgState* getState();
	Netcp* getNetcp();
	Game* getGame();
	const string& getLatestVersion() const;

private:
	void sendRoomName(Com::Code code, const string& name);
	void postConfigUpdate();
	static void setConfigAmounts(uint16* amts, LabelEdit** wgts, uint8 acnt, uint16 oarea, uint16 narea, bool scale);
	static void updateAmtSliders(Slider* sl, Com::Config& cfg, uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Com::Config::*counter)() const, string (*totstr)(const Com::Config&));
	void setSaveConfig(const string& name, bool save = true);
	void popuplateSetup(Setup& setup);
	Piece* getUnplacedDragon();
	void setShadowRes(uint16 newRes);
	void setColorPieces(Settings::Color clr, Piece* pos, Piece* end);
	template <class T> void setStandardSlider(Slider* sl, T& val);
	template <class T> void setStandardSlider(LabelEdit* le, T& val);

	void connect(bool client, const char* msg);
	void setState(ProgState* newState);
	BoardObject* pickBob(svec2& pos, Piece*& pce);

#ifdef EMSCRIPTEN
	static void fetchVersionSucceed(emscripten_fetch_t* fetch);
	static void fetchVersionFail(emscripten_fetch_t* fetch);
#elif defined(WEBUTILS)
	static int fetchVersion(void* data);
	static sizet writeText(char* ptr, sizet size, sizet nmemb, void* userdata);
#endif
	static void pushFetchedVersion(const string& html, const string& rver);
};

inline ProgState* Program::getState() {
	return state.get();
}

inline Netcp* Program::getNetcp() {
	return netcp.get();
}

inline Game* Program::getGame() {
	return &game;
}

inline const string& Program::getLatestVersion() const {
	return latestVersion;
}
