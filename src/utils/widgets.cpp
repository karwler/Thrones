#include "engine/scene.h"
#include "engine/inputSys.h"
#include "engine/world.h"
#include "prog/progs.h"

// SCROLL

void ScrollBar::draw(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert, float z) const {
	GLuint tex = World::scene()->texture();
	Quad::draw(barRect(listSize, pos, size, vert), Widget::colorDark, tex, z);		// bar
	Quad::draw(sliderRect(listSize, pos, size, vert), Widget::colorLight, tex, z);	// slider
}

void ScrollBar::draw(const Rect& frame, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert, float z) const {
	GLuint tex = World::scene()->texture();
	Quad::draw(barRect(listSize, pos, size, vert), frame, Widget::colorDark, tex, z);		// bar
	Quad::draw(sliderRect(listSize, pos, size, vert), frame, Widget::colorLight, tex, z);	// slider
}

void ScrollBar::tick(float dSec, const ivec2& listSize, const ivec2& size) {
	if (motion != vec2(0.f)) {
		moveListPos(motion, listSize, size);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
		World::scene()->updateSelect();
	}
}

void ScrollBar::hold(const ivec2& mPos, uint8 mBut, Interactable* wgt, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) {
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

void ScrollBar::drag(const ivec2& mPos, const ivec2& mMov, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse, listSize, pos, size, vert);
	else
		moveListPos(mMov * swap(0, -1, !vert), listSize, size);
}

void ScrollBar::undrag(uint8 mBut, bool vert) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mousePos()) && !draggingSlider)
			motion = World::input()->getMouseMove() * swap(0, -1, !vert);
		World::scene()->setCapture(nullptr);	// should call cancelDrag through the captured widget
	}
}

void ScrollBar::cancelDrag() {
	SDL_CaptureMouse(SDL_FALSE);
	draggingSlider = false;
}

void ScrollBar::scroll(const ivec2& wMov, const ivec2& listSize, const ivec2& size, bool vert) {
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

void ScrollBar::setSlider(int spos, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) {
	int lim = listLim(listSize, size)[vert];
	listPos[vert] = std::clamp((spos - pos[vert]) * lim / sliderLim(listSize, size, vert), 0, lim);
}

Rect ScrollBar::barRect(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) {
	int bs = barSize(listSize, size, vert);
	return vert ? Rect(pos.x + size.x - bs, pos.y, bs, size.y) : Rect(pos.x, pos.y + size.y - bs, size.x, bs);
}

Rect ScrollBar::sliderRect(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) const {
	int bs = barSize(listSize, size, vert);
	int sp = sliderPos(listSize, pos, size, vert);
	int ss = sliderSize(listSize, size, vert);
	return vert ? Rect(pos.x + size.x - bs, sp, bs, ss) : Rect(sp, pos.y + size.y - bs, ss, bs);
}

// QUAD

Quad::Quad() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(Shader::uvloc);
	glVertexAttribPointer(Shader::uvloc, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(0));
}

Quad::~Quad() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::vpos);
	glDisableVertexAttribArray(Shader::uvloc);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void Quad::draw(const Rect& rect, const vec4& uvrect, const vec4& color, GLuint tex, float z) {
	glUniform4f(World::sgui()->rect, float(rect.x), float(rect.y), float(rect.w), float(rect.h));
	glUniform4fv(World::sgui()->uvrc, 1, glm::value_ptr(uvrect));
	glUniform1f(World::sgui()->zloc, z);
	glUniform4fv(World::sgui()->color, 1, glm::value_ptr(color));
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, Quad::corners);
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

Button::Button(Size size, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, GLuint tex, const vec4& clr) :
	Widget(size),
	lcall(leftCall),
	rcall(rightCall),
	color(clr),
	dimFactor(dim, dim, dim, 1.f),
	tipTex(tooltip),
	bgTex(tex ? tex : World::scene()->texture())
{}

Button::~Button() {
	tipTex.close();
}

void Button::draw() const {
	Quad::draw(rect(), frame(), color * dimFactor, bgTex);
}

void Button::onClick(const ivec2&, uint8 mBut) {
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
	ivec2 pos = mousePos() + ivec2(0, ofs);
	ivec2 siz = tipTex.getRes() + tooltipMargin * 2;
	if (pos.x + siz.x > World::window()->getGuiView().x)
		pos.x = World::window()->getGuiView().x - siz.x;
	if (pos.y + siz.y > World::window()->getGuiView().y)
		pos.y = pos.y - ofs - siz.y;

	Quad::draw(Rect(pos, siz), colorTooltip, World::scene()->texture(), -1.f);
	Quad::draw(Rect(pos + tooltipMargin, tipTex.getRes()), vec4(1.f), tipTex, -1.f);
}

void Button::setTooltip(const Texture& tooltip) {
	tipTex.close();
	tipTex = tooltip;
}

// CHECK BOX

CheckBox::CheckBox(Size size, bool checked, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, GLuint tex, const vec4& clr) :
	Button(size, leftCall, rightCall, tooltip, dim, tex, clr),
	on(checked)
{}

