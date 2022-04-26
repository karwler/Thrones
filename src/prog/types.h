#pragma once

#include "utils/alias.h"
#include <numeric>

enum class TileType : uint8 {
	plains,
	forest,
	mountain,
	water,
	fortress,
	empty
};
constexpr uint8 tileLim = uint8(TileType::fortress);

constexpr array<const char*, uint8(TileType::empty)+1> tileNames = {
	"plains",
	"forest",
	"mountain",
	"water",
	"fortress",
	""
};

enum class PieceType : uint8 {
	rangers,
	spearmen,
	crossbowmen,
	catapult,
	trebuchet,
	lancer,
	warhorse,
	elephant,
	dragon,
	throne
};
constexpr uint8 pieceLim = uint8(PieceType::throne) + 1;

constexpr array<const char*, uint8(PieceType::throne)+1> pieceNames = {
	"rangers",
	"spearmen",
	"crossbowmen",
	"catapult",
	"trebuchet",
	"lancer",
	"warhorse",
	"elephant",
	"dragon",
	"throne"
};

constexpr array<uint16 (*const)(uint16, svec2), 8> adjacentIndex = {
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

	static constexpr char defaultName[] = "default";
	static constexpr uint8 maxNameLength = 63;
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;
	static constexpr svec2 minHomeSize = svec2(5, 2);
	static constexpr svec2 maxHomeSize = svec2(101, 50);
	static constexpr uint16 maxFavorMax = UINT16_MAX / 4;

	svec2 homeSize = { 9, 4 };	// neither width nor height shall exceed UINT8_MAX
	uint8 battlePass = randomLimit / 2;
	bool record = false;
	Option opts = favorTotal | terrainRules | dragonStraight;
	uint16 victoryPointsNum = 21;
	uint16 setPieceBattleNum = 10;
	uint16 favorLimit = 1;
	array<uint16, tileLim> tileAmounts = { 14, 9, 5, 7 };
	array<uint16, tileLim> middleAmounts = { 1, 1, 1, 1 };
	array<uint16, pieceLim> pieceAmounts = { 2, 2, 1, 1, 1, 2, 1, 1, 1, 1 };
	uint16 winThrone = 1;
	uint16 winFortress = 1;
	uint16 capturers = 1 << uint8(PieceType::throne);	// bitmask of piece types that can capture fortresses
	string recordName;

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
	return sizeof(uint8) + name.length() + 2 * sizeof(uint8) + sizeof(opts) + sizeof(battlePass) + sizeof(victoryPointsNum) + sizeof(setPieceBattleNum) + sizeof(favorLimit) + tileLim * sizeof(uint16) + tileLim * sizeof(uint16) + pieceLim * sizeof(uint16) + sizeof(winThrone) + sizeof(winFortress) + sizeof(capturers);
}

inline uint16 Config::countTiles() const {
	return std::accumulate(tileAmounts.begin(), tileAmounts.end(), 0);
}

inline uint16 Config::countMiddles() const {
	return std::accumulate(middleAmounts.begin(), middleAmounts.end(), 0);
}

inline uint16 Config::countPieces() const {
	return std::accumulate(pieceAmounts.begin(), pieceAmounts.end(), 0);
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

constexpr TileTop::operator Type() const {
	return type;
}

constexpr bool TileTop::isFarm() const {
	return type < none && !(type % 2);
}

constexpr bool TileTop::isCity() const {
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

constexpr const char* TileTop::name() const {
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
		bool operator()(Node a, Node b) const;
	};

public:
	static vector<uint16> travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16, void*), void* data);
};

inline bool Dijkstra::Comp::operator()(Node a, Node b) const {
	return a.dst > b.dst;
}

// setup save/load data
struct Setup {
	vector<pair<svec2, TileType>> tiles;
	vector<pair<uint16, TileType>> mids;
	vector<pair<svec2, PieceType>> pieces;

	void clear();
};
