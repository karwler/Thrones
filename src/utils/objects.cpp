#include "engine/world.h"
#include <glm/gtc/type_ptr.hpp>

// CAMERA

Camera::Camera(const vec3& pos, const vec3& lat, double fov, double znear, double zfar) :
	pos(pos),
	lat(lat),
	fov(fov),
	znear(znear),
	zfar(zfar)
{}

void Camera::update() const {
	vec2d res = World::winSys()->windowSize();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, res.x / res.y, znear, zfar);
	gluLookAt(double(pos.x), double(pos.y), double(pos.z), double(lat.x), double(lat.y), double(lat.z), 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
}

// VERTEX

Vertex::Vertex(const vec3& pos, const vec2& tuv) :
	pos(pos),
	tuv(tuv)
{}

// OBJECT

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const vector<Vertex>& verts, const vector<uint>& elems, Texture* tex, SDL_Color color) :
	pos(pos),
	rot(rot),
	scl(scl),
	verts(verts),
	elems(elems),
	tex(tex),
	color(color)
{}

void Object::draw() const {
	glLoadIdentity();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(rot.x, 1.f, 0.f, 0.f);
	glRotatef(rot.y, 0.f, 1.f, 0.f);
	glRotatef(rot.z, 0.f, 0.f, 1.f);
	glScalef(scl.x, scl.y, scl.z);

	if (tex) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
	glColor4bv(reinterpret_cast<const GLbyte*>(&color));
	
	glBegin(GL_TRIANGLES);
	for (uint id : elems) {
		glTexCoord2fv(glm::value_ptr(verts[id].tuv));
		glVertex3fv(glm::value_ptr(verts[id].pos));
	}
	glEnd();
}

Object Object::createFaceSquare(const vec3& pos, const vec3& rot, const vec3& scl, Texture* tex, SDL_Color color) {
	vector<Vertex> verts = {
		Vertex(vec3(-0.5f, 0.5f, 0.f), vec2(1.f, 0.f)),
		Vertex(vec3(-0.5f, -0.5f, 0.f), vec2(1.f, 1.f)),
		Vertex(vec3(0.5f, -0.5f, 0.f), vec2(0.f, 1.f)),
		Vertex(vec3(0.5f, 0.5f, 0.f), vec2(0.f, 0.f))
	};
	vector<uint> elems = {
		0, 1, 2,
		2, 3, 0
	};
	return Object(pos, rot, scl, verts, elems, tex, color);
}
