#ifdef OPENVR
#include "vrSys.h"
#include "fileSys.h"
#include "scene.h"
#include "world.h"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
using namespace vr;

VrSys::VertexWindow::VertexWindow(vec2 pos, vec2 tuv) :
	position(pos),
	texCoord(tuv)
{}

VrSys::VrSys() {
	EVRInitError error = VRInitError_None;
	system = VR_Init(&error, VRApplication_Scene);
	if (error != VRInitError_None)
		throw std::runtime_error("Failed to initialize VR runtime: "s + VR_GetVRInitErrorAsEnglishDescription(error));

	leftProj = hmdToMat4(system->GetProjectionMatrix(Eye_Left, znear, zfar));	// TODO: all this needs to be passed to Camera
	rightProj = hmdToMat4(system->GetProjectionMatrix(Eye_Right, znear, zfar));
	leftPos = hmdToMat4(system->GetEyeToHeadTransform(Eye_Left));
	rightPos = hmdToMat4(system->GetEyeToHeadTransform(Eye_Right));
	system->GetRecommendedRenderTargetSize(&renderSize.x, &renderSize.y);

	VRInput()->SetActionManifestPath((FileSys::dataPath() + "vr_actions.json").c_str());

	VRInput()->GetActionHandle("/actions/demo/in/HideCubes", &actionHideCubes);
	VRInput()->GetActionHandle("/actions/demo/in/HideThisController", &actionHideThisController);
	VRInput()->GetActionHandle("/actions/demo/in/TriggerHaptic", &actionTriggerHaptic);
	VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &actionAnalongInput);

	VRInput()->GetActionSetHandle("/actions/demo", &actionsetDemo);

	VRInput()->GetActionHandle("/actions/demo/out/Haptic_Left", &hands[handLeft].actionHaptic);
	VRInput()->GetInputSourceHandle("/user/hand/left", &hands[handLeft].source);
	VRInput()->GetActionHandle("/actions/demo/in/Hand_Left", &hands[handLeft].actionPose);

	VRInput()->GetActionHandle("/actions/demo/out/Haptic_Right", &hands[handRight].actionHaptic);
	VRInput()->GetInputSourceHandle("/user/hand/right", &hands[handRight].source);
	VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &hands[handRight].actionPose);

	// TODO: set up companion window
}

VrSys::~VrSys() {
	if (system)
		VR_Shutdown();
}

void VrSys::init(const umap<string, string>& sources, ivec2 windowSize) {
	shdController = new ShaderVrController(sources.at(ShaderVrController::fileVert), sources.at(ShaderVrController::fileFrag));
	shdModel = new ShaderVrModel(sources.at(ShaderVrModel::fileVert), sources.at(ShaderVrModel::fileFrag));
	shdWindow = new ShaderVrWindow(sources.at(ShaderVrWindow::fileVert), sources.at(ShaderVrWindow::fileFrag));

	VertexWindow verts[8] = {
		VertexWindow(vec2(-1.f, -1.f), vec2(0.f, 1.f)),
		VertexWindow(vec2(0.f, -1.f), vec2(1.f, 1.f)),
		VertexWindow(vec2(-1.f, 1.f), vec2(0.f, 0.f)),
		VertexWindow(vec2(0.f, 1.f), vec2(1.f, 0.f)),
		VertexWindow(vec2(0.f, -1.f), vec2(0.f, 1.f)),
		VertexWindow(vec2(1.f, -1.f), vec2(1.f, 1.f)),
		VertexWindow(vec2(0.f, 1.f), vec2(0.f, 0.f)),
		VertexWindow(vec2(1.f, 1.f), vec2(1.f, 0.f))
	};

	constexpr GLushort indices[companionWindowIndexSize] = { 0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6 };
	companionWindowSize = windowSize;

	glGenVertexArrays(1, &vaoCompanionWindow);
	glBindVertexArray(vaoCompanionWindow);

	glGenBuffers(1, &vboCompanionWindow);
	glBindBuffer(GL_ARRAY_BUFFER, vboCompanionWindow);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(VertexWindow), verts, GL_STATIC_DRAW);

	glGenBuffers(1, &eboCompanionWindow);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboCompanionWindow);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, companionWindowIndexSize * sizeof(GLushort), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexWindow), (void*)offsetof(VertexWindow, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexWindow), (void*)offsetof(VertexWindow, texCoord));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	createFrameBuffer(leftEye);
	createFrameBuffer(rightEye);
	if (!VRCompositor())
		throw std::runtime_error("Failed to initialize compositor");
}

