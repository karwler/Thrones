#pragma once

#include "prog/types.h"
#include "utils/context.h"
#include "utils/layouts.h"
#include <queue>

// additional data for rendering objects
class Camera {
public:
	enum class State : uint8 {
		stationary,	// nothing's happening
		animating,	// in an animation
		dragging	// user is moving it
	};

	static constexpr float fov = 35.f;
	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;
	static constexpr float pmaxSetup = -PI / 2.f + PI / 10.f, pmaxMatch = -PI / 2.f + PI / 20.f;
	static constexpr float ymaxSetup = PI / 6.f, ymaxMatch = PI * 2.f;
	static constexpr vec3 posSetup = { Config::boardWidth / 2.f, 9.f, Config::boardWidth / 2.f + 9.f };
	static constexpr vec3 posMatch = { Config::boardWidth / 2.f, 12.f, Config::boardWidth / 2.f + 10.f };
	static constexpr vec3 latSetup = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 2.5f };
	static constexpr vec3 latMatch = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 1.f };
	static constexpr vec3 up = { 0.f, 1.f, 0.f };
	static constexpr vec3 center = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f };

	State state;
	float pmax, ymax;
private:
	vec2 prot;			// pitch and yaw record of position relative to lat
	float lyaw;			// yaw record of lat relative to the board's center
	float pdst, ldst;	// relative distance of pos/lat to lat/center
	float defaultPdst;
	vec3 pos, lat;
	mat4 proj;

public:
	Camera(const vec3& position, const vec3& lookAt, float pitchMax, float yawMax);

	void updateView() const;
	void updateProjection();
	const vec3& getPos() const;
	const vec3& getLat() const;
	void setPos(const vec3& newPos, const vec3& newLat);
	void rotate(const vec2& dRot, float dYaw);
	void zoom(int mov);
	vec3 direction(const ivec2& mPos) const;
	ivec2 screenPos(const vec3& pnt) const;
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

// single light information
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

	ClickStamp(Interactable* interact = nullptr, ScrollArea* scrollArea = nullptr, const ivec2& position = ivec2(INT_MIN), uint8 button = 0);
};

// defines change of object properties at a time
struct Keyframe {
	optional<vec3> pos;
	union {
		optional<vec3> scl, lat;
	};
	optional<quat> rot;
	float time;		// time difference between this and previous keyframe

	Keyframe(float timeOfs, const optional<vec3>& position = std::nullopt, const optional<quat>& rotation = std::nullopt, const optional<vec3>& scale = std::nullopt);
};

// a sequence of keyframes applied to an object
class Animation {
private:
	std::queue<Keyframe> kframes;
	Keyframe begin;		// initial state of the object
	union {
		Object* object;
		Camera* camera;
	};	// cannot be null
	bool useObject;

public:
	Animation(Object* obj, std::queue<Keyframe>&& keyframes);
	Animation(Camera* cam, std::queue<Keyframe>&& keyframes);

	bool tick(float dSec);
	void append(Animation& ani);
	bool operator==(const Animation& ani) const;
};

inline bool Animation::operator==(const Animation& ani) const {
	return useObject == ani.useObject && object == ani.object;
}

// handles more back-end UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Camera camera;
private:
	Interactable* select;	// currently selected widget/object
	Interactable* firstSelect;
	Interactable* capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
	uptr<RootLayout> layout;
	uptr<Popup> popup;
	vector<Overlay*> overlays;
	uptr<Context> context;
	vector<Animation> animations;
	umap<string, Mesh> meshes;
	umap<string, Material> matls;
	umap<string, Texture> texes;
	Light light;
	ClickStamp cstamp;	// data about last mouse click

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;

public:
	Scene();
	~Scene();

	void draw();
	void tick(float dSec);
	void onResize();
	void onMouseMove(const ivec2& pos, const ivec2& mov, uint32 state);
	void onMouseDown(const ivec2& pos, uint8 but);
	void onMouseUp(const ivec2& pos, uint8 but);
	void onMouseWheel(const ivec2& mov);
	void onMouseLeave();
	void onText(const char* str);
	void onConfirm();
	void onXbutConfirm();
	void onCancel();
	void onXbutCancel();

	const Mesh* mesh(const string& name) const;
	const Material* material(const string& name) const;
	void loadObjects();
	const Texture* getTex(const string& name) const;
	GLuint texture(const string& name = string()) const;
	void loadTextures();
	void reloadTextures();
	void resetShadows();
	void reloadShader();
	Interactable* getSelect() const;
	Interactable* getFirstSelect() const;
	Interactable* getCapture() const;
	void setCapture(Interactable* inter, bool reset = true);
	void resetLayouts();
	RootLayout* getLayout();
	Popup* getPopup();
	vector<Overlay*>& getOverlays();
	void setPopup(uptr<Popup>&& newPopup, Widget* newCapture = nullptr);
	Context* getContext();
	void setContext(uptr<Context>&& newContext);
	void addAnimation(Animation&& anim);
	void delegateStamp(Interactable* inter);
	bool cursorInClickRange(const ivec2& mPos) const;
	vec3 pickerRay(const ivec2& mPos) const;
	vec3 rayXZIsct(const vec3& ray) const;

	void navSelect(Direction dir);
	void updateSelect();
	void updateSelect(Interactable* sel);
	void updateSelect(const ivec2& mPos);
	void deselect();
private:
	Interactable* getSelected(const ivec2& mPos);
	Interactable* getScrollOrObject(const ivec2& mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea(Interactable* inter) const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);
};

inline Interactable* Scene::getSelect() const {
	return select;
}

inline Interactable* Scene::getFirstSelect() const {
	return firstSelect;
}

inline Interactable* Scene::getCapture() const {
	return capture;
}

inline RootLayout* Scene::getLayout() {
	return layout.get();
}

inline Popup* Scene::getPopup() {
	return popup.get();
}

inline vector<Overlay*>& Scene::getOverlays() {
	return overlays;
}

inline Context* Scene::getContext() {
	return context.get();
}

inline bool Scene::cursorInClickRange(const ivec2& mPos) const {
	return glm::length(vec2(mPos - cstamp.pos)) <= clickThreshold;
}

inline void Scene::updateSelect(const ivec2& mPos) {
	updateSelect(getSelected(mPos));
}

inline ScrollArea* Scene::getSelectedScrollArea(Interactable* inter) const {
	return dynamic_cast<Widget*>(inter) ? findFirstScrollArea(static_cast<Widget*>(inter)) : nullptr;
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
	return &matls.at(name);
}

inline const Texture* Scene::getTex(const string& name) const {
	return &texes.at(name);
}

inline GLuint Scene::texture(const string& name) const {
	return texes.at(name);
}
