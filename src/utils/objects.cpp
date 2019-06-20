#include "engine/world.h"

// MATERIAL

Material::Material(const vec4& ambient, const vec4& diffuse, const vec4& specular, const vec4& emission, float shine) :
	ambient(ambient),
	diffuse(diffuse),
	specular(specular),
	emission(emission),
	shine(shine)
{}

void Material::updateColor(const vec4& ambient, const vec4& diffuse, const vec4& specular, const vec4& emission, float shine) {
	glMaterialfv(GL_FRONT, GL_AMBIENT, glm::value_ptr(ambient));
	glMaterialfv(GL_FRONT, GL_DIFFUSE, glm::value_ptr(diffuse));
	glMaterialfv(GL_FRONT, GL_SPECULAR, glm::value_ptr(specular));
	glMaterialfv(GL_FRONT, GL_EMISSION, glm::value_ptr(emission));
	glMaterialf(GL_FRONT, GL_SHININESS, shine);
}

// VERTEX

Vertex::Vertex(ushort v, ushort t, ushort n) :
	v(v),
	t(t),
	n(n)
{}

// BLUEPRINT

Blueprint::Blueprint(vector<vec3> verts, vector<vec2> tuvs, vector<vec3> norms, vector<Vertex> elems) :
	verts(std::move(verts)),
	tuvs(std::move(tuvs)),
	norms(std::move(norms)),
	elems(std::move(elems))
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

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const Blueprint* bpr, const Material* mat, const Texture* tex, Info mode) :
	bpr(bpr),
	mat(mat),
	tex(tex),
	pos(pos),
	rot(rot),
	scl(scl),
	mode(mode)
{}

void Object::draw() const {
	if (mode & INFO_SHOW) {
		mat->updateColor();
		updateTexture(tex, mode);
		updateTransform(pos, rot, scl);
		bpr->draw(!(mode & INFO_LINES) ? GL_TRIANGLES : GL_LINES);
	}
}

void Object::updateTransform(const vec3& pos, const vec3& rot, const vec3& scl) {
	glLoadIdentity();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(rot.x, 1.f, 0.f, 0.f);
	glRotatef(rot.y, 0.f, 1.f, 0.f);
	glRotatef(rot.z, 0.f, 0.f, 1.f);
	glScalef(scl.x, scl.y, scl.z);
}

void Object::updateTexture(const Texture* tex, Info mode) {
	if (tex && (mode & INFO_TEXTURE)) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->getID());
	} else
		glDisable(GL_TEXTURE_2D);
}

// GAME OBJECT

const vec4 BoardObject::moveIconColor(0.9f, 0.9f, 0.9f, 0.9f);
const vec4 BoardObject::fireIconColor(1.f, 0.1f, 0.1f, 0.9f);

BoardObject::BoardObject(const vec3& pos, float size, OCall hgcall, OCall ulcall, OCall urcall, const Material* mat, const Texture* tex, Info mode) :
	Object(pos, vec3(0.f), vec3(size, 1.f, size), World::scene()->blueprint("plane"), mat, tex, mode),
	dragState(DragState::none),
	hgcall(hgcall),
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
		if (dragState == DragState::move) {
			mat->updateColorDiffuse(mat->diffuse * moveIconColor);
			updateTexture(tex, mode);
		} else {
			mat->updateColorDiffuse(fireIconColor);
			updateTexture(World::winSys()->texture("crosshair"), mode);
		}
		updateTransform(World::scene()->getCamera()->pos + ray * (pos.y - upperPoz * 2.f - World::scene()->getCamera()->pos.y / ray.y), rot, scl);
		bpr->draw(GL_TRIANGLES);
	}
}

void BoardObject::onHold(vec2i, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT && ulcall) || (mBut == SDL_BUTTON_RIGHT && urcall)) {
		dragState = mBut == SDL_BUTTON_LEFT ? DragState::move : DragState::fire;
		World::scene()->capture = this;
		World::prun(hgcall, this);
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

Tile::Tile(const vec3& pos, float size, Com::Tile type, OCall hgcall, OCall ulcall, OCall urcall, Info mode) :
	BoardObject(pos, size, hgcall, ulcall, urcall, type != Com::Tile::empty ? World::scene()->material(Com::tileNames[uint8(type)]) : nullptr, nullptr, getModeShow(mode, type)),
	type(type),
	breached(false)
{}

void Tile::onText(const string&) {}

void Tile::setType(Com::Tile newType) {
	type = newType;
	mat = newType != Com::Tile::empty ? World::scene()->material(Com::tileNames[uint8(newType)]) : nullptr;
	mode = getModeShow(mode, type);
}

void Tile::setBreached(bool yes) {
	breached = yes;
	mat = World::scene()->material(breached ? "rubble" : Com::tileNames[uint8(type)]);
}

void Tile::setInteractivity(Interactivity lvl) {
	setRaycast(lvl != Interactivity::off);
	setUlcall(lvl == Interactivity::tiling && type != Com::Tile::empty ? &Program::eventMoveTile : nullptr);
}

// TILE COL

TileCol::TileCol(const Com::Config& conf) :
	tl(new Tile[conf.boardSize]),
	home(conf.numTiles),
	extra(conf.extraSize),
	size(conf.boardSize)
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

Piece::Piece(const vec3& pos, float size, Com::Piece type, OCall hgcall, OCall ulcall, OCall urcall, const Material* mat, Info mode) :
	BoardObject(pos, size, hgcall, ulcall, urcall, mat, World::winSys()->texture(Com::pieceNames[uint8(type)]), mode),
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
	pc(new Piece[conf.piecesSize]),
	num(conf.numPieces),
	size(conf.piecesSize)
{}

void PieceCol::update(const Com::Config& conf) {
	if (conf.piecesSize != size) {
		num = conf.numPieces;
		size = conf.piecesSize;

		delete[] pc;
		pc = new Piece[size];
	}
}
