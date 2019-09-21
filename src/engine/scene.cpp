#include "world.h"

// CAMERA

const vec3 Camera::posSetup(Com::Config::boardWidth / 2.f, 9.f, 9.f);
const vec3 Camera::posMatch(Com::Config::boardWidth / 2.f, 12.f, 10.f);
const vec3 Camera::latSetup(Com::Config::boardWidth / 2.f, 0.f, 2.5f);
const vec3 Camera::latMatch(Com::Config::boardWidth / 2.f, 0.f, 1.f);
const vec3 Camera::up(0.f, 1.f, 0.f);
const vec3 Camera::center(Com::Config::boardWidth / 2.f, 0.f, 0.f);

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
	glUseProgram(World::geom()->program);
	glUniformMatrix4fv(World::geom()->pview, 1, GL_FALSE, glm::value_ptr(projview));
	glUniform3fv(World::geom()->viewPos, 1, glm::value_ptr(pos));
}

void Camera::updateProjection() {
	vec2i res = World::window()->getView();
	proj = glm::perspective(glm::radians(fov), vec2f(res).ratio(), znear, zfar);

	vec2 hsiz(float(res.x / 2), float(res.y / 2));
	glUseProgram(World::gui()->program);
	glUniform2fv(World::gui()->pview, 1, glm::value_ptr(hsiz));
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
	prot = vec2(std::clamp(prot.x + dRot.x, 0.f, pmax), std::clamp(prot.y + dRot.y, -PI - ymax, ymax));
	pos = makeQuat(vec3(0.f, prot.y, prot.x)) * vec3(pdst, 0.f, 0.f);
	lat = glm::quat(vec3(0.f, lyaw + dYaw, 0.f)) * vec3(ldst, 0.f, 0.f);
	updateRotations(pos, lat);
	lat += center;
	pos += lat;
	updateView();
}

void Camera::zoom(int mov) {
	pdst = std::clamp(pdst + float(mov) / 2.f, defaultPdst - 8.f, defaultPdst + 20.f);
	pos = makeQuat(vec3(0.f, prot.y, prot.x)) * vec3(pdst, 0.f, 0.f) + lat;
	updateView();
}

