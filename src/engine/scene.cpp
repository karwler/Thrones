#include "world.h"

// CAMERA

const vec3 Camera::posSetup(Com::Config::boardWidth / 2.f, 9.f, Com::Config::boardWidth / 2.f + 9.f);
const vec3 Camera::posMatch(Com::Config::boardWidth / 2.f, 12.f, Com::Config::boardWidth / 2.f + 10.f);
const vec3 Camera::latSetup(Com::Config::boardWidth / 2.f, 0.f, Com::Config::boardWidth / 2.f + 2.5f);
const vec3 Camera::latMatch(Com::Config::boardWidth / 2.f, 0.f, Com::Config::boardWidth / 2.f + 1.f);
const vec3 Camera::up(0.f, 1.f, 0.f);
const vec3 Camera::center(Com::Config::boardWidth / 2.f, 0.f, Com::Config::boardWidth / 2.f);

Camera::Camera(const vec3& pos, const vec3& lat, float pmax, float ymax) :
	moving(false),
	pmax(pmax),
	ymax(ymax)
{
	updateProjection();
	setPos(pos, lat);
}

void Camera::updateView() const {
	mat4 projview = proj * glm::lookAt(pos, lat, up);
	glUseProgram(*World::geom());
	glUniformMatrix4fv(World::geom()->pview, 1, GL_FALSE, glm::value_ptr(projview));
	glUniform3fv(World::geom()->viewPos, 1, glm::value_ptr(pos));
}

void Camera::updateProjection() {
	vec2 res = World::window()->screenView();
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
	vec2 res = World::window()->screenView();
	vec3 dir = glm::normalize(lat - pos);

	float vl = std::tan(glm::radians(fov / 2.f)) * znear;
	float hl = vl * (res.x / res.y);
	vec3 h = glm::normalize(glm::cross(dir, up)) * hl;
	vec3 v = glm::normalize(glm::cross(h, dir)) * vl;

	res /= 2.f;
	vec2 m((float(mPos.x) - res.x) / res.x, -(float(mPos.y) - res.y) / res.y);
	return glm::normalize(dir * znear + h * m.x + v * m.y);
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

ClickStamp::ClickStamp(Interactable* inter, ScrollArea* area, const ivec2& pos, uint8 but) :
	inter(inter),
	area(area),
	pos(pos),
	but(but)
{}

// KEYFRAME

Keyframe::Keyframe(float time, Change change, const vec3& pos, const quat& rot, const vec3& scl) :
	pos(pos),
	scl(scl),
	rot(rot),
	time(time),
	change(change)
{}

// ANIMATION

Animation::Animation(Object* object, std::queue<Keyframe>&& keyframes) :
	keyframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, object->getPos(), object->getRot(), object->getScl()),
	object(object),
	useObject(true)
{}

Animation::Animation(Camera* camera, std::queue<Keyframe>&& keyframes) :
	keyframes(std::move(keyframes)),
	begin(0.f, Keyframe::CHG_NONE, camera->getPos(), quat(), camera->getLat()),
	camera(camera),
	useObject(false)
{
	camera->moving = true;
}

