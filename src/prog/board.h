#pragma once

#include "types.h"

// game board
class Board {
public:
	static constexpr uint16 lancerDist = 3;
	static constexpr uint16 dragonDist = 4;
	static constexpr float screenYUp = 0.f;
	static constexpr float screenYDown = -4.2f;
	static constexpr array<uint16 (*)(uint16, svec2), 8> adjacentFull = {
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x && id % lim.x ? id - lim.x - 1 : UINT16_MAX; },							// left up
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x ? id - lim.x : UINT16_MAX; },											// up
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x && id % lim.x != lim.x - 1 ? id - lim.x + 1 : UINT16_MAX; },				// right up
		[](uint16 id, svec2 lim) -> uint16 { return id % lim.x ? id - 1 : UINT16_MAX; },												// left
		[](uint16 id, svec2 lim) -> uint16 { return id % lim.x != lim.x - 1 ? id + 1 : UINT16_MAX; },									// right
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 && id % lim.x ? id + lim.x - 1 : UINT16_MAX; },				// left down
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 ? id + lim.x : UINT16_MAX; },								// down
		[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 && id % lim.x != lim.x - 1 ? id + lim.x + 1 : UINT16_MAX; }	// right down
	};

	Com::Config config;
	array<uint16, Com::pieceMax> ownPieceAmts, enePieceAmts;
	Tile* ownFarm;
	Tile* ownCity;
	Tile* eneFarm;
	Tile* eneCity;
private:
	uint16 boardHeight;	// total number of rows
	float objectSize;	// width and height of a tile/piece
	vec2 tilesOffset;	// top left corner
	vec2 bobOffset;		// offset for gtop
	vec4 boardBounds;

	Mesh gridat;
	Object ground, board, bgrid, screen, pxpad;
	TileCol tiles;
	PieceCol pieces;

public:
	Board(const Scene* scene);
	~Board();

	void updateConfigValues();
	vector<Object*> initObjects(const Com::Config& cfg, bool regular, const Settings* sets, const Scene* scene);
#ifdef DEBUG
	vector<Object*> initDummyObjects(const Com::Config& cfg, const Settings* sets, const Scene* scene);
#endif
	void uninitObjects();
	void initOwnPieces();
	uint16 countAvailableFavors();
	void prepareMatch(bool myTurn, Com::Tile* buf);
	void prepareTurn(bool myTurn, bool xmov, bool fcont, Record& orec, Record& erec);

	TileCol& getTiles();
	Tile* getTile(svec2 pos);
	Object* getScreen();
	const Object* getPxpad() const;
	bool isHomeTile(const Tile* til) const;
	bool isEnemyTile(const Tile* til) const;
	uint16 tileCompressionSize() const;
	uint8 compressTile(uint16 e) const;
	static Com::Tile decompressTile(const uint8* src, uint16 i);
	Piece* getPieces(Piece* beg, const array<uint16, Com::pieceMax>& amts, Com::Piece type);
	Piece* findPiece(Piece* beg, Piece* end, svec2 pos);
	PieceCol& getPieces();
	Piece* getOwnPieces(Com::Piece type);
	Piece* findOccupant(const Tile* tile);
	Piece* findOccupant(svec2 pos);
	BoardObject* findObject(const vec3& isct);
	bool isOwnPiece(const Piece* pce) const;
	bool isEnemyPiece(const Piece* pce) const;
	bool pieceOnBoard(const Piece* piece) const;
	bool pieceOnHome(const Piece* piece) const;

	static void setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim = false);
	void restorePiecesInteract(const Record& orec);
	void setOwnTilesInteract(Tile::Interact lvl, bool dim = false);
	void setMidTilesInteract(Tile::Interact lvl, bool dim = false);
	void disableOwnPiecesInteract(bool rigid, bool dim = false);
	void setOwnPiecesVisible(bool on);
	void checkOwnTiles() const;		// throws error string on failure
	void checkMidTiles() const;		// ^
	void checkOwnPieces() const;	// ^
	vector<uint16> countOwnTiles() const;
	vector<uint16> countMidTiles() const;
	vector<uint16> countOwnPieces() const;
	void fillInFortress();
	void takeOutFortress();
	void setFavorInteracts(Favor favor, const Record& orec);
	void setPxpadPos(const Piece* piece);
	void selectEstablishable();
	bool tileRebuildable(Piece* throne);
	void selectRebuildable();
	Tile* establishTile(Piece* throne, bool reinit, const Record& orec, ProgMatch* match);
	bool pieceSpawnable(Com::Piece type);
	void selectSpawners();
	Piece* findSpawnablePiece(Com::Piece type, Tile*& tile, bool reinit, const Record& orec);

	void collectTilesBySingle(uset<uint16>& tcol, uint16 pos);
	void collectTilesByStraight(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*));
	void collectTilesByArea(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*));
	void collectTilesByType(uset<uint16>& tcol, uint16 pos, bool (*stepable)(uint16, void*));
	void collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, Com::Tile type, bool (*stepable)(uint16, void*));
	void collectTilesByPorts(uset<uint16>& tcol, uint16 pos);
	void collectTilesForLancer(uset<uint16>& tcol, uint16 pos);
	void collectTilesByDistance(uset<uint16>& tcol, svec2 pos, pair<uint8, uint8> dist);
	static bool spaceAvailableAny(uint16 pos, void* board);
	static bool spaceAvailableGround(uint16 pos, void* board);
	static bool spaceAvailableDragon(uint16 pos, void* board);
	void highlightMoveTiles(Piece* pce, const Record& erec, Favor favor);	// nullptr to disable
	void highlightEngageTiles(Piece* pce);									// ^
	uset<uint16> collectMoveTiles(const Piece* piece, const Record& erec, Favor favor, bool single = false);
	uset<uint16> collectEngageTiles(const Piece* piece);
	bool checkThroneWin(Piece* pcs, const array<uint16, Com::pieceMax>& amts);
	bool checkFortressWin();

	svec2 boardLimit() const;
	const vec4& getBoardBounds() const;
	svec2 ptog(const vec3& p) const;
	vec3 gtop(svec2 p, float z = 0.f) const;
	uint16 posToId(svec2 p) const;
	svec2 idToPos(uint16 i) const;
	uint16 invertId(uint16 i) const;
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
	uint16 inversePieceId(Piece* piece) const;