vec3 Camera::direction(vec2i mPos) const {
	vec2 res = World::window()->getView().glm();
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
	position(position),
	ambient(color * ambiFac),
	diffuse(color),
	linear(4.5f / range),
	quadratic(75.f / (range * range))
{
	glUseProgram(World::geom()->program);
	glUniform3fv(World::geom()->lightPos, 1, glm::value_ptr(position));
	glUniform3fv(World::geom()->lightAmbient, 1, glm::value_ptr(ambient));
	glUniform3fv(World::geom()->lightDiffuse, 1, glm::value_ptr(diffuse));
	glUniform1f(World::geom()->lightLinear, linear);
	glUniform1f(World::geom()->lightQuadratic, quadratic);
}

// CLICK STAMP

ClickStamp::ClickStamp(Interactable* inter, ScrollArea* area, vec2i mPos) :
	inter(inter),
	area(area),
	mPos(mPos)
{}

// KEYFRAME

Keyframe::Keyframe(float time, Change change, const vec3& pos, const vec3& rot, const vec3& scl) :
	pos(pos),
	rot(rot),
	scl(scl),
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
	begin(0.f, Keyframe::CHG_NONE, camera->getPos(), camera->getLat()),
	camera(camera),
	useObject(false)
{
	camera->moving = true;
}

bool Animation::tick(float dSec) {
	begin.time += dSec;

	if (float td = keyframes.front().time > 0.f ? clampHigh(begin.time / keyframes.front().time, 1.f) : 1.f; useObject) {
		if (keyframes.front().change & Keyframe::CHG_POS)
			object->setPos(lerp(begin.pos, keyframes.front().pos, td));
		if (keyframes.front().change & Keyframe::CHG_ROT)
			object->setRot(lerp(begin.rot, keyframes.front().rot, td));
		if (keyframes.front().change & Keyframe::CHG_SCL)
			object->setScl(lerp(begin.scl, keyframes.front().scl, td));
	} else
		camera->setPos(keyframes.front().change & Keyframe::CHG_POS ? lerp(begin.pos, keyframes.front().pos, td) : camera->getPos(), keyframes.front().change & Keyframe::CHG_ROT ? lerp(begin.rot, keyframes.front().rot, td) : camera->getLat());

	if (begin.time >= keyframes.front().time) {
		float ovhead = begin.time - keyframes.front().time;
		if (keyframes.pop(); !keyframes.empty()) {
			begin = Keyframe(0.f, Keyframe::CHG_NONE, useObject ? object->getPos() : camera->getPos(), useObject ? object->getRot() : camera->getLat(), useObject ? object->getScl() : vec3());
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
	mouseMove(0),
	camera(Camera::posSetup, Camera::latSetup, Camera::pmaxSetup, Camera::ymaxSetup),
	layout(new Layout),	// dummy layout in case a function gets called preemptively
	light(vec3(Com::Config::boardWidth / 2.f, 4.f, 0.f), vec3(1.f, 0.98f, 0.92f), 0.8f, 140.f),
	meshes(FileSys::loadObjects()),
	materials(FileSys::loadMaterials()),
	texes(FileSys::loadTextures()),
	collims({
		pair(string(), CMesh()),
		pair("tile", CMesh::makeDefault()),
		pair("piece", CMesh::makeDefault(vec3(0.f, BoardObject::upperPoz, 0.f)))
	})
{}

Scene::~Scene() {
	glUseProgram(World::geom()->program);
	for (auto& [name, tex] : texes)
		tex.free();
	for (auto& [name, mesh] : meshes)
		mesh.free();
	for (auto& [name, mesh] : collims)
		mesh.free();
}

void Scene::draw() {
	glUseProgram(World::geom()->program);
	for (Object* it : objects)
		it->draw();
	if (dynamic_cast<Object*>(capture))
		capture->drawTop();

	glUseProgram(World::gui()->program);
	World::gui()->bindRect();
	layout->draw();
	if (popup)
		popup->draw();
	if (dynamic_cast<Widget*>(capture))
		capture->drawTop();
	if (Button* but = dynamic_cast<Button*>(select))
		but->drawTooltip();
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);

	for (sizet i = 0; i < animations.size();) {
		if (animations[i].tick(dSec))
			i++;
		else
			animations.erase(animations.begin() + pdift(i));
	}
}

void Scene::onResize() {
	camera.updateProjection();
	camera.updateView();
	layout->onResize();
	if (popup)
		popup->onResize();
}

void Scene::onKeyDown(const SDL_KeyboardEvent& key) {
	if (dynamic_cast<LabelEdit*>(capture))
		capture->onKeypress(key.keysym);
	else if (!key.repeat)
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5: case SDL_SCANCODE_6: case SDL_SCANCODE_7: case SDL_SCANCODE_8: case SDL_SCANCODE_9: case SDL_SCANCODE_0:
			World::state()->eventNumpress(uint8(key.keysym.scancode - SDL_SCANCODE_1));
			break;
		case SDL_SCANCODE_LALT:
			if (World::game()->favorState.use = true; Piece* pce = dynamic_cast<Piece*>(capture))
				World::program()->eventFavorStart(pce, pce->show ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);	// vaguely simulate what happens when holding down on a piece to refresh favor state and highlighted tiles (no need to take warhorse into account)
			break;
		case SDL_SCANCODE_C:
			World::state()->eventCameraReset();
			break;
		case SDL_SCANCODE_RETURN:
			if (popup)		// right now this is only necessary for popups, so no fancy virtual functions
				World::prun(popup->kcall, nullptr);
			break;
		case SDL_SCANCODE_ESCAPE:
			World::state()->eventEscape();
		}
}

void Scene::onKeyUp(const SDL_KeyboardEvent& key) {
	if (!(dynamic_cast<LabelEdit*>(capture) || key.repeat))
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LALT:
			if (World::game()->favorState.use = false; Piece* pce = dynamic_cast<Piece*>(capture))
				World::program()->eventFavorStart(pce, pce->show ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);	// same as above (I could probably just call the piece's onHold())
		}
}

void Scene::onMouseMove(vec2i mPos, vec2i mMov, uint32 mStat, uint32 time) {
	if ((mStat & SDL_BUTTON_MMASK) && !camera.moving) {
		vec2 rot(glm::radians(float(mMov.y) / 2.f), glm::radians(float(-mMov.x) / 2.f));
		camera.rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
		return;
	}
	mouseMove = mMov;
	moveTime = time;

	updateSelect(mPos);
	if (capture)
		capture->onDrag(mPos, mMov);
	else if (mStat)
		World::state()->eventDrag(mStat);
}

void Scene::onMouseDown(vec2i mPos, uint8 mBut) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();

	uint8 mbi = mBut - 1;
	select = getSelected(mPos);
	stamps[mbi] = ClickStamp(select, getSelectedScrollArea(), mPos);
	if (stamps[mbi].area)	// area goes first so widget can overwrite it's capture	// TODO: might need an "if (!capture)" to prevent overlapping onHold calls in a single widget/object
		stamps[mbi].area->onHold(mPos, mBut);
	if (stamps[mbi].inter != stamps[mbi].area)
		stamps[mbi].inter->onHold(mPos, mBut);
	if (!capture)	// can be set by previous onHold calls
		World::state()->eventDrag(SDL_BUTTON(mBut));
	if (mBut == SDL_BUTTON_MIDDLE)
		SDL_SetRelativeMouseMode(SDL_TRUE);
}

