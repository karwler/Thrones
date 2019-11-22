#include "engine/world.h"
#include <glm/gtc/matrix_inverse.hpp>

// GMESH

Mesh::Mesh() :
	vao(0),
	ecnt(0),
	shape(0)
{}

Mesh::Mesh(const vector<Vertex>& vertices, const vector<uint16>& elements, uint8 shape) :
	ecnt(uint16(elements.size())),
	shape(shape)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(*vertices.data())), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(World::geom()->vertex);
	glVertexAttribPointer(World::geom()->vertex, psiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(World::geom()->normal);
	glVertexAttribPointer(World::geom()->normal, nsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nrm)));
	glEnableVertexAttribArray(World::geom()->uvloc);
	glVertexAttribPointer(World::geom()->uvloc, tsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tuv)));

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(elements.size() * sizeof(*elements.data())), elements.data(), GL_STATIC_DRAW);
}

void Mesh::free() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(World::geom()->vertex);
	glDisableVertexAttribArray(World::geom()->normal);
	glDisableVertexAttribArray(World::geom()->uvloc);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

// OBJECT

Object::Object(const vec3& pos, const vec3& ert, const vec3& scl, const Mesh* mesh, const Material* matl, GLuint tex, bool rigid, bool show) :
	mesh(mesh),
	matl(matl),
	tex(tex),
	show(show),
	rigid(rigid),
	pos(pos),
	scl(scl),
	rot(ert)
{
	setTransform(trans, normat, pos, rot, scl);
}

void Object::draw() const {
	if (show) {
		glBindVertexArray(mesh->getVao());
		updateTransform(trans, normat);
		updateColor(matl->diffuse, matl->specular, matl->shininess, matl->alpha, tex);
		glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
	}
}

void Object::updateColor(const vec3& diffuse, const vec3& specular, float shininess, float alpha, GLuint texture) {
	glUniform3fv(World::geom()->materialDiffuse, 1, glm::value_ptr(diffuse));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(specular));
	glUniform1f(World::geom()->materialShininess, shininess);
	glUniform1f(World::geom()->materialAlpha, alpha);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void Object::updateTransform(const mat4& model, const mat3& norm) {
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(norm));
}

void Object::setTransform(mat4& model, mat3& norm, const vec3& pos, const quat& rot, const vec3& scl) {
	setTransform(model, pos, rot, scl);
	norm = glm::inverseTranspose(mat3(model));
}

// GAME OBJECT

const vec3 BoardObject::moveIconColor(0.9f, 0.9f, 0.9f);

BoardObject::BoardObject(const vec3& pos, float rot, const vec3& scl, GCall hgcall, GCall ulcall, GCall urcall, const Mesh* mesh, const Material* matl, GLuint tex, bool rigid, bool show) :
	Object(pos, vec3(0.f, rot, 0.f), scl, mesh, matl, tex, rigid, show),
	hgcall(hgcall),
	ulcall(ulcall),
	urcall(urcall),
	diffuseFactor(1.f),
	emission(EMI_NONE)
{}

void BoardObject::draw() const {
	if (show) {
		glBindVertexArray(mesh->getVao());
		updateTransform(getTrans(), getNormat());
		updateColor(matl->diffuse * diffuseFactor, matl->specular, matl->shininess, matl->alpha, tex);
		glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
	}
}

void BoardObject::drawTopMesh(float ypos, const Mesh* tmesh, const vec3& tdiffuse, GLuint ttexture) const {
	glBindVertexArray(tmesh->getVao());
	mat4 model;
	setTransform(model, World::scene()->rayXZIsct(World::scene()->pickerRay(mousePos())) + vec3(0.f, ypos, 0.f), getRot(), getScl());
	updateTransform(model, getNormat());
	updateColor(tdiffuse, matl->specular, matl->shininess, 0.9f, ttexture);
	glDrawElements(tmesh->getShape(), tmesh->getEcnt(), Mesh::elemType, nullptr);
}

void BoardObject::setRaycast(bool on, bool dim) {
	rigid = on;
	setEmission(dim ? emission | EMI_DIM : emission & ~EMI_DIM);
}

void BoardObject::setEmission(Emission emi) {
	emission = emi;
	diffuseFactor = (emission & EMI_DIM ? 0.6f : 1.f) + (emission & EMI_SEL ? 0.2f : 0.f) + (emission & EMI_HIGH ? 0.2f : 0.f);
}

// TILE

Tile::Tile(const vec3& pos, float size, Com::Tile type, GCall hgcall, GCall ulcall, GCall urcall, bool rigid, bool show) :
	BoardObject(pos, 0.f, vec3(size, type != Com::Tile::fortress ? size : 1.f, size), hgcall, ulcall, urcall, nullptr, World::scene()->material("tile"), 0, rigid, getShow(show, type)),
	breached(false)
{
	setTypeSilent(type);
}

