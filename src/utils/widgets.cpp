#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/shaders.h"
#include "engine/world.h"
#include "prog/progs.h"

// QUAD

void Quad::initBuffer() {
	if (bufferInitialized())
		return;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(uptrt(0)));

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);

	glEnableVertexAttribArray(ShaderGui::rect);
	glVertexAttribPointer(ShaderGui::rect, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, rect)));
	glVertexAttribDivisor(ShaderGui::rect, 1);
	glEnableVertexAttribArray(ShaderGui::uvrc);
	glVertexAttribPointer(ShaderGui::uvrc, vec4::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, uvrc)));
	glVertexAttribDivisor(ShaderGui::uvrc, 1);
	glEnableVertexAttribArray(ShaderGui::zloc);
	glVertexAttribPointer(ShaderGui::zloc, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, zloc)));
	glVertexAttribDivisor(ShaderGui::zloc, 1);
	glEnableVertexAttribArray(Shader::diffuse);
	glVertexAttribPointer(Shader::diffuse, decltype(Instance::color)::length(), GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, color)));
	glVertexAttribDivisor(Shader::diffuse, 1);
	glEnableVertexAttribArray(Shader::texid);
	glVertexAttribIPointer(Shader::texid, 1, GL_UNSIGNED_INT, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, texid)));
	glVertexAttribDivisor(Shader::texid, 1);
}

void Quad::freeBuffer() {
	if (!bufferInitialized())
		return;

	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::texid);
	glDisableVertexAttribArray(Shader::diffuse);
	glDisableVertexAttribArray(ShaderGui::zloc);
	glDisableVertexAttribArray(ShaderGui::uvrc);
	glDisableVertexAttribArray(ShaderGui::rect);
	glDisableVertexAttribArray(Shader::vpos);
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	vao = vbo = ibo = 0;
}

void Quad::drawInstances(GLsizei icnt) {
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, corners, icnt);
}

void Quad::updateInstance(uint id, uint cnt) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferSubData(GL_ARRAY_BUFFER, iptrt(id * sizeof(Instance)), GLsizei(cnt * sizeof(Instance)), &instanceData[id]);
}

void Quad::updateInstanceOffs(uint id, uint cnt) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, GLsizei(cnt * sizeof(Instance)), &instanceData[id]);
}

void Quad::updateInstanceData(uint id, uint cnt) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(cnt * sizeof(Instance)), &instanceData[id], GL_DYNAMIC_DRAW);
}

void Quad::setInstance(uint id, const Rect& rect, const vec4& color, const TexLoc& tex, float z) {
	float tbound = float(World::scene()->getTexCol()->getRes());
	Instance& ins = instanceData[id];
	ins.rect = rect;
	ins.uvrc = Rectf(float(tex.rct.x) / tbound, float(tex.rct.y) / tbound, float(tex.rct.w) / tbound, float(tex.rct.h) / tbound);
	ins.zloc = z;
	ins.color = color;
	ins.texid = tex.tid;
}

void Quad::setInstance(uint id, const Rect& rect, const Rect& frame, const vec4& color, const TexLoc& tex, float z) {
	Rect isct = rect.intersect(frame);
	float tbound = float(World::scene()->getTexCol()->getRes());
	Rectf trct = tex.rct;
	vec2 rsiz = rect.size();
	Instance& ins = instanceData[id];
	ins.rect = isct;
	ins.uvrc = Rectf(
		(trct.x + float(isct.x - rect.x) / rsiz.x * trct.w) / tbound,
		(trct.y + float(isct.y - rect.y) / rsiz.y * trct.h) / tbound,
		(ins.rect.w / rsiz.x * trct.w) / tbound,
		(ins.rect.h / rsiz.y * trct.h) / tbound
	);
	ins.zloc = z;
	ins.color = color;
	ins.texid = tex.tid;
}

// SCROLL BAR

void ScrollBar::setInstances(Quad* quad, uint id, ivec2 listSize, ivec2 pos, ivec2 size, bool vert, float z) const {
	if (listSize[vert] > size[vert]) {
		quad->setInstance(id, barRect(listSize, pos, size, vert), Widget::colorDark, TextureCol::blank, z);
		quad->setInstance(id + 1, sliderRect(listSize, pos, size, vert), Widget::colorLight, TextureCol::blank, z);
	} else {
		quad->setInstance(id);
		quad->setInstance(id + 1);
	}
}

void ScrollBar::setInstances(Quad* quad, uint id, const Rect& frame, ivec2 listSize, ivec2 pos, ivec2 size, bool vert, float z) const {
	if (listSize[vert] > size[vert]) {
		quad->setInstance(id, barRect(listSize, pos, size, vert), frame, Widget::colorDark, TextureCol::blank, z);
		quad->setInstance(id + 1, sliderRect(listSize, pos, size, vert), frame, Widget::colorLight, TextureCol::blank, z);
	} else {
		quad->setInstance(id);
		quad->setInstance(id + 1);
	}
}

bool ScrollBar::tick(float dSec, ivec2 listSize, ivec2 size) {
	if (motion != vec2(0.f)) {
		moveListPos(motion, listSize, size);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
		World::scene()->updateSelect();
		return true;
	}
	return false;
}

bool ScrollBar::hold(ivec2 mPos, uint8 mBut, Interactable* wgt, ivec2 listSize, ivec2 pos, ivec2 size, bool vert) {
	bool moved = false;
	motion = vec2(0.f);	// get rid of scroll motion
	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->setCapture(wgt);
		if ((draggingSlider = barRect(listSize, pos, size, vert).contain(mPos))) {
			int sp = sliderPos(listSize, pos, size, vert), ss = sliderSize(listSize, size, vert);
			if (moved = outRange(mPos[vert], sp, sp + ss); moved)	// if mouse outside of slider but inside bar
				setSlider(mPos[vert] - ss / 2, listSize, pos, size, vert);
			diffSliderMouse = mPos.y - sliderPos(listSize, pos, size, vert);	// get difference between mouse y and slider y
		}
		SDL_CaptureMouse(SDL_TRUE);
	}
	return moved;
}

