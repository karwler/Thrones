#pragma once

#include "utils.h"

// additional data for rendering objects
struct Camera {
	vec3 pos, lat;
	double fov, znear, zfar;

	Camera(const vec3& pos = vec3(0.f, 4.f, 8.f), const vec3& lat = vec3(0.f), double fov = 45.0, double znear = 0.001, double zfar = 1000.0);

	void update() const;
	static void updateUI();
	vec3 direction(const vec2i& mPos) const;
};

// exactly what it sounds like
struct Vertex {
	vec3 pos;
	vec2 tuv;

	Vertex(const vec3& pos = vec3(0.f), const vec2& tuv = vec2(0.f));
};

// generic 3D object
class Object {
public:
	vec3 pos, rot, scl;
	vector<Vertex> verts;
	vector<ushort> elems;
	Texture* tex;
	SDL_Color color;

public:
	Object(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const vector<Vertex>& verts = {}, const vector<ushort>& elems = {}, Texture* tex = nullptr, SDL_Color color = {255, 255, 255, 255});

	void draw() const;

	mat4 getTransform() const;
private:
	void setTransform() const;
};

// square object on a single plane with coordinates {(0, 0) ... (8, 3)}
class BoardObject : public Object {
public:
	static const vector<ushort> squareElements;
private:
	static const vector<Vertex> squareVertices;

public:
	BoardObject(vec2b pos = 0, Texture* tex = nullptr, SDL_Color color = {255, 255, 255, 255});

	vec2b getPos() const;
	void setPos(vec2b gpos);

private:
	static vec3 btop(vec2b bpos);
};

inline vec2b BoardObject::getPos() const {
	return vec2b(pos.x - 4.f, pos.z);	// game field starts at (-4, 0)
}

inline void BoardObject::setPos(vec2b gpos) {
	pos = btop(gpos);
}

inline vec3 BoardObject::btop(vec2b bpos) {
	return vec3(bpos.x + 4, 0.f, bpos.y);
}

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Type : uint8 {
		plains,
		forest,
		mountain,
		water
	};

private:
	static const array<SDL_Color, sizet(Type::water)+1> colors;

	Type type;

public:
	Tile() = default;
	Tile(vec2b pos, Type type);

	Type getType() const;
};

inline Tile::Type Tile::getType() const {
	return type;
}

// player on tiles
class Piece : public BoardObject {
public:
	enum class Type : uint8 {
		throne
	};
private:
	Type type;

public:
	Piece() = default;
	Piece(vec2b pos, Type type);

	Type getType() const;
};

inline Piece::Type Piece::getType() const {
	return type;
}
