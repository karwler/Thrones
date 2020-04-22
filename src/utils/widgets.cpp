#include "engine/world.h"

// SCROLL

ScrollBar::ScrollBar() :
	listPos(0),
	motion(0.f),
	diffSliderMouse(0),
	draggingSlider(false)
{}

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
		World::scene()->capture = wgt;
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
		SDL_CaptureMouse(SDL_FALSE);
		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
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
	glUniform4f(World::gui()->rect, float(rect.x), float(rect.y), float(rect.w), float(rect.h));
	glUniform4fv(World::gui()->uvrc, 1, glm::value_ptr(uvrect));
	glUniform1f(World::gui()->zloc, z);
	glUniform4fv(World::gui()->color, 1, glm::value_ptr(color));
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, Quad::corners);
}

// WIDGET

Widget::Widget(Size size) :
	relSize(size),
	parent(nullptr),
	index(SIZE_MAX)
{}

ivec2 Widget::position() const {
	return parent->wgtPosition(index);
}

ivec2 Widget::size() const {
	return parent->wgtSize(index);
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

Slider::Slider(Size size, int value, int minimum, int maximum, int navStep, BCall finishCall, BCall updateCall, const Texture& tooltip, float dim, GLuint tex, const vec4& clr) :
	Button(size, finishCall, updateCall, tooltip, dim, tex, clr),
	val(value),
	min(minimum),
	max(maximum),
	keyStep(navStep),
	diffSliderMouse(0)
{}

void Slider::draw() const {
	Rect frm = frame();
	Quad::draw(rect(), frm, color * dimFactor, bgTex);						// background
	Quad::draw(barRect(), frm, colorDark, World::scene()->texture());		// bar
	Quad::draw(sliderRect(), frm, colorLight, World::scene()->texture());	// slider
}

void Slider::onHold(const ivec2& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || rcall)) {
		World::scene()->capture = this;
		if (int sp = sliderPos(); outRange(mPos.x, sp, sp + barSize))	// if mouse outside of slider
			setSlider(mPos.x - barSize / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(const ivec2& mPos, const ivec2&) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {	// if dragging slider stop dragging slider
		World::scene()->capture = nullptr;
		World::prun(lcall, this);
	}
}

void Slider::onKeypress(const SDL_KeyboardEvent& key) {
	switch (key.keysym.scancode) {
	case SDL_SCANCODE_UP: case SDL_SCANCODE_HOME:
		val = min;
		World::prun(rcall, this);
		break;
	case SDL_SCANCODE_DOWN: case SDL_SCANCODE_END:
		val = max;
		World::prun(rcall, this);
		break;
	case SDL_SCANCODE_LEFT:
		if (val > min) {
			setVal(val - keyStep);
			World::prun(rcall, this);
		}
		break;
	case SDL_SCANCODE_RIGHT:
		if (val < max) {
			setVal(val + keyStep);
			World::prun(rcall, this);
		}
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_TAB: case SDL_SCANCODE_AC_BACK:
		if (!key.repeat)
			onUndrag(SDL_BUTTON_LEFT);
	}
}

void Slider::onJButton(uint8 but) {
	switch (but) {
	case InputSys::jbutA: case InputSys::jbutB:
		onUndrag(SDL_BUTTON_LEFT);
	}
}

void Slider::onJHat(uint8 hat) {
	if (hat & SDL_HAT_LEFT) {
		if (val > min) {
			setVal(val - keyStep);
			World::prun(rcall, this);
		}
	} else if (hat & SDL_HAT_UP) {
		val = min;
		World::prun(rcall, this);
	}

	if (hat & SDL_HAT_RIGHT) {
		if (val < max) {
			setVal(val + keyStep);
			World::prun(rcall, this);
		}
	} else if (hat & SDL_HAT_DOWN) {
		val = max;
		World::prun(rcall, this);
	}
}

void Slider::onGButton(SDL_GameControllerButton but) {
	switch (but) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		val = min;
		World::prun(rcall, this);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		val = max;
		World::prun(rcall, this);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		if (val > min) {
			setVal(val - keyStep);
			World::prun(rcall, this);
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		if (val < max) {
			setVal(val + keyStep);
			World::prun(rcall, this);
		}
		break;
	case SDL_CONTROLLER_BUTTON_A: case SDL_CONTROLLER_BUTTON_B:
		onUndrag(SDL_BUTTON_LEFT);
	}
}

void Slider::set(int value, int minimum, int maximum) {
	min = minimum;
	max = maximum;
	setVal(value);
}

void Slider::setSlider(int xpos) {
	setVal(min + int(std::round(float((xpos - position().x - size().y / 4) * (max - min)) / float(sliderLim()))));
	World::prun(rcall, this);
}

Rect Slider::barRect() const {
	ivec2 siz = size();
	int height = siz.y / 2;
	return Rect(position() + siz.y / 4, ivec2(siz.x - height, height));
}

Rect Slider::sliderRect() const {
	ivec2 pos = position(), siz = size();
	return Rect(sliderPos(), pos.y, barSize, siz.y);
}

int Slider::sliderPos() const {
	int lim = max - min;
	return position().x + size().y / 4 + (lim ? (val - min) * sliderLim() / lim : sliderLim());
}

int Slider::sliderLim() const {
	ivec2 siz = size();
	return siz.x - siz.y / 2 - barSize;
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
	if (textTex)
		Quad::draw(textRect(), Rect(rbg.x + textMargin, rbg.y, rbg.w - textMargin * 2, rbg.h).intersect(frm), dimFactor, textTex);
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
	return textOfs(textTex.getRes(), halign, position(), size(), textMargin);
}

void Label::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text.c_str(), size().y);
}

