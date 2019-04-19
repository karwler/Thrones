#include "utils/objects.h"

struct DataBatch {
	uint8 size;		// shall not exceed data size, is never actually checked though
	uint8 data[Server::bufSiz];

	DataBatch();

	void push(NetCode code);
	void push(NetCode code, const vector<uint8>& info);
	bool send(TCPsocket sock);
};

inline void DataBatch::push(NetCode code) {
	data[size++] = uint8(code);
}

class Game {
public:
	static constexpr int8 boardSize = 9;
	static constexpr int8 homeHeight = 4;
private:
	static constexpr uint8 numTiles = boardSize * homeHeight;	// amount of tiles on homeland (should equate to the sum of num<tile_type> + 1)
	static constexpr uint8 numPieces = 15;						// amount of one player's pieces

	struct TileCol {
		array<Tile, numTiles> ene;
		array<Tile, boardSize> mid;
		array<Tile, numTiles> own;

		Tile* begin();
		const Tile* begin() const;
		Tile* end();
		const Tile* end() const;
		constexpr sizet size() const;
	};

	struct PieceCol {
		array<Piece, numPieces> own;
		array<Piece, numPieces> ene;

		Piece* begin();
		const Piece* begin() const;
		Piece* end();
		const Piece* end() const;
		constexpr sizet size() const;
	};

	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 (Game::*conn)(const uint8*);
	uint8 recvb[Server::bufSiz];
	DataBatch sendb;

	TileCol tiles;
	PieceCol pieces;
	Object screen, board;

	struct Record {
		Piece* piece;
		bool attack, swap;

		Record(Piece* piece = nullptr, bool attack = false, bool swap = false);
	} record;	// what happened the previous turn
	bool myTurn;

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
	Piece* getOwnPieces(Piece::Type type);
	Piece* findPiece(vec2b pos);
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

	bool movePiece(Piece* piece, vec2b pos, Piece* occupant);	// returns true on end turn
	bool attackPiece(Piece* killer, vec2b pos, Piece* piece);	// ^
	bool placeDragon(vec2b pos, Piece* occupant);

private:
	uint8 connectionWait(const uint8* data);
	uint8 connectionSetup(const uint8* data);
	uint8 connectionMatch(const uint8* data);

	void setScreen();
	void setBoard();
	void setMidTiles();
	static void setTiles(array<Tile, numTiles>& tiles, int8 yofs, OCall call, Object::Info mode);
	static void setPieces(array<Piece, numPieces>& pieces, OCall call, Object::Info mode);
	static void setTilesInteract(Tile* tiles, sizet num, bool on);
	static void setPiecesInteract(array<Piece, numPieces> pieces, bool on);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void prepareTurn();

	bool survivalCheck(Piece* piece, vec2b pos);
	vector<Tile*> collectMoveTiles(Piece* piece, bool attacking);
	vector<Tile*> collectTilesBySingle(vec2b pos);
	vector<Tile*> collectTilesByArea(vec2b pos, int8 dist);
	vector<Tile*> collectTilesByType(vec2b pos);
	void appAdjacentTilesByType(uint8 pos, bool* visits, Tile**& out, Tile::Type type);
	vector<Tile*> collectAttackTiles(Piece* piece);
	vector<Tile*> collectTilesByDistance(vec2b pos, int8 dist);
	bool checkAttack(Piece* killer, Piece* victim);

	void sendData();
	void placePiece(Piece* piece, vec2b pos);	// just set the position
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool ruined);
	void updateRecord(Piece* piece, bool attack, bool swap);

	bool connectFail();
	void disconnectMessage(const string& msg);
	static void printInvalidCode(uint8 code);
	static vector<Tile*> mergeTiles(vector<Tile*> a, const vector<Tile*>& b);
};

inline Tile* Game::TileCol::begin() {
	return reinterpret_cast<Tile*>(this);
}

inline const Tile* Game::TileCol::begin() const {
	return reinterpret_cast<const Tile*>(this);
}

inline Tile* Game::TileCol::end() {
	return begin() + size();
}

inline const Tile* Game::TileCol::end() const {
	return begin() + size();
}

inline constexpr sizet Game::TileCol::size() const {
	return ene.size() + mid.size() + own.size();
}

inline Piece* Game::PieceCol::begin() {
	return reinterpret_cast<Piece*>(this);
}

inline const Piece* Game::PieceCol::begin() const {
	return reinterpret_cast<const Piece*>(this);
}

inline Piece* Game::PieceCol::end() {
	return begin() + size();
}

inline const Piece* Game::PieceCol::end() const {
	return begin() + size();
}

inline constexpr sizet Game::PieceCol::size() const {
	return own.size() + ene.size();
}

inline Tile* Game::getTile(vec2b pos) {
	return &tiles.mid[sizet(pos.y * boardSize + pos.x)];
}

inline Object* Game::getScreen() {
	return &screen;
}

inline bool Game::getMyTurn() const {
	return myTurn;
}

inline void Game::setOwnTilesInteract(bool on) {
	setTilesInteract(tiles.own.data(), tiles.own.size(), on);
}

inline void Game::setMidTilesInteract(bool on) {
	setTilesInteract(tiles.mid.data(), tiles.mid.size(), on);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces.own, on);
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
