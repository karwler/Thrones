#include "scene.h"
#include "fileSys.h"
#include "inputSys.h"
#include "shaders.h"
#include "world.h"
#include "prog/board.h"
#include "prog/progs.h"

// FRAME

Frame::Frame() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, vlength, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(Shader::uvloc);
	glVertexAttribPointer(Shader::uvloc, vlength, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(vlength * sizeof(*vertices)));
}

Frame::~Frame() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::vpos);
	glDisableVertexAttribArray(Shader::uvloc);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

// SKYBOX

Skybox::Skybox() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, vlength, GL_FLOAT, GL_FALSE, vlength * sizeof(*vertices), reinterpret_cast<void*>(0));
}

Skybox::~Skybox() {
	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::vpos);
	glDisableVertexAttribArray(Shader::uvloc);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

// FRAME SET

FrameSet::FrameSet(const Settings* sets, ivec2 res) {
	if (sets->ssao) {
		constexpr GLenum attach[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glGenFramebuffers(1, &fboGeom);
		glBindFramebuffer(GL_FRAMEBUFFER, fboGeom);
		texPosition = makeTexture(res, GL_RGBA32F, Shader::vposTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPosition, 0);
		texNormal = makeTexture(res, GL_RGBA32F, Shader::normTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texNormal, 0);
		glDrawBuffers(2, attach);
		glReadBuffer(GL_NONE);

		glGenRenderbuffers(1, &rboGeom);
		glBindRenderbuffer(GL_RENDERBUFFER, rboGeom);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboGeom);
		checkFramebufferStatus("geometry buffer");

		fboSsao = makeFramebuffer(texSsao, res, GL_R16F, Shader::ssaoTexa, "SSAO buffer");
		fboBlur = makeFramebuffer(texBlur, res, GL_R16F, Shader::blurTexa, "blur buffer");
	}

#ifndef OPENGLES
	if (sets->msamples) {
		glGenFramebuffers(1, &fboLight[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, fboLight[1]);

		glActiveTexture(Shader::texa);
		glGenTextures(1, &texLight[1]);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texLight[1]);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sets->msamples, GL_RGBA16F, res.x, res.y, GL_TRUE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texLight[1], 0);

		glGenRenderbuffers(1, &rboLight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboLight);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, sets->msamples, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboLight);
		checkFramebufferStatus("light buffer multisample");
	}
#endif
	glGenFramebuffers(1, &fboLight[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, fboLight[0]);
	texLight[0] = makeTexture(res, GL_RGBA16F, Shader::sceneTexa);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texLight[0], 0);
	glReadBuffer(GL_NONE);

	if (!sets->msamples) {
		glGenRenderbuffers(1, &rboLight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboLight);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboLight);
	}
	checkFramebufferStatus("light buffer");

	if (sets->bloom)
		for (sizet i = 0; i < fboGauss.size(); ++i)
			fboGauss[i] = makeFramebuffer(texGauss[i], res, GL_RGBA16F, Shader::texa, "gauss buffer " + toStr(i));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(Shader::texa);
}

GLuint FrameSet::makeFramebuffer(GLuint& tex, ivec2 res, GLint iform, GLenum active, const string& name) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	tex = makeTexture(res, iform, active);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glReadBuffer(GL_NONE);
	checkFramebufferStatus(name);
	return fbo;
}

GLuint FrameSet::makeTexture(ivec2 res, GLint iform, GLenum active) {
	GLuint tex;
	glActiveTexture(active);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#ifdef OPENGLES
	glTexStorage2D(GL_TEXTURE_2D, 0, iform, res.x, res.y);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, iform, res.x, res.y, 0, GL_RGBA, GL_FLOAT, nullptr);
#endif
	return tex;
}

void FrameSet::free() {
	glDeleteTextures(texGauss.size(), texGauss.data());
	glDeleteFramebuffers(fboGauss.size(), fboGauss.data());
	glDeleteRenderbuffers(1, &rboLight);
	glDeleteTextures(texLight.size(), texLight.data());
	glDeleteFramebuffers(fboLight.size(), fboLight.data());
	glDeleteTextures(1, &texBlur);
	glDeleteFramebuffers(1, &fboBlur);
	glDeleteTextures(1, &texSsao);
	glDeleteFramebuffers(1, &fboSsao);
	glDeleteRenderbuffers(1, &rboGeom);
	glDeleteTextures(1, &texNormal);
	glDeleteTextures(1, &texPosition);
	glDeleteFramebuffers(1, &fboGeom);
}

