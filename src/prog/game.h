#pragma once

#include "netcp.h"
#include "utils/objects.h"
#include <random>

enum Action : uint8 {
	ACT_NONE  = 0x00,
	ACT_MOVE  = 0x01,	// regular position change
	ACT_SWAP  = 0x02,	// actor switching with swapper
	ACT_ATCK  = 0x04,	// movement attack
	ACT_FIRE  = 0x08,	// firing attack
	ACT_AFAIL = 0x10,	// survival check failed during an attack
	ACT_FCONT = 0x20,	// continue original turn after a failed attack
	ACT_WIN   = 0x40,	// actor won
	ACT_LOOSE = 0x80,	// actor lost
	ACT_MS    = ACT_MOVE | ACT_SWAP,
	ACT_AF    = ACT_ATCK | ACT_FIRE
};
ENUM_OPERATIONS(Action, uint8)

class Record {
public:
	Piece* actor;	// the moved piece/the killer
	Piece* protect;	// protected piece
	Action action;	// action taken by actor

	Record(Piece* actor = nullptr, Piece* protect = nullptr, Action action = ACT_NONE);

	void update(Piece* doActor, Action didActions);
	void setProtectionDim() const;
};

// handles game logic
class Game {
public:
	static constexpr float screenYUp = 0.f;
	static constexpr float screenYDown = -4.2f;
private:
	static const array<uint16 (*)(uint16, svec2), 4> adjacentStraight;
	static const array<uint16 (*)(uint16, svec2), 8> adjacentFull;

	Com::Config config;
	uint16 boardHeight;	// total number of rows
	float objectSize;	// width and height of a tile/piece
	vec2 tilesOffset;	// top left corner
	vec2 bobOffset;		// offset for gtop
	vec4 boardBounds;

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;
	Com::Buffer sendb;

	Mesh gridat;
	Object ground, board, bgrid, screen, ffpad;
	TileCol tiles;
	PieceCol pieces;

	Record ownRec, eneRec;	// what happened the previous turn
	bool myTurn, firstTurn;
	uint8 favorCount, favorTotal;

public:
	Game();
	~Game();

	const Com::Config& getConfig();
	TileCol& getTiles();
	Tile* getTile(svec2 pos);
	bool isHomeTile(const Tile* til) const;
	bool isEnemyTile(const Tile* til) const;
	PieceCol& getPieces();
	Piece* getOwnPieces(Com::Piece type);
	bool isOwnPiece(const Piece* pce) const;
	bool isEnemyPiece(const Piece* pce) const;
	bool pieceOnBoard(const Piece* piece) const;
	bool pieceOnHome(const Piece* piece) const;
	Object* getScreen();
	void setFfpadPos(bool force = false, svec2 pos = svec2(UINT16_MAX));
	Com::Tile checkFavor();
	Com::Tile checkFavor(Piece* piece, Tile* dtil, Action action);	// checkFavor with possible change to mountain FF
	bool getMyTurn() const;
	uint8 getFavorCount() const;
	uint8 getFavorTotal() const;
	const vec4& getBoardBounds() const;
	svec2 ptog(const vec3& p) const;
	vec3 gtop(svec2 p, float z = 0.f) const;
	uint16 posToId(svec2 p) const;

	void sendStart();
	void sendConfig(bool onJoin = false);
	void sendSetup();
	void recvConfig(uint8* data);
	void recvStart(uint8* data);
	void recvSetup(uint8* data);
	void recvMove(uint8* data);
	void recvKill(uint8* data);
	void recvBreach(uint8* data);
	void recvRecord(uint8* data);
	vector<Object*> initObjects(const Com::Config& cfg);
#ifdef DEBUG
	vector<Object*> initDummyObjects(const Com::Config& cfg);
#endif
	void uninitObjects();
	void prepareMatch();
	void setOwnTilesInteract(Tile::Interact lvl, bool dim = false);
	void setMidTilesInteract(Tile::Interact lvl, bool dim = false);
	void setOwnPiecesVisible(bool on);
	void disableOwnPiecesInteract(bool rigid, bool dim = false);
	void checkOwnTiles() const;		// throws error string on failure
	void checkMidTiles() const;		// ^
	void checkOwnPieces() const;	// ^
	void highlightMoveTiles(Piece* pce);	// nullptr to disable
	void highlightFireTiles(Piece* pce);	// ^
	vector<uint16> countOwnTiles() const;
	vector<uint16> countMidTiles() const;
	vector<uint16> countOwnPieces() const;
	void fillInFortress();
	void takeOutFortress();

