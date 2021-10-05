#pragma once

#include "widgets.h"

// context menu for right click or combo box
class Context : public Interactable {
private:
	ivec2 position, size;
	ivec2 listSize;
	CCall call;
	Interactable* parent;	// the instance that got clicked (nullptr in case of blank right click)
	vector<pair<Texture, string>> items;
	ScrollBar scroll;
	uint selected;
	int lineHeight;

public:
	Context(ivec2 mPos, const vector<string>& txts, CCall cancelCall, ivec2 pos, int lineH, Widget* owner = nullptr, int width = 0);
	~Context() override;

	void onMouseMove(ivec2 mPos);
	void draw() const;
	void tick(float dSec) override;
	void onClick(ivec2 mPos, uint8 mBut) override;
	void onHold(ivec2 mPos, uint8 mBut) override;
	void onDrag(ivec2 mPos, ivec2 mMov) override;
	void onUndrag(uint8 mBut) override;
	void onScroll(ivec2 wMov) override;
	void onNavSelect(Direction dir) override;
	void onCancelCapture() override;

	void confirm();
	Rect rect() const;
	Interactable* getParent() const;

private:
	int itemPos(sizet id) const;
	static int calcPos(int pos, int& siz, int limit);
};

inline void Context::onMouseMove(ivec2 mPos) {
	selected = rect().contain(mPos) ? (mPos.y - position.y + scroll.listPos.y) / lineHeight : UINT_MAX;
}

inline Rect Context::rect() const {
	return Rect(position, size);
}

inline Interactable* Context::getParent() const {
	return parent;
}

inline int Context::itemPos(sizet id) const {
	return id * lineHeight - scroll.listPos.y;
}