void CheckBox::draw() const {
	Rect frm = frame();
	Quad::draw(rect(), frm, color * dimFactor, bgTex);								// draw background
	Quad::draw(boxRect(), frm, boxColor() * dimFactor, World::scene()->texture());	// draw checkbox
}

void CheckBox::onClick(const ivec2& mPos, uint8 mBut) {
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

Slider::Slider(Size size, int val, int minimum, int maximum, int navStep, BCall finishCall, BCall updateCall, const Texture& tooltip, float dim, GLuint tex, const vec4& clr) :
	Button(size, finishCall, updateCall, tooltip, dim, tex, clr),
	value(val),
	vmin(minimum),
	vmax(maximum),
	keyStep(navStep),
	oldVal(val)
{}

void Slider::draw() const {
	Rect frm = frame();
	Quad::draw(rect(), frm, color * dimFactor, bgTex);						// background
	Quad::draw(barRect(), frm, colorDark, World::scene()->texture());		// bar
	Quad::draw(sliderRect(), frm, colorLight, World::scene()->texture());	// slider
}

void Slider::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || rcall)) {
		World::scene()->setCapture(this);
		if (int sp = sliderPos(), sw = size().y / sliderWidthFactor; outRange(mPos.x, sp, sp + sw))	// if mouse outside of slider
			setSlider(mPos.x - sw / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
		SDL_CaptureMouse(SDL_TRUE);
	}
}

void Slider::onDrag(const ivec2& mPos, const ivec2&) {
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
	int height = siz.y / 2;
	return Rect(position() + siz.y / barMarginFactor, ivec2(siz.x - height, height));
}

Rect Slider::sliderRect() const {
	ivec2 pos = position(), siz = size();
	return Rect(sliderPos(), pos.y, siz.y / sliderWidthFactor, siz.y);
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

Label::Label(Size size, string line, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Button(size, leftCall, rightCall, tooltip, dim, tex, clr),
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
		Quad::draw(rbg, frm, color * dimFactor, bgTex);
	if (textTex) {
		int margin = rbg.h / textMarginFactor;
		Quad::draw(textRect(), Rect(rbg.x + margin, rbg.y, rbg.w - margin * 2, rbg.h).intersect(frm), dimFactor, textTex);
	}
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

ivec2 Label::textOfs(const ivec2& res, Alignment align, const ivec2& pos, const ivec2& siz, int margin) {
	switch (align) {
	case Alignment::left:
		return ivec2(pos.x + margin, pos.y);
	case Alignment::center:
		return ivec2(pos.x + (siz.x - res.x) / 2, pos.y);
	case Alignment::right:
		return ivec2(pos.x + siz.x - res.x - margin, pos.y);
	}
	return pos;
}

ivec2 Label::textPos() const {
	ivec2 siz = size();
	return textOfs(textTex.getRes(), halign, position(), siz, siz.y / textMarginFactor);
}

void Label::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text.c_str(), size().y);
}

// TEXT BOX

TextBox::TextBox(Size size, int lineH, string lines, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, uint16 lineL, bool stickTop, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(lines), leftCall, rightCall, tooltip, dim, Alignment::left, bg, tex, clr),
	alignTop(stickTop),
	lineHeight(lineH),
	lineLim(lineL)
{
	cutLines();
}

void TextBox::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		Quad::draw(rbg, frm, color * dimFactor, bgTex);
	if (textTex)
		Quad::draw(textRect(), rbg, dimFactor, textTex);
	scroll.draw(rbg, textTex.getRes(), rbg.pos(), rbg.size(), true);
}

void TextBox::tick(float dSec) {
	scroll.tick(dSec, textTex.getRes(), size());
}

void TextBox::postInit() {
	Label::postInit();
	updateListPos();
}

void TextBox::onHold(const ivec2& mPos, uint8 mBut) {
	scroll.hold(mPos, mBut, this, textTex.getRes(), position(), size(), true);
}

void TextBox::onDrag(const ivec2& mPos, const ivec2& mMov) {
	scroll.drag(mPos, mMov, textTex.getRes(), position(), size(), true);
}

void TextBox::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, true);
}

void TextBox::onScroll(const ivec2& wMov) {
	scroll.scroll(wMov, textTex.getRes(), size(), true);
}

void TextBox::onCancelCapture() {
	scroll.cancelDrag();
}

bool TextBox::selectable() const {
	return true;
}

ivec2 TextBox::textPos() const {
	return textOfs(textTex.getRes(), halign, position(), size(), lineHeight / textMarginFactor) - scroll.listPos;
}

void TextBox::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text.c_str(), lineHeight, uint(size().x - lineHeight / textMarginFactor * 2 - ScrollBar::width));
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

