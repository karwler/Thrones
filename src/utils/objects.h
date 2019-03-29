#pragma once

#include "utils.h"

// additional data for rendering objects
struct Camera {
	vec3 pos, lat;
	double fov, znear, zfar;

	Camera(const vec3& pos = vec3(0.f, 8.f, 8.f), const vec3& lat = vec3(0.f, 0.f, 2.f), double fov = 45.0, double znear = 0.01, double zfar = 20.0);

	void update() const;
	static void updateUI();
	vec3 direction(const vec2i& mPos) const;
};

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

	vec3 pos, rot, scl;
	vec4 color;
	Info mode;
	const Texture* tex;
	vector<Vertex> verts;
	vector<ushort> elems;	// size must be a multiple of 3

public:
	Object(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const vector<Vertex>& verts = {}, const vector<ushort>& elems = {}, const Texture* tex = nullptr, const vec4& color = defaultColor, Info mode = INFO_FILL);
	virtual ~Object() override = default;

	void draw() const;

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

// square object on a single plane with coordinates {(0, 0) ... (8, 3)}
class BoardObject : public Object {
public:
	static const vector<ushort> squareElements;
private:
	static const vector<Vertex> squareVertices;

	OCall lcall, rcall, ucall;

public:
	BoardObject(vec2b pos = 0, float poz = 0.f, OCall lcall = nullptr, OCall rcall = nullptr, OCall ucall = nullptr, const Texture* tex = nullptr, const vec4& color = defaultColor, Info mode = INFO_FILL | INFO_RAYCAST);
	virtual ~BoardObject() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onDrag(const vec2i& mPos, const vec2i& mMov) override;
	virtual void onUndrag(uint8 mBut) override;

	vec2b getPos() const;
	void setPos(vec2b gpos);
	void setLcall(OCall pcl);
	void disable();

private:
	static vec3 btop(vec2b bpos, float poz);
};

inline void BoardObject::setLcall(OCall pcl) {
	lcall = pcl;
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
	Tile(vec2b pos, Type type, OCall lcall, OCall rcall, OCall ucall, Info mode);
	virtual ~Tile() override = default;

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
		throne,
		empty
	};
	static const array<string, sizet(Type::empty)> names;	// for textures	
	static const array<uint8, sizet(Type::empty)> amounts;
private:
	Type type;

public:
	Piece() = default;
	Piece(vec2b pos, Type type, OCall lcall, OCall rcall, OCall ucall, Info mode);
	virtual ~Piece() override = default;

	Type getType() const;
	void setType(Type newType);
};

inline Piece::Type Piece::getType() const {
	return type;
}
