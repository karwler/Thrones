#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// additional data for rendering objects
class Camera {
public:
	vec3 pos, lat;
	float fov, znear, zfar;
	
	static const vec3 posSetup, posMatch;
	static const vec3 latSetup, latMatch;
private:
	static const vec3 up;

public:
	Camera(const vec3& pos = posSetup, const vec3& lat = latSetup, float fov = 45.f, float znear = 1.f, float zfar = 20.f);

	void update() const;
	static void updateUI();
	vec3 direction(const vec2i& mPos) const;
};

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Interactable* inter;
	ScrollArea* area;
	vec2i mPos;

	ClickStamp(Interactable* inter = nullptr, ScrollArea* area = nullptr, const vec2i& mPos = 0);
};

// defines change of object properties at a time
struct Keyframe {
	enum Change : uint8 {
		CHG_NONE = 0x0,
		CHG_POS  = 0x1,
		CHG_ROT  = 0x2,
		CHG_LAT  = CHG_ROT,
		CHG_CLR  = 0x4
	};

	vec3 pos;
	vec3 rot;		// lat if applied to camera
	vec4 color;
	float time;		// time difference between this and previous keyframe
	Change change;	// what members get affected

	Keyframe(float time, Change change, const vec3& pos = vec3(), const vec3& rot = vec3(), const vec4& color = vec4());
};

inline constexpr Keyframe::Change operator~(Keyframe::Change a) {
	return Keyframe::Change(~uint8(a));
}

inline constexpr Keyframe::Change operator&(Keyframe::Change a, Keyframe::Change b) {
	return Keyframe::Change(uint8(a) & uint8(b));
}

inline constexpr Keyframe::Change operator&=(Keyframe::Change& a, Keyframe::Change b) {
	return a = Keyframe::Change(uint8(a) & uint8(b));
}

inline constexpr Keyframe::Change operator^(Keyframe::Change a, Keyframe::Change b) {
	return Keyframe::Change(uint8(a) ^ uint8(b));
}

inline constexpr Keyframe::Change operator^=(Keyframe::Change& a, Keyframe::Change b) {
	return a = Keyframe::Change(uint8(a) ^ uint8(b));
}

inline constexpr Keyframe::Change operator|(Keyframe::Change a, Keyframe::Change b) {
	return Keyframe::Change(uint8(a) | uint8(b));
}

inline constexpr Keyframe::Change operator|=(Keyframe::Change& a, Keyframe::Change b) {
	return a = Keyframe::Change(uint8(a) | uint8(b));
}

// a sequence of keyframes applied to an object
class Animation {
private:
	queue<Keyframe> keyframes;
	Keyframe begin;		// initial state of the object
	union {
		Object* object;
		Camera* camera;
	};	// cannot be null
	bool useObject;

public:
	Animation(Object* object, const queue<Keyframe>& keyframes);
	Animation(Camera* camera, const queue<Keyframe>& keyframes);

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
	Interactable* select;	// currently selected widget/object
	Interactable* capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
private:
	vec2i mouseMove;
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
	void onMouseUp(const vec2i& mPos, uint8 mBut, uint8 mCnt);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();
	void onText(const string& str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary

	Camera* getCamera();
	void setObjects(const vector<Object*>& objs);
	void resetLayouts();
	Layout* getLayout();
	Popup* getPopup();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);
	void addAnimation(const Animation& anim);
	const vec2i& getMouseMove() const;
	bool cursorInClickRange(const vec2i& mPos, uint8 mBut);
	vec3 pickerRay(const vec2i& mPos) const;

private:
	Interactable* getSelected(const vec2i& mPos);
	Interactable* getScrollOrObject(const vec2i& mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea() const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);

	Object* rayCast(const vec3& ray) const;
	static bool rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t);
};

inline void Scene::onText(const string& str) {
	capture->onText(str);
}

inline Camera* Scene::getCamera() {
	return &camera;
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

inline ScrollArea* Scene::getSelectedScrollArea() const {
	return dynamic_cast<Widget*>(select) ? findFirstScrollArea(static_cast<Widget*>(select)) : nullptr;
}

inline vec3 Scene::pickerRay(const vec2i& mPos) const {
	return camera.direction(mPos) * float(camera.zfar);
}
