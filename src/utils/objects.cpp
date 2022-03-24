#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/shaders.h"
#include "engine/world.h"
#include "prog/board.h"
#include "prog/progs.h"
#include <glm/gtc/matrix_inverse.hpp>

// MESH

void Mesh::init(const vector<Vertex>& vertices, const vector<GLushort>& elements) {
	ecnt = elements.size();
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(*vertices.data())), vertices.data(), GL_STATIC_DRAW);
	setVertexAttrib();

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	setInstanceAttrib();

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(elements.size() * sizeof(*elements.data())), elements.data(), GL_STATIC_DRAW);
}

void Mesh::initTop() {
	glGenVertexArrays(1, &top->vao);
	glBindVertexArray(top->vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	setVertexAttrib();

	glGenBuffers(1, &top->ibo);
	glBindBuffer(GL_ARRAY_BUFFER, top->ibo);
	setInstanceAttrib();
}

void Mesh::free() {
	if (top) {
		glBindVertexArray(top->vao);
		disableAttrib();
		glDeleteBuffers(1, &top->ibo);
		glDeleteVertexArrays(1, &top->vao);
	}
	glBindVertexArray(vao);
	disableAttrib();
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void Mesh::setVertexAttrib() {
	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, decltype(Vertex::pos)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
	glEnableVertexAttribArray(Shader::normal);
	glVertexAttribPointer(Shader::normal, decltype(Vertex::nrm)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nrm)));
	glEnableVertexAttribArray(Shader::uvloc);
	glVertexAttribPointer(Shader::uvloc, decltype(Vertex::tuv)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tuv)));
	glEnableVertexAttribArray(Shader::tangent);
	glVertexAttribPointer(Shader::tangent, decltype(Vertex::tng)::length(), GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tng)));
}

void Mesh::setInstanceAttrib() {
	glEnableVertexAttribArray(Shader::model0);
	glVertexAttribPointer(Shader::model0, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, model)));
	glVertexAttribDivisor(Shader::model0, 1);
	glEnableVertexAttribArray(Shader::model1);
	glVertexAttribPointer(Shader::model1, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, model) + sizeof(vec4)));
	glVertexAttribDivisor(Shader::model1, 1);
	glEnableVertexAttribArray(Shader::model2);
	glVertexAttribPointer(Shader::model2, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, model) + sizeof(vec4) * 2));
	glVertexAttribDivisor(Shader::model2, 1);
	glEnableVertexAttribArray(Shader::model3);
	glVertexAttribPointer(Shader::model3, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, model) + sizeof(vec4) * 3));
	glVertexAttribDivisor(Shader::model3, 1);
	glEnableVertexAttribArray(Shader::normat0);
	glVertexAttribPointer(Shader::normat0, vec3::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, normat)));
	glVertexAttribDivisor(Shader::normat0, 1);
	glEnableVertexAttribArray(Shader::normat1);
	glVertexAttribPointer(Shader::normat1, vec3::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, normat) + sizeof(vec3)));
	glVertexAttribDivisor(Shader::normat1, 1);
	glEnableVertexAttribArray(Shader::normat2);
	glVertexAttribPointer(Shader::normat2, vec3::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, normat) + sizeof(vec3) * 2));
	glVertexAttribDivisor(Shader::normat2, 1);
	glEnableVertexAttribArray(Shader::diffuse);
	glVertexAttribPointer(Shader::diffuse, decltype(Instance::diffuse)::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, diffuse)));
	glVertexAttribDivisor(Shader::diffuse, 1);
	glEnableVertexAttribArray(Shader::specShine);
	glVertexAttribPointer(Shader::specShine, decltype(Instance::specShine)::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, specShine)));
	glVertexAttribDivisor(Shader::specShine, 1);
	glEnableVertexAttribArray(Shader::texid);
	glVertexAttribIPointer(Shader::texid, decltype(Instance::texid)::length(), GL_UNSIGNED_INT, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, texid)));
	glVertexAttribDivisor(Shader::texid, 1);
	glEnableVertexAttribArray(Shader::show);
	glVertexAttribIPointer(Shader::show, 1, GL_BYTE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, show)));
	glVertexAttribDivisor(Shader::show, 1);
}