bool Animation::tick(float dSec) {
	begin.time += dSec;
	if (float td = keyframes.front().time > 0.f ? std::clamp(begin.time / keyframes.front().time, 0.f, 1.f) : 1.f; useObject) {
		if (keyframes.front().change & Keyframe::CHG_POS)
			object->setPos(glm::mix(begin.pos, keyframes.front().pos, td));
		if (keyframes.front().change & Keyframe::CHG_ROT)
			object->setRot(glm::mix(begin.rot, keyframes.front().rot, td));
		if (keyframes.front().change & Keyframe::CHG_SCL)
			object->setScl(glm::mix(begin.scl, keyframes.front().scl, td));
	} else
		camera->setPos(keyframes.front().change & Keyframe::CHG_POS ? glm::mix(begin.pos, keyframes.front().pos, td) : camera->getPos(), keyframes.front().change & Keyframe::CHG_LAT ? glm::mix(begin.scl, keyframes.front().scl, td) : camera->getLat());

	if (begin.time >= keyframes.front().time) {
		float ovhead = begin.time - keyframes.front().time;
		if (keyframes.pop(); !keyframes.empty()) {
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
	while (!ani.keyframes.empty()) {
		keyframes.push(ani.keyframes.front());
		ani.keyframes.pop();
	}
}

// SCENE

Scene::Scene() :
	select(nullptr),
	capture(nullptr),
	meshes(FileSys::loadObjects()),
	materials(FileSys::loadMaterials()),
	texes(FileSys::loadTextures()),
	shadowFunc(World::sets()->shadowRes ? &Scene::renderShadows : &Scene::renderDummy),
	camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup),
	light(vec3(Com::Config::boardWidth / 2.f, 4.f, Com::Config::boardWidth / 2.f), vec3(1.f, 0.98f, 0.92f), 0.8f),
	mouseMove(0),
	moveTime(0),
	mouseLast(false)
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
	glViewport(0, 0, World::window()->screenView().x, World::window()->screenView().y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (Object* it : objects)
		if (it->show)
			it->draw();
	if (Object* obj = dynamic_cast<Object*>(capture))
		obj->drawTop();

	glUseProgram(*World::gui());
	glBindVertexArray(World::gui()->wrect.getVao());
	layout->draw();
	if (overlay && overlay->getOn())
		overlay->draw();
	if (popup)
		popup->draw();
	if (context)
		context->draw();
	else {
		if (Widget* wgt = dynamic_cast<Widget*>(capture))
			wgt->drawTop();
		if (Button* but = dynamic_cast<Button*>(select); but && mouseLast)
			but->drawTooltip();
	}
}

void Scene::renderShadows() {
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
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
	if (overlay)
		overlay->tick(dSec);
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
	if (overlay)
		overlay->onResize();
}

void Scene::onKeyDown(const SDL_KeyboardEvent& key) {
	if (dynamic_cast<LabelEdit*>(capture))
		capture->onKeypress(key.keysym);
	else if (context)
		context->onKeypress(key.keysym);
	else if (!key.repeat)
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5: case SDL_SCANCODE_6: case SDL_SCANCODE_7: case SDL_SCANCODE_8: case SDL_SCANCODE_9: case SDL_SCANCODE_0:
			World::state()->eventNumpress(uint8(key.keysym.scancode - SDL_SCANCODE_1));
			break;
		case SDL_SCANCODE_LALT:
			World::state()->eventFavorize(FavorAct::on);
			break;
		case SDL_SCANCODE_LSHIFT:
			World::state()->eventFavorize(FavorAct::now);
			break;
		case SDL_SCANCODE_SPACE:
			World::state()->eventEndTurn();
			break;
#ifndef EMSCRIPTEN
		case SDL_SCANCODE_LCTRL:
			SDL_SetRelativeMouseMode(SDL_TRUE);
			break;
#endif
		case SDL_SCANCODE_C:
			World::state()->eventCameraReset();
			break;
		case SDL_SCANCODE_TAB:
			World::program()->eventToggleChat();
			break;
		case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
			if (popup)
				World::prun(popup->kcall, nullptr);
			else
				World::state()->eventEnter();
			break;
		case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_AC_BACK:
			if (popup)
				World::prun(popup->ccall, nullptr);
			else
				World::state()->eventEscape();
		}
}

void Scene::onKeyUp(const SDL_KeyboardEvent& key) {
	if (!dynamic_cast<LabelEdit*>(capture) && !context && !key.repeat)
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LALT: case SDL_SCANCODE_LSHIFT:
			World::state()->eventFavorize(FavorAct::off);
			break;
#ifndef EMSCRIPTEN
		case SDL_SCANCODE_LCTRL:
			SDL_SetRelativeMouseMode(SDL_FALSE);
			simulateMouseMove();
#endif
		}
}

