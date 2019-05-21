#include "engine/world.h"

// VERTEX

Vertex::Vertex(ushort v, ushort t, ushort n) :
	v(v),
	t(t),
	n(n)
{}

// BLUEPRINT

const vector<vec3> Blueprint::squareVertices = {
	vec3(-0.5f, 0.f, -0.5f),
	vec3(0.5f, 0.f, -0.5f),
	vec3(0.5f, 0.f, 0.5f),
	vec3(-0.5f, 0.f, 0.5f)
};

const vector<vec2> Blueprint::squareTextureUVs = {
	vec2(0.f, 0.f),
	vec2(1.f, 0.f),
	vec2(1.f, 1.f),
	vec2(0.f, 1.f)
};

const vector<vec3> Blueprint::squareNormals = {
	vec3(0.f, 1.f, 0.f)
};

const vector<Vertex> Blueprint::squareElements = {
	{ 0, 0, 0 }, { 1, 1, 0 }, { 2, 2, 0 },
	{ 2, 2, 0 }, { 3, 3, 0 }, { 0, 0, 0 }
};

const vector<vec3> Blueprint::outlineVertices = {
	squareVertices[0],
	squareVertices[1],
	squareVertices[1] + vec3(0.f, 0.f, outlOfs),
	squareVertices[0] + vec3(0.f, 0.f, outlOfs),
	squareVertices[3] + vec3(0.f, 0.f, -outlOfs),
	squareVertices[2] + vec3(0.f, 0.f, -outlOfs),
	squareVertices[2],
	squareVertices[3],
	squareVertices[0] + vec3(outlOfs, 0.f, outlOfs),
	squareVertices[3] + vec3(outlOfs, 0.f, -outlOfs),
	squareVertices[2] + vec3(-outlOfs, 0.f, -outlOfs),
	squareVertices[1] + vec3(-outlOfs, 0.f, outlOfs)
};

const vector<vec2> Blueprint::outlineTextureUVs = {
	vec2(0.f, 0.f),
	vec2(1.f, 0.f),
	vec2(1.f, outlOfs),
	vec2(0.f, outlOfs),
	vec2(0.f, 1.f - outlOfs),
	vec2(1.f, 1.f - outlOfs),
	vec2(1.f, 1.f),
	vec2(0.f, 1.f),
	vec2(outlOfs),
	vec2(outlOfs, 1.f - outlOfs),
	vec2(1.f - outlOfs),
	vec2(1.f - outlOfs, outlOfs)
};

const vector<Vertex> Blueprint::outlineElements = {
	{ 0, 0, 0 }, { 1, 1, 0 }, { 2, 2, 0 },
	{ 2, 2, 0 }, { 3, 3, 0 }, { 0, 0, 0 },
	{ 4, 4, 0 }, { 5, 5, 0 }, { 6, 6, 0 },
	{ 6, 6, 0 }, { 7, 7, 0 }, { 4, 4, 0 },
	{ 3, 3, 0 }, { 8, 8, 0 }, { 9, 9, 0 },
	{ 9, 9, 0 }, { 4, 4, 0 }, { 3, 3, 0 },
	{ 11, 11, 0 }, { 2, 2, 0 }, { 5, 5, 0 },
	{ 5, 5, 0 }, { 10, 10, 0 }, { 11, 11, 0 }
};

Blueprint::Blueprint(const vector<glm::vec3>& verts, const vector<glm::vec2>& tuvs, const vector<glm::vec3>& norms, const vector<Vertex>& elems) :
	verts(verts),
	tuvs(tuvs),
	norms(norms),
	elems(elems)
{}

void Blueprint::draw(GLenum mode) const {
	glBegin(mode);
	for (const Vertex& id : elems) {
		glTexCoord2fv(glm::value_ptr(tuvs[id.t]));
		glNormal3fv(glm::value_ptr(norms[id.n]));
		glVertex3fv(glm::value_ptr(verts[id.v]));
	}
	glEnd();
}

// OBJECT

const vec4 Object::defaultColor(1.f);

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const Blueprint* bpr, const Texture* tex, const vec4& color, Info mode) :
	bpr(bpr),
	tex(tex),
	pos(pos),
	rot(rot),
	scl(scl),
	color(color),
	mode(mode)
{}

void Object::draw() const {
	if (mode & INFO_SHOW) {
		setColorization(tex, color, mode);
		setTransform(pos, rot, scl);
		bpr->draw(!(mode & INFO_LINES) ? GL_TRIANGLES : GL_LINES);
	}
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

const vec4 BoardObject::moveIconColor(0.9f, 0.9f, 0.9f, 0.9f);
const vec4 BoardObject::fireIconColor(1.f, 0.1f, 0.1f, 0.9f);

BoardObject::BoardObject(const vec3& pos, OCall clcall, OCall crcall, OCall ulcall, OCall urcall, const Texture* tex, const vec4& color, Info mode) :
	Object(pos, vec3(0.f), vec3(1.f), World::scene()->blueprint(Scene::bprRect), tex, color, mode),
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
		bpr->draw(GL_TRIANGLES);
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
		World::scene()->blueprint(Scene::bprOutline)->draw(GL_TRIANGLES);
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

// TILE COL

TileCol::TileCol(const Com::Config& conf) :
	home(conf.numTiles),
	extra(conf.extraSize),
	size(conf.boardSize),
	tl(new Tile[size])
{}

void TileCol::update(const Com::Config& conf) {
	if (conf.boardSize != size) {
		home = conf.numTiles;
		extra = conf.extraSize;
		size = conf.boardSize;

		delete[] tl;
		tl = new Tile[size];
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

// PIECE COL

PieceCol::PieceCol(const Com::Config& conf) :
	num(conf.numPieces),
	size(conf.piecesSize),
	pc(new Piece[size])
{}

void PieceCol::update(const Com::Config& conf) {
	if (conf.piecesSize != size) {
		num = conf.numPieces;
		size = conf.piecesSize;

		delete[] pc;
		pc = new Piece[size];
	}
}
