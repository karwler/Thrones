#include "engine/world.h"

// RECTANGLE

void Rectangle::init(ShaderGUI* gui) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(gui->vertex);
	glVertexAttribPointer(gui->vertex, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(gui->uvloc);
	glVertexAttribPointer(gui->uvloc, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(0));

	glBindVertexArray(0);
	glDeleteBuffers(1, &vbo);
}

void Rectangle::free(ShaderGUI* gui) {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(gui->vertex);
	glDisableVertexAttribArray(gui->uvloc);
	glDeleteVertexArrays(1, &vao);
}

// WIDGET

const vec4 Widget::colorNormal(0.5f, 0.1f, 0.f, 1.f);
const vec4 Widget::colorDark(0.298f, 0.f, 0.f, 1.f);
const vec4 Widget::colorLight(1.f, 0.757f, 0.145f, 1.f);
const vec4 Widget::colorTooltip(0.298f, 0.f, 0.f, 0.7f);

Widget::Widget(Size relSize, Layout* parent, sizet id) :
	parent(parent),
	pcID(id),
	relSize(relSize)
{}

void Widget::setParent(Layout* pnt, sizet id) {
	parent = pnt;
	pcID = id;
}

vec2i Widget::position() const {
	return parent->wgtPosition(pcID);
}

vec2i Widget::size() const {
	return parent->wgtSize(pcID);
}

Rect Widget::frame() const {
	return parent->frame();
}

bool Widget::selectable() const {
	return false;
}

void Widget::drawRect(const Rect& rect, const vec4& uvrect, const vec4& color, GLuint tex, float z) {
	float trans[4] = { float(rect.x), float(rect.y), float(rect.w), float(rect.h) };
	glUniform4fv(World::gui()->rect, 1, trans);
	glUniform4fv(World::gui()->uvrc, 1, glm::value_ptr(uvrect));
	glUniform1f(World::gui()->zloc, z);
	glUniform4fv(World::gui()->color, 1, glm::value_ptr(color));
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_QUADS, 0, Rectangle::corners);
}

// BUTTON

Button::Button(Size relSize, BCall leftCall, BCall rightCall, const Texture& tooltip, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	color(color),
	dimFactor(1.f),
	lcall(leftCall),
	rcall(rightCall),
	tipTex(tooltip),
	bgTex(bgTex ? bgTex : World::scene()->blank())
{}

Button::~Button() {
	tipTex.close();
}

void Button::draw() const {
	drawRect(rect(), frame(), color * dimFactor, bgTex);
}

void Button::onClick(vec2i, uint8 mBut) {
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

bool Button::selectable() const {
	return lcall || rcall || tipTex.valid();
}

void Button::drawTooltip() const {
	if (!tipTex.valid())
		return;

	int ofs = World::window()->getCursorHeight() + cursorMargin;
	vec2i pos = mousePos() + vec2i(0, ofs);
	vec2i siz = tipTex.getRes() + tooltipMargin * 2;
	if (pos.x + siz.x > World::window()->getView().x)
		pos.x = World::window()->getView().x - siz.x;
	if (pos.y + siz.y > World::window()->getView().y)
		pos.y = pos.y - ofs - siz.y;

	drawRect(Rect(pos, siz), colorTooltip, World::scene()->blank(), -1.f);
	drawRect(Rect(pos + tooltipMargin, tipTex.getRes()), vec4(1.f), tipTex.getID(), -1.f);
}

// CHECK BOX

CheckBox::CheckBox(Size relSize, bool on, BCall leftCall, BCall rightCall, const Texture& tooltip, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, tooltip, bgTex, color, parent, id),
	on(on)
{}

void CheckBox::draw() const {
	Rect frm = frame();
	drawRect(rect(), frm, color * dimFactor, bgTex);							// draw background
	drawRect(boxRect(), frm, boxColor() * dimFactor, World::scene()->blank());	// draw checkbox
}

void CheckBox::onClick(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		toggle();
	Button::onClick(mPos, mBut);
}

Rect CheckBox::boxRect() const {
	vec2i siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// SLIDER

Slider::Slider(Size relSize, int value, int minimum, int maximum, BCall leftCall, BCall rightCall, const Texture& tooltip, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, tooltip, bgTex, color, parent, id),
	val(value),
	vmin(minimum),
	vmax(maximum),
	diffSliderMouse(0)
{}

void Slider::draw() const {
	Rect frm = frame();
	drawRect(rect(), frm, color * dimFactor, bgTex);					// draw background
	drawRect(barRect(), frm, colorDark, World::scene()->blank());		// draw bar
	drawRect(sliderRect(), frm, colorLight, World::scene()->blank());	// draw slider
}

void Slider::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Slider::onHold(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		if (int sp = sliderPos(); outRange(mPos.x, sp, sp + barSize))	// if mouse outside of slider
			setSlider(mPos.x - barSize / 2);
		diffSliderMouse = mPos.x - sliderPos();	// get difference between mouse x and slider x
	}
}

void Slider::onDrag(vec2i mPos, vec2i) {
	setSlider(mPos.x - diffSliderMouse);
}

void Slider::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)	// if dragging slider stop dragging slider
		World::scene()->capture = nullptr;
}

