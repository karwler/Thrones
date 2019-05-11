#include "utils/objects.h"

// helper for populating send buffer
struct DataBatch {
	uint8 size;		// shall not exceed data size, is never actually checked though
	uint8 data[Com::recvSize];

	DataBatch();

	void push(Com::Code code);
	void push(Com::Code code, const initializer_list<uint8>& info);
};

inline DataBatch::DataBatch() :
	size(0)
{}

inline void DataBatch::push(Com::Code code) {
	data[size++] = uint8(code);
}

// handles game logic and networking
class Game {
public:
	static const vec3 screenPosUp, screenPosDown;
	static constexpr char messageTurnGo[] = "Your turn";
	static constexpr char messageTurnWait[] = "Opponent's turn";
private:
	static constexpr char messageWin[] = "You win";
	static constexpr char messageLoose[] = "You lose";

	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 (Game::*conn)(const uint8*);
	uint8 recvb[Com::recvSize];
	DataBatch sendb;

	Object board, bgrid, screen;
	TileCol tiles;
	PieceCol pieces;

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
	bool isEnemyTile(Tile* til) const;
	Piece* getOwnPieces(Piece::Type type);
	Piece* findPiece(vec2b pos);
	bool isOwnPiece(Piece* pce) const;
	Object* getScreen();
	bool getMyTurn() const;

	vector<Object*> initObjects();
#ifdef DEBUG
	vector<Object*> initDummyObjects();
#endif
	void uninitObjects();
	void prepareMatch();
	void setOwnTilesInteract(Tile::Interactivity lvl);
	void setMidTilesInteract(bool on);
	void setOwnPiecesInteract(bool on);
	void setOwnPiecesVisible(bool on);
	void checkOwnTiles() const;		// throws error string on failure
	void checkMidTiles() const;		// ^
	void checkOwnPieces() const;	// ^
	vector<uint8> countOwnTiles() const;
	vector<uint8> countMidTiles() const;
	vector<uint8> countOwnPieces() const;
	void fillInFortress();
	void takeOutFortress();

	void pieceMove(Piece* piece, vec2b pos, Piece* occupant);
	void pieceFire(Piece* killer, vec2b pos, Piece* piece);
	void placeDragon(vec2b pos, Piece* occupant);

private:
	uint8 connectionWait(const uint8* data);
	uint8 connectionSetup(const uint8* data);
	uint8 connectionMatch(const uint8* data);

	static Piece* getPieces(Piece* pieces, Piece::Type type);
	void setScreen();
	void setBoard();
	void setBgrid();
	void setMidTiles();
	static void setTiles(Tile* tiles, int8 yofs, OCall lcall, Object::Info mode);
	static void setPieces(Piece* pieces, OCall rcall, OCall ucall, Object::Info mode, const vec4& color);
	static void setTilesInteract(Tile* tiles, sizet num, bool on);
	static void setPiecesInteract(Piece* pieces, sizet num, bool on);
	static void setPiecesVisible(Piece* pieces, bool on);
	static vector<uint8> countTiles(const Tile* tiles, sizet num, vector<uint8> cnt);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	static void rearangeMiddle(Tile::Type* mid, Tile::Type* buf, bool fwd);

	bool checkMove(Piece* piece, vec2b pos, Piece* occupant, vec2b dst, bool attacking);
	static bool checkMoveBySingle(vec2b pos, vec2b dst);
	static bool spaceAvailible(uint8 pos);
	bool checkMoveByType(vec2b pos, vec2b dst);
	bool checkAdjacentTilesByType(uint8 pos, uint8 dst, bool* visited, Tile::Type type) const;
	bool checkFire(Piece* killer, vec2b pos, Piece* victim, vec2b dst);
	static bool checkTilesByDistance(vec2b pos, vec2b dst, int8 dist);
	bool checkAttack(Piece* killer, Piece* victim, Tile* dtil);
	bool survivalCheck(Piece* piece, vec2b pos);
	void failSurvivalCheck(Piece* piece);
	bool checkWin();

	void prepareTurn();
	void endTurn();
	void sendData();
	void placePiece(Piece* piece, vec2b pos);	// just set the position
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool ruined);

	bool connectFail();
	void disconnectMessage(const string& msg);
	static void printInvalidCode(uint8 code);
	static uint8 posToId(vec2b p);
	static vec2b idToPos(uint8 i);
	static vec2b invertPos(vec2b p);
};

inline Tile* Game::getTile(vec2b pos) {
	return &tiles.mid[sizet(pos.y * Com::boardLength + pos.x)];
}

inline bool Game::isHomeTile(Tile* til) const {
	return til >= tiles.own;
}

inline bool Game::isEnemyTile(Tile* til) const {
	return til < tiles.mid;
}

inline Piece* Game::getOwnPieces(Piece::Type type) {
	return getPieces(pieces.own, type);
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

inline void Game::setMidTilesInteract(bool on) {
	setTilesInteract(tiles.mid, Com::boardLength, on);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces.own, Com::numPieces, on);
}

inline void Game::setOwnPiecesVisible(bool on) {
	setPiecesVisible(pieces.own, on);
}

inline vector<uint8> Game::countOwnTiles() const {
	return countTiles(tiles.own, Com::numTiles, vector<uint8>(Tile::amounts.begin(), Tile::amounts.end() - 1));
}

inline vector<uint8> Game::countMidTiles() const {
	return countTiles(tiles.mid, Com::boardLength, vector<uint8>(Tile::amounts.size() - 1, 1));
}

template <class T>
void Game::setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id) {
	for (sizet i = 0; i < size; i++)
		dst[id+i] = &data[i];
	id += size;
}

inline void Game::printInvalidCode(uint8 code) {
	std::cerr << "invalid net code " << uint(code) << std::endl;
}

inline uint8 Game::posToId(vec2b p) {
	return uint8((p.y + Com::homeHeight) * Com::boardLength + p.x);
}

inline vec2b Game::idToPos(uint8 i) {
	return vec2b(int8(i % Com::boardLength), int8(i / Com::boardLength) - Com::homeHeight);
}

inline vec2b Game::invertPos(vec2b p) {
	return vec2b(Com::boardLength - p.x - 1, -p.y);
}
