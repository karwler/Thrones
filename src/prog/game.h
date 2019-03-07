#include "utils/objects.h"
#include <random>

class Game {
private:
	static constexpr vec2b boardSize = {9, 9};
	static constexpr int8 homeHeight = 4;
	static constexpr sizet numPlains = 11;
	static constexpr sizet numForests = 10;
	static constexpr sizet numMountains = 7;
	static constexpr sizet numWaters = 7;
	static constexpr sizet numTiles = boardSize.x * homeHeight;	// amount of tiles on homeland (should equate to the sum of num<tile_type> + 1)
	static constexpr sizet numPieces = 15;						// amount of one player's pieces

	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 rcvBuf[Server::bufSiz];
	void (Game::*conn)();

	array<Tile, numTiles> tiles, enemyTiles;
	array<Tile, boardSize.x> midTiles;
	array<Piece, numPieces> pieces, enemyPieces;
	array<Tile, boardSize.area()> board;
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

	void setBoard();
	void setScreen();
	void setMidTiles();
	static void setTiles(array<Tile, numTiles>& tiles);
	static void setPieces(array<Piece, numPieces>& pieces);
	template <class T, sizet L> static void setObjectAddrs(array<T, L>& src, vector<Object*>& dst, sizet& i);
	void receiveSetup();

	bool connectFail();
	void disconnectMessage(const string& msg);
	void printInvalidCode() const;
};

inline bool Game::survivalCheck() {
	return randDist(randGen);
}

template <class T, sizet L>
void Game::setObjectAddrs(array<T, L>& src, vector<Object*>& dst, sizet& i) {
	for (T& it : src)
		dst[i++] = &it;
}

inline void Game::printInvalidCode() const {
	std::cerr << "invalid net code " << uint(*rcvBuf) << std::endl;
}
