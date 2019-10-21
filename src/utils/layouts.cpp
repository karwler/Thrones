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
		if (it->getRelSize().usePix)
			space -= it->getRelSize().pix;
		else
			total += it->getRelSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(0);
	for (sizet i = 0; i < widgets.size(); i++) {
		positions[i] = pos;
		pos[vertical] += (widgets[i]->getRelSize().usePix ? widgets[i]->getRelSize().pix : int(widgets[i]->getRelSize().prc * space / total)) + spacing;
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

ivec2 Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(sizet id) const {
	return swap(size()[!vertical], positions[id+1][vertical] - positions[id][vertical] - spacing, !vertical);
}

ivec2 Layout::listSize() const {
	return positions.back() - spacing;
}

// ROOT LAYOUT

const vec4 RootLayout::defaultBgColor(0.f);
const vec4 RootLayout::uniformBgColor(0.f, 0.f, 0.f, 0.4f);

RootLayout::RootLayout(Size relSize, vector<Widget*>&& children, bool vertical, int spacing, const vec4& bgColor) :
	Layout(relSize, std::move(children), vertical, spacing),
	bgColor(bgColor)
{}

void RootLayout::draw() const {
	drawRect(Rect(ivec2(0), World::window()->getView()), bgColor, World::scene()->blank());	// dim other widgets
	Layout::draw();
}

ivec2 RootLayout::position() const {
	return ivec2(0);
}

ivec2 RootLayout::size() const {
	return World::window()->getView();
}

Rect RootLayout::frame() const {
	return Rect(ivec2(0), World::window()->getView());
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
	drawRect(Rect(ivec2(0), World::window()->getView()), bgColor, World::scene()->blank());					// dim other widgets
	drawRect(Rect(rct.pos() - margin, rct.size() + margin * 2), colorBackground, World::scene()->blank());	// draw background
	Layout::draw();
}

ivec2 Popup::position() const {
	return (World::window()->getView() - size()) / 2;
}

ivec2 Popup::size() const {
	vec2 res = World::window()->getView();
	return ivec2(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

// SCROLL AREA

ScrollArea::ScrollArea(Size relSize, vector<Widget*>&& children, bool vertical, int spacing, Layout* parent, sizet id) :
	Layout(relSize, std::move(children), vertical, spacing, parent, id),
	draggingSlider(false),
	listPos(0),
	motion(0.f),
	diffSliderMouse(0)
{}

void ScrollArea::draw() const {
	mvec2 vis = visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; i++)
		widgets[i]->draw();

	Rect frm = frame();
	drawRect(barRect(), frm, colorDark, World::scene()->blank());		// draw scroll bar
	drawRect(sliderRect(), frm, colorLight, World::scene()->blank());	// draw scroll slider
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);
	if (motion != vec2(0.f)) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
	}
}

void ScrollArea::postInit() {
	Layout::postInit();
	listPos = ivec2(0);
}

void ScrollArea::onHold(const ivec2& mPos, uint8 mBut) {
	motion = vec2(0.f);	// get rid of scroll motion
	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->capture = this;
		if ((draggingSlider = barRect().contain(mPos))) {
			if (int sp = sliderPos(), ss = sliderSize(); outRange(mPos[vertical], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[vertical] - ss /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(const ivec2& mPos, const ivec2& mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK)
		moveListPos(-mMov);
	else
		moveListPos(mMov * swap(0, -1, !vertical));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mousePos()) && !draggingSlider)
			motion = World::scene()->getMouseMove() * swap(0, -1, !vertical);
		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(const ivec2& wMov) {
	moveListPos(swap(wMov.x, -wMov.y, !vertical));
	motion = vec2(0.f);
}

Rect ScrollArea::frame() const {
	return rect().intersect(parent->frame());
}

ivec2 ScrollArea::wgtPosition(sizet id) const {
	return Layout::wgtPosition(id) - listPos;
}

ivec2 ScrollArea::wgtSize(sizet id) const {
	return Layout::wgtSize(id) - swap(barSize(), 0, !vertical);
}

void ScrollArea::setSlider(int spos) {
	int lim = listLim()[vertical];
	listPos[vertical] = std::clamp((spos - position()[vertical]) * lim / sliderLim(), 0, lim);
}

ivec2 ScrollArea::listLim() const {
	ivec2 wsiz = size(), lsiz = listSize();
	return ivec2(wsiz.x < lsiz.x ? lsiz.x - wsiz.x : 0, wsiz.y < lsiz.y ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::sliderSize() const {
	int siz = size()[vertical], lts = listSize()[vertical];
	return siz < lts ? siz * siz / lts : siz;
}

void ScrollArea::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		if (mov -= scrollThrottle * dSec; mov < 0.f)
			mov = 0.f;
	} else if (mov += scrollThrottle * dSec; mov > 0.f)
		mov = 0.f;
}

Rect ScrollArea::barRect() const {
	int bs = barSize();
	ivec2 pos = position();
	ivec2 siz = size();
	return vertical ? Rect(pos.x + siz.x - bs, pos.y, bs, siz.y) : Rect(pos.x, pos.y + siz.y - bs, siz.x, bs);
}

Rect ScrollArea::sliderRect() const {
	int bs = barSize();
	return vertical ? Rect(position().x + size().x - bs, sliderPos(), bs, sliderSize()) : Rect(sliderPos(), position().y + size().y - bs, sliderSize(), bs);
}

mvec2 ScrollArea::visibleWidgets() const {
	mvec2 ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	for (; ival.x < widgets.size() && wgtREnd(ival.x) < listPos[vertical]; ival.x++);
	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = listPos[vertical] + size()[vertical]; ival.y < widgets.size() && wgtRPos(ival.y) <= end; ival.y++);
	return ival;
}

void ScrollArea::moveListPos(const ivec2& mov) {
	listPos = glm::clamp(ivec2(listPos + mov), ivec2(0), listLim());
	World::scene()->updateSelect(mousePos());
}
