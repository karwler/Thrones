#pragma once

#include "utils/context.h"
#include "utils/layouts.h"
#include "utils/objects.h"
#include <queue>

// screen space quad
class Frame {
public:
	static constexpr uint corners = 4;
private:
	static constexpr uint stride = 4;
	static constexpr uint vlength = 2;
	static constexpr float vertices[corners * stride] = {
		-1.f,  1.f, 0.f, 1.f,
		1.f,  1.f, 1.f, 1.f,
		-1.f, -1.f, 0.f, 0.f,
		1.f, -1.f, 1.f, 0.f,
	};

	GLuint vao = 0, vbo = 0;

public:
	Frame();
	~Frame();

	GLuint getVao() const;
};

inline GLuint Frame::getVao() const {
	return vao;
}

// skybox cube
class Skybox {
public:
	static constexpr uint corners = 36;
private:
	static constexpr uint vlength = 3;
	static constexpr float vertices[corners * vlength] = {
		1.f, -1.f, -1.f,
		-1.f, -1.f, -1.f,
		-1.f, 1.f, -1.f,
		-1.f, 1.f, -1.f,
		1.f, 1.f, -1.f,
		1.f, -1.f, -1.f,
		-1.f, 1.f, -1.f,
		-1.f, -1.f, -1.f,
		-1.f, -1.f, 1.f,
		-1.f, -1.f, 1.f,
		-1.f, 1.f, 1.f,
		-1.f, 1.f, -1.f,
		1.f, 1.f, 1.f,
		1.f, -1.f, 1.f,
		1.f, -1.f, -1.f,
		1.f, -1.f, -1.f,
		1.f, 1.f, -1.f,
		1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,
		-1.f, 1.f, 1.f,
		-1.f, -1.f, 1.f,
		-1.f, -1.f, 1.f,
		1.f, -1.f, 1.f,
		1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,
		1.f, 1.f, -1.f,
		-1.f, 1.f, -1.f,
		-1.f, 1.f, -1.f,
		-1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,
		1.f, -1.f, -1.f,
		-1.f, -1.f, 1.f,
		-1.f, -1.f, -1.f,
		1.f, -1.f, 1.f,
		-1.f, -1.f, 1.f,
		1.f, -1.f, -1.f
	};

	GLuint vao = 0, vbo = 0;

public:
	Skybox();
	~Skybox();

	GLuint getVao() const;
};

inline GLuint Skybox::getVao() const {
	return vao;
}

// render data group
class FrameSet {
private:
#ifdef OPENGLES
	static constexpr uint lightBuffers = 1;
#else
	static constexpr uint lightBuffers = 2;
#endif

public:
	GLuint fboGeom = 0, texPosition = 0, texNormal = 0, rboGeom = 0;
	GLuint fboSsao = 0, texSsao = 0;
	GLuint fboBlur = 0, texBlur = 0;
	array<GLuint, lightBuffers> fboLight{}, texLight{};	// first is non-multisampled destination, second is multisampled source
	GLuint rboLight = 0;
	array<GLuint, 2> fboGauss{}, texGauss{};

	FrameSet(const Settings* sets, ivec2 res);

	void free();

private:
	static GLuint makeFramebuffer(GLuint& tex, ivec2 res, GLint iform, GLenum active, const string& name);
	static GLuint makeTexture(ivec2 res, GLint iform, GLenum active);
};

// additional data for rendering objects
class Camera {
public:
	enum class State : uint8 {
		stationary,	// nothing's happening
		animating,	// in an animation
		dragging	// user is moving it
	};

	static constexpr float fov = glm::radians(35.f);
	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;
	static constexpr float pmaxSetup = -glm::half_pi<float>() + glm::pi<float>() / 10.f, pmaxMatch = -glm::half_pi<float>() + glm::pi<float>() / 20.f;
	static constexpr float ymaxSetup = glm::pi<float>() / 6.f, ymaxMatch = glm::pi<float>() * 2.f;
	static constexpr vec3 posSetup = { Config::boardWidth / 2.f, 9.f, Config::boardWidth / 2.f + 9.f };
	static constexpr vec3 posMatch = { Config::boardWidth / 2.f, 12.f, Config::boardWidth / 2.f + 10.f };
	static constexpr vec3 latSetup = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 2.5f };
	static constexpr vec3 latMatch = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 1.f };
	static constexpr vec3 up = { 0.f, 1.f, 0.f };
	static constexpr vec3 center = { Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f };

	State state = State::stationary;
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
	void rotate(vec2 dRot, float dYaw);
	void zoom(int mov);
	vec3 direction(ivec2 mPos) const;
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
	GLuint fboDepth = 0, texDepth = 0;

private:
	static constexpr float snear = 0.1f;

	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;
	float farPlane;

public:
	Light(GLsizei res, const vec3& pos = vec3(Config::boardWidth / 2.f, 4.f, Config::boardWidth / 2.f), const vec3& color = vec3(1.f, 0.98f, 0.92f), float ambiFac = 0.8f, float range = 140.f);

	void free();
	void updateValues();
};

// saves what widget is being clicked on with what button at what position
struct ClickStamp {
	Interactable* inter;
	ScrollArea* area;
	ivec2 pos;
	uint8 but;

