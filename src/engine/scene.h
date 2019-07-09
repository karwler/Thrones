#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// additional data for rendering objects
class Camera {
public:
	static constexpr float fov = 45.f;
	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;
	static const vec3 posSetup, posMatch;
	static const vec3 latSetup, latMatch;
	static const vec3 up;

	vec3 pos, lat;
private:
	mat4 proj;

public:
	Camera(const vec3& pos, const vec3& lat = vec3(0.f));

	void update() const;
	void updateProjection();	// requires the gui shader to be in use
	const mat4& getOrtho() const;
	vec3 direction(vec2i mPos) const;
};

struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	Attenuation att;

	Light(const vec3& position, const vec3& ambient = vec3(1.f), const vec3& diffuse = vec3(1.f), const vec3& specular = vec3(1.f), Attenuation att = 100.f);

	void update() const;	// requires the scene shader to be in use
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
		CHG_NONE = 0,
		CHG_POS  = 1,
		CHG_ROT  = 2,
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
	std::queue<Keyframe> keyframes;
	Keyframe begin;		// initial state of the object
	union {
		Object* object;
		Camera* camera;
	};	// cannot be null
	bool useObject;

public:
	Animation(Object* object, std::queue<Keyframe>&& keyframes);
	Animation(Camera* camera, std::queue<Keyframe>&& keyframes);

	bool tick(float dSec);
};

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Interactable* select;	// currently selected widget/object
	Interactable* capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
private:
	vec2i mouseMove;	// last recorded cursor position difference
	uint32 moveTime;	// timestamp of last recorded mouseMove
	Camera camera;
	vector<Object*> objects;
	uptr<Layout> layout;
	uptr<Popup> popup;
	array<ClickStamp, SDL_BUTTON_X2+1> stamps;	// data about last mouse click (indexes are mouse button numbers
	vector<Animation> animations;
	Light light;
	umap<string, GMesh> meshes;
	umap<string, Material> materials;
	umap<string, Texture> texes;
	umap<string, CMesh> collims;

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;
	static constexpr uint32 moveTimeout = 50;
public:
	Scene();
	~Scene();

	void draw();
	void tick(float dSec);
	void onResize();
	void onKeyDown(const SDL_KeyboardEvent& key);
	void onKeyUp(const SDL_KeyboardEvent& key);
	void onMouseMove(vec2i mPos, vec2i mMov, uint32 mStat, uint32 time);
	void onMouseDown(vec2i mPos, uint8 mBut);
	void onMouseUp(vec2i mPos, uint8 mBut);
	void onMouseWheel(vec2i wMov);
	void onMouseLeave();
	void onText(const char* str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary

	const CMesh* collim(const string& name) const;
	const GMesh* mesh(const string& name) const;
	const Material* material(const string& name) const;
	const Texture* getTex(const string& name) const;
	GLuint texture(const string& name) const;
	GLuint blank() const;
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

	void updateSelect(vec2i mPos);
private:
	void unselect();
	Interactable* getSelected(vec2i mPos);
	Interactable* getScrollOrObject(vec2i mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea() const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);
	template <class T> static const T& findAsset(const umap<string, T>& assets, const string& name);

	Object* rayCast(const vec3& ray) const;
	static bool rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t);
};

inline void Scene::onText(const char* str) {
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
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : 0;
}

inline bool Scene::cursorInClickRange(vec2i mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= clickThreshold;
}

inline ScrollArea* Scene::getSelectedScrollArea() const {
	return dynamic_cast<Widget*>(select) ? findFirstScrollArea(static_cast<Widget*>(select)) : nullptr;
}

inline vec3 Scene::pickerRay(vec2i mPos) const {
	return camera.direction(mPos) * Camera::zfar;
}

inline const CMesh* Scene::collim(const string& name) const {
	return &findAsset(collims, name);
}

inline const GMesh* Scene::mesh(const string& name) const {
	return &findAsset(meshes, name);
}

inline const Material* Scene::material(const string& name) const {
	return &findAsset(materials, name);
}

inline const Texture* Scene::getTex(const string& name) const {
	return &findAsset(texes, name);
}

inline GLuint Scene::texture(const string& name) const {
	return findAsset(texes, name).getID();
}

inline GLuint Scene::blank() const {
	return texes.at("").getID();
}

template <class T>
const T& Scene::findAsset(const umap<string, T>& assets, const string& name) {
	typename umap<string, T>::const_iterator it = assets.find(name);
	return it != assets.end() ? it->second : assets.at("");
}