void ScrollBar::drag(ivec2 mPos, ivec2 mMov, ivec2 listSize, ivec2 pos, ivec2 size, bool vert) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse, listSize, pos, size, vert);
	else
		moveListPos(mMov * swap(0, -1, !vert), listSize, size);
}

void ScrollBar::undrag(uint8 mBut, bool vert) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(World::window()->mousePos()) && !draggingSlider)
			motion = World::input()->getMouseMove() * swap(0, -1, !vert);
		World::scene()->setCapture(nullptr);	// should call cancelDrag through the captured widget
	}
}

void ScrollBar::cancelDrag() {
	SDL_CaptureMouse(SDL_FALSE);
	draggingSlider = false;
}

void ScrollBar::scroll(ivec2 wMov, ivec2 listSize, ivec2 size, bool vert) {
	moveListPos(swap(wMov.x, wMov.y, !vert), listSize, size);
	motion = vec2(0.f);
}

void ScrollBar::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		if (mov -= throttle * dSec; mov < 0.f)
			mov = 0.f;
	} else if (mov += throttle * dSec; mov > 0.f)
		mov = 0.f;
}

void ScrollBar::setSlider(int spos, ivec2 listSize, ivec2 pos, ivec2 size, bool vert) {
	int lim = listLim(listSize, size)[vert];
	listPos[vert] = std::clamp((spos - pos[vert]) * lim / sliderLim(listSize, size, vert), 0, lim);
}

int ScrollBar::barSize(ivec2 listSize, ivec2 size, bool vert) {
	return listSize[vert] > size[vert] ? *World::pgui()->getSize(GuiGen::SizeRef::scrollBarWidth) : 0;
}

Rect ScrollBar::barRect(ivec2 listSize, ivec2 pos, ivec2 size, bool vert) {
	int bs = barSize(listSize, size, vert);
	return vert ? Rect(pos.x + size.x - bs, pos.y, bs, size.y) : Rect(pos.x, pos.y + size.y - bs, size.x, bs);
}

Rect ScrollBar::sliderRect(ivec2 listSize, ivec2 pos, ivec2 size, bool vert) const {
	int bs = barSize(listSize, size, vert);
	int sp = sliderPos(listSize, pos, size, vert);
	int ss = sliderSize(listSize, size, vert);
	return vert ? Rect(pos.x + size.x - bs, sp, bs, ss) : Rect(sp, pos.y + size.y - bs, ss, bs);
}

// WIDGET

uint Widget::setTopInstances(Quad&, uint qid) {
	return qid;
}

bool Widget::setInstances() {
	return false;
}

uint Widget::numInstances() const {
	return 0;
}

ivec2 Widget::position() const {
	return parent->wgtPosition(index);
}

ivec2 Widget::size() const {
	return parent->wgtSize(index);
}

void Widget::setSize(const Size& size) {
	relSize = size;
	parent->onResize();
}

Rect Widget::frame() const {
	return parent->frame();
}

bool Widget::selectable() const {
	return false;
}

void Widget::setParent(Layout* pnt, uint id, uint inst) {
	parent = pnt;
	index = id;
	instIndex = inst;
}

int Widget::sizeToPixRel(const Size& siz) const {
	switch (siz.mod) {
	case Size::rela:
		return int(siz.rel * float(size().y));
	case Size::abso:
		return int(siz.rel * float(World::window()->getScreenView().y));
	case Size::pref:
		return *siz.ptr;
	case Size::pixv:
		return siz.pix;
	case Size::calc:
		return siz(this);
	}
	return 0;
}

int Widget::sizeToPixAbs(const Size& siz, int res) const {
	switch (siz.mod) {
	case Size::rela: case Size::abso:
		return int(siz.rel * float(res));
	case Size::pref:
		return *siz.ptr;
	case Size::pixv:
		return siz.pix;
	case Size::calc:
		return siz(this);
	}
	return 0;
}

bool Widget::sizeValid() const {
	switch (relSize.mod) {
	case Size::rela: case Size::abso:
		return relSize.rel > 0.f;
	case Size::pref:
		return *relSize.ptr > 0;
	case Size::pixv:
		return relSize.pix > 0;
	case Size::calc:
		return relSize(this) > 0;
	}
	return false;
}

// BUTTON

Button::Button(const Size& size, BCall leftCall, BCall rightCall, string&& tooltip, float dim, const TexLoc& tex, const vec4& clr) :
	Widget(size),
	lcall(leftCall),
	rcall(rightCall),
	tipStr(std::move(tooltip)),
	bgTex(tex),
	color(clr),
	dimFactor(dim)
{
	if (World::sets()->tooltips) {
		auto [theight, tlimit] = tipParams();
		tipTex = World::scene()->getTexCol()->insert(World::fonts()->render(tipStr.c_str(), theight, tlimit));
	}
}

Button::~Button() {
	World::scene()->getTexCol()->erase(tipTex);
}

void Button::onResize() {
	setInstances();
}

void Button::postInit() {
	setInstances();
}

bool Button::setInstances() {
	if (parent->instanceVisible(instIndex)) {
		parent->setInstance(instIndex, rect(), frame(), baseColor(), bgTex);
		return true;
	}
	return false;
}

void Button::updateInstances() {
	if (setInstances())
		parent->updateInstance(instIndex, numInstances());
}

uint Button::numInstances() const {
	return 1;
}

