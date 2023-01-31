#include "scene.h"
#include "fileSys.h"
#include "inputSys.h"
#include "shaders.h"
#include "world.h"
#include "prog/board.h"
#include "prog/progs.h"
#include "utils/context.h"
#include "utils/layouts.h"

// FRAME SET

FrameSet::FrameSet(const Settings* sets, ivec2 res) {
	if (sets->ssao || sets->ssr) {
		constexpr array<GLenum, 3> attach = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glGenFramebuffers(1, &fboGeom);
		glBindFramebuffer(GL_FRAMEBUFFER, fboGeom);
		texPosition = makeTexture<GL_RGBA32F, GL_RGBA, GL_FLOAT, Shader::vposTexa>(res);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach[0], GL_TEXTURE_2D, texPosition, 0);
		texNormal = makeTexture<GL_RGBA32F, GL_RGBA, GL_FLOAT, Shader::normTexa>(res);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach[1], GL_TEXTURE_2D, texNormal, 0);
		if (sets->ssr) {
			texMatl = makeTexture<GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, Shader::matlTexa>(res);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attach[2], GL_TEXTURE_2D, texMatl, 0);
		}
		glDrawBuffers(attach.size() - !sets->ssr, attach.data());
		glReadBuffer(GL_NONE);

		glGenRenderbuffers(1, &rboGeom);
		glBindRenderbuffer(GL_RENDERBUFFER, rboGeom);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboGeom);
		checkFramebufferStatus("geometry buffer");
	}
	if (sets->ssao) {
		std::tie(fboSsao[0], texSsao[0]) = makeFramebuffer<GL_R16F, GL_RED, GL_FLOAT, Shader::ssao0Texa>(res, "SSAO buffer 0");
		std::tie(fboSsao[1], texSsao[1]) = makeFramebuffer<GL_R16F, GL_RED, GL_FLOAT, Shader::ssao1Texa>(res, "SSAO buffer 1");
	}
	if (sets->ssr) {
		std::tie(fboSsr[0], texSsr[0]) = makeFramebuffer<GL_RGBA16F, GL_RGBA, GL_FLOAT, Shader::ssr0Texa>(res, "SSR buffer 0");
		std::tie(fboSsr[1], texSsr[1]) = makeFramebuffer<GL_RGBA16F, GL_RGBA, GL_FLOAT, Shader::ssr1Texa>(res, "SSR buffer 1");
	}

	uint msaa = sets->getMsaa();
#ifndef OPENGLES
	if (msaa) {
		glGenFramebuffers(1, &fboLight[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, fboLight[1]);

		glActiveTexture(Shader::tmpTexa);
		glGenTextures(1, &texLight[1]);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texLight[1]);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaa, GL_RGBA16F, res.x, res.y, GL_TRUE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texLight[1], 0);

		glGenRenderbuffers(1, &rboLight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboLight);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboLight);
		checkFramebufferStatus("light buffer multisample");
	}
#endif
	glGenFramebuffers(1, &fboLight[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, fboLight[0]);
	texLight[0] = makeTexture<GL_RGBA16F, GL_RGBA, GL_FLOAT, Shader::sceneTexa>(res);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texLight[0], 0);
	glReadBuffer(GL_NONE);

	if (!msaa) {
		glGenRenderbuffers(1, &rboLight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboLight);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboLight);
	}
	checkFramebufferStatus("light buffer");

	if (sets->bloom)
		for (sizet i = 0; i < fboGauss.size(); ++i)
			std::tie(fboGauss[i], texGauss[i]) = makeFramebuffer<GL_RGBA16F, GL_RGBA, GL_FLOAT, Shader::gaussTexa>(res, "gauss buffer " + toStr(i));
}

void FrameSet::bindTextures() {
	glActiveTexture(Shader::vposTexa);
	glBindTexture(GL_TEXTURE_2D, texPosition);
	glActiveTexture(Shader::normTexa);
	glBindTexture(GL_TEXTURE_2D, texNormal);
	glActiveTexture(Shader::matlTexa);
	glBindTexture(GL_TEXTURE_2D, texMatl);
	glActiveTexture(Shader::ssao0Texa);
	glBindTexture(GL_TEXTURE_2D, texSsao[0]);
	glActiveTexture(Shader::ssao1Texa);
	glBindTexture(GL_TEXTURE_2D, texSsao[1]);
	glActiveTexture(Shader::ssr0Texa);
	glBindTexture(GL_TEXTURE_2D, texSsr[0]);
	glActiveTexture(Shader::ssr1Texa);
	glBindTexture(GL_TEXTURE_2D, texSsr[1]);
	glActiveTexture(Shader::sceneTexa);
	glBindTexture(GL_TEXTURE_2D, texLight[0]);
}