void Scene::onMouseUp(vec2i mPos, uint8 mBut) {
	if (mBut == SDL_BUTTON_MIDDLE) {	// in case camera was being moved
		SDL_SetRelativeMouseMode(SDL_FALSE);
		simulateMouseMove();
	}
	if (capture)
		capture->onUndrag(mBut);
	if (uint8 mbi = mBut - 1; select && stamps[mbi].inter == select && cursorInClickRange(mPos, mBut))
		stamps[mbi].inter->onClick(mPos, mBut);
	if (!capture)
		World::state()->eventUndrag();
}

void Scene::onMouseWheel(vec2i wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * scrollFactorWheel);
	else if (wMov.y)
		World::state()->eventWheel(wMov.y);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps)
		it.inter = it.area = nullptr;
	unselect();
}

void Scene::resetLayouts() {
	// clear scene
	World::fonts()->clear();
	onMouseLeave();	// reset stamps and select
	capture = nullptr;
	popup.reset();

	// set up new widgets
	layout.reset(World::state()->createLayout());
	layout->postInit();
	simulateMouseMove();
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	unselect();	// preemptive to avoid dangling pointer
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
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

void Scene::updateSelect(vec2i mPos) {
	if (Interactable* cur = getSelected(mPos); cur != select) {
		if (select)
			select->onUnhover();
		if (select = cur)
			select->onHover();
	}
}

Interactable* Scene::getSelected(vec2i mPos) {
	for (Layout* box = !popup ? layout.get() : popup.get();;) {
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

Interactable* Scene::getScrollOrObject(vec2i mPos, Widget* wgt) const {
	if (ScrollArea* lay = findFirstScrollArea(wgt))
		return lay;
	return !popup ? static_cast<Interactable*>(rayCast(pickerRay(mPos))) : static_cast<Interactable*>(popup.get());
}

ScrollArea* Scene::findFirstScrollArea(Widget* wgt) {
	Layout* parent = dynamic_cast<Layout*>(wgt);
	if (wgt && !parent)
		parent = wgt->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

Object* Scene::rayCast(const vec3& ray) const {
	float min = FLT_MAX;
	Object* mob = nullptr;
	for (Object* obj : objects) {
		if (!obj->rigid)	// assuming that obj->coli is set
			continue;

		const mat4& trans = obj->getTrans();
		for (uint16 i = 0; i < obj->coli->esiz; i += 3)
			if (float t; rayIntersectsTriangle(camera.getPos(), ray, trans * vec4(obj->coli->verts[obj->coli->elems[i]], 1.f), trans * vec4(obj->coli->verts[obj->coli->elems[i+1]], 1.f), trans * vec4(obj->coli->verts[obj->coli->elems[i+2]], 1.f), t) && t <= min) {
				min = t;
				mob = obj;
			}
	}
	return mob;
}

bool Scene::rayIntersectsTriangle(const vec3& ori, const vec3& dir, const vec3& v0, const vec3& v1, const vec3& v2, float& t) {
	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 h = glm::cross(dir, e2);
	float a = glm::dot(e1, h);
	if (a > -FLT_EPSILON && a < FLT_EPSILON)
		return false;

	float f = 1.f / a;
	vec3 s = ori - v0;
	float u = f * (glm::dot(s, h));
	if (u < 0.f || u > 1.f)
		return false;

	vec3 q = glm::cross(s, e1);
	float v = f * glm::dot(dir, q);
	if (v < 0.f || u + v > 1.f)
		return false;

	t = f * glm::dot(e2, q);
	return t > FLT_EPSILON;
}

void Scene::simulateMouseMove() {
	vec2i pos;
	uint32 state = SDL_GetMouseState(&pos.x, &pos.y);
	onMouseMove(pos, 0, state, SDL_GetTicks());
}