// TEXT BOX

TextBox::TextBox(Size size, int lineH, string lines, BCall leftCall, BCall rightCall, const Texture& tooltip, float dim, uint lineL, bool stickTop, bool bg, GLuint tex, const vec4& clr) :
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
	scroll.draw(rect(), textTex.getRes(), position(), size(), true);
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

bool TextBox::selectable() const {
	return true;
}

ivec2 TextBox::textPos() const {
	return textOfs(textTex.getRes(), halign, position(), size(), textMargin) - scroll.listPos;
}

void TextBox::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text.c_str(), lineHeight, uint(size().x - textMargin * 2 - ScrollBar::width));
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
		lineCnt++;
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
	if (!alignTop)
		scroll.listPos = scroll.listLim(textTex.getRes(), size());
}

void TextBox::cutLines() {
	if (lineCnt = uint(std::count(text.begin(), text.end(), '\n')); lineCnt > lineLim)
		for (sizet i = 0; i < text.length(); i++)
			if (text[i] == '\n' && --lineCnt <= lineLim) {
				text.erase(0, i + 1);
				break;
			}
}

// ICON

Icon::Icon(Size size, string line, BCall leftCall, BCall rightCall, BCall holdCall, const Texture& tooltip, float dim, Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(line), leftCall, rightCall, tooltip, dim, align, bg, tex, clr),
	selected(false),
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
		vec4 clr = colorLight * dimFactor;
		Quad::draw(Rect(rbg.pos(), ivec2(rbg.w, olSize)), clr, World::scene()->texture(), -1.f);
		Quad::draw(Rect(rbg.x, rbg.y + rbg.h - olSize, rbg.w, olSize), clr, World::scene()->texture(), -1.f);
		Quad::draw(Rect(rbg.x, rbg.y + olSize, olSize, rbg.h - olSize * 2), clr, World::scene()->texture(), -1.f);
		Quad::draw(Rect(rbg.x + rbg.w - olSize, rbg.y + olSize, olSize, rbg.h - olSize * 2), clr, World::scene()->texture(), -1.f);
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
		World::scene()->setContext(new Context(mPos, options, ocall, position(), World::state()->lineHeight, this, size().x));
}