template <GLint iform, GLenum pform, GLenum type, GLenum active>
pair<GLuint, GLuint> FrameSet::makeFramebuffer(ivec2 res, string_view name) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLuint tex = makeTexture<iform, pform, type, active>(res);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glReadBuffer(GL_NONE);
	checkFramebufferStatus(name);
	return pair(fbo, tex);
}

template <GLint iform, GLenum pform, GLenum type, GLenum active>
GLuint FrameSet::makeTexture(ivec2 res) {
	GLuint tex;
	glActiveTexture(active);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, iform, res.x, res.y, 0, pform, type, nullptr);
	return tex;
}

void FrameSet::free() {
	glDeleteTextures(texGauss.size(), texGauss.data());
	glDeleteFramebuffers(fboGauss.size(), fboGauss.data());
	glDeleteRenderbuffers(1, &rboLight);
	glDeleteTextures(texLight.size(), texLight.data());
	glDeleteFramebuffers(fboLight.size(), fboLight.data());
	glDeleteTextures(texSsr.size(), texSsr.data());
	glDeleteFramebuffers(texSsr.size(), fboSsr.data());
	glDeleteTextures(texSsao.size(), texSsao.data());
	glDeleteFramebuffers(fboSsao.size(), fboSsao.data());
	glDeleteRenderbuffers(1, &rboGeom);
	glDeleteTextures(1, &texMatl);
	glDeleteTextures(1, &texNormal);
	glDeleteTextures(1, &texPosition);
	glDeleteFramebuffers(1, &fboGeom);
}

// CAMERA

#ifndef OPENVR
Camera::Camera(const vec3& position, const vec3& lookAt, float vfov, float pitchMax, float yawMax, ivec2 res) :
	pmax(pitchMax),
	ymax(yawMax),
	fov(vfov)
{
	updateProjection(res);
	setPos(position, lookAt);
}

void Camera::updateView() const {
	mat4 view = glm::lookAt(pos, lat, up);
	mat4 projview = proj * view;

	glUseProgram(*World::geom());
	glUniformMatrix4fv(World::geom()->view, 1, GL_FALSE, glm::value_ptr(view));

	glUseProgram(*World::light());
	glUniformMatrix4fv(World::light()->pview, 1, GL_FALSE, glm::value_ptr(projview));
	glUniform3fv(World::light()->viewPos, 1, glm::value_ptr(pos));

	glUseProgram(*World::skybox());
	glUniformMatrix4fv(World::skybox()->pview, 1, GL_FALSE, glm::value_ptr(projview));
	glUniform3fv(World::skybox()->viewPos, 1, glm::value_ptr(pos));
}

void Camera::updateProjection(vec2 res) {
	proj = glm::perspective(fov, res.x / res.y, znear, zfar);

	glUseProgram(*World::geom());
	glUniformMatrix4fv(World::geom()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::ssao());
	glUniform2f(World::ssao()->noiseScale, res.x / 4.f, res.y / 4.f);
	glUniformMatrix4fv(World::ssao()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::ssr());
	glUniformMatrix4fv(World::ssr()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::sgui());
	glUniform2f(World::sgui()->pview, res.x / 2.f, res.y / 2.f);
}

void Camera::setFov(float vfov, ivec2 res) {
	fov = vfov;
	updateProjection(res);
	updateView();
}

void Camera::setPos(const vec3& newPos, const vec3& newLat) {
	pos = newPos;
	lat = newLat;

	vec3 pvec = pos - lat, lvec = lat - center;
	pdst = glm::length(pvec);
	ldst = glm::length(vec2(lvec.x, lvec.z));
	defaultPdst = pdst;
	updateRotations(pvec, lvec);
	updateView();
}

void Camera::updateRotations(const vec3& pvec, const vec3& lvec) {
	prot = vec2(calcPitch(pvec, pdst), calcYaw(pvec, glm::length(vec2(pvec.x, pvec.z))));
	lyaw = calcYaw(lvec, ldst);
}

void Camera::rotate(vec2 dRot, float dYaw) {
	prot = vec2(std::clamp(prot.x - dRot.x, pmax, 0.f), std::clamp(prot.y + dRot.y, -ymax, ymax));
	pos = quat(vec3(prot, 0.f)) * vec3(0.f, 0.f, pdst);
	lat = quat(vec3(0.f, lyaw + dYaw, 0.f)) * vec3(0.f, 0.f, ldst);
	updateRotations(pos, lat);
	lat += center;
	pos += lat;
	updateView();
}

void Camera::zoom(int mov) {
	pdst = std::clamp(pdst - float(mov) / 2.f, defaultPdst - 8.f, defaultPdst + 20.f);
	pos = quat(vec3(prot, 0.f)) * vec3(0.f, 0.f, pdst) + lat;
	updateView();
}

