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

	setColorization(tex, color, mode);
	setTransform(pos, rot, scl);

	glBegin(!(mode & INFO_LINES) ? GL_TRIANGLES : GL_LINES);
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

void Object::setTransform(const vec3& pos, const vec3& rot, const vec3& scl) {
	glLoadIdentity();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(rot.x, 1.f, 0.f, 0.f);
	glRotatef(rot.y, 0.f, 1.f, 0.f);
	glRotatef(rot.z, 0.f, 0.f, 1.f);
	glScalef(scl.x, scl.y, scl.z);
}

void Object::setColorization(const Texture* tex, const vec4& color, Info mode) {
	if (tex && (mode & INFO_TEXTURE)) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
	glColor4fv(glm::value_ptr(color));
}

// GAME OBJECT

const vector<ushort> BoardObject::squareElements = {
	0, 1, 2,
	2, 3, 0
};

const vector<Vertex> BoardObject::squareVertices = {
	Vertex(vec3(-halfSize, 0.f, -halfSize), defaultNormal, vec2(0.f, 0.f)),
	Vertex(vec3(halfSize, 0.f, -halfSize), defaultNormal, vec2(1.f, 0.f)),
	Vertex(vec3(halfSize, 0.f, halfSize), defaultNormal, vec2(1.f, 1.f)),
	Vertex(vec3(-halfSize, 0.f, halfSize), defaultNormal, vec2(0.f, 1.f))
};

const vec3 BoardObject::defaultNormal(0.f, 1.f, 0.f);
const vec4 BoardObject::moveIconColor(0.9f, 0.9f, 0.9f, 0.9f);
const vec4 BoardObject::fireIconColor(1.f, 0.1f, 0.1f, 0.9f);

BoardObject::BoardObject(const vec3& pos, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, const Texture* tex, const vec4& color, Info mode) :
	Object(pos, vec3(0.f), vec3(1.f), squareVertices, squareElements, tex, color, mode),
	dragState(DragState::none),
	clcall(clcall),
	crcall(crcall),
	ulcall(ulcall),
	urcall(urcall)
{}

void BoardObject::draw() const {
	if (dragState != DragState::move)
		Object::draw();
}

void BoardObject::drawTop() const {
	if (dragState != DragState::none) {
		vec3 ray = World::scene()->pickerRay(mousePos());
		setColorization(dragState == DragState::move ? tex : World::winSys()->texture("crosshair"), dragState == DragState::move ? color * moveIconColor : fireIconColor, mode);
		setTransform(World::scene()->getCamera()->pos + ray * (pos.y - upperPoz * 2.f - World::scene()->getCamera()->pos.y / ray.y), rot, scl);

		glBegin(GL_QUADS);
		for (const Vertex& it : squareVertices) {
			glTexCoord2fv(glm::value_ptr(it.tuv));
			glNormal3fv(glm::value_ptr(it.nrm));
			glVertex3fv(glm::value_ptr(it.pos));
		}
		glEnd();
	}
}

void BoardObject::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(clcall, this);
	else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(crcall, this);
}

