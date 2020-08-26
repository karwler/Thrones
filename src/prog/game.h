#pragma once

#include "board.h"
#include "server/server.h"
#include <random>

// handles game logic
class Game {
public:
	Board board;
	array<uint16, Tile::lim> favorsCount, favorsLeft;
	uint16 availableFF;
	uint16 vpOwn, vpEne;

private:
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;
	Com::Buffer sendb;

	Record ownRec, eneRec;	// what happened during this/previous turn
	bool anyFavorUsed, lastFavorUsed;
	bool miscActionTaken;
	bool myTurn, firstTurn;

public:
	Game();

	void finishSetup();
	void finishFavor(Favor next, Favor previous);
	void setNoEngage(Piece* piece);
	bool getMyTurn() const;
	const Record& getOwnRec() const;
	const Record& getEneRec() const;
	bool hasDoneAnything() const;

	void sendStart();
	void sendConfig(bool onJoin = false);
	void sendSetup();
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
	void establishTile(Piece* throne, bool reinit);
	void rebuildTile(Piece* throne, bool reinit);
	void spawnPiece(Piece::Type type, Tile* tile, bool reinit);	// set tile to nullptr for auto-select (only for farm, city or single fortress)
	void prepareTurn(bool fcont);
	void endTurn();
	bool checkPointsWin();
	void surrender();
	void changeTile(Tile* tile, Tile::Type type, TileTop top = TileTop::none);

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

#ifdef DEBUG
	Piece* readCommandPieceId(const char*& cmd);
	Piece* readCommandPiecePos(const char*& cmd);
	void readCommandPieceMove(const char*& cmd, Piece* pce, bool killOccupant);
	pair<uint16, string> readCommandTileId(const char*& cmd);
	pair<svec2, string> readCommandTilePos(const char*& cmd);
	void readCommandChange(uint16 id, const char* name);
	static svec2 readCommandVec(const char*& cmd);
	static pair<uint8, uint16> readCommandMnum(const char*& cmd, initlist<char> pref = { '+', '-' });
	static uint8 readCommandPref(const char*& cmd, initlist<char> chars = { '-' });
#endif
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

inline bool Game::hasDoneAnything() const {
	return myTurn && (!ownRec.actors.empty() || !ownRec.assault.empty() || anyFavorUsed || miscActionTaken);
}

inline void Game::recvBreach(const uint8* data) {
	board.getTiles()[Com::read16(data)].setBreached(data[sizeof(uint16)]);
}

#ifdef DEBUG
inline Piece* Game::readCommandPiecePos(const char*& cmd) {
	return board.findOccupant(readCommandVec(cmd) % board.boardLimit());
}
#endif
