#include "world.h"

// CAMERA

const vec3 Camera::posSetup(Com::Config::boardWidth / 2.f, 9.f, 8.f);
const vec3 Camera::posMatch(Com::Config::boardWidth / 2.f, 12.f, 5.f);
const vec3 Camera::latSetup(Com::Config::boardWidth / 2.f, 0.f, 2.f);
const vec3 Camera::latMatch(Com::Config::boardWidth / 2.f, 0.f, 1.f);
const vec3 Camera::up(0.f, 1.f, 0.f);

Camera::Camera(const vec3& pos, const vec3& lat) :
	pos(pos),
	lat(lat)
{
	glUseProgram(World::gui()->program);
	updateProjection();
}

void Camera::update() const {
	mat4 cviewMat = proj * glm::lookAt(pos, lat, up);
	glUniformMatrix4fv(World::space()->pview, 1, GL_FALSE, glm::value_ptr(cviewMat));
	glUniform3fv(World::space()->viewPos, 1, glm::value_ptr(pos));
}

void Camera::updateProjection() {
	vec2i res = World::window()->getView();
	proj = glm::perspective(glm::radians(fov), vec2f(res).ratio(), znear, zfar);

	vec2 hsiz(float(res.x / 2), float(res.y / 2));
	glUniform2fv(World::gui()->pview, 1, glm::value_ptr(hsiz));
}

vec3 Camera::direction(vec2i mPos) const {
	vec2 res = World::window()->getView().glm();
	vec3 dir = glm::normalize(lat - pos);

	float vl = std::tan(glm::radians(fov / 2.f)) * znear;
	float hl = vl * (res.x / res.y);
	vec3 h = glm::normalize(glm::cross(dir, up)) * hl;
	vec3 v = glm::normalize(glm::cross(h, dir)) * vl;

	res /= 2.f;
	vec2 m((float(mPos.x) - res.x) / res.x, -(float(mPos.y) - res.y) / res.y);
	return glm::normalize(dir * znear + h * m.x + v * m.y);
}

// LIGHT

Light::Light(const vec3& position, const vec3& ambient, const vec3& diffuse, const vec3& specular, Attenuation att) :
	position(position),
	ambient(ambient),
	diffuse(diffuse),
	specular(specular),
	att(att)
{
	glUseProgram(World::space()->program);
	update();
}

void Light::update() const {
	glUniform3fv(World::space()->lightPos, 1, glm::value_ptr(position));
	glUniform3fv(World::space()->lightAmbient, 1, glm::value_ptr(ambient));
	glUniform3fv(World::space()->lightDiffuse, 1, glm::value_ptr(diffuse));
	glUniform3fv(World::space()->lightSpecular, 1, glm::value_ptr(specular));
	glUniform1f(World::space()->lightLinear, att.linear);
	glUniform1f(World::space()->lightQuadratic, att.quadratic);
}

// CLICK STAMP

ClickStamp::ClickStamp(Interactable* inter, ScrollArea* area, vec2i mPos) :
	inter(inter),
	area(area),
	mPos(mPos)
{}

// KEYFRAME

Keyframe::Keyframe(float time, Change change, const vec3& pos, const vec3& rot) :
	pos(pos),
	rot(rot),
	time(time),
	change(change)
{}

// ANIMATION

Animation::Animation(Object* object, std::queue<Keyframe>&& keyframes) :
	keyframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, object->pos, object->rot),
	object(object),
	useObject(true)
{}

Animation::Animation(Camera* camera, std::queue<Keyframe>&& keyframes) :
	keyframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, camera->pos, camera->lat),
	camera(camera),
	useObject(false)
{}