void Slider::setSlider(int xpos) {
	setVal((xpos - position().x - size().y/4) * vmax / sliderLim());
	World::prun(lcall, this);
}

Rect Slider::barRect() const {
	vec2i siz = size();
	int height = siz.y / 2;
	return Rect(position() + siz.y / 4, vec2i(siz.x - height, height));
}

Rect Slider::sliderRect() const {
	vec2i pos = position(), siz = size();
	return Rect(sliderPos(), pos.y, barSize, siz.y);
}

int Slider::sliderLim() const {
	vec2i siz = size();
	return siz.x - siz.y/2 - barSize;
}

// LABEL

Label::Label(Size relSize, string text, BCall leftCall, BCall rightCall, const Texture& tooltip, Alignment alignment, bool showBG, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, tooltip, bgTex, color, parent, id),
	text(std::move(text)),
	align(alignment),
	showBG(showBG)
{}

Label::~Label() {
	textTex.close();
}

void Label::draw() const {
	Rect rbg = rect();
	Rect frm = frame();
	if (showBG)
		drawRect(rbg, frm, color * dimFactor, bgTex);
	if (textTex.valid())
		drawRect(textRect(), Rect(rbg.x + textMargin, rbg.y, rbg.w - textMargin * 2, rbg.h).intersect(frm), dimFactor, textTex.getID());
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

vec2i Label::textPos(const Texture& text, Label::Alignment alignment, vec2i pos, vec2i siz, int margin) {
	switch (alignment) {
	case Alignment::left:
		return vec2i(pos.x + margin, pos.y);
	case Alignment::center: {
		return vec2i(pos.x + (siz.x - text.getRes().x) / 2, pos.y); }
	case Alignment::right:
		return vec2i(pos.x + siz.x - text.getRes().x - margin, pos.y);
	}
	return 0;
}

vec2i Label::textPos() const {
	return textPos(textTex, align, position(), size(), textMargin);
}

void Label::updateTextTex() {
	textTex.close();
	textTex = World::fonts()->render(text, size().y);
}

// DRAGLET

Draglet::Draglet(Size relSize, BCall leftCall, BCall holdCall, BCall rightCall, GLuint bgTex, const vec4& color, const Texture& tooltip, string text, Alignment alignment, bool showBG, Layout* parent, sizet id) :
	Label(relSize, std::move(text), leftCall, rightCall, tooltip, alignment, showBG, bgTex, color, parent, id),
	dragging(false),
	selected(false),
	hcall(holdCall)
{}

void Draglet::draw() const {
	Rect frm = frame();
	vec4 bclr = color * dimFactor;
	vec4 tclr = vec4(dimFactor);
	drawRect(rect(), frm, bclr, bgTex);
	if (textTex.valid())
		drawRect(textRect(), frm, tclr, textTex.getID());

	if (selected) {
		Rect rbg = rect();
		drawRect(Rect(rbg.pos(), vec2i(rbg.w, olSize)), colorLight, World::scene()->blank(), -1.f);
		drawRect(Rect(rbg.x, rbg.y + rbg.h - olSize, rbg.w, olSize), colorLight, World::scene()->blank(), -1.f);
		drawRect(Rect(rbg.x, rbg.y + olSize, olSize, rbg.h - olSize * 2), colorLight, World::scene()->blank(), -1.f);
		drawRect(Rect(rbg.x + rbg.w - olSize, rbg.y + olSize, olSize, rbg.h - olSize * 2), colorLight, World::scene()->blank(), -1.f);
	}
	if (dragging) {
		vec2i siz = size() / 2;
		vec2i pos = mousePos() - siz / 2;
		drawRect(Rect(pos, siz), bclr, bgTex, -1.f);
		if (textTex.valid())
			drawRect(Rect(textPos(textTex, align, pos, siz, textMargin / 2), textTex.getRes() / 2), tclr, textTex.getID(), -1.f);
	}
}

void Draglet::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Draglet::onHold(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && lcall) {
		dragging = true;
		World::scene()->capture = this;
		World::prun(hcall, this);
	}
}

