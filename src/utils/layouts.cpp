#include "engine/scene.h"
#include "engine/world.h"
#include "prog/board.h"

// NAVIGATOR

bool Navigator::selectable() const {
	return true;
}

void Navigator::onNavSelect(Direction) {
	World::scene()->updateSelect(findFirstSelectable());
}

void Navigator::navSelectFrom(int mid, Direction dir) {
	Interactable* tile = findSelectable(swap(position()[dir.vertical()] + (dir.positive() ? 0 : size()[dir.vertical()]), mid, dir.vertical()));
	tile ? World::scene()->updateSelect(tile) : parent->navSelectNext(index, mid, dir);
}

Interactable* Navigator::findSelectable(ivec2 entry) const {
	vec3 pos = World::scene()->rayXZIsct(World::scene()->pickerRay(entry));
	float dmin = FLT_MAX;
	Tile* tile = nullptr;
	for (Tile& it : World::game()->board->getTiles())
		if (float dist = glm::distance(pos, it.getPos()); it.rigid && dist < dmin) {
			dmin = dist;
			tile = &it;
		}
	return tile;
}

Interactable* Navigator::findFirstSelectable() const {
	return findSelectable(position());
}

void Navigator::navSelectOut(const vec3& pos, Direction dir) {
	parent->navSelectNext(index, World::scene()->camera.screenPos(pos)[dir.horizontal()], dir);
}

// LAYOUT

Layout::Layout(const Size& size, vector<Widget*>&& children, bool vert, float space) :
	Navigator(size),
	widgets(std::move(children)),
	spaceFactor(space),
	vertical(vert)
{
	initWidgets();
}