bool Animation::tick(float dSec) {
	begin.time += dSec;

	float td = clampHigh(begin.time / keyframes.front().time, 1.f);
	if (keyframes.front().change & Keyframe::CHG_POS)
		(useObject ? object->pos : camera->pos) = linearTransition(begin.pos, keyframes.front().pos, td);
	if (keyframes.front().change & Keyframe::CHG_ROT)
		(useObject ? object->rot : camera->lat) = linearTransition(begin.rot, keyframes.front().rot, td);

	if (float ovhead = begin.time - keyframes.front().time; ovhead >= 0.f)
		if (keyframes.pop(); !keyframes.empty()) {
			begin = Keyframe(0.f, Keyframe::CHG_NONE, keyframes.front().pos, keyframes.front().rot);
			return tick(ovhead);
		}
	return !keyframes.empty();
}

// SCENE

Scene::Scene() :
	select(nullptr),
	capture(nullptr),
	mouseMove(0),
	camera(Camera::posSetup, Camera::latSetup),
	layout(new Layout),	// dummy layout in case a function gets called preemptively
	light(vec3(5.f, 4.f, 3.f), vec3(1.f, 0.98f, 0.92f), vec3(1.f, 0.99f, 0.92f), vec3(1.f), 100.f),
	meshes(FileSys::loadObjects()),
	materials(FileSys::loadMaterials()),
	texes(FileSys::loadTextures()),
	collims({
		pair("", CMesh()),
		pair("tile", CMesh::makeDefault()),
		pair("piece", CMesh::makeDefault(vec3(0.f, BoardObject::upperPoz, 0.f)))
	})
{}

Scene::~Scene() {
	for (auto& [name, tex] : texes)
		tex.free();
	for (auto& [name, mesh] : meshes)
		mesh.free();
	for (auto& [name, mesh] : collims)
		mesh.free();
}

void Scene::draw() {
	glUseProgram(World::space()->program);
	camera.update();
	for (Object* it : objects)
		it->draw();
	if (dynamic_cast<Object*>(capture))
		capture->drawTop();

	glUseProgram(World::gui()->program);
	World::gui()->bindRect();
	layout->draw();
	if (popup)
		popup->draw();
	if (dynamic_cast<Widget*>(capture))
		capture->drawTop();
	if (Button* but = dynamic_cast<Button*>(select))
		but->drawTooltip();
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);

	for (sizet i = 0; i < animations.size();) {
		if (animations[i].tick(dSec))
			i++;
		else
			animations.erase(animations.begin() + pdift(i));
	}
}

void Scene::onResize() {
	glUseProgram(World::gui()->program);
	camera.updateProjection();
	layout->onResize();
	if (popup)
		popup->onResize();
}

void Scene::onKeyDown(const SDL_KeyboardEvent& key) {
	if (dynamic_cast<LabelEdit*>(capture))
		capture->onKeypress(key.keysym);
	else if (!key.repeat)
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LALT:
			if (World::game()->favorState.use = true; Piece* pce = dynamic_cast<Piece*>(capture))
				World::program()->eventFavorStart(pce);
			break;
		case SDL_SCANCODE_RETURN:
			if (popup)		// right now this is only necessary for popups, so no fancy virtual functions
				World::prun(popup->kcall, nullptr);
			break;
		case SDL_SCANCODE_ESCAPE:
			World::state()->eventEscape();
		}
}

void Scene::onKeyUp(const SDL_KeyboardEvent& key) {
	if (!(dynamic_cast<LabelEdit*>(capture) || key.repeat))
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LALT:
			if (World::game()->favorState.use = false; Piece* pce = dynamic_cast<Piece*>(capture))
				World::program()->eventFavorStart(pce);
		}
}

void Scene::onMouseMove(vec2i mPos, vec2i mMov, uint32 mStat, uint32 time) {
	mouseMove = mMov;
	moveTime = time;

	updateSelect(mPos);
	if (capture)
		capture->onDrag(mPos, mMov);
	else if (mStat)
		World::state()->eventDrag(mStat);
}

void Scene::onMouseDown(vec2i mPos, uint8 mBut) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();

	select = getSelected(mPos);
	stamps[mBut] = ClickStamp(select, getSelectedScrollArea(), mPos);
	if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
		stamps[mBut].area->onHold(mPos, mBut);
	if (stamps[mBut].inter != stamps[mBut].area)
		stamps[mBut].inter->onHold(mPos, mBut);
	if (!capture)	// can be set by previous onHold calls
		World::state()->eventDrag(SDL_BUTTON(mBut));
}