// CAMERA

Camera::Camera(const vec3& position, const vec3& lookAt, float pitchMax, float yawMax) :
	pmax(pitchMax),
	ymax(yawMax)
{
	updateProjection();
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

void Camera::updateProjection() {
	vec2 res = World::window()->getScreenView();
	vec2 nscale = res / 4.f;
	proj = glm::perspective(fov, res.x / res.y, znear, zfar);

	glUseProgram(*World::geom());
	glUniformMatrix4fv(World::geom()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::ssao());
	glUniform2fv(World::ssao()->noiseScale, 1, glm::value_ptr(nscale));
	glUniformMatrix4fv(World::ssao()->proj, 1, GL_FALSE, glm::value_ptr(proj));

	glUseProgram(*World::light());
	glUniform2fv(World::light()->screenSize, 1, glm::value_ptr(res));

	res /= 2.f;
	glUseProgram(*World::sgui());
	glUniform2fv(World::sgui()->pview, 1, glm::value_ptr(res));
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
	vec2 pix = vec2(proj * glm::lookAt(pos, lat, up) * vec4(pnt, 1.f)) / 10.f;	// idk why 10
	return vec2((pix.x + 1.f) * res.x, (1.f - pix.y) * res.y);
}

// LIGHT

Light::Light(GLsizei res, const vec3& position, const vec3& color, float ambiFac, float range) :
	pos(position),
	ambient(color * ambiFac),
	diffuse(color),
	linear(4.5f / range),
	quadratic(75.f / (range * range)),
	farPlane(range)
{
	if (res) {
		glGenFramebuffers(1, &fboDepth);
		glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);

		glActiveTexture(ShaderLight::depthTexa);
		glGenTextures(1, &texDepth);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texDepth);
		for (uint i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, res, res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, texDepth, 0);	// bind one to make the check happy
#ifdef OPENGLES
		GLenum none = GL_NONE;
		glDrawBuffers(1, &none);
#else
		glDrawBuffer(GL_NONE);
#endif
		glReadBuffer(GL_NONE);
		checkFramebufferStatus("shadow buffer");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(Shader::texa);
	}
	updateValues();
}

void Light::free() {
	glDeleteTextures(1, &texDepth);
	glDeleteFramebuffers(1, &fboDepth);
}

void Light::updateValues() {
	glUseProgram(*World::light());
	glUniform1f(World::light()->farPlane, farPlane);
	glUniform3fv(World::light()->lightPos, 1, glm::value_ptr(pos));
	glUniform3fv(World::light()->lightAmbient, 1, glm::value_ptr(ambient));
	glUniform3fv(World::light()->lightDiffuse, 1, glm::value_ptr(diffuse));
	glUniform1f(World::light()->lightLinear, linear);
	glUniform1f(World::light()->lightQuadratic, quadratic);

	mat4 shadowProj = glm::perspective(glm::half_pi<float>(), 1.f, snear, farPlane);
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
	glUniform1f(World::depth()->farPlane, farPlane);
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

Animation::Animation(Camera* cam, std::queue<Keyframe>&& keyframes) :
	kframes(std::move(keyframes)),
	begin(0.f, cam->getPos(), std::nullopt, cam->getLat()),
	camera(cam),
	useObject(false)
{
	cam->state = Camera::State::animating;
}

bool Animation::tick(float dSec) {
	begin.time += dSec;
	if (float td = kframes.front().time > 0.f ? std::clamp(begin.time / kframes.front().time, 0.f, 1.f) : 1.f; useObject)
		object->setTrans(kframes.front().pos ? glm::mix(*begin.pos, *kframes.front().pos, td) : object->getPos(), kframes.front().rot ? glm::mix(*begin.rot, *kframes.front().rot, td) : object->getRot(), kframes.front().scl ? glm::mix(*begin.scl, *kframes.front().scl, td) : object->getScl());
	else
		camera->setPos(kframes.front().pos ? glm::mix(*begin.pos, *kframes.front().pos, td) : camera->getPos(), kframes.front().lat ? glm::mix(*begin.lat, *kframes.front().lat, td) : camera->getLat());

	if (begin.time >= kframes.front().time) {
		float ovhead = begin.time - kframes.front().time;
		if (kframes.pop(); !kframes.empty()) {
			begin = Keyframe(0.f, useObject ? object->getPos() : camera->getPos(), useObject ? object->getRot() : quat(), useObject ? object->getScl() : camera->getLat());
			return tick(ovhead);
		}
		if (!useObject)
			camera->state = Camera::State::stationary;
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
	light(World::sets()->shadowRes),
	frames(World::sets(), World::window()->getScreenView())
{}

Scene::~Scene() {
	setPtrVec(popups);
	setPtrVec(overlays);
	texSet.free();
	for (auto& [name, tex] : texes)
		tex.free();
	for (Mesh& it : meshes)
		it.free();
	frames.free();
	light.free();
}

void Scene::draw() {
	BoardObject* cbob = dynamic_cast<BoardObject*>(capture.inter);
	ivec2 res = World::window()->getScreenView();
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
			if (cbob && cbob->getTopDepth())
				World::state()->getObjectDragMesh()->drawTop();
		}
		glViewport(0, 0, res.x, res.y);
	}

	if (World::sets()->ssao) {
		glDisable(GL_BLEND);
		glUseProgram(*World::geom());
		glBindFramebuffer(GL_FRAMEBUFFER, frames.fboGeom);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (Mesh& it : meshes)
			it.draw();
		if (cbob && cbob->getTopDepth())
			World::state()->getObjectDragMesh()->drawTop();

		glUseProgram(*World::ssao());
		glBindFramebuffer(GL_FRAMEBUFFER, frames.fboSsao);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(scrFrame.getVao());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, Frame::corners);

		glUseProgram(*World::blur());
		glBindFramebuffer(GL_FRAMEBUFFER, frames.fboBlur);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, Frame::corners);
		glEnable(GL_BLEND);
	}