void VrSys::free() {
	for (Model* it : models) {
		freeModel(it);
		delete it;
	}

	delete shdController;
	delete shdModel;
	delete shdWindow;

	glDeleteTextures(1, &leftEye.texRender);
	glDeleteRenderbuffers(1, &leftEye.rboDepth);
	glDeleteFramebuffers(1, &leftEye.fboRender);

	glDeleteTextures(1, &rightEye.texRender);
	glDeleteRenderbuffers(1, &rightEye.rboDepth);
	glDeleteFramebuffers(1, &rightEye.fboRender);

	glBindVertexArray(vaoController);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDeleteBuffers(1, &vboController);
	glDeleteVertexArrays(1, &vaoController);

	glBindVertexArray(vaoCompanionWindow);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDeleteBuffers(1, &vboCompanionWindow);
	glDeleteBuffers(1, &eboCompanionWindow);
	glDeleteVertexArrays(1, &vaoCompanionWindow);
}

void VrSys::createFrameBuffer(Framebuffer& framebuffer) {
	glGenFramebuffers(1, &framebuffer.fboRender);	// TODO: we can handle generating both at once
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fboRender);

	glGenRenderbuffers(1, &framebuffer.rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.rboDepth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, renderSize.x, renderSize.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer.rboDepth);

	glGenTextures(1, &framebuffer.texRender);
	glBindTexture(GL_TEXTURE_2D, framebuffer.texRender);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderSize.x, renderSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.texRender, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("incomplete framebuffer");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VrSys::handleInput() {
	for (VREvent_t event; system->PollNextEvent(&event, sizeof(event));)
		switch (event.eventType) {
		case VREvent_TrackedDeviceDeactivated:
			logInfo("device ", event.trackedDeviceIndex, " detached");
			break;
		case VREvent_TrackedDeviceUpdated:
			logInfo("device ", event.trackedDeviceIndex, " updated.");
		}

	VRActiveActionSet_t actionSet{};
	actionSet.ulActionSet = actionsetDemo;
	VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	//m_bShowCubes = !getDigitalActionState(actionHideCubes);

	if (VRInputValueHandle_t hapticDevice; getDigitalActionRisingEdge(actionTriggerHaptic, &hapticDevice))
		for (ControllerInfo& it : hands)
			if (hapticDevice == it.source)
				VRInput()->TriggerHapticVibrationAction(it.actionHaptic, 0.f, 1.f, 4.f, 1.f, k_ulInvalidInputValueHandle);

	vec2 m_vAnalogValue;
	if (InputAnalogActionData_t analogData; VRInput()->GetAnalogActionData(actionAnalongInput, &analogData, sizeof(analogData), k_ulInvalidInputValueHandle) == VRInputError_None && analogData.bActive)
		m_vAnalogValue = vec2(analogData.x, analogData.y);

	hands[handLeft].showController = true;
	hands[handRight].showController = true;
	if (VRInputValueHandle_t hideDevice; getDigitalActionState(actionHideThisController, &hideDevice))
		for (ControllerInfo& it : hands)
			if (hideDevice == it.source)
				it.showController = false;

	for (ControllerInfo& it : hands) {
		InputPoseActionData_t poseData;
		if (VRInput()->GetPoseActionDataForNextFrame(it.actionPose, TrackingUniverseStanding, &poseData, sizeof(poseData), k_ulInvalidInputValueHandle) != VRInputError_None || !poseData.bActive || !poseData.pose.bPoseIsValid)
			it.showController = false;
		else {
			it.pose = hmdToMat4(poseData.pose.mDeviceToAbsoluteTracking);
			if (InputOriginInfo_t originInfo; VRInput()->GetOriginTrackedDeviceInfo(poseData.activeOrigin, &originInfo, sizeof(originInfo)) == VRInputError_None && originInfo.trackedDeviceIndex != k_unTrackedDeviceIndexInvalid)
				if (string renderModelName = getTrackedDeviceString(originInfo.trackedDeviceIndex, Prop_RenderModelName_String); renderModelName != it.model->name)
					it.model = findOrLoadRenderModel(renderModelName.c_str());
		}
	}
}

