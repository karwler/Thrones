#pragma once

#include "game.h"
#include "progs.h"

// handles the frontend
class Program {
private:
	uptr<ProgState> state;
	Game game;

public:
	Program();

	void start();

	// main menu
	void eventOpenMainMenu(Button* but = nullptr);
	void eventConnectServer(Button* but = nullptr);
	void eventConnectCancel(Button* but = nullptr);
	void eventUpdateAddress(Button* but);
	void eventUpdatePort(Button* but);

	// host menu
	void eventOpenHostMenu(Button* but = nullptr);
	void eventHostServer(Button* but = nullptr);
	void eventSwitchConfig(Button* but);
	void eventConfigDeleteInput(Button* but = nullptr);
	void eventConfigDelete(Button* but = nullptr);
	void eventConfigCopyInput(Button* but = nullptr);
	void eventConfigCopy(Button* but = nullptr);
	void eventConfigNewInput(Button* but = nullptr);
	void eventConfigNew(Button* but = nullptr);
	void eventUpdateSurvivalSL(Button* but);
	void eventUpdateConfig(Button* but = nullptr);
	void eventUpdateReset(Button* but);

	// game setup
	void eventOpenSetup();
	void eventIconSelect(Button* but);
	void eventPlaceTileH();
	void eventPlaceTileD(Button* but);
	void eventPlacePieceH();
	void eventPlacePieceD(Button* but);
	void eventMoveTile(BoardObject* obj);
	void eventMovePiece(BoardObject* obj);
	void eventClearTile();
	void eventClearPiece();
	void eventSetupNext(Button* but = nullptr);
	void eventSetupBack(Button* but = nullptr);
	void eventShowWaitPopup(Button* but = nullptr);

	// game match
	void eventOpenMatch();
	void eventEndTurn(Button* but = nullptr);
	void eventPlaceDragon(Button* but = nullptr);
	void eventFavorStart(BoardObject* obj);
	void eventMove(BoardObject* obj);
	void eventFire(BoardObject* obj);
	void eventExitGame(Button* but = nullptr);

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventApplySettings(Button* but = nullptr);
	void eventSetVsync(Button* but);
	void eventSetSamples(Button* but);
	void eventSetGammaSL(Button* but);
	void eventSetGammaLE(Button* but);
	void eventSetVolumeSL(Button* but);
	void eventSetVolumeLE(Button* but);
	void eventResetSettings(Button* but);
	void eventOpenInfo(Button* but = nullptr);

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventExit(Button* but = nullptr);
	void eventSBNext(Button* but);
	void eventSBPrev(Button* but);

	ProgState* getState();
	Game* getGame();

private:
	void updateConfigWidgets(ProgHost* ph, const Com::Config& cfg);
	void placeTile(Tile* tile, uint8 type);
	void placePiece(vec2s pos, uint8 type, Piece* occupant);

	void setState(ProgState* newState);
	BoardObject* pickBob(vec2s& pos, Piece*& pce);
	Piece* extractPiece(BoardObject* bob, vec2s pos);
};

inline ProgState* Program::getState() {
	return state.get();
}

inline Game* Program::getGame() {
	return &game;
}

inline Piece* Program::extractPiece(BoardObject* bob, vec2s pos) {
	return dynamic_cast<Piece*>(bob) ? static_cast<Piece*>(bob) : game.findPiece(pos);
}
