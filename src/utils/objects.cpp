#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/world.h"
#include "prog/board.h"
#include "prog/progs.h"
#include <glm/gtc/matrix_inverse.hpp>

// MESH

Mesh::Mesh(const vector<Vertex>& vertices, const vector<GLushort>& elements, uint8 type) :
	ecnt(uint16(elements.size())),
	shape(type)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(*vertices.data())), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, decltype(Vertex::pos)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(Shader::normal);
	glVertexAttribPointer(Shader::normal, decltype(Vertex::nrm)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nrm)));
	glEnableVertexAttribArray(Shader::uvloc);
	glVertexAttribPointer(Shader::uvloc, decltype(Vertex::tuv)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tuv)));

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(elements.size() * sizeof(*elements.data())), elements.data(), GL_STATIC_DRAW);
}

void Mesh::free() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::vpos);
	glDisableVertexAttribArray(Shader::normal);
	glDisableVertexAttribArray(Shader::uvloc);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

// OBJECT

Object::Object(const vec3& position, const vec3& rotation, const vec3& scale, const Mesh* model, const Material* material, GLuint texture, bool visible, bool interactive) :
	mesh(model),
	matl(material),
	tex(texture),
	show(visible),
	rigid(interactive),
	pos(position),
	scl(scale),
	rot(rotation)
{
	setTransform(trans, normat, position, rot, scale);
}

#ifndef OPENGLES
void Object::drawDepth() const {
	glBindVertexArray(mesh->getVao());
	glUniformMatrix4fv(World::depth()->model, 1, GL_FALSE, glm::value_ptr(trans));
	glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
}
#endif

void Object::draw() const {
	glBindVertexArray(mesh->getVao());
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(trans));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(normat));
	glUniform4fv(World::geom()->materialDiffuse, 1, glm::value_ptr(matl->color));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(matl->spec));
	glUniform1f(World::geom()->materialShininess, matl->shine);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
}

void Object::setTransform(mat4& model, mat3& norm, const vec3& pos, const quat& rot, const vec3& scl) {
	setTransform(model, pos, rot, scl);
	norm = glm::inverseTranspose(mat3(model));
}

// BOARD OBJECT

BoardObject::BoardObject(const vec3& position, float rotation, float size, const Mesh* model, const Material* material, GLuint texture, bool visible, float alpha) :
	Object(position, vec3(0.f, rotation, 0.f), vec3(size), model, material, texture, visible),
	alphaFactor(alpha)
{}

void BoardObject::draw() const {
	vec4 clr = matl->color * vec4(diffuseFactor, diffuseFactor, diffuseFactor, alphaFactor);
	glBindVertexArray(mesh->getVao());
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(getTrans()));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(getNormat()));
	glUniform4fv(World::geom()->materialDiffuse, 1, glm::value_ptr(clr));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(matl->spec));
	glUniform1f(World::geom()->materialShininess, matl->shine);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
}

#ifndef OPENGLES
void BoardObject::drawTopMeshDepth(float ypos) const {
	mat4 model;
	setTransform(model, vec3(World::state()->objectDragPos.x, ypos, World::state()->objectDragPos.y), getRot(), getScl());
	glBindVertexArray(mesh->getVao());
	glUniformMatrix4fv(World::depth()->model, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(mesh->getShape(), mesh->getEcnt(), Mesh::elemType, nullptr);
}
#endif

void BoardObject::drawTopMesh(float ypos, const Mesh* tmesh, const Material& tmatl, GLuint ttexture) const {
	glBindVertexArray(tmesh->getVao());
	mat4 model;
	setTransform(model, vec3(World::state()->objectDragPos.x, ypos, World::state()->objectDragPos.y), getRot(), getScl());
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(getNormat()));
	glUniform4fv(World::geom()->materialDiffuse, 1, glm::value_ptr(tmatl.color));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(tmatl.spec));
	glUniform1f(World::geom()->materialShininess, tmatl.shine);
	glBindTexture(GL_TEXTURE_2D, ttexture);
	glDrawElements(tmesh->getShape(), tmesh->getEcnt(), Mesh::elemType, nullptr);
}

void BoardObject::onDrag(const ivec2& mPos, const ivec2&) {
	vec3 isct = World::scene()->rayXZIsct(World::scene()->pickerRay(mPos));
	World::state()->objectDragPos = vec2(isct.x, isct.z);
}

