#pragma once

#include "utils.h"

struct Camera {
	vec3 pos, lat;
	double fov, znear, zfar;

	Camera(const vec3& pos = vec3(0.f, 1.f, 4.f), const vec3& lat = vec3(0.f), double fov = 45.0, double znear = 0.001, double zfar = 1000.0);

	void update() const;
};

struct Vertex {
	vec3 pos;
	vec2 tuv;

	Vertex(const vec3& pos = vec3(0.f), const vec2& tuv = vec2(0.f));
};

struct Object {
	vec3 pos, rot, scl;
	vector<Vertex> verts;
	vector<uint> elems;
	Texture* tex;
	SDL_Color color;

	Object(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const vector<Vertex>& verts = {}, const vector<uint>& elems = {}, Texture* tex = nullptr, SDL_Color color = {255, 255, 255, 255});

	void draw() const;

	static Object createFaceSquare(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), Texture* tex = nullptr, SDL_Color color = {255, 255, 255, 255});
};