#ifdef OPENGLES
	glBindFramebuffer(GL_FRAMEBUFFER, frames.fboLight[0]);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, frames.fboLight[World::sets()->msamples ? 1 : 0]);
#endif
	glUseProgram(*World::skybox());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(skybox.getVao());
	glDrawArrays(GL_TRIANGLES, 0, Skybox::corners);

	glUseProgram(*World::light());
	for (Mesh& it : meshes)
		it.draw();
	if (cbob) {
		if (cbob->getTopDepth())
			World::state()->getObjectDragMesh()->drawTop();
		else {
			glDisable(GL_DEPTH_TEST);
			World::state()->getObjectDragMesh()->drawTop();
			glEnable(GL_DEPTH_TEST);
		}
	}
#ifndef OPENGLES
	if (World::sets()->msamples) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frames.fboLight[1]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frames.fboLight[0]);
		glBlitFramebuffer(0, 0, res.x, res.y, 0, 0, res.x, res.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
#endif

	if (World::sets()->bloom) {
		glUseProgram(*World::brights());
		glBindFramebuffer(GL_FRAMEBUFFER, frames.fboGauss[0]);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, Frame::corners);

		glUseProgram(*World::gauss());
		for (uint i = 0; i < 10; ++i) {
			bool vertical = i % 2;
			glBindFramebuffer(GL_FRAMEBUFFER, frames.fboGauss[!vertical]);
			glClear(GL_COLOR_BUFFER_BIT);
			glUniform1i(World::gauss()->horizontal, !vertical);
			glBindTexture(GL_TEXTURE_2D, frames.texGauss[vertical]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, Frame::corners);
		}
	}
	glUseProgram(*World::sfinal());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(scrFrame.getVao());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, Frame::corners);

	glUseProgram(*World::sgui());
	glBindVertexArray(World::window()->getWrect()->getVao());
	layout->draw();
	for (Overlay* it : overlays)
		if (it->getShow())
			it->draw();
	for (Popup* it : popups)
		it->draw();
	if (context)
		context->draw();
	else {
		if (Widget* wgt = dynamic_cast<Widget*>(capture.inter))
			wgt->drawTop();
		if (Button* but = dynamic_cast<Button*>(select); but && World::input()->mouseLast)
			but->drawTooltip();
	}

	if (titleBar) {
		vec2 hres = res / 2;
		glViewport(0, res.y, res.x, World::window()->getTitleBarHeight());
		glUniform2f(World::sgui()->pview, hres.x, float(World::window()->getTitleBarHeight() / 2));
		titleBar->draw();
		glUniform2fv(World::sgui()->pview, 1, glm::value_ptr(hres));
		glViewport(0, 0, res.x, res.y);
	}
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

