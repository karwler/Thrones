#include "engine/world.h"
#include <glm/gtc/matrix_inverse.hpp>

// GMESH

Mesh::Mesh() :
	vao(0),
	ecnt(0),
	shape(0)
{}

Mesh::Mesh(const vector<Vertex>& vertices, const vector<uint16>& elements, uint8 type) :
	ecnt(uint16(elements.size())),
	shape(type)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(*vertices.data())), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, psiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(Shader::normal);
	glVertexAttribPointer(Shader::normal, nsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nrm)));
	glEnableVertexAttribArray(Shader::uvloc);
	glVertexAttribPointer(Shader::uvloc, tsiz, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tuv)));

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

Object::Object(const vec3& position, const vec3& rotation, const vec3& scale, const Mesh* model, const Material* material, GLuint texture, bool interactive, bool visible) :
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

// GAME OBJECT

BoardObject::BoardObject(const vec3& position, float rotation, float size, const Mesh* model, const Material* material, GLuint texture, bool visible, float alpha) :
	Object(position, vec3(0.f, rotation, 0.f), vec3(size), model, material, texture, false, visible),
	hgcall(nullptr),
	ulcall(nullptr),
	urcall(nullptr),
	alphaFactor(alpha),
	diffuseFactor(1.f),
	emission(EMI_NONE)
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
void BoardObject::drawTopMeshDepth(float ypos, const Mesh* tmesh) const {
	glBindVertexArray(tmesh->getVao());
	mat4 model;
	setTransform(model, vec3(World::state()->objectDragPos.x, ypos, World::state()->objectDragPos.y), getRot(), getScl());
	glUniformMatrix4fv(World::depth()->model, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(tmesh->getShape(), tmesh->getEcnt(), Mesh::elemType, nullptr);
}
#endif

void BoardObject::drawTopMesh(float ypos, const Mesh* tmesh, const vec4& tdiffuse, GLuint ttexture) const {
	glBindVertexArray(tmesh->getVao());
	mat4 model;
	setTransform(model, vec3(World::state()->objectDragPos.x, ypos, World::state()->objectDragPos.y), getRot(), getScl());
	glUniformMatrix4fv(World::geom()->model, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(World::geom()->normat, 1, GL_FALSE, glm::value_ptr(getNormat()));
	glUniform4fv(World::geom()->materialDiffuse, 1, glm::value_ptr(tdiffuse));
	glUniform3fv(World::geom()->materialSpecular, 1, glm::value_ptr(matl->spec));
	glUniform1f(World::geom()->materialShininess, matl->shine);
	glBindTexture(GL_TEXTURE_2D, ttexture);
	glDrawElements(tmesh->getShape(), tmesh->getEcnt(), Mesh::elemType, nullptr);
}

void BoardObject::onDrag(const ivec2& mPos, const ivec2&) {
	vec3 isct = World::scene()->rayXZIsct(World::scene()->pickerRay(mPos));
	World::state()->objectDragPos = vec2(isct.x, isct.z);
}

void BoardObject::onKeypress(const SDL_KeyboardEvent& key) {
	switch (key.keysym.scancode) {
	case SDL_SCANCODE_UP:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::up);
		break;
	case SDL_SCANCODE_DOWN:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::down);
		break;
	case SDL_SCANCODE_LEFT:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::left);
		break;
	case SDL_SCANCODE_RIGHT:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::right);
		break;
	case SDL_SCANCODE_D:
		if (!key.repeat)
			World::state()->eventDestroy(Switch::on);
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		if (!key.repeat)
			onUndrag(SDL_BUTTON_LEFT);
		break;
	case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_TAB: case SDL_SCANCODE_AC_BACK:
		if (!key.repeat)
			cancelDrag();
	}
}

void BoardObject::onKeyrelease(const SDL_KeyboardEvent& key) {
	switch (key.keysym.scancode) {
	case SDL_SCANCODE_D:
		if (!key.repeat)
			World::state()->eventDestroy(Switch::off);
	}
}

void BoardObject::onJButton(uint8 but) {
	switch (but) {
	case InputSys::jbutStart:
		World::state()->eventDestroy(Switch::toggle);
		break;
	case InputSys::jbutA:
		onUndrag(SDL_BUTTON_LEFT);
		break;
	case InputSys::jbutB:
		cancelDrag();
	}
}