void Button::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(lcall, this);
	else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Button::onHover() {
	updateInstances();
}

void Button::onUnhover() {
	updateInstances();
}

void Button::onNavSelect(Direction dir) {
	parent->navSelectNext(index, dir.vertical() ? center().x : center().y, dir);
}

bool Button::selectable() const {
	return lcall || rcall || !tipTex.blank();
}

uint Button::setTooltipInstances(Quad& quad, uint qid) const {
	if (tipTex.blank())
		return qid;

	int ofs = World::window()->getCursorHeight() + cursorMargin;
	ivec2 pos = World::window()->mousePos() + ivec2(0, ofs);
	ivec2 siz = tipTex.rct.size() + tooltipMargin * 2;
	ivec2 res = World::window()->getScreenView();
	if (pos.x + siz.x > res.x)
		pos.x = res.x - siz.x;
	if (pos.y + siz.y > res.y)
		pos.y = pos.y - ofs - siz.y;

	quad.setInstance(qid++, Rect(pos, siz), colorTooltip, TextureCol::blank, -1.f);
	quad.setInstance(qid++, Rect(pos + tooltipMargin, tipTex.rct.size()), vec4(1.f), tipTex, -1.f);
	return qid;
}

void Button::setTooltip(string&& tooltip) {
	tipStr = std::move(tooltip);
	updateTipTex();
}

void Button::updateTipTex() {
	if (World::sets()->tooltips) {
		auto [theight, tlimit] = tipParams();
		World::scene()->getTexCol()->replace(tipTex, World::fonts()->render(tipStr.c_str(), theight, tlimit));
	} else
		World::scene()->getTexCol()->erase(tipTex);
}

pair<int, uint> Button::tipParams() {
	int theight = *World::pgui()->getSize(GuiGen::SizeRef::tooltipHeight);
	uint tlimit = *World::pgui()->getSize(GuiGen::SizeRef::tooltipLimit);
	if (tipStr.find('\n') >= tipStr.length())
		return pair(theight, tlimit);

	uint width = 0;
	for (char* pos = tipStr.data(); *pos;) {
		sizet len = strcspn(pos, "\n");
		if (uint siz = World::fonts()->length(pos, theight, len); siz > width)
			if (width = std::min(siz, tlimit); width == tlimit)
				break;
		pos += pos[len] ? len + 1 : len;
	}
	return pair(theight, width);
}

void Button::setDim(float factor) {
	dimFactor = factor;
	updateInstances();
}

vec4 Button::baseColor() const {
	return dimColor(World::scene()->getSelect() != this ? color : color * selectFactor);
}

// CHECK BOX

CheckBox::CheckBox(const Size& size, bool checked, BCall leftCall, BCall rightCall, string&& tooltip, float dim, const TexLoc& tex, const vec4& clr) :
	Button(size, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	on(checked)
{}

void CheckBox::onResize() {
	setInstances();
}

void CheckBox::postInit() {
	setInstances();
}

bool CheckBox::setInstances() {
	if (parent->instanceVisible(instIndex)) {
		Rect frm = frame();
		parent->setInstance(instIndex, rect(), frm, baseColor(), bgTex);
		parent->setInstance(instIndex + 1, boxRect(), frm, dimColor(boxColor()), TextureCol::blank);
		return true;
	}
	return false;
}

uint CheckBox::numInstances() const {
	return 2;
}

void CheckBox::onClick(ivec2 mPos, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) && (lcall || rcall))
		setOn(!on);
	Button::onClick(mPos, mBut);
}

void CheckBox::setOn(bool checked) {
	on = checked;
	if (parent->instanceVisible(instIndex)) {
		parent->setInstance(instIndex + 1, boxRect(), frame(), dimColor(boxColor()), TextureCol::blank);
		parent->updateInstance(instIndex + 1);
	}
}

Rect CheckBox::boxRect() const {
	ivec2 siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& size, int val, int minimum, int maximum, int navStep, BCall finishCall, BCall updateCall, string&& tooltip, float dim, const TexLoc& tex, const vec4& clr) :
	Button(size, finishCall, updateCall, std::move(tooltip), dim, tex, clr),
	value(val),
	vmin(minimum),
	vmax(maximum),
	keyStep(navStep),
	oldVal(val)
{}

void Slider::onResize() {
	setInstances();
}

void Slider::postInit() {
	setInstances();
}

bool Slider::setInstances() {
	if (parent->instanceVisible(instIndex)) {
		Rect frm = frame();
		parent->setInstance(instIndex, rect(), frm, baseColor(), bgTex);
		parent->setInstance(instIndex + 1, barRect(), frm, dimColor(colorDark), TextureCol::blank);
		parent->setInstance(instIndex + 2, sliderRect(), frm, dimColor(colorLight), TextureCol::blank);
		return true;
	}
	return false;
}

uint Slider::numInstances() const {
	return 3;
}

void Slider::onHold(ivec2 mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || rcall)) {
		World::scene()->setCapture(this);
		if (int sp = sliderPos(), sw = size().y / sliderWidthFactor; outRange(mPos.x, sp, sp + sw))	// if mouse outside of slider
			setSlider(mPos.x - sw / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
		SDL_CaptureMouse(SDL_TRUE);
	}
}

void Slider::onDrag(ivec2 mPos, ivec2) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {	// if dragging slider stop dragging slider
		SDL_CaptureMouse(SDL_FALSE);
		World::scene()->setCapture(nullptr, false);
		World::prun(lcall, this);
	}
}

void Slider::onKeyDown(const SDL_KeyboardEvent& key) {
	World::input()->callBindings(key, [this](Binding::Type id) { onInput(id); });
}

void Slider::onJButtonDown(uint8 but) {
	World::input()->callBindings(JoystickButton(but), [this](Binding::Type id) { onInput(id); });
}

