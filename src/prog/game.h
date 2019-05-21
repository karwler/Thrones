#pragma once

#include "netcp.h"
#include "utils/objects.h"

// handles game logic
class Game {
public:
	static const vec3 screenPosUp, screenPosDown;
	static constexpr char messageTurnGo[] = "Your turn";
	static constexpr char messageTurnWait[] = "Opponent's turn";
private:
	static constexpr char messageWin[] = "You win";
	static constexpr char messageLoose[] = "You lose";
	static constexpr uint8 favorLimit = 4;

	uptr<Netcp> netcp;	// shall never be nullptr
	Com::Config config;
	Blueprint gridat;
	Object bgrid, board, screen;
	TileCol tiles;
	PieceCol pieces;

	struct Record {
		Piece* piece;
		bool attack, swap;

		Record(Piece* piece = nullptr, bool attack = false, bool swap = false);
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
	vec2s ptog(const vec3& p) const;	// TODO: optimize away
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

	void pieceMove(Piece* piece, vec2s pos, Piece* occupant);
	void pieceFire(Piece* killer, vec2s pos, Piece* piece);
	void placeFavor(vec2s pos);
	void placeDragon(vec2s pos, Piece* occupant);

private:
	void connect(Netcp* net);
	Piece* getPieces(Piece* pieces, Com::Piece type);
	void setBgrid();
	void setMidTiles();
	void setTiles(Tile* tiles, int16 yofs, OCall lcall, Object::Info mode);
	void setPieces(Piece* pieces, OCall rcall, OCall ucall, Object::Info mode, const vec4& color);
	static void setTilesInteract(Tile* tiles, uint16 num, bool on);
	static void setPiecesInteract(Piece* pieces, uint16 num, bool on);
	void setPiecesVisible(Piece* pieces, bool on);
	static vector<uint8> countTiles(const Tile* tiles, uint16 num, vector<uint8> cnt);
	template <class T> static void setObjectAddrs(T* data, sizet size, vector<Object*>& dst, sizet& id);
	void rearangeMiddle(Com::Tile* mid, Com::Tile* buf);
	static Com::Tile decompressTile(const uint8* src, uint16 i);

	bool checkMove(Piece* piece, vec2s pos, Piece* occupant, vec2s dst, bool attacking);
	bool checkMoveBySingle(vec2s pos, vec2s dst);
	bool checkMoveByArea(vec2s pos, vec2s dst, int16 dist);
	static bool spaceAvailible(uint16 pos);
	bool checkMoveByType(vec2s pos, vec2s dst);
	bool checkAdjacentTilesByType(uint16 pos, uint16 dst, vector<bool>& visited, Com::Tile type) const;
	bool checkFire(Piece* killer, vec2s pos, Piece* victim, vec2s dst);
	static bool checkTilesByDistance(vec2s pos, vec2s dst, int16 dist);
	bool checkAttack(Piece* killer, Piece* victim, Tile* dtil);
	bool survivalCheck(Piece* piece, vec2s spos, vec2s dpos, bool attacking);	// in this case "attacking" only refers to attack by movement
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
	void updateFavored(Tile* tile, bool favored);

	uint16 posToId(vec2s p);
	vec2s idToPos(uint16 i);
	uint16 invertId(uint16 i);
	uint16 tileId(const Tile* tile) const;
	uint16 inverseTileId(const Tile* tile) const;
};

inline void Game::connect() {
	return connect(new Netcp);
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

inline void Game::setMidTilesInteract(bool on) {
	setTilesInteract(tiles.mid(), config.homeWidth, on);
}

inline void Game::setOwnPiecesInteract(bool on) {
	setPiecesInteract(pieces.own(), config.numPieces, on);
}

inline void Game::setOwnPiecesVisible(bool on) {
	setPiecesVisible(pieces.own(), on);
}

inline vector<uint8> Game::countOwnTiles() const {
	return countTiles(tiles.own(), config.numTiles, vector<uint8>(config.tileAmounts.begin(), config.tileAmounts.end() - 1));
}

inline vector<uint8> Game::countMidTiles() const {
	return countTiles(tiles.mid(), config.homeWidth, vector<uint8>(config.tileAmounts.size() - 1, 1));
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

inline vec2s Game::ptog(const vec3& p) const {
	return vec2s(uint16(p.x) + config.homeWidth / 2, p.z);
}

inline vec3 Game::gtop(vec2s p, float z) const {
	return vec3(p.x - config.homeWidth / 2, z, p.y);
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