void Scene::onMouseUp(vec2i mPos, uint8 mBut) {
	if (capture)
		capture->onUndrag(mBut);
	if (select && stamps[mBut].inter == select && cursorInClickRange(mPos, mBut))
		stamps[mBut].inter->onClick(mPos, mBut);
	if (!capture)
		World::state()->eventUndrag();
}

void Scene::onMouseWheel(vec2i wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * scrollFactorWheel);
	else if (wMov.y)
		World::state()->eventWheel(wMov.y);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps)
		it.inter = it.area = nullptr;
	unselect();
}

void Scene::resetLayouts() {
	// clear scene
	World::fonts()->clear();
	onMouseLeave();	// reset stamps and select
	capture = nullptr;
	popup.reset();

	// set up new widgets
	layout.reset(World::state()->createLayout());
	layout->postInit();
	onMouseMove(mousePos(), 0, 0, 0);
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	unselect();	// preemptive to avoid dangling pointer
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	onMouseMove(mousePos(), 0, 0, 0);
}

void Scene::unselect() {
	if (select) {
		select->onUnhover();
		select = nullptr;
	}
}

void Scene::updateSelect(vec2i mPos) {
	if (Interactable* cur = getSelected(mPos); cur != select) {
		if (select)
			select->onUnhover();
		if (select = cur)
			select->onHover();
	}
}

Interactable* Scene::getSelected(vec2i mPos) {
	for (Layout* box = !popup ? layout.get() : popup.get();;) {
		Rect frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->selectable() ? *it : getScrollOrObject(mPos, *it);
		} else
			return getScrollOrObject(mPos, box);
	}
}

Interactable* Scene::getScrollOrObject(vec2i mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return !popup ? static_cast<Interactable*>(rayCast(pickerRay(mPos))) : static_cast<Interactable*>(popup.get());
}

ScrollArea* Scene::findFirstScrollArea(Widget* wgt) {
	Layout* parent = dynamic_cast<Layout*>(wgt);
	if (wgt && !parent)
		parent = wgt->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

Object* Scene::rayCast(const vec3& ray) const {
	float min = FLT_MAX;
	Object* mob = nullptr;
	for (Object* obj : objects) {
		if (!obj->rigid)	// assuming that obj->coli is set
			continue;

		mat4 trans = glm::translate(mat4(1.f), obj->pos);
		trans = glm::rotate(trans, obj->rot.x, vec3(1.f, 0.f, 0.f));
		trans = glm::rotate(trans, obj->rot.y, vec3(0.f, 1.f, 0.f));
		trans = glm::rotate(trans, obj->rot.z, vec3(0.f, 0.f, 1.f));
		trans = glm::scale(trans, obj->scl);
		for (uint16 i = 0; i < obj->coli->esiz; i += 3)
			if (float t; rayIntersectsTriangle(camera.pos, ray, trans * vec4(obj->coli->verts[obj->coli->elems[i]], 1.f), trans * vec4(obj->coli->verts[obj->coli->elems[i+1]], 1.f), trans * vec4(obj->coli->verts[obj->coli->elems[i+2]], 1.f), t) && t <= min) {
				min = t;
				mob = obj;
			}
	}
	return mob;
}

bool Scene::rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t) {
	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 h = glm::cross(dir, e2);
	float a = glm::dot(e1, h);
	if (a > -FLT_EPSILON && a < FLT_EPSILON)
		return false;

	float f = 1.f / a;
	vec3 s = ori - v0;
	float u = f * (glm::dot(s, h));
	if (u < 0.f || u > 1.f)
		return false;

	vec3 q = glm::cross(s, e1);
	float v = f * glm::dot(dir, q);
	if (v < 0.f || u + v > 1.f)
		return false;

	t = f * glm::dot(e2, q);
	return t > FLT_EPSILON;
}