vec3 Camera::direction(ivec2 mPos) const {
	vec2 res = World::window()->getScreenView();
	vec3 dir = glm::normalize(lat - pos);

	float vl = std::tan(fov / 2.f) * znear;
	float hl = vl * (res.x / res.y);
	vec3 h = glm::normalize(glm::cross(dir, up)) * hl;
	vec3 v = glm::normalize(glm::cross(h, dir)) * vl;

	res /= 2.f;
	vec2 m((float(mPos.x) - res.x) / res.x, -(float(mPos.y) - res.y) / res.y);
	return glm::normalize(dir * znear + h * m.x + v * m.y);
}

ivec2 Camera::screenPos(const vec3& pnt) const {
	vec2 res = vec2(World::window()->getScreenView()) / 2.f;
	vec4 clip = proj * glm::lookAt(pos, lat, up) * vec4(pnt, 1.f);
	return vec2((clip.x / clip.w + 1.f) * res.x, (1.f - clip.y / clip.w) * res.y);
}
#endif

// LIGHT

Light::Light(const Settings* sets) {
	if (sets->shadowRes) {
		glGenFramebuffers(1, &fboDepth);
		glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);

		glActiveTexture(ShaderLight::shadowTexa);
		glGenTextures(1, &texDepth);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texDepth);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
		for (uint i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, sets->shadowRes, sets->shadowRes, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, texDepth, 0);	// bind one to make the check happy
#ifdef OPENGLES
		GLenum none = GL_NONE;
		glDrawBuffers(1, &none);
#else
		glDrawBuffer(GL_NONE);
#endif
		glReadBuffer(GL_NONE);
		checkFramebufferStatus("shadow buffer");
	}

	glUseProgram(*World::light());
	glUniform1i(World::light()->optShadow, sets->getShadowOpt());
	glUniform3fv(World::light()->lightPos, 1, glm::value_ptr(pos));

	mat4 shadowProj = glm::perspective(glm::half_pi<float>(), 1.f, snear, sfar);
	mat4 shadowTransforms[6] = {
		shadowProj * lookAt(pos, pos + vec3(1.f, 0.f, 0.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(-1.f, 0.f, 0.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, -1.f, 0.f), vec3(0.f, 0.f, -1.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 0.f, 1.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 0.f, -1.f), vec3(0.f, -1.f, 0.f))
	};
	glUseProgram(*World::depth());
	glUniformMatrix4fv(World::depth()->pvTrans, 6, GL_FALSE, reinterpret_cast<float*>(shadowTransforms));
	glUniform3fv(World::depth()->lightPos, 1, glm::value_ptr(pos));
}

void Light::free() {
	glDeleteTextures(1, &texDepth);
	glDeleteFramebuffers(1, &fboDepth);
}

// CLICK STAMP

ClickStamp::ClickStamp(Interactable* interact, ScrollArea* scrollArea, ivec2 position, uint8 button) :
	inter(interact),
	area(scrollArea),
	pos(position),
	but(button)
{}

// KEYFRAME

Keyframe::Keyframe(float timeOfs, const optional<vec3>& position, const optional<quat>& rotation, const optional<vec3>& scale) :
	pos(position),
	scl(scale),
	rot(rotation),
	time(timeOfs)
{}

// ANIMATION

Animation::Animation(Object* obj, std::queue<Keyframe>&& keyframes) :
	kframes(std::move(keyframes)),
	begin(0.f, obj->getPos(), obj->getRot(), obj->getScl()),
	object(obj),
	useObject(true)
{}

#ifndef OPENVR
Animation::Animation(Camera* cam, std::queue<Keyframe>&& keyframes) :
	kframes(std::move(keyframes)),
	begin(0.f, cam->getPos(), std::nullopt, cam->getLat()),
	camera(cam),
	useObject(false)
{
	cam->state = Camera::State::animating;
}
#endif