void BoardObject::onHold(const vec2i&, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT && ulcall) || (mBut == SDL_BUTTON_RIGHT && urcall)) {
		dragState = mBut == SDL_BUTTON_LEFT ? DragState::move : DragState::fire;
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

// TILE

const array<vec4, Tile::colors.size()> Tile::colors = {
	vec4(0.157f, 1.f, 0.157f, 1.f),
	vec4(0.f, 0.627f, 0.f, 1.f),
	vec4(0.471f, 0.471f, 0.471f, 1.f),
	vec4(0.157f, 0.784f, 1.f, 1.f),
	vec4(0.549f, 0.275f, 0.078f, 1.f),
	vec4(0.3f, 0.3f, 0.3f, 1.f)
};

const array<vec4, Tile::favColors.size()> Tile::favColors = {
	vec4(1.f, 0.843f, 0.f, 1.f),
	vec4(1.f, 0.f, 0.f, 1.f)
};

const array<Vertex, Tile::outlineVertices.size()> Tile::outlineVertices = {
	Vertex(squareVertices[0].pos + vec3(0.f, upperPoz / 2.f, 0.f), defaultNormal, vec2(0.f, 0.f)),
	Vertex(squareVertices[1].pos + vec3(0.f, upperPoz / 2.f, 0.f), defaultNormal, vec2(1.f, 0.f)),
	Vertex(squareVertices[1].pos + vec3(0.f, upperPoz / 2.f, outlineSize), defaultNormal, vec2(1.f, outlineUV)),
	Vertex(squareVertices[0].pos + vec3(0.f, upperPoz / 2.f, outlineSize), defaultNormal, vec2(0.f, outlineUV)),
	Vertex(squareVertices[3].pos + vec3(0.f, upperPoz / 2.f, -outlineSize), defaultNormal, vec2(0.f, 1.f - outlineUV)),
	Vertex(squareVertices[2].pos + vec3(0.f, upperPoz / 2.f, -outlineSize), defaultNormal, vec2(1.f, 1.f - outlineUV)),
	Vertex(squareVertices[2].pos + vec3(0.f, upperPoz / 2.f, 0.f), defaultNormal, vec2(1.f, 1.f)),
	Vertex(squareVertices[3].pos + vec3(0.f, upperPoz / 2.f, 0.f), defaultNormal, vec2(0.f, 1.f)),
	Vertex(squareVertices[0].pos + vec3(outlineSize, upperPoz / 2.f, outlineSize), defaultNormal, vec2(outlineUV)),
	Vertex(squareVertices[3].pos + vec3(outlineSize, upperPoz / 2.f, -outlineSize), defaultNormal, vec2(outlineUV, 1.f - outlineUV)),
	Vertex(squareVertices[2].pos + vec3(-outlineSize, upperPoz / 2.f, -outlineSize), defaultNormal, vec2(1.f - outlineUV)),
	Vertex(squareVertices[1].pos + vec3(-outlineSize, upperPoz / 2.f, outlineSize), defaultNormal, vec2(1.f - outlineUV, outlineUV)),
};

const array<uint8, Tile::outlineElements.size()> Tile::outlineElements = {
	0, 1, 2, 3,
	4, 5, 6, 7,
	3, 8, 9, 4,
	11, 2, 5, 10
};

Tile::Tile(const vec3& pos, Com::Tile type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode) :
	BoardObject(pos, clcall, crcall, ulcall, urcall, nullptr, colors[uint8(type)], getModeByType(mode, type)),
	favored(Favor::none),
	type(type),
	breached(false)
{}

void Tile::draw() const {
	BoardObject::draw();
	if (favored != Favor::none) {
		glDisable(GL_TEXTURE_2D);
		glColor4fv(glm::value_ptr(favColors[uint8(favored)-1]));

		glBegin(GL_QUADS);
		for (uint8 id : outlineElements) {
			glTexCoord2fv(glm::value_ptr(outlineVertices[id].tuv));
			glNormal3fv(glm::value_ptr(outlineVertices[id].nrm));
			glVertex3fv(glm::value_ptr(outlineVertices[id].pos));
		}
		glEnd();
	}
}

void Tile::onText(const string&) {}

void Tile::setType(Com::Tile newType) {
	type = newType;
	color = colors[uint8(type)];
	mode = getModeByType(mode, type);
}

void Tile::setBreached(bool yes) {
	breached = yes;
	color = colors[uint8(type)] * (breached ? 0.5f : 1.f);
}

void Tile::setCalls(Interactivity lvl) {
	setModeByInteract(lvl != Interactivity::none);
	switch (lvl) {
	case Interactivity::tiling:
		clcall = &Program::eventPlaceTileC;
		if (type != Com::Tile::empty) {
			crcall = &Program::eventClearTile;
			ulcall = &Program::eventMoveTile;
		} else {
			crcall = nullptr;
			ulcall = nullptr;
		}
		break;
	case Interactivity::piecing:
		clcall = &Program::eventPlacePieceC;
		crcall = &Program::eventClearPiece;
		ulcall = nullptr;
		break;
	default:
		clcall = nullptr;
		crcall = nullptr;
		ulcall = nullptr;
	}
}

// PIECE

const vec4 Piece::enemyColor(0.5f, 0.5f, 1.f, 1.f);

Piece::Piece(const vec3& pos, Com::Piece type, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, Info mode, const vec4& color) :
	BoardObject(pos, clcall, crcall, ulcall, urcall, World::winSys()->texture(Com::pieceNames[uint8(type)]), color, mode),
	lastFortress(INT16_MAX),
	type(type)
{}

void Piece::onText(const string&) {}

void Piece::setType(Com::Piece newType) {
	type = newType;
	tex = World::winSys()->texture(Com::pieceNames[uint8(type)]);
}

bool Piece::active() const {
	return World::game()->ptog(pos).hasNot(INT16_MIN) && (mode & (INFO_SHOW | INFO_RAYCAST));
}

void Piece::enable(vec2s bpos) {
	pos = World::game()->gtop(bpos, upperPoz);
	setModeByOn(true);
}

void Piece::disable() {
	pos = World::game()->gtop(INT16_MIN, upperPoz);
	setModeByOn(false);
}
