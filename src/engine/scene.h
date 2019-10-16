#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// additional data for rendering objects
class Camera {
public:
	static constexpr float fov = 35.f;
	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;
	static constexpr float pmaxSetup = -PI / 2.f + PI / 10.f, pmaxMatch = -PI / 2.f + PI / 20.f;
	static constexpr float ymaxSetup = PI / 6.f, ymaxMatch = PI * 2.f;
	static const vec3 posSetup, posMatch;
	static const vec3 latSetup, latMatch;
	static const vec3 up, center;

	bool moving;		// whether the camera is currently moving (animation)
	float pmax, ymax;
private:
	vec2 prot;			// pitch and yaw record of position relative to lat
	float lyaw;			// yaw record of lat relative to the board's center
	float pdst, ldst;	// relative distance of pos/lat to lat/center
	float defaultPdst;
	vec3 pos, lat;
	mat4 proj;

public:
	Camera(const vec3& pos, const vec3& lat, float pmax, float ymax);

	void updateView() const;
	void updateProjection();
	const vec3& getPos() const;
	const vec3& getLat() const;
	void setPos(const vec3& newPos, const vec3& newLat);
	void rotate(const vec2& dRot, float dYaw);
	void zoom(int mov);
	vec3 direction(const ivec2& mPos) const;
private:
	void updateRotations(const vec3& pvec, const vec3& lvec);
	static float calcPitch(const vec3& pos, float dist);
	static float calcYaw(const vec3& pos, float dist);
};

inline const vec3& Camera::getPos() const {
	return pos;
}

inline const vec3& Camera::getLat() const {
	return lat;
}

inline float Camera::calcPitch(const vec3& pos, float dist) {
	return -std::asin(pos.y / dist);
}

inline float Camera::calcYaw(const vec3& pos, float dist) {
	return std::acos(pos.z / dist) * (pos.x >= 0.f ? 1.f : -1.f);
}

struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;

	Light(const vec3& position, const vec3& color, float ambiFac, float range);
};

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Interactable* inter;
	ScrollArea* area;
	ivec2 pos;
	uint8 but;

	ClickStamp(Interactable* inter = nullptr, ScrollArea* area = nullptr, const ivec2& pos = ivec2(INT_MIN), uint8 but = 0);
};

// defines change of object properties at a time
struct Keyframe {
	enum Change : uint8 {
		CHG_NONE = 0,
		CHG_POS = 1,
		CHG_ROT = 2,
		CHG_SCL = 4,
		CHG_LAT = CHG_SCL
	};

	vec3 pos, scl;	// scl = lat if applied to camera
	quat rot;
	float time;		// time difference between this and previous keyframe
	Change change;	// what members get affected

	Keyframe(float time, Change change, const vec3& pos = vec3(), const quat& rot = quat(), const vec3& scl = vec3());
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
	void append(Animation& ani);
	bool operator==(const Animation& ani) const;
};

inline bool Animation::operator==(const Animation& ani) const {
	return useObject == ani.useObject && (useObject ? object == ani.object : camera == ani.camera);
}

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Interactable* select;	// currently selected widget/object
	Interactable* capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
private:
	ivec2 mouseMove;	// last recorded cursor position difference
	uint32 moveTime;	// timestamp of last recorded mouseMove
	Camera camera;
	vector<Object*> objects;
	uptr<Layout> layout;
	uptr<Popup> popup;
	ClickStamp cstamp;	// data about last mouse click
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
	void onMouseMove(const SDL_MouseMotionEvent& mot);
	void onMouseDown(const SDL_MouseButtonEvent& but);
	void onMouseUp(const SDL_MouseButtonEvent& but);
	void onMouseWheel(const SDL_MouseWheelEvent& whe);
	void onMouseLeave();
	void onText(const char* str);

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
	void addAnimation(Animation&& anim);
	ivec2 getMouseMove() const;
	bool cursorInClickRange(const ivec2& mPos) const;
	vec3 pickerRay(const ivec2& mPos) const;

	void updateSelect(const ivec2& mPos);
private:
	void unselect();
	Interactable* getSelected(const ivec2& mPos);
	Interactable* getScrollOrObject(const ivec2& mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea() const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);
	Object* rayCast(const vec3& ray) const;
	static bool rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t);
	void simulateMouseMove();
};

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

inline ivec2 Scene::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : ivec2(0);
}

inline bool Scene::cursorInClickRange(const ivec2& mPos) const {
	return glm::length(vec2(mPos - cstamp.pos)) <= clickThreshold;
}

inline ScrollArea* Scene::getSelectedScrollArea() const {
	return dynamic_cast<Widget*>(select) ? findFirstScrollArea(static_cast<Widget*>(select)) : nullptr;
}

inline vec3 Scene::pickerRay(const ivec2& mPos) const {
	return camera.direction(mPos) * Camera::zfar;
}

inline const CMesh* Scene::collim(const string& name) const {
	return &collims.at(name);
}

inline const GMesh* Scene::mesh(const string& name) const {
	return &meshes.at(name);
}

inline const Material* Scene::material(const string& name) const {
	return &materials.at(name);
}

inline const Texture* Scene::getTex(const string& name) const {
	return &texes.at(name);
}

inline GLuint Scene::texture(const string& name) const {
	return texes.at(name).getID();
}

inline GLuint Scene::blank() const {
	return texes.at(string()).getID();
}
