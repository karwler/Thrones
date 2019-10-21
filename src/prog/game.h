#pragma once

#include "netcp.h"
#include "utils/objects.h"
#include <random>

struct FavorState {
	Piece* piece;	// when dragging piece
	bool use;		// when holding down lalt

	FavorState();
};

enum Action : uint8 {
	ACT_NONE  = 0x00,
	ACT_MOVE  = 0x01,	// regular position change
	ACT_SWAP  = 0x02,	// actor switching with swapper
	ACT_ATCK  = 0x04,	// movement attack
	ACT_FIRE  = 0x08,	// firing attack
	ACT_WIN   = 0x10,	// actor won
	ACT_LOOSE = 0x20	// actor lost
};
ENUM_OPERATIONS(Action, uint8)

class Record {
public:
	Piece* actor;	// the moved piece/the killer
	Piece* swapper;	// who swapped with acter
	Action action;

public:
	Record(Piece* actor = nullptr, Piece* swapper = nullptr, Action action = ACT_NONE);

	void update(Piece* doActor, Piece* didSwap, Action didActions);
	void updateProtectionColors(bool on) const;
	bool isProtectedMember(Piece* pce) const;
private:
	void updateProtectionColor(Piece* pce, bool on) const;
	bool pieceProtected(Piece* pce) const;		// piece mustn't be nullptr
};

inline bool Record::isProtectedMember(Piece* pce) const {
	return (actor == pce || swapper == pce) && pieceProtected(pce);
}

inline void Record::updateProtectionColor(Piece* pce, bool on) const {
	pce->setEmission(pieceProtected(pce) && on ? pce->getEmission() | BoardObject::EMI_DIM : pce->getEmission() & ~BoardObject::EMI_DIM);
}

inline bool Record::pieceProtected(Piece* pce) const {
	return pce->getType() == Com::Piece::warhorse && (action & (ACT_SWAP | ACT_ATCK | ACT_FIRE));
}

// handles game logic
class Game {
public:
	Com::Config config;
	string configName;
	FavorState favorState;	// favor state when dragging piece

	static constexpr float screenYUp = 0.f;
	static constexpr float screenYDown = -4.2f;
private:
	static const array<uint16 (*)(uint16, uint16), 4> adjacentStraight;
	static const array<uint16 (*)(uint16, uint16), 8> adjacentFull;

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint> randDist;
	Buffer sendb;

	GMesh gridat;
	Object ground, board, bgrid, screen, ffpad;
	TileCol tiles;
	PieceCol pieces;

	Record ownRec, eneRec;	// what happened the previous turn
	bool myTurn, firstTurn;
	uint8 favorCount, favorTotal;

public:
	Game();
	~Game();

	TileCol& getTiles();
	Tile* getTile(svec2 pos);
	bool isHomeTile(Tile* til) const;
	bool isEnemyTile(Tile* til) const;
	PieceCol& getPieces();
	Piece* getOwnPieces(Com::Piece type);
	Piece* findPiece(svec2 pos);
	bool isOwnPiece(Piece* pce) const;
	bool isEnemyPiece(Piece* pce) const;
	bool pieceOnBoard(const Piece* piece) const;
	bool pieceOnHome(const Piece* piece) const;
	Object* getScreen();
	bool getMyTurn() const;
	uint8 getFavorCount() const;
	uint8 getFavorTotal() const;
	svec2 ptog(const vec3& p) const;
	vec3 gtop(svec2 p, float z = 0.f) const;

	void tick();
	void sendStart();
	void sendConfig(bool onJoin = false);
	void sendSetup();
	void recvStart(uint8* data);
	void recvTiles(uint8* data);
	void recvPieces(uint8* data);
	void recvMove(uint8* data);
	void recvKill(uint8* data);
	void recvBreach(uint8* data);
	void recvRecord(uint8* data);
	vector<Object*> initObjects();
#ifdef DEBUG
	vector<Object*> initDummyObjects();
#endif
	void uninitObjects();
	void prepareMatch();
	void setOwnTilesInteract(Tile::Interact lvl, bool dim = false);
	void setMidTilesInteract(Tile::Interact lvl, bool dim = false);
	void setOwnPiecesVisible(bool on, bool event = true);
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

	void updateFavorState();
	void pieceMove(Piece* piece, svec2 pos, Piece* occupant, bool forceSwitch);
	void pieceFire(Piece* killer, svec2 pos, Piece* piece);
	void placeDragon(svec2 pos, Piece* occupant);
	void prepareTurn();
	void endTurn();

private:
	Piece* getPieces(Piece* pieces, Com::Piece type);
	void setBgrid();
	void setMidTiles();
	void setTiles(Tile* tiles, int16 yofs, bool inter);
	void setPieces(Piece* pieces, float rot, const Material* matl);
	static void setTilesInteract(Tile* tiles, uint16 num, Tile::Interact lvl, bool dim = false);
	static void setPieceInteract(Piece* piece, bool on, bool dim, GCall hgcall, GCall ulcall, GCall urcall);
	static vector<uint16> countTiles(const Tile* tiles, uint16 num, vector<uint16> cnt);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void rearangeMiddle(Com::Tile* mid, Com::Tile* buf);
	uint8 compressTile(uint16 e) const;
	static Com::Tile decompressTile(uint8* src, uint16 i);
	Com::Tile pollFavor();	// resets FF indicator
	Com::Tile checkFavor();	// unlike pollFavor it just returns the type
	void useFavor();

