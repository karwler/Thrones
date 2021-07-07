#pragma once

#include "settings.h"
#include "utils.h"

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

constexpr Size::Size(int pixels) :
	pix(pixels),
	usePix(true)
{}

constexpr Size::Size(float percent) :
	prc(percent),
	usePix(false)
{}

// scroll information
class ScrollBar {
public:
	static constexpr int width = 10;

	ivec2 listPos = { 0, 0 };
private:
	vec2 motion = { 0.f, 0.f };	// how much the list scrolls over time
	int diffSliderMouse = 0;	// space between slider and mouse position
	bool draggingSlider = false;

	static constexpr float throttle = 10.f;

public:
	void draw(const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert, float z = 0.f) const;
	void draw(const Rect& frame, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert, float z = 0.f) const;
	void tick(float dSec, const ivec2& listSize, const ivec2& size);
	void hold(const ivec2& mPos, uint8 mBut, Interactable* wgt, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	void drag(const ivec2& mPos, const ivec2& mMov, const ivec2& listSize, const ivec2& pos, const ivec2& size, bool vert);
	void undrag(uint8 mBut, bool vert);
	void cancelDrag();
	void scroll(const ivec2& wMov, const ivec2& listSize, const ivec2& size, bool vert);

	static ivec2 listLim(const ivec2& listSize, const ivec2& size);	// max list position
	void setListPos(const ivec2& pos, const ivec2& listSize, const ivec2& size);
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

inline void ScrollBar::setListPos(const ivec2& pos, const ivec2& listSize, const ivec2& size) {
	listPos = glm::clamp(pos, ivec2(0), listLim(listSize, size));
}

inline void ScrollBar::moveListPos(const ivec2& mov, const ivec2& listSize, const ivec2& size) {
	listPos = glm::clamp(listPos + mov, ivec2(0), listLim(listSize, size));
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
	static constexpr float vertices[corners * stride] = {
		0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,
		1.f, 1.f
	};

	GLuint vao, vbo;

public:
	Quad();
	~Quad();

	GLuint getVao() const;
	static void draw(const Rect& rect, const vec4& color, GLuint tex, float z = 0.f);
	static void draw(const Rect& rect, Rect frame, const vec4& color, GLuint tex, float z = 0.f);
	static void draw(const Rect& rect, const vec4& uvrect, const vec4& color, GLuint tex, float z = 0.f);
};

inline GLuint Quad::getVao() const {
	return vao;
}

inline void Quad::draw(const Rect& rect, const vec4& color, GLuint tex, float z) {
	draw(rect, vec4(0.f, 0.f, 1.f, 1.f), color, tex, z);
}

inline void Quad::draw(const Rect& rect, Rect frame, const vec4& color, GLuint tex, float z) {
	frame = rect.intersect(frame);
	draw(frame, vec4(float(frame.x - rect.x) / float(rect.w), float(frame.y - rect.y) / float(rect.h), float(frame.w) / float(rect.w), float(frame.h) / float(rect.h)), color, tex, z);
}

// can be used as spacer
class Widget : public Interactable {
public:
	static constexpr vec4 colorNormal = { 0.5f, 0.1f, 0.f, 1.f };
	static constexpr vec4 colorDimmed = { 0.4f, 0.05f, 0.f, 1.f };
	static constexpr vec4 colorDark = { 0.3f, 0.f, 0.f, 1.f };
	static constexpr vec4 colorLight = { 1.f, 0.757f, 0.145f, 1.f };
	static constexpr vec4 colorTooltip = { 0.3f, 0.f, 0.f, 0.7f };

protected:
	Size relSize;				// size relative to parent's parameters
	Layout* parent = nullptr;	// every widget that isn't a Layout should have a parent
	sizet index = SIZE_MAX;		// this widget's id in parent's widget list

public:
	Widget(Size size = 1.f);
	~Widget() override = default;

	virtual void draw() const {}
	virtual void onResize() {}
	virtual void postInit() {}		// gets called after parent is set and all set up
	virtual ivec2 position() const;
	virtual ivec2 size() const;
	virtual void setSize(const Size& size);
	virtual Rect frame() const;		// the rectangle to restrain a widget's visibility (in Widget it returns the parent's frame and if in Layout, it returns a frame for it's children)
	virtual bool selectable() const;

	const Size& getSize() const;
	sizet getIndex() const;
	Layout* getParent() const;
	void setParent(Layout* pnt, sizet id);
	Rect rect() const;			// the rectangle that is the widget
	ivec2 center() const;
};

inline Widget::Widget(Size size) :
	relSize(size)
{}

inline const Size& Widget::getSize() const {
	return relSize;
}

inline sizet Widget::getIndex() const {
	return index;
}

inline Layout* Widget::getParent() const {
	return parent;
}

inline Rect Widget::rect() const {
	return Rect(position(), size());
}

inline ivec2 Widget::center() const {
	return position() + size() / 2;
}

// clickable widget with function calls for left and right click (it's rect is drawn so you can use it like a spacer with color)
class Button : public Widget {
public:
	BCall lcall, rcall;
	vec4 color;
protected:
	vec4 dimFactor;
	Texture tipTex;	// memory is handled by the button
	GLuint bgTex;

private:
	static constexpr float selectFactor = 1.2f;
	static constexpr ivec2 tooltipMargin = { 4, 1 };
	static constexpr int cursorMargin = 2;

public:
	Button(Size size = 1.f, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint tex = 0, const vec4& clr = colorNormal);
	~Button() override;

	void draw() const override;
	void onClick(const ivec2& mPos, uint8 mBut) override;
	void onHover() override;
	void onUnhover() override;
	void onNavSelect(Direction dir) override;
	bool selectable() const override;

	void drawTooltip() const;
	void setTooltip(const Texture& tooltip);
	void setDim(float factor);
};

inline void Button::setDim(float factor) {
	dimFactor = vec4(factor, factor, factor, dimFactor.a);
}

// if you don't know what a checkbox is then I don't know what to tell ya
class CheckBox : public Button {
public:
	bool on;

	CheckBox(Size size = 1.f, bool checked = false, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint tex = 0, const vec4& clr = colorNormal);
	~CheckBox() final = default;

	void draw() const final;
	void onClick(const ivec2& mPos, uint8 mBut) final;

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
	static constexpr int barMarginFactor = 4;
	static constexpr int sliderWidthFactor = 3;

private:
	int value, vmin, vmax;
	int keyStep;
	int oldVal;
	int diffSliderMouse = 0;

public:
	Slider(Size size = 1.f, int value = 0, int minimum = 0, int maximum = 255, int navStep = 1, BCall finishCall = nullptr, BCall updateCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, GLuint tex = 0, const vec4& clr = colorNormal);
	~Slider() final = default;

	void draw() const final;
	void onClick(const ivec2&, uint8) final {}
	void onHold(const ivec2& mPos, uint8 mBut) final;
	void onDrag(const ivec2& mPos, const ivec2& mMov) final;
	void onUndrag(uint8 mBut) final;
	void onKeyDown(const SDL_KeyboardEvent& key) final;
	void onJButtonDown(uint8 but) final;
	void onJHatDown(uint8 hat, uint8 val) final;
	void onJAxisDown(uint8 axis, bool positive) final;
	void onGButtonDown(SDL_GameControllerButton but) final;
	void onGAxisDown(SDL_GameControllerAxis axis, bool positive) final;
	void onCancelCapture() final;

	int getValue() const;
	int getMin() const;
	int getMax() const;
	void setVal(int value);
	void set(int val, int minimum, int maximum);

	Rect barRect() const;
	Rect sliderRect() const;

private:
	void onInput(Binding::Type bind);
	void setSlider(int xpos);
	int sliderPos() const;
	int sliderLim() const;
};

inline int Slider::getValue() const {
	return value;
}

inline int Slider::getMin() const {
	return vmin;
}

inline int Slider::getMax() const {
	return vmax;
}

inline void Slider::setVal(int val) {
	value = std::clamp(val, vmin, vmax);
}

// it's a little ass backwards but labels (aka a line of text) are buttons
class Label : public Button {
public:
	enum class Alignment : uint8 {
		left,
		center,
		right,
	};
	static constexpr int textMarginFactor = 6;

protected:
	string text;
	Texture textTex;	// managed by Label
	Alignment halign;	// text alignment
	bool showBG;

public:
	Label(Size size = 1.f, string line = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment align = Alignment::left, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal);
	~Label() override;

	void draw() const override;
	void onResize() override;
	void postInit() override;

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

// multi-line scrollable label
class TextBox : public Label {
private:
	bool alignTop;
	ScrollBar scroll;
	int lineHeight;
	uint16 lineLim, lineCnt;

public:
	TextBox(Size size = 1.f, int lineH = 0, string lines = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, uint16 lineL = UINT16_MAX, bool stickTop = true, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal);
	~TextBox() final = default;

	void draw() const final;
	void tick(float dSec) final;
	void postInit() final;
	void onHold(const ivec2& mPos, uint8 mBut) final;
	void onDrag(const ivec2& mPos, const ivec2& mMov) final;
	void onUndrag(uint8 mBut) final;
	void onHover() final {}
	void onUnhover() final {}
	void onScroll(const ivec2& wMov) final;
	void onCancelCapture() final;

	bool selectable() const final;
	void setText(string&& str) final;
	void setText(const string &str) final;
	void addLine(const string& line);
	string moveText();
private:
	ivec2 textPos() const final;
	void updateTextTex() final;
	void updateListPos();
	void resetListPos();
	void cutLines();
};

inline void TextBox::resetListPos() {
	scroll.listPos = alignTop ? ivec2(0) : scroll.listLim(textTex.getRes(), size());
}

// a label with selection highlight
class Icon : public Label {
public:
	bool selected;
	BCall hcall;

private:
	static constexpr int outlineFactor = 16;

public:
	Icon(Size size = 1.f, string line = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, BCall holdCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment align = Alignment::left, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal, bool select = false);
	~Icon() final = default;

	void draw() const final;
	void onHold(const ivec2& mPos, uint8 mBut) final;
	bool selectable() const final;
};

// for switching between multiple options (kinda like a drop-down menu except I was too lazy to make an actual one)
class ComboBox : public Label {
private:
	vector<string> options;
	CCall ocall;

public:
	ComboBox(Size size = 1.f, string curOpt = string(), vector<string> opts = vector<string>(), CCall optCall = nullptr, BCall leftCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment align = Alignment::left, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal);
	~ComboBox() final = default;

	void onClick(const ivec2& mPos, uint8 mBut) final;

	bool selectable() const final;
	void setText(string&& str) final;
	void setText(const string& str) final;
	sizet getCurOpt() const;
	void setCurOpt(sizet id);
	void set(vector<string>&& opts, sizet id);
	void set(vector<string>&& opts, const string& name);
};

inline sizet ComboBox::getCurOpt() const {
	return sizet(std::find(options.begin(), options.end(), text) - options.begin());
}

inline void ComboBox::setCurOpt(sizet id) {
	Label::setText(id < options.size() ? options[id] : string());
}

// for editing a line of text (ignores Label's align), (calls Button's lcall on text confirm instead of on click and ecall when enter is pressed)
class LabelEdit : public Label {
public:
	static constexpr int caretWidth = 4;

private:
	uint16 cpos = 0;	// caret position
	string oldText;
	BCall ecall, ccall;
	bool chatMode;
	uint16 limit;
	int textOfs = 0;	// text's horizontal offset

public:
	LabelEdit(Size size = 1.f, string line = string(), BCall leftCall = nullptr, BCall rightCall = nullptr, BCall retCall = nullptr, BCall cancCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, uint16 lim = UINT16_MAX, bool isChat = false, Alignment align = Alignment::left, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal);
	~LabelEdit() final = default;

	void drawTop() const final;
	void onClick(const ivec2& mPos, uint8 mBut) final;
	void onKeyDown(const SDL_KeyboardEvent& key) final;
	void onJButtonDown(uint8 but) final;
	void onJHatDown(uint8 hat, uint8 val) final;
	void onJAxisDown(uint8 axis, bool positive) final;
	void onGButtonDown(SDL_GameControllerButton but) final;
	void onGAxisDown(SDL_GameControllerAxis axis, bool positive) final;
	void onText(const char* str) final;
	void onCancelCapture() final;

	bool selectable() const final;
	const string& getOldText() const;
	void setText(string&& str) final;
	void setText(const string& str) final;

	void confirm();
	void cancel();

protected:
	ivec2 textPos() const final;
private:
	void onInput(Binding::Type bind, uint16 mod = 0, bool joypad = true);
	void onTextReset();
	int caretPos() const;	// caret's relative x position
	void setCPos(uint16 cp);

	static bool kmodCtrl(uint16 mod);
	static bool kmodAlt(uint16 mod);
	static sizet cutLength(const char* str, sizet lim);
	void cutText();
	uint16 jumpCharB(uint16 i);
	uint16 jumpCharF(uint16 i);
	uint16 findWordStart();	// returns index of first character of word before cpos
	uint16 findWordEnd();		// returns index of character after last character of word after cpos
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

// for getting a key/button/axis
class KeyGetter : public Label {
public:
	Binding::Accept accept;	// what kind of binding is being accepted
	Binding::Type bind;		// binding index
	sizet kid;				// key id

	KeyGetter(Size size = 1.f, Binding::Accept atype = Binding::Accept::keyboard, Binding::Type binding = Binding::Type(-1), sizet keyId = SIZE_MAX, BCall exitCall = nullptr, BCall rightCall = nullptr, const Texture& tooltip = Texture(), float dim = 1.f, Alignment align = Alignment::left, bool bg = true, GLuint tex = 0, const vec4& clr = colorNormal);
	~KeyGetter() final = default;

	void onClick(const ivec2& mPos, uint8 mBut) final;
	void onKeyDown(const SDL_KeyboardEvent& key) final;
	void onJButtonDown(uint8 but) final;
	void onJHatDown(uint8 hat, uint8 val) final;
	void onJAxisDown(uint8 axis, bool positive) final;
	void onGButtonDown(SDL_GameControllerButton but) final;
	void onGAxisDown(SDL_GameControllerAxis axis, bool positive) final;
	void onCancelCapture() final;
	bool selectable() const final;

private:
	template <class T> void setBinding(Binding::Accept expect, T key);
	static string bindingText(Binding::Accept accept, Binding::Type bind, sizet kid);
};