void VrSys::draw() {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (system->IsInputAvailable())
		drawControllerAxes();
	drawStereoTargets();
	drawCompanionWindow();

	Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEye.texRender, TextureType_OpenGL, ColorSpace_Gamma };
	VRCompositor()->Submit(Eye_Left, &leftEyeTexture);
	Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEye.texRender, TextureType_OpenGL, ColorSpace_Gamma };
	VRCompositor()->Submit(Eye_Right, &rightEyeTexture);

	// Spew out the controller and pose count whenever they change.
	if (trackedControllerCount != trackedControllerCount_Last || validPoseCount != validPoseCount_Last) {
		validPoseCount_Last = validPoseCount;
		trackedControllerCount_Last = trackedControllerCount;
		logInfo("PoseCount:", validPoseCount, "(", poseClasses, ") Controllers:", trackedControllerCount);
	}

	// update pose
	VRCompositor()->WaitGetPoses(trackedDevicePose, k_unMaxTrackedDeviceCount, NULL, 0);
	validPoseCount = 0;
	poseClasses.clear();
	for (int i = 0; i < int(k_unMaxTrackedDeviceCount); ++i)
		if (trackedDevicePose[i].bPoseIsValid) {
			++validPoseCount;
			if (devicePose[i] = hmdToMat4(trackedDevicePose[i].mDeviceToAbsoluteTracking); !devClassChar[i])
				switch (system->GetTrackedDeviceClass(i)) {
				case TrackedDeviceClass_Invalid:
					devClassChar[i] = 'I';
					break;
				case TrackedDeviceClass_HMD:
					devClassChar[i] = 'H';
					break;
				case TrackedDeviceClass_Controller:
					devClassChar[i] = 'C';
					break;
				case TrackedDeviceClass_GenericTracker:
					devClassChar[i] = 'G';
					break;
				case TrackedDeviceClass_TrackingReference:
					devClassChar[i] = 'T';
					break;
				default:
					devClassChar[i] = '?';
				}
			poseClasses += devClassChar[i];
		}
	if (trackedDevicePose[k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		sysPose = glm::inverse(devicePose[k_unTrackedDeviceIndex_Hmd]);	// used to be .invert(); needs to be checked
}

void VrSys::drawControllerAxes() {
	controllerVertCount = 0;
	trackedControllerCount = 0;

	std::vector<float> vertData;
	for (ControllerInfo& it : hands) {
		if (it.showController)
			continue;

		vec4 center = it.pose * vec4(0.f, 0.f, 0.f, 1.f);
		for (int i = 0; i < 3; ++i) {
			vec3 color(0.f, 0.f, 0.f);
			vec4 point(0.f, 0.f, 0.f, 1.f);
			point[i] += 0.05f;
			color[i] = 1.0;
			point = it.pose * point;
			vertData.push_back(center.x);
			vertData.push_back(center.y);
			vertData.push_back(center.z);

			vertData.push_back(color.x);
			vertData.push_back(color.y);
			vertData.push_back(color.z);

			vertData.push_back(point.x);
			vertData.push_back(point.y);
			vertData.push_back(point.z);

			vertData.push_back(color.x);
			vertData.push_back(color.y);
			vertData.push_back(color.z);
			controllerVertCount += 2;
		}

		vec4 start = it.pose * vec4(0.f, 0.f, -0.02f, 1.f);
		vec4 end = it.pose * vec4(0.f, 0.f, -39.f, 1.f);
		vec3 color(0.92f, 0.92f, 0.71f);
		vertData.push_back(start.x); vertData.push_back(start.y); vertData.push_back(start.z);
		vertData.push_back(color.x); vertData.push_back(color.y); vertData.push_back(color.z);
		vertData.push_back(end.x); vertData.push_back(end.y); vertData.push_back(end.z);
		vertData.push_back(color.x); vertData.push_back(color.y); vertData.push_back(color.z);
		controllerVertCount += 2;
	}

	// Setup the VAO the first time through.
	if (!vaoController) {
		glGenVertexArrays(1, &vaoController);
		glBindVertexArray(vaoController);

		glGenBuffers(1, &vboController);
		glBindBuffer(GL_ARRAY_BUFFER, vboController);

		GLuint stride = 2 * 3 * sizeof(float);
		uintptr_t offset = 0;

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

		offset += sizeof(vec3);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

		glBindVertexArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, vboController);
	if (vertData.size() > 0)
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertData.size(), &vertData[0], GL_STREAM_DRAW);
}

