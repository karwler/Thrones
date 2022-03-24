#include "engine/scene.h"
#include "engine/world.h"

// INTERACTABLE

void Interactable::onNavSelect(Direction) {
	World::scene()->updateSelect(World::scene()->getFirstSelect());
}

// CONTEXT

Context::Context(ivec2 mPos, const vector<string>& txts, CCall cancelCall, ivec2 pos, int lineH, Widget* owner, int width) :
	lineHeight(lineH),
	size(width, txts.size() * lineH),
	listSize(0, size.y),
	call(cancelCall),
	parent(owner)
{
	items.resize(txts.size());
	for (sizet i = 0; i < txts.size(); ++i) {
		items[i] = pair(World::scene()->getTexCol()->insert(World::fonts()->render(txts[i].c_str(), lineH)), txts[i]);
		if (int w = items[i].first.rct.w + lineHeight / Label::textMarginFactor * 2 + *World::pgui()->getSize(GuiGen::SizeRef::scrollBarWidth); w > size.x)
			size.x = w;
	}
	ivec2 res = World::window()->getScreenView();
	position = ivec2(calcPos(pos.x, size.x, res.x), calcPos(pos.y, size.y, res.y));
	selected = getSelected(mPos);

	initBuffer();
	updateInstances();
}

Context::~Context() {
	for (auto& [itx, str] : items)
		World::scene()->getTexCol()->erase(itx);
	freeBuffer();
}

void Context::updateInstances() {
	uvec2 vis = listSize.y <= size.y ? uvec2(0, items.size()) : uvec2(scroll.listPos.y / lineHeight, (size.y + scroll.listPos.y + lineHeight - 1) / lineHeight);
	instanceData.resize(vis.y - vis.x + 2 + ScrollBar::numInstances);

	Rect rct = rect();
	setInstance(bgInst, rct, Widget::colorDark, TextureCol::blank, -1.f);
	setSelectedInstance();

	uint id = selInst + 1;
	for (ivec2 pos(rct.x + lineHeight / Label::textMarginFactor, rct.y + vis.x * lineHeight - scroll.listPos.y); vis.x < vis.y; ++vis.x, ++id, pos.y += lineHeight)
		setInstance(id, Rect(pos, items[vis.x].first.rct.size()), rct, vec4(1.f), items[vis.x].first, -1.f);
	scroll.setInstances(this, id, listSize, position, size, true, -1.f);
	updateInstanceData(0, instanceData.size());
}

void Context::setSelectedInstance() {
	if (selected < items.size()) {
		Rect rct = rect();
		setInstance(selInst, Rect(rct.x, rct.y + itemPos(selected), size.x, lineHeight), rct, Widget::colorDimmed, TextureCol::blank, -1.f);
	} else
		setInstance(selInst);
}

void Context::onClick(ivec2 mPos, uint8 mBut) {
	if (onMouseMove(mPos); mBut == SDL_BUTTON_LEFT) {
		if (ComboBox* cb = dynamic_cast<ComboBox*>(parent))
			cb->setText(items[selected].second);
		World::prun(call, selected, items[selected].second);
	}
	World::scene()->setContext(nullptr);
}

void Context::onMouseMove(ivec2 mPos) {
	if (uint sel = getSelected(mPos); sel != selected) {
		selected = sel;
		setSelectedInstance();
		updateInstance(selInst);
	}
}

void Context::tick(float dSec) {
	if (scroll.tick(dSec, listSize, size)) {
		selected = getSelected(World::window()->mousePos());
		updateInstances();
	}
}

void Context::onHold(ivec2 mPos, uint8 mBut) {
	if (scroll.hold(mPos, mBut, this, listSize, position, size, true)) {
		selected = getSelected(mPos);
		updateInstances();
	}
}

void Context::onDrag(ivec2 mPos, ivec2 mMov) {
	scroll.drag(mPos, mMov, listSize, position, size, true);
	selected = getSelected(mPos);
	updateInstances();
}

void Context::onUndrag(uint8 mBut) {
	scroll.undrag(mBut, true);
}

void Context::onScroll(ivec2 wMov) {
	scroll.scroll(wMov, listSize, size, true);
	selected = getSelected(World::window()->mousePos());
	updateInstances();
}

void Context::onNavSelect(Direction dir) {
	if (dir.positive()) {
		selected = selected < items.size() - 1 ? selected + 1 : 0;
		if (int y = itemPos(selected) + lineHeight; y > size.y)
			scroll.moveListPos(ivec2(0, y - size.y), listSize, size);
	} else {
		selected = selected ? selected - 1 : items.size() - 1;
		if (int y = itemPos(selected); y < 0)
			scroll.moveListPos(ivec2(0, y), listSize, size);
	}
	updateInstances();
}

void Context::onCancelCapture() {
	scroll.cancelDrag();
}

void Context::confirm() {
	if (selected < items.size())
		onClick(ivec2(position.x, position.y + itemPos(selected)), SDL_BUTTON_LEFT);
	else
		World::scene()->setContext(nullptr);
}

int Context::calcPos(int pos, int& siz, int limit) {
	if (siz >= limit) {
		siz = limit;
		return 0;
	}
	return pos + siz <= limit ? pos : limit - siz;
}
