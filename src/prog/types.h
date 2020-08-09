#pragma once

#include "utils/objects.h"

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

class TileTop {
public:
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
private:
	Type type;

public:
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

class Dijkstra {
private:
	struct Node {
		uint16 id;
		uint16 dst;

		Node() = default;
		constexpr Node(uint16 pos, uint16 distance);
	};

	struct Adjacent {
		uint8 cnt;
		uint16 adj[8];
	};

	struct Comp {
		bool operator()(Node a, Node b);
	};

public:
	static vector<uint16> travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16, void*), uint16 (*const* vmov)(uint16, svec2), uint8 movSize, void* data);
};

inline constexpr Dijkstra::Node::Node(uint16 pos, uint16 distance) :
	id(pos),
	dst(distance)
{}

inline bool Dijkstra::Comp::operator()(Node a, Node b) {
	return a.dst > b.dst;
}

class TileCol {
private:
	Tile* tl;
	uint16 home, extra, size;	// home = number of home tiles, extra = home + board width, size = all tiles

public:
	TileCol();
	TileCol(const TileCol&) = delete;
	TileCol(TileCol&&) = delete;
	~TileCol();

	void update(const Com::Config& conf);
	uint16 getHome() const;
	uint16 getExtra() const;
	uint16 getSize() const;

	TileCol& operator=(const TileCol&) = delete;
	TileCol& operator=(TileCol&&) = delete;
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

inline TileCol::~TileCol() {
	delete[] tl;
}

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
	return tl;
}

inline const Tile* TileCol::begin() const {
	return tl;
}

inline Tile* TileCol::end() {
	return tl + size;
}

inline const Tile* TileCol::end() const {
	return tl + size;
}

inline Tile* TileCol::ene(pdift i) {
	return tl + i;
}

inline const Tile* TileCol::ene(pdift i) const {
	return tl + i;
}

inline Tile* TileCol::mid(pdift i) {
	return tl + home + i;
}

inline const Tile* TileCol::mid(pdift i) const {
	return tl + home + i;
}

inline Tile* TileCol::own(pdift i) {
	return tl + extra + i;
}

inline const Tile* TileCol::own(pdift i) const {
	return tl + extra + i;
}

class PieceCol {
private:
	Piece* pc;
	uint16 num, size;	// num = number of one player's pieces, i.e. size / 2

public:
	PieceCol();
	PieceCol(const PieceCol&) = delete;
	PieceCol(PieceCol&&) = delete;
	~PieceCol();

	void update(const Com::Config& conf, bool regular);
	uint16 getNum() const;
	uint16 getSize() const;

	PieceCol& operator=(const PieceCol&) = delete;
	PieceCol& operator=(PieceCol&&) = delete;
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

inline PieceCol::~PieceCol() {
	delete[] pc;
}

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
	return pc;
}

inline const Piece* PieceCol::begin() const {
	return pc;
}

inline Piece* PieceCol::end() {
	return pc + size;
}

inline const Piece* PieceCol::end() const {
	return pc + size;
}

inline Piece* PieceCol::own(pdift i) {
	return pc + i;
}

inline const Piece* PieceCol::own(pdift i) const {
	return pc + i;
}

inline Piece* PieceCol::ene(pdift i) {
	return pc + num + i;
}

inline const Piece* PieceCol::ene(pdift i) const {
	return pc + num + i;
}
