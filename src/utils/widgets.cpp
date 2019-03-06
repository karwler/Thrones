#include "engine/world.h"

// WIDGET

const array<SDL_Color, Widget::colors.size()> Widget::colors = {
	SDL_Color({205/2, 175/2, 149/2, 255}),
	SDL_Color({139/2, 119/2, 101/2, 255}),
	SDL_Color({238/2, 203/2, 173/2, 255}),
	SDL_Color({255/2, 218/2, 185/2, 255})
};

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

void Widget::drawRect(const Rect& rect, SDL_Color color) const {
	glDisable(GL_TEXTURE_2D);
	glColor4ubv(reinterpret_cast<const GLubyte*>(&color));

	glBegin(GL_QUADS);
	glVertex2i(rect.x, rect.y);
	glVertex2i(rect.x + rect.w, rect.y);
	glVertex2i(rect.x + rect.w, rect.y + rect.h);
	glVertex2i(rect.x, rect.y + rect.h);
	glEnd();
}

void Widget::drawTexture(const Texture* tex, Rect rect, const Rect& frame) const {
	vec4 crop = rect.crop(frame);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->getID());
	glColor4f(1.f, 1.f, 1.f, 1.f);

	glBegin(GL_QUADS);
	glTexCoord2f(crop.x, crop.y);
	glVertex2i(rect.x, rect.y);
	glTexCoord2f(crop.z, crop.y);
	glVertex2i(rect.x + rect.w, rect.y);
	glTexCoord2f(crop.z, crop.a);
	glVertex2i(rect.x + rect.w, rect.y + rect.h);
	glTexCoord2f(crop.x, crop.a);
	glVertex2i(rect.x, rect.y + rect.h);
	glEnd();
}

// PICTURE

Picture::Picture(const Size& relSize, bool showColor, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	bgTex(bgTex),
	bgMargin(bgMargin),
	showColor(showColor)
{}

void Picture::draw() const {
	Rect frm = frame();
	if (showColor)
		drawRect(rect().getOverlap(frm), color());
	if (bgTex)
		drawTexture(bgTex, texRect(), frm);
}

Widget::Color Picture::color() const {
	return Color::normal;
}

Rect Picture::texRect() const {
	Rect rct = rect();
	return Rect(rct.pos() + bgMargin, rct.size() - bgMargin * 2);
}

// BUTTON

Button::Button(const Size& relSize, PCall leftCall, PCall rightCall, PCall doubleCall, bool showColor, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Picture(relSize, showColor, bgTex, bgMargin, parent, id),
	lcall(leftCall),
	rcall(rightCall),
	dcall(doubleCall)
{}

void Button::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		parent->selectWidget(pcID);
		World::prun(lcall, this);
	} else if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Button::onDoubleClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT)
		World::prun(dcall, this);
}

Widget::Color Button::color() const {
	if (parent->getSelected() == this)
		return Color::light;
	if (selectable() && World::scene()->select == this)
		return Color::select;
	return Color::normal;
}

bool Button::selectable() const {
	return lcall || rcall || dcall;
}

// DRAGLET

Draglet::Draglet(const Size& relSize, PCall leftCall, PCall rightCall, PCall doubleCall, bool showColor, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, bgTex, bgMargin, parent, id),
	dragging(false)
{}

void Draglet::draw() const {
	Picture::draw();
	if (dragging)
		drawRect(Rect(mousePos(), size()), color());
}

void Draglet::onClick(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_RIGHT)
		World::prun(rcall, this);
}

void Draglet::onHold(const vec2i&, uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
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

// CHECK BOX

CheckBox::CheckBox(const Size& relSize, bool on, PCall leftCall, PCall rightCall, PCall doubleCall, bool showColor, const Texture* bgTex, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, bgTex, bgMargin, parent, id),
	on(on)
{}

void CheckBox::draw() const {
	Picture::draw();										// draw background
	drawRect(boxRect().getOverlap(frame()), boxColor());	// draw checkbox
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

Label::Label(const Size& relSize, const string& text, PCall leftCall, PCall rightCall, PCall doubleCall, Alignment alignment, const Texture* bgTex, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Button(relSize, leftCall, rightCall, doubleCall, showColor, bgTex, bgMargin, parent, id),
	text(text),
	textMargin(textMargin),
	align(alignment)
{}

Label::~Label() {
	if (textTex.valid())
		textTex.close();
}

void Label::draw() const {
	Picture::draw();
	if (textTex.valid())
		drawTexture(&textTex, textRect(), textFrame());
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
	return Rect(rct.x + ofs + textMargin, rct.y, rct.w - ofs - textMargin * 2, rct.h).getOverlap(frame());
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
	if (textTex.valid())
		textTex.close();
	textTex = World::winSys()->renderText(text, size().y);
}

// SWITCH BOX

SwitchBox::SwitchBox(const Size& relSize, const string* opts, sizet ocnt, const string& curOption, PCall call, Alignment alignment, const Texture* bgTex, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, curOption, call, call, nullptr, alignment, bgTex, showColor, textMargin, bgMargin, parent, id),
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

LabelEdit::LabelEdit(const Size& relSize, const string& text, PCall leftCall, PCall rightCall, PCall doubleCall, TextType type, const Texture* bgTex, bool showColor, int textMargin, int bgMargin, Layout* parent, sizet id) :
	Label(relSize, text, leftCall, rightCall, doubleCall, Alignment::left, bgTex, showColor, textMargin, bgMargin, parent, id),
	textType(type),
	oldText(text),
	cpos(0),
	textOfs(0)
{
	cleanText();
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

void LabelEdit::drawCaret() const {
	vec2i ps = position();
	drawRect({caretPos() + ps.x + textIconOffset() + textMargin, ps.y, caretWidth, size().y}, Color::light);
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
