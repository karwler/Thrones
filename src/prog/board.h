#pragma once

#include "utils/objects.h"

// game board
class Board {
public:
	static constexpr uint16 lancerDist = 3;
	static constexpr uint16 dragonDist = 4;
	static constexpr float screenYUp = 0.f;
	static constexpr float screenYDown = -4.2f;

	Config config;
	array<uint16, pieceLim> ownPieceAmts, enePieceAmts;
private:
	uint16 boardHeight;	// total number of rows
	float objectSize;	// width and height of a tile/piece
	vec2 tilesOffset;	// top left corner
	vec2 bobOffset;		// offset for gtop
	vec4 boardBounds;

	Object screen;
	TileCol tiles;
	array<BoardObject, TileTop::none> tileTops;
	Object pxpad;
	PieceCol pieces;

	Scene* scene;
	Settings* sets;

public:
	Board(Scene* sceneSys, Settings* settings);

	uint16 initConfig(const Config& cfg);	// returns number of pieces left to pick
	void initObjects(bool regular, bool basicInitPieces = true, bool initAllPieces = false);
	void initEnePieces(Mesh** meshes);
#ifndef NDEBUG
	void initDummyObjects();
#endif
	void uninitObjects();
	uint16 countAvailableFavors();
	void prepareMatch(bool myTurn, TileType* buf);
	void prepareTurn(bool myTurn, bool xmov, bool fcont, Record& orec, Record& erec);

	TileCol& getTiles();
	Tile* getTile(svec2 pos);
	Object* getScreen();
	const Object* getPxpad() const;
	bool isHomeTile(const Tile* til) const;
	bool isEnemyTile(const Tile* til) const;
	uint16 tileCompressionSize() const;
	uint8 compressTile(uint16 e) const;
	static TileType decompressTile(const uint8* src, uint16 i);
	Piece* getPieces(Piece* beg, const array<uint16, pieceLim>& amts, PieceType type);
	Piece* findPiece(svec2 pos);
	PieceCol& getPieces();
	Piece* getOwnPieces(PieceType type);
	Piece* getEnePieces(PieceType type);
	Piece* findOccupant(const Tile* tile);
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
	BoardObject* getTileTop(TileTop top);
	TileTop findTileTop(const Tile* tile);
	Tile* getTileBot(TileTop top);
	void setTileTop(TileTop top, const Tile* tile);
	void selectEstablishers();
	pair<Tile*, TileTop> checkTileEstablishable(const Piece* throne);
	bool tileRebuildable(const Piece* throne);
	void selectRebuilders();
	bool pieceSpawnable(PieceType type);
	void selectSpawners();	// can only happen when spawning on fortresses
	Tile* findSpawnableTile(PieceType type);
	Piece* findSpawnablePiece(PieceType type);
	void resetTilesAfterSpawn();

	void collectTilesBySingle(uset<uint16>& tcol, uint16 pos);
	void collectTilesByStraight(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*));
	void collectTilesByArea(uset<uint16>& tcol, uint16 pos, uint16 dlim, bool (*stepable)(uint16, void*));
	void collectTilesByType(uset<uint16>& tcol, uint16 pos, bool (*stepable)(uint16, void*));
	void collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, TileType type, bool (*stepable)(uint16, void*));
	void collectTilesByPorts(uset<uint16>& tcol, uint16 pos);
	void collectTilesForLancer(uset<uint16>& tcol, uint16 pos);
	void collectTilesByDistance(uset<uint16>& tcol, svec2 pos, pair<uint8, uint8> dist);
	static bool spaceAvailableAny(uint16 pos, void* board);
	static bool spaceAvailableGround(uint16 pos, void* board);
	static bool spaceAvailableDragon(uint16 pos, void* board);
	void highlightMoveTiles(const Piece* pce, const Record& erec, Favor favor);	// nullptr to disable
	void highlightEngageTiles(const Piece* pce);								// ^
	uset<uint16> collectMoveTiles(const Piece* piece, const Record& erec, Favor favor, bool single = false);
	uset<uint16> collectEngageTiles(const Piece* piece);
	bool checkThroneWin(Piece* pcs, const array<uint16, pieceLim>& amts);
	bool checkFortressWin(const Tile* tit, const Piece* pit, const array<uint16, pieceLim>& amts) const;
	Record::Info countVictoryPoints(uint16& own, uint16& ene, const Record& erec);

	svec2 boardLimit() const;
	const vec4& getBoardBounds() const;
	svec2 ptog(const vec3& p) const;
	vec3 gtop(svec2 p, float z = 0.f) const;
	uint16 posToId(svec2 p) const;
	svec2 idToPos(uint16 i) const;
	uint16 invertId(uint16 i) const;
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
	uint16 pieceId(const Piece* piece) const;
	uint16 inversePieceId(Piece* piece) const;
	void updateTileInstances(Tile* til, Mesh* old);

private:
	void setTiles(uint16 id, uint16 yofs, Mesh* mesh, uint matl, uvec2 tex);
	void setMidFortressTiles();
	void setPieces(Piece* pces, float rot);
	void setBgrid();
	static vector<uint16> countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt);
	uint16 findEmptyMiddle(const vector<TileType>& mid, uint16 i, int16 m) const;
};

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
	return uint8(tiles[tiles.getSize() - i - 1].getType()) << (i % 2 * 4);
}

inline TileType Board::decompressTile(const uint8* src, uint16 i) {
	return TileType((src[i/2] >> (i % 2 * 4)) & 0xF);
}

inline PieceCol& Board::getPieces() {
	return pieces;
}

inline Piece* Board::getOwnPieces(PieceType type) {
	return getPieces(pieces.own(), ownPieceAmts, type);
}

inline Piece* Board::getEnePieces(PieceType type) {
	return getPieces(pieces.ene(), ownPieceAmts, type);
}

inline Piece* Board::findOccupant(const Tile* tile) {
	return findPiece(idToPos(tileId(tile)));
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

inline vector<uint16> Board::countOwnTiles() const {
	return countTiles(tiles.own(), tiles.getHome(), vector<uint16>(config.tileAmounts.begin(), config.tileAmounts.end()));
}

inline vector<uint16> Board::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeSize.x, vector<uint16>(config.middleAmounts.begin(), config.middleAmounts.end()));
}

inline BoardObject* Board::getTileTop(TileTop top) {
	return &tileTops[top];
}

inline Tile* Board::getTileBot(TileTop top) {
	return getTile(ptog(tileTops[top].getPos()));
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
	return p.y * config.homeSize.x + p.x;
}

inline svec2 Board::idToPos(uint16 i) const {
	return svec2(i % config.homeSize.x, i / config.homeSize.x);
}

inline uint16 Board::invertId(uint16 i) const {
	return tiles.getSize() - i - 1;
}

inline uint16 Board::tileId(const Tile* tile) const {
	return tile - tiles.begin();
}

inline uint16 Board::inverseTileId(const Tile* tile) const {
	return tiles.end() - tile - 1;
}

inline uint16 Board::pieceId(const Piece* piece) const {
	return piece - pieces.begin();
}

inline uint16 Board::inversePieceId(Piece* piece) const {
	return piece ? isOwnPiece(piece) ? piece - pieces.own() + pieces.getNum() : piece - pieces.ene() : UINT16_MAX;
}
