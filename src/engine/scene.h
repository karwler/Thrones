#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Interactable* inter;
	ScrollArea* area;
	vec2i mPos;

	ClickStamp(Interactable* inter = nullptr, ScrollArea* area = nullptr, const vec2i& mPos = 0);
};

struct Keyframe {
	enum Change : uint8 {
		CHG_NONE = 0x0,
		CHG_POS  = 0x1,
		CHG_ROT  = 0x2,
		CHG_CLR  = 0x4
	};

	vec3 pos;
	vec3 rot;
	SDL_Color color;
	float time;		// time difference between this and previous keyframe
	Change change;	// what members get affected

	Keyframe(float time, Change change, const vec3& pos = vec3(), const vec3& rot = vec3(), SDL_Color color = {});
};

class Animation {
private:
	queue<Keyframe> keyframes;
	Keyframe begin;		// initial state of the object
	Object* object;		// cannot be null

public:
	Animation(Object* object, const std::initializer_list<Keyframe>& keyframes);

	bool tick(float dSec);

private:
	template <class T> static T linearTransition(const T& start, const T& end, float factor);
};

template <class T>
T Animation::linearTransition(const T& start, const T& end, float factor) {
	return start + (end - start) * factor;
}

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	vec2i mouseMove;
	Interactable* select;		// currently selected widget
	Widget* capture;	// either pointer to widget currently hogging all keyboard input or ScrollArea whichs slider is currently being dragged. nullptr if nothing is being captured or dragged
private:
	Camera camera;
	vector<Object*> objects;
	uptr<Layout> layout;
	uptr<Popup> popup;
	array<ClickStamp, SDL_BUTTON_X2+1> stamps;	// data about last mouse click (indexes are mouse button numbers
	vector<Animation> animations;

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
	void addAnimation(const Animation& anim);
	const vec2i& getMouseMove() const;

	bool cursorInClickRange(const vec2i& mPos, uint8 mBut);
	vec3 cursorDirection(const vec2i& mPos) const;
	Object* rayCast(const vec3& ray) const;
private:
	Interactable* getSelected(const vec2i& mPos, Layout* box);
	ScrollArea* getSelectedScrollArea() const;
	Layout* topLayout();
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

inline void Scene::addAnimation(const Animation& anim) {
	animations.push_back(anim);
}

inline const vec2i& Scene::getMouseMove() const {
	return mouseMove;
}

inline bool Scene::cursorInClickRange(const vec2i& mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= clickThreshold;
}

inline vec3 Scene::cursorDirection(const vec2i& mPos) const {
	return camera.direction(mPos) * float(camera.zfar);
}

inline Layout* Scene::topLayout() {
	return popup ? popup.get() : layout.get();
}