void Scene::onMouseMove(const SDL_MouseMotionEvent& mot, bool mouse) {
	if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] && !capture && !camera.moving) {
		vec2 rot(glm::radians(float(mot.yrel) / 2.f), glm::radians(float(-mot.xrel) / 2.f));
		camera.rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
		return;
	}
	mouseMove = ivec2(mot.xrel, mot.yrel);
	moveTime = mot.timestamp;
	mouseLast = mouse;

	updateSelect(ivec2(mot.x, mot.y));
	if (capture)
		capture->onDrag(ivec2(mot.x, mot.y), mouseMove);
	else if (context)
		context->onMouseMove(ivec2(mot.x, mot.y));
	else if (mot.state)
		World::state()->eventDrag(mot.state);
}

void Scene::onMouseDown(const SDL_MouseButtonEvent& but, bool mouse) {
	switch (but.button) {
	case SDL_BUTTON_MIDDLE:
		World::state()->eventEndTurn();
		break;
	case SDL_BUTTON_X1:
		if (dynamic_cast<ProgMatch*>(World::state()))
			World::state()->eventFavorize(FavorAct::on);
		else
			World::state()->eventEscape();
		return;
	case SDL_BUTTON_X2:
		if (dynamic_cast<ProgMatch*>(World::state()))
			World::state()->eventFavorize(FavorAct::now);
		else
			World::state()->eventEnter();
		return;
	}

	if (cstamp.but)
		return;
	mouseLast = mouse;
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	if (context && select != context.get())
		setContext(nullptr);

	cstamp = ClickStamp(select, getSelectedScrollArea(), ivec2(but.x, but.y), but.button);
	if (cstamp.area)	// area goes first so widget can overwrite it's capture
		cstamp.area->onHold(ivec2(but.x, but.y), but.button);
	if (cstamp.inter != cstamp.area)
		cstamp.inter->onHold(ivec2(but.x, but.y), but.button);
	if (!capture)		// can be set by previous onHold calls
		World::state()->eventDrag(SDL_BUTTON(but.button));
}

void Scene::onMouseUp(const SDL_MouseButtonEvent& but, bool mouse) {
	if (but.button == SDL_BUTTON_X1 || but.button == SDL_BUTTON_X2)
		return World::state()->eventFavorize(FavorAct::off);

	if (but.button != cstamp.but)
		return;
	mouseLast = mouse;
	if (capture)
		capture->onUndrag(but.button);
	if (select && cstamp.inter == select && cursorInClickRange(ivec2(but.x, but.y)))
		cstamp.inter->onClick(ivec2(but.x, but.y), but.button);
	if (!capture)
		World::state()->eventUndrag();
	cstamp = ClickStamp();
}

void Scene::onMouseWheel(const SDL_MouseWheelEvent& whe) {
	mouseLast = true;
	Interactable* box = getSelectedScrollArea();
	if (!box)
		if (box = dynamic_cast<Context*>(select); !box)
			box = dynamic_cast<TextBox*>(select);
	if (box)
		box->onScroll(ivec2(whe.x, whe.y) * scrollFactorWheel);
	else if (whe.y)
		World::state()->eventWheel(whe.y);
}

void Scene::onMouseLeave() {
	if (unselect(); cstamp.but) {		// get rid of select first to prevent click event
		ivec2 mPos = mousePos();
		onMouseUp({ SDL_MOUSEBUTTONUP, SDL_GetTicks(), World::window()->windowID(), 0, cstamp.but, SDL_RELEASED, 1, 0, mPos.x, mPos.y });
	}
}

void Scene::onFingerMove(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::window()->screenView();
	onMouseMove({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LMASK, int(fin.x * size.x), int(fin.y * size.y), int(fin.dx * size.x), int(fin.dy * size.y) }, false);
}

void Scene::onFingerGesture(const SDL_MultiGestureEvent& ges) {
	if (dynamic_cast<ProgMatch*>(World::state()) && ges.numFingers == 2 && std::abs(ges.dDist) > FLT_EPSILON)
		camera.zoom(int(ges.dDist * float(World::window()->screenView().y)));
}

void Scene::onFingerDown(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::window()->screenView());
	updateSelect(pos);
	onMouseDown({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_PRESSED, 1, 0, pos.x, pos.y }, false);
}