void Mesh::disableAttrib() {
	glDisableVertexAttribArray(Shader::show);
	glDisableVertexAttribArray(Shader::texid);
	glDisableVertexAttribArray(Shader::specShine);
	glDisableVertexAttribArray(Shader::diffuse);
	glDisableVertexAttribArray(Shader::normat2);
	glDisableVertexAttribArray(Shader::normat1);
	glDisableVertexAttribArray(Shader::normat0);
	glDisableVertexAttribArray(Shader::model3);
	glDisableVertexAttribArray(Shader::model2);
	glDisableVertexAttribArray(Shader::model1);
	glDisableVertexAttribArray(Shader::model0);
	glDisableVertexAttribArray(Shader::tangent);
	glDisableVertexAttribArray(Shader::uvloc);
	glDisableVertexAttribArray(Shader::normal);
	glDisableVertexAttribArray(Shader::vpos);
}

void Mesh::draw() {
	glBindVertexArray(vao);
	glDrawElementsInstanced(GL_TRIANGLES, ecnt, GL_UNSIGNED_SHORT, nullptr, GLsizei(instanceData.size()));
}

void Mesh::drawTop() {
	glBindVertexArray(top->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glDrawElements(GL_TRIANGLES, ecnt, GL_UNSIGNED_SHORT, nullptr);
}

void Mesh::updateInstance(uint id, iptrt loffs, sizet size) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferSubData(GL_ARRAY_BUFFER, iptrt(id * sizeof(Instance)) + loffs, GLsizei(size), reinterpret_cast<uint8*>(&instanceData[id]) + loffs);
}

void Mesh::updateInstanceData() {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(instanceData.size() * sizeof(Instance)), instanceData.data(), GL_DYNAMIC_DRAW);
}

void Mesh::updateInstanceTop(iptrt loffs, sizet size) {
	glBindVertexArray(top->vao);
	glBindBuffer(GL_ARRAY_BUFFER, top->ibo);
	glBufferSubData(GL_ARRAY_BUFFER, loffs, GLsizei(size), reinterpret_cast<uint8*>(&top->data) + loffs);
}

void Mesh::updateInstanceDataTop() {
	glBindVertexArray(top->vao);
	glBindBuffer(GL_ARRAY_BUFFER, top->ibo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(top->data), &top->data, GL_DYNAMIC_DRAW);
}

void Mesh::allocate(uint size, bool setTop) {
	instanceData.resize(size);
	if (setTop && !top) {
		top = std::make_unique<Top>();
		initTop();
	}
}

uint Mesh::insert(const Instance& data) {
	uint id = instanceData.size();
	instanceData.push_back(data);
	return id;
}

Mesh::Instance Mesh::erase(uint id) {
	Instance data = instanceData[id];
	instanceData.erase(instanceData.begin() + id);
	return data;
}

// OBJECT

Object::Object(const vec3& position, const vec3& rotation, const vec3& scale, bool interactive) :
	pos(position),
	scl(scale),
	rot(rotation),
	rigid(interactive)
{}

void Object::init(Mesh* model, uint id, const Material* material, uvec2 texture, bool visible) {
	mesh = model;
	matl = material;
	meshIndex = id;
	init(model, id, pos, rot, scl, material, texture, visible);
}

void Object::init(Mesh* model, uint id, const vec3& pos, const quat& rot, const vec3& scl, const Material* material, uvec2 texture, bool visible) {
	Mesh::Instance& ins = model->getInstance(id);
	ins.model = getTransform(pos, rot, scl);
	ins.normat = glm::inverseTranspose(mat3(ins.model));
	ins.diffuse = material->color;
	ins.specShine = vec4(material->spec, material->shine);
	ins.texid = texture;
	ins.show = visible;
}

void Object::setPos(const vec3& vec) {
	pos = vec;
	mesh->getInstance(meshIndex).model = getTransform(pos, rot, scl);
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, model), sizeof(Mesh::Instance::model));
}

void Object::setRot(const quat& qut) {
	rot = qut;
	updateTransform();
}

void Object::setScl(const vec3& vec) {
	scl = vec;
	updateTransform();
}

void Object::setTrans(const vec3& position, const quat& rotation) {
	pos = position;
	rot = rotation;
	updateTransform();
}

