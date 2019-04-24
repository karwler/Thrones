#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Interactable* inter, ScrollArea* area, const vec2i& mPos) :
	inter(inter),
	area(area),
	mPos(mPos)
{}

// KEYFRAME

Keyframe::Keyframe(float time, Keyframe::Change change, const glm::vec3& pos, const glm::vec3& rot, const vec4 color) :
	pos(pos),
	rot(rot),
	color(color),
	time(time),
	change(change)
{}

// ANIMATION

Animation::Animation(Object* object, const std::initializer_list<Keyframe>& keyframes) :
	keyframes(keyframes),
	begin(0.f, Keyframe::CHG_NONE, object->pos, object->rot, object->color),
	object(object)
{}

bool Animation::tick(float dSec) {
	begin.time += dSec;

	float td = clampHigh(begin.time / keyframes.front().time, 1.f);
	if (keyframes.front().change & Keyframe::CHG_POS)
		object->pos = linearTransition(begin.pos, keyframes.front().pos, td);
	if (keyframes.front().change & Keyframe::CHG_ROT)
		object->rot = linearTransition(begin.rot, keyframes.front().rot, td);
	if (keyframes.front().change & Keyframe::CHG_CLR)
		object->color = linearTransition(begin.color, keyframes.front().color, td);

	if (float ovhead = begin.time - keyframes.front().time; ovhead >= 0.f) {
		begin = Keyframe(0.f, Keyframe::CHG_NONE, keyframes.front().pos, keyframes.front().rot, keyframes.front().color);
		keyframes.pop();
		return tick(ovhead);
	}
	return keyframes.empty();
}

// CAMERA

const vec3 Camera::up(0.f, 1.f, 0.f);

Camera::Camera(const vec3& pos, const vec3& lat, float fov, float znear, float zfar) :
	pos(pos),
	lat(lat),
	fov(fov),
	znear(znear),
	zfar(zfar)
{}

void Camera::update() const {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(double(fov), vec2d(World::winSys()->windowSize()).ratio(), double(znear), double(zfar));
	gluLookAt(double(pos.x), double(pos.y), double(pos.z), double(lat.x), double(lat.y), double(lat.z), double(up.x), double(up.y), double(up.z));
	glMatrixMode(GL_MODELVIEW);
}

void Camera::updateUI() {
	vec2d res = World::winSys()->windowSize();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, res.x, res.y, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

vec3 Camera::direction(const vec2i& mPos) const {
	vec2 res = World::winSys()->windowSize().glm();
	vec3 dir = glm::normalize(lat - pos);
	float vl = std::tan(glm::radians(fov / 2.f)) * znear;
	float hl = vl * (res.x / res.y);
	vec3 h = glm::normalize(glm::cross(dir, up)) * hl;
	vec3 v = glm::normalize(glm::cross(h, dir)) * vl;

	res /= 2.f;
	vec2 m((float(mPos.x) - res.x) / res.x, -(float(mPos.y) - res.y) / res.y);
	return glm::normalize(dir * znear + h * m.x + v * m.y);
}

// SCENE

Scene::Scene() :
	mouseMove(0),
	select(nullptr),
	capture(nullptr),
	layout(new Layout)	// dummy layout in case a function gets called preemptively
{}

void Scene::draw() {
	glClear(clearSet);

	// draw objects
	camera.update();
	for (Object* it : objects)
		it->draw();

	// draw UI
	Camera::updateUI();
	layout->draw();
	if (popup)
		popup->draw();
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);

	for (vector<Animation>::iterator it = animations.begin(); it != animations.end(); it++)
		if (it->tick(dSec))
			animations.erase(it);
}

void Scene::onResize() {
	layout->onResize();
	if (popup)
		popup->onResize();
	World::state()->eventResized();
}

void Scene::onKeyDown(const SDL_KeyboardEvent& key) {
	if (capture)
		capture->onKeypress(key.keysym);
	else switch (key.keysym.scancode) {
	case SDL_SCANCODE_ESCAPE:
		World::state()->eventEscape();
	}
}

void Scene::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	mouseMove = mMov;
	select = getSelected(mPos, topLayout());

	if (capture)
		capture->onDrag(mPos, mMov);
	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();

	select = getSelected(mPos, topLayout());
	if (mCnt == 1) {
		stamps[mBut] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
			stamps[mBut].area->onHold(mPos, mBut);
		if (stamps[mBut].inter != stamps[mBut].area)
			stamps[mBut].inter->onHold(mPos, mBut);
	} else if (mCnt == 2 && stamps[mBut].inter == select && cursorInClickRange(mPos, mBut))
		select->onDoubleClick(mPos, mBut);
}

void Scene::onMouseUp(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (mCnt != 1)
		return;

	if (capture)
		capture->onUndrag(mBut);
	if (select && stamps[mBut].inter == select && cursorInClickRange(mPos, mBut))
		stamps[mBut].inter->onClick(mPos, mBut);
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * scrollFactorWheel);
	else if (select)
		select->onScroll(wMov * scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps) {
		it.inter = nullptr;
		it.area = nullptr;
	}
}

void Scene::resetLayouts() {
	// clear scene
	World::winSys()->clearFonts();
	onMouseLeave();	// reset stamps
	select = nullptr;
	capture = nullptr;
	popup.reset();

	// set up new widgets
	layout.reset(World::state()->createLayout());
	layout->postInit();
	onMouseMove(mousePos(), 0);
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	onMouseMove(mousePos(), 0);
}

Interactable* Scene::getSelected(const vec2i& mPos, Layout* box) {
	for (;;) {
		Rect frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->selectable() ? *it : getScrollAreaOrObject(mPos, *it);
		} else
			return getScrollAreaOrObject(mPos, box);
	}
}

Interactable* Scene::getScrollAreaOrObject(const vec2i& mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return pickObject(mPos);
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
	for (Object* obj : objects) {
		if (!(obj->mode & Object::INFO_RAYCAST))
			continue;

		mat4 trans = obj->getTransform();
		for (sizet i = 0; i < obj->elems.size(); i += 3)
			if (float t; rayIntersectsTriangle(camera.pos, ray, trans * vec4(obj->verts[obj->elems[i]].pos, 1.f), trans * vec4(obj->verts[obj->elems[i+1]].pos, 1.f), trans * vec4(obj->verts[obj->elems[i+2]].pos, 1.f), t))
				return obj;
	}
	return nullptr;
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