void Slider::onJHatDown(uint8 hat, uint8 val) {
	World::input()->callBindings(AsgJoystick(hat, val), [this](Binding::Type id) { onInput(id); });
}

void Slider::onJAxisDown(uint8 axis, bool positive) {
	World::input()->callBindings(AsgJoystick(JoystickAxis(axis), positive), [this](Binding::Type id) { onInput(id); });
}

void Slider::onGButtonDown(SDL_GameControllerButton but) {
	World::input()->callBindings(but, [this](Binding::Type id) { onInput(id); });
}

void Slider::onGAxisDown(SDL_GameControllerAxis axis, bool positive) {
	World::input()->callBindings(AsgGamepad(axis, positive), [this](Binding::Type id) { onInput(id); });
}

void Slider::onInput(Binding::Type bind) {
	switch (bind) {
	case Binding::Type::up: case Binding::Type::textHome:
		setVal(vmin);
		World::prun(rcall, this);
		break;
	case Binding::Type::down: case Binding::Type::textEnd:
		setVal(vmax);
		World::prun(rcall, this);
		break;
	case Binding::Type::left:
		if (value > vmin) {
			setVal(value - keyStep);
			World::prun(rcall, this);
		}
		break;
	case Binding::Type::right:
		if (value < vmax) {
			setVal(value + keyStep);
			World::prun(rcall, this);
		}
		break;
	case Binding::Type::confirm:
		onUndrag(SDL_BUTTON_LEFT);
		break;
	case Binding::Type::cancel:
		World::scene()->setCapture(nullptr);
	}
}

void Slider::onCancelCapture() {
	setVal(oldVal);
	SDL_CaptureMouse(SDL_FALSE);
	World::prun(rcall, this);
}

void Slider::setVal(int val) {
	value = std::clamp(val, vmin, vmax);
	if (parent->instanceVisible(instIndex)) {
		parent->setInstance(instIndex + 2, sliderRect(), frame(), dimColor(colorLight), TextureCol::blank);
		parent->updateInstance(instIndex + 2);
	}
}

void Slider::set(int val, int minimum, int maximum) {
	vmin = minimum;
	vmax = maximum;
	setVal(val);
}

void Slider::setSlider(int xpos) {
	setVal(vmin + int(std::round(float((xpos - position().x - size().y / barMarginFactor) * (vmax - vmin)) / float(sliderLim()))));
	World::prun(rcall, this);
}

Rect Slider::barRect() const {
	ivec2 siz = size();
	int height = siz.y / (barMarginFactor / 2);
	return Rect(position() + siz.y / barMarginFactor, siz.x - height, height);
}

Rect Slider::sliderRect() const {
	ivec2 pos = position(), siz = size();
	int margin = siz.y / barMarginFactor;
	return Rect(sliderPos(), pos.y + margin / 2, siz.y / sliderWidthFactor, siz.y - margin);
}

int Slider::sliderPos() const {
	int lim = vmax - vmin;
	return position().x + size().y / barMarginFactor + (lim ? (value - vmin) * sliderLim() / lim : sliderLim());
}

int Slider::sliderLim() const {
	ivec2 siz = size();
	return siz.x - siz.y / 2 - siz.y / sliderWidthFactor;
}

// LABEL

