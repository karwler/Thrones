#include "utils/objects.h"

class Game {
public:
	static constexpr vec2b boardSize = {9, 9};
	static constexpr int8 homeHeight = 4;
private:
	static constexpr sizet numTiles = boardSize.x * homeHeight;	// amount of tiles on homeland (should equate to the sum of num<tile_type> + 1)
	static constexpr sizet numPieces = 15;						// amount of one player's pieces

	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 rcvBuf[Server::bufSiz];
	void (Game::*conn)();

	array<Tile, numTiles> tiles, enemyTiles;
	array<Tile, boardSize.x> midTiles;
	array<Piece, numPieces> pieces, enemyPieces;
	Object screen, board;

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;

public:
	Game();
	~Game();

	bool connect();
	void disconnect();
	void tick();
	void sendSetup() const;

	vector<Object*> initObjects();
	Piece* getPieces(Piece::Type type);
	Piece* findPiece(vec2b pos);
	Object* getScreen();
	void setOwnTiles();
	void setMidTiles();
	void setOwnTilesInteract(bool on);
	void setMidTilesInteract(bool on);
	void setOwnPiecesInteract(bool on);
	void prepareMatch();
	string checkOwnTiles() const;	// returns empty string on success
	string checkMidTiles() const;	// ^
	string checkOwnPieces() const;	// ^
	void fillInFortress();
	void takeOutFortress();

	void killPiece(Piece* piece);
	bool survivalCheck();

private:
	void connectionWait();
	void connectionSetup();
	void connectionMatch();

	void setScreen();
	void setBoard();
	static void setTiles(array<Tile, numTiles>& tiles, int8 yofs, OCall call, Object::Info mode);
	static void setPieces(array<Piece, numPieces>& pieces, OCall call, Object::Info mode);
	static void setTilesInteract(Tile* tiles, sizet num, bool on);
	static void setPiecesInteract(array<Piece, numPieces> pieces, bool on);
	static void bufferizeTilesType(const Tile* tiles, sizet num, uint8* dst, sizet& di);
	template <class T, sizet L> static void setObjectAddrs(array<T, L>& src, vector<Object*>& dst, sizet& i);
	void receiveSetup();

	bool connectFail();
	void disconnectMessage(const string& msg);
	void printInvalidCode() const;
};

inline Piece* Game::getPieces(Piece::Type type) {
	return pieces.data() + Piece::amounts[uint8(type)];
}

inline Object* Game::getScreen() {
	return &screen;
}

inline void Game::setOwnTilesInteract(bool on) {
	setTilesInteract(tiles.data(), tiles.size(), on);
}

inline void Game::setMidTilesInteract(bool on) {
	setTilesInteract(midTiles.data(), midTiles.size(), on);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces, on);
}

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
