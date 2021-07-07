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
	GLuint tex;

public:
	Context(const ivec2& mPos, const vector<string>& txts, CCall cancelCall, const ivec2& pos, int lineH, Widget* owner = nullptr, int width = 0);
	~Context() final;

	void onMouseMove(const ivec2& mPos);
	void draw() const;
	void tick(float dSec) final;
	void onClick(const ivec2& mPos, uint8 mBut) final;
	void onHold(const ivec2& mPos, uint8 mBut) final;
	void onDrag(const ivec2& mPos, const ivec2& mMov) final;
	void onUndrag(uint8 mBut) final;
	void onScroll(const ivec2& wMov) final;
	void onNavSelect(Direction dir) final;
	void onCancelCapture() final;

	void confirm();
	Rect rect() const;
	Interactable* getParent() const;

private:
	int itemPos(sizet id) const;
	static int calcPos(int pos, int& siz, int limit);
};

inline void Context::onMouseMove(const ivec2& mPos) {
	selected = rect().contain(mPos) ? uint((mPos.y - position.y + scroll.listPos.y) / lineHeight) : UINT_MAX;
}

inline Rect Context::rect() const {
	return Rect(position, size);
}

inline Interactable* Context::getParent() const {
	return parent;
}

inline int Context::itemPos(sizet id) const {
	return int(id) * lineHeight - scroll.listPos.y;
}
