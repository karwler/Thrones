#include "engine/world.h"

// WIDGET

const vec4 Widget::colorNormal(205.f/2.f / 255.f, 175.f/2.f / 255.f, 149.f/2.f / 255.f, 1.f);
const vec4 Widget::colorDark(139.f/2.f / 255.f, 119.f/2.f / 255.f, 101.f/2.f / 255.f, 1.f);
const vec4 Widget::colorLight(238.f/2.f / 255.f, 203.f/2.f / 255.f, 173.f/2.f / 255.f, 1.f);
const vec4 Widget::colorSelect(255.f/2.f / 255.f, 218.f/2.f / 255.f, 185.f/2.f / 255.f, 1.f);
const vec4 Widget::colorTexture(1.f);

Widget::Widget(Size relSize, Layout* parent, sizet id) :
	parent(parent),
	pcID(id),
	relSize(relSize)
{}

Rect Widget::draw() const {
	return Rect();
}

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

void Widget::drawRect(const Rect& rect, const vec4& color, int z) {
	glDisable(GL_TEXTURE_2D);
	glColor4fv(glm::value_ptr(color));

	glBegin(GL_QUADS);
	glVertex3i(rect.x, rect.y, z);
	glVertex3i(rect.x + rect.w, rect.y, z);
	glVertex3i(rect.x + rect.w, rect.y + rect.h, z);
	glVertex3i(rect.x, rect.y + rect.h, z);
	glEnd();
}

void Widget::drawTexture(const Texture* tex, Rect rect, const Rect& frame, const vec4& color, int z) {
	vec4 crop = rect.crop(frame);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->getID());
	glColor4fv(glm::value_ptr(color));

	glBegin(GL_QUADS);
	glTexCoord2f(crop.x, crop.y);
	glVertex3i(rect.x, rect.y, z);
	glTexCoord2f(crop.z, crop.y);
	glVertex3i(rect.x + rect.w, rect.y, z);
	glTexCoord2f(crop.z, crop.a);
	glVertex3i(rect.x + rect.w, rect.y + rect.h, z);
	glTexCoord2f(crop.x, crop.a);
	glVertex3i(rect.x, rect.y + rect.h, z);
	glEnd();
}

// PICTURE

