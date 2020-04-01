#include "world.h"

// CONTROLLER

InputSys::Controller::Controller(int id) :
	gamepad(SDL_IsGameController(id) ? SDL_GameControllerOpen(id) : nullptr),
	joystick(gamepad ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id)),
	index(joystick ? id : -1)
{}

void InputSys::Controller::close() {
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = nullptr;
		joystick = nullptr;
	} else if (joystick) {
		SDL_JoystickClose(joystick);
		joystick = nullptr;
	}
}

// INPUT SYS

InputSys::InputSys() :
	mouseLast(false),
	mouseMove(0),
	moveTime(0)
{
	for (int i = 0; i < SDL_NumJoysticks(); i++)
		addController(i);
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion, bool mouse) {
	mouseLast = mouse;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	if (!SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || World::scene()->capture || World::scene()->getCamera()->moving)
		World::scene()->onMouseMove(ivec2(motion.x, motion.y), mouseMove, motion.state);
	else {
		vec2 rot(glm::radians(float(motion.yrel) / 2.f), glm::radians(float(-motion.xrel) / 2.f));
		World::scene()->getCamera()->rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
	}
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button, bool mouse) {
	switch (mouseLast = mouse; button.button) {
	case SDL_BUTTON_MIDDLE:
		World::state()->eventEndTurn();
		break;
	case SDL_BUTTON_X1:
		if (dynamic_cast<ProgMatch*>(World::state()))
			World::state()->eventFavorize(FavorAct::on);
		else
			World::state()->eventEscape();
		break;
	case SDL_BUTTON_X2:
		if (dynamic_cast<ProgMatch*>(World::state()))
			World::state()->eventFavorize(FavorAct::now);
		else
			World::state()->eventEnter();
		break;
	default:
		World::scene()->onMouseDown(ivec2(button.x, button.y), button.button);
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button, bool mouse) {
	if (mouseLast = mouse; button.button != SDL_BUTTON_X1 && button.button != SDL_BUTTON_X2)
		World::scene()->onMouseUp(ivec2(button.x, button.y), button.button);
	else
		World::state()->eventFavorize(FavorAct::off);
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseLast = true;
	World::scene()->onMouseWheel(ivec2(wheel.x, wheel.y));
}

void InputSys::eventKeyDown(const SDL_KeyboardEvent& key) {
	if (World::scene()->capture)
		World::scene()->capture->onKeypress(key.keysym);
	else switch (key.keysym.scancode) {
	case SDL_SCANCODE_UP:
		World::scene()->navSelect(Direction::up);
		break;
	case SDL_SCANCODE_DOWN:
		World::scene()->navSelect(Direction::down);
		break;
	case SDL_SCANCODE_LEFT:
		World::scene()->navSelect(Direction::left);
		break;
	case SDL_SCANCODE_RIGHT:
		World::scene()->navSelect(Direction::right);
		break;
	case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5: case SDL_SCANCODE_6: case SDL_SCANCODE_7: case SDL_SCANCODE_8: case SDL_SCANCODE_9: case SDL_SCANCODE_0:
		if (!key.repeat)
			World::state()->eventNumpress(uint8(key.keysym.scancode - SDL_SCANCODE_1));
		break;
	case SDL_SCANCODE_LALT:
		if (!key.repeat)
			World::state()->eventFavorize(FavorAct::on);
		break;
	case SDL_SCANCODE_LSHIFT:
		if (!key.repeat)
			World::state()->eventFavorize(FavorAct::now);
		break;
	case SDL_SCANCODE_SPACE:
		if (!key.repeat)
			World::state()->eventEndTurn();
		break;
	case SDL_SCANCODE_LCTRL:
#ifndef EMSCRIPTEN
		if (!key.repeat)
			SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
		break;
	case SDL_SCANCODE_C:
		if (!key.repeat)
			World::state()->eventCameraReset();
		break;
	case SDL_SCANCODE_TAB:
		if (!key.repeat)
			World::program()->eventToggleChat();
		break;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER:
		if (!key.repeat)
			World::scene()->onConfirm();
		break;
	case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_AC_BACK:
		if (!key.repeat)
			World::scene()->onCancel();
	}
}

void InputSys::eventKeyUp(const SDL_KeyboardEvent& key) {
	if (!(dynamic_cast<LabelEdit*>(World::scene()->capture) || key.repeat))
		switch (key.keysym.scancode) {
		case SDL_SCANCODE_LALT: case SDL_SCANCODE_LSHIFT:
			World::state()->eventFavorize(FavorAct::off);
			break;
		case SDL_SCANCODE_LCTRL:
#ifndef EMSCRIPTEN
			SDL_SetRelativeMouseMode(SDL_FALSE);
			simulateMouseMove();
#endif
		}
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a gamecontroller event
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJButton(jbutton.button);
	else switch (jbutton.button) {
	case jbutRS:
		World::state()->eventFavorize(FavorAct::on);
		break;
	case jbutLS:
		World::state()->eventFavorize(FavorAct::now);
		break;
	case jbutX:
		World::state()->eventEndTurn();
		break;
	case jbutBack:
		World::state()->eventCameraReset();
		break;
	case jbutY:
		World::program()->eventToggleChat();
		break;
	case jbutA:
		World::scene()->onConfirm();
		break;
	case jbutB:
		World::scene()->onCancel();
	}
}

void InputSys::eventJoystickHat(const SDL_JoyHatEvent& jhat) {
	if (jhat.value == SDL_HAT_CENTERED || SDL_GameControllerFromInstanceID(jhat.which))
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJHat(jhat.value);
	else {
		if (jhat.value & SDL_HAT_UP)
			World::scene()->navSelect(Direction::up);
		if (jhat.value & SDL_HAT_DOWN)
			World::scene()->navSelect(Direction::down);
		if (jhat.value & SDL_HAT_LEFT)
			World::scene()->navSelect(Direction::left);
		if (jhat.value & SDL_HAT_RIGHT)
			World::scene()->navSelect(Direction::right);
	}
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->capture)
		World::scene()->capture->onGButton(SDL_GameControllerButton(gbutton.button));
	else switch (gbutton.button) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		World::scene()->navSelect(Direction::up);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		World::scene()->navSelect(Direction::down);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		World::scene()->navSelect(Direction::left);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		World::scene()->navSelect(Direction::right);
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		World::state()->eventFavorize(FavorAct::on);
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		World::state()->eventFavorize(FavorAct::now);
		break;
	case SDL_CONTROLLER_BUTTON_X:
		World::state()->eventEndTurn();
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		World::state()->eventCameraReset();
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		World::program()->eventToggleChat();
		break;
	case SDL_CONTROLLER_BUTTON_A:
		World::scene()->onConfirm();
		break;
	case SDL_CONTROLLER_BUTTON_B:
		World::scene()->onCancel();
	}
}

