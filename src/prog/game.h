#include "utils/objects.h"

struct DataBatch {
	uint8 size;		// shall not exceed data size, is never actually checked though
	uint8 data[Com::recvSize];

	DataBatch();

	void push(Com::Code code);
	void push(Com::Code code, const vector<uint8>& info);
};

inline DataBatch::DataBatch() :
	size(0)
{}

inline void DataBatch::push(Com::Code code) {
	data[size++] = uint8(code);
}

class Game {
private:
	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 (Game::*conn)(const uint8*);
	uint8 recvb[Com::recvSize];
	DataBatch sendb;

	TileCol tiles;
	PieceCol pieces;
	Object screen, board;

	struct Record {
		Piece* piece;
		bool attack, swap;

		Record(Piece* piece = nullptr, bool attack = false, bool swap = false);
	} record;	// what happened the previous turn
	bool myTurn, firstTurn;

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;

public:
	Game();
	~Game();

	bool connect();
	void disconnect();
	void tick();
	void sendSetup();

	Tile* getTile(vec2b pos);
	bool isHomeTile(Tile* til) const;
	Piece* getOwnPieces(Piece::Type type);
	Piece* findPiece(vec2b pos);
	bool isOwnPiece(Piece* pce) const;
	Object* getScreen();
	bool getMyTurn() const;

	vector<Object*> initObjects();
	void prepareMatch();
	void setOwnTilesInteract(bool on);
	void setMidTilesInteract(bool on);
	void setOwnPiecesInteract(bool on);
	string checkOwnTiles() const;	// returns empty string on success
	string checkMidTiles() const;	// ^
	string checkOwnPieces() const;	// ^
	void fillInFortress();
	void takeOutFortress();

	void pieceMove(Piece* piece, vec2b pos, Piece* occupant);
	void pieceFire(Piece* killer, vec2b pos, Piece* piece);
	void placeDragon(vec2b pos, Piece* occupant);

private:
	uint8 connectionWait(const uint8* data);
	uint8 connectionSetup(const uint8* data);
	uint8 connectionMatch(const uint8* data);

	void setScreen();
	void setBoard();
	void setMidTiles();
	static void setTiles(Tile* tiles, int8 yofs, OCall lcall, Object::Info mode);
	static void setPieces(Piece* pieces, OCall rcall, OCall ucall, Object::Info mode);
	static void setTilesInteract(Tile* tiles, sizet num, bool on);
	static void setPiecesInteract(Piece* pieces, bool on);
	static Object::Info getTileInfoInteract(Object::Info mode, bool on);
	static Object::Info getPieceInfoInteract(Object::Info mode, bool on);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);

	bool survivalCheck(Piece* piece, vec2b pos);
	bool checkMove(Piece* piece, vec2b pos, Piece* occupant, vec2b dst, bool attacking);
	bool checkMoveBySingle(vec2b pos, vec2b dst);
	bool checkMoveByArea(Piece* piece, vec2b pos, vec2b dst, uint dist);
	static bool spaceAvailible(uint8 pos, void* data);
	bool checkMoveByType(vec2b pos, vec2b dst);
	bool checkAdjacentTilesByType(uint8 pos, uint8 dst, bool* visited, Tile::Type type);
	bool checkFire(Piece* killer, vec2b pos, Piece* victim, vec2b dst);
	bool checkTilesByDistance(vec2b pos, vec2b dst, int8 dist);
	bool checkAttack(Piece* killer, Piece* victim, Tile* dtil);
	bool tryWin(Piece* piece, Piece* victim, Tile* dest);

	void prepareTurn();
	void endTurn();
	void sendData();
	void placePiece(Piece* piece, vec2b pos);	// just set the position
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool ruined);

	bool connectFail();
	void disconnectMessage(const string& msg);
	static void printInvalidCode(uint8 code);
	static uint8 posToGid(vec2b p);
	static vec2b gidToPos(uint8 i);
};

inline Tile* Game::getTile(vec2b pos) {
	return &tiles.mid[sizet(pos.y * Com::boardLength + pos.x)];
}

inline bool Game::isHomeTile(Tile* til) const {
	return til >= tiles.own;
}

inline bool Game::isOwnPiece(Piece* pce) const {
	return pce < pieces.ene;
}

inline Object* Game::getScreen() {
	return &screen;
}

inline bool Game::getMyTurn() const {
	return myTurn;
}

inline void Game::setOwnTilesInteract(bool on) {
	setTilesInteract(tiles.own, Com::numTiles, on);
}

inline void Game::setMidTilesInteract(bool on) {
	setTilesInteract(tiles.mid, Com::boardLength, on);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces.own, on);
}

inline Object::Info Game::getTileInfoInteract(Object::Info mode, bool on) {
	return on ? mode | Object::INFO_RAYCAST : mode & ~Object::INFO_RAYCAST;
}

inline Object::Info Game::getPieceInfoInteract(Object::Info mode, bool on) {
	return on ? mode | Object::INFO_SHOW | Object::INFO_RAYCAST : mode & ~(Object::INFO_SHOW | Object::INFO_RAYCAST);
}

template <class T>
void Game::setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id) {
	for (sizet i = 0; i < size; i++)
		dst[id+i] = &data[i];
	id += size;
}

inline bool Game::survivalCheck(Piece* piece, vec2b pos) {
	return piece->getType() == Piece::Type::dragon || (piece->getType() == Piece::Type::ranger && getTile(pos)->getType() == Tile::Type::mountain) || (piece->getType() == Piece::Type::spearman && getTile(pos)->getType() == Tile::Type::water) || randDist(randGen);
}

inline void Game::printInvalidCode(uint8 code) {
	std::cerr << "invalid net code " << uint(code) << std::endl;
}

inline uint8 Game::posToGid(vec2b p) {
	return uint8((p.y + Com::homeHeight) * Com::boardLength + p.x);
}

inline vec2b Game::gidToPos(uint8 i) {
	return vec2b(int8(i % Com::boardLength), int8(i / Com::boardLength) - Com::homeHeight);
}
