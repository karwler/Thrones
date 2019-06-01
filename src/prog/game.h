#pragma once

#include "netcp.h"
#include "utils/objects.h"

// handles game logic
class Game {
public:
	struct FavorState {
		Piece* piece;	// when dragging piece
		bool use;		// when holding down lalt

		FavorState();
	} favorState;	// favor state when dragging piece

	static const vec2 screenPosUp, screenPosDown;
	static constexpr char messageTurnGo[] = "Your turn";
	static constexpr char messageTurnWait[] = "Opponent's turn";
private:
	static constexpr char messageWin[] = "You win";
	static constexpr char messageLoose[] = "You lose";
	static const array<uint16 (*)(uint16, uint16), 4> adjacentStraight;
	static const array<uint16 (*)(uint16, uint16), 8> adjacentFull;

	uptr<Netcp> netcp;	// shall never be nullptr
	Com::Config config;
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;

	Blueprint gridat;
	Object bgrid, board, screen;
	TileCol tiles;
	PieceCol pieces;

	struct Record {
		Piece* actor;				// the moved piece/the killer
		Piece* victim;				// the attacked/restored piece	
		vec2s actorPos, victimPos;	// necessary for restoring
		bool attack, swap, restore;

		Record(Piece* actor = nullptr, Piece* victim = nullptr, vec2s actorPos = INT16_MIN, vec2s victimPos = INT16_MIN, bool attack = false, bool swap = false, bool restore = false);
	} record;	// what happened the previous turn
	bool myTurn, firstTurn;
	uint8 favorCount, favorTotal;

public:
	Game();

	Tile* getTile(vec2s pos);
	bool isHomeTile(Tile* til) const;
	bool isEnemyTile(Tile* til) const;
	Piece* getOwnPieces(Com::Piece type);
	Piece* findPiece(vec2s pos);
	bool isOwnPiece(Piece* pce) const;
	Object* getScreen();
	const Com::Config& getConfig() const;
	bool getMyTurn() const;
	uint8 getFavorCount() const;
	uint8 getFavorTotal() const;
	vec2s ptog(const vec3& p) const;
	vec3 gtop(vec2s p, float z) const;

	void connect();
	void conhost(const Com::Config& cfg);
	void disconnect();
	void disconnect(const string& msg);
	void tick();
	void processCode(Com::Code code, const uint8* data);
	void sendSetup();
	vector<Object*> initObjects();
#ifdef DEBUG
	vector<Object*> initDummyObjects();
#endif
	void uninitObjects();
	void prepareMatch();
	void setOwnTilesInteract(Tile::Interactivity lvl);
	void setMidTilesInteract(Tile::Interactivity lvl);
	void setOwnPiecesInteract(bool on);
	void setOwnPiecesVisible(bool on);
	void checkOwnTiles() const;		// throws error string on failure
	void checkMidTiles() const;		// ^
	void checkOwnPieces() const;	// ^
	vector<uint16> countOwnTiles() const;
	vector<uint16> countMidTiles() const;
	vector<uint16> countOwnPieces() const;
	void fillInFortress();
	void takeOutFortress();

	void updateFavorState();
	void pieceMove(Piece* piece, vec2s pos, Piece* occupant);
	void pieceFire(Piece* killer, vec2s pos, Piece* piece);
	void placeDragon(vec2s pos, Piece* occupant);
	void negateAttack();

private:
	void connect(Netcp* net);
	Piece* getPieces(Piece* pieces, Com::Piece type);
	void setBgrid();
	void setMidTiles();
	void setTiles(Tile* tiles, int16 yofs, Object::Info mode);
	void setPieces(Piece* pieces, OCall ucall, const Material* mat, Object::Info mode);
	static void setTilesInteract(Tile* tiles, uint16 num, Tile::Interactivity lvl);
	static void setPiecesInteract(Piece* pieces, uint16 num, bool on);
	void setPiecesVisible(Piece* pieces, bool on);
	static vector<uint16> countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void rearangeMiddle(Com::Tile* mid, Com::Tile* buf);
	static Com::Tile decompressTile(const uint8* src, uint16 i);
	Com::Tile pollFavor();
	void useFavor();