void Tile::drawTop() const {
	drawTopMesh(0.1f, mesh, matl->diffuse, tex);
}

void Tile::onHold(const ivec2&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && ulcall) {
		show = false;
		World::scene()->capture = this;
		World::prun(hgcall, this, mBut);
	}
}

void Tile::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		show = true;
		World::scene()->capture = nullptr;
		World::prun(ulcall, this, mBut);
	}
}

void Tile::onHover() {
	setEmission(getEmission() | EMI_SEL);
}

void Tile::onUnhover() {
	setEmission(getEmission() & ~EMI_SEL);
}

void Tile::setType(Com::Tile newType) {
	setTypeSilent(newType);
	show = type != Com::Tile::empty;
}

void Tile::setTypeSilent(Com::Tile newType) {
	type = newType;
	mesh = World::scene()->mesh(type != Com::Tile::fortress ? "tile" : breached ? "breached" : "fortress");
	tex = World::scene()->texture(type < Com::Tile::empty ? Com::tileNames[uint8(type)] : string());
}

void Tile::setBreached(bool yes) {
	breached = yes;
	setEmission(breached ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
	mesh = World::scene()->mesh(type != Com::Tile::fortress ? "tile" : breached ? "breached" : "fortress");
}

void Tile::setInteractivity(Interact lvl, bool dim) {
	setRaycast(lvl != Interact::ignore, dim);
	ulcall = lvl == Interact::interact && type != Com::Tile::empty ? &Program::eventMoveTile : nullptr;
}

// TILE COL

TileCol::TileCol() :
	tl(nullptr),
	home(0)
{}

void TileCol::update(const Com::Config& conf) {
	if (uint16 cnt = conf.homeSize.x * conf.homeSize.y; cnt != home) {
		home = cnt;
		extra = home + conf.homeSize.x;
		size = extra + home;

		delete[] tl;
		tl = new Tile[size];
	}
}

// PIECE

const vec3 Piece::fireIconColor(1.f, 0.1f, 0.1f);
const vec3 Piece::attackHorseColor(0.9f, 0.7f, 0.7f);

Piece::Piece(const vec3& pos, float rot, float size, Com::Piece type, GCall hgcall, GCall ulcall, GCall urcall, const Material* matl, bool rigid, bool show) :
	BoardObject(pos, rot, vec3(size), hgcall, ulcall, urcall, World::scene()->mesh(Com::pieceNames[uint8(type)]), matl, World::scene()->blank(), rigid, show),
	lastFortress(UINT16_MAX),
	type(type)
{}

void Piece::drawTop() const {
	if (drawTopSelf)
		drawTopMesh(dynamic_cast<Piece*>(World::scene()->select) && World::scene()->select != this ? 1.1f * getScl().y : 0.01f, mesh, matl->diffuse * (type != Com::Piece::warhorse ? moveIconColor : attackHorseColor), tex);
	else {
		glDisable(GL_DEPTH_TEST);
		drawTopMesh(0.1f, World::scene()->mesh("plane"), fireIconColor, World::scene()->texture("crosshair"));
		glEnable(GL_DEPTH_TEST);
	}
}

void Piece::onHold(const ivec2&, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT && ulcall) || (mBut == SDL_BUTTON_RIGHT && urcall)) {
		if (drawTopSelf = mBut == SDL_BUTTON_LEFT || !firingDistance())
			show = false;
		else
			SDL_ShowCursor(SDL_DISABLE);
		World::scene()->capture = this;
		World::prun(hgcall, this, mBut);
	}
}

void Piece::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		show = true;
		SDL_ShowCursor(SDL_ENABLE);
		World::scene()->capture = nullptr;
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this, mBut);
	}
}

void Piece::onHover() {
	if (setEmission(getEmission() | EMI_SEL); World::game()->pieceOnBoard(this)) {
		Tile* til = World::game()->getTile(World::game()->ptog(getPos()));
		til->setEmission(til->getEmission() | EMI_SEL);
	}
}

void Piece::onUnhover() {
	if (setEmission(getEmission() & ~EMI_SEL); World::game()->pieceOnBoard(this)) {	// in case there is no tile (especially when disabling the piece)
		Tile* til = World::game()->getTile(World::game()->ptog(getPos()));
		til->setEmission(til->getEmission() & ~EMI_SEL);
	}
}

void Piece::updatePos(svec2 bpos, bool active) {
	setPos(World::game()->gtop(bpos));
	setActive(active);
	World::scene()->updateSelect(mousePos());
}

// PIECE COL

PieceCol::PieceCol() :
	pc(nullptr),
	num(0)
{}

void PieceCol::update(const Com::Config& conf) {
	if (uint16 cnt = conf.countPieces(); cnt != num) {
		num = cnt;
		size = cnt * 2;

		delete[] pc;
		pc = new Piece[size];
	}
}
