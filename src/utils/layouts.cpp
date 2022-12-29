#include "layouts.h"
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
	parent->navSelectNext(index, World::scene()->getCamera()->screenPos(pos)[dir.horizontal()], dir);
}

// LAYOUT

Layout::Layout(const Size& size, vector<Widget*>&& children, bool vert, const Size& space) :
	Navigator(size),
	vertical(vert),
	widgets(std::move(children)),
	positions(widgets.size() + 1),
	spaceSize(space)
{}

Layout::~Layout() {
	setPtrVec(widgets);
}

void Layout::draw() {
	for (Widget* it : widgets)
		it->draw();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::onResize(Quad::Instance* ins) {
	calculateWidgetPositions();
	if (ins) {
		for (uint i = 0; i < widgets.size(); ins += widgets[i++]->numInstances())
			widgets[i]->onResize(ins);
	} else
		for (Widget* it : widgets)
			it->onResize(nullptr);
}

void Layout::postInit(Quad::Instance* ins) {
	for (uint i = 0; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	if (ins) {
		for (uint i = 0; i < widgets.size(); ins += widgets[i++]->numInstances())
			widgets[i]->postInit(ins);
	} else
		for (Widget* it : widgets)
			it->postInit(nullptr);
}

void Layout::setInstances(Quad::Instance* ins) {
	for (uint i = 0; i < widgets.size(); ins += widgets[i++]->numInstances())
		widgets[i]->setInstances(ins);
}

void Layout::calculateWidgetPositions() {
	// get amount of space for widgets with percent size and get sum of sizes
	ivec2 wsiz = size();
	float scrh = float(World::window()->getGuiView().y);
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

void Layout::setSpacing(const Size& space) {
	spaceSize = space;
	findFirstInstLayout()->onResize(nullptr);
}

ivec2 Layout::wgtPosition(uint id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(uint id) const {
	return swap(size()[!vertical], positions[id + 1][vertical] - positions[id][vertical] - sizeToPixRel(spaceSize), !vertical);
}

uint Layout::wgtInstanceId(uint id) const {
	uint iid = parent->wgtInstanceId(index);
	return iid != UINT_MAX ? iid + std::accumulate(widgets.begin(), widgets.begin() + id, 0u, [](uint v, const Widget* it) -> uint { return v + it->numInstances(); }) : iid;
}

uint Layout::numInstances() const {
	return std::accumulate(widgets.begin(), widgets.end(), 0u, [](uint v, const Widget* it) -> uint { return v + it->numInstances(); });
}

bool Layout::selectable() const {
	return !widgets.empty();
}

bool Layout::deselectWidgets() const {
	for (Widget* it : widgets)
		if (deselectWidget(it))
			return true;
	return false;
}

bool Layout::deselectWidget(Widget* wgt) {
	if (World::scene()->getCapture() == wgt)
		World::scene()->setCapture(nullptr);
	if (World::scene()->getSelect() == wgt) {
		World::scene()->deselect();
		return true;
	}
	InstLayout* box = dynamic_cast<InstLayout*>(wgt);
	return box && box->deselectWidgets();
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

// INST LAYOUT

InstLayout::InstLayout(const Size& size, vector<Widget*>&& children, bool vert, const Size& space, uint widgetInstId) :
	Layout(size, std::move(children), vert, space),
	startInst(widgetInstId)
{}

InstLayout::~InstLayout() {
	freeBuffer();
}

void InstLayout::draw() {
	if (numInsts)
		drawInstances(numInsts);
	for (Widget* it : widgets)
		it->draw();
}

void InstLayout::onResize(Instance*) {
	calculateWidgetPositions();
	if (numInsts) {
		vector<Instance> instanceData = initInstanceData();
		for (uint i = 0, qid = startInst; i < widgets.size(); qid += widgets[i++]->numInstances())
			widgets[i]->onResize(&instanceData[qid]);
		updateInstances(instanceData.data(), 0, instanceData.size());
	} else
		for (Widget* it : widgets)
			it->onResize(nullptr);
}

void InstLayout::postInit(Instance*) {
	numInsts = startInst;
	for (uint i = 0; i < widgets.size(); ++i) {
		widgets[i]->setParent(this, i);
		numInsts += widgets[i]->numInstances();
	}
	calculateWidgetPositions();

	if (numInsts) {
		vector<Instance> instanceData = initInstanceData();
		for (uint i = 0, qid = startInst; i < widgets.size(); qid += widgets[i++]->numInstances())
			widgets[i]->postInit(&instanceData[qid]);
		initBuffer();
		uploadInstances(instanceData.data(), instanceData.size());
	} else
		for (Widget* it : widgets)
			it->postInit(nullptr);
}

void InstLayout::setInstances(Instance*) {
	if (numInsts) {
		vector<Instance> instanceData = initInstanceData();
		for (uint i = 0, qid = startInst; i < widgets.size(); qid += widgets[i++]->numInstances())
			widgets[i]->setInstances(&instanceData[qid]);
		updateInstances(instanceData.data(), 0, instanceData.size());
	}
}

void InstLayout::setWidgets(vector<Widget*>&& wgts) {
	deselectWidgets();
	setPtrVec(widgets, std::move(wgts));
	positions.resize(widgets.size() + 1);
	freeBuffer();
	postInit(nullptr);
	World::scene()->updateSelect();
}

void InstLayout::insertWidgets(uint id, Widget** wgts, uint cnt) {
	widgets.insert(widgets.begin() + id, wgts, wgts + cnt);
	positions.resize(positions.size() + cnt);
	uint i;
	for (i = id; i < id + cnt; ++i) {
		widgets[i]->setParent(this, i);
		numInsts += widgets[i]->numInstances();
	}
	for (; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();

	vector<Instance> instanceData = initInstanceData();
	uint qid = startInst;
	for (i = 0; i < id; qid += widgets[i++]->numInstances())
		widgets[i]->onResize(&instanceData[qid]);
	for (; i < id + cnt; qid += widgets[i++]->numInstances())
		widgets[i]->postInit(&instanceData[qid]);
	for (; i < widgets.size(); qid += widgets[i++]->numInstances())
		widgets[i]->onResize(&instanceData[qid]);
	if (numInsts) {
		initBuffer();
		uploadInstances(instanceData.data(), instanceData.size());
	}
	World::scene()->updateSelect();
}

void InstLayout::deleteWidgets(uint id, uint cnt) {
	deleteWidgetsInit(id, cnt);
	numInsts = std::accumulate(widgets.begin(), widgets.begin() + id, startInst, [](uint v, const Widget* it) -> uint { return v + it->numInstances(); });
	for (uint i = id; i < widgets.size(); ++i) {
		widgets[i]->setParent(this, i);
		numInsts += widgets[i++]->numInstances();
	}
	if (!numInsts)
		freeBuffer();
	onResize(nullptr);
	World::scene()->updateSelect();
}

void InstLayout::deleteWidgetsInit(uint id, uint cnt) {
	deselectWidgets();
	uint end = id + cnt;
	for (uint i = id; i < end; ++i)
		delete widgets[i];
	widgets.erase(widgets.begin() + id, widgets.begin() + end);
	positions.resize(positions.size() - cnt);
}

vector<Quad::Instance> InstLayout::initInstanceData() {
	return vector<Instance>(numInsts);
}

uint InstLayout::wgtInstanceId(uint id) const {
	return std::accumulate(widgets.begin(), widgets.begin() + id, startInst, [](uint v, const Widget* it) -> uint { return v + it->numInstances(); });
}

uint InstLayout::numInstances() const {
	return 0;
}

InstLayout* InstLayout::findFirstInstLayout() {
	return this;
}

// ROOT LAYOUT

RootLayout::RootLayout(const Size& size, vector<Widget*>&& children, bool vert, const Size& space, float topSpace, const vec4& color, uint widgetInstId) :
	InstLayout(size, std::move(children), vert, space, widgetInstId),
	topSpacingFac(topSpace),
	bgColor(color)
{}

vector<Quad::Instance> RootLayout::initInstanceData() {
	vector<Instance> ins(numInsts);
	if (startInst)
		setInstance(ins.data(), Rect(ivec2(0), World::window()->getGuiView()), bgColor);	// dim other widgets
	return ins;
}

ivec2 RootLayout::position() const {
	return ivec2(0, pixTopSpacing());
}

ivec2 RootLayout::size() const {
	ivec2 res = World::window()->getGuiView();
	return ivec2(res.x, res.y - pixTopSpacing());
}

void RootLayout::setSize(const Size& size) {
	relSize = size;
	onResize(nullptr);
}

Rect RootLayout::frame() const {
	return rect();
}

int RootLayout::pixTopSpacing() const {
	return World::window()->getTitleBarHeight() ? int(topSpacingFac * float(World::window()->getGuiView().y)) : 0;
}

// TITLE BAR

ivec2 TitleBar::position() const {
	return ivec2(0);
}

ivec2 TitleBar::size() const {
	return ivec2(World::window()->getGuiView().x, World::window()->getTitleBarHeight());
}

// POPUP

Popup::Popup(const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, const Size& space, float topSpace, Widget* firstSelect, Type ctxType, const vec4& color) :
	RootLayout(size.first, std::move(children), vert, space, topSpace, color, 2),
	type(ctxType),
	kcall(okCall),
	ccall(cancelCall),
	defaultSelect(firstSelect),
	sizeY(size.second)
{}

vector<Quad::Instance> Popup::initInstanceData() {
	vector<Instance> ins(numInsts);
	setInstance(&ins[0], Rect(ivec2(0), World::window()->getGuiView()), bgColor);	// dim other widgets
	setInstance(&ins[1], getBackgroundRect(), colorBackground);						// background
	return ins;
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
	return (World::window()->getGuiView() - size()) / 2;
}

ivec2 Popup::size() const {
	ivec2 res = World::window()->getGuiView();
	return ivec2(sizeToPixAbs(relSize, res.x), sizeToPixAbs(sizeY, res.y));
}

// OVERLAY

Overlay::Overlay(const pair<Size, Size>& pos, const pair<Size, Size>& size, vector<Widget*>&& children, BCall okCall, BCall cancelCall, bool vert, bool visible, bool interactive, const Size& space, float topSpace, const vec4& color) :
	Popup(size, std::move(children), okCall, cancelCall, vert, space, topSpace, nullptr, Type::overlay, vec4(0.f)),
	relPos(pos),
	boxColor(color),
	interact(interactive),
	show(visible)
{}

void Overlay::draw() {
	if (show)
		InstLayout::draw();
}

vector<Quad::Instance> Overlay::initInstanceData() {
	vector<Instance> ins(numInsts);
	setInstance(&ins[0], Rect(ivec2(0), World::window()->getGuiView()), bgColor);	// dim other widgets
	setInstance(&ins[1], rect(), boxColor);											// background
	return ins;
}

ivec2 Overlay::position() const {
	ivec2 res = World::window()->getGuiView();
	return ivec2(sizeToPixAbs(relPos.first, res.x), std::max(sizeToPixAbs(relPos.second, res.y), pixTopSpacing()));
}

ivec2 Overlay::size() const {
	ivec2 siz = Popup::size();
	int topSpacing = pixTopSpacing();
	int ypos = sizeToPixAbs(relPos.second, World::window()->getGuiView().y);
	return ivec2(siz.x, ypos >= topSpacing ? siz.y : siz.y - topSpacing + ypos);
}

void Overlay::setShow(bool yes) {
	show = yes;
	World::prun(show ? kcall : ccall, nullptr);
	World::scene()->updateSelect();
}

// SCROLL AREA

void ScrollArea::draw() {
	drawInstances(numInsts);
	for (uint i = visWgts.x; i < visWgts.y; ++i)
		widgets[i]->draw();
}

void ScrollArea::tick(float dSec) {
	bool update = scroll.tick(dSec, listSize(), size());
	InstLayout::tick(dSec);
	if (update)
		setInstances(nullptr);
}

void ScrollArea::onResize(Instance*) {
	calculateWidgetPositions();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(listSize(), size()));
	uint oldVis = numInsts;
	setVisibleWidgets();

	vector<Instance> instanceData = initInstanceData();
	uint i;
	for (i = 0; i < visWgts.x; ++i)
		widgets[i]->onResize(nullptr);
	for (uint qid = startInst; i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->onResize(&instanceData[qid]);
	for (; i < widgets.size(); ++i)
		widgets[i]->onResize(nullptr);
	if (oldVis == numInsts)
		updateInstances(instanceData.data(), 0, instanceData.size());
	else
		uploadInstances(instanceData.data(), instanceData.size());
}

void ScrollArea::postInit(Quad::Instance*) {
	uint i;
	for (i = 0; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(listSize(), size()));
	setVisibleWidgets();

	vector<Instance> instanceData = initInstanceData();
	for (i = 0; i < visWgts.x; ++i)
		widgets[i]->postInit(nullptr);
	for (uint qid = startInst; i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->postInit(&instanceData[qid]);
	for (; i < widgets.size(); ++i)
		widgets[i]->postInit(nullptr);
	initBuffer();
	uploadInstances(instanceData.data(), instanceData.size());
}

void ScrollArea::setInstances(Quad::Instance*) {
	uint oldVis = numInsts;
	setVisibleWidgets();
	vector<Instance> instanceData = initInstanceData();
	for (uint i = visWgts.x, qid = startInst; i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->setInstances(&instanceData[qid]);
	if (numInsts == oldVis)
		updateInstances(instanceData.data(), 0, instanceData.size());
	else
		uploadInstances(instanceData.data(), instanceData.size());
}

void ScrollArea::insertWidgets(uint id, Widget** wgts, uint cnt) {
	widgets.insert(widgets.begin() + id, wgts, wgts + cnt);
	positions.resize(positions.size() + cnt);
	uint i, end = id + cnt;
	for (i = id; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(listSize(), size()));
	setVisibleWidgets();

	vector<Instance> instanceData = initInstanceData();
	uint qid = startInst;
	for (i = 0; i < id && i < visWgts.x; ++i)
		widgets[i]->onResize(nullptr);
	for (; i < id && i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->onResize(&instanceData[qid]);
	while (i < id)
		widgets[i++]->onResize(nullptr);
	while (i < end && i < visWgts.x)
		widgets[i++]->postInit(nullptr);
	for (; i < end && i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->postInit(&instanceData[qid]);
	while (i < end)
		widgets[i++]->postInit(nullptr);
	while (i < widgets.size() && i < visWgts.x)
		widgets[i++]->onResize(nullptr);
	for (; i < widgets.size() && i < visWgts.y; qid += widgets[i++]->numInstances())
		widgets[i]->onResize(&instanceData[qid]);
	while (i < widgets.size())
		widgets[i++]->onResize(nullptr);
	initBuffer();
	uploadInstances(instanceData.data(), instanceData.size());
	World::scene()->updateSelect();
}

void ScrollArea::deleteWidgets(uint id, uint cnt) {
	deleteWidgetsInit(id, cnt);
	uint i;
	for (i = id; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	onResize(nullptr);
	World::scene()->updateSelect();
}

vector<Quad::Instance> ScrollArea::initInstanceData() {
	vector<Instance> instanceData(numInsts);
	scroll.setInstances(instanceData.data(), frame(), listSize(), position(), size(), vertical);
	return instanceData;
}

void ScrollArea::setVisibleWidgets() {
	if (!widgets.empty()) {
		for (visWgts.x = 0; visWgts.x < widgets.size() && wgtREnd(visWgts.x) < scroll.listPos[vertical]; ++visWgts.x);
		numInsts = startInst + widgets[visWgts.x]->numInstances();
		visWgts.y = visWgts.x + 1;	// last is one greater than the actual last index
		for (int end = scroll.listPos[vertical] + size()[vertical]; visWgts.y < widgets.size() && wgtRPos(visWgts.y) < end; ++visWgts.y)
			numInsts += widgets[visWgts.y]->numInstances();
	} else {
		visWgts = uvec2(0);
		numInsts = startInst;
	}
}

void ScrollArea::onHold(ivec2 mPos, uint8 mBut) {
	if (scroll.hold(mPos, mBut, this, listSize(), position(), size(), vertical))
		setInstances(nullptr);
}

void ScrollArea::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, listSize(), position(), size(), vertical);
	setInstances(nullptr);
}

void ScrollArea::onUndrag(ivec2 mPos, uint8 mBut) {
	scroll.undrag(mPos, mBut, vertical);
}

void ScrollArea::onScroll(ivec2, ivec2 wMov) {
	scroll.scroll(wMov, listSize(), size(), vertical);
	setInstances(nullptr);
	World::scene()->updateSelect();
}

Rect ScrollArea::frame() const {
	return rect().intersect(parent->frame());
}

ivec2 ScrollArea::wgtPosition(uint id) const {
	return InstLayout::wgtPosition(id) - scroll.listPos;
}

ivec2 ScrollArea::wgtSize(uint id) const {
	return InstLayout::wgtSize(id) - swap(ScrollBar::barSize(listSize(), size(), vertical), 0, !vertical);
}

uint ScrollArea::wgtInstanceId(uint id) const {
	return inRange(id, visWgts.x, visWgts.y) ? std::accumulate(widgets.begin() + visWgts.x, widgets.begin() + id, startInst, [](uint v, const Widget* it) -> uint { return v + it->numInstances(); }) : UINT_MAX;
}

void ScrollArea::onNavSelect(Direction dir) {
	InstLayout::onNavSelect(dir);
	scrollToSelected();
}

void ScrollArea::navSelectNext(uint id, int mid, Direction dir) {
	InstLayout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, Direction dir) {
	InstLayout::navSelectFrom(mid, dir);
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
				setInstances(nullptr);
			} else if (cpos + child->size()[vertical] > fpos + size()[vertical]) {
				scroll.setListPos(vertical ? ivec2(scroll.listPos.x, wgtREnd(child->getIndex()) - size().y) : ivec2(wgtREnd(child->getIndex()) - size().x, scroll.listPos.y), listSize(), size());
				setInstances(nullptr);
			}
			break;
		}
}