	bool checkMove(Piece* piece, vec2s pos, Piece* occupant, vec2s dst, bool attacking, Com::Tile favor);
	bool checkMoveBySingle(vec2s pos, vec2s dst, Com::Tile favor);
	bool checkMoveByArea(vec2s pos, vec2s dst, Com::Tile favor, uint16 dlim, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, uint16), uint8 movSize);
	static bool spaceAvailible(uint16 pos);
	static bool spaceAvailibleDummy(uint16 pos);
	bool checkMoveByType(vec2s pos, vec2s dst, Com::Tile favor);
	bool checkAdjacentTilesByType(uint16 pos, uint16 dst, vector<bool>& visited, Com::Tile type) const;
	bool checkFire(Piece* killer, vec2s pos, Piece* victim, vec2s dst);
	static bool checkTilesByDistance(vec2s pos, vec2s dst, int16 dist);
	bool checkAttack(Piece* killer, Piece* victim, Tile* dtil);
	bool survivalCheck(Piece* piece, vec2s spos, vec2s dpos, bool attacking, bool switching, Com::Tile favor);	// in this case "attacking" only refers to attack by movement
	void failSurvivalCheck(Piece* piece);	// pass null for soft fail
	bool checkWin();
	bool checkThroneWin(Piece* thrones);
	bool checkFortressWin();
	bool doWin(bool win);	// always returns true

	void prepareTurn();
	void endTurn();
	void placePiece(Piece* piece, vec2s pos);	// set the position and check if a favor has been gained
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool breached);

	uint16 posToId(vec2s p);
	vec2s idToPos(uint16 i);
	uint16 invertId(uint16 i);
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
	uint16 inversePieceId(Piece* piece) const;
};

inline void Game::connect() {
	connect(new Netcp);
}

inline void Game::disconnect() {
	netcp.reset();
}

inline Tile* Game::getTile(vec2s pos) {
	return tiles.mid(pos.y * config.homeWidth + pos.x);
}

inline bool Game::isHomeTile(Tile* til) const {
	return til >= tiles.own();
}

inline bool Game::isEnemyTile(Tile* til) const {
	return til < tiles.mid();
}

inline Piece* Game::getOwnPieces(Com::Piece type) {
	return getPieces(pieces.own(), type);
}

inline bool Game::isOwnPiece(Piece* pce) const {
	return pce < pieces.ene();
}

inline Object* Game::getScreen() {
	return &screen;
}

inline const Com::Config& Game::getConfig() const {
	return config;
}

inline bool Game::getMyTurn() const {
	return myTurn;
}

inline uint8 Game::getFavorCount() const {
	return favorCount;
}

inline uint8 Game::getFavorTotal() const {
	return favorTotal;
}

inline void Game::setOwnTilesInteract(Tile::Interactivity lvl) {
	setTilesInteract(tiles.own(), config.numTiles, lvl);
}

inline void Game::setMidTilesInteract(Tile::Interactivity lvl) {
	setTilesInteract(tiles.mid(), config.homeWidth, lvl);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces.own(), config.numPieces, on);
}

inline void Game::setOwnPiecesVisible(bool on) {
	setPiecesVisible(pieces.own(), on);
}

inline vector<uint16> Game::countOwnTiles() const {
	return countTiles(tiles.own(), config.numTiles, vector<uint16>(config.tileAmounts.begin(), config.tileAmounts.end() - 1));
}

inline vector<uint16> Game::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeWidth, vector<uint16>(config.middleAmounts.begin(), config.middleAmounts.end()));
}

template <class T>
void Game::setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id) {
	for (sizet i = 0; i < size; i++)
		dst[id+i] = &data[i];
	id += size;
}

inline Com::Tile Game::decompressTile(const uint8* src, uint16 i) {
	return Com::Tile((src[i/2] >> (i % 2 * 4)) & 0xF);
}

inline bool Game::checkMoveBySingle(vec2s pos, vec2s dst, Com::Tile favor) {
	return checkMoveByArea(pos, dst, favor, 1, spaceAvailibleDummy, adjacentFull.data(), uint8(adjacentFull.size()));
}

inline bool Game::spaceAvailibleDummy(uint16) {
	return true;
}

inline vec2s Game::ptog(const vec3& p) const {
	return vec2s((p.x - config.objectSize / 2.f) / config.objectSize, p.z / config.objectSize);
}

inline vec3 Game::gtop(vec2s p, float z) const {
	return vec3(p.x * config.objectSize + config.objectSize / 2.f, z, p.y * config.objectSize);
}

inline uint16 Game::posToId(vec2s p) {
	return uint16((p.y + config.homeHeight) * config.homeWidth + p.x);
}

inline vec2s Game::idToPos(uint16 i) {
	return vec2s(int16(i % config.homeWidth), int16(i / config.homeWidth) - config.homeHeight);
}

inline uint16 Game::invertId(uint16 i) {
	return config.boardSize - i - 1;
}

inline uint16 Game::tileId(const Tile* tile) const {
	return uint16(tile - tiles.begin());
}

inline uint16 Game::inverseTileId(const Tile* tile) const {
	return uint16(tiles.end() - tile - 1);
}

inline uint16 Game::inversePieceId(Piece* piece) const {
	return isOwnPiece(piece) ? uint16(piece - pieces.own()) + config.numPieces : uint16(piece - pieces.ene());
}