Label::Label(const Size& size, string line, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, const TexLoc& tex, const vec4& clr) :
	Button(size, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	text(std::move(line)),
	halign(align),
	showBG(bg)
{}

Label::Label(string line, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, const TexLoc& tex, const vec4& clr) :
	Button(precalcWidth, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	text(std::move(line)),
	halign(align),
	showBG(bg)
{}

Label::~Label() {
	World::scene()->getTexCol()->erase(textTex);
}

void Label::onResize() {
	updateTextTex();
	setInstances();
}

void Label::postInit() {
	ivec2 siz = size();
	SDL_Surface* txt = World::fonts()->render(text.c_str(), siz.y);
	textTex = World::scene()->getTexCol()->insert(txt, restrictTextTexSize(txt, siz));
	SDL_FreeSurface(txt);
	setInstances();
}

bool Label::setInstances() {
	if (parent->instanceVisible(instIndex)) {
		if (showBG)
			parent->setInstance(instIndex, rect(), frame(), baseColor(), bgTex);
		setTextInstance();
		return true;
	}
	return false;
}

void Label::setTextInstance() {
	if (!textTex.blank())
		parent->setInstance(instIndex + showBG, textRect(), frame(), dimColor(vec4(1.f)), textTex);
	else
		parent->setInstance(instIndex + showBG);
}

void Label::updateTextInstance() {
	if (parent->instanceVisible(instIndex)) {
		setTextInstance();
		parent->updateInstance(instIndex + showBG);
	}
}

uint Label::numInstances() const {
	return showBG + 1;
}

void Label::setText(string&& str) {
	text = std::move(str);
	updateTextTex();
	updateTextInstance();
}

void Label::setText(const string& str) {
	text = str;
	updateTextTex();
	updateTextInstance();
}

ivec2 Label::textOfs(int resx, Alignment align, ivec2 pos, int sizx, int margin) {
	switch (align) {
	case Alignment::left:
		return ivec2(pos.x + margin, pos.y);
	case Alignment::center:
		return ivec2(pos.x + (sizx - resx) / 2, pos.y);
	case Alignment::right:
		return ivec2(pos.x + sizx - resx - margin, pos.y);
	}
	return pos;
}

inline Rect Label::textRect() const {
	ivec2 siz = size();
	return Rect(textOfs(textTex.rct.size().x, halign, position(), siz.x, siz.y / textMarginFactor), textTex.rct.size());
}

void Label::updateTextTex() {
	ivec2 siz = size();
	SDL_Surface* txt = World::fonts()->render(text.c_str(), siz.y);
	World::scene()->getTexCol()->replace(textTex, txt, restrictTextTexSize(txt, siz));
	SDL_FreeSurface(txt);
}

int Label::precalcWidth(const Widget* self) {
	return txtLen(static_cast<const Label*>(self)->text, self->size().y);	// at this point the height should already be calculated	// TODO: using size() may be stupid
}

int Label::txtLen(const char* str, int height) {
	return World::fonts()->length(str, height) + height / textMarginFactor * 2;
}

int Label::txtLen(const char* str, float hfac) {
	int height = int(std::ceil(hfac * float(World::window()->getScreenView().y)));	// this function is more of an estimate for a parent's size, so let's just ceil this bitch
	return World::fonts()->length(str, height) + height / textMarginFactor * 2;
}

// TEXT BOX

TextBox::TextBox(const Size& size, const Size& lineH, string lines, BCall leftCall, BCall rightCall, string&& tooltip, float dim, uint16 lineL, bool stickTop, bool bg, const TexLoc& tex, const vec4& clr) :
	Label(size, std::move(lines), leftCall, rightCall, std::move(tooltip), dim, Alignment::left, bg, tex, clr),
	alignTop(stickTop),
	lineSize(lineH),
	lineLim(lineL)
{
	cutLines();
}

TextBox::~TextBox() {
	SDL_FreeSurface(textImg);
}

void TextBox::tick(float dSec) {
	if (scroll.tick(dSec, textTex.rct.size(), size())) {
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
		updateScrollBarInstances();
	}
}

void TextBox::onResize() {
	updateTextTex();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(ivec2(textImg->w, textImg->h), size()));
	setInstances();
}

void TextBox::postInit() {
	ivec2 siz = size();
	int lineHeight = sizeToPixRel(lineSize);
	int margin = lineHeight / textMarginFactor;
	int width = siz.x - margin * 2 - *World::pgui()->getSize(GuiGen::SizeRef::scrollBarWidth);
	textImg = World::fonts()->render(text.c_str(), lineHeight, width);
	alignVerticalScroll();
	textTex = World::scene()->getTexCol()->insert(textImg, ivec2(width, siz.y - margin * 2), scroll.listPos);
	setInstances();
}

bool TextBox::setInstances() {
	bool update = Label::setInstances();
	if (update)
		setScrollBarInstances();
	return update;
}

void TextBox::setScrollBarInstances() {
	Rect rbg = rect();
	scroll.setInstances(parent, instIndex + showBG + 1, rbg, ivec2(textImg->w, textImg->h), rbg.pos(), rbg.size(), true);
}

void TextBox::updateScrollBarInstances() {
	if (parent->instanceVisible(instIndex)) {
		setScrollBarInstances();
		parent->updateInstance(instIndex + showBG + 1);
	}
}

uint TextBox::numInstances() const {
	return showBG + 1 + ScrollBar::numInstances;
}

void TextBox::onHold(ivec2 mPos, uint8 mBut) {
	if (scroll.hold(mPos, mBut, this, ivec2(textImg->w, textImg->h), position(), size(), true)) {
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
		updateScrollBarInstances();
	}
}

void TextBox::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, ivec2(textImg->w, textImg->h), position(), size(), true);
	World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
	updateScrollBarInstances();
}

void TextBox::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, true);
}

void TextBox::onScroll(ivec2 wMov) {
	scroll.scroll(wMov, ivec2(textImg->w, textImg->h), size(), true);
	World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
	updateScrollBarInstances();
}

void TextBox::onCancelCapture() {
	scroll.cancelDrag();
}

bool TextBox::selectable() const {
	return true;
}

Rect TextBox::textRect() const {
	return Rect(textOfs(textTex.rct.w, halign, position(), size().x, sizeToPixRel(lineSize) / textMarginFactor), textTex.rct.size());
}

void TextBox::setText(string&& str) {
	text = std::move(str);
	resetListPos();
}

void TextBox::setText(const string& str) {
	text = str;
	resetListPos();
}

void TextBox::addLine(const string& line) {
	if (lineCnt == lineLim) {
		sizet n = text.find('\n');
		text.erase(0, n < text.length() ? n + 1 : n);
	} else
		++lineCnt;
	if (!text.empty())
		text += '\n';
	text += line;
	updateTextTex();
	if (alignVerticalScroll())
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
	updateTextInstance();
	updateScrollBarInstances();
}

string TextBox::moveText() {
	string str = std::move(text);
	text.clear();
	resetListPos();
	return str;
}

void TextBox::updateTextTex() {
	int lineHeight = sizeToPixRel(lineSize);
	SDL_FreeSurface(textImg);
	textImg = World::fonts()->render(text.c_str(), lineHeight, size().x - lineHeight / textMarginFactor * 2 - *World::pgui()->getSize(GuiGen::SizeRef::scrollBarWidth));
	World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), scroll.listPos);
}

bool TextBox::alignVerticalScroll() {
	if (ivec2 siz = size(); !alignTop && siz.x && siz.y) {
		scroll.listPos = ScrollBar::listLim(ivec2(textImg->w, textImg->h), siz);
		return true;
	}
	return false;
}

void TextBox::resetListPos() {
	cutLines();
	updateTextTex();
	scroll.listPos = alignTop ? ivec2(0) : scroll.listLim(ivec2(textImg->w, textImg->h), size());
	updateTextInstance();
	updateScrollBarInstances();
}

void TextBox::cutLines() {
	if (lineCnt = std::count(text.begin(), text.end(), '\n'); lineCnt > lineLim)
		for (sizet i = 0; i < text.length(); ++i)
			if (text[i] == '\n' && --lineCnt <= lineLim) {
				text.erase(0, i + 1);
				break;
			}
}

