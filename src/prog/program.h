#pragma once

#include "game.h"
#include "progs.h"

// handles the frontend
class Program {
public:
	enum Info : uint8 {
		INF_NONE = 0,
		INF_HOST = 1,			// is host of a room
		INF_UNIQ = 2,			// is using or connected to single NetcpHost session
		INF_GUEST_WAITING = 4	// shall only be set if INF_HOST is set
	} info;
	string curConfig;
private:
	uptr<ProgState> state;
	uptr<Netcp> netcp;
	Game game;

public:
	Program();

	void start();

	// main menu
	void eventOpenMainMenu(Button* but = nullptr);
	void eventConnectServer(Button* but = nullptr);
	void eventConnectCancel(Button* but = nullptr);
	void eventUpdateAddress(Button* but);
	void eventResetAddress(Button* but);
	void eventUpdatePort(Button* but);
	void eventResetPort(Button* but);

	// lobby menu
	void eventOpenLobby(uint8* data);
	void eventHostRoomInput(Button* but = nullptr);
	void eventHostRoomRequest(Button* but = nullptr);
	void eventHostRoomReceive(uint8* data);
	void eventJoinRoomRequest(Button* but);
	void eventJoinRoomReceive(uint8* data);
	void eventStartGame(Button* but = nullptr);
	void eventExitLobby(Button* but = nullptr);

	// room menu
	void eventOpenRoom(Button* but = nullptr);
	void eventOpenHostMenu(Button* but = nullptr);
	void eventHostServer(Button* but = nullptr);
	void eventSwitchConfig(Button* but);
	void eventConfigDelete(Button* but = nullptr);
	void eventConfigCopyInput(Button* but = nullptr);
	void eventConfigCopy(Button* but = nullptr);
	void eventConfigNewInput(Button* but = nullptr);
	void eventConfigNew(Button* but = nullptr);
	void eventUpdateConfig(Button* but = nullptr);
	void eventUpdateReset(Button* but);
	void eventPrcSliderUpdate(Button* but);
	void eventTileSliderUpdate(Button* but);
	void eventMiddleSliderUpdate(Button* but);
	void eventPieceSliderUpdate(Button* but);
	void eventKickPlayer(Button* but = nullptr);
	void eventPlayerHello(bool onJoin);
	void eventExitRoom(Button* but = nullptr);

	// game setup
	void eventOpenSetup();
	void eventOpenSetup(Button* but);
	void eventIconSelect(Button* but);
	void eventPlaceTileH();
	void eventPlaceTileD(Button* but);
	void eventPlacePieceH();
	void eventPlacePieceD(Button* but);
	void eventMoveTile(BoardObject* obj, uint8 mBut);
	void eventMovePiece(BoardObject* obj, uint8 mBut);
	void eventClearTile();
	void eventClearPiece();
	void eventSetupNext(Button* but = nullptr);
	void eventSetupBack(Button* but = nullptr);
	void eventShowWaitPopup(Button* but = nullptr);
	void eventOpenSetupSave(Button* but = nullptr);
	void eventOpenSetupLoad(Button* but = nullptr);
	void eventSetupNew(Button* but);
	void eventSetupSave(Button* but);
	void eventSetupLoad(Button* but);
	void eventSetupDelete(Button* but);
	void eventShowConfig(Button* but = nullptr);
	void eventSwitchSetupButtons(Button* but = nullptr);

	// game match
	void eventOpenMatch();
	void eventEndTurn(Button* but = nullptr);
	void eventPlaceDragon(Button* but = nullptr);
	void eventSwitchFavor(Button* but = nullptr);
	void eventSwitchFavorNow(Button* but = nullptr);
	void eventFavorStart(BoardObject* obj, uint8 mBut = 0);
	void eventMove(BoardObject* obj, uint8 mBut);
	void eventFire(BoardObject* obj, uint8 mBut);
	void eventAbortGame(Button* but = nullptr);
	void uninitGame();
	void finishMatch(bool win);
	void eventPostFinishMatch(Button* but = nullptr);
	void eventPostDisconnectGame(Button* but = nullptr);
	void eventHostLeft(uint8* data);
	void eventPlayerLeft();

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventApplySettings(Button* but = nullptr);
	void eventSetVsync(Button* but);
	void eventSetSamples(Button* but);
	void eventSetTexturesScaleSL(Button* but);
	void eventSetTextureScaleLE(Button* but);
	void eventSetGammaSL(Button* but);
	void eventSetGammaLE(Button* but);
	void eventSetVolumeSL(Button* but);
	void eventSetVolumeLE(Button* but);
	void eventSetScaleTiles(Button* but);
	void eventSetScalePieces(Button* but);
	void eventResetSettings(Button* but);
	void eventSaveSettings(Button* but = nullptr);
	void eventOpenInfo(Button* but = nullptr);

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventExit(Button* but = nullptr);
	void eventSBNext(Button* but);
	void eventSBPrev(Button* but);
	void eventSLUpdateLE(Button* but);
	void eventDummy(Button* = nullptr) {}
	void disconnect();

	ProgState* getState();
	Netcp* getNetcp();
	Game* getGame();

private:
	void sendRoomName(Com::Code code, const string& name);
	void postConfigUpdate();
	static void setConfigAmounts(uint16* amts, LabelEdit** wgts, uint8 acnt, uint16 oarea, uint16 narea, bool scale);
	static void updateAmtSlider(uint16* amts, LabelEdit** wgts, uint8 cnt, Slider* sld);
	void setSaveConfig(const string& name, bool save = true);
	void placeTile(Tile* tile, uint8 type);
	void placePiece(svec2 pos, uint8 type, Piece* occupant);
	void popuplateSetup(Setup& setup);

	void connect(Netcp* net, const char* msg);
	void setState(ProgState* newState);
	BoardObject* pickBob(svec2& pos, Piece*& pce);
};
ENUM_OPERATIONS(Program::Info, uint8)

inline void Program::disconnect() {
	netcp.reset();
}

inline ProgState* Program::getState() {
	return state.get();
}

inline Netcp* Program::getNetcp() {
	return netcp.get();
}

inline Game* Program::getGame() {
	return &game;
}