bool Animation::tick(float dSec) {
	begin.time += dSec;
	if (float td = kframes.front().time > 0.f ? std::clamp(begin.time / kframes.front().time, 0.f, 1.f) : 1.f; useObject)
		object->setTrans(kframes.front().pos ? glm::mix(*begin.pos, *kframes.front().pos, td) : object->getPos(), kframes.front().rot ? glm::mix(*begin.rot, *kframes.front().rot, td) : object->getRot(), kframes.front().scl ? glm::mix(*begin.scl, *kframes.front().scl, td) : object->getScl());
#ifndef OPENVR
	else
		camera->setPos(kframes.front().pos ? glm::mix(*begin.pos, *kframes.front().pos, td) : camera->getPos(), kframes.front().lat ? glm::mix(*begin.lat, *kframes.front().lat, td) : camera->getLat());
#endif
	if (begin.time >= kframes.front().time) {
		float ovhead = begin.time - kframes.front().time;
		if (kframes.pop(); !kframes.empty()) {
			if (useObject)
				begin = Keyframe(0.f, object->getPos(), object->getRot(), object->getScl());
#ifndef OPENVR
			else
				begin = Keyframe(0.f, camera->getPos(), std::nullopt, camera->getLat());
#endif
			return tick(ovhead);
		}
#ifndef OPENVR
		if (!useObject)
			camera->state = Camera::State::stationary;
#endif
		return false;
	}
	return true;
}

void Animation::append(Animation& ani) {
	for (; !ani.kframes.empty(); ani.kframes.pop())
		kframes.push(ani.kframes.front());
}

// SCENE

Scene::Scene() :
#ifndef OPENVR
	camera(std::make_unique<Camera>(Camera::posSetup, Camera::latSetup, float(glm::radians(World::sets()->fov)), Camera::pmaxSetup, Camera::ymaxSetup, World::window()->getScreenView())),
#endif
	light(World::sets()),
	frames(World::sets(), World::window()->getScreenView())
{
	glGenVertexArrays(1, &wgtVao);
	wgtTops.initBuffer(wgtTopInsts.size());
}

Scene::~Scene() {
	delete layout;	// needs to happen before texCol gets freed
	setPtrVec(popups);
	setPtrVec(overlays);
	delete context;
	delete titleBar;
	texSet.free();
	texCol.free();
	for (Mesh& it : meshes)
		it.free();
	wgtTops.freeBuffer();
	frames.free();
	light.free();
	glDeleteVertexArrays(1, &wgtVao);
}

void Scene::draw(ivec2 mPos, GLuint finalFbo) {
	BoardObject* cbob = dynamic_cast<BoardObject*>(capture.inter);
	ivec2 res = World::window()->getScreenView();
	bool msaa = World::sets()->antiAliasing >= Settings::AntiAliasing::msaa2;
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (World::sets()->shadowRes) {
		glViewport(0, 0, World::sets()->shadowRes, World::sets()->shadowRes);
		glUseProgram(*World::depth());
		glBindFramebuffer(GL_FRAMEBUFFER, light.fboDepth);
		for (uint i = 0; i < 6; ++i) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, light.texDepth, 0);
			glUniform1ui(World::depth()->pvId, i);
			glClear(GL_DEPTH_BUFFER_BIT);
			for (Mesh& it : meshes)
				it.draw();
			if (cbob && cbob->getTopSolid())
				World::state()->getObjectDragMesh()->drawTop();
		}
		glViewport(0, 0, res.x, res.y);
	}

	if (World::sets()->ssao || World::sets()->ssr) {
		startDepthDraw(World::geom(), frames.fboGeom);
		for (Mesh& it : meshes)
			it.draw();
		if (cbob && cbob->getTopSolid())
			World::state()->getObjectDragMesh()->drawTop();
	}
	glBindVertexArray(wgtVao);
	if (World::sets()->ssao) {
		glDisable(GL_DEPTH_TEST);
		drawFrame(World::ssao(), frames.fboSsao[0]);
		drawFrame(World::mblur(), frames.fboSsao[1]);
		glEnable(GL_DEPTH_TEST);
	}

	startDepthDraw(World::skybox(), frames.fboLight[msaa]);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glEnable(GL_BLEND);
	glUseProgram(*World::light());
	for (Mesh& it : meshes)
		it.draw();
	glDisable(GL_DEPTH_TEST);
	if (cbob)
		World::state()->getObjectDragMesh()->drawTop();
	glDisable(GL_BLEND);