Picture::Picture(Size relSize, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	bgTex(bgTex),
	color(color),
	dimFactor(1.f),
	bgMargin(bgMargin),
	showColor(showColor)
{}

Rect Picture::draw() const {
	Rect frm = frame();
	if (showColor)
		drawRect(rect().intersect(frm), (World::scene()->select != this ? color : colorSelect) * dimFactor);
	if (bgTex)
		drawTexture(bgTex, texRect(), frm);
	return frm;
}

Rect Picture::texRect() const {
	Rect rct = rect();
	return Rect(rct.pos() + bgMargin, rct.size() - bgMargin * 2);
}

// BUTTON

Button::Button(Size relSize, BCall leftCall, BCall rightCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Picture(relSize, showColor, color, bgTex, bgMargin, parent, id),
	lcall(leftCall),
	rcall(rightCall)
{}

void Button::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(lcall, this);
	else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

bool Button::selectable() const {
	return lcall || rcall;
}

// CHECK BOX

CheckBox::CheckBox(Size relSize, bool on, BCall leftCall, BCall rightCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, showColor, color, bgTex, bgMargin, parent, id),
	on(on)
{}

Rect CheckBox::draw() const {
	Rect frm = Picture::draw();						// draw background
	drawRect(boxRect().intersect(frm), boxColor());	// draw checkbox
	return frm;
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

Slider::Slider(Size relSize, int value, int minimum, int maximum, BCall leftCall, BCall rightCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, showColor, color, bgTex, bgMargin, parent, id),
	val(value),
	vmin(minimum),
	vmax(maximum),
	diffSliderMouse(0)
{}

Rect Slider::draw() const {
	Rect frm = Picture::draw();							// draw background
	drawRect(barRect().intersect(frm), colorDark);		// draw bar
	drawRect(sliderRect().intersect(frm), colorLight);	// draw slider
	return frm;
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

Label::Label(Size relSize, string text, BCall leftCall, BCall rightCall, Alignment alignment, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, showColor, color, bgTex, bgMargin, parent, id),
	text(std::move(text)),
	textMargin(textMargin),
	align(alignment)
{}

Label::~Label() {
	textTex.close();
}

Rect Label::draw() const {
	Rect frm = Picture::draw();
	if (textTex.valid())
		drawTexture(&textTex, textRect(), frm);
	return frm;
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(string str) {
	text = std::move(str);
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

void Label::updateTextTex() {
	textTex.close();
	textTex = World::winSys()->renderText(text, size().y);
}

// DRAGLET

const vec4 Draglet::borderColor(0.804f, 0.522f, 0.247f, 1.f);

Draglet::Draglet(Size relSize, BCall leftCall, BCall rightCall, bool showColor, const vec4& color, const Texture* bgTex, string text, Alignment alignment, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, std::move(text), leftCall, rightCall, alignment, bgTex, color, showColor, textMargin, bgMargin, parent, id),
	dragging(false),
	selected(false)
{}

Rect Draglet::draw() const {
	Rect frm = frame();
	vec4 tclr = colorTexture * dimFactor;
	if (showColor)
		drawRect(rect().intersect(frm), (dragging || World::scene()->select == this ? glm::clamp(color * selectFactor, vec4(0.f), vec4(1.f)) : color * dimFactor));
	if (bgTex)
		drawTexture(bgTex, texRect(), frm, tclr);
	if (textTex.valid())
		drawTexture(&textTex, textRect(), frame(), tclr);

	if (selected) {
		Rect rbg = rect();
		drawRect(Rect(rbg.pos(), vec2i(rbg.w, bgMargin)), borderColor, 1);
		drawRect(Rect(rbg.x, rbg.y + rbg.h - bgMargin, rbg.w, bgMargin), borderColor, 1);
		drawRect(Rect(rbg.x, rbg.y + bgMargin, bgMargin, rbg.h - bgMargin * 2), borderColor, 1);
		drawRect(Rect(rbg.x + rbg.w - bgMargin, rbg.y + bgMargin, bgMargin, rbg.h - bgMargin * 2), borderColor, 1);
	}
	if (dragging) {
		frm = Rect(0, World::winSys()->windowSize());
		vec2i siz = size() / 2;
		vec2i pos = mousePos() - siz / 2;
		if (showColor)
			drawRect(Rect(pos, siz), color, 1);
		if (bgTex)
			drawTexture(bgTex, Rect(pos + bgMargin / 2, siz - bgMargin), frm, tclr, 1);
		if (textTex.valid())
			drawTexture(&textTex, Rect(textPos(textTex, align, pos, siz, textMargin / 2), textTex.getRes() / 2), frm, tclr, 1);
	}
	return Rect();
}

void Draglet::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Draglet::onHold(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT && lcall) {
		World::scene()->capture = this;
		dragging = true;
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

SwitchBox::SwitchBox(Size relSize, const string* opts, sizet ocnt, string curOption, BCall call, Alignment alignment, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, std::move(curOption), call, call, alignment, bgTex, color, showColor, textMargin, bgMargin, parent, id),
	options(opts, opts + ocnt),
	curOpt(sizet(std::find(options.begin(), options.end(), curOption) - options.begin()))
{
	if (curOpt >= options.size())
		curOpt = 0;
}

void SwitchBox::onClick(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		shiftOption(true);
	else if (mBut == SDL_BUTTON_RIGHT)
		shiftOption(false);
	Button::onClick(mPos, mBut);
}

bool SwitchBox::selectable() const {
	return true;
}

void SwitchBox::shiftOption(bool fwd) {
	if (curOpt += btom<sizet>(fwd); curOpt >= options.size())
		curOpt = fwd ? 0 : options.size() - 1;
	setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(Size relSize, string line, BCall leftCall, BCall rightCall, BCall retCall, TextType type, Label::Alignment alignment, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, std::move(line), leftCall, rightCall, alignment, bgTex, color, showColor, textMargin, bgMargin, parent, id),
	textType(type),
	textOfs(0),
	oldText(text),
	cpos(0),
	ecall(retCall)
{
	cleanText();
}

void LabelEdit::drawTop() const {
	vec2i ps = position();
	drawRect({ caretPos() + ps.x + textMargin, ps.y, caretWidth, size().y }, colorLight, 1);
}

void LabelEdit::onClick(vec2i, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		World::scene()->capture = this;
		SDL_StartTextInput();
		setCPos(text.length());
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
			setCPos(text.length());
		else if (cpos < text.length())	// otherwise go right by one
			setCPos(cpos + 1);
		break;
	case SDL_SCANCODE_BACKSPACE:	// delete left
		if (key.mod & KMOD_LALT) {	// if holding alt delete left word
			sizet id = findWordStart();
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
		setCPos(text.length());
		break;
	case SDL_SCANCODE_V:	// paste text
		if (key.mod & KMOD_CTRL)
			onText(SDL_GetClipboardText());
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

void LabelEdit::onText(const string& str) {
	sizet olen = text.length();
	text.insert(cpos, str);
	cleanText();
	updateTextTex();
	setCPos(cpos + (text.length() - olen));
}

void LabelEdit::setText(string str) {
	oldText = std::move(text);
	text = std::move(str);

	cleanText();
	updateTextTex();
	setCPos(text.length());
}

void LabelEdit::setCPos(sizet cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - bgMargin * 2; ce > sx)
		textOfs -= ce - sx;
}

int LabelEdit::caretPos() const {
	return World::winSys()->textLength(text.substr(0, cpos), size().y) + textOfs;
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

sizet LabelEdit::findWordStart() {
	sizet i = cpos;
	if (notSpace(text[i]) && i > 0 && isSpace(text[i-1]))	// skip if first letter of word
		i--;
	for (; isSpace(text[i]) && i > 0; i--);		// skip first spaces
	for (; notSpace(text[i]) && i > 0; i--);	// skip word
	return i == 0 ? i : i + 1;			// correct position if necessary
}

sizet LabelEdit::findWordEnd() {
	sizet i = cpos;
	for (; isSpace(text[i]) && i < text.length(); i++);		// skip first spaces
	for (; notSpace(text[i]) && i < text.length(); i++);	// skip word
	return i;
}

void LabelEdit::cleanText() {
	text.erase(std::remove_if(text.begin(), text.end(), [](char c) -> bool { return uchar(c) < ' '; }), text.end());

	switch (textType) {
	case TextType::sInt:
		text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
		text.erase(std::remove_if(text.begin() + pdift(text[0] == '-'), text.end(), notDigit), text.end());
		break;
	case TextType::sIntSpaced:
		cleanSIntSpacedText();
		break;
	case TextType::uInt:
		text.erase(std::remove_if(text.begin(), text.end(), notDigit), text.end());
		break;
	case TextType::uIntSpaced:
		cleanUIntSpacedText();
		break;
	case TextType::sFloat:
		cleanSFloatText();
		break;
	case TextType::sFloatSpaced:
		cleanSFloatSpacedText();
		break;
	case TextType::uFloat:
		cleanUFloatText();
		break;
	case TextType::uFloatSpaced:
		cleanUFloatSpacedText();
	}
}

void LabelEdit::cleanSIntSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isDigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUIntSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [](char c) -> bool { return isDigit(c) || c == ' '; }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == '.'  && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isDigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanSFloatSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin() + pdift(text[0] == '-'); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == ' ') {
			if (it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; }); it != text.end() && *it == '-')
				it++;
		} else if (*it == '.' && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isDigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == '.'  && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isDigit(c) || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}

void LabelEdit::cleanUFloatSpacedText() {
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
	bool dot = false;
	for (string::iterator it = text.begin(); it != text.end();) {
		if (isDigit(*it))
			it = std::find_if(it + 1, text.end(), notDigit);
		else if (*it == ' ')
			it = std::find_if(it + 1, text.end(), [](char c) -> bool { return c != ' '; });
		else if (*it == '.' && !dot) {
			dot = true;
			it++;
		} else {
			pdift ofs = it - text.begin();
			text.erase(it, std::find_if(it + 1, text.end(), [dot](char c) -> bool { return isDigit(c) || c == ' ' || (c == '.' && !dot); }));
			it = text.begin() + ofs;
		}
	}
}