void BoardObject::onKeyDown(const SDL_KeyboardEvent& key) {
	World::input()->callBindings(key, [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onKeyUp(const SDL_KeyboardEvent& key) {
	World::input()->callBindings(key, [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onJButtonDown(uint8 but) {
	World::input()->callBindings(JoystickButton(but), [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onJButtonUp(uint8 but) {
	World::input()->callBindings(JoystickButton(but), [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onJHatDown(uint8 hat, uint8 val) {
	World::input()->callBindings(AsgJoystick(hat, val), [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onJHatUp(uint8 hat, uint8 val) {
	World::input()->callBindings(AsgJoystick(hat, val), [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onJAxisDown(uint8 axis, bool positive) {
	World::input()->callBindings(AsgJoystick(JoystickAxis(axis), positive), [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onJAxisUp(uint8 axis, bool positive) {
	World::input()->callBindings(AsgJoystick(JoystickAxis(axis), positive), [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onGButtonDown(SDL_GameControllerButton but) {
	World::input()->callBindings(but, [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onGButtonUp(SDL_GameControllerButton but) {
	World::input()->callBindings(but, [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onGAxisDown(SDL_GameControllerAxis axis, bool positive) {
	World::input()->callBindings(AsgGamepad(axis, positive), [this](Binding::Type id) { return onInputDown(id); });
}

void BoardObject::onGAxisUp(SDL_GameControllerAxis axis, bool positive) {
	World::input()->callBindings(AsgGamepad(axis, positive), [this](Binding::Type id) { return onInputUp(id); });
}

void BoardObject::onInputDown(Binding::Type bind) {
	switch (bind) {
	case Binding::Type::up:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::up);
		break;
	case Binding::Type::down:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::down);
		break;
	case Binding::Type::left:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::left);
		break;
	case Binding::Type::right:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::right);
		break;
	case Binding::Type::destroyHold: case Binding::Type::destroyToggle:
		World::srun(World::input()->getBinding(bind).bcall);
		break;
	case Binding::Type::cancel:
		World::scene()->updateSelect(this);
	case Binding::Type::confirm: case Binding::Type::engage:
		onUndrag(leftDrag ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
	}
}

void BoardObject::onInputUp(Binding::Type bind) {
	switch (bind) {
	case Binding::Type::destroyHold:
		World::srun(World::input()->getBinding(bind).ucall);
	}
}

void BoardObject::onNavSelect(Direction dir) {
	svec2 mov = swap(uint16(dir.positive() ? 1 : -1), uint16(0), dir.vertical());
	for (svec2 gpos = World::game()->board->ptog(getPos()) + mov; inRange(gpos, svec2(0), World::game()->board->boardLimit()); gpos += mov) {
		if (Piece* pce = World::game()->board->findOccupant(gpos); pce && pce->rigid) {
			World::state()->objectDragPos = vec2(pce->getPos().x, pce->getPos().z);
			World::scene()->updateSelect(pce);
			return;
		}
		if (Tile* til = World::game()->board->getTile(gpos); til->rigid) {
			World::state()->objectDragPos = vec2(til->getPos().x, til->getPos().z);
			World::scene()->updateSelect(til);
			return;
		}
	}
	if (ProgGame* pg = dynamic_cast<ProgGame*>(World::state()); pg && !World::scene()->getCapture())
		pg->planeSwitch->navSelectOut(getPos(), dir);
}

void BoardObject::startKeyDrag(uint8 mBut) {
	onHold(ivec2(0), mBut);
	World::state()->objectDragPos = vec2(getPos().x, getPos().z);
}

void BoardObject::setEmission(Emission emi) {
	emission = emi;
	diffuseFactor = (emission & EMI_DIM ? 0.6f : 1.f) + (emission & EMI_SEL ? 0.2f : 0.f) + (emission & EMI_HIGH ? 0.2f : 0.f);
}

// TILE

Tile::Tile(const vec3& position, float size, bool visible) :
	BoardObject(position, 0.f, size, World::scene()->mesh("tile"), World::scene()->material("empty"), World::scene()->texture(tileNames[uint8(TileType::empty)]), visible, 0.f)
{}

#ifndef OPENGLES
void Tile::drawDepth() const {
	if (type != TileType::empty)
		Object::drawDepth();
}

void Tile::drawTopDepth() const {
	drawTopMeshDepth(topYpos);
}
#endif

void Tile::drawTop() const {
	drawTopMesh(topYpos, mesh, Material(matl->color * moveColorFactor, matl->spec, matl->shine), tex);
}

void Tile::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (leftDrag = mBut == SDL_BUTTON_LEFT; leftDrag ? ulcall : urcall) {
			show = false;
			onDrag(mPos, ivec2());
			World::scene()->setCapture(this);
		}
		World::prun(hgcall, this, mBut);
	}
}

void Tile::onUndrag(uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		World::scene()->setCapture(nullptr);
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this, mBut);
	}
}

void Tile::onHover() {
	alphaFactor = 1.f;
	setEmission(getEmission() | EMI_SEL);
}

void Tile::onUnhover() {
	alphaFactor = type != TileType::empty ? 1.f : 0.f;
	setEmission(getEmission() & ~EMI_SEL);
}

void Tile::onCancelCapture() {
	show = true;
}

void Tile::setType(TileType newType) {
	type = newType;
	alphaFactor = type != TileType::empty || (getEmission() & EMI_SEL) ? 1.f : 0.f;
	mesh = World::scene()->mesh(pickMesh());
	matl = World::scene()->material(type != TileType::empty ? "tile" : "empty");
	tex = World::scene()->texture(tileNames[uint8(type)]);
}

void Tile::setBreached(bool yes) {
	breached = yes;
	mesh = World::scene()->mesh(pickMesh());
	setEmission(breached ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
}

void Tile::setInteractivity(Interact lvl, bool dim) {
	rigid = lvl != Interact::ignore;
	setEmission(dim || breached ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
	ulcall = lvl == Interact::interact && type != TileType::empty ? &Program::eventMoveTile : nullptr;
}

void Tile::setEmission(Emission emi) {
	BoardObject::setEmission(emi);
	if (TileTop top = World::game()->board->findTileTop(this); top != TileTop::none)
		World::game()->board->getTileTop(top)->setEmission(emi);
}

// TILE COL

void TileCol::update(const Config& conf) {
	if (uint16 cnt = conf.homeSize.x * conf.homeSize.y; cnt != home) {
		home = cnt;
		extra = home + conf.homeSize.x;
		size = extra + home;
		tl = std::make_unique<Tile[]>(size);
	}
}

// PIECE

Piece::Piece(const vec3& position, float rotation, float size, const Material* material) :
	BoardObject(position, rotation, size, nullptr, material, World::scene()->texture(), false, 1.f)
{}

#ifndef OPENGLES
void Piece::drawTopDepth() const {
	if (leftDrag)
		drawTopMeshDepth(selfTopYpos(World::scene()->getSelect()));
}
#endif

void Piece::drawTop() const {
	if (leftDrag)
		drawTopMesh(selfTopYpos(World::scene()->getSelect()), mesh, Material(matl->color * moveColorFactor, matl->spec, matl->shine), tex);
	else {
		glDisable(GL_DEPTH_TEST);
		drawTopMesh(topYpos, World::scene()->mesh("plane"), *World::scene()->material("reticle"), World::scene()->texture("reticle"));
		glEnable(GL_DEPTH_TEST);
	}
}

void Piece::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (mBut == SDL_BUTTON_LEFT ? ulcall : urcall) {
			if (leftDrag = mBut == SDL_BUTTON_LEFT || !firingArea().first; leftDrag)
				show = false;
			else
				SDL_ShowCursor(SDL_DISABLE);
			onDrag(mPos, ivec2());
			World::scene()->setCapture(this);
		}
		World::prun(hgcall, this, mBut);
	}
}

void Piece::onUndrag(uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		World::scene()->setCapture(nullptr);
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this, mBut);
	}
}

void Piece::onHover() {
	if (setEmission(getEmission() | EMI_SEL); World::game()->board->pieceOnBoard(this)) {
		Tile* til = World::game()->board->getTile(World::game()->board->ptog(getPos()));
		til->setEmission(til->getEmission() | EMI_SEL);
	}
}

void Piece::onUnhover() {
	if (setEmission(getEmission() & ~EMI_SEL); World::game()->board->pieceOnBoard(this)) {	// in case there is no tile (especially when disabling the piece)
		Tile* til = World::game()->board->getTile(World::game()->board->ptog(getPos()));
		til->setEmission(til->getEmission() & ~EMI_SEL);
	}
}

void Piece::onCancelCapture() {
	show = true;
	SDL_ShowCursor(SDL_ENABLE);
}

void Piece::setType(PieceType newType) {
	type = newType;
	mesh = World::scene()->mesh(pieceNames[uint8(type)]);
}

void Piece::updatePos(svec2 bpos, bool forceRigid) {
	svec2 oldPos = World::game()->board->ptog(getPos());
	if (setPos(World::game()->board->gtop(bpos)); !World::game()->board->pieceOnBoard(this))
		setActive(false);
	else if (show = true; forceRigid)
		rigid = true;

	// needlessly complicated selection update because of retarded controller support
	if (World::scene()->getSelect() == this) {
		if (Piece* pce = World::game()->board->findOccupant(oldPos))
			World::scene()->updateSelect(pce);
		else if (inRange(oldPos, svec2(0), World::game()->board->boardLimit()))
			World::scene()->updateSelect(World::game()->board->getTile(oldPos));
	} else if (BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getSelect()); bob && World::game()->board->ptog(getPos()) == World::game()->board->ptog(bob->getPos()))
		World::scene()->updateSelect(this);
}

void Piece::setInteractivity(bool on, bool dim, GCall holdCall, GCall leftCall, GCall rightCall) {
	rigid = on;
	hgcall = holdCall;
	ulcall = leftCall;
	urcall = rightCall;
	setEmission(dim ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
}

pair<uint8, uint8> Piece::firingArea() const {
	switch (type) {
	case PieceType::crossbowmen:
		return pair(1, 1);
	case PieceType::catapult:
		return pair(1, 2);
	case PieceType::trebuchet:
		return pair(3, 3);
	}
	return pair(0, 0);
}

// PIECE COL

void PieceCol::update(const Config& conf, bool regular) {
	if (uint16 cnt = (conf.opts & Config::setPieceBattle) && regular ? std::min(conf.countPieces(), conf.setPieceBattleNum) : conf.countPieces(); cnt != num) {
		num = cnt;
		size = cnt * 2;
		pc = std::make_unique<Piece[]>(size);
	}
}
