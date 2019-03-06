#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Widget* widget;
	ScrollArea* area;
	vec2i mPos;

	ClickStamp(Widget* widget = nullptr, ScrollArea* area = nullptr, const vec2i& mPos = 0);
};

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	vec2i mouseMove;
	Widget* select;		// currently selected widget
	Widget* capture;	// either pointer to widget currently hogging all keyboard input or ScrollArea whichs slider is currently being dragged. nullptr if nothing is being captured or dragged
private:
	Camera camera;
	vector<Object*> objects;
	uptr<Layout> layout;
	uptr<Popup> popup;
	array<ClickStamp, SDL_BUTTON_X2+1> stamps;	// data about last mouse click (indexes are mouse button numbers

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;
	static constexpr GLbitfield clearSet = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

public:
	Scene();

	void draw();
	void tick(float dSec);
	void onResize();
	void onKeyDown(const SDL_KeyboardEvent& key);
	void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	void onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	void onMouseUp(const vec2i& mPos, uint8 mBut);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();
	void onText(const string& str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary

	void setObjects(const vector<Object*>& objs);
	void resetLayouts();
	Layout* getLayout();
	Popup* getPopup();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);
	const vec2i& getMouseMove() const;

	sizet findSelectedID(Layout* box);	// get id of possibly select or select's parent in relation to box
	bool cursorInClickRange(const vec2i& mPos, uint8 mBut);
private:
	void mouseDownWidget(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	void mouseDownObject(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	Widget* setSelected(const vec2i& mPos, Layout* box);
	ScrollArea* getSelectedScrollArea() const;
	Layout* topLayout();
	Object* rayCast(const vec3& ray) const;
	static bool rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t);
};

inline void Scene::onText(const string& str) {
	capture->onText(str);
}

inline void Scene::setObjects(const vector<Object*>& objs) {
	objects = objs;
}

inline Layout* Scene::getLayout() {
	return layout.get();
}

inline Popup* Scene::getPopup() {
	return popup.get();
}

inline void Scene::setPopup(const pair<Popup*, Widget*>& popcap) {
	setPopup(popcap.first, popcap.second);
}

inline const vec2i& Scene::getMouseMove() const {
	return mouseMove;
}

inline bool Scene::cursorInClickRange(const vec2i& mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= clickThreshold;
}

inline Layout* Scene::topLayout() {
	return popup ? popup.get() : layout.get();
}
