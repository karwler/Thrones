#pragma once

#include "widgets.h"

// widget that can be navigated from/into (also handles switching navigation between UI and Objects)
class Navigator : public Widget {
public:
	using Widget::Widget;
	virtual ~Navigator() override = default;

	virtual bool selectable() const override;
	virtual void onNavSelect(Direction dir) override;
	virtual void navSelectFrom(int mid, Direction dir);
	virtual Interactable* findFirstSelectable() const;
	void navSelectOut(const vec3& pos, Direction dir);

private:
	Interactable* findSelectable(const ivec2& entry) const;
};

// container for other widgets
class Layout : public Navigator {
protected:
	vector<Widget*> widgets;
	vector<ivec2> positions;	// widgets' positions. one element larger than widgets. last element is layout's size
	int spacing;		// space between widgets
	bool vertical;		// how to arrange widgets

public:
	Layout(Size size = 1.f, vector<Widget*>&& children = vector<Widget*>(), bool vert = true, int space = 0);
	virtual ~Layout() override;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void onResize() override;
	virtual void postInit() override;
	virtual bool selectable() const override;
	virtual void navSelectFrom(int mid, Direction dir) override;
	virtual void navSelectNext(sizet id, int mid, Direction dir);
	virtual Interactable* findFirstSelectable() const override;

	Widget* getWidget(sizet id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);
	void insertWidget(sizet id, Widget* wgt);
	void deleteWidget(sizet id);
	void setSpacing(int space);
	bool getVertical() const;
	virtual ivec2 wgtPosition(sizet id) const;
	virtual ivec2 wgtSize(sizet id) const;

protected:
	void initWidgets(vector<Widget*>&& wgts);
	void reinitWidgets(sizet id);
	virtual ivec2 listSize() const;

	void navSelectWidget(sizet id, int mid, Direction dir);
private:
	void scanSequential(sizet id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);
	void clearWidgets();
};

inline Layout::~Layout() {
	clearWidgets();
}

inline Widget* Layout::getWidget(sizet id) const {
	return widgets[id];
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline bool Layout::getVertical() const {
	return vertical;
}

// top level layout
class RootLayout : public Layout {
public:
	static constexpr vec4 uniformBgColor = { 0.f, 0.f, 0.f, 0.4f };

protected:
	vec4 bgColor;

public:
	RootLayout(Size size = 1.f, vector<Widget*>&& children = vector<Widget*>(), bool vert = true, int space = 0, const vec4& color = vec4(0.f));
	virtual ~RootLayout() override = default;

	virtual void draw() const override;
	virtual ivec2 position() const override;
	virtual ivec2 size() const override;
	virtual Rect frame() const override;
};

// layout with background with free position/size (shouldn't have a parent)
class Popup : public RootLayout {
public:
	BCall kcall, ccall;	// gets called on enter/escape press
protected:
	Size sizeY;			// use Widget's relSize as width

	static constexpr int margin = 5;
	static constexpr vec4 colorBackground = { 0.42f, 0.05f, 0.f, 1.f };

public:
	Popup(const pair<Size, Size>& size = pair(1.f, 1.f), vector<Widget*>&& children = vector<Widget*>(), BCall okCall = nullptr, BCall cancelCall = nullptr, bool vert = true, int space = 0, const vec4& color = uniformBgColor);
	virtual ~Popup() override = default;

	virtual void draw() const override;
	virtual ivec2 position() const override;
	virtual ivec2 size() const override;
};

// popup that can be enabled or disabled
class Overlay : public Popup {
private:
	pair<Size, Size> relPos;
	vec4 boxColor;
	bool show, interact;

public:
	Overlay(const pair<Size, Size>& pos = pair(0.f, 0.f), const pair<Size, Size>& size = pair(1.f, 1.f), vector<Widget*>&& children = vector<Widget*>(), BCall okCall = nullptr, BCall cancelCall = nullptr, bool vert = true, bool visible = false, bool interactive = true, int space = 0, const vec4& color = vec4(0.f));
	virtual ~Overlay() override = default;

	virtual void draw() const override;
	virtual ivec2 position() const override;

	bool getShow() const;
	void setShow(bool yes);
	bool canInteract() const;
};

inline bool Overlay::getShow() const {
	return show;
}

inline bool Overlay::canInteract() const {
	return show && interact;
}

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
private:
	ScrollBar scroll;

public:
	using Layout::Layout;
	virtual ~ScrollArea() override = default;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(const ivec2& wMov) override;
	virtual void onNavSelect(Direction dir) override;
	virtual void navSelectNext(sizet id, int mid, Direction dir) override;
	virtual void navSelectFrom(int mid, Direction dir) override;

	virtual Rect frame() const override;
	virtual ivec2 wgtPosition(sizet id) const override;
	virtual ivec2 wgtSize(sizet id) const override;
	mvec2 visibleWidgets() const;

private:
	void scrollToSelected();
	void scrollToWidgetPos(sizet id);
	void scrollToWidgetEnd(sizet id);
	int wgtRPos(sizet id) const;
	int wgtREnd(sizet id) const;
};

inline void ScrollArea::scrollToWidgetPos(sizet id) {
	scroll.setListPos(vertical ? ivec2(scroll.listPos.x, wgtRPos(id)) : ivec2(wgtRPos(id), scroll.listPos.y), listSize(), size());
}

inline void ScrollArea::scrollToWidgetEnd(sizet id) {
	scroll.setListPos(vertical ? ivec2(scroll.listPos.x, wgtREnd(id) - size().y) : ivec2(wgtREnd(id) - size().x, scroll.listPos.y), listSize(), size());
}

inline int ScrollArea::wgtRPos(sizet id) const {
	return positions[id][vertical];
}

inline int ScrollArea::wgtREnd(sizet id) const {
	return positions[id+1][vertical] - spacing;
}
