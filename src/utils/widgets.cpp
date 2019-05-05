#include "engine/world.h"

// WIDGET

const vec4 Widget::colorNormal(205.f/2.f / 255.f, 175.f/2.f / 255.f, 149.f/2.f / 255.f, 1.f);
const vec4 Widget::colorDark(139.f/2.f / 255.f, 119.f/2.f / 255.f, 101.f/2.f / 255.f, 1.f);
const vec4 Widget::colorLight(238.f/2.f / 255.f, 203.f/2.f / 255.f, 173.f/2.f / 255.f, 1.f);
const vec4 Widget::colorSelect(255.f/2.f / 255.f, 218.f/2.f / 255.f, 185.f/2.f / 255.f, 1.f);

Widget::Widget(const Size& relSize, Layout* parent, sizet id) :
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

void Widget::drawTexture(const Texture* tex, Rect rect, const Rect& frame) {
	vec4 crop = rect.crop(frame);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->getID());
	glColor4f(1.f, 1.f, 1.f, 1.f);

	glBegin(GL_QUADS);
	glTexCoord2f(crop.x, crop.y);
	glVertex3i(rect.x, rect.y, 0);
	glTexCoord2f(crop.z, crop.y);
	glVertex3i(rect.x + rect.w, rect.y, 0);
	glTexCoord2f(crop.z, crop.a);
	glVertex3i(rect.x + rect.w, rect.y + rect.h, 0);
	glTexCoord2f(crop.x, crop.a);
	glVertex3i(rect.x, rect.y + rect.h, 0);
	glEnd();
}

// PICTURE

Picture::Picture(const Size& relSize, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	bgTex(bgTex),
	color(color),
	bgMargin(bgMargin),
	showColor(showColor)
{}

void Picture::draw() const {
	Rect frm = frame();
	if (showColor)
		drawRect(rect().intersect(frm), bgColor());
	if (bgTex)
		drawTexture(bgTex, texRect(), frm);
}

Rect Picture::texRect() const {
	Rect rct = rect();
	return Rect(rct.pos() + bgMargin, rct.size() - bgMargin * 2);
}

const vec4& Picture::bgColor() const {
	return World::scene()->select != this ? color : colorSelect;
}

// BUTTON

Button::Button(const Size& relSize, BCall leftCall, BCall rightCall, BCall doubleCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Picture(relSize, showColor, color, bgTex, bgMargin, parent, id),
	lcall(leftCall),
	rcall(rightCall),
	dcall(doubleCall)
{}

void Button::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(lcall, this);
	else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Button::onDoubleClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(dcall, this);
}

bool Button::selectable() const {
	return lcall || rcall || dcall;
}

// DRAGLET

const vec4 Draglet::borderColor(0.804f, 0.522f, 0.247f, 1.f);

Draglet::Draglet(const Size& relSize, BCall leftCall, BCall rightCall, BCall doubleCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, color, bgTex, bgMargin, parent, id),
	dragging(false),
	selected(false)
{
	updateSelectColor();
}

void Draglet::draw() const {
	Picture::draw();

	if (selected) {
		Rect rbg = rect();
		drawRect(Rect(rbg.pos(), vec2i(rbg.w, bgMargin)), borderColor, 1);
		drawRect(Rect(rbg.x, rbg.y + rbg.h - bgMargin, rbg.w, bgMargin), borderColor, 1);
		drawRect(Rect(rbg.x, rbg.y + bgMargin, bgMargin, rbg.h - bgMargin * 2), borderColor, 1);
		drawRect(Rect(rbg.x + rbg.w - bgMargin, rbg.y + bgMargin, bgMargin, rbg.h - bgMargin * 2), borderColor, 1);
	}
	if (dragging) {
		vec2i siz = size() / 2;
		vec2i pos = mousePos() - siz / 2;
		if (showColor)
			drawRect(Rect(pos, siz), color, 1);
		if (bgTex)
			drawTexture(bgTex, Rect(pos + bgMargin / 2, siz - bgMargin), Rect(0, World::winSys()->windowSize()));
	}
}

void Draglet::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Draglet::onHold(const vec2i&, uint8 mBut) {
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

void Draglet::setColor(const vec4& clr) {
	color = clr;
	updateSelectColor();
}

const vec4& Draglet::bgColor() const {
	return dragging || World::scene()->select == this ? selectColor : color;
}

// CHECK BOX

CheckBox::CheckBox(const Size& relSize, bool on, BCall leftCall, BCall rightCall, BCall doubleCall, bool showColor, const vec4& color, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, color, bgTex, bgMargin, parent, id),
	on(on)
{}

void CheckBox::draw() const {
	Picture::draw();									// draw background
	drawRect(boxRect().intersect(frame()), boxColor());	// draw checkbox
}

void CheckBox::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT || mBut == SDL_BUTTON_RIGHT)
		toggle();
	Button::onClick(mPos, mBut);
}