	void pieceMove(Piece* piece, svec2 pos, Piece* occupant, bool forceSwitch);
	void pieceFire(Piece* killer, svec2 pos, Piece* piece);
	void placeDragon(svec2 pos, Piece* occupant);
	void prepareTurn();
	void endTurn();

private:
	void updateConfigValues();
	Piece* getPieces(Piece* pieces, Com::Piece type);
	Piece* findPiece(Piece* beg, Piece* end, svec2 pos);
	void setBgrid();
	void setMidTiles();
	void setTiles(Tile* tils, uint16 yofs, bool inter);
	void setPieces(Piece* pces, float rot, const Material* matl);
	static void setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim = false);
	static void setPieceInteract(Piece* piece, bool on, bool dim, GCall hgcall, GCall ulcall, GCall urcall);
	static vector<uint16> countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void rearangeMiddle(Com::Tile* mid, Com::Tile* buf);
	uint16 tileCompressionSize() const;
	uint8 compressTile(uint16 e) const;
	static Com::Tile decompressTile(const uint8* src, uint16 i);

	void concludeAction(Action last, FavorAct fact, Com::Tile favor);
	void checkMove(Piece* piece, svec2 pos, Piece* occupant, svec2 dst, Action action, FavorAct fact, Com::Tile favor);
	template <class F, class... A> void checkMoveDestination(svec2 pos, svec2 dst, F check, A... args);
	void collectTilesBySingle(uset<uint16>& tcol, uint16 pos);
	void collectTilesByStraight(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, svec2), uint8 movSize);
	void collectTilesByArea(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, svec2), uint8 movSize);
	void collectTilesByType(uset<uint16>& tcol, uint16 pos, bool (*stepable)(uint16));
	void collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, Com::Tile type, bool (*stepable)(uint16));
	static bool spaceAvailibleAny(uint16 pos);
	static bool spaceAvailibleSpearman(uint16 pos);
	static bool spaceAvailibleLancer(uint16 pos);
	static bool spaceAvailibleDragon(uint16 pos);
	void checkFire(Piece* killer, svec2 pos, Piece* victim, svec2 dst, FavorAct fact, Com::Tile favor);
	uset<uint16> collectTilesByDistance(svec2 pos, uint16 dist);
	bool checkFireLine(svec2& pos, svec2 mov, uint16 dist);
	static bool canForestFF(FavorAct fact, Com::Tile favor, Tile* src);
	bool canForestFF(FavorAct fact, Com::Tile favor, Tile* src, Tile* dst, Piece* occ) const;
	void checkFavorAction(Tile* src, Tile* dst, Piece* occupant, Action action, FavorAct fact, Com::Tile favor);
	void checkAttack(Piece* killer, Piece* victim, Tile* dtil) const;
	bool survivalCheck(Piece* piece, Piece* occupant, Tile* stil, Tile* dtil, Action action, FavorAct fact, Com::Tile favor);
	bool runSurvivalCheck();
	void failSurvivalCheck(Piece* piece, Action action);
	bool checkWin();
	bool checkThroneWin(Piece* thrones);
	bool checkFortressWin();
	void doWin(bool win);
	void placePiece(Piece* piece, svec2 pos);	// set the position and check if a favor has been gained
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool breached);

	svec2 idToPos(uint16 i) const;
	uint16 invertId(uint16 i) const;
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
	uint16 inversePieceId(Piece* piece) const;
	string pastRecordAction() const;
	static std::default_random_engine createRandomEngine();
};

inline Game::~Game() {
	gridat.free();
}

inline const Com::Config& Game::getConfig() {
	return config;
}

inline TileCol& Game::getTiles() {
	return tiles;
}

inline Tile* Game::getTile(svec2 pos) {
	return &tiles[pos.y * config.homeSize.x + pos.x];
}

inline bool Game::isHomeTile(const Tile* til) const {
	return til >= tiles.own();
}

inline bool Game::isEnemyTile(const Tile* til) const {
	return til < tiles.mid();
}

