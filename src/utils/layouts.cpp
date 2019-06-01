#include "engine/world.h"

// LAYOUT

Layout::Layout(Size relSize, vector<Widget*> children, bool vertical, int spacing, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	spacing(spacing),
	vertical(vertical)
{
	initWidgets(std::move(children));
}

Layout::~Layout() {
	clear(widgets);
}

Rect Layout::draw() const {
	for (Widget* it : widgets)
		it->draw();
	return Rect();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::onResize() {
	// get amount of space for widgets with prc and get sum of widget's prc
	vec2i wsiz = size();
	float space = float(wsiz[vertical] - int(widgets.size()-1) * spacing);
	float total = 0;
	for (Widget* it : widgets) {
		if (it->getRelSize().usePix)
			space -= it->getRelSize().pix;
		else
			total += it->getRelSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	vec2i pos = 0;
	for (sizet i = 0; i < widgets.size(); i++) {
		positions[i] = pos;
		pos[vertical] += (widgets[i]->getRelSize().usePix ? widgets[i]->getRelSize().pix : int(widgets[i]->getRelSize().prc * space / total)) + spacing;
	}
	positions.back() = vec2i::swap(wsiz[!vertical], pos[vertical], !vertical);

	// do the same for children
	for (Widget* it : widgets)
		it->onResize();
}

void Layout::postInit() {
	onResize();
	for (Widget* it : widgets)
		it->postInit();
}

void Layout::onMouseMove(vec2i mPos, vec2i mMov) {
	for (Widget* it : widgets)
		it->onMouseMove(mPos, mMov);
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

void Layout::deleteWidget(sizet id) {
	delete widgets[id];
	widgets.erase(widgets.begin() + pdift(id));
	positions.pop_back();

	for (sizet i = id; i < widgets.size(); i++)
		widgets[i]->setParent(this, i);
	postInit();
}

vec2i Layout::position() const {
	return parent ? parent->wgtPosition(pcID) : 0;
}

vec2i Layout::size() const {
	return parent ? parent->wgtSize(pcID) : World::winSys()->windowSize();
}

Rect Layout::frame() const {
	return parent ? parent->frame() : Rect(0, World::winSys()->windowSize());
}

vec2i Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

vec2i Layout::wgtSize(sizet id) const {
	return vec2i::swap(size()[!vertical], positions[id+1][vertical] - positions[id][vertical] - spacing, !vertical);
}

vec2i Layout::listSize() const {
	return positions.back() - spacing;
}

// POPUP

const vec4 Popup::colorDim(0.f, 0.f, 0.f, 0.5f);

Popup::Popup(const cvec2<Size>& relSize, vector<Widget*> children, BCall kcall, BCall ccall, bool vertical, int spacing) :
	Layout(relSize.x, std::move(children), vertical, spacing, nullptr, SIZE_MAX),
	kcall(kcall),
	ccall(ccall),
	sizeY(relSize.y)
{}

Rect Popup::draw() const {
	drawRect(Rect(0, World::winSys()->windowSize()), colorDim);	// dim other widgets
	drawRect(rect(), colorNormal);	// draw background
	return Layout::draw();
}

vec2i Popup::position() const {
	return (World::winSys()->windowSize() - size()) / 2;
}

vec2i Popup::size() const {
	vec2f res = World::winSys()->windowSize();
	return vec2i(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

Rect Popup::frame() const {
	return Rect(0, World::winSys()->windowSize());
}

// SCROLL AREA

ScrollArea::ScrollArea(Size relSize, vector<Widget*> children, bool vertical, int spacing, Layout* parent, sizet id) :
	Layout(relSize, std::move(children), vertical, spacing, parent, id),
	draggingSlider(false),
	listPos(0),
	motion(0.f),
	diffSliderMouse(0)
{}

Rect ScrollArea::draw() const {
	vec2t vis = visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.b; i < vis.t; i++)
		widgets[i]->draw();

	drawRect(barRect(), colorDark);		// draw scroll bar
	drawRect(sliderRect(), colorLight);	// draw scroll slider
	return Rect();
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);

	if (motion != 0.f) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
	}
}

void ScrollArea::postInit() {
	Layout::postInit();
	listPos = 0;
}

void ScrollArea::onHold(vec2i mPos, uint8 mBut) {
	motion = 0.f;	// get rid of scroll motion

	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->capture = this;
		if ((draggingSlider = barRect().contain(mPos))) {
			if (int sp = sliderPos(), ss = sliderSize(); outRange(mPos[vertical], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[vertical] - ss /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(vec2i mPos, vec2i mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))
		moveListPos(-mMov);
	else
		moveListPos(mMov * vec2i::swap(0, -1, !vertical));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mousePos(), mBut) && !draggingSlider)
			motion = World::scene()->getMouseMove() * vec2i::swap(0, -1, !vertical);

		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(vec2i wMov) {
	moveListPos(wMov.swap(!vertical));
	motion = 0.f;
}

Rect ScrollArea::frame() const {
	return parent ? rect().intersect(parent->frame()) : rect();
}

vec2i ScrollArea::wgtPosition(sizet id) const {
	return Layout::wgtPosition(id) - listPos;
}

vec2i ScrollArea::wgtSize(sizet id) const {
	return Layout::wgtSize(id) - vec2i::swap(barSize(), 0, !vertical);
}

void ScrollArea::setSlider(int spos) {
	int lim = listLim()[vertical];
	listPos[vertical] = std::clamp((spos - position()[vertical]) * lim / sliderLim(), 0, lim);
}

vec2i ScrollArea::listLim() const {
	vec2i wsiz = size(), lsiz = listSize();
	return vec2i(wsiz.x < lsiz.x ? lsiz.x - wsiz.x : 0, wsiz.y < lsiz.y ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::sliderSize() const {
	int siz = size()[vertical], lts = listSize()[vertical];
	return siz < lts ? siz * siz / lts : siz;
}

void ScrollArea::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		mov -= scrollThrottle * dSec;
		if (mov < 0.f)
			mov = 0.f;
	} else {
		mov += scrollThrottle * dSec;
		if (mov > 0.f)
			mov = 0.f;
	}
}

Rect ScrollArea::barRect() const {
	int bs = barSize();
	vec2i pos = position();
	vec2i siz = size();
	return vertical ? Rect(pos.x + siz.x - bs, pos.y, bs, siz.y) : Rect(pos.x, pos.y + siz.y - bs, siz.x, bs);
}

Rect ScrollArea::sliderRect() const {
	int bs = barSize();
	return vertical ? Rect(position().x + size().x - bs, sliderPos(), bs, sliderSize()) : Rect(sliderPos(), position().y + size().y - bs, sliderSize(), bs);
}

vec2t ScrollArea::visibleWidgets() const {
	vec2t ival = 0;
	if (widgets.empty())	// nothing to draw
		return ival;

	for (; ival.b < widgets.size() && wgtREnd(ival.b) < listPos[vertical]; ival.b++);
	ival.t = ival.b + 1;	// last is one greater than the actual last index
	for (int end = listPos[vertical] + size()[vertical]; ival.t < widgets.size() && wgtRPos(ival.t) <= end; ival.t++);
	return ival;
}
