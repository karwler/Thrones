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

Layout::Layout(const Size& size, vector<Widget*>&& children, bool vert, const Size& space, uint widgetInstId) :
	Navigator(size),
	startInst(widgetInstId),
	widgets(std::move(children)),
	positions(widgets.size() + 1),
	spaceSize(space),
	vertical(vert)
{
	if (uint numInst = setWidgetsParent(0, startInst)) {
		instanceData.resize(numInst);
		initBuffer();
	}
}

Layout::~Layout() {
	setPtrVec(widgets);
	freeBuffer();
}

void Layout::draw() {
	if (!instanceData.empty())
		drawInstances();
	for (Widget* it : widgets)
		it->draw();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::onResize() {
	calculateWidgetPositions();
	doWidgetsResize();
}

void Layout::doWidgetsResize() {
	for (Widget* it : widgets)
		it->onResize();
	if (!instanceData.empty())
		updateInstanceData(0, instanceData.size());
}

void Layout::postInit() {
	calculateWidgetPositions();
	for (Widget* it : widgets)
		it->postInit();
	if (!instanceData.empty())
		updateInstanceData(0, instanceData.size());
}

bool Layout::setInstances() {
	for (Widget* it : widgets)
		it->setInstances();
	if (!instanceData.empty())
		updateInstance(0, instanceData.size());
	return true;
}

void Layout::calculateWidgetPositions() {
	// get amount of space for widgets with percent size and get sum of sizes
	ivec2 wsiz = size();
	float scrh = float(World::window()->getScreenView().y);
	vector<int> pixSizes(widgets.size());
	int spacing = sizeToPixRel(spaceSize);
	int totalSpacing = (widgets.size() - 1) * spacing;
	int space = wsiz[vertical] > totalSpacing ? wsiz[vertical] - totalSpacing : 0;
	float total = 0;
	for (sizet i = 0; i < widgets.size(); ++i)
		switch (const Size& siz = widgets[i]->getSize(); siz.mod) {
		case Size::rela:
			total += siz.rel;
			break;
		case Size::abso:
			pixSizes[i] = int(siz.rel * scrh);
			space -= std::min(pixSizes[i], space);
			break;
		case Size::pref:
			pixSizes[i] = *siz.ptr;
			space -= std::min(pixSizes[i], space);
			break;
		case Size::pixv:
			pixSizes[i] = siz.pix;
			space -= std::min(pixSizes[i], space);
			break;
		case Size::calc:
			pixSizes[i] = siz(widgets[i]);
			space -= std::min(pixSizes[i], space);
			break;
		}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(0);
	for (sizet i = 0; i < widgets.size(); ++i) {
		positions[i] = pos;
		if (const Size& siz = widgets[i]->getSize(); siz.mod != Size::rela)
			pos[vertical] += pixSizes[i] + spacing;
		else if (float val = siz.rel * float(space); val != 0.f)
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
	positions.resize(widgets.size() + 1);
	if (uint numInst = setWidgetsParent(0, startInst)) {
		instanceData.resize(numInst);
		initBuffer();
	} else {
		instanceData.clear();
		freeBuffer();
	}
	postInit();
	World::scene()->updateSelect();
}

void Layout::insertWidget(uint id, Widget* wgt) {
	initBuffer();
	uint qid = !widgets.empty() ? widgets[id]->getInstIndex() : startInst;
	instanceData.emplace(instanceData.begin() + qid);
	widgets.insert(widgets.begin() + id, wgt);
	positions.emplace_back();

	setWidgetsParent(id, qid);
	calculateWidgetPositions();
	wgt->postInit();
	doWidgetsResize();
	World::scene()->updateSelect();
}

void Layout::deleteWidget(uint id) {
	bool updateSelect = deselectWidget(widgets[id]);
	uint qid = widgets[id]->getInstIndex();
	delete widgets[id];
	instanceData.erase(instanceData.begin() + qid);
	widgets.erase(widgets.begin() + id);
	positions.pop_back();
	if (instanceData.empty())
		freeBuffer();

	setWidgetsParent(id, qid);
	onResize();
	if (updateSelect)
		World::scene()->updateSelect();
}

uint Layout::setWidgetsParent(uint id, uint qid) {
	for (; id < widgets.size(); ++id) {
		widgets[id]->setParent(this, id, qid);
		qid += widgets[id]->numInstances();
	}
	return qid;
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

void Layout::setSpacing(const Size& space) {
	spaceSize = space;
	onResize();
}

ivec2 Layout::wgtPosition(uint id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(uint id) const {
	return swap(size()[!vertical], positions[id+1][vertical] - positions[id][vertical] - sizeToPixRel(spaceSize), !vertical);
}

bool Layout::instanceVisible(uint) const {
	return true;
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
		scanSequential(dir.positive() ? UINT_MAX : widgets.size(), mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::navSelectNext(uint id, int mid, Direction dir) {
	if (dir.vertical() == vertical && (dir.positive() ? id < widgets.size() - 1 : id))
		scanSequential(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanSequential(uint id, int mid, Direction dir) {
	for (uint mov = btom<uint>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->selectable(););
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	uint id = 0;
	for (int hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->selectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); ++id);
	if (id == widgets.size())
		while (--id < widgets.size() && !widgets[id]->selectable());

	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::navSelectWidget(uint id, int mid, Direction dir) {
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

RootLayout::RootLayout(const Size& size, vector<Widget*>&& children, bool vert, const Size& space, float topSpace, const vec4& color, uint widgetInstId) :
	Layout(size, std::move(children), vert, space, widgetInstId),
	topSpacingFac(topSpace),
	bgColor(color)
{}

void RootLayout::onResize() {
	setInstance(dimInst, Rect(ivec2(0), World::window()->getScreenView()), bgColor, TextureCol::blank);	// dim other widgets
	Layout::onResize();
}

void RootLayout::postInit() {
	setInstance(dimInst, Rect(ivec2(0), World::window()->getScreenView()), bgColor, TextureCol::blank);	// dim other widgets
	Layout::postInit();
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

Popup::Popup(const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, const Size& space, float topSpace, Widget* firstSelect, Type ctxType, const vec4& color) :
	RootLayout(size.first, std::move(children), vert, space, topSpace, color, bgInst + 1),
	kcall(okCall),
	ccall(cancelCall),
	defaultSelect(firstSelect),
	sizeY(size.second),
	type(ctxType)
{}

void Popup::onResize() {
	setInstance(bgInst, getBackgroundRect(), colorBackground, TextureCol::blank);
	RootLayout::onResize();
}

void Popup::postInit() {
	setInstance(bgInst, getBackgroundRect(), colorBackground, TextureCol::blank);
	RootLayout::postInit();
}

void Popup::onClick(ivec2 mPos, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) && !getBackgroundRect().contain(mPos))
		World::prun(ccall, nullptr);
}

Rect Popup::getBackgroundRect() const {
	Rect rct = rect();
	int margin = *World::pgui()->getSize(GuiGen::SizeRef::popupMargin);
	return Rect(rct.pos() - margin, rct.size() + margin * 2);
}

ivec2 Popup::position() const {
	return (World::window()->getScreenView() - size()) / 2;
}

ivec2 Popup::size() const {
	ivec2 res = World::window()->getScreenView();
	return ivec2(sizeToPixAbs(relSize, res.x), sizeToPixAbs(sizeY, res.y));
}

// OVERLAY

Overlay::Overlay(const pair<Size, Size>& pos, const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, bool visible, bool interactive, const Size& space, float topSpace, const vec4& color) :
	Popup(size, std::move(children), okCall, cancelCall, vert, space, topSpace, nullptr, Type::overlay, vec4(0.f)),
	show(visible),
	interact(interactive),
	relPos(pos),
	boxColor(color)
{}

void Overlay::draw() {
	if (show)
		Layout::draw();
}

void Overlay::onResize() {
	setInstance(bgInst, rect(), boxColor, TextureCol::blank);
	RootLayout::onResize();
}

void Overlay::postInit() {
	setInstance(bgInst, rect(), boxColor, TextureCol::blank);
	RootLayout::postInit();
}

ivec2 Overlay::position() const {
	ivec2 res = World::window()->getScreenView();
	return ivec2(sizeToPixAbs(relPos.first, res.x), std::max(sizeToPixAbs(relPos.second, res.y), pixTopSpacing()));
}

ivec2 Overlay::size() const {
	ivec2 siz = Popup::size();
	int topSpacing = pixTopSpacing();
	int ypos = sizeToPixAbs(relPos.second, World::window()->getScreenView().y);
	return ivec2(siz.x, ypos >= topSpacing ? siz.y : siz.y - topSpacing + ypos);
}

void Overlay::setShow(bool yes) {
	show = yes;
	World::prun(show ? kcall : ccall, nullptr);
	World::scene()->updateSelect();
}

// SCROLL AREA

void ScrollArea::tick(float dSec) {
	bool update = scroll.tick(dSec, listSize(), size());
	Layout::tick(dSec);
	if (update)
		setInstances();
}

void ScrollArea::doWidgetsResize() {
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(listSize(), size()));
	setVisibleInstances();
	for (Widget* it : widgets)
		it->onResize();
	scroll.setInstances(this, visibleInsts.x, frame(), listSize(), position(), size(), vertical);
	updateInstanceData(visibleInsts.x, visibleInsts.y - visibleInsts.x);	// TODO: use subdata if range hasn't changed
}

void ScrollArea::postInit() {
	calculateWidgetPositions();
	setVisibleInstances();
	for (Widget* it : widgets)
		it->postInit();
	scroll.setInstances(this, visibleInsts.x, frame(), listSize(), position(), size(), vertical);
	updateInstanceData(visibleInsts.x, visibleInsts.y - visibleInsts.x);
}

bool ScrollArea::setInstances() {
	uint oldVis = visibleInsts.y - visibleInsts.x;
	setVisibleInstances();
	for (Widget* it : widgets)
		it->setInstances();
	scroll.setInstances(this, visibleInsts.x, frame(), listSize(), position(), size(), vertical);

	if (uint newVis = visibleInsts.y - visibleInsts.x; newVis == oldVis)
		updateInstanceOffs(visibleInsts.x, newVis);
	else
		updateInstanceData(visibleInsts.x, newVis);
	return true;
}

void ScrollArea::setVisibleInstances() {
	if (widgets.empty()) {
		visibleInsts = uvec2(0, ScrollBar::numInstances);
		return;
	}

	uint pos = 0;
	for (; pos < widgets.size() && wgtREnd(pos) < scroll.listPos[vertical]; ++pos);
	uint fin = pos + 1;	// last is one greater than the actual last index
	for (int end = scroll.listPos[vertical] + size()[vertical]; fin < widgets.size() && wgtRPos(fin) < end; ++fin);
	visibleInsts = uvec2(widgets[pos]->getInstIndex() - ScrollBar::numInstances, widgets[fin-1]->getInstIndex() + widgets[fin-1]->numInstances());
}

void ScrollArea::onHold(ivec2 mPos, uint8 mBut) {
	if (scroll.hold(mPos, mBut, this, listSize(), position(), size(), vertical))
		setInstances();
}

void ScrollArea::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, listSize(), position(), size(), vertical);
	setInstances();
}

void ScrollArea::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, vertical);
}

void ScrollArea::onScroll(ivec2 wMov) {
	scroll.scroll(wMov, listSize(), size(), vertical);
	setInstances();
	World::scene()->updateSelect();
}

Rect ScrollArea::frame() const {
	return rect().intersect(parent->frame());
}

ivec2 ScrollArea::wgtPosition(uint id) const {
	return Layout::wgtPosition(id) - scroll.listPos;
}

ivec2 ScrollArea::wgtSize(uint id) const {
	return Layout::wgtSize(id) - swap(ScrollBar::barSize(listSize(), size(), vertical), 0, !vertical);
}

bool ScrollArea::instanceVisible(uint qid) const {
	return qid >= visibleInsts.x + ScrollBar::numInstances && qid < visibleInsts.y;
}

void ScrollArea::onNavSelect(Direction dir) {
	Layout::onNavSelect(dir);
	scrollToSelected();
}

void ScrollArea::navSelectNext(uint id, int mid, Direction dir) {
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
			if (int cpos = child->position()[vertical], fpos = position()[vertical]; cpos < fpos) {
				scroll.setListPos(vertical ? ivec2(scroll.listPos.x, wgtRPos(child->getIndex())) : ivec2(wgtRPos(child->getIndex()), scroll.listPos.y), listSize(), size());
				setInstances();
			} else if (cpos + child->size()[vertical] > fpos + size()[vertical]) {
				scroll.setListPos(vertical ? ivec2(scroll.listPos.x, wgtREnd(child->getIndex()) - size().y) : ivec2(wgtREnd(child->getIndex()) - size().x, scroll.listPos.y), listSize(), size());
				setInstances();
			}
			break;
		}
}
