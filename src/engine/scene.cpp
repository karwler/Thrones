#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* widget, ScrollArea* area, const vec2i& mPos) :
	widget(widget),
	area(area),
	mPos(mPos)
{}

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
	if (LabelEdit* let = dynamic_cast<LabelEdit*>(capture))
		let->drawCaret();
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
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
	setSelected(mPos, topLayout());

	if (capture)
		capture->onDrag(mPos, mMov);
	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();

	setSelected(mPos, topLayout()) ? mouseDownWidget(mPos, mBut, mCnt) : mouseDownObject(mPos, mBut, mCnt);	// update in case selection has changed through keys while cursor remained at the old position
}

void Scene::onMouseUp(const vec2i& mPos, uint8 mBut) {
	if (capture)
		capture->onUndrag(mBut);

	if (select && stamps[mBut].widget == select && cursorInClickRange(mPos, mBut))
		stamps[mBut].widget->onClick(mPos, mBut);
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * scrollFactorWheel);
	else if (select)
		select->onScroll(wMov * scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps) {
		it.widget = nullptr;
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

Widget* Scene::setSelected(const vec2i& mPos, Layout* box) {
	Rect frame = box->frame();
	if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().getOverlap(frame).overlap(mPos); }); it != box->getWidgets().end()) {
		if (Layout* lay = dynamic_cast<Layout*>(*it))
			return setSelected(mPos, lay);
		return select = (*it)->selectable() ? *it : nullptr;
	}
	return select = dynamic_cast<ScrollArea*>(box);
}

ScrollArea* Scene::getSelectedScrollArea() const {
	Layout* parent = dynamic_cast<Layout*>(select);
	if (select && !parent)
		parent = select->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

sizet Scene::findSelectedID(Layout* box) {
	Widget* child = select;
	while (child->getParent() && child->getParent() != box)
		child = child->getParent();
	return child->getParent() ? child->getID() : SIZE_MAX;
}

void Scene::mouseDownWidget(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (mCnt == 1) {
		stamps[mBut] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
			stamps[mBut].area->onHold(mPos, mBut);
		if (stamps[mBut].widget != stamps[mBut].area)
			stamps[mBut].widget->onHold(mPos, mBut);
	} else if (mCnt == 2 && stamps[mBut].widget == select && cursorInClickRange(mPos, mBut))
		select->onDoubleClick(mPos, mBut);
}

void Scene::mouseDownObject(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (mCnt != 1 || mBut != SDL_BUTTON_LEFT)
		return;
	if (Object* obj = rayCast(camera.direction(mPos) * 10.f))
		std::cout << obj << std::endl;	// TODO: actual things
}

Object* Scene::rayCast(const vec3& ray) const {
	for (Object* obj : objects) {
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