void BoardObject::onJHat(uint8 hat) {
	if ((hat & SDL_HAT_UP) && World::scene()->getSelect())
		World::scene()->getSelect()->onNavSelect(Direction::up);
	if ((hat & SDL_HAT_RIGHT) && World::scene()->getSelect())
		World::scene()->getSelect()->onNavSelect(Direction::right);
	if ((hat & SDL_HAT_DOWN) && World::scene()->getSelect())
		World::scene()->getSelect()->onNavSelect(Direction::down);
	if ((hat & SDL_HAT_LEFT) && World::scene()->getSelect())
		World::scene()->getSelect()->onNavSelect(Direction::left);
}

void BoardObject::onGButton(SDL_GameControllerButton but) {
	switch (but) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::up);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::down);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::left);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		if (World::scene()->getSelect())
			World::scene()->getSelect()->onNavSelect(Direction::right);
		break;
	case SDL_CONTROLLER_BUTTON_START:
		World::state()->eventDestroy(Switch::toggle);
		break;
	case SDL_CONTROLLER_BUTTON_A:
		onUndrag(SDL_BUTTON_LEFT);
		break;
	case SDL_CONTROLLER_BUTTON_B:
		cancelDrag();
	}
}

void BoardObject::onNavSelect(Direction dir) {
	if (svec2 gpos = World::game()->board.ptog(getPos()) + swap(uint16(dir.positive() ? 1 : -1), uint16(0), dir.vertical()); inRange(gpos, svec2(0), World::game()->board.boardLimit())) {
		if (Piece* pce = World::game()->board.findOccupant(gpos); pce && pce->rigid) {
			World::state()->objectDragPos = vec2(pce->getPos().x, pce->getPos().z);
			World::scene()->updateSelect(pce);
		} else if (Tile* til = World::game()->board.getTile(gpos); til->rigid) {
			World::state()->objectDragPos = vec2(til->getPos().x, til->getPos().z);
			World::scene()->updateSelect(til);
		} else if (ProgGame* pg = dynamic_cast<ProgGame*>(World::state()); pg && !World::scene()->capture)
			pg->planeSwitch->navSelectOut(getPos(), dir);
	} else if (ProgGame* pg = dynamic_cast<ProgGame*>(World::state()); pg && !World::scene()->capture)
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
	BoardObject(position, 0.f, size, World::scene()->mesh("tile"), World::scene()->material("empty"), World::scene()->texture(Com::tileNames[uint8(Com::Tile::empty)]), visible, 0.f),
	type(Com::Tile::empty),
	breached(false)
{}

#ifndef OPENGLES
void Tile::drawTopDepth() const {
	drawTopMeshDepth(topYpos, mesh);
}
#endif

void Tile::drawTop() const {
	drawTopMesh(topYpos, mesh, matl->color * moveIconColor, tex);
}

void Tile::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (mBut == SDL_BUTTON_LEFT ? ulcall : urcall) {
			show = false;
			onDrag(mPos, ivec2());
			World::scene()->capture = this;
		}
		World::prun(hgcall, this, mBut);
	}
}

void Tile::onUndrag(uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		cancelDrag();
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this, mBut);
	}
}

void Tile::onHover() {
	alphaFactor = 1.f;
	setEmission(getEmission() | EMI_SEL);
}

void Tile::onUnhover() {
	alphaFactor = type != Com::Tile::empty ? 1.f : 0.f;
	setEmission(getEmission() & ~EMI_SEL);
}

void Tile::cancelDrag() {
	show = true;
	World::scene()->capture = nullptr;
}

void Tile::setType(Com::Tile newType, Com::Tile altType) {
	type = newType;
	alphaFactor = newType != Com::Tile::empty || (getEmission() & EMI_SEL) ? 1.f : 0.f;
	mesh = World::scene()->mesh(pickMesh());
	matl = World::scene()->material(altType == Com::Tile::empty ? type != Com::Tile::empty ? "tile" : "empty" : Com::tileNames[uint8(type)]);
	tex = World::scene()->texture(Com::tileNames[uint8(altType == Com::Tile::empty ? type : altType)]);
}