bool ComboBox::selectable() const {
	return true;
}

void ComboBox::setText(const string& str) {
	setCurOpt(uint(std::find(options.begin(), options.end(), str) - options.begin()));
}

void ComboBox::set(vector<string>&& opts, uint id) {
	options = std::move(opts);
	setCurOpt(id);
}

// LABEL EDIT

LabelEdit::LabelEdit(Size size, string line, BCall leftCall, BCall rightCall, BCall retCall, BCall cancCall, const Texture& tooltip, float dim, uint lim, bool isChat, Label::Alignment align, bool bg, GLuint tex, const vec4& clr) :
	Label(size, std::move(line), leftCall, rightCall, tooltip, dim, align, bg, tex, clr),
	oldText(text),
	ecall(retCall),
	ccall(cancCall),
	limit(lim),
	cpos(0),
	textOfs(0),
	chatMode(isChat)
{
	cutText();
}

void LabelEdit::drawTop() const {
	ivec2 ps = position();
	Quad::draw(Rect(caretPos() + ps.x + textMargin, ps.y, caretWidth, size().y), frame(), colorLight, World::scene()->texture(), -1.f);
}

void LabelEdit::onClick(const ivec2&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && (lcall || ecall)) {
		setCPos(uint(text.length()));
		World::window()->setTextInput(this);
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void LabelEdit::onKeypress(const SDL_KeyboardEvent& key) {
	switch (key.keysym.scancode) {
	case SDL_SCANCODE_UP:	// move caret to start
		setCPos(0);
		break;
	case SDL_SCANCODE_DOWN:	// move caret to end
		setCPos(uint(text.length()));
		break;
	case SDL_SCANCODE_LEFT:	// move caret left
		if (kmodAlt(key.keysym.mod))	// if holding alt skip word
			setCPos(findWordStart());
		else if (kmodCtrl(key.keysym.mod))	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos)	// otherwise go left by one
			setCPos(jumpCharB(cpos));
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (kmodAlt(key.keysym.mod))	// if holding alt skip word
			setCPos(findWordEnd());
		else if (kmodCtrl(key.keysym.mod))	// if holding ctrl go to end
			setCPos(uint(text.length()));
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(jumpCharF(cpos));
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (kmodAlt(key.keysym.mod)) {	// if holding alt delete left word
			uint id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		} else if (kmodCtrl(key.keysym.mod)) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTextTex();
			setCPos(0);
		} else if (cpos) {	// otherwise delete left character
			uint id = jumpCharB(cpos);
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		}
		break;
	case SDL_SCANCODE_DELETE:	// delete right character
		if (kmodAlt(key.keysym.mod)) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTex();
		} else if (kmodCtrl(key.keysym.mod)) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTex();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, jumpCharF(cpos) - cpos);
			updateTextTex();
		}
		break;
	case SDL_SCANCODE_HOME:	// move caret to beginning
		if (!key.repeat)
			setCPos(0);
		break;
	case SDL_SCANCODE_END:	// move caret to end
		if (!key.repeat)
			setCPos(uint(text.length()));
		break;
	case SDL_SCANCODE_V:	// paste text
		if (kmodCtrl(key.keysym.mod))
			if (char* str = SDL_GetClipboardText()) {
				onText(str);
				SDL_free(str);
			}
		break;
	case SDL_SCANCODE_C:	// copy text
		if (!key.repeat && kmodCtrl(key.keysym.mod))
			SDL_SetClipboardText(text.c_str());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (!key.repeat && kmodCtrl(key.keysym.mod)) {
			SDL_SetClipboardText(text.c_str());
			setText(string());
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (!key.repeat && kmodCtrl(key.keysym.mod))
			setText(std::move(oldText));
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		if (!key.repeat) {
			confirm();
			World::prun(ecall, this);
		}
		break;
	case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_TAB: case SDL_SCANCODE_AC_BACK:
		if (!key.repeat) {
			cancel();
			World::prun(ccall, this);
		}
	}
}

