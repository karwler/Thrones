#pragma once

#include "utils/objects.h"
#include "utils/widgets.h"
#include <queue>

// render data group
class FrameSet {
private:
#ifdef OPENGLES
	static constexpr uint lightBuffers = 1;
#else
	static constexpr uint lightBuffers = 2;
#endif

public:
	GLuint fboGeom = 0, texPosition = 0, texNormal = 0, texMatl = 0, rboGeom = 0;
	array<GLuint, 2> fboSsao{}, texSsao{};
	array<GLuint, 2> fboSsr{}, texSsr{};
	array<GLuint, lightBuffers> fboLight{}, texLight{};	// first is non-multisampled destination, second is multisampled source
	GLuint rboLight = 0;
	array<GLuint, 2> fboGauss{}, texGauss{};

	FrameSet(const Settings* sets, ivec2 res);

	void bindTextures();
	void free();

private:
	template <GLint iform, GLenum pform, GLenum active> static pair<GLuint, GLuint> makeFramebuffer(ivec2 res, string_view name);
	template <GLint iform, GLenum pform, GLenum active> static GLuint makeTexture(ivec2 res);
};

#ifndef OPENVR
// additional data for rendering objects
class Camera {
public:
	enum class State : uint8 {
		stationary,	// nothing's happening
		animating,	// in an animation
		dragging	// user is moving it
	};

	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;
	static constexpr float pmaxSetup = -glm::half_pi<float>() + glm::pi<float>() / 10.f, pmaxMatch = -glm::half_pi<float>() + glm::pi<float>() / 20.f;
	static constexpr float ymaxSetup = glm::pi<float>() / 6.f, ymaxMatch = glm::pi<float>() * 2.f;
	static constexpr vec3 posSetup = vec3(Config::boardWidth / 2.f, 9.f, Config::boardWidth / 2.f + 9.f);
	static constexpr vec3 posMatch = vec3(Config::boardWidth / 2.f, 12.f, Config::boardWidth / 2.f + 10.f);
	static constexpr vec3 latSetup = vec3(Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 2.5f);
	static constexpr vec3 latMatch = vec3(Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f + 1.f);
	static constexpr vec3 up = vec3(0.f, 1.f, 0.f);
	static constexpr vec3 center = vec3(Config::boardWidth / 2.f, 0.f, Config::boardWidth / 2.f);

	float pmax, ymax;
private:
	vec2 prot;			// pitch and yaw record of position relative to lat
	float lyaw;			// yaw record of lat relative to the board's center
	float pdst, ldst;	// relative distance of pos/lat to lat/center
	float defaultPdst;
	vec3 pos, lat;
	mat4 proj;
	float fov;
public:
	State state = State::stationary;

	Camera(const vec3& position, const vec3& lookAt, float vfov, float pitchMax, float yawMax, ivec2 res);

	void updateView() const;
	void updateProjection(vec2 res);	// call updateView afterwards
	void setFov(float vfov, ivec2 res);
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
#endif

// single light information
class Light {
public:
	GLuint fboDepth = 0, texDepth = 0;
	vec3 pos;

private:
	static constexpr float snear = 0.1f;

	vec3 ambient;
	vec3 diffuse;
	float linear;
	float quadratic;
	float farPlane;

public:
	Light(const Settings* sets, const vec3& position = vec3(Config::boardWidth / 2.f, 4.f, Config::boardWidth / 2.f), const vec3& color = vec3(1.f, 0.98f, 0.92f), float ambiFac = 0.8f, float range = 140.f);

	void free();
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
private:
	struct Capture {
		Interactable* inter;
		sizet len = 0;	// composing substring length

		constexpr Capture(Interactable* icap = nullptr);

		constexpr operator bool() const;
		Interactable* operator->();
	};

	static constexpr float clickThreshold = 8.f;
	static constexpr int scrollFactorWheel = 140;