Rect CheckBox::boxRect() const {
	vec2i siz = size();
	int margin = (siz.x > siz.y ? siz.y : siz.x) / 4;
	return Rect(position() + margin, siz - margin * 2);
}

// LABEL

Label::Label(const Size& relSize, const string& text, BCall leftCall, BCall rightCall, BCall doubleCall, Alignment alignment, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, color, bgTex, bgMargin, parent, id),
	text(text),
	textMargin(textMargin),
	align(alignment)
{}

Label::~Label() {
	textTex.close();
}

void Label::draw() const {
	Picture::draw();
	if (textTex.valid())
		drawTexture(&textTex, textRect(), textFrame());
}

void Label::onResize() {
	updateTextTex();
}

void Label::postInit() {
	updateTextTex();
}

void Label::setText(const string& str) {
	text = str;
	updateTextTex();
}

Rect Label::textFrame() const {
	Rect rct = rect();
	int ofs = textIconOffset();
	return Rect(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).intersect(frame());
}

Rect Label::texRect() const {
	Rect rct = rect();
	rct.h -= bgMargin * 2;
	return Rect(rct.pos() + bgMargin, vec2i(float(rct.h * bgTex->getRes().x) / float(bgTex->getRes().y), rct.h));
}

vec2i Label::textPos() const {
	switch (vec2i pos = position(); align) {
	case Alignment::left:
		return vec2i(pos.x + textIconOffset() + textMargin, pos.y);
	case Alignment::center: {
		int iofs = textIconOffset();
		return vec2i(pos.x + iofs + (size().x - iofs - textTex.getRes().x)/2, pos.y); }
	case Alignment::right:
		return vec2i(pos.x + size().x - textTex.getRes().x - textMargin, pos.y);
	}
	return 0;	// to get rid of warning
}

void Label::updateTextTex() {
	textTex.close();
	textTex = World::winSys()->renderText(text, size().y);
}

// SWITCH BOX

SwitchBox::SwitchBox(const Size& relSize, const string* opts, sizet ocnt, const string& curOption, BCall call, Alignment alignment, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, curOption, call, call, nullptr, alignment, bgTex, color, showColor, textMargin, bgMargin, parent, id),
	options(opts, opts + ocnt),
	curOpt(sizet(std::find(options.begin(), options.end(), curOption) - options.begin()))
{
	if (curOpt >= options.size())
		curOpt = 0;
}

void SwitchBox::onClick(const vec2i& mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		shiftOption(1);
	else if (mBut == SDL_BUTTON_RIGHT)
		shiftOption(-1);
	Button::onClick(mPos, mBut);
}

void SwitchBox::shiftOption(int ofs) {
	curOpt = (curOpt + sizet(ofs)) % options.size();
	setText(options[curOpt]);
}

// LABEL EDIT

LabelEdit::LabelEdit(const Size& relSize, const string& text, BCall leftCall, BCall rightCall, BCall doubleCall, TextType type, const Texture* bgTex, const vec4& color, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, text, leftCall, rightCall, doubleCall, Alignment::left, bgTex, color, showColor, textMargin, bgMargin, parent, id),
	textType(type),
	oldText(text),
	cpos(0),
	textOfs(0)
{
	cleanText();
}

void LabelEdit::draw() const {
	Label::draw();

	if (World::scene()->capture == this) {	// caret
		vec2i ps = position();
		drawRect({ caretPos() + ps.x + textIconOffset() + textMargin, ps.y, caretWidth, size().y }, colorLight, 1);
	}
}

void LabelEdit::onClick(const vec2i&, uint8 mBut) {
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
			setText(emptyStr);
		}
		break;
	case SDL_SCANCODE_Z:	// set text to old text
		if (key.mod & KMOD_CTRL)
			setText(string(oldText));
		break;
	case SDL_SCANCODE_RETURN:
		confirm();
		World::prun(dcall, this);
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

vec2i LabelEdit::textPos() const {
	vec2i pos = position();
	return vec2i(pos.x + textOfs + textIconOffset() + textMargin, pos.y);
}

void LabelEdit::setText(const string& str) {
	oldText = text;
	text = str;

	cleanText();
	updateTextTex();
	setCPos(text.length());
}

void LabelEdit::setCPos(sizet cp) {
	cpos = cp;
	if (int cl = caretPos(); cl < 0)
		textOfs -= cl;
	else if (int ce = cl + caretWidth, sx = size().x - bgMargin*2; ce > sx)
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
