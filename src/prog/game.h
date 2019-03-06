#include "utils/objects.h"
#include <random>

class Game {
private:
	static constexpr sizet numTiles = 36;	// amount of tiles on homeland
	static constexpr sizet numPieces = 15;	// amount of one player's pieces
	static constexpr sizet numPlains = 9;	// TODO: look up the actual amount
	static constexpr sizet numForests = 9;
	static constexpr sizet numMountains = 9;
	static constexpr sizet numWaters = 9;

	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 rcvBuf[Server::bufSiz];
	void (Game::*conn)();

	array<Tile, numTiles> tiles, enemyTiles;		// TODO: add 9 for connecting middle tiles
	array<Piece, numPieces> pieces, enemyPieces;
	Object screen;

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;

public:
	Game();
	~Game();

	bool connect();
	void disconnect();

	void tick();
	vector<Object*> initObjects();

	bool survivalCheck();

private:
	void connectionWait();
	void connectionSetup();
	void connectionMatch();

	static void createTiles(array<Tile, numTiles>& tiles);
	static void createPieces(array<Piece, numPieces>& pieces);
	static Object createScreen();

	bool connectFail();
	void disconnectMessage(const string& msg);
	void printInvalidCode() const;
};

inline bool Game::survivalCheck() {
	return randDist(randGen);
}

inline void Game::printInvalidCode() const {
	std::cerr << "invalid net code " << uint(*rcvBuf) << std::endl;
}