void Scene::onResize() {
	resetFrames();
	camera.updateProjection();
	camera.updateView();
	layout->onResize();
	for (Popup* it : popups)
		it->onResize();
	for (Overlay* it : overlays)
		it->onResize();
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
	if (context && select != context.get())
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
		capture->onUndrag(but);
	if (select && cstamp.inter == select && cursorInClickRange(pos))
		cstamp.inter->onClick(pos, but);
	if (!capture)
		World::state()->eventUndrag();
	cstamp = ClickStamp();
}

void Scene::onMouseWheel(ivec2 mov) {
	if (Interactable* box = context ? dynamic_cast<Context*>(select) : dynamic_cast<TextBox*>(select) ? select : getSelectedScrollArea(select))
		box->onScroll(ivec2(mov.x, mov.y * btom<int>(World::sets()->invertWheel)) * scrollFactorWheel);
	else if (mov.y)
		World::state()->eventWheel(mov.y * btom<int>(!World::sets()->invertWheel));
}

void Scene::onMouseLeave() {
	if (capture)
		capture->onUndrag(cstamp.but);
	else
		World::state()->eventUndrag();
	deselect();
	cstamp = ClickStamp();
}

void Scene::onCompose(const char* str) {
	if (capture)
		capture->onCompose(str, capture.len);
}

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str, capture.len);
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
	matls = FileSys::loadMaterials();
}

void Scene::loadTextures() {
	texSet = FileSys::loadTextures(texes, World::sets()->texScale);
}

void Scene::reloadTextures() {
	texSet.free();
	texSet = FileSys::reloadTextures(texes, World::sets()->texScale);
}

void Scene::reloadShader() {
	World::window()->reloadVaryingShaders();
	camera.updateProjection();
	camera.updateView();
	light.updateValues();
}

void Scene::resetShadows() {
	light.free();
	light = Light(World::sets()->shadowRes);
}

void Scene::resetFrames() {
	frames.free();
	frames = FrameSet(World::sets(), World::window()->getScreenView());
}

void Scene::resetLayouts() {
	// clear scene
	onMouseLeave();	// reset stamp and select
	setCapture(nullptr);
	setPtrVec(popups);
	context.reset();
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	World::fonts()->clear();
#endif

	// set up new widgets
	layout = World::state()->createLayout(firstSelect);
	setPtrVec(overlays, World::state()->createOverlays());
	layout->postInit();
	for (Overlay* it : overlays)
		it->postInit();
	World::state()->updateTitleBar();
	updateSelect();
}

void Scene::updateTooltips() {
	layout->updateTipTex();
	for (Popup* it : popups)
		it->updateTipTex();
	for (Overlay* it : overlays)
		it->updateTipTex();
}

void Scene::pushPopup(uptr<Popup>&& newPopup, Widget* newCapture) {
	deselect();	// clear select and capture in case of a dangling pointer
	setCapture(nullptr);
	popups.push_back(newPopup.release());
	popups.back()->postInit();
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

void Scene::setContext(uptr<Context>&& newContext) {
	if (context) {
		if (context->getParent() && !World::input()->mouseLast)
			updateSelect(context->getParent());
		else if (select == context.get())
			deselect();
	}
	context = std::move(newContext);
	updateSelect();
}

void Scene::setTitleBar(uptr<TitleBar>&& bar) {
	deselect();	// clear select and capture in case of a dangling pointer
	if (titleBar = std::move(bar); titleBar)
		titleBar->postInit();
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
		if (select)
			select->onUnhover();
		if (select = sel; select)
			select->onHover();
	}
}

void Scene::deselect() {
	if (select) {
		select->onUnhover();
		select = nullptr;
	}
}

Interactable* Scene::getSelected(ivec2 mPos) {
	if (outRange(mPos, ivec2(0), World::window()->getScreenView()))
		return titleBar ? getTitleBarSelected(ivec2(mPos.x, mPos.y + World::window()->getTitleBarHeight())) : nullptr;
	if (context && context->rect().contain(mPos))
		return context.get();

	Layout* box = layout.get();
	if (!popups.empty())
		box = popups.back();
	else for (vector<Overlay*>::reverse_iterator it = overlays.rbegin(); it != overlays.rend(); ++it)
		if ((*it)->canInteract() && (*it)->rect().contain(mPos))
			box = *it;

	for (;;) {
		Rect frame = box->rect();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
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
	for (Layout* box = titleBar.get();;) {
		Rect frame = box->rect();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &tmPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(tmPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->selectable() ? *it : findFirstScrollArea(*it);
		} else
			return findFirstScrollArea(box);
	}
}
