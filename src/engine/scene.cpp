#include "world.h"

// CAMERA

Camera::Camera(const vec3& position, const vec3& lookAt, float pitchMax, float yawMax) :
	moving(false),
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
	glUseProgram(*World::gui());
	glUniform2fv(World::gui()->pview, 1, glm::value_ptr(res));
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
	depthFrame(makeFramebufferNodraw(GL_DEPTH_ATTACHMENT, depthMap)),
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

Keyframe::Keyframe(float timeOfs, Change changes, const vec3& position, const quat& rotation, const vec3& scale) :
	pos(position),
	scl(scale),
	rot(rotation),
	time(timeOfs),
	change(changes)
{}

// ANIMATION

Animation::Animation(Object* obj, std::queue<Keyframe>&& keyframes) :
	kframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, obj->getPos(), obj->getRot(), obj->getScl()),
	object(obj),
	useObject(true)
{}

Animation::Animation(Camera* cam, std::queue<Keyframe>&& keyframes) :
	kframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, cam->getPos(), quat(), cam->getLat()),
	camera(cam),
	useObject(false)
{
	cam->moving = true;
}

bool Animation::tick(float dSec) {
	begin.time += dSec;
	if (float td = kframes.front().time > 0.f ? std::clamp(begin.time / kframes.front().time, 0.f, 1.f) : 1.f; useObject) {
		if (kframes.front().change & Keyframe::CHG_POS)
			object->setPos(glm::mix(begin.pos, kframes.front().pos, td));
		if (kframes.front().change & Keyframe::CHG_ROT)
			object->setRot(glm::mix(begin.rot, kframes.front().rot, td));
		if (kframes.front().change & Keyframe::CHG_SCL)
			object->setScl(glm::mix(begin.scl, kframes.front().scl, td));
	} else
		camera->setPos(kframes.front().change & Keyframe::CHG_POS ? glm::mix(begin.pos, kframes.front().pos, td) : camera->getPos(), kframes.front().change & Keyframe::CHG_LAT ? glm::mix(begin.scl, kframes.front().scl, td) : camera->getLat());

	if (begin.time >= kframes.front().time) {
		float ovhead = begin.time - kframes.front().time;
		if (kframes.pop(); !kframes.empty()) {
			begin = Keyframe(0.f, Keyframe::CHG_NONE, useObject ? object->getPos() : camera->getPos(), useObject ? object->getRot() : quat(), useObject ? object->getScl() : camera->getLat());
			return tick(ovhead);
		}
		if (!useObject)
			camera->moving = false;
		return false;
	}
	return true;
}

void Animation::append(Animation& ani) {
	while (!ani.kframes.empty()) {
		kframes.push(ani.kframes.front());
		ani.kframes.pop();
	}
}

// SCENE

Scene::Scene() :
	camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup),
	capture(nullptr),
	select(nullptr),
	firstSelect(nullptr),
	shadowFunc(World::sets()->shadowRes ? &Scene::renderShadows : &Scene::renderDummy),
	light(vec3(Com::Config::boardWidth / 2.f, 4.f, Com::Config::boardWidth / 2.f), vec3(1.f, 0.98f, 0.92f), 0.8f)
{}

Scene::~Scene() {
	for (auto& [name, tex] : texes)
		tex.free();
	for (auto& [name, mesh] : meshes)
		mesh.free();
}