void VrSys::drawStereoTargets() {
	for (Hmd_Eye eye = Eye_Left; eye <= Eye_Right; ++eye) {
		glViewport(0, 0, renderSize.x, renderSize.y);	// TODO: this needs to be handled in scene
		drawScene(Eye_Left);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void VrSys::drawScene(Hmd_Eye eye) {
	// SECTION this shit is ripped straight from Camera and needs to be cleaner
	//glUseProgram(*World::geom());
	//glUniformMatrix4fv(World::geom()->view, 1, GL_FALSE, glm::value_ptr(view));

	mat4 viewProj = getCurrentViewProjectionMatrix(eye);
	vec3 pos(0.f, 2.f, 0.f);
	mat4 guiMpv = viewProj * glm::translate(mat4(1.f), vec3(2.f, 1.f, 2.f));
	glUseProgram(*World::light());
	glUniformMatrix4fv(World::light()->pview, 1, GL_FALSE, glm::value_ptr(viewProj));
	glUniform3fv(World::light()->viewPos, 1, glm::value_ptr(pos));

	glUseProgram(*World::skybox());
	glUniformMatrix4fv(World::skybox()->pview, 1, GL_FALSE, glm::value_ptr(viewProj));
	glUniform3fv(World::skybox()->viewPos, 1, glm::value_ptr(pos));

	//glUseProgram(*World::geom());
	//glUniformMatrix4fv(World::geom()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	//glUseProgram(*World::ssao());
	//glUniform2f(World::ssao()->noiseScale, res.x / 4.f, res.y / 4.f);
	//glUniformMatrix4fv(World::ssao()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	//glUseProgram(*World::ssr());
	//glUniformMatrix4fv(World::ssr()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::sgui());
	glUniformMatrix4fv(World::sgui()->pviewModel, 1, GL_FALSE, glm::value_ptr(guiMpv));
	glUniform2f(World::sgui()->pview, float(World::window()->getGuiView().x) / 2.f, float(World::window()->getGuiView().y) / 2.f);
	// ENDSECTION

	World::scene()->draw(World::window()->mousePos(), eye == Eye_Left ? leftEye.fboRender : rightEye.fboRender);

	if (system->IsInputAvailable()) {
		glUseProgram(*shdController);
		glUniformMatrix4fv(shdController->matrix, 1, GL_FALSE, glm::value_ptr(viewProj));
		glBindVertexArray(vaoController);
		glDrawArrays(GL_LINES, 0, controllerVertCount);
		glBindVertexArray(0);
	}

	glUseProgram(*shdModel);
	for (Hand hid = handLeft; hid <= handRight; ++hid)
		if (hands[hid].showController && hands[hid].model) {
			viewProj = getCurrentViewProjectionMatrix(eye) * hands[hid].pose;
			glUniformMatrix4fv(shdModel->matrix, 1, GL_FALSE, glm::value_ptr(viewProj));
			drawModel(hands[hid].model);
		}
	glUseProgram(0);
}

void VrSys::drawCompanionWindow() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, companionWindowSize.x, companionWindowSize.y);

	glBindVertexArray(vaoCompanionWindow);
	glUseProgram(*shdWindow);

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEye.texRender);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, companionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEye.texRender);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, companionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void*)(uintptr_t)(companionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}

mat4 VrSys::getCurrentViewProjectionMatrix(Hmd_Eye eye) {
	switch (eye) {
	case Eye_Left:
		return leftProj * leftPos * sysPose;
	case Eye_Right:
		return rightProj * rightPos * sysPose;
	}
	return mat4(1.f);
}

bool VrSys::getDigitalActionRisingEdge(VRActionHandle_t action, VRInputValueHandle_t* devicePath) {
	InputDigitalActionData_t actionData;
	VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), k_ulInvalidInputValueHandle);
	if (devicePath) {
		*devicePath = k_ulInvalidInputValueHandle;
		if (actionData.bActive)
			if (InputOriginInfo_t originInfo; VRInputError_None == VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
				*devicePath = originInfo.devicePath;
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}

bool VrSys::getDigitalActionFallingEdge(VRActionHandle_t action, VRInputValueHandle_t* devicePath) {
	InputDigitalActionData_t actionData;
	VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), k_ulInvalidInputValueHandle);
	if (devicePath) {
		*devicePath = k_ulInvalidInputValueHandle;
		if (actionData.bActive)
			if (InputOriginInfo_t originInfo; VRInputError_None == VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
				*devicePath = originInfo.devicePath;
	}
	return actionData.bActive && actionData.bChanged && !actionData.bState;
}

