#include "inputSys.h"
#include "scene.h"
#include "world.h"
#include "prog/progs.h"

// CONTROLLER

int16 Controller::axisValue(int16 value) {
	return std::abs(value) > World::sets()->deadzone ? value : 0;
}

optional<bool> Controller::axisDown(int16 val, uint8 id, bool* vrec) {
	bool curOn = axisValue(val);
	if (vrec[id] == curOn)
		return std::nullopt;
	vrec[id] = curOn;
	return curOn;
}

// JOYSTICK

Joystick::Joystick(SDL_Joystick* joy) :
	ctr(joy),
	hats(initRecord<uint8>(SDL_JoystickNumHats(joy), SDL_HAT_CENTERED)),
	axes(initRecord<bool>(SDL_JoystickNumAxes(joy), false))
{}

template <class T>
uptr<T[]> Joystick::initRecord(int num, T fill) {
	if (num > 0) {
		uptr<T[]> vals = std::make_unique<T[]>(num);
		std::fill_n(vals.get(), num, fill);
		return vals;
	}
	return nullptr;
}

void Joystick::close() {
	SDL_JoystickClose(ctr);
}

uint8 Joystick::hatDown(uint8 val, uint8 id) {
	uint8 prev = hats[id];
	hats[id] = val;
	return prev;
}

// INPUT SYS

InputSys::InputSys() {
	for (uint8 i = 0; i < bindings.size(); ++i)
		bindings[i].init(Binding::Type(i));
}

InputSys::~InputSys() {
	for (auto& [id, joy] : joysticks)
		joy.close();
	for (auto& [id, pad] : gamepads)
		pad.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion, int yoffset, bool mouse) {
	mouseLast = mouse;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	if (World::scene()->camera.state != Camera::State::dragging)
		World::scene()->onMouseMove(ivec2(motion.x, motion.y - yoffset), mouseMove, motion.state);
	else {
		vec2 rot(glm::radians(float(motion.yrel) / 2.f), glm::radians(float(-motion.xrel) / 2.f));
		World::scene()->camera.rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
	}
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button, int yoffset, bool mouse) {
	mouseLast = mouse;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseDown(ivec2(button.x, button.y - yoffset), button.button);
		break;
	case SDL_BUTTON_MIDDLE:
		World::state()->eventStartCamera();
		break;
	case SDL_BUTTON_X1:
		World::scene()->onXbutCancel();
		break;
	case SDL_BUTTON_X2:
		World::scene()->onXbutConfirm();
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button, int yoffset, bool mouse) {
	mouseLast = mouse;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseUp(ivec2(button.x, button.y - yoffset), button.button);
		break;
	case SDL_BUTTON_MIDDLE:
		World::state()->eventStopCamera();
	}
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseLast = true;
	World::scene()->onMouseWheel(ivec2(wheel.x, wheel.y));
}

void InputSys::eventKeyDown(const SDL_KeyboardEvent& key) {
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onKeyDown(key);
	else
		checkInput(keymap, key.keysym.scancode, &Binding::bcall, key.repeat ? Binding::singles : Binding::holders);
}

void InputSys::eventKeyUp(const SDL_KeyboardEvent& key) {
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onKeyUp(key);
	else
		checkInput(keymap, key.keysym.scancode, &Binding::ucall, key.repeat ? Binding::singles : Binding::holders);
}

void InputSys::eventJoystickButtonDown(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJButtonDown(jbutton.button);
	else
		checkInput(joymap, JoystickButton(jbutton.button), &Binding::bcall);
}