void Scene::draw() {
	(this->*shadowFunc)();
	glUseProgram(*World::geom());
	glViewport(0, 0, World::window()->getScreenView().x, World::window()->getScreenView().y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (Object* it : objects)
		if (it->show)
			it->draw();
	if (Object* obj = dynamic_cast<Object*>(capture))
		obj->drawTop();

	glUseProgram(*World::gui());
	glBindVertexArray(World::gui()->wrect.getVao());
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

void Scene::renderShadows() {
#ifndef OPENGLES
	glUseProgram(*World::depth());
	glViewport(0, 0, World::sets()->shadowRes, World::sets()->shadowRes);
	glBindFramebuffer(GL_FRAMEBUFFER, light.depthFrame);
	glClear(GL_DEPTH_BUFFER_BIT);
	for (Object* it : objects)
		if (it->show)
			it->drawDepth();
	if (Object* obj = dynamic_cast<Object*>(capture))
		obj->drawTopDepth();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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
			i++;
		else
			animations.erase(animations.begin() + pdift(i));
	}
}

void Scene::onResize() {
	World::state()->eventResize();
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
	if (cstamp.but)
		return;
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	if (context && select != context.get())
		setContext(nullptr);

	updateSelect(pos);
	cstamp = ClickStamp(select, getSelectedScrollArea(select), pos, but);
	if (cstamp.area)	// area goes first so widget can overwrite it's capture
		cstamp.area->onHold(pos, but);
	if (cstamp.inter != cstamp.area)
		cstamp.inter->onHold(pos, but);
	if (!capture)		// can be set by previous onHold calls
		World::state()->eventDrag(SDL_BUTTON(but));
}

void Scene::onMouseUp(const ivec2& pos, uint8 but) {
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
	Interactable* box = getSelectedScrollArea(select);
	if (!box)
		if (box = dynamic_cast<Context*>(select); !box)
			box = dynamic_cast<TextBox*>(select);
	if (box)
		box->onScroll(ivec2(mov.x, -mov.y) * scrollFactorWheel);
	else if (mov.y)
		World::state()->eventWheel(mov.y);
}

void Scene::onMouseLeave() {
	if (unselect(); cstamp.but) {		// get rid of select first to prevent click event
		ivec2 mPos = mousePos();
		World::input()->eventMouseButtonUp({ SDL_MOUSEBUTTONUP, SDL_GetTicks(), 0, 0, cstamp.but, SDL_RELEASED, 1, 0, mPos.x, mPos.y });
	}
}

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str);
}

void Scene::onConfirm() {
	World::input()->mouseLast = false;
	if (context)
		context->confirm();
	else if (popup)
		World::prun(popup->kcall, nullptr);
	else if (Slider* sl = dynamic_cast<Slider*>(select))
		sl->onHold(sl->sliderRect().pos(), SDL_BUTTON_LEFT);
	else if (Button* but = dynamic_cast<Button*>(select))
		but->onClick(but->position(), SDL_BUTTON_LEFT);
	else
		World::state()->eventEnter();
}

void Scene::onXbutConfirm() {
	if (context)
		context->confirm();
	else if (popup)
		World::prun(popup->kcall, nullptr);
	else
		World::state()->eventEndTurn();
}

void Scene::onCancel() {
	World::input()->mouseLast = false;
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
	meshes = FileSys::loadObjects(World::geom());
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
	shadowFunc = World::sets()->shadowRes ? &Scene::renderShadows : &Scene::renderDummy;
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
	World::fonts()->clear();
	onMouseLeave();	// reset stamps and select
	capture = nullptr;
	popup.reset();
	context.reset();

	// set up new widgets
	layout.reset(World::state()->createLayout(firstSelect));
	overlays = World::state()->createOverlays();
	layout->postInit();
	for (Overlay* it : overlays)
		it->postInit();
	World::input()->simulateMouseMove();
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	unselect();	// preemptive to avoid dangling pointer
	if (popup.reset(newPopup); popup)
		popup->postInit();
	if (capture = newCapture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	World::input()->simulateMouseMove();
}

void Scene::setContext(Context* newContext) {
	if (context) {
		if (context->getParent() && !World::input()->mouseLast)
			updateSelect(context->getParent());
		else if (select == context.get())
			unselect();
	}
	if (context.reset(newContext); World::input()->mouseLast)
		World::input()->simulateMouseMove();
}

void Scene::addAnimation(Animation&& anim) {
	if (vector<Animation>::iterator it = std::find(animations.begin(), animations.end(), anim); it == animations.end())
		animations.push_back(anim);
	else
		it->append(anim);
}

void Scene::delegateStamp(Interactable* inter) {
	cstamp.inter = inter;
	cstamp.area = getSelectedScrollArea(inter);
}

void Scene::navSelect(Direction dir, bool& mouseLast) {
	if (!popup) {
		mouseLast = false;
		if (context)
			context->onNavSelect(dir);
		else if (select)
			select->onNavSelect(dir);
		else
			updateSelect(firstSelect);
	}
}

void Scene::unselect() {
	if (select) {
		select->onUnhover();
		select = nullptr;
	}
}

void Scene::resetSelect() {
	select = nullptr;
	updateSelect();
}

void Scene::updateSelect() {
	if (World::input()->mouseLast)
		updateSelect(mousePos());
}

void Scene::updateSelect(Interactable* sel) {
	if (sel != select) {
		if (select)
			select->onUnhover();
		if (select = sel)
			select->onHover();
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
	else for (vector<Overlay*>::reverse_iterator it = overlays.rbegin(); it != overlays.rend(); it++)
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
