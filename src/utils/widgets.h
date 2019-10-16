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

// vertex data for widgets
class Shape {
public:
	GLuint vao;

	static constexpr uint corners = 4;
private:
	static constexpr uint stride = 2;
	static constexpr float vertices[corners * stride] = {
		0.f, 0.f,
		1.f, 0.f,
		1.f, 1.f,
		0.f, 1.f,
	};

public:
	void init(ShaderGUI* gui);
	void free(ShaderGUI* gui);
};

// can be used as spacer
class Widget : public Interactable {
public:
	static const vec4 colorNormal;
	static const vec4 colorDark;
	static const vec4 colorLight;
	static const vec4 colorTooltip;

protected:
	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizet pcID;		// this widget's id in parent's widget list
	Size relSize;	// size relative to parent's parameters

public:
	Widget(Size relSize = 1.f, Layout* parent = nullptr, sizet id = SIZE_MAX);	// parent and id should be set by Layout
	virtual ~Widget() override = default;

	virtual void draw() const {}	// returns frame for possible futher reuse
	virtual void tick(float) {}
	virtual void onResize() {}
	virtual void postInit() {}		// gets called after parent is set and all set up
	virtual void onScroll(const ivec2&) {}
	virtual ivec2 position() const;
	virtual ivec2 size() const;
	virtual Rect frame() const;	// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)
	virtual bool selectable() const;

