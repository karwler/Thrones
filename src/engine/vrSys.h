#pragma once

#ifdef OPENVR
#include "shaders.h"
#ifdef SYSLIBS
#include <openvr/openvr.h>
#else
#include <openvr.h>
#endif

class VrSys {
private:
	enum Hand : uint8 {
		handLeft,
		handRight
	};

	struct Model {
		GLuint vao, vbo, ebo;
		GLuint tex;
		GLsizei count;
		string name;	// TODO: this can probably be a const char*
	};

	struct ControllerInfo {
		vr::VRInputValueHandle_t source = vr::k_ulInvalidInputValueHandle;
		vr::VRActionHandle_t actionPose = vr::k_ulInvalidActionHandle;
		vr::VRActionHandle_t actionHaptic = vr::k_ulInvalidActionHandle;
		mat4 pose;
		Model* model = nullptr;
		bool showController;
	};

	struct VertexWindow {
		vec2 position;
		vec2 texCoord;

		VertexWindow(vec2 pos, vec2 tuv);
	};

	static constexpr float znear = 0.1f;
	static constexpr float zfar = 100.f;

	array<GLuint, 2> fboEye{}, rboEye{}, texEye{};
	mat4 leftProj, rightProj;
	mat4 leftPos, rightPos;
	mat4 sysPose;
	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	mat4 devicePose[vr::k_unMaxTrackedDeviceCount];

	vr::IVRSystem* system = nullptr;
	uvec2 renderSize;

	vector<Model*> models;
	ControllerInfo hands[2];
	vr::VRActionHandle_t actionHideCubes = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t actionHideThisController = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t actionTriggerHaptic = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t actionAnalongInput = vr::k_ulInvalidActionHandle;
	vr::VRActionSetHandle_t actionsetDemo = vr::k_ulInvalidActionSetHandle;

	int trackedControllerCount = 0;
	int trackedControllerCount_Last = -1;
	int validPoseCount = 0;
	int validPoseCount_Last = -1;
	string poseClasses;
	char devClassChar[vr::k_unMaxTrackedDeviceCount]{};

	GLuint vaoController = 0;
	GLuint vboController = 0;
	uint controllerVertCount;

	GLuint vaoCompanionWindow = 0;
	GLuint vboCompanionWindow = 0;
	GLuint eboCompanionWindow = 0;
	static constexpr uint companionWindowIndexSize = 12;
	ivec2 companionWindowSize;

	ShaderVrController* shdController = nullptr;	// TODO: compile and use shaders
	ShaderVrModel* shdModel = nullptr;
	ShaderVrWindow* shdWindow = nullptr;

public:
	VrSys();
	~VrSys();

	void init(const umap<string, string>& sources, ivec2 windowSize);
	void free();
	void handleInput();
	void draw();

	uvec2 getRenderSize() const;

private:
	void createFrameBuffer(GLuint fbo, GLuint rbo, GLuint tex, string_view name);
	void drawControllerAxes();
	void drawStereoTargets();
	void drawScene(vr::Hmd_Eye eye);
	void drawCompanionWindow();

	mat4 getCurrentViewProjectionMatrix(vr::Hmd_Eye eye);
	bool getDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* devicePath = nullptr);
	bool getDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* devicePath = nullptr);
	bool getDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* devicePath = nullptr);
	string getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr);
	Model* findOrLoadRenderModel(const char* renderModelName);
	void initModel(Model* model, const vr::RenderModel_t* vrModel, const vr::RenderModel_TextureMap_t* vrTexture, const char* renderModelName);
	void freeModel(Model* model);
	void drawModel(Model* model);

	static mat4 hmdToMat4(const vr::HmdMatrix44_t& mat);
	static mat4 hmdToMat4(const vr::HmdMatrix34_t& mat);
};

inline uvec2 VrSys::getRenderSize() const {
	return renderSize;
}

#endif