// ICON

Icon::Icon(const Size& size, string line, BCall leftCall, BCall rightCall, BCall holdCall, string&& tooltip, float dim, Alignment align, bool bg, const TexLoc& tex, const vec4& clr, bool select) :
	Label(size, std::move(line), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	selected(select),
	hcall(holdCall)
{}

bool Icon::setInstances() {
	bool update = Label::setInstances();
	if (update)
		setSelectionInstances();
	return update;
}

void Icon::setSelectionInstances() {
	uint qid = instIndex + showBG + 1;
	if (selected) {
		Rect rbg = rect();
		Rect frm = frame();
		int olSize = std::min(rbg.w, rbg.h) / outlineFactor;
		vec4 clr = dimColor(colorLight);
		parent->setInstance(qid, Rect(rbg.pos(), ivec2(rbg.w, olSize)), frm, clr, TextureCol::blank);
		parent->setInstance(qid + 1, Rect(rbg.x, rbg.y + rbg.h - olSize, rbg.w, olSize), frm, clr, TextureCol::blank);
		parent->setInstance(qid + 2, Rect(rbg.x, rbg.y + olSize, olSize, rbg.h - olSize * 2), frm, clr, TextureCol::blank);
		parent->setInstance(qid + 3, Rect(rbg.x + rbg.w - olSize, rbg.y + olSize, olSize, rbg.h - olSize * 2), frm, clr, TextureCol::blank);
	} else
		for (uint i = 0; i < 4; ++i)
			parent->setInstance(qid + i);
}

uint Icon::numInstances() const {
	return showBG + 5;
}

void Icon::onHold(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		World::prun(hcall, this);
}

bool Icon::selectable() const {
	return lcall || rcall || hcall || !tipTex.blank();
}

void Icon::setSelected(bool on) {
	selected = on;
	if (parent->instanceVisible(instIndex)) {
		setSelectionInstances();
		parent->updateInstance(instIndex + showBG + 1);
	}
}

// COMBO BOX

ComboBox::ComboBox(const Size& size, string curOpt, vector<string> opts, CCall optCall, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, const TexLoc& tex, const vec4& clr) :
	Label(size, std::move(curOpt), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	options(std::move(opts)),
	ocall(optCall)
{}

void ComboBox::onClick(ivec2 mPos, uint8 mBut) {
	if (Button::onClick(mPos, mBut); mBut == SDL_BUTTON_LEFT && ocall)
		World::scene()->setContext(std::make_unique<Context>(mPos, options, ocall, position(), *World::pgui()->getSize(GuiGen::SizeRef::lineHeight), this, size().x));
}

bool ComboBox::selectable() const {
	return true;
}

void ComboBox::setText(string&& str) {
	setText(str);
}

void ComboBox::setText(const string& str) {
	setCurOpt(std::find(options.begin(), options.end(), str) - options.begin());
}

void ComboBox::set(vector<string>&& opts, sizet id) {
	options = std::move(opts);
	setCurOpt(id);
}

void ComboBox::set(vector<string>&& opts, const string& name) {
	options = std::move(opts);
	setText(name);
}

// LABEL EDIT

LabelEdit::LabelEdit(const Size& size, string line, BCall leftCall, BCall rightCall, BCall retCall, BCall cancCall, string&& tooltip, float dim, uint16 lim, bool isChat, Label::Alignment align, bool bg, const TexLoc& tex, const vec4& clr) :
	Label(size, std::move(line), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	oldText(text),
	ecall(retCall),
	ccall(cancCall),
	limit(lim),
	chatMode(isChat)
{
	cutText();
}

LabelEdit::~LabelEdit() {
	SDL_FreeSurface(textImg);
}

void LabelEdit::postInit() {
	ivec2 siz = size();
	textImg = World::fonts()->render(text.c_str(), siz.y);
	textTex = World::scene()->getTexCol()->insert(textImg, restrictTextTexSize(textImg, siz), ivec2(textOfs, 0));
	setInstances();
}

uint LabelEdit::setTopInstances(Quad& quad, uint qid) {
	ivec2 ps = position(), sz = size();
	int margin = sz.y / textMarginFactor;
	quad.setInstance(qid++, Rect(caretPos() + ps.x + margin, ps.y + margin / 2, *World::pgui()->getSize(GuiGen::SizeRef::caretWidth), sz.y - margin), frame(), colorLight, TextureCol::blank);
	return qid;
}

void LabelEdit::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || ecall)) {
		moveCPos(text.length());
		Rect field = rect();
		World::window()->setTextCapture(&field);
		World::scene()->setCapture(this);
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void LabelEdit::onKeyDown(const SDL_KeyboardEvent& key) {
	World::input()->callBindings(key, [this, key](Binding::Type id) { onInput(id, key.keysym.mod, false); });
}

void LabelEdit::onJButtonDown(uint8 but) {
	World::input()->callBindings(JoystickButton(but), [this](Binding::Type id) { onInput(id); });
}

void LabelEdit::onJHatDown(uint8 hat, uint8 val) {
	World::input()->callBindings(AsgJoystick(hat, val), [this](Binding::Type id) { onInput(id); });
}

void LabelEdit::onJAxisDown(uint8 axis, bool positive) {
	World::input()->callBindings(AsgJoystick(JoystickAxis(axis), positive), [this](Binding::Type id) { onInput(id); });
}

void LabelEdit::onGButtonDown(SDL_GameControllerButton but) {
	World::input()->callBindings(but, [this](Binding::Type id) { onInput(id); });
}

void LabelEdit::onGAxisDown(SDL_GameControllerAxis axis, bool positive) {
	World::input()->callBindings(AsgGamepad(axis, positive), [this](Binding::Type id) { onInput(id); });
}

void LabelEdit::onInput(Binding::Type bind, uint16 mod, bool joypad) {
	switch (bind) {
	case Binding::Type::up:
		moveCPos(0);
		break;
	case Binding::Type::down:
		moveCPos(text.length());
		break;
	case Binding::Type::left:
		if (kmodAlt(mod))	// if holding alt skip word
			moveCPos(findWordStart());
		else if (kmodCtrl(mod))	// if holding ctrl move to beginning
			moveCPos(0);
		else if (cpos)	// otherwise go left by one
			moveCPos(jumpCharB(cpos));
		break;
	case Binding::Type::right:
		if (kmodAlt(mod))	// if holding alt skip word
			moveCPos(findWordEnd());
		else if (kmodCtrl(mod))	// if holding ctrl go to end
			moveCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			moveCPos(jumpCharF(cpos));
		break;
	case Binding::Type::textBackspace:
		if (kmodAlt(mod)) {	// if holding alt delete left word
			uint16 id = findWordStart();
			text.erase(id, cpos - id);
			moveCPosFullUpdate(id);
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			moveCPosFullUpdate(0);
		} else if (cpos) {	// otherwise delete left character
			uint16 id = jumpCharB(cpos);
			text.erase(id, cpos - id);
			moveCPosFullUpdate(id);
		}
		break;
	case Binding::Type::textDelete:
		if (kmodAlt(mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTex();
			updateTextInstance();
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTex();
			updateTextInstance();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, jumpCharF(cpos) - cpos);
			updateTextTex();
			updateTextInstance();
		}
		break;
	case Binding::Type::textHome:
		moveCPos(0);
		break;
	case Binding::Type::textEnd:
		moveCPos(text.length());
		break;
	case Binding::Type::textPaste:
		if (kmodCtrl(mod) || joypad)
			if (char* str = SDL_GetClipboardText()) {
				sizet garbagio = 0;
				onText(str, garbagio);
				SDL_free(str);
			}
		break;
	case Binding::Type::textCopy:
		if (kmodCtrl(mod) || joypad)
			SDL_SetClipboardText(text.c_str());
		break;
	case Binding::Type::textCut:
		if (kmodCtrl(mod) || joypad) {
			SDL_SetClipboardText(text.c_str());
			text.clear();
			onTextReset();
		}
		break;
	case Binding::Type::textRevert:
		if (kmodCtrl(mod) || joypad) {
			text = oldText;
			onTextReset();
		}
		break;
	case Binding::Type::confirm:
		confirm();
		World::prun(ecall, this);
		break;
	case Binding::Type::chat:
		if (!chatMode)
			break;
	case Binding::Type::cancel:
		cancel();
		World::prun(ccall, this);
	}
}

void LabelEdit::onCompose(const char* str, sizet& len) {
	text.erase(cpos, len);
	len = strlen(str);
	text.insert(cpos, str, len);
	updateTextTex();
	updateTextInstance();
}

void LabelEdit::onText(const char* str, sizet& len) {
	text.erase(cpos, len);
	sizet slen = strlen(str);
	if (sizet avail = limit - text.length(); slen > avail)
		slen = cutLength(str, avail);
	text.insert(cpos, str, slen);
	moveCPosFullUpdate(cpos + slen);
	len = 0;
}

void LabelEdit::onCancelCapture() {
	textOfs = 0;
	if (chatMode)
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), ivec2(textOfs, 0));
	else {
		text = oldText;
		updateTextTex();
		updateTextInstance();
	}
	World::window()->setTextCapture(nullptr);
}

