#pragma once

#include "utils.h"

// exactly what it sounds like
struct Vertex {
	vec3 pos;
	vec3 nrm;
	vec2 tuv;

	Vertex(const vec3& pos = vec3(0.f), const vec3& nrm = vec3(0.f), const vec2& tuv = vec2(0.f));
};

// 3D object with triangles
class Object : public Interactable {
public:
	enum Info : uint8 {
		INFO_NONE    = 0x0,
		INFO_SHOW    = 0x1,	// draw at all
		INFO_TEXTURE = 0x2,	// draw texture
		INFO_LINES   = 0x4,	// draw lines
		INFO_RAYCAST = 0x8,	// be detected by raycast
		INFO_FILL    = INFO_SHOW | INFO_TEXTURE
	};

	static const vec4 defaultColor;

	vector<Vertex> verts;
	vector<ushort> elems;	// size must be a multiple of 3
	const Texture* tex;
	vec3 pos, rot, scl;
	vec4 color;
	Info mode;

public:
	Object(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const vector<Vertex>& verts = {}, const vector<ushort>& elems = {}, const Texture* tex = nullptr, const vec4& color = defaultColor, Info mode = INFO_FILL);
	virtual ~Object() override = default;

	virtual void draw() const;

	mat4 getTransform() const;
protected:
	static void setTransform(const vec3& pos, const vec3& rot, const vec3& scl);
	static void setColorization(const Texture* tex, const vec4& color, Info mode);
};

inline constexpr Object::Info operator~(Object::Info a) {
	return Object::Info(~uint8(a));
}

inline constexpr Object::Info operator&(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) & uint8(b));
}

inline constexpr Object::Info operator&=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) & uint8(b));
}

inline constexpr Object::Info operator^(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) ^ uint8(b));
}

inline constexpr Object::Info operator^=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) ^ uint8(b));
}

inline constexpr Object::Info operator|(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) | uint8(b));
}

inline constexpr Object::Info operator|=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) | uint8(b));
}

// square object on a single plane
class BoardObject : public Object {
public:
	static constexpr float upperPoz = 0.001f;
	static constexpr float halfSize = 0.5f;
	static const vec3 defaultNormal;
	static const vector<ushort> squareElements;
protected:
	static const vector<Vertex> squareVertices;
private:
	static const vec4 moveIconColor, fireIconColor;

	enum class DragState : uint8 {
		none,
		move,
		fire
	} dragState;
protected:
	OCall clcall, crcall, ulcall, urcall;

public:
	BoardObject(const vec3& pos = vec3(0.f), OCall clcall = nullptr, OCall crcall = nullptr, OCall ulcall = nullptr, OCall urcall = nullptr, const Texture* tex = nullptr, const vec4& color = defaultColor, Info mode = INFO_FILL | INFO_RAYCAST);
	virtual ~BoardObject() override = default;

	virtual void draw() const override;
	virtual void drawTop() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	void setClcall(OCall pcl);
	void setCrcall(OCall pcl);
	void setUlcall(OCall pcl);
	void setUrcall(OCall pcl);
	void setModeByInteract(bool on);
};

inline void BoardObject::setClcall(OCall pcl) {
	clcall = pcl;
}

inline void BoardObject::setCrcall(OCall pcl) {
	crcall = pcl;
}

inline void BoardObject::setUlcall(OCall pcl) {
	ulcall = pcl;
}

inline void BoardObject::setUrcall(OCall pcl) {
	urcall = pcl;
}

inline void BoardObject::setModeByInteract(bool on) {
	on ? mode |= INFO_RAYCAST : mode &= ~INFO_RAYCAST;
}

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Interactivity : uint8 {
		none,
		raycast,
		tiling,
		piecing
	};

	enum class Favor : uint8 {
		none,
		own,
		enemy
	};
	static const array<vec4, sizet(Favor::enemy)> favColors;
	static const array<vec4, Com::tileMax+1> colors;

	Favor favored;	// for non-fortress