void InputSys::eventFingerMove(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::window()->screenView();
	eventMouseMotion({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LMASK, int(fin.x * size.x), int(fin.y * size.y), int(fin.dx * size.x), int(fin.dy * size.y) }, false);
}

void InputSys::eventFingerGesture(const SDL_MultiGestureEvent& ges) {
	if (dynamic_cast<ProgMatch*>(World::state()) && ges.numFingers == 2 && std::abs(ges.dDist) > FLT_EPSILON)
		World::scene()->getCamera()->zoom(int(ges.dDist * float(World::window()->screenView().y)));
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::window()->screenView());
	World::scene()->updateSelect(pos);
	eventMouseButtonDown({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_PRESSED, 1, 0, pos.x, pos.y }, false);
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::window()->screenView();
	eventMouseButtonUp({ fin.type, fin.timestamp, World::window()->windowID(), SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_RELEASED, 1, 0, int(fin.x * size.x), int(fin.y * size.y) }, false);
	World::scene()->updateSelect(ivec2(-1));
}

void InputSys::tick(float dSec) {
	for (const Controller& it : controllers)
		if (int ax = getAxisValue(it, 0, SDL_CONTROLLER_AXIS_LEFTX), ay = getAxisValue(it, 1, SDL_CONTROLLER_AXIS_LEFTY); ax || ay) {
			vec2 rot(axisToFloat(ax) * dSec, axisToFloat(ay) * dSec);
			World::scene()->getCamera()->rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
		}
}

int InputSys::getAxisValue(const Controller& ctr, uint8 jid, SDL_GameControllerAxis gid) {
	int val = ctr.gamepad ? SDL_GameControllerGetAxis(ctr.gamepad, gid) : SDL_JoystickGetAxis(ctr.joystick, jid);
	return std::abs(val) > deadzone ? val : 0;
}

void InputSys::addController(int id) {
	if (Controller ctr(id); ctr.index >= 0)
		controllers.push_back(ctr);
	else
		ctr.close();
}

void InputSys::removeController(int id) {
	if (vector<Controller>::iterator it = std::find_if(controllers.begin(), controllers.end(), [id](const Controller& ci) -> bool { return ci.index == id; }); it != controllers.end()) {
		it->close();
		controllers.erase(it);
	}
}

void InputSys::simulateMouseMove() {
	if (ivec2 pos; mouseLast) {
		uint32 state = SDL_GetMouseState(&pos.x, &pos.y);
		eventMouseMotion({ SDL_MOUSEMOTION, SDL_GetTicks(), World::window()->windowID(), 0, state, pos.x, pos.y, 0, 0 }, mouseLast);
	} else
		eventMouseMotion({ SDL_FINGERMOTION, SDL_GetTicks(), World::window()->windowID(), SDL_TOUCH_MOUSEID, 0, -1, -1, 0, 0 }, mouseLast);
}