void TextBox::updateListPos() {
	if (ivec2 siz = size(); !alignTop && siz.x && siz.y)
		scroll.listPos = scroll.listLim(textTex.getRes(), siz);
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

Icon::Icon(Size size, string line, BCall leftCall, BCall rightCall, BCall holdCall, const Texture& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr, bool select) :
	Label(size, std::move(line), leftCall, rightCall, tooltip, dim, align, bg, tex, clr),
	selected(select),
	hcall(holdCall)
{}

void Icon::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		Quad::draw(rbg, frm, color * dimFactor, bgTex);
	if (textTex)
		Quad::draw(textRect(), frm, dimFactor, textTex);

	if (selected) {
		int olSize = std::min(rbg.w, rbg.h) / outlineFactor;
		vec4 clr = colorLight * dimFactor;
		Quad::draw(Rect(rbg.pos(), ivec2(rbg.w, olSize)), clr, World::scene()->texture());
		Quad::draw(Rect(rbg.x, rbg.y + rbg.h - olSize, rbg.w, olSize), clr, World::scene()->texture());
		Quad::draw(Rect(rbg.x, rbg.y + olSize, olSize, rbg.h - olSize * 2), clr, World::scene()->texture());
		Quad::draw(Rect(rbg.x + rbg.w - olSize, rbg.y + olSize, olSize, rbg.h - olSize * 2), clr, World::scene()->texture());
	}
}

void Icon::onHold(const ivec2&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		World::prun(hcall, this);
}

bool Icon::selectable() const {
	return lcall || rcall || hcall || tipTex;
}

// COMBO BOX

ComboBox::ComboBox(Size size, string curOpt, vector<string> opts, CCall optCall, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(curOpt), leftCall, rightCall, tooltip, dim, align, bg, tex, clr),
	options(std::move(opts)),
	ocall(optCall)
{}

void ComboBox::onClick(const ivec2& mPos, uint8 mBut) {
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

LabelEdit::LabelEdit(Size size, string line, BCall leftCall, BCall rightCall, BCall retCall, BCall cancCall, const Texture& tooltip, float dim, uint16 lim, bool isChat, Label::Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(line), leftCall, rightCall, tooltip, dim, align, bg, tex, clr),
	oldText(text),
	ecall(retCall),
	ccall(cancCall),
	chatMode(isChat),
	limit(lim)
{
	cutText();
}

void LabelEdit::drawTop() const {
	ivec2 ps = position(), sz = size();
	Quad::draw(Rect(caretPos() + ps.x + sz.y / textMarginFactor, ps.y, caretWidth, sz.y), frame(), colorLight, World::scene()->texture());
}

void LabelEdit::onClick(const ivec2&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || ecall)) {
		setCPos(uint16(text.length()));
		World::window()->setTextCapture(true);
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
		setCPos(uint16(text.length()));
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
			setCPos(uint16(text.length()));
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
		setCPos(uint16(text.length()));
		break;
	case Binding::Type::textPaste:
		if (kmodCtrl(mod) || joypad)
			if (char* str = SDL_GetClipboardText()) {
				onText(str);
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

void LabelEdit::onText(const char* str) {
	sizet slen = strlen(str);
	if (sizet avail = limit - text.length(); slen > avail)
		slen = cutLength(str, avail);
	text.insert(cpos, str, slen);
	updateTextTex();
	setCPos(cpos + uint16(slen));
}

void LabelEdit::onCancelCapture() {
	if (!chatMode) {
		text = oldText;
		updateTextTex();
	}
	textOfs = 0;
	World::window()->setTextCapture(false);
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
	updateTextTex();
	cpos = uint16(text.length());
	textOfs = 0;
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
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else {
		ivec2 siz = size();
		if (int ce = cl + caretWidth, sx = siz.x - siz.y / textMarginFactor * 2; ce > sx)
			textOfs -= ce - sx;
	}
}

ivec2 LabelEdit::textPos() const {
	return Label::textPos() + ivec2(textOfs, 0);
}

int LabelEdit::caretPos() const {
	return World::fonts()->length(text.substr(0, cpos).c_str(), size().y) + textOfs;
}

void LabelEdit::confirm() {
	oldText = text;
	if (chatMode) {
		text.clear();
		onTextReset();
	} else {
		World::window()->setTextCapture(false);
		World::scene()->setCapture(nullptr, false);
	}
	textOfs = 0;
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

KeyGetter::KeyGetter(Size size, Binding::Accept atype, Binding::Type binding, sizet keyId, BCall exitCall, BCall rightCall, const Texture& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, bindingText(atype, binding, keyId), exitCall, rightCall, tooltip, dim, align, bg, tex, clr),
	accept(atype),
	bind(binding),
	kid(keyId)
{}

void KeyGetter::onClick(const ivec2&, uint8 mBut) {
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
			return InputSys::gbuttonNames[uint8(ag.getButton())];
		case AsgGamepad::axisPos:
			return '+' + string(InputSys::gaxisNames[uint8(ag.getAxis())]);
		case AsgGamepad::axisNeg:
			return '-' + string(InputSys::gaxisNames[uint8(ag.getAxis())]);
		}
	}
	return string();
}