private:
	Com::Tile type;
	bool breached;	// only for fortress

	static const array<Vertex, 12> outlineVertices;
	static const array<uint8, 16> outlineElements;
	static constexpr float outlineSize = halfSize / 5.f;
	static constexpr float outlineUV = outlineSize / (halfSize * 2);

public:
	Tile() = default;
	Tile(const vec3& pos, Com::Tile type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode);
	virtual ~Tile() override = default;

	virtual void draw() const override;
	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setCalls(Interactivity lvl);
private:
	static Info getModeByType(Info mode, Com::Tile type);
};

inline Com::Tile Tile::getType() const {
	return type;
}

inline bool Tile::isBreachedFortress() const {
	return type == Com::Tile::fortress && breached;
}

inline bool Tile::isUnbreachedFortress() const {
	return type == Com::Tile::fortress && !breached;
}

inline bool Tile::getBreached() const {
	return breached;
}

inline Object::Info Tile::getModeByType(Info mode, Com::Tile type) {
	return type != Com::Tile::empty ? mode | INFO_SHOW : mode & ~INFO_SHOW;
}

// tiles on a board
struct TileCol {
	Tile* tl;	// must be allocated at some point
	const Com::Config& conf;

	TileCol(const Com::Config& conf);
	~TileCol();

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

inline TileCol::TileCol(const Com::Config& conf) :
	conf(conf)
{}

inline TileCol::~TileCol() {
	delete[] tl;
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
	return tl + conf.boardSize;
}

inline const Tile* TileCol::end() const {
	return tl + conf.boardSize;
}

inline Tile* TileCol::ene(pdift i) {
	return tl + i;
}

inline const Tile* TileCol::ene(pdift i) const {
	return tl + i;
}

inline Tile* TileCol::mid(pdift i) {
	return tl + conf.numTiles + i;
}

inline const Tile* TileCol::mid(pdift i) const {
	return tl + conf.numTiles + i;
}

inline Tile* TileCol::own(pdift i) {
	return tl + conf.extraSize + i;
}

inline const Tile* TileCol::own(pdift i) const {
	return tl + conf.extraSize + i;
}

// player on tiles
class Piece : public BoardObject {
public:
	static const vec4 enemyColor;

	uint16 lastFortress;	// index of last visited fortress (only relevant to throne)
private:
	Com::Piece type;

public:
	Piece() = default;
	Piece(const vec3& pos, Com::Piece type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode, const vec4& color);
	virtual ~Piece() override = default;

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Com::Piece getType() const;
	void setType(Com::Piece newType);
	void setModeByOn(bool on);
	bool canFire() const;
	bool active() const;
	void enable(vec2s bpos);
	void disable();
};

inline Com::Piece Piece::getType() const {
	return type;
}

inline void Piece::setModeByOn(bool on) {
	on ? mode |= INFO_SHOW | INFO_RAYCAST : mode &= ~(INFO_SHOW | INFO_RAYCAST);
}

inline bool Piece::canFire() const {
	return type >= Com::Piece::crossbowman && type <= Com::Piece::trebuchet;
}

// pieces on a board
struct PieceCol {
	Piece* pc;	// must be allocated at some point
	const Com::Config& conf;

	PieceCol(const Com::Config& conf);
	~PieceCol();

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

inline PieceCol::PieceCol(const Com::Config& conf) :
	conf(conf)
{}

inline PieceCol::~PieceCol() {
	delete[] pc;
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
	return pc + conf.piecesSize;
}

inline const Piece* PieceCol::end() const {
	return pc + conf.piecesSize;
}

inline Piece* PieceCol::own(pdift i) {
	return pc + i;
}

inline const Piece* PieceCol::own(pdift i) const {
	return pc + i;
}

inline Piece* PieceCol::ene(pdift i) {
	return pc + conf.numPieces + i;
}

inline const Piece* PieceCol::ene(pdift i) const {
	return pc + conf.numPieces + i;
}