void InputSys::eventJoystickButtonUp(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event
		return;

	if (World::scene()->getCapture())
		World::scene()->getCapture()->onJButtonUp(jbutton.button);
	else
		checkInput(joymap, JoystickButton(jbutton.button), &Binding::ucall);
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) {
	umap<SDL_JoystickID, Joystick>::iterator joy = joysticks.find(jhat.which);
	if (joy == joysticks.end())
		return;

	if (joy->second.hatDown(jhat.value, jhat.hat) != SDL_HAT_CENTERED) {	// release
		if (World::scene()->getCapture())
			World::scene()->getCapture()->onJHatUp(jhat.hat, jhat.value);
		else
			checkInput(joymap, AsgJoystick(jhat.hat, jhat.value), &Binding::ucall);
	}
	if (jhat.value != SDL_HAT_CENTERED) {	// press
		if (World::scene()->getCapture())
			World::scene()->getCapture()->onJHatDown(jhat.hat, jhat.value);
		else
			checkInput(joymap, AsgJoystick(jhat.hat, jhat.value), &Binding::bcall);
	}
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) {
	umap<SDL_JoystickID, Joystick>::iterator joy = joysticks.find(jaxis.which);
	if (joy == joysticks.end())
		return;
	optional<bool> adown = joy->second.axisDown(jaxis.value, jaxis.axis);
	if (!adown)
		return;

	if (*adown) {	// press
		if (World::scene()->getCapture())
			World::scene()->getCapture()->onJAxisDown(jaxis.axis, jaxis.value >= 0);
		else
			checkInput(joymap, AsgJoystick(JoystickAxis(jaxis.axis), jaxis.value >= 0), &Binding::bcall);
	} else if (World::scene()->getCapture())	// release
		World::scene()->getCapture()->onJAxisUp(jaxis.axis, jaxis.value >= 0);
	else
		checkInput(joymap, AsgJoystick(JoystickAxis(jaxis.axis), jaxis.value >= 0), &Binding::ucall);
}

void InputSys::eventGamepadButtonDown(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onGButtonDown(SDL_GameControllerButton(gbutton.button));
	else
		checkInput(padmap, SDL_GameControllerButton(gbutton.button), &Binding::bcall);
}

void InputSys::eventGamepadButtonUp(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->getCapture())
		World::scene()->getCapture()->onGButtonUp(SDL_GameControllerButton(gbutton.button));
	else
		checkInput(padmap, SDL_GameControllerButton(gbutton.button), &Binding::ucall);
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	umap<SDL_JoystickID, Gamepad>::iterator pad = gamepads.find(gaxis.which);
	if (pad == gamepads.end())
		return;
	optional<bool> adown = pad->second.axisDown(gaxis.value, gaxis.axis);
	if (!adown)
		return;

	if (*adown) {	// press
		if (World::scene()->getCapture())
			World::scene()->getCapture()->onGAxisDown(SDL_GameControllerAxis(gaxis.axis), gaxis.value >= 0);
		else
			checkInput(padmap, AsgGamepad(SDL_GameControllerAxis(gaxis.axis), gaxis.value >= 0), &Binding::bcall);
	} else if (World::scene()->getCapture())	// release
		World::scene()->getCapture()->onGAxisUp(SDL_GameControllerAxis(gaxis.axis), gaxis.value >= 0);
	else
		checkInput(padmap, AsgGamepad(SDL_GameControllerAxis(gaxis.axis), gaxis.value >= 0), &Binding::ucall);
}

void InputSys::eventFingerMove(const SDL_TouchFingerEvent& fin, int yoffset, int windowHeight) {
	vec2 size(World::window()->getScreenView().x, windowHeight);
	eventMouseMotion({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LMASK, int(fin.x * size.x), int(fin.y * size.y), int(fin.dx * size.x), int(fin.dy * size.y) }, yoffset, false);
}

void InputSys::eventFingerGesture(const SDL_MultiGestureEvent& ges) {
	if (World::scene()->camera.state != Camera::State::animating && dynamic_cast<ProgMatch*>(World::state()) && ges.numFingers == 2 && std::abs(ges.dDist) > FLT_EPSILON)
		World::scene()->camera.zoom(int(ges.dDist * float(World::window()->getScreenView().y)));
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin, int yoffset, int windowHeight) {
	ivec2 pos(fin.x * float(World::window()->getScreenView().x), fin.y * float(windowHeight));
	eventMouseButtonDown({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_PRESSED, 1, 0, pos.x, pos.y }, yoffset, false);
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin, int yoffset, int windowHeight) {
	ivec2 pos(fin.x * float(World::window()->getScreenView().x), fin.y * float(windowHeight));
	eventMouseButtonUp({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_RELEASED, 1, 0, pos.x, pos.y }, yoffset, false);
	World::scene()->deselect();	// deselect last Interactable because Scene::onMouseUp updates the select
}

void InputSys::eventMouseLeave() {
	SDL_SetRelativeMouseMode(SDL_FALSE);
	World::scene()->onMouseLeave();
}