void Object::setTrans(const vec3& position, const vec3& scale) {
	pos = position;
	scl = scale;
	updateTransform();
}

void Object::setTrans(const quat& rotation, const vec3& scale) {
	rot = rotation;
	scl = scale;
	updateTransform();
}

void Object::setTrans(const vec3& position, const quat& rotation, const vec3& scale) {
	pos = position;
	scl = scale;
	rot = rotation;
	updateTransform();
}

void Object::updateTransform() {
	Mesh::Instance& ins = mesh->getInstance(meshIndex);
	getTransform(ins.model, ins.normat);
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, model), sizeof(ins.model) + sizeof(ins.normat));
}

void Object::getTransform(mat4& model, mat3& normat) {
	model = getTransform(pos, rot, scl);
	normat = glm::inverseTranspose(mat3(model));
}

void Object::setMaterial(const Material* material) {
	matl = material;
	Mesh::Instance& ins = mesh->getInstance(meshIndex);
	ins.diffuse = matl->color;
	ins.specShine = vec4(matl->spec, matl->shine);
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, diffuse), sizeof(ins.diffuse) + sizeof(ins.specShine));
}

void Object::setTexture(uvec2 texture) {
	mesh->getInstance(meshIndex).texid = texture;
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, texid), sizeof(Mesh::Instance::texid));
}

void Object::setShow(bool visible) {
	mesh->getInstance(meshIndex).show = visible;
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, show), sizeof(Mesh::Instance::show));
}

// BOARD OBJECT

void BoardObject::init(Mesh* model, uint id, const Material* material, uvec2 texture, bool visible, float alpha) {
	mesh = model;
	matl = material;
	meshIndex = id;

	float dfac = emissionFactor();
	Mesh::Instance& ins = mesh->getInstance(meshIndex);
	getTransform(ins.model, ins.normat);
	ins.diffuse = matl->color * vec4(dfac, dfac, dfac, alpha);
	ins.specShine = vec4(matl->spec, matl->shine);
	ins.texid = texture;
	ins.show = visible;
}

void BoardObject::setTop(vec2 mPos, const mat3& normat, Mesh* tmesh, const Material* material, const vec4& colorFactor, uvec2 texture, bool depth) {
	topDepth = depth;

	vec3 isct = World::scene()->rayXZIsct(World::scene()->pickerRay(mPos));
	Mesh::Instance& top = tmesh->getInstanceTop();
	top.model = getTransform(vec3(isct.x, getTopY(), isct.z), getRot(), getScl());
	top.normat = normat;
	top.diffuse = material->color * colorFactor;
	top.specShine = vec4(material->spec, material->shine);
	top.texid = texture;
	top.show = true;
	tmesh->updateInstanceDataTop();
	World::state()->initObjectDrag(this, tmesh, vec2(isct.x, isct.z));
}

float BoardObject::getTopY() {
	return 0.1f;
}

void BoardObject::onDrag(ivec2 mPos, ivec2) {
	vec3 isct = World::scene()->rayXZIsct(World::scene()->pickerRay(mPos));
	World::state()->setObjectDragPos(vec2(isct.x, isct.z));
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
	svec2 mov = swap(dir.positive() ? 1 : -1, 0, dir.vertical());
	for (svec2 gpos = World::game()->board->ptog(getPos()) + mov; inRange(gpos, svec2(0), World::game()->board->boardLimit()); gpos += mov) {
		if (Piece* pce = World::game()->board->findOccupant(gpos); pce && pce->rigid) {
			World::state()->setObjectDragPos(vec2(pce->getPos().x, pce->getPos().z));
			World::scene()->updateSelect(pce);
			return;
		}
		if (Tile* til = World::game()->board->getTile(gpos); til->rigid) {
			World::state()->setObjectDragPos(vec2(til->getPos().x, til->getPos().z));
			World::scene()->updateSelect(til);
			return;
		}
	}
	if (ProgGame* pg = dynamic_cast<ProgGame*>(World::state()); pg && !World::scene()->getCapture())
		pg->planeSwitch->navSelectOut(getPos(), dir);
}

void BoardObject::startKeyDrag(uint8 mBut) {
	onHold(ivec2(0), mBut);
	World::state()->setObjectDragPos(vec2(getPos().x, getPos().z));
}