void Scene::onFingerUp(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::window()->screenView();
	onMouseUp({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_RELEASED, 1, 0, int(fin.x * size.x), int(fin.y * size.y) }, false);
	updateSelect(ivec2(-1));
}

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str);
}

void Scene::resetShadows() {
	shadowFunc = World::sets()->shadowRes ? &Scene::renderShadows : &Scene::renderDummy;
	glUseProgram(*World::geom());
	loadCubemap(light.depthMap, World::sets()->shadowRes ? World::sets()->shadowRes : 1, Light::depthTexa);
	glActiveTexture(GL_TEXTURE0);
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
	layout.reset(World::state()->createLayout());
	overlay.reset(World::state()->createOverlay());
	layout->postInit();
	if (overlay)
		overlay->postInit();
	simulateMouseMove();
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	unselect();	// preemptive to avoid dangling pointer
	if (popup.reset(newPopup); popup)
		popup->postInit();
	if (capture = newCapture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	simulateMouseMove();
}

void Scene::setContext(Context* newContext) {
	unselect();
	context.reset(newContext);
	simulateMouseMove();
}

void Scene::addAnimation(Animation&& anim) {
	if (vector<Animation>::iterator it = std::find(animations.begin(), animations.end(), anim); it == animations.end())
		animations.push_back(anim);
	else
		it->append(anim);
}

void Scene::unselect() {
	if (select) {
		select->onUnhover();
		select = nullptr;
	}
}

void Scene::updateSelect() {
	if (mouseLast)
		updateSelect(mousePos());
}

void Scene::updateSelect(const ivec2& mPos) {
	if (Interactable* cur = getSelected(mPos); cur != select) {
		if (select)
			select->onUnhover();
		if (select = cur)
			select->onHover();
	}
}

Interactable* Scene::getSelected(const ivec2& mPos) {
	if (outRange(mPos, ivec2(0), World::window()->screenView()))
		return nullptr;
	if (context && context->rect().contain(mPos))
		return context.get();

	for (Layout* box = popup ? popup.get() : overlay && overlay->getOn() && overlay->rect().contain(mPos) ? overlay.get() : layout.get();;) {
		Rect frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->selectable() ? *it : getScrollOrObject(mPos, *it);
		} else
			return getScrollOrObject(mPos, box);
	}
}

Interactable* Scene::getScrollOrObject(const ivec2& mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return !popup ? static_cast<Interactable*>(findBoardObject(mPos)) : static_cast<Interactable*>(popup.get());
}

ScrollArea* Scene::findFirstScrollArea(Widget* wgt) {
	Layout* parent = dynamic_cast<Layout*>(wgt);
	if (wgt && !parent)
		parent = wgt->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

BoardObject* Scene::findBoardObject(const ivec2& mPos) const {
	vec3 isct = rayXZIsct(pickerRay(mPos));
	if (const vec4& bb = World::game()->getBoardBounds(); isct.x < bb.x || isct.x >= bb.z || isct.z < bb.y || isct.z >= bb.a)
		return nullptr;

	svec2 pp = World::game()->ptog(isct);
	for (Piece& it : World::game()->getPieces())
		if (it.rigid && World::game()->ptog(it.getPos()) == pp)
			return &it;
	if (uint16 id = World::game()->posToId(pp); id < World::game()->getTiles().getSize())
		if (Tile& it = World::game()->getTiles()[id]; it.rigid)
			return &it;
	return nullptr;
}

void Scene::simulateMouseMove() {
	if (ivec2 pos; mouseLast) {
		uint32 state = SDL_GetMouseState(&pos.x, &pos.y);
		onMouseMove({ SDL_MOUSEMOTION, SDL_GetTicks(), World::window()->windowID(), 0, state, pos.x, pos.y, 0, 0 }, mouseLast);
	} else
		onMouseMove({ SDL_FINGERMOTION, SDL_GetTicks(), World::window()->windowID(), SDL_TOUCH_MOUSEID, 0, -1, -1, 0, 0 }, mouseLast);
}
