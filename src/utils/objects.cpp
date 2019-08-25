#include "engine/world.h"

// CMESH

const vec3 CMesh::defaultVertices[defaultVertexSize] = {
	vec3(-0.5f, 0.f, -0.5f),
	vec3(0.5f, 0.f, -0.5f),
	vec3(0.5f, 0.f, 0.5f),
	vec3(-0.5f, 0.f, 0.5f)
};

CMesh::CMesh() :
	elems(nullptr),
	verts(nullptr),
	esiz(0)
{}

CMesh::CMesh(uint16 ec, uint16 vc) :
	elems(new uint16[ec]),
	verts(new vec3[vc]),
	esiz(ec)
{}

void CMesh::free() {
	delete[] elems;
	delete[] verts;
}

CMesh CMesh::makeDefault(const vec3& ofs) {
	CMesh mesh(defaultElementSize, defaultVertexSize);
	std::copy_n(defaultElements, defaultElementSize, mesh.elems);
	std::copy_n(defaultVertices, defaultVertexSize, mesh.verts);
	for (uint16 i = 0; i < defaultVertexSize; i++)
		mesh.verts[i] += ofs;
	return mesh;
}

// GMESH

GMesh::GMesh() :
	vao(0),
	ecnt(0),
	shape(0)
{}

GMesh::GMesh(const vector<Vertex>& vertices, const vector<uint16>& elements, uint8 shape) :
	ecnt(uint16(elements.size())),
	shape(shape)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(*vertices.data()), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(World::geom()->vertex);
	glVertexAttribPointer(World::geom()->vertex, psiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(World::geom()->normal);
	glVertexAttribPointer(World::geom()->normal, nsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nrm)));
	glEnableVertexAttribArray(World::geom()->uvloc);
	glVertexAttribPointer(World::geom()->uvloc, tsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tuv)));

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(*elements.data()), elements.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
}

void GMesh::free() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(World::geom()->vertex);
	glDisableVertexAttribArray(World::geom()->normal);
	glDisableVertexAttribArray(World::geom()->uvloc);
	glDeleteVertexArrays(1, &vao);
}

// OBJECT

Object::Object(const vec3& pos, const vec3& rot, const vec3& scl, const GMesh* mesh, const Material* matl, GLuint tex, const CMesh* coli, bool rigid, bool show) :
	mesh(mesh),
	matl(matl),
	coli(coli),
	tex(tex),
	show(show),
	rigid(rigid),
	pos(pos),
	rot(rot),
	scl(scl)
{
	setTransform(trans, normat, pos, rot, scl);
}

void Object::draw() const {
	if (show) {
		glBindVertexArray(mesh->vao);
		updateTransform();
		updateColor();
		glDrawElements(mesh->shape, mesh->ecnt, GMesh::elemType, nullptr);
	}
}

void Object::updateColor(const vec3& diffuse, const vec3& specular, const vec3& emission, float shininess, float alpha, GLuint texture) {
	glUniform3fv(World::geom()->materialDiffuse, 1, glm::value_ptr(diffuse));
	glUniform3fv(World::geom()->materialEmission, 1, glm::value_ptr(emission));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(specular));
	glUniform1f(World::geom()->materialShininess, shininess);
	glUniform1f(World::geom()->materialAlpha, alpha);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void Object::updateTransform(const mat4& model, const mat3& norm) {
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(norm));
}

void Object::setTransform(mat4& model, mat3& norm, const vec3& pos, const vec3& rot, const vec3& scl) {
	setTransform(model, pos, rot, scl);
	norm = glm::transpose(glm::inverse(mat3(model)));
}

// GAME OBJECT

const vec3 BoardObject::moveIconColor(0.9f, 0.9f, 0.9f);
const vec3 BoardObject::fireIconColor(1.f, 0.1f, 0.1f);

BoardObject::BoardObject(const vec3& pos, float rot, float size, GCall hgcall, GCall ulcall, GCall urcall, const GMesh* mesh, const Material* matl, GLuint tex, const CMesh* coli, bool rigid, bool show) :
	Object(pos, vec3(0.f, rot, 0.f), vec3(size, 1.f, size), mesh, matl, tex, coli, rigid, show),
	hgcall(hgcall),
	ulcall(ulcall),
	urcall(urcall),
	diffuseFactor(1.f),
	emissionFactor(0.f),
	dragState(DragState::none)
{}

