#pragma once

#include "widgets.h"

// container for other widgets
class Layout : public Widget {
public:
	static constexpr int defaultItemSpacing = 5;

protected:
	vector<Widget*> widgets;
	vector<vec2i> positions;	// widgets' positions. one element larger than wgts. last element is layout's size
	int spacing;				// space between widgets
	bool vertical;				// how to arrange widgets

public:
	Layout(Size relSize = 1.f, vector<Widget*> children = {}, bool vertical = true, int spacing = defaultItemSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~Layout() override;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void onResize() override;
	virtual void postInit() override;

	Widget* getWidget(sizet id) const;
	const vector<Widget*>& getWidgets() const;
	void setWidgets(vector<Widget*>&& wgts);
	void deleteWidget(sizet id);
	bool getVertical() const;
	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual Rect frame() const override;
	virtual vec2i wgtPosition(sizet id) const;
	virtual vec2i wgtSize(sizet id) const;

protected:
	void initWidgets(vector<Widget*>&& wgts);
	virtual vec2i listSize() const;
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

// layout with background with free position/size (shouldn't have a parent)
class Popup : public Layout {
public:
	BCall kcall, ccall;	// gets called on enter/escape press
private:
	Size sizeY;			// use Widget's relSize as sizeX

	static constexpr int margin = defaultItemSpacing;
	static const vec4 colorDim;
	static const vec4 colorBackground;

public:
	Popup(const cvec2<Size>& relSize = 1.f, vector<Widget*> children = {}, BCall kcall = nullptr, BCall ccall = nullptr, bool vertical = true, int spacing = defaultItemSpacing);
	virtual ~Popup() override = default;

	virtual void draw() const override;

	virtual vec2i position() const override;
	virtual vec2i size() const override;
	virtual Rect frame() const override;
};

// places widgets vertically through which the user can scroll (DON"T PUT SCROLL AREAS INTO OTHER SCROLL AREAS)
class ScrollArea : public Layout {
protected:
	bool draggingSlider;
	vec2i listPos;
private:
	vec2f motion;			// how much the list scrolls over time
	int diffSliderMouse;	// space between slider and mouse position

	static constexpr int barWidth = 10;
	static constexpr float scrollThrottle = 10.f;

public:
	ScrollArea(Size relSize = 1.f, vector<Widget*> children = {}, bool vertical = true, int spacing = defaultItemSpacing, Layout* parent = nullptr, sizet id = SIZE_MAX);
	virtual ~ScrollArea() override = default;

	virtual void draw() const override;
	virtual void tick(float dSec) override;
	virtual void postInit() override;
	virtual void onHold(vec2i mPos, uint8 mBut) override;
	virtual void onDrag(vec2i mPos, vec2i mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(vec2i wMov) override;

	virtual Rect frame() const override;
	virtual vec2i wgtPosition(sizet id) const override;
	virtual vec2i wgtSize(sizet id) const override;
	Rect barRect() const;
	Rect sliderRect() const;
	vec2t visibleWidgets() const;
	void moveListPos(vec2i mov);

protected:
	virtual vec2i listLim() const;	// max list position
	int wgtRPos(sizet id) const;
	int wgtREnd(sizet id) const;

private:
	void setSlider(int spos);
	int barSize() const;	// returns 0 if slider isn't needed
	int sliderSize() const;
	int sliderPos() const;
	int sliderLim() const;	// max slider position

	static void throttleMotion(float& mov, float dSec);
};

inline int ScrollArea::wgtRPos(sizet id) const {
	return positions[id][vertical];
}

inline int ScrollArea::wgtREnd(sizet id) const {
	return positions[id+1][vertical] - spacing;
}

inline int ScrollArea::barSize() const {
	return listSize()[vertical] > size()[vertical] ? barWidth : 0;
}

inline int ScrollArea::sliderPos() const {
	return listSize()[vertical] > size()[vertical] ? position()[vertical] + listPos[vertical] * sliderLim() / listLim()[vertical] : position()[vertical];
}

inline int ScrollArea::sliderLim() const {
	return size()[vertical] - sliderSize();
}
