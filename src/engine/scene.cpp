#include "world.h"
#include <GL/glew.h>

// CLICK STAMP

ClickStamp::ClickStamp(Widget* widget, ScrollArea* area, const vec2i& mPos) :
	widget(widget),
	area(area),
	mPos(mPos)
{}

// SCENE

Scene::Scene() :
	select(nullptr),
	capture(nullptr),
	layout(new Layout)	// dummy layout in case a function gets called preemptively
{}

void Scene::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw objects
	camera.update();
	glLoadIdentity();

	for (Object* it : objects)
		it->draw();

	// draw UI
	vec2i res = World::winSys()->windowSize();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, double(res.x), double(res.y), 0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	layout->draw();
	if (popup) {	// dim everything behind popup and display if present
		glDisable(GL_TEXTURE_2D);
		glColor4ubv(reinterpret_cast<const GLubyte*>(&colorPopupDim));

		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(res.x, 0);
		glVertex2i(res.x, res.y);
		glVertex2i(0, res.y);
		glEnd();

		popup->draw();
	}
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
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box && box->unfocusConfirm)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	
	setSelected(mPos, topLayout());	// update in case selection has changed through keys while cursor remained at the old position
	if (mCnt == 1) {
		stamps[mBut] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
			stamps[mBut].area->onHold(mPos, mBut);
		if (stamps[mBut].widget != stamps[mBut].area)
			stamps[mBut].widget->onHold(mPos, mBut);
	} else if (mCnt == 2 && stamps[mBut].widget == select && cursorInClickRange(mPos, mBut))
		select->onDoubleClick(mPos, mBut);
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

void Scene::setSelected(const vec2i& mPos, Layout* box) {
	Rect frame = box->frame();
	if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().getOverlap(frame).overlap(mPos); }); it != box->getWidgets().end()) {
		if (Layout* lay = dynamic_cast<Layout*>(*it))
			setSelected(mPos, lay);
		else
			select = (*it)->selectable() ? *it : box;
	} else
		select = box;
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
