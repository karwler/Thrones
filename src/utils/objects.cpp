#include "engine/world.h"
#include <glm/gtc/matrix_transform.hpp>
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, vec2d(World::winSys()->windowSize()).ratio(), znear, zfar);
	gluLookAt(double(pos.x), double(pos.y), double(pos.z), double(lat.x), double(lat.y), double(lat.z), 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
}

void Camera::updateUI() {
	vec2d res = World::winSys()->windowSize();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, res.x, res.y, 0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

vec3 Camera::direction(const vec2i& mPos) const {
	vec2 res = World::winSys()->windowSize().glm();
	vec2 mouse = mPos.glm() / (res / 2.f) - 1.f;

	mat4 proj = glm::perspective(glm::radians(float(fov)), res.x / res.y, float(znear), float(zfar));
	mat4 view = glm::lookAt(pos, lat, vec3(0.f, 1.f, 0.f));
	return glm::normalize(glm::inverse(proj * view) * vec4(mouse.x, -mouse.y, 1.f, 1.f));
}

// VERTEX

Vertex::Vertex(const vec3& pos, const vec2& tuv) :
	pos(pos),
	tuv(tuv)
{}

// OBJECT

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const vector<Vertex>& verts, const vector<ushort>& elems, Texture* tex, SDL_Color color) :
	pos(pos),
	rot(rot),
	scl(scl),
	verts(verts),
	elems(elems),
	tex(tex),
	color(color)
{}

void Object::draw() const {
	setTransform();
	if (tex) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
	glColor4ubv(reinterpret_cast<const GLubyte*>(&color));
	
	glBegin(GL_TRIANGLES);
	for (ushort id : elems) {
		glTexCoord2fv(glm::value_ptr(verts[id].tuv));
		glVertex3fv(glm::value_ptr(verts[id].pos));
	}
	glEnd();
}

mat4 Object::getTransform() const {
	mat4 trans = glm::translate(mat4(1.f), pos);
	trans = glm::rotate(trans, rot.x, vec3(1.f, 0.f, 0.f));
	trans = glm::rotate(trans, rot.y, vec3(0.f, 1.f, 0.f));
	trans = glm::rotate(trans, rot.z, vec3(0.f, 0.f, 1.f));
	return glm::scale(trans, scl);
}

void Object::setTransform() const {
	glLoadIdentity();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(rot.x, 1.f, 0.f, 0.f);
	glRotatef(rot.y, 0.f, 1.f, 0.f);
	glRotatef(rot.z, 0.f, 0.f, 1.f);
	glScalef(scl.x, scl.y, scl.z);
}

// GAME OBJECT

const vector<ushort> BoardObject::squareElements = {
	0, 1, 2,
	2, 3, 0
};

const vector<Vertex> BoardObject::squareVertices = {
	Vertex(vec3(-0.5f, 0.f, 0.5f), vec2(1.f, 0.f)),
	Vertex(vec3(-0.5f, 0.f, -0.5f), vec2(1.f, 1.f)),
	Vertex(vec3(0.5f, 0.f, -0.5f), vec2(0.f, 1.f)),
	Vertex(vec3(0.5f, 0.f, 0.f), vec2(0.f, 0.f))
};

BoardObject::BoardObject(vec2b pos, Texture* tex, SDL_Color color) :
	Object(btop(pos), vec3(0.f), vec3(0.f), squareVertices, squareElements, tex, color)
{}

// TILE

const array<SDL_Color, Tile::colors.size()> Tile::colors = {
	SDL_Color({40, 255, 40, 255}),
	SDL_Color({0, 160, 0, 255}),
	SDL_Color({120, 120, 120, 255}),
	SDL_Color({80, 255, 255, 255})
};

Tile::Tile(vec2b pos, Type type) :
	BoardObject(pos, nullptr, colors[uint8(type)]),
	type(type)
{}

// PIECE

Piece::Piece(vec2b pos, Type type) :
	BoardObject(pos, nullptr, {255, 255, 255, 255}),
	type(type)
{}
