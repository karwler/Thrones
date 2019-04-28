#pragma once

#include "utils.h"

// exactly what it sounds like
struct Vertex {
	vec3 pos;
	vec3 nrm;
	vec2 tuv;

	Vertex(const vec3& pos = vec3(0.f), const vec3& nrm = vec3(0.f), const vec2& tuv = vec2(0.f));
};

// generic 3D object
class Object : public Interactable {
public:
	enum Info : uint8 {
		INFO_NONE      = 0x0,
		INFO_SHOW      = 0x1,	// draw at all
		INFO_TEXTURE   = 0x2,	// draw texture
		INFO_WIREFRAME = 0x4,	// draw lines
		INFO_RAYCAST   = 0x8,	// be detected by raycast
		INFO_FILL      = INFO_SHOW | INFO_TEXTURE,
		INFO_LINES     = INFO_SHOW | INFO_WIREFRAME,
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
private:
	void setTransform() const;
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

// square object on a single plane with coordinates { (0, 0) ... (8, 3) }
class BoardObject : public Object {
public:
	static const vector<ushort> squareElements;
private:
	static const vector<Vertex> squareVertices;
	static const vec4 moveIconColor, fireIconColor;

	enum class DragState : uint8 {
		none,
		move,
		fire
	};

	DragState dragState;
	OCall clcall, crcall, ulcall, urcall;

public:
	BoardObject(vec2b pos = 0, float poz = 0.f, OCall clcall = nullptr, OCall crcall = nullptr, OCall ulcall = nullptr, OCall urcall = nullptr, const Texture* tex = nullptr, const vec4& color = defaultColor, Info mode = INFO_FILL | INFO_RAYCAST);
	virtual ~BoardObject() override = default;

	virtual void draw() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	vec2b getPos() const;
	void setPos(vec2b gpos);
	void setClcall(OCall pcl);
	void setCrcall(OCall pcl);
	void setUlcall(OCall pcl);
	void setUrcall(OCall pcl);
	void disable();

private:
	static void drawRect(const vec3& pos, const vec3& rot, const vec3& scl, const vec4& color, const Texture* tex);
	static vec3 btop(vec2b bpos, float poz);
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

inline vec2b BoardObject::getPos() const {
	return vec2b(uint8(pos.x) + 4, pos.z);	// game field starts at (-4, 0)
}

inline void BoardObject::setPos(vec2b gpos) {
	pos = btop(gpos, pos.y);
}

inline vec3 BoardObject::btop(vec2b bpos, float poz = 0.f) {
	return vec3(bpos.x - 4, poz, bpos.y);
}

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Type : uint8 {
		plains,
		forest,
		mountain,
		water,
		fortress,
		empty
	};
	static const array<vec4, sizet(Type::empty)+1> colors;
	static const array<string, sizet(Type::empty)+1> names;
	static const array<uint8, sizet(Type::empty)> amounts;

	bool ruined;	// only for fortress
private:
	Type type;

public:
	Tile() = default;
	Tile(vec2b pos, Type type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode);
	virtual ~Tile() override = default;

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Type getType() const;
	void setType(Type newType);
private:
	static Info getModeByType(Info mode, Type type);
};

inline Tile::Type Tile::getType() const {
	return type;
}

inline Object::Info Tile::getModeByType(Info mode, Type type) {
	return type != Type::empty ? mode & ~INFO_LINES | INFO_FILL : mode & ~INFO_FILL | INFO_LINES;
}

// tiles on a board
struct TileCol {
	Tile ene[Com::numTiles], mid[Com::boardLength], own[Com::numTiles];
	// TODO: need more logic in here to simplify game.cpp
	Tile& operator[](sizet i);
	const Tile& operator[](sizet i) const;
	Tile* begin();
	const Tile* begin() const;
	Tile* end();
	const Tile* end() const;
};

inline Tile& TileCol::operator[](sizet i) {
	return reinterpret_cast<Tile*>(this)[i];
}

inline const Tile& TileCol::operator[](sizet i) const {
	return reinterpret_cast<const Tile*>(this)[i];
}

inline Tile* TileCol::begin() {
	return reinterpret_cast<Tile*>(this);
}

inline const Tile* TileCol::begin() const {
	return reinterpret_cast<const Tile*>(this);
}

inline Tile* TileCol::end() {
	return begin() + Com::boardSize;
}

inline const Tile* TileCol::end() const {
	return begin() + Com::boardSize;
}

// player on tiles
class Piece : public BoardObject {
public:
	enum class Type : uint8 {
		ranger,
		spearman,
		crossbowman,
		catapult,
		trebuchet,
		lancer,
		warhorse,
		elephant,
		dragon,
		throne
	};
	static const array<string, sizet(Type::throne)+1> names;	// for textures
	static const array<uint8, sizet(Type::throne)+1> amounts;
private:
	Type type;

public:
	Piece() = default;
	Piece(vec2b pos, Type type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode);
	virtual ~Piece() override = default;

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Type getType() const;
	void setType(Type newType);
};

inline Piece::Type Piece::getType() const {
	return type;
}

// pieces on a board
struct PieceCol {
	Piece own[Com::numPieces], ene[Com::numPieces];
	// TODO: need more logic in here to simplify game.cpp
	Piece& operator[](sizet i);
	const Piece& operator[](sizet i) const;
	Piece* begin();
	const Piece* begin() const;
	Piece* end();
	const Piece* end() const;
};

inline Piece& PieceCol::operator[](sizet i) {
	return reinterpret_cast<Piece*>(this)[i];
}

inline const Piece& PieceCol::operator[](sizet i) const {
	return reinterpret_cast<const Piece*>(this)[i];
}

inline Piece* PieceCol::begin() {
	return reinterpret_cast<Piece*>(this);
}

inline const Piece* PieceCol::begin() const {
	return reinterpret_cast<const Piece*>(this);
}

inline Piece* PieceCol::end() {
	return begin() + Com::piecesSize;
}

inline const Piece* PieceCol::end() const {
	return begin() + Com::piecesSize;
}
