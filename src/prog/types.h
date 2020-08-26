#pragma once

#include "utils/objects.h"
#include <numeric>

constexpr array<uint16 (* const)(uint16, svec2), 8> adjacentIndex = {
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x && id % lim.x ? id - lim.x - 1 : UINT16_MAX; },							// left up
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x ? id - lim.x : UINT16_MAX; },											// up
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x && id % lim.x != lim.x - 1 ? id - lim.x + 1 : UINT16_MAX; },				// right up
	[](uint16 id, svec2 lim) -> uint16 { return id % lim.x ? id - 1 : UINT16_MAX; },												// left
	[](uint16 id, svec2 lim) -> uint16 { return id % lim.x != lim.x - 1 ? id + 1 : UINT16_MAX; },									// right
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 && id % lim.x ? id + lim.x - 1 : UINT16_MAX; },				// left down
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 ? id + lim.x : UINT16_MAX; },								// down
	[](uint16 id, svec2 lim) -> uint16 { return id / lim.x != lim.y - 1 && id % lim.x != lim.x - 1 ? id + lim.x + 1 : UINT16_MAX; }	// right down
};

enum class Favor : uint8 {	// for now these values must be equivalent to the first tile types until there are FF textures
	hasten,
	assault,
	conspire,
	deceive,
	none
};
constexpr uint8 favorMax = uint8(Favor::none);

constexpr array<const char*, favorMax> favorNames = {
	"hasten",
	"assault",
	"conspire",
	"deceive"
};

// variable game properties (shall never be changed after loading)
struct Config {
	enum Option : uint16 {
		victoryPoints = 0x1,
		victoryPointsEquidistant = 0x2,
		ports = 0x4,
		rowBalancing = 0x8,
		homefront = 0x10,
		setPieceBattle = 0x20,
		favorTotal = 0x40,
		firstTurnEngage = 0x80,
		terrainRules = 0x100,
		dragonLate = 0x200,
		dragonStraight = 0x400
	};

	svec2 homeSize;		// neither width nor height shall exceed UINT8_MAX
	uint8 battlePass;
	Option opts;
	uint16 victoryPointsNum;
	uint16 setPieceBattleNum;
	uint16 favorLimit;
	array<uint16, Tile::lim> tileAmounts;
	array<uint16, Tile::lim> middleAmounts;
	array<uint16, Piece::lim> pieceAmounts;
	uint16 winThrone, winFortress;
	uint16 capturers;	// bitmask of piece types that can capture fortresses

	static constexpr char defaultName[] = "default";
	static constexpr uint8 maxNameLength = 63;
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;
	static constexpr svec2 minHomeSize = { 5, 2 };
	static constexpr svec2 maxHomeSize = { 101, 50 };
	static constexpr uint16 maxFavorMax = UINT16_MAX / 4;

	Config();

	Config& checkValues();
	uint16 dataSize(const string& name) const;
	void toComData(uint8* data, const string& name) const;
	string fromComData(const uint8* data);	// returns name
	uint16 countTiles() const;
	uint16 countMiddles() const;
	uint16 countPieces() const;
	uint16 countFreeTiles() const;
	uint16 countFreeMiddles() const;
	uint16 countFreePieces() const;

	static uint16 floorAmounts(uint16 total, uint16* amts, uint16 limit, uint8 ei, uint16 floor = 0);
	static uint16 ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei);
};

inline uint16 Config::dataSize(const string& name) const {
	return uint16(sizeof(uint8) + name.length() + 2 * sizeof(uint8) + sizeof(opts) + sizeof(battlePass) + sizeof(victoryPointsNum) + sizeof(setPieceBattleNum) + sizeof(favorLimit) + Tile::lim * sizeof(uint16) + Tile::lim * sizeof(uint16) + Piece::lim * sizeof(uint16) + sizeof(winThrone) + sizeof(winFortress) + sizeof(capturers));
}

