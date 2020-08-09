#pragma once

#include "board.h"
#include "netcp.h"
#include "utils/objects.h"
#include <random>

// handles game logic
class Game {
public:
	Board board;
	array<uint16, Com::tileLim> favorsCount, favorsLeft;
	uint16 availableFF;
	uint16 vpOwn, vpEne;
private:
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;
	Com::Buffer sendb;

	Record ownRec, eneRec;	// what happened during this/previous turn
	bool anyFavorUsed, lastFavorUsed;
	bool myTurn, firstTurn;

public:
	Game();

	void finishSetup();
	void finishFavor(Favor next, Favor previous);
	void setNoEngage(Piece* piece);
	bool turnDone() const;
	bool getMyTurn() const;
	const Record& getOwnRec() const;
	const Record& getEneRec() const;

	void sendStart();
	void sendConfig(bool onJoin = false);
	void sendSetup();
	void recvConfig(const uint8* data);
	void recvStart(const uint8* data);
	void recvSetup(const uint8* data);
	void recvMove(const uint8* data);
	void recvKill(const uint8* data);
	void recvBreach(const uint8* data);
	void recvTile(const uint8* data);
	void recvRecord(const uint8* data);

	void pieceMove(Piece* piece, svec2 dst, Piece* occupant, bool move);
	void pieceFire(Piece* killer, svec2 dst, Piece* victim);
	void placeDragon(Piece* dragon, svec2 pos, Piece* occupant);
	void rebuildTile(Piece* throne, bool reinit);
	void establishTile(Piece* throne, bool reinit);
	void spawnPiece(Com::Piece type, Tile* tile, bool reinit);	// set tile to nullptr for auto-select (only for farm, city or single fortress)
	void prepareTurn(bool fcont);
	void endTurn();
	bool checkPointsWin();
	void surrender();
	void changeTile(Tile* tile, Com::Tile type, TileTop top = TileTop::none);

#ifdef DEBUG
	void processCommand(const char* cmd);
#endif
private:
	bool concludeAction(Piece* piece, Action action, Favor favor);	// returns false if the match ended
	void checkActionRecord(Piece* piece, Piece* occupant, Action action, Favor favor);
	void checkKiller(Piece* killer, Piece* victim, Tile* dtil, bool attack);
	void doEngage(Piece* killer, svec2 pos, svec2 dst, Piece* victim, Tile* dtil, Action action);	// return true if the killer can move to the victim's position
	bool checkWin();
	void doWin(Record::Info win);
	void placePiece(Piece* piece, svec2 pos);	// set the position and check if a favor has been gained
	void removePiece(Piece* piece);				// remove from board
	void breachTile(Tile* tile, bool yes = true);
	static string actionRecordMsg(Action action, bool self);
	static std::default_random_engine createRandomEngine();
};

inline bool Game::getMyTurn() const {
	return myTurn;
}

inline const Record& Game::getOwnRec() const {
	return ownRec;
}

inline const Record& Game::getEneRec() const {
	return eneRec;
}

inline void Game::recvBreach(const uint8* data) {
	board.getTiles()[Com::read16(data)].setBreached(data[sizeof(uint16)]);
}