void LabelEdit::setText(string&& str) {
	oldText = str;
	text = std::move(str);
	cutText();
	onTextReset();
}

void LabelEdit::setText(const string& str) {
	oldText = str;
	text = str.length() <= limit ? str : str.substr(0, cutLength(str.c_str(), limit));
	onTextReset();
}

void LabelEdit::onTextReset() {
	cpos = text.length();
	textOfs = 0;
	updateTextTex();
	updateTextInstance();
}

sizet LabelEdit::cutLength(const char* str, sizet lim) {
	sizet i = lim - 1;
	if (str[i] >= 0)
		return lim;

	for (; i && str[i-1] < 0; --i);	// cut possible u8 char
	sizet end = i + u8clen(str[i]);
	return end <= lim ? end : i;
}

void LabelEdit::cutText() {
	if (text.length() > limit)
		text.erase(cutLength(text.c_str(), limit));
}

void LabelEdit::moveCPos(uint16 cp) {
	if (setCPos(cp))
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), ivec2(textOfs, 0));
}

void LabelEdit::moveCPosFullUpdate(uint16 cp) {
	setCPos(cp);
	updateTextTex();
	updateTextInstance();
}

bool LabelEdit::setCPos(uint16 cp) {
	cpos = cp;
	int cl = caretPos();
	if (cl < 0) {
		textOfs += cl;
		return true;
	}
	ivec2 siz = size();
	if (int ce = cl + *World::pgui()->getSize(GuiGen::SizeRef::caretWidth), sx = siz.x - siz.y / textMarginFactor * 2; ce > sx) {
		textOfs += ce - sx;
		return true;
	}
	return false;
}

void LabelEdit::updateTextTex() {
	ivec2 siz = size();
	SDL_FreeSurface(textImg);
	textImg = World::fonts()->render(text.c_str(), siz.y);
	World::scene()->getTexCol()->replace(textTex, textImg, restrictTextTexSize(textImg, siz), ivec2(textOfs, 0));
}

int LabelEdit::caretPos() {
	char tmp = text[cpos];
	text[cpos] = '\0';
	int sublen = World::fonts()->length(text.c_str(), size().y);
	text[cpos] = tmp;
	return sublen - textOfs;
}

