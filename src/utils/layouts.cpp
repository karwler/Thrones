#include "engine/world.h"

// LAYOUT

Layout::Layout(Size relSize, vector<Widget*>&& children, bool vertical, int spacing, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	spacing(spacing),
	vertical(vertical)
{
	initWidgets(std::move(children));
}

Layout::~Layout() {
	clear(widgets);
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
	// get amount of space for widgets with prc and get sum of widget's prc
	ivec2 wsiz = size();
	float space = float(wsiz[vertical] - int(widgets.size()-1) * spacing);
	float total = 0;
	for (Widget* it : widgets) {
		if (it->relSize.usePix)
			space -= it->relSize.pix;
		else
			total += it->relSize.prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(0);
	for (sizet i = 0; i < widgets.size(); i++) {
		positions[i] = pos;
		pos[vertical] += (widgets[i]->relSize.usePix ? widgets[i]->relSize.pix : int(widgets[i]->relSize.prc * space / total)) + spacing;
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

void Layout::initWidgets(vector<Widget*>&& wgts) {
	clear(widgets);
	widgets = std::move(wgts);
	positions.resize(widgets.size() + 1);
	for (sizet i = 0; i < widgets.size(); i++)
		widgets[i]->setParent(this, i);
}

void Layout::setWidgets(vector<Widget*>&& wgts) {
	initWidgets(std::move(wgts));
	postInit();
}

void Layout::insertWidget(sizet id, Widget* wgt) {
	widgets.insert(widgets.begin() + pdift(id), wgt);
	positions.emplace_back();
	reinitWidgets(id);
}

void Layout::deleteWidget(sizet id) {
	delete widgets[id];
	widgets.erase(widgets.begin() + pdift(id));
	positions.pop_back();
	reinitWidgets(id);
}

void Layout::reinitWidgets(sizet id) {
	for (; id < widgets.size(); id++)
		widgets[id]->setParent(this, id);
	postInit();
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

void Layout::onNavSelect(Direction) {
	World::scene()->updateSelect(findFirstSelectable());
}

Interactable* Layout::findFirstSelectable() const {
	for (Widget* it : widgets) {
		if (Layout* box = dynamic_cast<Layout*>(it)) {
			if (Interactable* wgt = box->findFirstSelectable())
				return wgt;
		} else if (it->selectable())
			return it;
	}
	return World::scene()->getFirstSelect();
}

void Layout::navSelectNext(sizet id, int mid, Direction dir) {
	if (dir.vertical() == vertical && (dir.positive() ? id < widgets.size() - 1 : id))
		scanSequential(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

void Layout::navSelectFrom(int mid, Direction dir) {
	if (dir.vertical() == vertical)
		scanSequential(dir.positive() ? SIZE_MAX : widgets.size(), mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::scanSequential(sizet id, int mid, Direction dir) {
	for (sizet mov = btom<sizet>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->selectable(););
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	sizet id = 0;
	for (int hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->selectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); id++);
	if (id == widgets.size())
		while (--id < widgets.size() && !widgets[id]->selectable());

	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

void Layout::navSelectWidget(sizet id, int mid, Direction dir) {
	if (Layout* lay = dynamic_cast<Layout*>(widgets[id]))
		lay->navSelectFrom(mid, dir);
	else if (widgets[id]->selectable())
		World::scene()->updateSelect(widgets[id]);
}

// ROOT LAYOUT

const vec4 RootLayout::defaultBgColor(0.f);
const vec4 RootLayout::uniformBgColor(0.f, 0.f, 0.f, 0.4f);

RootLayout::RootLayout(Size relSize, vector<Widget*>&& children, bool vertical, int spacing, const vec4& bgColor) :
	Layout(relSize, std::move(children), vertical, spacing),
	bgColor(bgColor)
{}

void RootLayout::draw() const {
	Quad::draw(Rect(ivec2(0), World::window()->screenView()), bgColor, World::scene()->blank());	// dim other widgets
	Layout::draw();
}

ivec2 RootLayout::position() const {
	return ivec2(0);
}

ivec2 RootLayout::size() const {
	return World::window()->guiView();
}

Rect RootLayout::frame() const {
	return Rect(ivec2(0), World::window()->guiView());
}

// POPUP

const vec4 Popup::colorBackground(0.42f, 0.05f, 0.f, 1.f);

Popup::Popup(const pair<Size, Size>& relSize, vector<Widget*>&& children, BCall kcall, BCall ccall, bool vertical, int spacing, const vec4& bgColor) :
	RootLayout(relSize.first, std::move(children), vertical, spacing, bgColor),
	kcall(kcall),
	ccall(ccall),
	sizeY(relSize.second)
{}

void Popup::draw() const {
	Rect rct = rect();
	Quad::draw(Rect(ivec2(0), World::window()->screenView()), bgColor, World::scene()->blank());					// dim other widgets
	Quad::draw(Rect(rct.pos() - margin, rct.size() + margin * 2), colorBackground, World::scene()->blank());		// draw background
	Layout::draw();
}

ivec2 Popup::position() const {
	return (World::window()->guiView() - size()) / 2;
}

ivec2 Popup::size() const {
	vec2 res = World::window()->guiView();
	return ivec2(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

// OVERLAY

Overlay::Overlay(const pair<Size, Size>& relPos, const pair<Size, Size>& relSize, vector<Widget*>&& children, BCall kcall, BCall ccall, bool vertical, int spacing, const vec4& bgColor) :
	Popup(relSize, std::move(children), kcall, ccall, vertical, spacing, bgColor),
	relPos(relPos),
	on(false)
{}

void Overlay::draw() const {
	Quad::draw(Rect(ivec2(0), World::window()->screenView()), bgColor, World::scene()->blank());	// dim other widgets
	Quad::draw(rect(), colorBackground, World::scene()->blank());									// draw background
	Layout::draw();
}

ivec2 Overlay::position() const {
	vec2 res = World::window()->guiView();
	return ivec2(relPos.first.usePix ? relPos.first.pix : relPos.first.prc * res.x, relPos.second.usePix ? relPos.second.pix : relPos.second.prc * res.y);
}

void Overlay::setOn(bool yes) {
	on = yes;
	World::prun(on ? kcall : ccall, nullptr);
}

// SCROLL AREA

ScrollArea::ScrollArea(Size relSize, vector<Widget*>&& children, bool vertical, int spacing, Layout* parent, sizet id) :
	Layout(relSize, std::move(children), vertical, spacing, parent, id)
{}

void ScrollArea::draw() const {
	mvec2 vis = visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; i++)
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

void ScrollArea::scrollToSelected() {
	for (Widget* child = dynamic_cast<Widget*>(World::scene()->getSelect()); child; child = child->getParent())
		if (child->getParent() == this) {
			if (int cpos = child->position()[vertical], fpos = position()[vertical]; cpos < fpos)
				scrollToWidgetPos(child->getID());
			else if (cpos + child->size()[vertical] > fpos + size()[vertical])
				scrollToWidgetEnd(child->getID());
			break;
		}
}

mvec2 ScrollArea::visibleWidgets() const {
	mvec2 ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	for (; ival.x < widgets.size() && wgtREnd(ival.x) < scroll.listPos[vertical]; ival.x++);
	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = scroll.listPos[vertical] + size()[vertical]; ival.y < widgets.size() && wgtRPos(ival.y) <= end; ival.y++);
	return ival;
}