private:
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void setTiles(Tile* tils, uint16 yofs, bool show);
	void setMidTiles();
	void setPieces(Piece* pces, float rot, const Material* matl);
	void setBgrid();
	static vector<uint16> countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt);
	void rearangeMiddle(Com::Tile* mid, Com::Tile* buf, bool myTurn);
	uint16 findEmptyMiddle(Com::Tile* mid, uint16 i, uint16 m);
};

inline Board::~Board() {
	gridat.free();
}

inline TileCol& Board::getTiles() {
	return tiles;
}

inline Tile* Board::getTile(svec2 pos) {
	return &tiles[pos.y * config.homeSize.x + pos.x];
}

inline Object* Board::getScreen() {
	return &screen;
}

inline const Object* Board::getPxpad() const {
	return &pxpad;
}

inline bool Board::isHomeTile(const Tile* til) const {
	return til >= tiles.own();
}

inline bool Board::isEnemyTile(const Tile* til) const {
	return til < tiles.mid();
}

inline uint16 Board::tileCompressionSize() const {
	return tiles.getExtra() / 2 + tiles.getExtra() % 2;
}

inline uint8 Board::compressTile(uint16 i) const {
	return uint8(uint8(tiles[tiles.getSize()-i-1].getType()) << (i % 2 * 4));
}

inline Com::Tile Board::decompressTile(const uint8* src, uint16 i) {
	return Com::Tile((src[i/2] >> (i % 2 * 4)) & 0xF);
}

inline PieceCol& Board::getPieces() {
	return pieces;
}

inline Piece* Board::getOwnPieces(Com::Piece type) {
	return getPieces(pieces.own(), ownPieceAmts, type);
}

inline Piece* Board::findOccupant(const Tile* tile) {
	return findOccupant(idToPos(tileId(tile)));
}

inline Piece* Board::findOccupant(svec2 pos) {
	return findPiece(pieces.begin(), pieces.end(), pos);
}

inline bool Board::isOwnPiece(const Piece* pce) const {
	return pce < pieces.ene();
}

inline bool Board::isEnemyPiece(const Piece* pce) const {
	return pce >= pieces.ene();
}

inline bool Board::pieceOnBoard(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0), boardLimit());
}

inline bool Board::pieceOnHome(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0, config.homeSize.y + 1), boardLimit());
}

inline void Board::setOwnTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.own(), tiles.getHome(), lvl, dim);
}

inline void Board::setMidTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.mid(), config.homeSize.x, lvl, dim);
}

inline vector<uint16> Board::countOwnTiles() const {
	return countTiles(tiles.own(), tiles.getHome(), vector<uint16>(config.tileAmounts.begin(), config.tileAmounts.end()));
}

inline vector<uint16> Board::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeSize.x, vector<uint16>(config.middleAmounts.begin(), config.middleAmounts.end()));
}

inline void Board::collectTilesBySingle(uset<uint16>& tcol, uint16 pos) {
	collectTilesByStraight(tcol, pos, 1, spaceAvailableAny);
}

inline svec2 Board::boardLimit() const {
	return svec2(config.homeSize.x, boardHeight);
}

inline const vec4& Board::getBoardBounds() const {
	return boardBounds;
}

inline svec2 Board::ptog(const vec3& p) const {
	return svec2((vec2(p.x, p.z) - tilesOffset) / objectSize);
}

inline vec3 Board::gtop(svec2 p, float z) const {
	return vec3(float(p.x) * objectSize + bobOffset.x, z, float(p.y) * objectSize + bobOffset.y);
}

inline uint16 Board::posToId(svec2 p) const {
	return uint16(p.y * config.homeSize.x + p.x);
}

inline svec2 Board::idToPos(uint16 i) const {
	return svec2(i % config.homeSize.x, i / config.homeSize.x);
}

inline uint16 Board::invertId(uint16 i) const {
	return tiles.getSize() - i - 1;
}

inline uint16 Board::tileId(const Tile* tile) const {
	return uint16(tile - tiles.begin());
}

inline uint16 Board::inverseTileId(const Tile* tile) const {
	return uint16(tiles.end() - tile - 1);
}

inline uint16 Board::inversePieceId(Piece* piece) const {
	return piece ? isOwnPiece(piece) ? uint16(piece - pieces.own()) + pieces.getNum() : uint16(piece - pieces.ene()) : UINT16_MAX;
}