void LabelEdit::confirm() {
	oldText = text;
	if (chatMode) {
		text.clear();
		onTextReset();
	} else {
		textOfs = 0;
		World::scene()->getTexCol()->replace(textTex, textImg, textTex.rct.size(), ivec2(textOfs, 0));

		World::window()->setTextCapture(nullptr);
		World::scene()->setCapture(nullptr, false);
	}
	World::prun(lcall, this);
}

void LabelEdit::cancel() {
	World::scene()->setCapture(nullptr);
}

uint16 LabelEdit::jumpCharB(uint16 i) {
	while (--i && (text[i] & 0xC0) == 0x80);
	return i;
}

uint16 LabelEdit::jumpCharF(uint16 i) {
	while (++i < text.length() && (text[i] & 0xC0) == 0x80);
	return i;
}

uint16 LabelEdit::findWordStart() {
	uint16 i = cpos;
	if (i == text.length() && i)
		--i;
	else if (notSpace(text[i]) && i)
		if (uint16 n = jumpCharB(i); isSpace(text[n]))	// skip if first letter of word
			i = n;
	for (; isSpace(text[i]) && i; --i);		// skip first spaces
	for (; notSpace(text[i]) && i; --i);	// skip word
	return i ? i + 1 : i;			// correct position if necessary
}

uint16 LabelEdit::findWordEnd() {
	uint16 i = cpos;
	for (; isSpace(text[i]) && i < text.length(); ++i);		// skip first spaces
	for (; notSpace(text[i]) && i < text.length(); ++i);	// skip word
	return i;
}

bool LabelEdit::selectable() const {
	return true;
}

// KEY GETTER

KeyGetter::KeyGetter(const Size& size, Binding::Accept atype, Binding::Type binding, sizet keyId, BCall exitCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, const TexLoc& tex, const vec4& clr) :
	Label(size, bindingText(atype, binding, keyId), exitCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	accept(atype),
	bind(binding),
	kid(keyId)
{}

void KeyGetter::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->setCapture(this);
		setText("...");
	} else if (mBut == SDL_BUTTON_RIGHT) {
		World::scene()->setCapture(nullptr);
		World::prun(rcall, this);
	}
}

void KeyGetter::onKeyDown(const SDL_KeyboardEvent& key) {
	setBinding(Binding::Accept::keyboard, key.keysym.scancode);
}

void KeyGetter::onJButtonDown(uint8 but) {
	setBinding(Binding::Accept::joystick, JoystickButton(but));
}

void KeyGetter::onJHatDown(uint8 hat, uint8 val) {
	if (val != SDL_HAT_UP && val != SDL_HAT_RIGHT && val != SDL_HAT_DOWN && val != SDL_HAT_LEFT) {
		if (val & SDL_HAT_RIGHT)
			val = SDL_HAT_RIGHT;
		else if (val & SDL_HAT_LEFT)
			val = SDL_HAT_LEFT;
	}
	setBinding(Binding::Accept::joystick, AsgJoystick(hat, val));
}

void KeyGetter::onJAxisDown(uint8 axis, bool positive) {
	setBinding(Binding::Accept::joystick, AsgJoystick(JoystickAxis(axis), positive));
}

void KeyGetter::onGButtonDown(SDL_GameControllerButton but) {
	setBinding(Binding::Accept::gamepad, but);
}

void KeyGetter::onGAxisDown(SDL_GameControllerAxis axis, bool positive) {
	setBinding(Binding::Accept::gamepad, AsgGamepad(axis, positive));
}

void KeyGetter::onCancelCapture() {
	setText(bindingText(accept, bind, kid));
}

bool KeyGetter::selectable() const {
	return true;
}

template <class T>
void KeyGetter::setBinding(Binding::Accept expect, T key) {
	if (accept != expect && accept != Binding::Accept::any)
		return;

	if (kid != SIZE_MAX) {	// edit an existing binding from the list
		World::input()->setBinding(bind, kid, key);
		World::scene()->setCapture(nullptr);
		World::prun(lcall, this);
	} else if (kid = World::input()->addBinding(bind, key); kid != SIZE_MAX) {	// accept should be any and it shouldn't be in the list yet
		accept = expect;
		World::scene()->setCapture(nullptr, false);
		World::prun(lcall, this);
	}
}

string KeyGetter::bindingText(Binding::Accept accept, Binding::Type bind, sizet kid) {
	switch (accept) {
	case Binding::Accept::keyboard:
		return SDL_GetScancodeName(World::input()->getBinding(bind).keys[kid]);
	case Binding::Accept::joystick:
		switch (AsgJoystick aj = World::input()->getBinding(bind).joys[kid]; aj.getAsg()) {
		case AsgJoystick::button:
			return "Button " + toStr(aj.getJct());
		case AsgJoystick::hat:
			return "Hat " + toStr(aj.getJct()) + ' ' + InputSys::hatNames.at(aj.getHvl());
		case AsgJoystick::axisPos:
			return "Axis +" + toStr(aj.getJct());
		case AsgJoystick::axisNeg:
			return "Axis -" + toStr(aj.getJct());
		}
		break;
	case Binding::Accept::gamepad:
		switch (AsgGamepad ag = World::input()->getBinding(bind).gpds[kid]; ag.getAsg()) {
		case AsgGamepad::button:
			return InputSys::gbuttonNames[ag.getButton()];
		case AsgGamepad::axisPos:
			return '+' + string(InputSys::gaxisNames[ag.getAxis()]);
		case AsgGamepad::axisNeg:
			return '-' + string(InputSys::gaxisNames[ag.getAxis()]);
		}
	}
	return string();
}
