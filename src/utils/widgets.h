#pragma once

#include "utils.h"

class Widget;

// size of a widget in pixels or relative to it's parent
struct Size {
	union {
		int pix;
		float prc;
	};
	bool usePix;

	constexpr Size(int pixels);
	constexpr Size(float percent);
};

inline constexpr Size::Size(int pixels) :
	pix(pixels),
	usePix(true)
{}

inline constexpr Size::Size(float percent) :
	prc(percent),
	usePix(false)
{}

// scroll information
class ScrollBar {
public:
	static constexpr int width = 10;

	ivec2 listPos;
private:
	vec2 motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position
	bool draggingSlider;

	static constexpr float throttle = 10.f;

public:
	ScrollBar();

	void draw(const Rect& frame, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) const;
	void tick(float dSec, const ivec2& listSize, const ivec2& size);
	void hold(const ivec2& mPos, uint8 mBut, Widget* wgt, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	void drag(const ivec2& mPos, const ivec2& mMov, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	void undrag(uint8 mBut, bool vert);
	void scroll(const ivec2& wMov, const ivec2& listSize, const ivec2& size, bool vert);

	static ivec2 listLim(const ivec2& listSize, const ivec2& size);	// max list position
	void moveListPos(const ivec2& mov, const ivec2& listSize, const ivec2& size);
	static int barSize(const ivec2& listSize, const ivec2& size, bool vert);	// returns 0 if slider isn't needed
private:
	static Rect barRect(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	Rect sliderRect(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) const;
	void setSlider(int spos, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	static int sliderSize(const ivec2& listSize, const ivec2& size, bool vert);
	int sliderPos(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) const;
	static int sliderLim(const ivec2& listSize, const ivec2& size, bool vert);	// max slider position
	static void throttleMotion(float& mov, float dSec);
};

inline ivec2 ScrollBar::listLim(const ivec2& listSize, const ivec2& size) {
	return ivec2(size.x < listSize.x ? listSize.x - size.x : 0, size.y < listSize.y ? listSize.y - size.y : 0);
}

inline void ScrollBar::moveListPos(const ivec2& mov, const ivec2& listSize, const ivec2& size) {
	listPos = glm::clamp(ivec2(listPos + mov), ivec2(0), listLim(listSize, size));
}

inline int ScrollBar::barSize(const ivec2& listSize, const ivec2& size, bool vert) {
	return listSize[vert] > size[vert] ? width : 0;
}

inline int ScrollBar::sliderSize(const ivec2& listSize, const ivec2& size, bool vert) {
	return size[vert] < listSize[vert] ? size[vert] * size[vert] / listSize[vert] : size[vert];
}

inline int ScrollBar::sliderPos(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert) const {
	return listSize[vert] > size[vert] ? pos[vert] + listPos[vert] * sliderLim(listSize, size, vert) / listLim(listSize, size)[vert] : pos[vert];
}

inline int ScrollBar::sliderLim(const ivec2& listSize, const ivec2& size, bool vert) {
	return size[vert] - sliderSize(listSize, size, vert);
}

// vertex data for widgets
class Quad {
public:
	static constexpr uint corners = 4;
private:
	static constexpr uint stride = 2;
	static const float vertices[corners * stride];

	GLuint vao, vbo;

public:
	Quad();
	~Quad();

	GLuint getVao() const;
};

inline GLuint Quad::getVao() const {
	return vao;
}

// can be used as spacer
class Widget : public Interactable {
public:
	static const vec4 colorNormal;
	static const vec4 colorDimmed;
	static const vec4 colorDark;
	static const vec4 colorLight;
	static const vec4 colorTooltip;

	Size relSize;	// size relative to parent's parameters
protected:
	Layout* parent;	// every widget that isn't a Layout should have a parent
	sizet pcID;		// this widget's id in parent's widget list

public:
	Widget(Size relSize = 1.f, Layout* parent = nullptr, sizet id = SIZE_MAX);	// parent and id should be set by Layout
	virtual ~Widget() override = default;

	virtual void draw() const {}	// returns frame for possible futher reuse
	virtual void drawTop() const {}
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
	Rect rect() const;			// the rectangle that is the widget

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
	BCall lcall, rcall;
	vec4 color;
protected:
	vec4 dimFactor;
	Texture tipTex;		// memory is handled by the button
	GLuint bgTex;

private:
	static constexpr float selectFactor = 1.2f;
	static constexpr ivec2 tooltipMargin = { 4, 1 };
	static constexpr int cursorMargin = 2;

public:
	Button(Size relSize = 1.f, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Button() override;

	virtual void draw() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;

	virtual bool selectable() const override;
	void setDim(float factor);
	void drawTooltip() const;
};

inline void Button::setDim(float factor) {
	dimFactor = vec4(factor, factor, factor, dimFactor.a);
}

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	bool on;

	CheckBox(Size relSize = 1.f, bool on = false, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
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
	Slider(Size relSize = 1.f, int value = 0, int minimum = 0, int maximum = 255, BCall finishCall = nullptr, BCall updateCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Slider() override = default;

	virtual void draw() const override;
	virtual void onClick(const ivec2&, uint8) override {}
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;

	int getVal() const;
	void setVal(int value);
	void set(int value, int minimum, int maximum);

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

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right,
	};
	static constexpr int textMargin = 5;

protected:
	string text;
	Texture textTex;	// managed by Label
	Alignment halign;	// text alignment
	bool showBG;

public:
	Label(Size relSize = 1.f, string text = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment halign = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Label() override;

	virtual void draw() const override;
	virtual void onResize() override;
	virtual void postInit() override;

	const string& getText() const;
	virtual void setText(string&& str);
	virtual void setText(const string& str);
	Rect textRect() const;
protected:
	static ivec2 textOfs(const ivec2& res, Alignment align, const ivec2& pos, const ivec2& siz, int margin);
	virtual ivec2 textPos() const;
	virtual void updateTextTex();
};

inline const string& Label::getText() const {
	return text;
}

inline Rect Label::textRect() const {
	return Rect(textPos(), textTex.getRes());
}

// multiline scrollable label
class TextBox : public Label {
private:
	bool alignTop;
	ScrollBar scroll;
	int lineHeight;
	uint lineLim, lineCnt;

public:
	TextBox(Size relSize = 1.f, int lineHeight = 0, string lines = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, uint lineLim = UINT_MAX, bool alignTop = true, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~TextBox() override = default;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onHover() override {}
	virtual void onUnhover() override {}
	virtual void onScroll(const ivec2& wMov) override;

	virtual bool selectable() const override;
	virtual void setText(string &&str) override;
	virtual void setText(const string &str) override;
	void addLine(const string& line);
	string moveText();
private:
	virtual ivec2 textPos() const override;
	virtual void updateTextTex() override;
	void updateListPos();
	void resetListPos();
	void cutLines();
};

inline void TextBox::resetListPos() {
	scroll.listPos = alignTop ? ivec2(0) : scroll.listLim(textTex.getRes(), size());
}

// a weird thing for game related icons
class Draglet : public Label {
public:
	bool selected;

private:
	static constexpr int olSize = 3;

	bool showTop, showingTop;
	BCall hcall;

public:
	Draglet(Size relSize = 1.f, BCall leftCall = nullptr, BCall holdCall = nullptr, BCall rightCall = nullptr, GLuint bgTex = 0, const vec4& color = vec4(1.f), float dim = 1.f, const Texture& tooltip = Texture(), string text = string(), Alignment alignment = Alignment::left, bool showTop = true, bool showBG = true, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Draglet() override = default;

	virtual void draw() const override;
	virtual void drawTop() const override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
};

// for switching between multiple options (kinda like a dropdown menu except I was too lazy to make an actual one)
class SwitchBox : public Label {
private:
	vector<string> options;
	uint curOpt;

public:
	SwitchBox(Size relSize = 1.f, vector<string> opts = {}, string curOption = string(), BCall call = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment alignment = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~SwitchBox() override = default;

	virtual void onClick(const ivec2& mPos, uint8 mBut) override;

	virtual bool selectable() const override;
	virtual void setText(const string& str) override;
	uint getCurOpt() const;
	void setCurOpt(uint id);
	void set(vector<string>&& opts, uint id);
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
	BCall ecall, ccall;
	uint limit;
	uint cpos;		// caret position
	int textOfs;	// text's horizontal offset
	bool chatMode;

public:
	LabelEdit(Size relSize = 1.f, string line = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, BCall retCall = nullptr, BCall cancCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, uint limit = UINT_MAX, bool chatMode = false, Alignment alignment = Alignment::left, bool showBG = true, GLuint bgTex = 0, const vec4& color = colorNormal, Layout* parent = nullptr, sizet id = SIZE_MAX);
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

	static bool kmodCtrl(uint16 mod);
	static bool kmodAlt(uint16 mod);
	static sizet cutLength(const char* str, sizet lim);
	void cutText();
	uint jumpCharB(uint i);
	uint jumpCharF(uint i);
	uint findWordStart();	// returns index of first character of word before cpos
	uint findWordEnd();		// returns index of character after last character of word after cpos
};

inline const string& LabelEdit::getOldText() const {
	return oldText;
}

inline bool LabelEdit::kmodCtrl(uint16 mod) {
	return mod & KMOD_CTRL && !(mod & (KMOD_SHIFT | KMOD_ALT));
}

inline bool LabelEdit::kmodAlt(uint16 mod) {
	return mod & KMOD_ALT && !(mod & (KMOD_SHIFT | KMOD_CTRL));
}