void InputSys::simulateMouseMove() {
	if (ivec2 pos; mouseLast)
		eventMouseMotion({ SDL_MOUSEMOTION, SDL_GetTicks(), 0, 0, SDL_GetMouseState(&pos.x, &pos.y), pos.x, pos.y, 0, 0 }, mouseLast);
	else
		eventMouseMotion({ SDL_FINGERMOTION, SDL_GetTicks(), 0, SDL_TOUCH_MOUSEID, 0, INT_MIN, INT_MIN, 0, 0 }, mouseLast);
}

template <class K, class T, class M>
void InputSys::checkInput(const mumap<K, Binding::Type>& kmap, T key, M Binding::*call, Binding::Type lim) const {
	for (auto [pos, end] = kmap.equal_range(key); pos != end; ++pos)
		if (pos->second < lim)
			World::srun(bindings[uint8(pos->second)].*call);
}

void InputSys::tick() {
	if (!World::scene()->getCapture())	// handle key hold
		for (uint8 i = uint8(Binding::holders); i < bindings.size(); ++i)
			if (int16 amt = isPressed(bindings[i]))
				World::srun(bindings[i].acall, Controller::axisToFloat(amt));
}

int16 InputSys::isPressed(const Binding& bind) const {
	const uint8* kstate = SDL_GetKeyboardState(nullptr);
	for (SDL_Scancode sc : bind.keys)
		if (kstate[sc])
			return INT16_MAX;

	for (AsgJoystick aj : bind.joys)
		switch (aj.getAsg()) {
		case AsgJoystick::button:
			for (auto& [id, joy] : joysticks)
				if (SDL_JoystickGetButton(joy.getCtr(), aj.getJct()))
					return INT16_MAX;
			break;
		case AsgJoystick::hat:
			for (auto& [id, joy] : joysticks)
				if (aj.getJct() < SDL_JoystickNumHats(joy.getCtr()) && SDL_JoystickGetHat(joy.getCtr(), aj.getJct()) == aj.getHvl())
					return INT16_MAX;
			break;
		case AsgJoystick::axisPos:
			if (int16 val = getAxis(aj.getJct()); val > 0)
				return val;
			break;
		case AsgJoystick::axisNeg:
			if (int16 val = getAxis(aj.getJct()); val < 0)
				return -(val + 1);	// adjust for INT16_MIN
		}

	for (AsgGamepad ag : bind.gpds)
		switch (ag.getAsg()) {
		case AsgGamepad::button:
			for (auto& [id, pad] : gamepads)
				if (SDL_GameControllerGetButton(pad.getCtr(), ag.getButton()))
					return INT16_MAX;
			break;
		case AsgGamepad::axisPos:
			if (int16 val = getAxis(ag.getAxis()); val > 0)
				return val;
			break;
		case AsgGamepad::axisNeg:
			if (int16 val = getAxis(ag.getAxis()); val < 0)
				return -(val + 1);	// adjust for INT16_MIN
		}
	return 0;
}

int16 InputSys::getAxis(uint8 jaxis) const {
	for (auto& [id, joy] : joysticks)	// get first axis that isn't 0
		if (int16 val = Controller::axisValue(SDL_JoystickGetAxis(joy.getCtr(), jaxis)))
			return val;
	return 0;
}

int16 InputSys::getAxis(SDL_GameControllerAxis gaxis) const {
	for (auto& [id, pad] : gamepads)	// get first axis that isn't 0
		if (int16 val = Controller::axisValue(SDL_GameControllerGetAxis(pad.getCtr(), gaxis)))
			return val;
	return 0;
}

vector<SDL_Joystick*> InputSys::listJoysticks() const {
	vector<SDL_Joystick*> joys(joysticks.size() + gamepads.size());
	sizet i = 0;
	for (auto& [id, joy] : joysticks)
		joys[i++] = joy.getCtr();
	for (auto& [id, pad] : gamepads)
		joys[i++] = SDL_GameControllerGetJoystick(pad.getCtr());
	return joys;
}

void InputSys::addController(int id) {
	if (SDL_GameController* pad = SDL_GameControllerOpen(id)) {
		if (SDL_JoystickID jid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)); jid >= 0)
			gamepads.emplace(jid, pad);
		else
			SDL_GameControllerClose(pad);
	} else if (SDL_Joystick* joy = SDL_JoystickOpen(id)) {
		if (SDL_JoystickID jid = SDL_JoystickInstanceID(joy); jid >= 0)
			joysticks.emplace(jid, joy);
		else
			SDL_JoystickClose(joy);
	}
}