	void concludeAction(bool end);
	void checkMove(Piece* piece, svec2 pos, Piece* occupant, svec2 dst, Action action, Com::Tile favor);
	template <class F, class... A> void checkMoveDestination(svec2 pos, svec2 dst, Com::Tile favor, F check, A... args);
	uset<uint16> collectTilesBySingle(svec2 pos, Com::Tile favor, bool& favorUsed);
	uset<uint16> collectTilesByArea(svec2 pos, Com::Tile favor, bool& favorUsed, uint16 dlim, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, uint16), uint8 movSize);
	uset<uint16> collectTilesByType(svec2 pos, Com::Tile favor, bool& favorUsed);
	void collectAdjacentTilesByType(uset<uint16>& tcol, uint16 pos, Com::Tile type) const;
	static bool spaceAvailible(uint16 pos);
	static bool spaceAvailibleDummy(uint16 pos);
	void checkFire(Piece* killer, svec2 pos, Piece* victim, svec2 dst);
	uset<uint16> collectTilesByDistance(svec2 pos, int16 dist);
	void checkAttack(Piece* killer, Piece* victim, Tile* dtil) const;
	bool survivalCheck(Piece* piece, Tile* stil, Tile* dtil, Action action, Com::Tile favor);
	void failSurvivalCheck(Piece* piece, Action action);
	bool checkWin();
	bool checkThroneWin(Piece* thrones);
	bool checkFortressWin();
	void doWin(bool win);
	void placePiece(Piece* piece, svec2 pos);	// set the position and check if a favor has been gained
	void removePiece(Piece* piece);				// remove from board
	void updateFortress(Tile* fort, bool breached);

	uint16 posToId(svec2 p);
	svec2 idToPos(uint16 i);
	uint16 invertId(uint16 i);
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
	uint16 inversePieceId(Piece* piece) const;
	std::default_random_engine createRandomEngine();
};

inline Game::~Game() {
	gridat.free();
}

inline TileCol& Game::getTiles() {
	return tiles;
}

inline Tile* Game::getTile(svec2 pos) {
	return tiles.mid(pos.y * config.homeSize.x + pos.x);
}

inline bool Game::isHomeTile(Tile* til) const {
	return til >= tiles.own();
}

inline bool Game::isEnemyTile(Tile* til) const {
	return til < tiles.mid();
}

inline PieceCol& Game::getPieces() {
	return pieces;
}

inline Piece* Game::getOwnPieces(Com::Piece type) {
	return getPieces(pieces.own(), type);
}

inline bool Game::isOwnPiece(Piece* pce) const {
	return pce < pieces.ene();
}

inline bool Game::isEnemyPiece(Piece* pce) const {
	return pce >= pieces.ene();
}

inline bool Game::pieceOnBoard(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0, -int16(config.homeSize.y)), svec2(config.homeSize.x - 1, config.homeSize.y));
}

inline bool Game::pieceOnHome(const Piece* piece) const {
	return inRange(ptog(piece->getPos()), svec2(0, 1), svec2(config.homeSize.x - 1, config.homeSize.y));
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

inline void Game::setOwnTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.own(), config.numTiles, lvl, dim);
}

inline void Game::setMidTilesInteract(Tile::Interact lvl, bool dim) {
	setTilesInteract(tiles.mid(), config.homeSize.x, lvl, dim);
}

inline vector<uint16> Game::countOwnTiles() const {
	return countTiles(tiles.own(), config.numTiles, vector<uint16>(config.tileAmounts.begin(), config.tileAmounts.end() - 1));
}

inline vector<uint16> Game::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeSize.x, vector<uint16>(config.middleAmounts.begin(), config.middleAmounts.end()));
}

template <class T>
void Game::setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id) {
	for (sizet i = 0; i < size; i++)
		dst[id+i] = &data[i];
	id += size;
}

inline uint8 Game::compressTile(uint16 e) const {
	return uint8(uint8(tiles.mid(e)->getType()) << (e % 2 * 4));
}

inline Com::Tile Game::decompressTile(uint8* src, uint16 i) {
	return Com::Tile((src[i/2] >> (i % 2 * 4)) & 0xF);
}

inline Com::Tile Game::checkFavor() {
	return ffpad.show ? getTile(ptog(ffpad.getPos()))->getType() : Com::Tile::empty;
}

inline uset<uint16> Game::collectTilesBySingle(svec2 pos, Com::Tile favor, bool& favorUsed) {
	return collectTilesByArea(pos, favor, favorUsed, 1, spaceAvailibleDummy, adjacentFull.data(), uint8(adjacentFull.size()));
}

inline bool Game::spaceAvailibleDummy(uint16) {
	return true;
}

inline void Game::recvMove(uint8* data) {
	pieces[SDLNet_Read16(data)].updatePos(idToPos(SDLNet_Read16(data + 2)), true);
}

inline void Game::recvKill(uint8* data) {
	pieces[SDLNet_Read16(data)].updatePos();
}

inline void Game::recvBreach(uint8* data) {
	tiles[SDLNet_Read16(data + 1)].setBreached(data[0]);
}

inline svec2 Game::ptog(const vec3& p) const {
	return svec2((p.x - config.objectSize / 2.f) / config.objectSize, p.z / config.objectSize);
}

inline vec3 Game::gtop(svec2 p, float z) const {
	return vec3(p.x * config.objectSize + config.objectSize / 2.f, z, p.y * config.objectSize);
}

inline uint16 Game::posToId(svec2 p) {
	return uint16((p.y + config.homeSize.y) * config.homeSize.x + p.x);
}

inline svec2 Game::idToPos(uint16 i) {
	return svec2(int16(i % config.homeSize.x), int16(i / config.homeSize.x) - config.homeSize.y);
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
	return piece ? isOwnPiece(piece) ? uint16(piece - pieces.own()) + config.numPieces : uint16(piece - pieces.ene()) : UINT16_MAX;
}