inline uint16 Config::countTiles() const {
	return std::accumulate(tileAmounts.begin(), tileAmounts.end(), uint16(0));
}

inline uint16 Config::countMiddles() const {
	return std::accumulate(middleAmounts.begin(), middleAmounts.end(), uint16(0));
}

inline uint16 Config::countPieces() const {
	return std::accumulate(pieceAmounts.begin(), pieceAmounts.end(), uint16(0));
}

inline uint16 Config::countFreeTiles() const {
	return homeSize.x * homeSize.y - countTiles();
}

inline uint16 Config::countFreeMiddles() const {
	return homeSize.x / 2 - countMiddles();
}

inline uint16 Config::countFreePieces() const {
	return homeSize.x * homeSize.y - countPieces();
}

enum Action : uint8 {
	ACT_NONE = 0x00,
	ACT_MOVE = 0x01,	// regular position change
	ACT_SWAP = 0x02,	// actor switching with piece
	ACT_ATCK = 0x04,	// movement attack
	ACT_FIRE = 0x08,	// firing attack
	ACT_SPAWN = 0x10,	// spawn piece
	ACT_MS = ACT_MOVE | ACT_SWAP,
	ACT_AF = ACT_ATCK | ACT_FIRE
};

// turn action information
struct Record {
	enum Info : uint8 {
		none,
		win,
		loose,
		tie,
		battleFail
	};

	umap<Piece*, Action> actors;	// the moved piece/the killer
	umap<Piece*, Action> assault;	// actions of assault pieces
	umap<Piece*, bool> protects;	// pieces that can't attack/fire or be attacked/fired at (true if set by conspire and to block throne)
	pair<Piece*, Action> lastAct;	// last actors action
	pair<Piece*, Action> lastAss;	// last assault action
	Info info;						// additional information

	Record(const pair<Piece*, Action>& last = pair(nullptr, ACT_NONE), umap<Piece*, bool>&& doProtect = umap<Piece*, bool>(), Info tinf = none);

	void update(Piece* actor, Action action, bool regular = true);
	void addProtect(Piece* piece, bool strong);
	Action actionsExhausted() const;	// returns terminating actions
};

// tile on top of another tile
struct TileTop {
	enum Type : uint8 {
		ownFarm,
		ownCity,
		eneFarm,
		eneCity,
		none
	};

	static constexpr array<const char*, 2> names = {
		"farm",
		"city"
	};

	Type type;

	template <class T> constexpr TileTop(T tileTop);

	constexpr operator Type() const;
	constexpr bool isFarm() const;
	constexpr bool isCity() const;
	constexpr bool isOwn() const;
	constexpr bool isEne() const;
	constexpr TileTop::Type invert() const;
	constexpr const char* name() const;
};

template <class T>
constexpr TileTop::TileTop(T tileTop) :
	type(Type(tileTop))
{}

inline constexpr TileTop::operator Type() const {
	return type;
}

inline constexpr bool TileTop::isFarm() const {
	return type < none && !(type % 2);
}

inline constexpr bool TileTop::isCity() const {
	return type < none && type % 2;
}

constexpr bool TileTop::isOwn() const {
	return type <= ownCity;
}

constexpr bool TileTop::isEne() const {
	return type == eneFarm || type == eneCity;
}

constexpr TileTop::Type TileTop::invert() const {
	return isOwn() ? type + 2 : isEne() ? type - 2 : type;
}

inline constexpr const char* TileTop::name() const {
	return names[type%2];
}

// path finding
class Dijkstra {
private:
	struct Node {
		uint16 id;
		uint16 dst;

		Node() = default;
		Node(uint16 pos, uint16 distance);
	};

	struct Adjacent {
		uint8 cnt;
		uint16 adj[8];
	};

	struct Comp {
		bool operator()(Node a, Node b);
	};

public:
	static vector<uint16> travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16, void*), void* data);
};