	sizet getID() const;
	Layout* getParent() const;
	void setParent(Layout* pnt, sizet id);
	const Size& getRelSize() const;
	Rect rect() const;			// the rectangle that is the widget

protected:
	static void drawRect(const Rect& rect, const vec4& color, GLuint tex, float z = 0.f);
	static void drawRect(const Rect& rect, Rect frame, const vec4& color, GLuint tex, float z = 0.f);
private:
	static void drawRect(const Rect& rect, const vec4& uvrect, const vec4& color, GLuint tex, float z = 0.f);
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

inline void Widget::drawRect(const Rect& rect, const vec4& color, GLuint tex, float z) {
	drawRect(rect, vec4(0.f, 0.f, 1.f, 1.f), color, tex, z);
}

inline void Widget::drawRect(const Rect& rect, Rect frame, const vec4& color, GLuint tex, float z) {
	frame = rect.intersect(frame);
	drawRect(frame, vec4(float(frame.x - rect.x) / float(rect.w), float(frame.y - rect.y) / float(rect.h), float(frame.w) / float(rect.w), float(frame.h) / float(rect.h)), color, tex, z);
}

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Widget {
public:
	vec4 color;
protected:
	vec4 dimFactor;
	BCall lcall, rcall;
	Texture tipTex;		// memory is handled by the button
	GLuint bgTex;

private:
	static constexpr float selectFactor = 1.2f;
	static constexpr ivec2 tooltipMargin = { 4, 1 };
	static constexpr int cursorMargin = 2;

public:
	Button(Size relSize = 1.f, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Button() override;

	virtual void draw() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;

	virtual bool selectable() const override;
	void setDim(float factor);
	void setLcall(BCall pcl);
	void drawTooltip() const;
};

inline void Button::setDim(float factor) {
	dimFactor = vec4(factor, factor, factor, dimFactor.a);
}

inline void Button::setLcall(BCall pcl) {
	lcall = pcl;
}

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	bool on;

public:
	CheckBox(Size relSize = 1.f, bool on = false, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~CheckBox() override = default;

	virtual void draw() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;

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

// horizontal slider (maybe one day it'll be able to be vertical)
class Slider : public Button {
public:
	static constexpr int barSize = 10;
private:
	int val, vmin, vmax;
	int diffSliderMouse;

public:
	Slider(Size relSize = 1.f, int value = 0, int minimum = 0, int maximum = 255, BCall finishCall = nullptr, BCall updateCall = nullptr, const Texture& tooltip = Texture(), GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Slider() override = default;

	virtual void draw() const override;
	virtual void onClick(const ivec2&, uint8) override {}
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;

	int getVal() const;
	void setVal(int value);

	Rect barRect() const;
	Rect sliderRect() const;

private:
	void setSlider(int xpos);
	int sliderPos() const;
	int sliderLim() const;
};

inline int Slider::getVal() const {
	return val;
}

inline void Slider::setVal(int value) {
	val = std::clamp(value, vmin, vmax);
}

inline int Slider::sliderPos() const {
	return position().x + size().y/4 + val * sliderLim() / vmax;
}

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right
	};
	static constexpr int textMargin = 5;

protected:
	string text;
	Texture textTex;	// managed by Label
	Alignment align;	// text alignment
	bool showBG;

public:
	Label(Size relSize = 1.f, string text = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), Alignment alignment = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Label() override;

	virtual void draw() const override;
	virtual void onResize() override;
	virtual void postInit() override;

	const string& getText() const;
	virtual void setText(string&& str);
	virtual void setText(const string& str);
	Rect textRect() const;
protected:
	static ivec2 textPos(const Texture& text, Alignment alignment, ivec2 pos, ivec2 siz, int margin);
	virtual ivec2 textPos() const;
	void updateTextTex();
};

inline const string& Label::getText() const {
	return text;
}

inline Rect Label::textRect() const {
	return Rect(textPos(), textTex.getRes());
}

// a weird thing for game related icons
class Draglet : public Label {
private:
	static constexpr int olSize = 3;

	bool dragging;
	bool selected;
	BCall hcall;

public:
	Draglet(Size relSize = 1.f, BCall leftCall = nullptr, BCall holdCall = nullptr, BCall rightCall = nullptr, GLuint bgTex = 0, const vec4& color = vec4(1.f), const Texture& tooltip = Texture(), string text = string(), Alignment alignment = Alignment::left, bool showBG = true, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Draglet() override = default;

	virtual void draw() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	void setSelected(bool on);
};

inline void Draglet::setSelected(bool on) {
	selected = on;
}

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
private:
	vector<string> options;
	uint curOpt;

public:
	SwitchBox(Size relSize = 1.f, vector<string> opts = {}, string curOption = string(), BCall call = nullptr, const Texture& tooltip = Texture(), Alignment alignment = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~SwitchBox() override = default;

	virtual void onClick(const ivec2& mPos, uint8 mBut) override;

	virtual bool selectable() const override;
	virtual void setText(const string& str) override;
	uint getCurOpt() const;
	void setCurOpt(uint id);
private:
	void shiftOption(int mov);
};

inline uint SwitchBox::getCurOpt() const {
	return curOpt;
}

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm rather than on click and dcall when enter is pressed)
class LabelEdit : public Label {
public:
	static constexpr int caretWidth = 4;

private:
	string oldText;
	BCall ecall;
	uint limit;
	uint cpos;		// caret position
	int textOfs;	// text's horizontal offset

public:
	LabelEdit(Size relSize = 1.f, string line = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, BCall retCall = nullptr, const Texture& tooltip = Texture(), uint limit = UINT_MAX, Alignment alignment = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~LabelEdit() override = default;

	virtual void drawTop() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onKeypress(const SDL_Keysym& key) override;
	virtual void onText(const char* str) override;

	virtual bool selectable() const override;
	const string& getOldText() const;
	virtual void setText(string&& str) override;
	virtual void setText(const string& str) override;

	void confirm();
	void cancel();

protected:
	virtual ivec2 textPos() const override;
private:
	void onTextReset();
	int caretPos() const;	// caret's relative x position
	void setCPos(uint cp);

	uint findWordStart();	// returns index of first character of word before cpos
	uint findWordEnd();		// returns index of character after last character of word after cpos
};

inline const string& LabelEdit::getOldText() const {
	return oldText;
}