void LabelEdit::onJButton(uint8 but) {
	switch (but) {
	case InputSys::jbutA:
		confirm();
		World::prun(ecall, this);
		break;
	case InputSys::jbutB:
		cancel();
		World::prun(ccall, this);
	}
}

void LabelEdit::onJHat(uint8 hat) {
	if (hat & SDL_HAT_LEFT) {
		if (cpos)
			setCPos(jumpCharB(cpos));
	} else if (hat & SDL_HAT_UP)
		setCPos(0);

	if (hat & SDL_HAT_RIGHT) {
		if (cpos < text.length())
			setCPos(jumpCharF(cpos));
	} else if (hat & SDL_HAT_DOWN)
		setCPos(uint(text.length()));
}

void LabelEdit::onGButton(SDL_GameControllerButton but) {
	switch (but) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		setCPos(0);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		setCPos(uint(text.length()));
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		if (cpos)
			setCPos(jumpCharB(cpos));
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		if (cpos < text.length())
			setCPos(jumpCharF(cpos));
		break;
	case SDL_CONTROLLER_BUTTON_A:
		confirm();
		World::prun(ecall, this);
		break;
	case SDL_CONTROLLER_BUTTON_B:
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
	setCPos(cpos + uint(slen));
}

void LabelEdit::setText(string&& str) {
	oldText = std::move(text);
	text = std::move(str);
	cutText();
	onTextReset();
}

void LabelEdit::setText(const string& str) {
	oldText = std::move(text);
	text = str.length() <= limit ? str : str.substr(0, cutLength(str.c_str(), limit));
	onTextReset();
}

void LabelEdit::onTextReset() {
	updateTextTex();
	cpos = uint(text.length());
	textOfs = 0;
}

sizet LabelEdit::cutLength(const char* str, sizet lim) {
	sizet i = lim - 1;
	if (str[i] >= 0)
		return lim;

	for (; i && str[i-1] < 0; i--);	// cut possible u8 char
	sizet end = i + u8clen(str[i]);
	return end <= lim ? end : i;
}

void LabelEdit::cutText() {
	if (text.length() > limit)
		text.erase(cutLength(text.c_str(), limit));
}

void LabelEdit::setCPos(uint cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - textMargin * 2; ce > sx)
		textOfs -= ce - sx;
}

ivec2 LabelEdit::textPos() const {
	return Label::textPos() + ivec2(textOfs, 0);
}

int LabelEdit::caretPos() const {
	return World::fonts()->length(text.substr(0, cpos).c_str(), size().y) + textOfs;
}

void LabelEdit::confirm() {
	if (chatMode)
		setText(string());
	else
		World::window()->setTextInput();
	textOfs = 0;
	World::prun(lcall, this);
}

void LabelEdit::cancel() {
	if (!chatMode) {
		text = oldText;
		updateTextTex();
	}
	textOfs = 0;
	World::window()->setTextInput();
}

uint LabelEdit::jumpCharB(uint i) {
	while (--i && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::jumpCharF(uint i) {
	while (++i < text.length() && (text[i] & 0xC0) == 0x80);
	return i;
}

uint LabelEdit::findWordStart() {
	uint i = cpos;
	if (i == text.length() && i)
		i--;
	else if (notSpace(text[i]) && i)
		if (uint n = jumpCharB(i); isSpace(text[n]))	// skip if first letter of word
			i = n;
	for (; isSpace(text[i]) && i; i--);		// skip first spaces
	for (; notSpace(text[i]) && i; i--);	// skip word
	return i ? i + 1 : i;			// correct position if necessary
}

uint LabelEdit::findWordEnd() {
	uint i = cpos;
	for (; isSpace(text[i]) && i < text.length(); i++);		// skip first spaces
	for (; notSpace(text[i]) && i < text.length(); i++);	// skip word
	return i;
}

bool LabelEdit::selectable() const {
	return true;
}
