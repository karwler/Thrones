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

Interactable* Navigator::findSelectable(const ivec2& entry) const {
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

Layout::Layout(Size size, vector<Widget*>&& children, bool vert, int space) :
	Navigator(size),
	widgets(std::move(children)),
	spacing(space),
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
	// get amount of space for widgets with percent size and get sum of sizes
	ivec2 wsiz = size();
	int totalSpacing = int(widgets.size() - 1) * spacing;
	int space = wsiz[vertical] > totalSpacing ? wsiz[vertical] - totalSpacing : 0;
	float total = 0;
	for (Widget* it : widgets) {
		if (it->getSize().usePix)
			space -= it->getSize().pix <= space ? it->getSize().pix : space;
		else
			total += it->getSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(0);
	for (sizet i = 0; i < widgets.size(); ++i) {
		positions[i] = pos;
		pos[vertical] += (widgets[i]->getSize().usePix ? widgets[i]->getSize().pix : int(widgets[i]->getSize().prc * float(space) / total)) + spacing;
	}
	positions.back() = swap(wsiz[!vertical], pos[vertical], !vertical);

	// do the same for children
	for (Widget* it : widgets)
		it->onResize();
}

void Layout::postInit() {
	onResize();
	for (Widget* it : widgets)
		it->postInit();
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

void Layout::setSpacing(int space) {
	spacing = space;
	onResize();
}

ivec2 Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(sizet id) const {
	return swap(size()[!vertical], positions[id+1][vertical] - positions[id][vertical] - spacing, !vertical);
}

ivec2 Layout::listSize() const {
	return positions.back() - spacing;
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

void Layout::navSelectWidget(sizet id, int mid, Direction dir) {
	if (Navigator* box = dynamic_cast<Navigator*>(widgets[id]))
		box->navSelectFrom(mid, dir);
	else if (widgets[id]->selectable())
		World::scene()->updateSelect(widgets[id]);
}

// ROOT LAYOUT

RootLayout::RootLayout(Size size, vector<Widget*>&& children, bool vert, int space, const vec4& color) :
	Layout(size, std::move(children), vert, space),
	bgColor(color)
{}

void RootLayout::draw() const {
	Quad::draw(Rect(ivec2(0), World::window()->getScreenView()), bgColor, World::scene()->texture());	// dim other widgets
	Layout::draw();
}

ivec2 RootLayout::position() const {
	return ivec2(0);
}

ivec2 RootLayout::size() const {
	return World::window()->getGuiView();
}

void RootLayout::setSize(const Size& size) {
	relSize = size;
	onResize();
}

Rect RootLayout::frame() const {
	return Rect(ivec2(0), World::window()->getGuiView());
}

// POPUP

Popup::Popup(const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, int space, Widget* firstSelect, const vec4& color) :
	RootLayout(size.first, std::move(children), vert, space, color),
	kcall(okCall),
	ccall(cancelCall),
	defaultSelect(firstSelect),
	sizeY(size.second)
{}

void Popup::draw() const {
	Rect rct = rect();
	Quad::draw(Rect(ivec2(0), World::window()->getScreenView()), bgColor, World::scene()->texture());				// dim other widgets
	Quad::draw(Rect(rct.pos() - margin, rct.size() + margin * 2), colorBackground, World::scene()->texture());		// draw background
	Layout::draw();
}

ivec2 Popup::position() const {
	return (World::window()->getGuiView() - size()) / 2;
}

ivec2 Popup::size() const {
	vec2 res = World::window()->getGuiView();
	return ivec2(relSize.usePix ? relSize.pix : int(relSize.prc * res.x), sizeY.usePix ? sizeY.pix : int(sizeY.prc * res.y));
}

// OVERLAY

Overlay::Overlay(const pair<Size, Size>& pos, const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, bool visible, bool interactive, int space, const vec4& color) :
	Popup(size, std::move(children), okCall, cancelCall, vert, space, nullptr, vec4(0.f)),
	relPos(pos),
	boxColor(color),
	show(visible),
	interact(interactive)
{}

void Overlay::draw() const {
	Quad::draw(Rect(ivec2(0), World::window()->getScreenView()), bgColor, World::scene()->texture());	// dim other widgets
	Quad::draw(rect(), boxColor, World::scene()->texture());										// draw background
	Layout::draw();
}

ivec2 Overlay::position() const {
	vec2 res = World::window()->getGuiView();
	return ivec2(relPos.first.usePix ? relPos.first.pix : int(relPos.first.prc * res.x), relPos.second.usePix ? relPos.second.pix : int(relPos.second.prc * res.y));
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

void ScrollArea::postInit() {
	Layout::postInit();
	scroll.listPos = ivec2(0);
}

void ScrollArea::onHold(const ivec2& mPos, uint8 mBut) {
	scroll.hold(mPos, mBut, this, listSize(), position(), size(), vertical);
}

void ScrollArea::onDrag(const ivec2& mPos, const ivec2& mMov) {
	scroll.drag(mPos, mMov, listSize(), position(), size(), vertical);
}

void ScrollArea::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, vertical);
}

void ScrollArea::onScroll(const ivec2& wMov) {
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
	return Layout::wgtSize(id) - swap(scroll.barSize(listSize(), size(), vertical), 0, !vertical);
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