inline PieceCol& Game::getPieces() {
	return pieces;
}

inline Piece* Game::getOwnPieces(Com::Piece type) {
	return getPieces(pieces.own(), type);
}

inline bool Game::isOwnPiece(const Piece* pce) const {
	return pce < pieces.ene();
}

inline bool Game::isEnemyPiece(const Piece* pce) const {
	return pce >= pieces.ene();
}

inline bool Game::pieceOnBoard(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0), svec2(config.homeSize.x, boardHeight));
}

inline bool Game::pieceOnHome(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0, config.homeSize.y + 1), svec2(config.homeSize.x, boardHeight));
}

inline Object* Game::getScreen() {
	return &screen;
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

inline const vec4& Game::getBoardBounds() const {
	return boardBounds;
}

inline void Game::setOwnTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.own(), tiles.getHome(), lvl, dim);
}

inline void Game::setMidTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.mid(), config.homeSize.x, lvl, dim);
}

inline vector<uint16> Game::countOwnTiles() const {
	return countTiles(tiles.own(), tiles.getHome(), vector<uint16>(config.tileAmounts.begin(), config.tileAmounts.end()));
}

inline vector<uint16> Game::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeSize.x, vector<uint16>(config.middleAmounts.begin(), config.middleAmounts.end()));
}

inline uint16 Game::tileCompressionSize() const {
	return tiles.getExtra() / 2 + tiles.getExtra() % 2;
}

inline uint8 Game::compressTile(uint16 i) const {
	return uint8(uint8(tiles[tiles.getSize()-i-1].getType()) << (i % 2 * 4));
}

inline Com::Tile Game::decompressTile(const uint8* src, uint16 i) {
	return Com::Tile((src[i/2] >> (i % 2 * 4)) & 0xF);
}

inline void Game::collectTilesBySingle(uset<uint16>& tcol, uint16 pos) {
	return collectTilesByStraight(tcol, pos, 1, spaceAvailibleAny, adjacentFull.data(), uint8(adjacentFull.size()));
}

inline bool Game::spaceAvailibleAny(uint16) {
	return true;
}

inline bool Game::canForestFF(FavorAct fact, Com::Tile favor, Tile* src) {
	return fact == FavorAct::now && favor == Com::Tile::forest && src->getType() == Com::Tile::forest;
}

inline bool Game::canForestFF(FavorAct fact, Com::Tile favor, Tile* src, Tile* dst, Piece* occ) const {
	return canForestFF(fact, favor, src) && dst->getType() == Com::Tile::forest && isOwnPiece(occ);
}

inline bool Game::runSurvivalCheck() {
	return randDist(randGen) < config.survivalPass;
}

inline void Game::recvMove(uint8* data) {
	pieces[SDLNet_Read16(data)].updatePos(idToPos(SDLNet_Read16(data + sizeof(uint16))), true);
}

inline void Game::recvKill(uint8* data) {
	pieces[SDLNet_Read16(data)].updatePos();
}

inline void Game::recvBreach(uint8* data) {
	tiles[SDLNet_Read16(data + 1)].setBreached(data[0]);
}

inline svec2 Game::ptog(const vec3& p) const {
	return svec2((vec2(p.x, p.z) - tilesOffset) / objectSize);
}

inline vec3 Game::gtop(svec2 p, float z) const {
	return vec3(float(p.x) * objectSize + bobOffset.x, z, float(p.y) * objectSize + bobOffset.y);
}

inline uint16 Game::posToId(svec2 p) const {
	return uint16(p.y * config.homeSize.x + p.x);
}

inline svec2 Game::idToPos(uint16 i) const {
	return svec2(i % config.homeSize.x, i / config.homeSize.x);
}

inline uint16 Game::invertId(uint16 i) const {
	return tiles.getSize() - i - 1;
}

inline uint16 Game::tileId(const Tile* tile) const {
	return uint16(tile - tiles.begin());
}

inline uint16 Game::inverseTileId(const Tile* tile) const {
	return uint16(tiles.end() - tile - 1);
}

inline uint16 Game::inversePieceId(Piece* piece) const {
	return piece ? isOwnPiece(piece) ? uint16(piece - pieces.own()) + pieces.getNum() : uint16(piece - pieces.ene()) : UINT16_MAX;
}