void Draglet::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		dragging = false;
		World::scene()->capture = nullptr;
		World::prun(lcall, this);
	}
}

// SWITCH BOX

SwitchBox::SwitchBox(Size relSize, const string* opts, uint ocnt, string curOption, BCall call, const Texture& tooltip, Alignment alignment, bool showBG, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Label(relSize, std::move(curOption), call, call, tooltip, alignment, showBG, bgTex, color, parent, id),
	options(opts, opts + ocnt),
	curOpt(uint(std::find(options.begin(), options.end(), text) - options.begin()))
{
	if (curOpt >= options.size())
		curOpt = 0;
}

void SwitchBox::onClick(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		shiftOption(1);
	else if (mBut == SDL_BUTTON_RIGHT)
		shiftOption(-1);
	Button::onClick(mPos, mBut);
}

bool SwitchBox::selectable() const {
	return true;
}

void SwitchBox::setText(const string& str) {
	uint op = uint(std::find(options.begin(), options.end(), str) - options.begin());
	curOpt = op < options.size() ? op : 0;
	Label::setText(options[curOpt]);
}

void SwitchBox::setCurOpt(uint id) {
	curOpt = clampHigh(id, uint(options.size() - 1));
	Label::setText(options[curOpt]);
}

void SwitchBox::shiftOption(int mov) {
	curOpt = cycle(curOpt, uint(options.size()), mov);
	Label::setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(Size relSize, string line, BCall leftCall, BCall rightCall, BCall retCall, const Texture& tooltip, Label::Alignment alignment, bool showBG, GLuint bgTex, const vec4& color, Layout* parent, sizet id) :
	Label(relSize, std::move(line), leftCall, rightCall, tooltip, alignment, showBG, bgTex, color, parent, id),
	oldText(text),
	ecall(retCall),
	cpos(0),
	textOfs(0)
{}

void LabelEdit::drawTop() const {
	vec2i ps = position();
	drawRect(Rect(caretPos() + ps.x + textMargin, ps.y, caretWidth, size().y), frame(), colorLight, World::scene()->blank(), -1.f);
}

void LabelEdit::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		SDL_StartTextInput();
		setCPos(uint(text.length()));
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void LabelEdit::onKeypress(const SDL_Keysym& key) {
	switch (key.scancode) {
	case SDL_SCANCODE_LEFT:	// move caret left
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordStart());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl move to beginning
			setCPos(0);
		else if (cpos > 0)	// otherwise go left by one
			setCPos(cpos - 1);
		break;
	case SDL_SCANCODE_RIGHT:	// move caret right
		if (key.mod & KMOD_LALT)	// if holding alt skip word
			setCPos(findWordEnd());
		else if (key.mod & KMOD_CTRL)	// if holding ctrl go to end
			setCPos(uint(text.length()));
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(cpos + 1);
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (key.mod & KMOD_LALT) {	// if holding alt delete left word
			uint id = findWordStart();
			text.erase(id, cpos - id);
			updateTextTex();
			setCPos(id);
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to left
			text.erase(0, cpos);
			updateTextTex();
			setCPos(0);
		} else if (cpos > 0) {	// otherwise delete left character
			text.erase(cpos - 1, 1);
			updateTextTex();
			setCPos(cpos - 1);
		}
		break;
	case SDL_SCANCODE_DELETE:	// delete right character
		if (key.mod & KMOD_LALT) {	// if holding alt delete right word
			text.erase(cpos, findWordEnd() - cpos);
			updateTextTex();
		} else if (key.mod & KMOD_CTRL) {	// if holding ctrl delete line to right
			text.erase(cpos, text.length() - cpos);
			updateTextTex();
		} else if (cpos < text.length()) {	// otherwise delete right character
			text.erase(cpos, 1);
			updateTextTex();
		}
		break;
	case SDL_SCANCODE_HOME:	// move caret to beginning
		setCPos(0);
		break;
	case SDL_SCANCODE_END:	// move caret to end
		setCPos(uint(text.length()));
		break;
	case SDL_SCANCODE_V:	// paste text
		if (key.mod & KMOD_CTRL)
			if (char* str = SDL_GetClipboardText()) {
				onText(str);
				SDL_free(str);
			}
		break;
	case SDL_SCANCODE_C:	// copy text
		if (key.mod & KMOD_CTRL)
			SDL_SetClipboardText(text.c_str());
		break;
	case SDL_SCANCODE_X:	// cut text
		if (key.mod & KMOD_CTRL) {
			SDL_SetClipboardText(text.c_str());
			setText("");
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (key.mod & KMOD_CTRL)
			setText(std::move(oldText));
		break;
	case SDL_SCANCODE_RETURN:
		confirm();
		World::prun(ecall, this);
		break;
	case SDL_SCANCODE_ESCAPE:
		cancel();
	}
}

void LabelEdit::onText(const char* str) {
	sizet olen = text.length();
	text.insert(cpos, str);
	updateTextTex();
	setCPos(cpos + (uint(text.length() - olen)));
}

void LabelEdit::setText(string&& str) {
	oldText = std::move(text);
	text = std::move(str);
	onTextReset();
}

void LabelEdit::setText(const string& str) {
	oldText = std::move(text);
	text = str;
	onTextReset();
}

void LabelEdit::onTextReset() {
	updateTextTex();
	cpos = text.length();
	textOfs = 0;
}

void LabelEdit::setCPos(uint cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - textMargin * 2; ce > sx)
		textOfs -= ce - sx;
}

vec2i LabelEdit::textPos() const {
	return Label::textPos() + vec2i(textOfs, 0);
}

int LabelEdit::caretPos() const {
	return World::fonts()->length(text.substr(0, cpos), size().y) + textOfs;
}

void LabelEdit::confirm() {
	textOfs = 0;
	World::scene()->capture = nullptr;
	SDL_StopTextInput();
	World::prun(lcall, this);
}

void LabelEdit::cancel() {
	textOfs = 0;
	text = oldText;
	updateTextTex();

	World::scene()->capture = nullptr;
	SDL_StopTextInput();
}

uint LabelEdit::findWordStart() {
	uint i = cpos;
	if (notSpace(text[i]) && i > 0 && isSpace(text[i-1]))	// skip if first letter of word
		i--;
	for (; isSpace(text[i]) && i > 0; i--);		// skip first spaces
	for (; notSpace(text[i]) && i > 0; i--);	// skip word
	return i == 0 ? i : i + 1;			// correct position if necessary
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
