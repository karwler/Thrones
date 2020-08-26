#include "scene.h"
#include "fileSys.h"
#include "inputSys.h"
#include "world.h"
#include "prog/progs.h"

// CAMERA

Camera::Camera(const vec3& position, const vec3& lookAt, float pitchMax, float yawMax) :
	state(State::stationary),
	pmax(pitchMax),
	ymax(yawMax)
{
	updateProjection();
	setPos(position, lookAt);
}

void Camera::updateView() const {
	mat4 projview = proj * glm::lookAt(pos, lat, up);
	glUseProgram(*World::geom());
	glUniformMatrix4fv(World::geom()->pview, 1, GL_FALSE, glm::value_ptr(projview));
	glUniform3fv(World::geom()->viewPos, 1, glm::value_ptr(pos));
}

void Camera::updateProjection() {
	vec2 res = World::window()->getScreenView();
	proj = glm::perspective(glm::radians(fov), res.x / res.y, znear, zfar);

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

void Camera::rotate(const vec2& dRot, float dYaw) {
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

vec3 Camera::direction(const ivec2& mPos) const {
	vec2 res = World::window()->getScreenView();
	vec3 dir = glm::normalize(lat - pos);

	float vl = std::tan(glm::radians(fov / 2.f)) * znear;
	float hl = vl * (res.x / res.y);
	vec3 h = glm::normalize(glm::cross(dir, up)) * hl;
	vec3 v = glm::normalize(glm::cross(h, dir)) * vl;

	res /= 2.f;
	vec2 m((float(mPos.x) - res.x) / res.x, -(float(mPos.y) - res.y) / res.y);
	return glm::normalize(dir * znear + h * m.x + v * m.y);
}

ivec2 Camera::screenPos(const vec3& pnt) const {
	vec2 res = vec2(World::window()->getScreenView()) / 2.f;
	vec2 pix = vec2(proj * glm::lookAt(pos, lat, up) * vec4(pnt, 1.f)) / 10.f;	// TODO: why 10?
	return vec2((pix.x + 1.f) * res.x, (1.f - pix.y) * res.y);
}

// LIGHT

Light::Light(const vec3& position, const vec3& color, float ambiFac, float range) :
#ifndef OPENGLES
	depthMap(makeCubemap(World::sets()->shadowRes ? World::sets()->shadowRes : 1, depthTexa)),
	depthFrame(makeFramebufferDepth(depthMap)),
#endif
	pos(position),
	ambient(color * ambiFac),
	diffuse(color),
	linear(4.5f / range),
	quadratic(75.f / (range * range))
{
	glActiveTexture(GL_TEXTURE0);
	updateValues(range);
}

Light::~Light() {
#ifndef OPENGLES
	glDeleteFramebuffers(1, &depthFrame);
	glDeleteTextures(1, &depthMap);
#endif
}

void Light::updateValues(float range) {
	glUseProgram(*World::geom());
	glUniform3fv(World::geom()->lightPos, 1, glm::value_ptr(pos));
	glUniform3fv(World::geom()->lightAmbient, 1, glm::value_ptr(ambient));
	glUniform3fv(World::geom()->lightDiffuse, 1, glm::value_ptr(diffuse));
	glUniform1f(World::geom()->lightLinear, linear);
	glUniform1f(World::geom()->lightQuadratic, quadratic);
	glUniform1f(World::geom()->farPlane, range);

#ifndef OPENGLES
	mat4 shadowProj = glm::perspective(PI / 2.f, 1.f, snear, range);
	mat4 shadowTransforms[6] = {
		shadowProj * lookAt(pos, pos + vec3(1.f, 0.f, 0.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(-1.f, 0.f, 0.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, -1.f, 0.f), vec3(0.f, 0.f, -1.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 0.f, 1.f), vec3(0.f, -1.f, 0.f)),
		shadowProj * lookAt(pos, pos + vec3(0.f, 0.f, -1.f), vec3(0.f, -1.f, 0.f))
	};
	glUseProgram(*World::depth());
	glUniformMatrix4fv(World::depth()->shadowMats, 6, GL_FALSE, reinterpret_cast<GLfloat*>(shadowTransforms));
	glUniform3fv(World::depth()->lightPos, 1, glm::value_ptr(pos));
	glUniform1f(World::depth()->farPlane, range);
#endif
}

// CLICK STAMP

ClickStamp::ClickStamp(Interactable* interact, ScrollArea* scrollArea, const ivec2& position, uint8 button) :
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
	if (float td = kframes.front().time > 0.f ? std::clamp(begin.time / kframes.front().time, 0.f, 1.f) : 1.f; useObject) {
		if (kframes.front().pos)
			object->setPos(glm::mix(*begin.pos, *kframes.front().pos, td));
		if (kframes.front().rot)
			object->setRot(glm::mix(*begin.rot, *kframes.front().rot, td));
		if (kframes.front().scl)
			object->setScl(glm::mix(*begin.scl, *kframes.front().scl, td));
	} else
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
	camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup),
	select(nullptr),
	firstSelect(nullptr),
	capture(nullptr),
	light(vec3(Config::boardWidth / 2.f, 4.f, Config::boardWidth / 2.f), vec3(1.f, 0.98f, 0.92f), 0.8f)
{}

Scene::~Scene() {
	setPtrVec(overlays, {});
	for (auto& [name, tex] : texes)
		tex.free();
	for (auto& [name, mesh] : meshes)
		mesh.free();
}

void Scene::draw() {
#ifndef OPENGLES
	if (World::sets()->shadowRes) {
		glUseProgram(*World::depth());
		glViewport(0, 0, World::sets()->shadowRes, World::sets()->shadowRes);
		glBindFramebuffer(GL_FRAMEBUFFER, light.depthFrame);
		glClear(GL_DEPTH_BUFFER_BIT);
		World::program()->getGame()->board.drawObjectDepths();
		if (Object* obj = dynamic_cast<Object*>(capture))
			obj->drawTopDepth();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
#endif
	glUseProgram(*World::geom());
	glViewport(0, 0, World::window()->getScreenView().x, World::window()->getScreenView().y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	World::program()->getGame()->board.drawObjects();
	if (Object* obj = dynamic_cast<Object*>(capture))
		obj->drawTop();

	glUseProgram(*World::sgui());
	glBindVertexArray(World::sgui()->wrect.getVao());
	layout->draw();
	for (Overlay* it : overlays)
		if (it->getShow())
			it->draw();
	if (popup)
		popup->draw();
	if (context)
		context->draw();
	else {
		if (Widget* wgt = dynamic_cast<Widget*>(capture))
			wgt->drawTop();
		if (Button* but = dynamic_cast<Button*>(select); but && World::input()->mouseLast)
			but->drawTooltip();
	}
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
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
	World::pgui()->resize();
	camera.updateProjection();
	camera.updateView();
	layout->onResize();
	if (popup)
		popup->onResize();
	for (Overlay* it : overlays)
		it->onResize();
}

void Scene::onMouseMove(const ivec2& pos, const ivec2& mov, uint32 state) {
	updateSelect(pos);
	if (capture)
		capture->onDrag(pos, mov);
	else if (context)
		context->onMouseMove(pos);
	else if (state)
		World::state()->eventDrag(state);
}

void Scene::onMouseDown(const ivec2& pos, uint8 but) {
	updateSelect(pos);
	if (cstamp.but)
		return;
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
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

void Scene::onMouseUp(const ivec2& pos, uint8 but) {
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

void Scene::onMouseWheel(const ivec2& mov) {
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

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str);
}

void Scene::onConfirm() {
	if (context)
		context->confirm();
	else if (popup && (!popup->defaultSelect || !select))
		World::prun(popup->kcall, nullptr);
	else if (Slider* sl = dynamic_cast<Slider*>(select))
		sl->onHold(sl->sliderRect().pos(), SDL_BUTTON_LEFT);
	else if (Button* but = dynamic_cast<Button*>(select))
		but->onClick(but->position(), SDL_BUTTON_LEFT);
	else if (!popup)
		World::state()->eventEnter();
}

void Scene::onXbutConfirm() {
	if (context)
		context->confirm();
	else if (popup)
		World::prun(popup->kcall, nullptr);
	else
		World::state()->eventFinish();
}

void Scene::onCancel() {
	if (context)
		setContext(nullptr);
	else if (popup)
		World::prun(popup->ccall, nullptr);
	else
		World::state()->eventEscape();
}

void Scene::onXbutCancel() {
	if (context)
		setContext(nullptr);
	else if (popup)
		World::prun(popup->ccall, nullptr);
	else
		World::state()->eventEscape();
}

void Scene::loadObjects() {
	glUseProgram(*World::geom());
	meshes = FileSys::loadObjects();
	matls = FileSys::loadMaterials();
}

void Scene::loadTextures() {
	texes = FileSys::loadTextures(World::sets()->texScale);
}

void Scene::reloadTextures() {
	FileSys::reloadTextures(texes, World::sets()->texScale);
}

void Scene::resetShadows() {
#ifndef OPENGLES
	glUseProgram(*World::geom());
	loadCubemap(light.depthMap, World::sets()->shadowRes ? World::sets()->shadowRes : 1, Light::depthTexa);
	glActiveTexture(GL_TEXTURE0);
#endif
}

void Scene::reloadShader() {
	World::window()->reloadGeom();
	camera.updateProjection();
	camera.updateView();
	light.updateValues();
}

void Scene::resetLayouts() {
	// clear scene
	onMouseLeave();	// reset stamp and select
	setCapture(nullptr);
	popup.reset();
	context.reset();
	World::fonts()->clear();

	// set up new widgets
	layout = World::state()->createLayout(firstSelect);
	setPtrVec(overlays, World::state()->createOverlays());
	layout->postInit();
	for (Overlay* it : overlays)
		it->postInit();
	updateSelect();
}

void Scene::setPopup(uptr<Popup>&& newPopup, Widget* newCapture) {
	deselect();	// clear select and capture in case of a dangling pointer
	setCapture(nullptr);
	if (popup = std::move(newPopup); popup)
		popup->postInit();
	if (newCapture)
		newCapture->onClick(newCapture->position(), SDL_BUTTON_LEFT);

	if (popup && popup->defaultSelect && !World::input()->mouseLast)
		updateSelect(popup->defaultSelect);
	else
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
	else if (!popup || popup->defaultSelect) {
		if (select)
			select->onNavSelect(dir);
		else
			updateSelect(popup ? popup->defaultSelect : firstSelect);
	}
}

void Scene::updateSelect() {
	if (World::input()->mouseLast)
		updateSelect(mousePos());
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

Interactable* Scene::getSelected(const ivec2& mPos) {
	if (outRange(mPos, ivec2(0), World::window()->getScreenView()))
		return nullptr;
	if (context && context->rect().contain(mPos))
		return context.get();

	Layout* box = layout.get();
	if (popup)
		box = popup.get();
	else for (vector<Overlay*>::reverse_iterator it = overlays.rbegin(); it != overlays.rend(); ++it)
		if ((*it)->canInteract() && (*it)->rect().contain(mPos))
			box = *it;

	for (;;) {
		Rect frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return dynamic_cast<Navigator*>(*it) ? World::game()->board.findObject(rayXZIsct(pickerRay(mPos))) : (*it)->selectable() ? *it : getScrollOrObject(mPos, *it);
		} else
			return getScrollOrObject(mPos, box);
	}
}

Interactable* Scene::getScrollOrObject(const ivec2& mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return !popup ? static_cast<Interactable*>(World::game()->board.findObject(rayXZIsct(pickerRay(mPos)))) : static_cast<Interactable*>(popup.get());
}

ScrollArea* Scene::findFirstScrollArea(Widget* wgt) {
	Layout* parent = dynamic_cast<Layout*>(wgt);
	if (wgt && !parent)
		parent = wgt->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}