void BoardObject::setEmission(Emission emi) {
	emission = emi;

	float dfac = emissionFactor();
	vec4& diffuse = mesh->getInstance(meshIndex).diffuse;
	diffuse.r = matl->color.r * dfac;
	diffuse.g = matl->color.g * dfac;
	diffuse.b = matl->color.b * dfac;
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, diffuse), sizeof(diffuse.r) + sizeof(diffuse.g) + sizeof(diffuse.b));
}

void BoardObject::setAlphaFactor(float alpha) {
	mesh->getInstance(meshIndex).diffuse.a = matl->color.a * alpha;
	mesh->updateInstance(meshIndex, offsetof(Mesh::Instance, diffuse.a), sizeof(Mesh::Instance::diffuse.a));
}

// TILE

void Tile::onHold(ivec2 mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (leftDrag = mBut == SDL_BUTTON_LEFT; leftDrag ? ulcall : urcall) {
			const Mesh::Instance& ins = mesh->getInstance(meshIndex);
			setEmission(getEmission() & ~EMI_SEL);
			setShow(false);
			setTop(mPos, ins.normat, mesh, matl, moveColorFactor, ins.texid);
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
	if (World::scene()->getCapture() != this) {
		setEmission(getEmission() | EMI_SEL);
		setAlphaFactor(1.f);
		setShow(true);
	}
}

void Tile::onUnhover() {
	if (World::scene()->getCapture() != this) {
		setEmission(getEmission() & ~EMI_SEL);
		setAlphaFactor(type != TileType::empty ? 1.f : 0.f);
		setShow(type != TileType::empty);
	}
}

void Tile::onCancelCapture() {
	setShow(type != TileType::empty || (getEmission() & EMI_SEL));
}

void Tile::setType(TileType newType) {
	type = newType;
	bool amVisible = type != TileType::empty || (getEmission() & EMI_SEL);

	swapMesh();
	setMaterial(World::scene()->material(type != TileType::empty ? "tile" : "empty"));
	setAlphaFactor(amVisible ? 1.f : 0.f);
	setTexture(World::scene()->objTex(tileNames[uint8(type)]));
	setShow(amVisible);
}

void Tile::setBreached(bool yes) {
	breached = yes;
	swapMesh();
	setEmission(breached ? getEmission() | EMI_DIM : getEmission() & ~EMI_DIM);
}

void Tile::swapMesh() {
	if (Mesh* next = World::scene()->mesh(type != TileType::fortress ? "tile" : breached ? "breached" : "fortress"); next != mesh) {
		Mesh* prev = mesh;
		mesh = next;
		World::game()->board->updateTileInstances(this, prev);
	}
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

void Piece::init(Mesh* model, uint id, const Material* material, uvec2 texture, bool visible, PieceType iniType) {
	type = iniType;
	BoardObject::init(model, id, material, texture, visible);
}

float Piece::getTopY() {
	return topDepth && dynamic_cast<const Piece*>(World::scene()->getSelect()) && World::scene()->getSelect() != this ? 1.1f * getScl().y : 0.01f;
}

void Piece::onHold(ivec2 mPos, uint8 mBut) {
	if (mBut = World::state()->switchButtons(mBut); mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) {
		if (mBut == SDL_BUTTON_LEFT ? ulcall : urcall) {
			if (leftDrag = mBut == SDL_BUTTON_LEFT || !firingArea().first; leftDrag) {
				const Mesh::Instance& ins = mesh->getInstance(meshIndex);
				setShow(false);
				setTop(mPos, ins.normat, mesh, matl, moveColorFactor, ins.texid);
			} else {
				SDL_ShowCursor(SDL_DISABLE);
				setTop(mPos, mat3(1.f), World::scene()->mesh("plane"), World::scene()->material("reticle"), vec4(1.f), World::scene()->objTex("reticle"), false);
			}
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
	setShow(true);
	SDL_ShowCursor(SDL_ENABLE);
}

void Piece::updatePos(svec2 bpos, bool forceRigid) {
	svec2 oldPos = World::game()->board->ptog(getPos());
	if (setPos(World::game()->board->gtop(bpos)); !World::game()->board->pieceOnBoard(this))
		setActive(false);
	else if (setShow(true); forceRigid)
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