	Interactable* select = nullptr;	// currently selected widget/object
	Interactable* firstSelect = nullptr;
	Capture capture;	// either pointer to widget currently hogging all keyboard input or something that's currently being dragged. nullptr otherwise
#ifndef OPENVR
	uptr<Camera> camera;
#endif
	RootLayout* layout = nullptr;
	vector<Popup*> popups;
	vector<Overlay*> overlays;
	Context* context = nullptr;
	TitleBar* titleBar = nullptr;
	vector<Animation> animations;
	vector<Mesh> meshes;
	umap<string, uint16> meshRefs;
	umap<string, Material> matls;
	umap<string, TexLoc> texColRefs;
	TextureSet texSet;
	TextureCol texCol;
	Light light;
	ClickStamp cstamp;	// data about last mouse click
	FrameSet frames;
	GLuint wgtVao;
	Quad wgtTops;
	array<Quad::Instance, 3> wgtTopInsts;	// can't be more than 3 instances (one for LabelEdit caret and two for tooltip)

public:
	Scene();
	~Scene();

	void draw(ivec2 mPos, GLuint finalFbo = 0);
	void tick(float dSec);
	void onExternalResize();
	void onInternalResize();
	void onMouseMove(ivec2 pos, ivec2 mov, uint32 state);
	void onMouseDown(ivec2 pos, uint8 but);
	void onMouseUp(ivec2 pos, uint8 but);
	void onMouseWheel(ivec2 pos, ivec2 mov);
	void onMouseLeave(ivec2 pos);
	void onCompose(string_view str);
	void onText(string_view str);
	void onConfirm();
	void onXbutConfirm();
	void onCancel();
	void onXbutCancel();

	Mesh* mesh(const string& name);
	const Material* material(const string& name) const;
	void loadObjects();
	TextureCol* getTexCol();
	const TexLoc& wgtTex(const string& name = string()) const;
	uvec2 objTex(const string& name = string()) const;
	void loadTextures(int recomTexSize);
	void reloadTextures();
	void resetShadows();
	void resetFrames();
	void setPieceIcons();
	Interactable* getSelect() const;
	Interactable* getFirstSelect() const;
	Interactable* getCapture() const;
	void setCapture(Interactable* inter, bool reset = true);
	void resetLayouts();
	void updateTooltips();
	Camera* getCamera();
	RootLayout* getLayout();
	Popup* getPopup();
	vector<Overlay*>& getOverlays();
	void pushPopup(Popup* newPopup, Widget* newCapture = nullptr);
	void popPopup();
	Context* getContext();
	void setContext(Context* newContext);
	TitleBar* getTitleBar();
	void setTitleBar(TitleBar* bar);
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

	array<SDL_Surface*, pieceLim> renderPieceIcons();
	void renderModel(GLuint fbo, const FrameSet& frms, const string& name, const Material* matl);
	static void startDepthDraw(const Shader* shader, GLuint fbo);
	static void drawFrame(const Shader* shader, GLuint fbo);
};

constexpr Scene::Capture::Capture(Interactable* icap) :
	inter(icap)
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

inline Camera* Scene::getCamera() {
	return camera.get();
}

inline RootLayout* Scene::getLayout() {
	return layout;
}

inline Popup* Scene::getPopup() {
	return !popups.empty() ? popups.back() : nullptr;
}

inline vector<Overlay*>& Scene::getOverlays() {
	return overlays;
}

inline Context* Scene::getContext() {
	return context;
}

inline TitleBar* Scene::getTitleBar() {
	return titleBar;
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
	return camera->direction(mPos) * Camera::zfar;	// TODO: VR handling
}

inline vec3 Scene::rayXZIsct(const vec3& ray) const {
	return camera->getPos() - ray * (camera->getPos().y / ray.y);	// TODO: VR handling
}

inline Mesh* Scene::mesh(const string& name) {
	return &meshes[meshRefs.at(name)];
}

inline const Material* Scene::material(const string& name) const {
	return &matls.at(name);
}

inline TextureCol* Scene::getTexCol() {
	return &texCol;
}

inline const TexLoc& Scene::wgtTex(const string& name) const {
	return texColRefs.at(name);
}

inline uvec2 Scene::objTex(const string& name) const {
	return texSet.get(name);
}
