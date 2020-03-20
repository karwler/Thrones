#pragma once

#include "engine/fileSys.h"
#include "utils/context.h"
#include "utils/layouts.h"

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

class Light {
public:
	GLuint depthMap, depthFrame;

	static constexpr GLenum depthTexa = GL_TEXTURE7;
	static constexpr float defaultRange = 140.f;
private:
	static constexpr float snear = 0.1f;

	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;

public:
	Light(const vec3& pos, const vec3& color, float ambiFac, float range = defaultRange);
	~Light();

	void updateValues(float range = defaultRange);
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
	vector<Object*> objects;
	uptr<RootLayout> layout;
	uptr<Popup> popup;
	uptr<Overlay> overlay;
	uptr<Context> context;
	vector<Animation> animations;
	umap<string, Mesh> meshes;
	umap<string, Material> materials;
	umap<string, Texture> texes;
	void (Scene::*shadowFunc)();
	Camera camera;
	Light light;
	ivec2 mouseMove;	// last recorded cursor position difference
	uint32 moveTime;	// timestamp of last recorded mouseMove
	bool mouseLast;		// last input was mouse or touch
	ClickStamp cstamp;	// data about last mouse click

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
	void onMouseMove(const SDL_MouseMotionEvent& mot, bool mouse = true);
	void onMouseDown(const SDL_MouseButtonEvent& but, bool mouse = true);
	void onMouseUp(const SDL_MouseButtonEvent& but, bool mouse = true);
	void onMouseWheel(const SDL_MouseWheelEvent& whe);
	void onMouseLeave();
	void onFingerMove(const SDL_TouchFingerEvent& fin);
	void onFingerGesture(const SDL_MultiGestureEvent& ges);
	void onFingerDown(const SDL_TouchFingerEvent& fin);
	void onFingerUp(const SDL_TouchFingerEvent& fin);
	void onText(const char* str);

	const Mesh* mesh(const string& name) const;
	const Material* material(const string& name) const;
	const Texture* getTex(const string& name) const;
	GLuint texture(const string& name) const;
	GLuint blank() const;
	void reloadTextures();
	void resetShadows();
	void reloadShader();
	Camera* getCamera();
	void setObjects(vector<Object*>&& objs);
	void resetLayouts();
	RootLayout* getLayout();
	Popup* getPopup();
	Overlay* getOverlay();
	void setPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void setPopup(const pair<Popup*, Widget*>& popcap);
	void setContext(Context* newContext);
	void addAnimation(Animation&& anim);
	ivec2 getMouseMove() const;
	bool cursorInClickRange(const ivec2& mPos) const;
	vec3 pickerRay(const ivec2& mPos) const;
	vec3 rayXZIsct(const vec3& ray) const;

	void updateSelect();
private:
	void updateSelect(const ivec2& mPos);
	void unselect();
	Interactable* getSelected(const ivec2& mPos);
	Interactable* getScrollOrObject(const ivec2& mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea() const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);
	BoardObject* findBoardObject(const ivec2& mPos) const;
	void simulateMouseMove();

	void renderShadows();
	void renderDummy() {}
};

inline Camera* Scene::getCamera() {
	return &camera;
}

inline void Scene::setObjects(vector<Object*>&& objs) {
	objects = std::move(objs);
}

inline RootLayout* Scene::getLayout() {
	return layout.get();
}

inline Popup* Scene::getPopup() {
	return popup.get();
}

inline Overlay* Scene::getOverlay() {
	return overlay.get();
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

inline vec3 Scene::rayXZIsct(const vec3& ray) const {
	return camera.getPos() - ray * (camera.getPos().y / ray.y);
}

inline const Mesh* Scene::mesh(const string& name) const {
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

inline void Scene::reloadTextures() {
	FileSys::reloadTextures(texes);
}