void Layout::draw() const {
	for (Widget* it : widgets)
		it->draw();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::onResize() {
	calculateWidgetPositions();
	for (Widget* it : widgets)
		it->onResize();
}

void Layout::postInit() {
	calculateWidgetPositions();
	for (Widget* it : widgets)
		it->postInit();
}

void Layout::calculateWidgetPositions() {
	// get amount of space for widgets with percent size and get sum of sizes
	ivec2 wsiz = size();
	float scrh = float(World::window()->getScreenView().y);
	vector<int> pixSizes(widgets.size());
	int spacing = pixSpacing();
	int totalSpacing = (widgets.size() - 1) * spacing;
	int space = wsiz[vertical] > totalSpacing ? wsiz[vertical] - totalSpacing : 0;
	float total = 0;
	for (sizet i = 0; i < widgets.size(); ++i) {
		if (widgets[i]->getSize().mod != Size::rela) {
			pixSizes[i] = widgets[i]->getSize().mod == Size::abso ? int(widgets[i]->getSize() * scrh) : widgets[i]->getSize()(widgets[i]);
			space -= std::min(pixSizes[i], space);
		} else
			total += widgets[i]->getSize();
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(0);
	for (sizet i = 0; i < widgets.size(); ++i) {
		positions[i] = pos;
		if (widgets[i]->getSize().mod != Size::rela)
			pos[vertical] += pixSizes[i] + spacing;
		else if (float val = widgets[i]->getSize() * float(space); val != 0.f)
			pos[vertical] += int(val / total) + spacing;
	}
	positions.back() = swap(wsiz[!vertical], pos[vertical], !vertical);
}

bool Layout::selectable() const {
	return !widgets.empty();
}

void Layout::setWidgets(vector<Widget*>&& wgts) {
	deselectWidgets();
	setPtrVec(widgets, std::move(wgts));
	initWidgets();
	postInit();
	World::scene()->updateSelect();
}

void Layout::insertWidget(sizet id, Widget* wgt) {
	widgets.insert(widgets.begin() + pdift(id), wgt);
	positions.emplace_back();
	reinitWidgets(id);
	World::scene()->updateSelect();
}

void Layout::deleteWidget(sizet id) {
	bool updateSelect = deselectWidget(widgets[id]);
	delete widgets[id];
	widgets.erase(widgets.begin() + pdift(id));
	positions.pop_back();
	reinitWidgets(id);
	if (updateSelect)
		World::scene()->updateSelect();
}

void Layout::initWidgets() {
	positions.resize(widgets.size() + 1);
	for (sizet i = 0; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
}

void Layout::reinitWidgets(sizet id) {
	for (; id < widgets.size(); ++id)
		widgets[id]->setParent(this, id);
	postInit();
}

bool Layout::deselectWidget(Widget* wgt) {
	if (World::scene()->getCapture() == wgt)
		World::scene()->setCapture(nullptr);
	if (World::scene()->getSelect() == wgt) {
		World::scene()->deselect();
		return true;
	}
	Layout* box = dynamic_cast<Layout*>(wgt);
	return box && box->deselectWidgets();
}

bool Layout::deselectWidgets() const {
	for (Widget* it : widgets)
		if (deselectWidget(it))
			return true;
	return false;
}

void Layout::setSpacing(float space) {
	spaceFactor = space;
	onResize();
}

ivec2 Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(sizet id) const {
	return swap(size()[!vertical], positions[id+1][vertical] - positions[id][vertical] - pixSpacing(), !vertical);
}

Interactable* Layout::findFirstSelectable() const {
	for (Widget* it : widgets) {
		if (Navigator* box = dynamic_cast<Navigator*>(it)) {
			if (Interactable* wgt = box->findFirstSelectable())
				return wgt;
		} else if (it->selectable())
			return it;
	}
	return World::scene()->getFirstSelect();
}

void Layout::navSelectFrom(int mid, Direction dir) {
	if (dir.vertical() == vertical)
		scanSequential(dir.positive() ? SIZE_MAX : widgets.size(), mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::navSelectNext(sizet id, int mid, Direction dir) {
	if (dir.vertical() == vertical && (dir.positive() ? id < widgets.size() - 1 : id))
		scanSequential(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanSequential(sizet id, int mid, Direction dir) {
	for (sizet mov = btom<sizet>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->selectable(););
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	sizet id = 0;
	for (int hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->selectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); ++id);
	if (id == widgets.size())
		while (--id < widgets.size() && !widgets[id]->selectable());

	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

int Layout::pixSpacing() const {
	return int(spaceFactor * float(World::window()->getScreenView().y));
}

void Layout::navSelectWidget(sizet id, int mid, Direction dir) {
	if (Navigator* box = dynamic_cast<Navigator*>(widgets[id]))
		box->navSelectFrom(mid, dir);
	else if (widgets[id]->selectable())
		World::scene()->updateSelect(widgets[id]);
}

void Layout::updateTipTex() {
	for (Widget* it : widgets)
		it->updateTipTex();
}

// ROOT LAYOUT

RootLayout::RootLayout(const Size& size, vector<Widget*>&& children, bool vert, float space, float topSpace, const vec4& color) :
	Layout(size, std::move(children), vert, space),
	bgColor(color),
	topSpacingFac(topSpace)
{}

void RootLayout::draw() const {
	Quad::draw(World::sgui(), Rect(ivec2(0), World::window()->getScreenView()), bgColor, World::scene()->texture());	// dim other widgets
	Layout::draw();
}

ivec2 RootLayout::position() const {
	return ivec2(0, pixTopSpacing());
}

ivec2 RootLayout::size() const {
	ivec2 res = World::window()->getScreenView();
	return ivec2(res.x, res.y - pixTopSpacing());
}

void RootLayout::setSize(const Size& size) {
	relSize = size;
	onResize();
}

Rect RootLayout::frame() const {
	return rect();
}

int RootLayout::pixTopSpacing() const {
	return World::window()->getTitleBarHeight() ? int(topSpacingFac * float(World::window()->getScreenView().y)) : 0;
}

// TITLE BAR

ivec2 TitleBar::position() const {
	return ivec2(0);
}

ivec2 TitleBar::size() const {
	return ivec2(World::window()->getScreenView().x, World::window()->getTitleBarHeight());
}

// POPUP

Popup::Popup(const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, float space, float topSpace, Widget* firstSelect, Type ctxType, const vec4& color) :
	RootLayout(size.first, std::move(children), vert, space, topSpace, color),
	kcall(okCall),
	ccall(cancelCall),
	defaultSelect(firstSelect),
	sizeY(size.second),
	type(ctxType)
{}

void Popup::draw() const {
	Rect rct = rect();
	GLuint tex = World::scene()->texture();
	ivec2 res = World::window()->getScreenView();
	int margin = int(marginFactor * float(res.y));
	Quad::draw(World::sgui(), Rect(ivec2(0), res), bgColor, tex);				// dim other widgets
	Quad::draw(World::sgui(), Rect(rct.pos() - margin, rct.size() + margin * 2), colorBackground, tex);	// draw background
	Layout::draw();
}

ivec2 Popup::position() const {
	return (World::window()->getScreenView() - size()) / 2;
}

ivec2 Popup::size() const {
	vec2 res = World::window()->getScreenView();
	return ivec2(relSize.mod != Size::calc ? int(relSize * res.x) : relSize(this), sizeY.mod != Size::calc ? int(sizeY * res.y) : sizeY(this));
}

// OVERLAY

Overlay::Overlay(const pair<Size, Size>& pos, const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, bool visible, bool interactive, float space, float topSpace, const vec4& color) :
	Popup(size, std::move(children), okCall, cancelCall, vert, space, topSpace, nullptr, Type::overlay, vec4(0.f)),
	relPos(pos),
	boxColor(color),
	show(visible),
	interact(interactive)
{}

void Overlay::draw() const {
	GLuint tex = World::scene()->texture();
	Quad::draw(World::sgui(), Rect(ivec2(0), World::window()->getScreenView()), bgColor, tex);	// dim other widgets
	Quad::draw(World::sgui(), rect(), boxColor, tex);											// draw background
	Layout::draw();
}

ivec2 Overlay::position() const {
	vec2 res = World::window()->getScreenView();
	return ivec2(relPos.first.mod != Size::calc ? int(relPos.first * res.x) : relPos.first(this), std::max(relPos.second.mod != Size::calc ? int(relPos.second * res.y) : relPos.second(this), pixTopSpacing()));
}

ivec2 Overlay::size() const {
	ivec2 siz = Popup::size();
	int topSpacing = pixTopSpacing();
	int ypos = relPos.second.mod != Size::calc ? int(relPos.second * float(World::window()->getScreenView().y)) : relPos.second(this);
	return ivec2(siz.x, ypos >= topSpacing ? siz.y : siz.y - topSpacing + ypos);
}

void Overlay::setShow(bool yes) {
	show = yes;
	World::prun(show ? kcall : ccall, nullptr);
	World::scene()->updateSelect();
}

// SCROLL AREA

void ScrollArea::draw() const {
	mvec2 vis = visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; ++i)
		widgets[i]->draw();
	scroll.draw(frame(), listSize(), position(), size(), vertical);
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);
	scroll.tick(dSec, listSize(), size());
}

void ScrollArea::onResize() {
	Layout::onResize();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(listSize(), size()));
}

void ScrollArea::postInit() {
	Layout::postInit();
	scroll.listPos = ivec2(0);
}

void ScrollArea::onHold(ivec2 mPos, uint8 mBut) {
	scroll.hold(mPos, mBut, this, listSize(), position(), size(), vertical);
}

void ScrollArea::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, listSize(), position(), size(), vertical);
}

void ScrollArea::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, vertical);
}