inline bool Dijkstra::Comp::operator()(Node a, Node b) {
	return a.dst > b.dst;
}

// tile container
class TileCol {
private:
	uptr<Tile[]> tl;
	uint16 home, extra, size;	// home = number of home tiles, extra = home + board width, size = all tiles

public:
	TileCol();

	void update(const Config& conf);
	uint16 getHome() const;
	uint16 getExtra() const;
	uint16 getSize() const;

	Tile& operator[](uint16 i);
	const Tile& operator[](uint16 i) const;
	Tile* begin();
	const Tile* begin() const;
	Tile* end();
	const Tile* end() const;
	Tile* ene(pdift i = 0);
	const Tile* ene(pdift i = 0) const;
	Tile* mid(pdift i = 0);
	const Tile* mid(pdift i = 0) const;
	Tile* own(pdift i = 0);
	const Tile* own(pdift i = 0) const;
};

inline uint16 TileCol::getHome() const {
	return home;
}

inline uint16 TileCol::getExtra() const {
	return extra;
}

inline uint16 TileCol::getSize() const {
	return size;
}

inline Tile& TileCol::operator[](uint16 i) {
	return tl[i];
}

inline const Tile& TileCol::operator[](uint16 i) const {
	return tl[i];
}

inline Tile* TileCol::begin() {
	return tl.get();
}

inline const Tile* TileCol::begin() const {
	return tl.get();
}

inline Tile* TileCol::end() {
	return &tl[size];
}

inline const Tile* TileCol::end() const {
	return &tl[size];
}

inline Tile* TileCol::ene(pdift i) {
	return &tl[i];
}

inline const Tile* TileCol::ene(pdift i) const {
	return &tl[i];
}

inline Tile* TileCol::mid(pdift i) {
	return &tl[home+i];
}

inline const Tile* TileCol::mid(pdift i) const {
	return &tl[home+i];
}

inline Tile* TileCol::own(pdift i) {
	return &tl[extra+i];
}

inline const Tile* TileCol::own(pdift i) const {
	return &tl[extra+i];
}

// piece container
class PieceCol {
private:
	uptr<Piece[]> pc;
	uint16 num, size;	// num = number of one player's pieces, i.e. size / 2

public:
	PieceCol();

	void update(const Config& conf, bool regular);
	uint16 getNum() const;
	uint16 getSize() const;

	Piece& operator[](uint16 i);
	const Piece& operator[](uint16 i) const;
	Piece* begin();
	const Piece* begin() const;
	Piece* end();
	const Piece* end() const;
	Piece* own(pdift i = 0);
	const Piece* own(pdift i = 0) const;
	Piece* ene(pdift i = 0);
	const Piece* ene(pdift i = 0) const;
};

inline uint16 PieceCol::getNum() const {
	return num;
}

inline uint16 PieceCol::getSize() const {
	return size;
}

inline Piece& PieceCol::operator[](uint16 i) {
	return pc[i];
}

inline const Piece& PieceCol::operator[](uint16 i) const {
	return pc[i];
}

inline Piece* PieceCol::begin() {
	return pc.get();
}

inline const Piece* PieceCol::begin() const {
	return pc.get();
}

inline Piece* PieceCol::end() {
	return &pc[size];
}

inline const Piece* PieceCol::end() const {
	return &pc[size];
}

inline Piece* PieceCol::own(pdift i) {
	return &pc[i];
}

inline const Piece* PieceCol::own(pdift i) const {
	return &pc[i];
}

inline Piece* PieceCol::ene(pdift i) {
	return &pc[num+i];
}

inline const Piece* PieceCol::ene(pdift i) const {
	return &pc[num+i];
}

// setup save/load data
struct Setup {
	vector<pair<svec2, Tile::Type>> tiles;
	vector<pair<uint16, Tile::Type>> mids;
	vector<pair<svec2, Piece::Type>> pieces;

	void clear();
};
