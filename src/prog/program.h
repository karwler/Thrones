#pragma once

#include "game.h"
#include "progs.h"

// handles the frontend
class Program {
private:
	static const string argAddress;
	static const string argPort;
	static const string argConnect;
	static const string argSetup;

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

	// game setup
	void eventOpenSetup();
	void eventPlaceTileC(BoardObject* obj);
	void eventPlaceTileD(Button* but);
	void eventPlacePieceC(BoardObject* obj);
	void eventPlacePieceD(Button* but);
	void eventMoveTile(BoardObject* obj);
	void eventMovePiece(BoardObject* obj);
	void eventClearTile(BoardObject* obj);
	void eventClearPiece(BoardObject* obj);
	void eventSetupNext(Button* but = nullptr);
	void eventSetupBack(Button* but = nullptr);
	void eventShowWaitPopup(Button* but = nullptr);

	// game match
	void eventOpenMatch();
	void eventPlaceDragon(Button* but = nullptr);
	void eventMove(BoardObject* obj);
	void eventFire(BoardObject* obj);
	void eventExitGame(Button* but = nullptr);

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventSetFullscreen(Button* but);
	void eventSetResolution(Button* but);
	void eventSetVsync(Button* but);
	void eventResetSettings(Button* but);
	void eventOpenInfo(Button* but = nullptr);

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventExit(Button* but = nullptr);
	
	ProgState* getState();
	Game* getGame();

private:
	void placeTile(Tile* tile, uint8 type);
	void placePiece(vec2b pos, uint8 type, Piece* occupant);

	void setState(ProgState* newState);
	BoardObject* pickBob(vec2b& pos, Piece*& pce);
	Piece* extractPiece(BoardObject* bob, vec2b pos);
};

inline Program::Program() :
	state(new ProgState)	// necessary as a placeholder to prevent nullptr exceptions
{}

inline ProgState* Program::getState() {
	return state.get();
}

inline Game* Program::getGame() {
	return &game;
}

inline Piece* Program::extractPiece(BoardObject* bob, vec2b pos) {
	return dynamic_cast<Piece*>(bob) ? static_cast<Piece*>(bob) : game.findPiece(pos);
}