bool VrSys::getDigitalActionState(VRActionHandle_t action, VRInputValueHandle_t* devicePath) {
	InputDigitalActionData_t actionData;
	VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), k_ulInvalidInputValueHandle);
	if (devicePath) {
		*devicePath = k_ulInvalidInputValueHandle;
		if (actionData.bActive)
			if (InputOriginInfo_t originInfo; VRInputError_None == VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
				*devicePath = originInfo.devicePath;
	}
	return actionData.bActive && actionData.bState;
}

string VrSys::getTrackedDeviceString(TrackedDeviceIndex_t unDevice, TrackedDeviceProperty prop, TrackedPropertyError* peError) {
	string txt;
	if (uint32 len = VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError); len > 1) {
		txt.resize(len - 1);
		VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, txt.data(), len, peError);
	}
	return txt;
}

VrSys::Model* VrSys::findOrLoadRenderModel(const char* renderModelName) {
	for (Model* it : models)
		if (!SDL_strcasecmp(it->name.c_str(), renderModelName))
			return it;

	// load the model if we didn't find one
	RenderModel_t* vrModel;
	EVRRenderModelError error;
	for (;;) {
		error = VRRenderModels()->LoadRenderModel_Async(renderModelName, &vrModel);
		if (error != VRRenderModelError_Loading)
			break;
		SDL_Delay(1);
	}
	if (error != VRRenderModelError_None) {
		logError("unable to load render model ", renderModelName, " - ", VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
		return nullptr;
	}

	RenderModel_TextureMap_t* vrTexture;
	for (;;) {
		error = VRRenderModels()->LoadTexture_Async(vrModel->diffuseTextureId, &vrTexture);
		if (error != VRRenderModelError_Loading)
			break;
		SDL_Delay(1);
	}
	if (error != VRRenderModelError_None) {
		logError("unable to load render texture id:", vrModel->diffuseTextureId, " for render model ", renderModelName);
		VRRenderModels()->FreeRenderModel(vrModel);
		return nullptr;
	}

	models.push_back(new Model);
	initModel(models.back(), vrModel, vrTexture, renderModelName);
	VRRenderModels()->FreeRenderModel(vrModel);
	VRRenderModels()->FreeTexture(vrTexture);
	return models.back();
}

void VrSys::initModel(Model* model, const RenderModel_t* vrModel, const RenderModel_TextureMap_t* vrTexture, const char* renderModelName) {
	model->name = renderModelName;
	model->count = vrModel->unTriangleCount * 3;

	glGenVertexArrays(1, &model->vao);
	glBindVertexArray(model->vao);

	glGenBuffers(1, &model->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RenderModel_Vertex_t) * vrModel->unVertexCount, vrModel->rVertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderModel_Vertex_t), (void*)offsetof(RenderModel_Vertex_t, vPosition));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderModel_Vertex_t), (void*)offsetof(RenderModel_Vertex_t, vNormal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderModel_Vertex_t), (void*)offsetof(RenderModel_Vertex_t, rfTextureCoord));

	glGenBuffers(1, &model->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * model->count, vrModel->rIndexData, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glGenTextures(1, &model->tex);
	glBindTexture(GL_TEXTURE_2D, model->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrTexture->unWidth, vrTexture->unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, vrTexture->rubTextureMapData);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

#ifndef OPENGLES
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);	// TODO: maybe remove this
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
}

void VrSys::freeModel(Model* model) {
	glBindVertexArray(vaoController);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glDeleteBuffers(1, &model->ebo);
	glDeleteBuffers(1, &model->vbo);
	glDeleteVertexArrays(1, &model->vao);
}

void VrSys::drawModel(Model* model) {
	glBindVertexArray(model->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model->tex);
	glDrawElements(GL_TRIANGLES, model->count, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

mat4 VrSys::hmdToMat4(const vr::HmdMatrix44_t& mat) {
	return glm::transpose(glm::make_mat4(reinterpret_cast<const float*>(&mat.m)));
}

mat4 VrSys::hmdToMat4(const HmdMatrix34_t& mat) {
	return mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.f
	);
}
#endif
