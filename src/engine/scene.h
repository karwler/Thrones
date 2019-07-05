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
	vec3 direction(vec2i mPos) const;
};

struct Light {
	GLenum id;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;

	Light() = default;
	Light(GLenum id, const vec4& ambient = vec4(0.f, 0.f, 0.f, 1.f), const vec4& diffuse = vec4(1.f, 1.f, 1.f, 1.f), const vec4& specular = vec4(1.f, 1.f, 1.f, 1.f), const vec4& position = vec4(0.f, 4.f, 5.f, 0.f));

	void update() const;
};

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Interactable* inter;
	ScrollArea* area;
	vec2i mPos;

	ClickStamp(Interactable* inter = nullptr, ScrollArea* area = nullptr, vec2i mPos = 0);
};

// defines change of object properties at a time
struct Keyframe {
	enum Change : uint8 {
		CHG_NONE = 0x0,
		CHG_POS  = 0x1,
		CHG_ROT  = 0x2,
		CHG_LAT  = CHG_ROT
	};

	vec3 pos;
	vec3 rot;		// lat if applied to camera
	float time;		// time difference between this and previous keyframe
	Change change;	// what members get affected

	Keyframe(float time, Change change, const vec3& pos = vec3(), const vec3& rot = vec3());
};
ENUM_OPERATIONS(Keyframe::Change, uint8)

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
	Animation(Object* object, queue<Keyframe> keyframes);
	Animation(Camera* camera, queue<Keyframe> keyframes);

	bool tick(float dSec);
};

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Interactable* select;	// currently selected widget/object
	Interactable* capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
	vector<Object> effects;	// extra objects that'll get drawn on top without culling and have no interactivity. first element is the favor indicator
private:
	vec2i mouseMove;
	Camera camera;
	vector<Object*> objects;
	uptr<Layout> layout;
	uptr<Popup> popup;
	array<ClickStamp, SDL_BUTTON_X2+1> stamps;	// data about last mouse click (indexes are mouse button numbers
	vector<Animation> animations;
	vector<Light> lights;
	umap<string, Blueprint> bprints;
	umap<string, Material> materials;

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;
	static constexpr GLbitfield clearSet = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

public:
	Scene();

	void draw();
	void tick(float dSec);
	void onResize();
	void onKeyDown(const SDL_KeyboardEvent& key);
	void onKeyUp(const SDL_KeyboardEvent& key);
	void onMouseMove(vec2i mPos, vec2i mMov, uint32 mStat);
	void onMouseDown(vec2i mPos, uint8 mBut);
	void onMouseUp(vec2i mPos, uint8 mBut);
	void onMouseWheel(vec2i wMov);
	void onMouseLeave();
	void onText(const string& str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary

	const Blueprint* blueprint(const string& name) const;
	const Material* material(const string& name) const;
	Camera* getCamera();
	void setObjects(vector<Object*>&& objs);
	void resetLayouts();
	Layout* getLayout();
	Popup* getPopup();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);
	void addAnimation(const Animation& anim);
	vec2i getMouseMove() const;
	bool cursorInClickRange(vec2i mPos, uint8 mBut);
	vec3 pickerRay(vec2i mPos) const;

private:
	Interactable* getSelected(vec2i mPos);
	Interactable* getScrollOrObject(vec2i mPos, Widget* wgt) const;
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

inline void Scene::setObjects(vector<Object*>&& objs) {
	objects = std::move(objs);
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

inline vec2i Scene::getMouseMove() const {
	return mouseMove;
}

inline bool Scene::cursorInClickRange(vec2i mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= clickThreshold;
}

inline ScrollArea* Scene::getSelectedScrollArea() const {
	return dynamic_cast<Widget*>(select) ? findFirstScrollArea(static_cast<Widget*>(select)) : nullptr;
}

inline vec3 Scene::pickerRay(vec2i mPos) const {
	return camera.direction(mPos) * float(camera.zfar);
}
