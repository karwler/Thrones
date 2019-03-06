#pragma once

#include "utils.h"

// size of a widget in pixels or relative to it's parent
struct Size {
	union {
		int pix;
		float prc;
	};
	bool usePix;

	Size(int pixels);
	Size(float percent);

	void set(int pxiels);
	void set(float precent);
};
using vec2s = cvec2<Size>;

inline Size::Size(int pixels) :
	pix(pixels),
	usePix(true)
{}

inline Size::Size(float percent) :
	prc(percent),
	usePix(false)
{}

inline void Size::set(int pixels) {
	usePix = true;
	pix = pixels;
}

inline void Size::set(float percent) {
	usePix = false;
	prc = percent;
}

// can be used as spacer
class Widget {
protected:
	enum class Color : uint8 {
		normal,
		dark,
		light,
		select
	};
	static const array<SDL_Color, sizet(Color::select)+1> colors;

	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizet pcID;		// this widget's id in parent's widget list
	Size relSize;	// size relative to parent's parameters

public:
	Widget(const Size& relSize = 1.f, Layout* parent = nullptr, sizet id = SIZE_MAX);	// parent and id should be set by Layout
	virtual ~Widget() = default;

	virtual void draw() const {}
	virtual void tick(float) {}
	virtual void onResize() {}
	virtual void postInit() {}	// gets called after parent is set and all set up
	virtual void onClick(const vec2i&, uint8) {}
	virtual void onDoubleClick(const vec2i&, uint8) {}
	virtual void onMouseMove(const vec2i&, const vec2i&) {}
	virtual void onHold(const vec2i&, uint8) {}
	virtual void onDrag(const vec2i&, const vec2i&) {}	// mouse move while left button down
	virtual void onUndrag(uint8) {}			// get's called on mouse button up if instance is Scene's capture
	virtual void onScroll(const vec2i&) {}	// on mouse wheel y movement
	virtual void onKeypress(const SDL_Keysym&) {}
	virtual void onText(const string&) {}

	sizet getID() const;
	Layout* getParent() const;
	void setParent(Layout* pnt, sizet id);

	const Size& getRelSize() const;
	virtual vec2i position() const;
	virtual vec2i size() const;
	Rect rect() const;			// the rectangle that is the widget
	virtual Rect frame() const;	// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)
	virtual bool selectable() const;

protected:
	void drawRect(const Rect& rect, Color color) const;
	void drawRect(const Rect& rect, SDL_Color color) const;
	void drawTexture(const Texture* tex, Rect rect, const Rect& frame) const;
};

inline sizet Widget::getID() const {
	return pcID;
}

inline Layout* Widget::getParent() const {
	return parent;
}

inline const Size& Widget::getRelSize() const {
	return relSize;
}

inline Rect Widget::rect() const {
	return Rect(position(), size());
}

inline void Widget::drawRect(const Rect& rect, Color color) const {
	drawRect(rect, colors[uint8(color)]);
}

// visible widget with texture and background color
class Picture : public Widget {
public:
	static constexpr int defaultIconMargin = 2;

protected:
	const Texture* bgTex;	// managed by WindowSys
	int bgMargin;
private:
	bool showColor;

public:
	Picture(const Size& relSize = 1.f, bool showColor = true, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Picture() override = default;

	virtual void draw() const override;

	virtual Color color() const;
	virtual Rect texRect() const;
};

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Picture {
protected:
	PCall lcall, rcall, dcall;

public:
	Button(const Size& relSize = 1.f, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, bool showColor = true, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Button() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onDoubleClick(const vec2i& mPos, uint8 mBut) override;
	
	virtual Color color() const override;
	virtual bool selectable() const override;
};

class Draglet : public Button {
private:
	bool dragging;

public:
	Draglet(const Size& relSize = 1.f, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, bool showColor = true, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Draglet() override = default;

	virtual void draw() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;
};

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	bool on;

public:
	CheckBox(const Size& relSize = 1.f, bool on = false, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, bool showColor = true, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~CheckBox() override = default;

	virtual void draw() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	Rect boxRect() const;
	Color boxColor() const;
	bool toggle();
};

inline Widget::Color CheckBox::boxColor() const {
	return on ? Color::light : Color::dark;
}

inline bool CheckBox::toggle() {
	return on = !on;
}

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right
	};

	static constexpr int defaultTextMargin = 5;

protected:
	string text;
	Texture textTex;	// managed by Label
	int textMargin;
	Alignment align;	// text alignment

public:
	Label(const Size& relSize = 1.f, const string& text = emptyStr, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, Alignment alignment = Alignment::left, const Texture* bgTex = nullptr, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Label() override;

	virtual void draw() const override;
	virtual void postInit() override;

	const string& getText() const;
	virtual void setText(const string& str);
	Rect textRect() const;
	Rect textFrame() const;
	virtual Rect texRect() const override;
	int textIconOffset() const;
protected:
	virtual vec2i textPos() const;
	virtual void updateTextTex();
};

inline const string& Label::getText() const {
	return text;
}

inline Rect Label::textRect() const {
	return Rect(textPos(), textTex.getRes());
}

inline int Label::textIconOffset() const {
	return bgTex ? int(float(size().y * bgTex->getRes().x) / float(bgTex->getRes().y)) : 0;
}

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
private:
	vector<string> options;
	sizet curOpt;

public:
	SwitchBox(const Size& relSize = 1.f, const string* opts = nullptr, sizet ocnt = 0, const string& curOption = emptyStr, PCall call = nullptr, Alignment alignment = Alignment::left, const Texture* bgTex = nullptr, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~SwitchBox() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	sizet getCurOpt() const;
private:
	void shiftOption(int ofs);
};

inline sizet SwitchBox::getCurOpt() const {
	return curOpt;
}

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm rather than on click and dcall when enter is pressed)
class LabelEdit : public Label {
public:
	enum class TextType : uint8 {
		text,
		sInt,
		sIntSpaced,
		uInt,
		uIntSpaced,
		sFloat,
		sFloatSpaced,
		uFloat,
		uFloatSpaced
	};

private:
	TextType textType;
	string oldText;
	sizet cpos;		// caret position
	int textOfs;	// text's horizontal offset

	static constexpr int caretWidth = 4;

public:
	LabelEdit(const Size& relSize = 1.f, const string& text = emptyStr, PCall leftCall = nullptr, PCall rightCall = nullptr, PCall doubleCall = nullptr, TextType type = TextType::text, const Texture* bgTex = nullptr, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~LabelEdit() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onDoubleClick(const vec2i&, uint8) override {}
	virtual void onKeypress(const SDL_Keysym& key) override;
	virtual void onText(const string& str) override;

	void drawCaret() const;
	const string& getOldText() const;
	virtual void setText(const string& str) override;

	void confirm();
	void cancel();

private:
	virtual vec2i textPos() const override;
	int caretPos() const;	// caret's relative x position
	void setCPos(sizet cp);

	sizet findWordStart();	// returns index of first character of word before cpos
	sizet findWordEnd();		// returns index of character after last character of word after cpos
	void cleanText();
	void cleanSIntSpacedText();
	void cleanUIntSpacedText();
	void cleanSFloatText();
	void cleanSFloatSpacedText();
	void cleanUFloatText();
	void cleanUFloatSpacedText();
};

inline const string& LabelEdit::getOldText() const {
	return oldText;
}