void InputSys::delController(SDL_JoystickID id) {
	if (!tryDelController(joysticks, id))
		tryDelController(gamepads, id);
}

template <class T>
bool InputSys::tryDelController(umap<SDL_JoystickID, T>& ctrls, SDL_JoystickID id) {
	if (typename umap<SDL_JoystickID, T>::iterator it = ctrls.find(id); it != ctrls.end()) {
		it->second.close();
		ctrls.erase(it);
		return true;
	}
	return false;
}

void InputSys::setBinding(Binding::Type bid, sizet kid, SDL_Scancode sc) {
	setBinding(bid, keymap, sc, bindings[uint8(bid)].keys[kid]);
}

void InputSys::setBinding(Binding::Type bid, sizet kid, AsgJoystick aj) {
	setBinding(bid, joymap, aj, bindings[uint8(bid)].joys[kid]);
}

void InputSys::setBinding(Binding::Type bid, sizet kid, AsgGamepad ag) {
	setBinding(bid, padmap, ag, bindings[uint8(bid)].gpds[kid]);
}

template <class T, class M>
void InputSys::setBinding(Binding::Type bid, mumap<T, Binding::Type>& kmap, T key, M& kref) {
	typename mumap<T, Binding::Type>::node_type node = kmap.extract(findMapBind(bid, kref, kmap));
	node.key() = key;
	kmap.insert(std::move(node));
	kref = key;
}

sizet InputSys::addBinding(Binding::Type bid, SDL_Scancode sc) {
	return addBinding(bid, sc, keymap, &Binding::keys);
}

sizet InputSys::addBinding(Binding::Type bid, AsgJoystick aj) {
	return addBinding(bid, aj, joymap, &Binding::joys);
}

sizet InputSys::addBinding(Binding::Type bid, AsgGamepad ag) {
	return addBinding(bid, ag, padmap, &Binding::gpds);
}

template <class T, class M>
sizet InputSys::addBinding(Binding::Type bid, T key, mumap<T, Binding::Type>& kmap, M bkey) {
	if (std::none_of((bindings[uint8(bid)].*bkey).begin(), (bindings[uint8(bid)].*bkey).end(), [key](T k) -> bool { return k == key; })) {
		kmap.emplace(key, bid);
		(bindings[uint8(bid)].*bkey).push_back(key);
		return (bindings[uint8(bid)].*bkey).size() - 1;
	}
	return SIZE_MAX;	// key already assigned to this binding
}

void InputSys::delBindingK(Binding::Type bid, sizet kid) {
	delBinding(bid, kid, &Binding::keys, keymap);
}

void InputSys::delBindingJ(Binding::Type bid, sizet kid) {
	delBinding(bid, kid, &Binding::joys, joymap);
}

void InputSys::delBindingG(Binding::Type bid, sizet kid) {
	delBinding(bid, kid, &Binding::gpds, padmap);
}

template <class T, class M>
void InputSys::delBinding(Binding::Type bid, sizet kid, M bkey, mumap<T, Binding::Type>& kmap) {
	kmap.erase(findMapBind(bid, (bindings[uint8(bid)].*bkey)[kid], kmap));
	(bindings[uint8(bid)].*bkey).erase((bindings[uint8(bid)].*bkey).begin() + kid);
}

template <class T>
typename mumap<T, Binding::Type>::iterator InputSys::findMapBind(Binding::Type bid, T key, mumap<T, Binding::Type>& kmap) {
	auto [pos, end] = kmap.equal_range(key);
	return std::find_if(pos, end, [bid](const pair<const T, Binding::Type>& it) -> bool { return it.second == bid; });
}

void InputSys::clearBindings() {
	for (Binding& it : bindings) {
		it.keys.clear();
		it.joys.clear();
		it.gpds.clear();
	}
	keymap.clear();
	joymap.clear();
	padmap.clear();
}

void InputSys::resetBindings() {
	keymap.clear();
	joymap.clear();
	padmap.clear();
	for (uint8 i = 0; i < bindings.size(); ++i) {
		bindings[i].reset(Binding::Type(i));
		for (SDL_Scancode sc : bindings[i].keys)
			keymap.emplace(sc, Binding::Type(i));
		for (AsgJoystick aj : bindings[i].joys)
			joymap.emplace(aj, Binding::Type(i));
		for (AsgGamepad ag : bindings[i].gpds)
			padmap.emplace(ag, Binding::Type(i));
	}
}