void Tile::setBreached(bool yes) {
	breached = yes;
	mesh = World::scene()->mesh(pickMesh());
	setEmission(breached ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
}

void Tile::setInteractivity(Interact lvl, bool dim) {
	rigid = lvl != Interact::ignore;
	setEmission(dim ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
	ulcall = lvl == Interact::interact && type != Com::Tile::empty ? &Program::eventMoveTile : nullptr;
}

// PIECE

Piece::Piece(const vec3& position, float rotation, float size, const Material* material) :
	BoardObject(position, rotation, size, nullptr, material, World::scene()->texture(), false, 1.f),
	lastFortress(UINT16_MAX)
{}

#ifndef OPENGLES
void Piece::drawTopDepth() const {
	if (drawTopSelf)
		drawTopMeshDepth(selfTopYpos(World::scene()->getSelect()), mesh);
}
#endif

void Piece::drawTop() const {
	if (drawTopSelf)
		drawTopMesh(selfTopYpos(World::scene()->getSelect()), mesh, matl->color * moveIconColor, tex);
	else {
		glDisable(GL_DEPTH_TEST);
		drawTopMesh(topYpos, World::scene()->mesh("plane"), fireIconColor, World::scene()->texture("crosshair"));
		glEnable(GL_DEPTH_TEST);
	}
}

void Piece::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (mBut == SDL_BUTTON_LEFT ? ulcall : urcall) {
			if (drawTopSelf = mBut == SDL_BUTTON_LEFT || !firingArea().first)
				show = false;
			else
				SDL_ShowCursor(SDL_DISABLE);
			onDrag(mPos, ivec2());
			World::scene()->capture = this;
		}
		World::prun(hgcall, this, mBut);
	}
}

void Piece::onUndrag(uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		cancelDrag();
		World::prun(mBut == SDL_BUTTON_LEFT ? ulcall : urcall, this, mBut);
	}
}

void Piece::onHover() {
	if (setEmission(getEmission() | EMI_SEL); World::game()->board.pieceOnBoard(this)) {
		Tile* til = World::game()->board.getTile(World::game()->board.ptog(getPos()));
		til->setEmission(til->getEmission() | EMI_SEL);
	}
}

void Piece::onUnhover() {
	if (setEmission(getEmission() & ~EMI_SEL); World::game()->board.pieceOnBoard(this)) {	// in case there is no tile (especially when disabling the piece)
		Tile* til = World::game()->board.getTile(World::game()->board.ptog(getPos()));
		til->setEmission(til->getEmission() & ~EMI_SEL);
	}
}

void Piece::cancelDrag() {
	show = true;
	SDL_ShowCursor(SDL_ENABLE);
	World::scene()->capture = nullptr;
}

void Piece::setType(Com::Piece newType) {
	type = newType;
	mesh = World::scene()->mesh(Com::pieceNames[uint8(type)]);
}

void Piece::updatePos(svec2 bpos, bool forceRigid) {
	svec2 oldPos = World::game()->board.ptog(getPos());
	if (setPos(World::game()->board.gtop(bpos)); !World::game()->board.pieceOnBoard(this))
		setActive(false);
	else if (show = true; forceRigid)
		rigid = true;

	// needlessly complicated selection update because of retarded controller support
	if (World::scene()->getSelect() == this) {
		if (Piece* pce = World::game()->board.findOccupant(oldPos))
			World::scene()->updateSelect(pce);
		else if (inRange(oldPos, svec2(0), World::game()->board.boardLimit()))
			World::scene()->updateSelect(World::game()->board.getTile(oldPos));
	} else if (BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getSelect()); bob && World::game()->board.ptog(getPos()) == World::game()->board.ptog(bob->getPos()))
		World::scene()->updateSelect(this);
}

void Piece::setInteractivity(bool on, bool dim, GCall holdCall, GCall leftCall, GCall rightCall) {
	rigid = on;
	hgcall = holdCall;
	ulcall = leftCall;
	urcall = rightCall;
	setEmission(dim ? getEmission() | BoardObject::EMI_DIM : getEmission() & ~BoardObject::EMI_DIM);
}

pair<uint8, uint8> Piece::firingArea() const {
	switch (type) {
	case Com::Piece::crossbowmen:
		return pair(1, 1);
	case Com::Piece::catapult:
		return pair(1, 2);
	case Com::Piece::trebuchet:
		return pair(3, 3);
	}
	return pair(0, 0);
}