void ScrollArea::onScroll(ivec2 wMov) {
	scroll.scroll(wMov, listSize(), size(), vertical);
	World::scene()->updateSelect();
}

Rect ScrollArea::frame() const {
	return rect().intersect(parent->frame());
}

ivec2 ScrollArea::wgtPosition(sizet id) const {
	return Layout::wgtPosition(id) - scroll.listPos;
}

ivec2 ScrollArea::wgtSize(sizet id) const {
	return Layout::wgtSize(id) - swap(ScrollBar::barSize(listSize(), size(), vertical), 0, !vertical);
}

void ScrollArea::onNavSelect(Direction dir) {
	Layout::onNavSelect(dir);
	scrollToSelected();
}

void ScrollArea::navSelectNext(sizet id, int mid, Direction dir) {
	Layout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, Direction dir) {
	Layout::navSelectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::onCancelCapture() {
	scroll.cancelDrag();
}

void ScrollArea::scrollToSelected() {
	for (Widget* child = dynamic_cast<Widget*>(World::scene()->getSelect()); child; child = child->getParent())
		if (child->getParent() == this) {
			if (int cpos = child->position()[vertical], fpos = position()[vertical]; cpos < fpos)
				scrollToWidgetPos(child->getIndex());
			else if (cpos + child->size()[vertical] > fpos + size()[vertical])
				scrollToWidgetEnd(child->getIndex());
			break;
		}
}

mvec2 ScrollArea::visibleWidgets() const {
	mvec2 ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	for (; ival.x < widgets.size() && wgtREnd(ival.x) < scroll.listPos[vertical]; ++ival.x);
	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = scroll.listPos[vertical] + size()[vertical]; ival.y < widgets.size() && wgtRPos(ival.y) <= end; ++ival.y);
	return ival;
}