#ifndef OPENGLES
	if (msaa) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frames.fboLight[1]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frames.fboLight[0]);
		glBlitFramebuffer(0, 0, res.x, res.y, 0, 0, res.x, res.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
#endif

	glBindVertexArray(wgtVao);
	if (World::sets()->bloom) {
		drawFrame(World::brights(), frames.fboGauss[0]);

		glUseProgram(*World::gauss());
		glActiveTexture(Shader::gaussTexa);
		for (uint i = 0; i < 10; ++i) {
			bool vertical = i % 2;
			glBindFramebuffer(GL_FRAMEBUFFER, frames.fboGauss[!vertical]);
			glClear(GL_COLOR_BUFFER_BIT);
			glUniform1i(World::gauss()->horizontal, !vertical);
			glBindTexture(GL_TEXTURE_2D, frames.texGauss[vertical]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		glBindTexture(GL_TEXTURE_2D, frames.texGauss[0]);
	}
	if (World::sets()->ssr) {
		drawFrame(World::ssr(), frames.fboSsr[0]);
		drawFrame(World::ssrColor(), frames.fboSsr[1]);
		drawFrame(World::cblur(), frames.fboSsr[0]);
	}
	drawFrame(World::sfinal(), finalFbo);

	glEnable(GL_BLEND);
	glUseProgram(*World::sgui());	// TODO: if in VR render to the texture of an object
	layout->draw();
	for (Overlay* it : overlays)
		if (it->getShow())
			it->draw();
	for (Popup* it : popups)
		it->draw();
	if (context)
		context->draw();
	else {
		uint icnt = 0;
		if (Widget* wgt = dynamic_cast<Widget*>(capture.inter))
			icnt += wgt->setTopInstances(&wgtTopInsts[icnt]);
		if (Button* but = dynamic_cast<Button*>(select); but && World::input()->mouseLast)
			icnt += but->setTooltipInstances(&wgtTopInsts[icnt], mPos);
		if (icnt) {
			wgtTops.updateInstances(wgtTopInsts.data(), 0, icnt);
			wgtTops.drawInstances(icnt);
		}
	}

	if (titleBar) {
		vec2 hres = vec2(res) / 2.f;
		glViewport(0, res.y, res.x, World::window()->getTitleBarHeight());
		glUniform2f(World::sgui()->pview, hres.x, float(World::window()->getTitleBarHeight()) / 2.f);
		titleBar->draw();
		glUniform2fv(World::sgui()->pview, 1, glm::value_ptr(hres));
		glViewport(0, 0, res.x, res.y);
	}
}

void Scene::startDepthDraw(const Shader* shader, GLuint fbo) {
	glUseProgram(*shader);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene::drawFrame(const Shader* shader, GLuint fbo) {
	glUseProgram(*shader);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	for (Popup* it : popups)
		it->tick(dSec);
	for (Overlay* it : overlays)
		it->tick(dSec);
	if (context)
		context->tick(dSec);

	for (sizet i = 0; i < animations.size();) {
		if (animations[i].tick(dSec))
			++i;
		else
			animations.erase(animations.begin() + pdift(i));
	}
}

void Scene::onExternalResize() {
	resetFrames();
#ifdef OPENVR
	// TODO: update program uniforms
#else
	camera->updateProjection(World::window()->getScreenView());
	camera->updateView();
#endif
	onInternalResize();
}

void Scene::onInternalResize() {
	World::pgui()->calcSizes();
	layout->onResize(nullptr);
	for (Popup* it : popups)
		it->onResize(nullptr);
	for (Overlay* it : overlays)
		it->onResize(nullptr);
}

void Scene::onMouseMove(ivec2 pos, ivec2 mov, uint32 state) {
	updateSelect(pos);
	if (capture)
		capture->onDrag(pos, mov);
	else if (context)
		context->onMouseMove(pos);
	else if (state)
		World::state()->eventDrag(state);
}

void Scene::onMouseDown(ivec2 pos, uint8 but) {
	updateSelect(pos);
	if (cstamp.but)
		return;
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture.inter); popups.empty() && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	if (context && select != context)
		setContext(nullptr);

	cstamp = ClickStamp(select, getSelectedScrollArea(select), pos, but);
	if (cstamp.area)	// area goes first so widget can overwrite its capture
		cstamp.area->onHold(pos, but);
	if (cstamp.inter != cstamp.area)
		cstamp.inter->onHold(pos, but);
	if (!capture)		// can be set by previous onHold calls
		World::state()->eventDrag(SDL_BUTTON(but));
}

void Scene::onMouseUp(ivec2 pos, uint8 but) {
	updateSelect(pos);
	if (but != cstamp.but)
		return;
	if (capture)
		capture->onUndrag(pos, but);
	if (select && cstamp.inter == select && cursorInClickRange(pos))
		cstamp.inter->onClick(pos, but);
	if (!capture)
		World::state()->eventUndrag();
	cstamp = ClickStamp();
}

void Scene::onMouseWheel(ivec2 pos, ivec2 mov) {
	if (Interactable* box = context ? dynamic_cast<Context*>(select) : dynamic_cast<TextBox*>(select) ? select : getSelectedScrollArea(select))
		box->onScroll(pos, ivec2(mov.x, mov.y * btom<int>(World::sets()->invertWheel)) * scrollFactorWheel);
	else if (mov.y)
		World::state()->eventWheel(mov.y * btom<int>(!World::sets()->invertWheel));
}

void Scene::onMouseLeave(ivec2 pos) {
	if (capture)
		capture->onUndrag(pos, cstamp.but);
	else
		World::state()->eventUndrag();
	deselect();
	cstamp = ClickStamp();
}

void Scene::onCompose(string_view str) {
	if (capture) {
		capture->onCompose(str, capture.len);
		capture.len = str.length();
	}
}

void Scene::onText(string_view str) {
	if (capture) {
		capture->onText(str, capture.len);
		capture.len = 0;
	}
}

void Scene::onConfirm() {
	if (context)
		context->confirm();
	else if (!popups.empty() && (!popups.back()->defaultSelect || !select))
		World::prun(popups.back()->kcall, nullptr);
	else if (Slider* sl = dynamic_cast<Slider*>(select))
		sl->onHold(sl->sliderRect().pos(), SDL_BUTTON_LEFT);
	else if (Button* but = dynamic_cast<Button*>(select))
		but->onClick(but->position(), SDL_BUTTON_LEFT);
	else if (popups.empty())
		World::state()->eventEnter();
}

void Scene::onXbutConfirm() {
	if (context)
		context->confirm();
	else if (!popups.empty())
		World::prun(popups.back()->kcall, nullptr);
	else
		World::state()->eventFinish();
}

void Scene::onCancel() {
	if (context)
		setContext(nullptr);
	else if (!popups.empty())
		World::prun(popups.back()->ccall, nullptr);
	else
		World::state()->eventEscape();
}

void Scene::onXbutCancel() {
	if (context)
		setContext(nullptr);
	else if (!popups.empty())
		World::prun(popups.back()->ccall, nullptr);
	else
		World::state()->eventEscape();
}

void Scene::loadObjects() {
	meshes = FileSys::loadObjects(meshRefs);
	materials = FileSys::loadMaterials(matlRefs);
	World::light()->setMaterials(materials);
	World::ssr()->setMaterials(materials);
	World::sfinal()->setMaterials(materials);
}

void Scene::loadTextures(int recomTexSize) {
	int minUiIcon = recomTexSize;
	vector<TextureCol::Element> elems(pieceLim, TextureCol::Element(string(), nullptr, TextureCol::textPixFormat));
	texSet.init(FileSys::loadTextures(elems, minUiIcon, float(World::sets()->texScale) / 100.f));

	array<SDL_Surface*, pieceLim> icons = renderPieceIcons();
	for (sizet i = 0; i < icons.size(); ++i) {
		elems[i].name = pieceNames[i];
		elems[i].image = icons[i];
	}
	texColRefs = texCol.init(std::move(elems), recomTexSize, minUiIcon);
}

void Scene::reloadTextures() {
	texSet.free();
	texSet.init(FileSys::reloadTextures(float(World::sets()->texScale) / 100.f));
	setPieceIcons();
}

void Scene::setPieceIcons() {
	array<SDL_Surface*, pieceLim> icons = renderPieceIcons();
	for (sizet i = 0; i < icons.size(); ++i) {
		TexLoc tloc = wgtTex(pieceNames[i]);
		texCol.replace(tloc, icons[i]);
		texColRefs[pieceNames[i]] = tloc;
	}
}

void Scene::resetShadows() {
	light.free();
	light = Light(World::sets());
}

void Scene::resetFrames() {
	frames.free();
	frames = FrameSet(World::sets(), World::window()->getScreenView());
}

void Scene::resetLayouts() {
	// clear scene
	onMouseLeave(World::window()->mousePos());	// reset stamp and select
	setCapture(nullptr);
	delete layout;
	setPtrVec(popups);
	delete context;
	context = nullptr;
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	World::fonts()->clear();
#endif

	// set up new widgets
	std::tie(layout, firstSelect) = World::state()->createLayout();
	setPtrVec(overlays, World::state()->createOverlays());
	layout->postInit(nullptr);
	for (Overlay* it : overlays)
		it->postInit(nullptr);
	World::state()->updateTitleBar(World::window()->getTitleBarHeight());
	updateSelect();
}

void Scene::updateTooltips() {
	layout->updateTipTex();
	for (Popup* it : popups)
		it->updateTipTex();
	for (Overlay* it : overlays)
		it->updateTipTex();
}

void Scene::pushPopup(Popup* newPopup, Widget* newCapture) {
	deselect();	// clear select and capture in case of a dangling pointer
	setCapture(nullptr);
	popups.push_back(newPopup);
	popups.back()->postInit(nullptr);
	if (newCapture)
		newCapture->onClick(newCapture->position(), SDL_BUTTON_LEFT);

	if (popups.back()->defaultSelect && !World::input()->mouseLast)
		updateSelect(popups.back()->defaultSelect);
	else
		updateSelect();
}

void Scene::popPopup() {
	deselect();	// clear select and capture in case of a dangling pointer
	setCapture(nullptr);
	delete popups.back();
	popups.pop_back();
	updateSelect();
}

void Scene::setContext(Context* newContext) {
	if (context) {
		if (context->getParent() && !World::input()->mouseLast)
			updateSelect(context->getParent());
		else if (select == context)
			deselect();
		delete context;
	}
	context = newContext;
	updateSelect();
}

void Scene::setTitleBar(TitleBar* bar) {
	deselect();	// clear select and capture in case of a dangling pointer
	delete titleBar;
	if (titleBar = bar; titleBar)
		titleBar->postInit(nullptr);
	updateSelect();
}

void Scene::addAnimation(Animation&& anim) {
	if (vector<Animation>::iterator it = std::find(animations.begin(), animations.end(), anim); it == animations.end())
		animations.push_back(std::move(anim));
	else
		it->append(anim);
}

void Scene::delegateStamp(Interactable* inter) {
	cstamp.inter = inter;
	cstamp.area = getSelectedScrollArea(inter);
}

void Scene::setCapture(Interactable* inter, bool reset) {
	if (capture && reset)
		capture->onCancelCapture();
	capture = inter;
}

void Scene::navSelect(Direction dir) {
	World::input()->mouseLast = false;
	if (context)
		context->onNavSelect(dir);
	else if (popups.empty() || popups.back()->defaultSelect) {
		if (select)
			select->onNavSelect(dir);
		else
			updateSelect(popups.empty() ? firstSelect : popups.back()->defaultSelect);
	}
}

void Scene::updateSelect() {
	if (World::input()->mouseLast)
		updateSelect(World::window()->mousePos());
}

void Scene::updateSelect(Interactable* sel) {
	if (sel != select) {
		std::swap(select, sel);
		if (sel)
			sel->onUnhover();
		if (select)
			select->onHover();
	}
}

void Scene::deselect() {
	if (select) {
		Interactable* old = select;	// select must be nullptr during onUnhover because of checks
		select = nullptr;
		old->onUnhover();
	}
}

Interactable* Scene::getSelected(ivec2 mPos) {
	if (outRange(mPos, ivec2(0), World::window()->getScreenView()))
		return titleBar ? getTitleBarSelected(ivec2(mPos.x, mPos.y + World::window()->getTitleBarHeight())) : nullptr;
	if (context && context->rect().contains(mPos))
		return context;

	Layout* box = layout;
	if (!popups.empty())
		box = popups.back();
	else for (vector<Overlay*>::reverse_iterator it = overlays.rbegin(); it != overlays.rend(); ++it)
		if ((*it)->canInteract() && (*it)->rect().contains(mPos))
			box = *it;

	for (;;) {
		Rect frame = box->rect();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contains(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return dynamic_cast<Navigator*>(*it) ? World::game()->board->findObject(rayXZIsct(pickerRay(mPos))) : (*it)->selectable() ? *it : getScrollOrObject(mPos, *it);
		} else
			return getScrollOrObject(mPos, box);
	}
}

Interactable* Scene::getScrollOrObject(ivec2 mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return popups.empty() ? static_cast<Interactable*>(World::game()->board->findObject(rayXZIsct(pickerRay(mPos)))) : static_cast<Interactable*>(popups.back());
}

ScrollArea* Scene::findFirstScrollArea(Widget* wgt) {
	Layout* parent = dynamic_cast<Layout*>(wgt);
	if (wgt && !parent)
		parent = wgt->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

Interactable* Scene::getTitleBarSelected(ivec2 tmPos) {
	for (Layout* box = titleBar;;) {
		Rect frame = box->rect();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &tmPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contains(tmPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->selectable() ? *it : findFirstScrollArea(*it);
		} else
			return findFirstScrollArea(box);
	}
}

array<SDL_Surface*, pieceLim> Scene::renderPieceIcons() {
#ifdef OPENVR
	GLsizei res = GLsizei(float(World::window()->getGuiView().y) * GuiGen::iconSize);
#else
	GLsizei res = GLsizei(float(World::sets()->windowSizes().back().y) * GuiGen::iconSize);
#endif
	uint matl = matlId(Settings::colorNames[uint8(World::sets()->colorAlly)]);
	Settings::AntiAliasing oldAa = World::sets()->antiAliasing;
	uint16 oldShadowRes = World::sets()->shadowRes;
	bool oldSsao = World::sets()->ssao;
	bool oldBloom = World::sets()->bloom;
	bool oldSsr = World::sets()->ssr;
	World::sets()->antiAliasing = Settings::AntiAliasing::fxaa;
	World::sets()->shadowRes = 0;
	World::sets()->ssao = true;
	World::sets()->bloom = false;
	World::sets()->ssr = false;
	World::window()->setShaderOptions();

	array<SDL_Surface*, pieceLim> icons;
	for (sizet i = 0; i < icons.size(); ++i)
		if (icons[i] = SDL_CreateRGBSurfaceWithFormat(0, res, res, 32, SDL_PIXELFORMAT_BGRA32); !icons[i]) {
			std::for_each_n(icons.begin(), i, SDL_FreeSurface);
			throw std::runtime_error("Failed to render piece icons:"s + linend + SDL_GetError());
		}

#ifndef OPENVR	// TODO: use camera
	constexpr pair<vec3, float> camPositions[10] = {
		pair(vec3(1.52f, 1.1f, 0.9f), 0.37f),
		pair(vec3(1.55f, 1.1f, 0.95f), 0.39f),
		pair(vec3(0.9f, 1.1f, 1.3f), 0.32f),
		pair(vec3(1.f, 1.1f, 1.15f), 0.3f),
		pair(vec3(1.6f, 1.4f, 1.3f), 0.5f),
		pair(vec3(1.43f, 1.3f, 1.13f), 0.42f),
		pair(vec3(1.38f, 1.3f, 1.08f), 0.4f),
		pair(vec3(1.5f, 1.4f, 1.3f), 0.48f),
		pair(vec3(1.7f, 1.4f, 1.3f), 0.53f),
		pair(vec3(1.3f, 1.4f, 1.4f), 0.46f)
	};
	vec3 oldCamPos = camera->getPos();
	vec3 oldCamLat = camera->getLat();
	camera->updateProjection(vec2(float(res)));

	constexpr vec3 lightPos(1.f);
	glUseProgram(*World::light());
	glUniform3fv(World::light()->lightPos, 1, glm::value_ptr(lightPos));

	GLuint tex;
	glActiveTexture(Shader::tmpTexa);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
#ifdef OPENGLES
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#else
	glReadBuffer(GL_NONE);
#endif
	checkFramebufferStatus("piece icon buffer");

	FrameSet frameSet(World::sets(), ivec2(res));
	glViewport(0, 0, res, res);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glDisable(GL_BLEND);
	for (sizet i = 0; i < icons.size(); ++i) {
		camera->setPos(camPositions[i].first, vec3(0.f, camPositions[i].second, 0.f));
		renderModel(fbo, frameSet, pieceNames[i], matl);
#ifdef OPENGLES
		glReadPixels(0, 0, res, res, TextureCol::textPixFormat, GL_UNSIGNED_BYTE, icons[i]->pixels);
#else
		glGetTexImage(GL_TEXTURE_2D, 0, TextureCol::textPixFormat, GL_UNSIGNED_BYTE, icons[i]->pixels);
#endif
		invertImage(icons[i]);
	}
	glViewport(0, 0, World::window()->getScreenView().x, World::window()->getScreenView().y);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	frameSet.free();
	frames.bindTextures();
	glDeleteTextures(1, &tex);
	glDeleteFramebuffers(1, &fbo);

	glUseProgram(*World::light());
	glUniform3fv(World::light()->lightPos, 1, glm::value_ptr(Light::pos));
	camera->updateProjection(World::window()->getScreenView());
	camera->setPos(oldCamPos, oldCamLat);

	World::sets()->antiAliasing = oldAa;
	World::sets()->shadowRes = oldShadowRes;
	World::sets()->ssao = oldSsao;
	World::sets()->bloom = oldBloom;
	World::sets()->ssr = oldSsr;
	World::window()->setShaderOptions();
#endif
	return icons;
}

void Scene::renderModel(GLuint fbo, const FrameSet& frms, const string& name, uint matl) {
	Mesh* model = mesh(name);
	bool hadTop = model->hasTop();
	if (!hadTop)
		model->allocateTop(true);

	Mesh::Instance& insTop = model->getInstanceTop();
	Mesh::Instance oldTop = insTop;
	insTop.model = mat4(1.f);
	insTop.normat = mat3(1.f);
	insTop.diffuse = materials[matl].color;
	insTop.texid = uvec3(objTex("metal"), matl);
	insTop.show = true;
	model->updateInstanceDataTop();

	glEnable(GL_DEPTH_TEST);
	startDepthDraw(World::geom(), frms.fboGeom);
	model->drawTop();
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(wgtVao);
	drawFrame(World::ssao(), frms.fboSsao[0]);
	drawFrame(World::mblur(), frms.fboSsao[1]);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	startDepthDraw(World::light(), frms.fboLight[0]);
	model->drawTop();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glBindVertexArray(wgtVao);
	drawFrame(World::sfinal(), fbo);

	if (hadTop) {
		insTop = oldTop;
		model->updateInstanceDataTop();
	} else
		model->allocateTop(false);
}
