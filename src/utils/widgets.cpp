#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/shaders.h"
#include "engine/world.h"
#include "prog/progs.h"

// SCROLL BAR

void ScrollBar::draw(ivec2 listSize, ivec2 pos, ivec2 size, bool vert, float z) const {
	GLuint tex = World::scene()->texture();
	Quad::draw(World::sgui(), barRect(listSize, pos, size, vert), Widget::colorDark, tex, z);		// bar
	Quad::draw(World::sgui(), sliderRect(listSize, pos, size, vert), Widget::colorLight, tex, z);	// slider
}

void ScrollBar::draw(const Rect& frame, ivec2 listSize, ivec2 pos, ivec2 size, bool vert, float z) const {
	GLuint tex = World::scene()->texture();
	Quad::draw(World::sgui(), barRect(listSize, pos, size, vert), frame, Widget::colorDark, tex, z);		// bar
	Quad::draw(World::sgui(), sliderRect(listSize, pos, size, vert), frame, Widget::colorLight, tex, z);	// slider
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

void ScrollBar::hold(ivec2 mPos, uint8 mBut, Interactable* wgt, ivec2 listSize, ivec2 pos, ivec2 size, bool vert) {
	motion = vec2(0.f);	// get rid of scroll motion
	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->setCapture(wgt);
		if ((draggingSlider = barRect(listSize, pos, size, vert).contain(mPos))) {
			if (int sp = sliderPos(listSize, pos, size, vert), ss = sliderSize(listSize, size, vert); outRange(mPos[vert], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[vert] - ss / 2, listSize, pos, size, vert);
			diffSliderMouse = mPos.y - sliderPos(listSize, pos, size, vert);	// get difference between mouse y and slider y
		}
		SDL_CaptureMouse(SDL_TRUE);
	}
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

void Widget::setParent(Layout* pnt, sizet id) {
	parent = pnt;
	index = id;
}

// BUTTON

Button::Button(const Size& size, BCall leftCall, BCall rightCall, string&& tooltip, float dim, GLuint tex, const vec4& clr) :
	Widget(size),
	lcall(leftCall),
	rcall(rightCall),
	color(clr),
	dimFactor(dim, dim, dim, 1.f),
	tipStr(std::move(tooltip)),
	bgTex(tex ? tex : World::scene()->texture())
{
	updateTipTex();
}

Button::~Button() {
	tipTex.close();
}

void Button::draw() const {
	Quad::draw(World::sgui(), rect(), frame(), color * dimFactor, bgTex);
}

void Button::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(lcall, this);
	else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Button::onHover() {
	color *= selectFactor;
}

void Button::onUnhover() {
	color /= selectFactor;
}

void Button::onNavSelect(Direction dir) {
	parent->navSelectNext(index, dir.vertical() ? center().x : center().y, dir);
}

bool Button::selectable() const {
	return lcall || rcall || tipTex;
}

void Button::drawTooltip() const {
	if (!tipTex)
		return;

	int ofs = World::window()->getCursorHeight() + cursorMargin;
	ivec2 pos = World::window()->mousePos() + ivec2(0, ofs);
	ivec2 siz = tipTex.getRes() + tooltipMargin * 2;
	ivec2 res = World::window()->getScreenView();
	if (pos.x + siz.x > res.x)
		pos.x = res.x - siz.x;
	if (pos.y + siz.y > res.y)
		pos.y = pos.y - ofs - siz.y;

	Quad::draw(World::sgui(), Rect(pos, siz), colorTooltip, World::scene()->texture(), -1.f);
	Quad::draw(World::sgui(), Rect(pos + tooltipMargin, tipTex.getRes()), vec4(1.f), tipTex, -1.f);
}

void Button::setTooltip(string&& tooltip) {
	tipStr = std::move(tooltip);
	tipTex.close();
	updateTipTex();
}

void Button::updateTipTex() {
	if (!World::sets()->tooltips) {
		tipTex.close();
		return;
	}

	float resy = float(World::window()->getScreenView().y);
	int theight = int(tooltipHeightFactor * resy);
	uint tlimit = uint(tooltipLimitFactor * resy);
	if (tipStr.find('\n') >= tipStr.length())
		tipTex = World::fonts()->render(tipStr.c_str(), theight, tlimit);
	else {
		uint width = 0;
		for (char* pos = tipStr.data(); *pos;) {
			sizet len = strcspn(pos, "\n");
			if (uint siz = World::fonts()->length(pos, theight, len); siz > width)
				if (width = std::min(siz, tlimit); width == tlimit)
					break;
			pos += pos[len] ? len + 1 : len;
		}
		tipTex = World::fonts()->render(tipStr.c_str(), theight, width);
	}
}

// CHECK BOX

CheckBox::CheckBox(const Size& size, bool checked, BCall leftCall, BCall rightCall, string&& tooltip, float dim, GLuint tex, const vec4& clr) :
	Button(size, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	on(checked)
{}

void CheckBox::draw() const {
	Rect frm = frame();
	Quad::draw(World::sgui(), rect(), frm, color * dimFactor, bgTex);								// draw background
	Quad::draw(World::sgui(), boxRect(), frm, boxColor() * dimFactor, World::scene()->texture());	// draw checkbox
}

void CheckBox::onClick(ivec2 mPos, uint8 mBut) {
	if ((mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT) && (lcall || rcall))
		toggle();
	Button::onClick(mPos, mBut);
}

Rect CheckBox::boxRect() const {
	ivec2 siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(const Size& size, int val, int minimum, int maximum, int navStep, BCall finishCall, BCall updateCall, string&& tooltip, float dim, GLuint tex, const vec4& clr) :
	Button(size, finishCall, updateCall, std::move(tooltip), dim, tex, clr),
	value(val),
	vmin(minimum),
	vmax(maximum),
	keyStep(navStep),
	oldVal(val)
{}

void Slider::draw() const {
	Rect frm = frame();
	GLuint tex = World::scene()->texture();
	Quad::draw(World::sgui(), rect(), frm, color * dimFactor, bgTex);			// background
	Quad::draw(World::sgui(), barRect(), frm, colorDark * dimFactor, tex);		// bar
	Quad::draw(World::sgui(), sliderRect(), frm, colorLight * dimFactor, tex);	// slider
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
		value = vmin;
		World::prun(rcall, this);
		break;
	case Binding::Type::down: case Binding::Type::textEnd:
		value = vmax;
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
	value = oldVal;
	SDL_CaptureMouse(SDL_FALSE);
	World::prun(rcall, this);
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

Label::Label(const Size& size, string line, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Button(size, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	text(std::move(line)),
	halign(align),
	showBG(bg)
{}

Label::Label(string line, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Button(precalcWidth, leftCall, rightCall, std::move(tooltip), dim, tex, clr),
	text(std::move(line)),
	halign(align),
	showBG(bg)
{}

Label::~Label() {
	textTex.close();
}

void Label::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		Quad::draw(World::sgui(), rbg, frm, color * dimFactor, bgTex);
	if (textTex)
		Quad::draw(World::sgui(), textRect(), textFrame(rbg, frm), dimFactor, textTex);
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(string&& str) {
	text = std::move(str);
	updateTextTex();
}

void Label::setText(const string& str) {
	text = str;
	updateTextTex();
}

Rect Label::textFrame(const Rect& rbg, const Rect& frm) {
	int margin = rbg.h / textMarginFactor;
	return Rect(rbg.x + margin, rbg.y, rbg.w - margin * 2, rbg.h).intersect(frm);
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

ivec2 Label::textPos() const {
	ivec2 siz = size();
	return textOfs(textTex.getRes().x, halign, position(), siz.x, siz.y / textMarginFactor);
}

void Label::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text.c_str(), size().y);
}

int Label::precalcWidth(const Widget* self) {
	return txtLen(static_cast<const Label*>(self)->text, self->size().y);	// at this point the height should already be calculated
}

int Label::txtLen(const char* str, int height) {
	return World::fonts()->length(str, height) + height / textMarginFactor * 2;
}

int Label::txtLen(const char* str, float hfac) {
	int height = int(std::ceil(hfac * float(World::window()->getScreenView().y)));	// this function is more of an estimate for a parent's size, so let's just ceil this bitch
	return World::fonts()->length(str, height) + height / textMarginFactor * 2;
}

// TEXT BOX

TextBox::TextBox(const Size& size, float lineH, string lines, BCall leftCall, BCall rightCall, string&& tooltip, float dim, uint16 lineL, bool stickTop, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(lines), leftCall, rightCall, std::move(tooltip), dim, Alignment::left, bg, tex, clr),
	alignTop(stickTop),
	lineHeightFactor(lineH),
	lineLim(lineL)
{
	cutLines();
}

TextBox::~TextBox() {
	SDL_FreeSurface(textImg);
}

void TextBox::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		Quad::draw(World::sgui(), rbg, frm, color * dimFactor, bgTex);
	if (textTex)
		Quad::draw(World::sgui(), textRect(), textFrame(rbg, frm), dimFactor, textTex);
	scroll.draw(rbg, textTex.getRes(), rbg.pos(), rbg.size(), true);
}

void TextBox::tick(float dSec) {
	if (scroll.tick(dSec, textTex.getRes(), size()))
		textTex.update(textImg, scroll.listPos);
}

void TextBox::onResize() {
	Label::onResize();
	scroll.listPos = glm::clamp(scroll.listPos, ivec2(0), ScrollBar::listLim(textTex.getRes(), size()));
}

void TextBox::postInit() {
	Label::postInit();
	updateListPos();
}

void TextBox::onHold(ivec2 mPos, uint8 mBut) {
	scroll.hold(mPos, mBut, this, textTex.getRes(), position(), size(), true);
}

void TextBox::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, textTex.getRes(), position(), size(), true);
	textTex.update(textImg, scroll.listPos);
}

void TextBox::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, true);
}

void TextBox::onScroll(ivec2 wMov) {
	scroll.scroll(wMov, textTex.getRes(), size(), true);
	textTex.update(textImg, scroll.listPos);
}

void TextBox::onCancelCapture() {
	scroll.cancelDrag();
}

bool TextBox::selectable() const {
	return true;
}

ivec2 TextBox::textPos() const {
	return textOfs(textTex.getRes().x, halign, position(), size().x, pixLineHeight() / textMarginFactor) - scroll.listPos;
}

void TextBox::setText(string&& str) {
	text = std::move(str);
	cutLines();
	updateTextTex();
	resetListPos();
}

void TextBox::setText(const string& str) {
	text = str;
	cutLines();
	updateTextTex();
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
	updateListPos();
}

string TextBox::moveText() {
	string str = std::move(text);
	updateTextTex();
	return str;
}

void TextBox::updateTextTex() {
	textTex.close();
	SDL_FreeSurface(textImg);
	int lineHeight = pixLineHeight();
	textTex = World::fonts()->render(text.c_str(), lineHeight, size().x - lineHeight / textMarginFactor * 2 - ScrollBar::width, textImg, scroll.listPos);
}

void TextBox::updateListPos() {
	if (ivec2 siz = size(); !alignTop && siz.x && siz.y) {
		scroll.listPos = ScrollBar::listLim(textTex.getRes(), siz);
		textTex.update(textImg, scroll.listPos);
	}
}

void TextBox::cutLines() {
	if (lineCnt = std::count(text.begin(), text.end(), '\n'); lineCnt > lineLim)
		for (sizet i = 0; i < text.length(); ++i)
			if (text[i] == '\n' && --lineCnt <= lineLim) {
				text.erase(0, i + 1);
				break;
			}
}

int TextBox::pixLineHeight() const {
	return int(lineHeightFactor * float(World::window()->getScreenView().y));
}

// ICON

Icon::Icon(const Size& size, string line, BCall leftCall, BCall rightCall, BCall holdCall, string&& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr, bool select) :
	Label(size, std::move(line), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	selected(select),
	hcall(holdCall)
{}

void Icon::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		Quad::draw(World::sgui(), rbg, frm, color * dimFactor, bgTex);
	if (textTex)
		Quad::draw(World::sgui(), textRect(), textFrame(rbg, frm), dimFactor, textTex);

	if (selected) {
		int olSize = std::min(rbg.w, rbg.h) / outlineFactor;
		vec4 clr = colorLight * dimFactor;
		GLuint tex = World::scene()->texture();
		Quad::draw(World::sgui(), Rect(rbg.pos(), ivec2(rbg.w, olSize)), frm, clr, tex);
		Quad::draw(World::sgui(), Rect(rbg.x, rbg.y + rbg.h - olSize, rbg.w, olSize), frm, clr, tex);
		Quad::draw(World::sgui(), Rect(rbg.x, rbg.y + olSize, olSize, rbg.h - olSize * 2), frm, clr, tex);
		Quad::draw(World::sgui(), Rect(rbg.x + rbg.w - olSize, rbg.y + olSize, olSize, rbg.h - olSize * 2), frm, clr, tex);
	}
}

void Icon::onHold(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		World::prun(hcall, this);
}

bool Icon::selectable() const {
	return lcall || rcall || hcall || tipTex;
}

// COMBO BOX

ComboBox::ComboBox(const Size& size, string curOpt, vector<string> opts, CCall optCall, BCall leftCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(curOpt), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	options(std::move(opts)),
	ocall(optCall)
{}

void ComboBox::onClick(ivec2 mPos, uint8 mBut) {
	if (Button::onClick(mPos, mBut); mBut == SDL_BUTTON_LEFT && ocall)
		World::scene()->setContext(std::make_unique<Context>(mPos, options, ocall, position(), World::pgui()->getLineHeight(), this, size().x));
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

LabelEdit::LabelEdit(const Size& size, string line, BCall leftCall, BCall rightCall, BCall retCall, BCall cancCall, string&& tooltip, float dim, uint16 lim, bool isChat, Label::Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(line), leftCall, rightCall, std::move(tooltip), dim, align, bg, tex, clr),
	oldText(text),
	ecall(retCall),
	ccall(cancCall),
	chatMode(isChat),
	limit(lim)
{
	cutText();
}

LabelEdit::~LabelEdit() {
	SDL_FreeSurface(textImg);
}

void LabelEdit::drawTop() {
	ivec2 ps = position(), sz = size();
	int margin = sz.y / textMarginFactor;
	Quad::draw(World::sgui(), Rect(std::min(caretPos(), sz.x - margin * 2 - caretWidth) + ps.x + margin, ps.y + margin / 2, caretWidth, sz.y - margin), frame(), colorLight, World::scene()->texture());
}

void LabelEdit::onClick(ivec2, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || ecall)) {
		setCPos(text.length());
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
		setCPos(0);
		break;
	case Binding::Type::down:
		setCPos(text.length());
		break;
	case Binding::Type::left:
		if (kmodAlt(mod))	// if holding alt skip word
			setCPos(findWordStart());
		else if (kmodCtrl(mod))	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos)	// otherwise go left by one
			setCPos(jumpCharB(cpos));
		break;
	case Binding::Type::right:
		if (kmodAlt(mod))	// if holding alt skip word
			setCPos(findWordEnd());
		else if (kmodCtrl(mod))	// if holding ctrl go to end
			setCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(jumpCharF(cpos));
		break;
	case Binding::Type::textBackspace:
		if (kmodAlt(mod)) {	// if holding alt delete left word
			uint16 id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTextTex();
			setCPos(0);
		} else if (cpos) {	// otherwise delete left character
			uint16 id = jumpCharB(cpos);
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		}
		break;
	case Binding::Type::textDelete:
		if (kmodAlt(mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTex();
		} else if (kmodCtrl(mod)) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTex();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, jumpCharF(cpos) - cpos);
			updateTextTex();
		}
		break;
	case Binding::Type::textHome:
		setCPos(0);
		break;
	case Binding::Type::textEnd:
		setCPos(text.length());
		break;
	case Binding::Type::textPaste:
		if (kmodCtrl(mod) || joypad)
			if (char* str = SDL_GetClipboardText()) {
				uint garbagio = 0;
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

void LabelEdit::onCompose(const char* str, uint& len) {
	text.erase(cpos, len);
	len = strlen(str);
	text.insert(cpos, str, len);
	updateTextTex();
}

void LabelEdit::onText(const char* str, uint& len) {
	text.erase(cpos, len);
	sizet slen = strlen(str);
	if (sizet avail = limit - text.length(); slen > avail)
		slen = cutLength(str, avail);
	text.insert(cpos, str, slen);
	updateTextTex();
	setCPos(cpos + slen);
	len = 0;
}

void LabelEdit::onCancelCapture() {
	viewPos = texiPos = 0;
	if (chatMode)
		textTex.update(textImg, ivec2(texiPos, 0));
	else {
		text = oldText;
		updateTextTex();
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
	viewPos = texiPos = 0;
	updateTextTex();
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

void LabelEdit::setCPos(uint16 cp) {
	cpos = cp;
	ivec2 siz = size();
	if (int cl = caretPos(); shiftText(cl, caretWidth, viewPos, siz.x - siz.y / textMarginFactor * 2, texiPos, textTex.getRes().x, textImg ? textImg->w : 0))
		textTex.update(textImg, ivec2(texiPos, 0));
}

bool LabelEdit::shiftText(int cp, int cs, int& vp, int vs, int& tp, int ts, int is) {
	if (cp + cs > vs) {
		if (vs -= cs; vp + cp <= ts)
			vp += cp - vs;
		else {
			int tn = tp + ts - vs;
			if (tn + ts <= is)
				vp = 0;
			else {
				tn = is - ts;
				vp -= tn - tp;
			}
			tp = tn;
			return true;
		}
	} else if (cp < 0) {
		if (vp >= -cp)
			vp += cp;
		else {
			if (int tb = ts - vs; tp >= tb) {
				tp -= tb;
				vp = tb;
			} else {
				vp += tp;
				tp = 0;
			}
			return true;
		}
	}
	return false;
}

ivec2 LabelEdit::textPos() const {
	return Label::textPos() - ivec2(viewPos, 0);
}

void LabelEdit::updateTextTex() {
	textTex.close();
	SDL_FreeSurface(textImg);
	textTex = World::fonts()->render(text.c_str(), size().y, textImg, ivec2(texiPos, 0));
}

int LabelEdit::caretPos() {
	char tmp = text[cpos];
	text[cpos] = '\0';
	int sublen = World::fonts()->length(text.c_str(), size().y);
	text[cpos] = tmp;
	return sublen - viewPos - texiPos;
}

void LabelEdit::confirm() {
	oldText = text;
	if (chatMode) {
		text.clear();
		onTextReset();
	} else {
		viewPos = texiPos = 0;
		textTex.update(textImg, ivec2(texiPos, 0));

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

KeyGetter::KeyGetter(const Size& size, Binding::Accept atype, Binding::Type binding, sizet keyId, BCall exitCall, BCall rightCall, string&& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
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
