#pragma once

#include "widgets.h"

// widget that can be navigated from/into (also handles switching navigation between UI and Objects)
class Navigator : public Widget {
public:
	using Widget::Widget;
	~Navigator() override = default;

	bool selectable() const override;
	void onNavSelect(Direction dir) override;
	virtual void navSelectFrom(int mid, Direction dir);
	virtual Interactable* findFirstSelectable() const;
	void navSelectOut(const vec3& pos, Direction dir);

private:
	Interactable* findSelectable(ivec2 entry) const;
};

// container for other widgets
class Layout : public Navigator, public Quad {
protected:
	const uint startInst = 0;	// index of first widget instance
	vector<Widget*> widgets;
	vector<ivec2> positions;	// widgets' positions. one element larger than widgets. last element is layout's size
	Size spaceSize;		// factor for space between widgets
	const bool vertical;	// how to arrange widgets

public:
	Layout(const Size& size = 1.f, vector<Widget*>&& children = vector<Widget*>(), bool vert = true, const Size& space = 0, uint widgetInstId = 0);
	~Layout() override;

	void draw() override;
	void tick(float dSec) override;
	void onResize() override;
	void postInit() override;
	bool setInstances() override;	// unlike a regular widget, a layout also uploads the instance buffer
	bool selectable() const override;
	void navSelectFrom(int mid, Direction dir) override;
	virtual void navSelectNext(uint id, int mid, Direction dir);
	Interactable* findFirstSelectable() const override;
	void updateTipTex() override;

	template <class T = Widget> T* getWidget(uint id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);
	void insertWidget(uint id, Widget* wgt);
	void deleteWidget(uint id);
	void setSpacing(const Size& space);
	bool getVertical() const;
	virtual ivec2 wgtPosition(uint id) const;
	virtual ivec2 wgtSize(uint id) const;
	virtual bool instanceVisible(uint qid) const;
protected:
	virtual ivec2 listSize() const;
	virtual void doWidgetsResize();

	void calculateWidgetPositions();
private:
	uint setWidgetsParent(uint id, uint qid);	// returns last instance id
	static bool deselectWidget(Widget* wgt);
	bool deselectWidgets() const;
	void navSelectWidget(uint id, int mid, Direction dir);
	void scanSequential(uint id, int mid, Direction dir);
	void scanPerpendicular(int mid, Direction dir);
};

template <class T>
T* Layout::getWidget(uint id) const {
	return static_cast<T*>(widgets[id]);
}

inline const vector<Widget*>& Layout::getWidgets() const {
	return widgets;
}

inline bool Layout::getVertical() const {
	return vertical;
}

inline ivec2 Layout::listSize() const {
	return positions.back() - sizeToPixRel(spaceSize);
}

// top level layout
class RootLayout : public Layout {
public:
	static constexpr vec4 uniformBgColor = vec4(0.f, 0.f, 0.f, 0.4f);

protected:
	const float topSpacingFac;
	Quad background;
	const vec4 bgColor;

private:
	static constexpr uint dimInst = 0;

public:
	RootLayout(const Size& size = 1.f, vector<Widget*>&& children = vector<Widget*>(), bool vert = true, const Size& space = 0, float topSpace = 0.f, const vec4& color = vec4(0.f), uint widgetInstId = dimInst + 1);
	~RootLayout() override = default;

	void onResize() override;
	void postInit() override;
	ivec2 position() const override;
	ivec2 size() const override;
	void setSize(const Size& size) override;
	Rect frame() const override;
protected:
	int pixTopSpacing() const;
};

// custom title bar
class TitleBar : public RootLayout {
public:
	TitleBar(vector<Widget*>&& children = vector<Widget*>(), bool vert = false, const Size& space = 0, const vec4& color = Widget::colorDark);
	~TitleBar() override = default;

	ivec2 position() const override;
	ivec2 size() const override;
};

inline TitleBar::TitleBar(vector<Widget*>&& children, bool vert, const Size& space, const vec4& color) :
	RootLayout(0.f, std::move(children), vert, space, 0.f, color)
{}

// layout with background with free position/size (shouldn't have a parent)
class Popup : public RootLayout {
public:
	enum class Type : uint8 {
		generic,
		config,
		settings,
		overlay
	};

	BCall kcall, ccall;		// gets called on enter/escape press
	Widget* defaultSelect;	// nav select to this if nothing selected (popup is not navigable if this is nullptr)
protected:
	const Size sizeY;		// use Widget's relSize as width

	static constexpr uint bgInst = 1;
private:
	static constexpr vec4 colorBackground = vec4(0.42f, 0.05f, 0.f, 1.f);

	const Type type;

public:
	Popup(const pair<Size, Size>& size = pair(1.f, 1.f), vector<Widget*>&& children = vector<Widget*>(), BCall okCall = nullptr, BCall cancelCall = nullptr, bool vert = true, const Size& space = 0, float topSpace = 0.f, Widget* firstSelect = nullptr, Type ctxType = Type::generic, const vec4& color = uniformBgColor);
	~Popup() override = default;

	void onResize() override;
	void postInit() override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	ivec2 position() const override;
	ivec2 size() const override;
	Type getType() const;
private:
	Rect getBackgroundRect() const;
};

inline Popup::Type Popup::getType() const {
	return type;
}

// popup that can be enabled or disabled
class Overlay : public Popup {
private:
	bool show;
	const bool interact;
	const pair<Size, Size> relPos;
	const vec4 boxColor;

public:
	Overlay(const pair<Size, Size>& pos = pair(0.f, 0.f), const pair<Size, Size>& size = pair(1.f, 1.f), vector<Widget*>&& children = vector<Widget*>(), BCall okCall = nullptr, BCall cancelCall = nullptr, bool vert = true, bool visible = false, bool interactive = true, const Size& space = 0, float topSpace = 0.f, const vec4& color = vec4(0.f));
	~Overlay() override = default;

	void draw() override;
	void onResize() override;
	void postInit() override;
	ivec2 position() const override;
	ivec2 size() const override;

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
	uvec2 visibleInsts = uvec2(0);

public:
	ScrollArea(const Size& size = 1.f, vector<Widget*>&& children = vector<Widget*>(), bool vert = true, const Size& space = 0);
	~ScrollArea() override = default;

	void tick(float dSec) override;
	void postInit() override;
	bool setInstances() override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(uint8 mBut) override;
	void onScroll(ivec2 wMov) override;
	void onNavSelect(Direction dir) override;
	void navSelectNext(uint id, int mid, Direction dir) override;
	void navSelectFrom(int mid, Direction dir) override;
	void onCancelCapture() override;

	Rect frame() const override;
	ivec2 wgtPosition(uint id) const override;
	ivec2 wgtSize(uint id) const override;
	bool instanceVisible(uint qid) const override;
protected:
	void doWidgetsResize() override;
private:
	void updateInstancesFull();
	void setVisibleInstances();
	void scrollToSelected();
	int wgtRPos(uint id) const;
	int wgtREnd(uint id) const;
};

inline ScrollArea::ScrollArea(const Size& size, vector<Widget*>&& children, bool vert, const Size& space) :
	Layout(size, std::move(children), vert, space, ScrollBar::numInstances)
{}

inline int ScrollArea::wgtRPos(uint id) const {
	return positions[id][vertical];
}

inline int ScrollArea::wgtREnd(uint id) const {
	return positions[id+1][vertical] - sizeToPixRel(spaceSize);
}
