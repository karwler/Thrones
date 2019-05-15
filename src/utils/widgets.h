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

	void set(int pixels);
	void set(float percent);
};

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
class Widget : public Interactable {
public:
	static const vec4 colorNormal;
	static const vec4 colorDark;
	static const vec4 colorLight;
	static const vec4 colorSelect;
protected:
	static const vec4 colorTexture;

	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizet pcID;		// this widget's id in parent's widget list
	Size relSize;	// size relative to parent's parameters

public:
	Widget(const Size& relSize = 1.f, Layout* parent = nullptr, sizet id = SIZE_MAX);	// parent and id should be set by Layout
	virtual ~Widget() override = default;

	virtual void draw() const {}
	virtual void tick(float) {}
	virtual void onResize() {}
	virtual void postInit() {}	// gets called after parent is set and all set up
	virtual void onScroll(const vec2i&) {}

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
	static void drawRect(const Rect& rect, const vec4& color, int z = 0);
	static void drawTexture(const Texture* tex, Rect rect, const Rect& frame, const vec4& color = colorTexture, int z = 0);
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

// visible widget with texture and background color
class Picture : public Widget {
public:
	static constexpr int defaultIconMargin = 2;

	const Texture* bgTex;	// managed by WindowSys
	vec4 color;
protected:
	int bgMargin;
	bool showColor;

public:
	Picture(const Size& relSize = 1.f, bool showColor = true, const vec4& color = colorNormal, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Picture() override = default;

	virtual void draw() const override;

	Rect texRect() const;
};

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Picture {
protected:
	BCall lcall, rcall;

public:
	Button(const Size& relSize = 1.f, BCall leftCall = nullptr, BCall rightCall = nullptr, bool showColor = true, const vec4& color = colorNormal, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Button() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	virtual bool selectable() const override;
	void setLcall(BCall pcl);
};

inline void Button::setLcall(BCall pcl) {
	lcall = pcl;
}

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	bool on;

public:
	CheckBox(const Size& relSize = 1.f, bool on = false, BCall leftCall = nullptr, BCall rightCall = nullptr, bool showColor = true, const vec4& color = colorNormal, const Texture* bgTex = nullptr, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~CheckBox() override = default;

	virtual void draw() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	Rect boxRect() const;
	const vec4& boxColor() const;
	bool toggle();
};

inline const vec4& CheckBox::boxColor() const {
	return on ? colorLight : colorDark;
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
	Label(const Size& relSize = 1.f, const string& text = emptyStr, BCall leftCall = nullptr, BCall rightCall = nullptr, Alignment alignment = Alignment::left, const Texture* bgTex = nullptr, const vec4& color = colorNormal, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Label() override;

	virtual void draw() const override;
	virtual void onResize() override;
	virtual void postInit() override;

	const string& getText() const;
	virtual void setText(const string& str);
	Rect textRect() const;
protected:
	static vec2i textPos(const Texture& text, Alignment alignment, const vec2i& pos, const vec2i& siz, int margin);
	vec2i textPos() const;
	void updateTextTex();
};

inline const string& Label::getText() const {
	return text;
}

inline Rect Label::textRect() const {
	return Rect(textPos(), textTex.getRes());
}

inline vec2i Label::textPos() const {
	return textPos(textTex, align, position(), size(), textMargin);
}

// a weird thing for game related icons
class Draglet : public Label {
private:
	static constexpr float selectFactor = 1.2f;
	static const vec4 borderColor;

	bool dragging;
	bool selected;
	float dimFactor;

public:
	Draglet(const Size& relSize = 1.f, BCall leftCall = nullptr, BCall rightCall = nullptr, bool showColor = true, const vec4& color = colorNormal, const Texture* bgTex = nullptr, const string& text = emptyStr, Alignment alignment = Alignment::left, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Draglet() override = default;

	virtual void draw() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	void setSelected(bool on);
	void setDim(float factor);
};

inline void Draglet::setSelected(bool on) {
	selected = on;
}

inline void Draglet::setDim(float factor) {
	dimFactor = factor;
}

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
private:
	vector<string> options;
	sizet curOpt;

public:
	SwitchBox(const Size& relSize = 1.f, const string* opts = nullptr, sizet ocnt = 0, const string& curOption = emptyStr, BCall call = nullptr, Alignment alignment = Alignment::left, const Texture* bgTex = nullptr, const vec4& color = colorNormal, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~SwitchBox() override = default;

	virtual void onClick(const vec2i& mPos, uint8 mBut) override;

	virtual bool selectable() const override;
	sizet getCurOpt() const;
private:
	void shiftOption(bool fwd);
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
	int textOfs;	// text's horizontal offset
	string oldText;
	sizet cpos;		// caret position
	BCall ecall;

	static constexpr int caretWidth = 4;

public:
	LabelEdit(const Size& relSize = 1.f, const string& text = emptyStr, BCall leftCall = nullptr, BCall rightCall = nullptr, BCall retCall = nullptr, TextType type = TextType::text, Alignment alignment = Alignment::left, const Texture* bgTex = nullptr, const vec4& color = colorNormal, bool showColor = true, int textMargin = defaultTextMargin, int bgMargin = defaultIconMargin, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~LabelEdit() override = default;

	virtual void drawTop() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onKeypress(const SDL_Keysym& key) override;
	virtual void onText(const string& str) override;

	const string& getOldText() const;
	virtual void setText(const string& str) override;

	void confirm();
	void cancel();

private:
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
