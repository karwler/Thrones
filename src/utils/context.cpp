#include "engine/world.h"

Context::Context(const ivec2& mPos, const vector<string>& txts, CCall call, const ivec2& pos, int lineHeight, Widget* widget, int width) :
	size(width, int(txts.size()) * lineHeight),
	listSize(0, size.y),
	call(call),
	parent(widget),
	lineHeight(lineHeight),
	tex(World::scene()->blank())
{
	items.resize(txts.size());
	for (sizet i = 0; i < txts.size(); i++) {
		items[i] = pair(World::fonts()->render(txts[i], lineHeight), txts[i]);
		if (int w = items[i].first.getRes().x + Label::textMargin * 2 + ScrollBar::width; w > size.x)
			size.x = w;
	}
	position = ivec2(calcPos(pos.x, size.x, World::window()->screenView().x), calcPos(pos.y, size.y, World::window()->screenView().y));
	onMouseMove(mPos);
}

Context::~Context() {
	for (auto& [tex, str] : items)
		tex.close();
}

void Context::draw() const {
	Rect rct = rect();
	if (Quad::draw(rct, Widget::colorDark, tex, -1.f); selected < items.size())
		Quad::draw(Rect(rct.x, rct.y + itemPos(selected), size.x, lineHeight), rct, Widget::colorDimmed, tex, -1.f);

	mvec2 i = listSize.y <= size.y ? mvec2(0, items.size()) : mvec2(scroll.listPos.y / lineHeight, (size.y + scroll.listPos.y + lineHeight - 1) / lineHeight);
	for (ivec2 pos(rct.x + Label::textMargin, rct.y + int(i.x) * lineHeight - scroll.listPos.y); i.x < i.y; i.x++, pos.y += lineHeight)
		Quad::draw(Rect(pos, items[i.x].first.getRes()), rct, vec4(1.f), items[i.x].first.getID(), -1.f);
	scroll.draw(listSize, position, size, true, -1.f);
}

void Context::onClick(const ivec2& mPos, uint8 mBut) {
	if (onMouseMove(mPos); mBut == SDL_BUTTON_LEFT) {
		if (ComboBox* cb = dynamic_cast<ComboBox*>(parent))
			cb->setText(items[selected].second);
		World::prun(call, selected, items[selected].second);
	}
	World::scene()->setContext(nullptr);
}

void Context::tick(float dSec) {
	scroll.tick(dSec, listSize, size);
}

void Context::onHold(const ivec2& mPos, uint8 mBut) {
	scroll.hold(mPos, mBut, this, listSize, position, size, true);
}

void Context::onDrag(const ivec2& mPos, const ivec2& mMov) {
	scroll.drag(mPos, mMov, listSize, position, size, true);
}

void Context::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, true);
}

void Context::onScroll(const ivec2& wMov) {
	scroll.scroll(wMov, listSize, size, true);
}

void Context::onKeypress(const SDL_Keysym& key) {
	switch (key.scancode) {
	case SDL_SCANCODE_UP: case SDL_SCANCODE_LEFT:
		if (selected) {
			selected--;
			if (int y = itemPos(selected); y < 0)
				scroll.moveListPos(ivec2(0, y), listSize, size);
		}
		break;
	case SDL_SCANCODE_DOWN: case SDL_SCANCODE_RIGHT:
		if (selected < items.size() - 1) {
			selected++;
			if (int y = itemPos(selected) + lineHeight; y > size.y)
				scroll.moveListPos(ivec2(0, y - size.y), listSize, size);
		}
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		if (ivec2 mPos = mousePos(); rect().contain(mPos))
			onClick(mPos, SDL_BUTTON_LEFT);
		else
			World::scene()->setContext(nullptr);
		break;
	case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_TAB: case SDL_SCANCODE_AC_BACK:
		World::scene()->setContext(nullptr);
	}
}

int Context::calcPos(int pos, int& size, int limit) {
	if (size >= limit) {
		size = limit;
		return 0;
	}
	return pos + size <= limit ? pos : limit - size;
}