	ClickStamp(Interactable* interact = nullptr, ScrollArea* scrollArea = nullptr, ivec2 position = ivec2(INT_MIN), uint8 button = 0);
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
	Camera camera = Camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup);

private:
	struct Capture {
		Interactable* inter;
		uint len = 0;	// composing substring length

		constexpr Capture(Interactable* capture = nullptr);

		constexpr operator bool() const;
		Interactable* operator->();
	};

	Interactable* select = nullptr;	// currently selected widget/object
	Interactable* firstSelect = nullptr;
	Capture capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
	uptr<RootLayout> layout;
	vector<Popup*> popups;
	vector<Overlay*> overlays;
	uptr<Context> context;
	uptr<TitleBar> titleBar;
	vector<Animation> animations;
	vector<Mesh> meshes;
	umap<string, uint16> meshRefs;
	umap<string, Material> matls;
	umap<string, Texture> texes;
	TextureSet texSet;
	Light light;
	ClickStamp cstamp;	// data about last mouse click
	FrameSet frames;
	Frame scrFrame;
	Skybox skybox;

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;

public:
	Scene();
	~Scene();

	void draw();
	void tick(float dSec);
	void onResize();
	void onMouseMove(ivec2 pos, ivec2 mov, uint32 state);
	void onMouseDown(ivec2 pos, uint8 but);
	void onMouseUp(ivec2 pos, uint8 but);
	void onMouseWheel(ivec2 mov);
	void onMouseLeave();
	void onCompose(const char* str);
	void onText(const char* str);
	void onConfirm();
	void onXbutConfirm();
	void onCancel();
	void onXbutCancel();

	Mesh* mesh(const string& name);
	const Material* material(const string& name) const;
	void loadObjects();
	const Texture* getTex(const string& name) const;
	GLuint texture(const string& name = string()) const;
	int objTex(const string& name = string()) const;
	void loadTextures();
	void reloadTextures();
	void reloadShader();
	void resetShadows();
	void resetFrames();
	Interactable* getSelect() const;
	Interactable* getFirstSelect() const;
	Interactable* getCapture() const;
	void setCapture(Interactable* inter, bool reset = true);
	void resetLayouts();
	void updateTooltips();
	RootLayout* getLayout();
	Popup* getPopup();
	vector<Overlay*>& getOverlays();
	void pushPopup(uptr<Popup>&& newPopup, Widget* newCapture = nullptr);
	void popPopup();
	Context* getContext();
	void setContext(uptr<Context>&& newContext);
	TitleBar* getTitleBar();
	void setTitleBar(uptr<TitleBar>&& bar);
	void addAnimation(Animation&& anim);
	void delegateStamp(Interactable* inter);
	bool cursorInClickRange(ivec2 mPos) const;
	vec3 pickerRay(ivec2 mPos) const;
	vec3 rayXZIsct(const vec3& ray) const;

	void navSelect(Direction dir);
	void updateSelect();
	void updateSelect(Interactable* sel);
	void updateSelect(ivec2 mPos);
	void deselect();
	Interactable* getTitleBarSelected(ivec2 tmPos);
private:
	Interactable* getSelected(ivec2 mPos);
	Interactable* getScrollOrObject(ivec2 mPos, Widget* wgt) const;
	ScrollArea* getSelectedScrollArea(Interactable* inter) const;
	static ScrollArea* findFirstScrollArea(Widget* wgt);
};

constexpr Scene::Capture::Capture(Interactable* capture) :
	inter(capture)
{}

constexpr Scene::Capture::operator bool() const {
	return inter;
}

inline Interactable* Scene::Capture::operator->() {
	return inter;
}

inline Interactable* Scene::getSelect() const {
	return select;
}

inline Interactable* Scene::getFirstSelect() const {
	return firstSelect;
}

inline Interactable* Scene::getCapture() const {
	return capture.inter;
}

inline RootLayout* Scene::getLayout() {
	return layout.get();
}

inline Popup* Scene::getPopup() {
	return !popups.empty() ? popups.back() : nullptr;
}

inline vector<Overlay*>& Scene::getOverlays() {
	return overlays;
}

inline Context* Scene::getContext() {
	return context.get();
}

inline TitleBar* Scene::getTitleBar() {
	return titleBar.get();
}

inline bool Scene::cursorInClickRange(ivec2 mPos) const {
	return glm::length(vec2(mPos - cstamp.pos)) <= clickThreshold;
}

inline void Scene::updateSelect(ivec2 mPos) {
	updateSelect(getSelected(mPos));
}

inline ScrollArea* Scene::getSelectedScrollArea(Interactable* inter) const {
	return dynamic_cast<Widget*>(inter) ? findFirstScrollArea(static_cast<Widget*>(inter)) : nullptr;
}

inline vec3 Scene::pickerRay(ivec2 mPos) const {
	return camera.direction(mPos) * Camera::zfar;
}

inline vec3 Scene::rayXZIsct(const vec3& ray) const {
	return camera.getPos() - ray * (camera.getPos().y / ray.y);
}

inline Mesh* Scene::mesh(const string& name) {
	return &meshes[meshRefs.at(name)];
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

inline int Scene::objTex(const string& name) const {
	return texSet.get(name);
}
