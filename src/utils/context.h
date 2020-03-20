#pragma once

#include "utils/widgets.h"

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
	Context(const ivec2& mPos, const vector<string>& txts, CCall call, const ivec2& pos, int lineHeight, Widget* parent = nullptr, int width = 0);
	virtual ~Context() override;

	void draw() const;
	void onMouseMove(const ivec2& mPos);
	virtual void tick(float dSec) override;
	virtual void onClick(const ivec2& mPos, uint8 mBut) override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onScroll(const ivec2& wMov) override;
	virtual void onKeypress(const SDL_Keysym& key) override;

	Rect rect() const;
	Interactable* getParent() const;

private:
	int itemPos(sizet id) const;
	int calcPos(int pos, int& size, int limit);
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
