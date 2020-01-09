#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
protected:
	vector<Widget*> widgets;
	vector<ivec2> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	int spacing;		// space between widgets
	bool vertical;		// how to arrange widgets

public:
	Layout(Size relSize = 1.f, vector<Widget*>&& children = {}, bool vertical = true, int spacing = 0, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Layout() override;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void onResize() override;
	virtual void postInit() override;

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
};

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
	static const vec4 defaultBgColor, uniformBgColor;

protected:
	const vec4 bgColor;

public:
	RootLayout(Size relSize = 1.f, vector<Widget*>&& children = {}, bool vertical = true, int spacing = 0, const vec4& bgColor = defaultBgColor);
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
	Size sizeY;			// use Widget's relSize as sizeX

	static constexpr int margin = 5;
	static const vec4 colorBackground;

public:
	Popup(const pair<Size, Size>& relSize = pair(1.f, 1.f), vector<Widget*>&& children = {}, BCall kcall = nullptr, BCall ccall = nullptr, bool vertical = true, int spacing = 0, const vec4& bgColor = uniformBgColor);
	virtual ~Popup() override = default;

	virtual void draw() const override;
	virtual ivec2 position() const override;
	virtual ivec2 size() const override;
};

// popup that can be enabled or disabled
class Overlay : public Popup {
private:
	pair<Size, Size> relPos;
	bool on;

public:
	Overlay(const pair<Size, Size>& relPos = pair(0.f, 0.f), const pair<Size, Size>& relSize = pair(1.f, 1.f), vector<Widget*>&& children = {}, BCall kcall = nullptr, BCall ccall = nullptr, bool vertical = true, int spacing = 0, const vec4& bgColor = defaultBgColor);
	virtual ~Overlay() override = default;

	virtual void draw() const override;
	virtual ivec2 position() const override;

	bool getOn() const;
	void setOn(bool yes);
};

inline bool Overlay::getOn() const {
	return on;
}

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
private:
	ScrollBar scroll;

public:
	ScrollArea(Size relSize = 1.f, vector<Widget*>&& children = {}, bool vertical = true, int spacing = 0, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~ScrollArea() override = default;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(const ivec2& wMov) override;
	virtual Rect frame() const override;

	virtual ivec2 wgtPosition(sizet id) const override;
	virtual ivec2 wgtSize(sizet id) const override;
	mvec2 visibleWidgets() const;

private:
	int wgtRPos(sizet id) const;
	int wgtREnd(sizet id) const;
};

inline int ScrollArea::wgtRPos(sizet id) const {
	return positions[id][vertical];
}

inline int ScrollArea::wgtREnd(sizet id) const {
	return positions[id+1][vertical] - spacing;
}
