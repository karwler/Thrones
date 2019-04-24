#include "engine/world.h"

// VERTEX

Vertex::Vertex(const vec3& pos, const vec3& nrm, const vec2& tuv) :
	pos(pos),
	nrm(nrm),
	tuv(tuv)
{}

// OBJECT

const vec4 Object::defaultColor(1.f);

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const vector<Vertex>& verts, const vector<ushort>& elems, const Texture* tex, const vec4& color, Info mode) :
	verts(verts),
	elems(elems),
	tex(tex),
	pos(pos),
	rot(rot),
	scl(scl),
	color(color),
	mode(mode)
{}

void Object::draw() const {
	if (!(mode & INFO_SHOW))
		return;

	if (tex && (mode & INFO_TEXTURE)) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
	glColor4fv(glm::value_ptr(color));
	
	setTransform();
	glBegin(!(mode & INFO_WIREFRAME) ? GL_TRIANGLES : GL_LINE_STRIP);
	for (ushort id : elems) {
		glTexCoord2fv(glm::value_ptr(verts[id].tuv));
		glNormal3fv(glm::value_ptr(verts[id].nrm));
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
	Vertex(vec3(-0.5f, 0.f, 0.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 1.f)),
	Vertex(vec3(-0.5f, 0.f, -0.5f), vec3(0.f, 1.f, 0.f), vec2(0.f, 0.f)),
	Vertex(vec3(0.5f, 0.f, -0.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 0.f)),
	Vertex(vec3(0.5f, 0.f, 0.5f), vec3(0.f, 1.f, 0.f), vec2(1.f, 1.f))
};

const vec4 BoardObject::moveIconColor(1.f, 1.f, 0.f, 1.f);
const vec4 BoardObject::fireIconColor(1.f, 0.f, 0.f, 1.f);

BoardObject::BoardObject(vec2b pos, float poz, OCall clcall, OCall ulcall, OCall urcall, const Texture* tex, const vec4& color, Info mode) :
	Object(btop(pos, poz), vec3(0.f), vec3(1.f), squareVertices, squareElements, tex, color, mode),
	dragState(DragState::none),
	clcall(clcall),
	ulcall(ulcall),
	urcall(urcall)
{}

void BoardObject::draw() const {
	if (dragState == DragState::none)
		Object::draw();
	else {
		vec3 ray = World::scene()->pickerRay(mousePos());
		if (vec3 loc = World::scene()->getCamera().pos + ray * (pos.y - World::scene()->getCamera().pos.y / ray.y); dragState == DragState::self)
			drawRect(loc, rot, scl, color, mode & INFO_TEXTURE ? tex : nullptr);
		else {
			Object::draw();
			drawRect(loc, rot, scl, dragState == DragState::move ? moveIconColor : fireIconColor, nullptr);	// TODO: icon textures
		}
	}
}

void BoardObject::drawRect(const vec3& pos, const vec3& rot, const vec3& scl, const vec4& color, const Texture* tex) {
	if (tex) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
	glColor4fv(glm::value_ptr(color));

	glLoadIdentity();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(rot.x, 1.f, 0.f, 0.f);
	glRotatef(rot.y, 0.f, 1.f, 0.f);
	glRotatef(rot.z, 0.f, 0.f, 1.f);
	glScalef(scl.x, scl.y, scl.z);

	glBegin(GL_QUADS);
	for (Vertex it : squareVertices) {
		glTexCoord2fv(glm::value_ptr(it.tuv));
		glNormal3fv(glm::value_ptr(it.nrm));
		glVertex3fv(glm::value_ptr(it.pos));
	}
	glEnd();
}

void BoardObject::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(clcall, this);
}

void BoardObject::onHold(const vec2i&, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT && ulcall) || (mBut == SDL_BUTTON_RIGHT && urcall)) {
		dragState = mBut == SDL_BUTTON_LEFT ? ulcall == &Program::eventMove ? DragState::move : DragState::self : DragState::fire;
		World::scene()->capture = this;
	}
}

void BoardObject::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		dragState = DragState::none;
		World::scene()->capture = nullptr;
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this);
	}
}

void BoardObject::disable() {
	setPos(INT8_MIN);
	mode &= ~(Object::INFO_SHOW | Object::INFO_RAYCAST);
}

// TILE

const array<vec4, Tile::colors.size()> Tile::colors = {
	vec4(0.157f, 1.f, 0.157f, 1.f),
	vec4(0.f, 0.627f, 0.f, 1.f),
	vec4(0.471f, 0.471f, 0.471f, 1.f),
	vec4(0.157f, 0.784f, 1.f, 1.f),
	vec4(0.549f, 0.275f, 0.078f, 1.f),
	vec4(0.235f, 0.235f, 0.235f, 1.f)
};

const array<string, Tile::names.size()> Tile::names = {
	"plains",
	"forest",
	"mountain",
	"water",
	"fortress",
	"empty"
};

const array<uint8, Tile::amounts.size()> Tile::amounts = {
	11,
	10,
	7,
	7
};

Tile::Tile(vec2b pos, Type type, OCall clcall, OCall ulcall, OCall urcall, Info mode) :
	BoardObject(pos, 0.f, clcall, ulcall, urcall, nullptr, colors[uint8(type)], getModeByType(mode, type)),
	ruined(false),
	type(type)
{}

void Tile::setType(Type newType) {
	type = newType;
	color = colors[uint8(type)];
	mode = getModeByType(mode, type);
}

// PIECE

const array<string, Piece::names.size()> Piece::names = {
	"ranger",
	"spearman",
	"crossbowman",
	"catapult",
	"trebuchet",
	"lancer",
	"warhorse",
	"elephant",
	"dragon",
	"throne"
};

const array<uint8, Piece::amounts.size()> Piece::amounts = {
	2,
	2,
	2,
	1,
	1,
	2,
	1,
	2,
	1,
	1
};

Piece::Piece(vec2b pos, Type type, OCall lcall, OCall rcall, OCall ucall, Info mode) :
	BoardObject(pos, 0.01f, lcall, rcall, ucall, World::winSys()->texture(names[uint8(type)]), defaultColor, mode),
	type(type)
{}

void Piece::setType(Piece::Type newType) {
	type = newType;
	tex = World::winSys()->texture(names[uint8(type)]);
}
