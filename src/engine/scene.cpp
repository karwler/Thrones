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
	capture(nullptr),
	select(nullptr),
	firstSelect(nullptr),
	meshes(FileSys::loadObjects()),
	materials(FileSys::loadMaterials()),
	texes(FileSys::loadTextures()),
	shadowFunc(World::sets()->shadowRes ? &Scene::renderShadows : &Scene::renderDummy),
	camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup),
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
		if (Button* but = dynamic_cast<Button*>(select); but && World::input()->mouseLast)
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

	cstamp = ClickStamp(select, getSelectedScrollArea(), pos, but);
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
	Interactable* box = getSelectedScrollArea();
	if (!box)
		if (box = dynamic_cast<Context*>(select); !box)
			box = dynamic_cast<TextBox*>(select);
	if (box)
		box->onScroll(mov * scrollFactorWheel);
	else if (mov.y)
		World::state()->eventWheel(mov.y);
}

void Scene::onMouseLeave() {
	if (unselect(); cstamp.but) {		// get rid of select first to prevent click event
		ivec2 mPos = mousePos();
		World::input()->eventMouseButtonUp({ SDL_MOUSEBUTTONUP, SDL_GetTicks(), World::window()->windowID(), 0, cstamp.but, SDL_RELEASED, 1, 0, mPos.x, mPos.y });
	}
}

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str);
}

void Scene::onConfirm() {
	if (World::input()->mouseLast = false; context)
		context->confirm();
	else if (popup)
		World::prun(popup->kcall, nullptr);
	else if (dynamic_cast<Slider*>(select))
		capture = select;
	else if (Button* but = dynamic_cast<Button*>(select))
		but->onClick(but->position(), SDL_BUTTON_LEFT);
	else
		World::state()->eventEnter();
}

void Scene::onCancel() {
	if (World::input()->mouseLast = false; context)
		setContext(nullptr);
	else if (popup)
		World::prun(popup->ccall, nullptr);
	else
		World::state()->eventEscape();
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
	layout.reset(World::state()->createLayout(firstSelect));
	overlay.reset(World::state()->createOverlay());
	layout->postInit();
	if (overlay)
		overlay->postInit();
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

void Scene::navSelect(Direction dir) {
	if (!popup) {
		if (World::input()->mouseLast = false; context)
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