void BoardObject::draw() const {
	if (dragState != DragState::move && show) {
		glBindVertexArray(mesh->vao);
		updateTransform();
		updateColor(matl->diffuse * diffuseFactor, matl->specular, matl->emission * emissionFactor, matl->shininess, matl->alpha, tex);
		glDrawElements(mesh->shape, mesh->ecnt, GMesh::elemType, nullptr);
	}
}

void BoardObject::drawTop() const {
	if (dragState != DragState::none && show) {
		vec3 ray = World::scene()->pickerRay(mousePos());
		const GMesh* tmesh = dragState == DragState::move ? mesh : World::scene()->mesh("plane");
		glBindVertexArray(tmesh->vao);

		mat4 model;
		setTransform(model, World::scene()->getCamera()->getPos() + ray * (getPos().y - upperPoz * 2.f - World::scene()->getCamera()->getPos().y / ray.y), getRot(), getScl());
		updateTransform(model, getNormat());
		updateColor(dragState == DragState::move ? matl->diffuse * moveIconColor : fireIconColor, matl->specular, vec3(0.f), matl->shininess, 0.9f, dragState == DragState::move ? tex : World::scene()->texture("crosshair"));
		glDrawElements(tmesh->shape, tmesh->ecnt, GMesh::elemType, nullptr);
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

void BoardObject::setRaycast(bool on, bool dim) {
	rigid = on;
	diffuseFactor = on || !dim ? 1.f : 0.6f;
}

// TILE

Tile::Tile(const vec3& pos, float size, Com::Tile type, GCall hgcall, GCall ulcall, GCall urcall, bool rigid, bool show) :
	BoardObject(pos, 0.f, size, hgcall, ulcall, urcall, World::scene()->mesh("tile"), World::scene()->material(type != Com::Tile::empty ? Com::tileNames[uint8(type)] : string()), World::scene()->blank(), World::scene()->collim("tile"), rigid, getShow(show, type)),
	type(type),
	breached(false)
{}

void Tile::onHover() {
	emissionFactor += emissionSelect;
	if (Piece* pce = World::game()->findPiece(World::game()->ptog(getPos())))
		pce->emissionFactor += emissionSelect;
}

void Tile::onUnhover() {
	emissionFactor -= emissionSelect;
	if (Piece* pce = World::game()->findPiece(World::game()->ptog(getPos())))
		pce->emissionFactor -= emissionSelect;
}

void Tile::setType(Com::Tile newType) {
	setTypeSilent(newType);
	show = matl;
}

void Tile::setTypeSilent(Com::Tile newType) {
	type = newType;
	matl = type != Com::Tile::empty ? World::scene()->material(Com::tileNames[uint8(type)]) : nullptr;
}

void Tile::setBreached(bool yes) {
	breached = yes;
	diffuseFactor = breached ? 0.5f : 1.f;
}

void Tile::setInteractivity(Interactivity lvl, bool dim) {
	setRaycast(lvl != Interactivity::ignore, dim);
	ulcall = lvl == Interactivity::interact && type != Com::Tile::empty ? &Program::eventMoveTile : nullptr;
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

Piece::Piece(const vec3& pos, float rot, float size, Com::Piece type, GCall hgcall, GCall ulcall, GCall urcall, const Material* matl, bool rigid, bool show) :
	BoardObject(pos, rot, size, hgcall, ulcall, urcall, World::scene()->mesh(Com::pieceNames[uint8(type)]), matl, World::scene()->blank(), World::scene()->collim("piece"), rigid, show),
	lastFortress(INT16_MAX),
	type(type)
{}

void Piece::onHover() {
	emissionFactor += emissionSelect;
	World::game()->getTile(World::game()->ptog(getPos()))->emissionFactor += emissionSelect;
}

void Piece::onUnhover() {
	emissionFactor -= emissionSelect;
	World::game()->getTile(World::game()->ptog(getPos()))->emissionFactor -= emissionSelect;
}

bool Piece::active() const {
	return World::game()->ptog(getPos()).hasNot(INT16_MIN) && show && rigid;
}

void Piece::enable(vec2s bpos) {
	setPos(World::game()->gtop(bpos));
	setActive(true);
	World::scene()->updateSelect(mousePos());
}

void Piece::disable() {
	setActive(false);
	World::scene()->updateSelect(mousePos());
	setPos(World::game()->gtop(INT16_MIN));
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
