#include "world.h"

// CONTROLLER

bool InputSys::Controller::open(int id) {
	gamepad = SDL_IsGameController(id) ? SDL_GameControllerOpen(id) : nullptr;
	joystick = gamepad ? SDL_GameControllerGetJoystick(gamepad) : SDL_JoystickOpen(id);
	return gamepad || joystick;
}

void InputSys::Controller::close() {
	if (gamepad)
		SDL_GameControllerClose(gamepad);
	else if (joystick)
		SDL_JoystickClose(joystick);
}

// INPUT SYS

InputSys::InputSys() :
	mouseLast(false),
	mouseMove(0),
	moveTime(0),
	lastJAxes{},
	lastGAxes{},
	lastJAxisOn(0),
	lastGAxisOn(0)
{
	resetControllers();
}

InputSys::~InputSys() {
	for (Controller& it : controllers)
		it.close();
}

void InputSys::eventMouseMotion(const SDL_MouseMotionEvent& motion, bool mouse) {
	mouseLast = mouse;
	mouseMove = ivec2(motion.xrel, motion.yrel);
	moveTime = motion.timestamp;
	if (!SDL_GetRelativeMouseMode() || World::scene()->capture || World::scene()->camera.moving)
		World::scene()->onMouseMove(ivec2(motion.x, motion.y), mouseMove, motion.state);
	else {
		vec2 rot(glm::radians(float(motion.yrel) / 2.f), glm::radians(float(-motion.xrel) / 2.f));
		World::scene()->camera.rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
	}
}

void InputSys::eventMouseButtonDown(const SDL_MouseButtonEvent& button, bool mouse) {
	mouseLast = mouse;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseDown(ivec2(button.x, button.y), button.button);
		break;
	case SDL_BUTTON_MIDDLE:
#ifndef EMSCRIPTEN
		SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
		break;
	case SDL_BUTTON_X1:
		World::scene()->onXbutCancel();
		break;
	case SDL_BUTTON_X2:
		World::scene()->onXbutConfirm();
	}
}

void InputSys::eventMouseButtonUp(const SDL_MouseButtonEvent& button, bool mouse) {
	mouseLast = mouse;
	switch (button.button) {
	case SDL_BUTTON_LEFT: case SDL_BUTTON_RIGHT:
		World::scene()->onMouseUp(ivec2(button.x, button.y), button.button);
		break;
	case SDL_BUTTON_MIDDLE:
#ifdef EMSCRIPTEN
		break;
#else
		SDL_SetRelativeMouseMode(SDL_FALSE);
		simulateMouseMove();
#endif
	}
}

void InputSys::eventMouseWheel(const SDL_MouseWheelEvent& wheel) {
	mouseLast = true;
	World::scene()->onMouseWheel(ivec2(wheel.x, wheel.y));
}

void InputSys::eventKeyDown(const SDL_KeyboardEvent& key) {
	if (World::scene()->capture)
		World::scene()->capture->onKeypress(key);
	else switch (key.keysym.scancode) {
	case SDL_SCANCODE_UP:
		World::scene()->navSelect(Direction::up, mouseLast);
		break;
	case SDL_SCANCODE_DOWN:
		World::scene()->navSelect(Direction::down, mouseLast);
		break;
	case SDL_SCANCODE_LEFT:
		World::scene()->navSelect(Direction::left, mouseLast);
		break;
	case SDL_SCANCODE_RIGHT:
		World::scene()->navSelect(Direction::right, mouseLast);
		break;
	case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5: case SDL_SCANCODE_6: case SDL_SCANCODE_7: case SDL_SCANCODE_8: case SDL_SCANCODE_9: case SDL_SCANCODE_0:
		if (!key.repeat)
			World::state()->eventNumpress(uint8(key.keysym.scancode - SDL_SCANCODE_1));
		break;
	case SDL_SCANCODE_KP_1: case SDL_SCANCODE_KP_2: case SDL_SCANCODE_KP_3: case SDL_SCANCODE_KP_4: case SDL_SCANCODE_KP_5: case SDL_SCANCODE_KP_6: case SDL_SCANCODE_KP_7: case SDL_SCANCODE_KP_8: case SDL_SCANCODE_KP_9: case SDL_SCANCODE_KP_0:
		if (!key.repeat)
			World::state()->eventNumpress(uint8(key.keysym.scancode - SDL_SCANCODE_KP_1));
		break;
	case SDL_SCANCODE_SPACE:
		if (!key.repeat)
			World::state()->eventEndTurn();
		break;
	case SDL_SCANCODE_LALT:
#ifndef EMSCRIPTEN
		if (!key.repeat)
			SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
		break;
	case SDL_SCANCODE_C:
		if (!key.repeat)
			World::state()->eventCameraReset();
		break;
	case SDL_SCANCODE_X:
		if (!key.repeat)
			World::state()->eventEngage();
		break;
	case SDL_SCANCODE_D:
		if (!key.repeat)
			World::state()->eventDestroy(Switch::on);
		break;
	case SDL_SCANCODE_S:
		if (!key.repeat)
			World::state()->eventSurrender();
		break;
	case SDL_SCANCODE_TAB:
		if (!key.repeat)
			World::program()->eventToggleChat();
		break;
	case SDL_SCANCODE_P:
		if (!key.repeat)
			World::program()->eventCycleFrameCounter();
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
	if (World::scene()->capture)
		World::scene()->capture->onKeyrelease(key);
	else switch (key.keysym.scancode) {
	case SDL_SCANCODE_LALT:
#ifndef EMSCRIPTEN
		if (!key.repeat) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			simulateMouseMove();
		}
#endif
		break;
	case SDL_SCANCODE_D:
		if (!key.repeat)
			World::state()->eventDestroy(Switch::off);
	}
}

void InputSys::eventJoystickButton(const SDL_JoyButtonEvent& jbutton) {
	if (SDL_GameControllerFromInstanceID(jbutton.which))	// don't execute if there can be a game controller event
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJButton(jbutton.button);
	else switch (jbutton.button) {
	case jbutL1:
		World::state()->eventFavor(Favor::hasten);
		break;
	case jbutR1:
		World::state()->eventFavor(Favor::assault);
		break;
	case jbutL2:
		World::state()->eventFavor(Favor::conspire);
		break;
	case jbutR2:
		World::state()->eventFavor(Favor::deceive);
		break;
	case jbutY:
		World::state()->eventEndTurn();
		break;
	case jbutLS:
		World::state()->eventCameraReset();
		break;
	case jbutX:
		World::state()->eventEngage();
		break;
	case jbutStart:
		World::state()->eventDestroy(Switch::toggle);
		break;
	case jbutGuide:
		World::state()->eventSurrender();
		break;
	case jbutBack:
		World::program()->eventToggleChat();
		break;
	case jbutRS:
		World::program()->eventCycleFrameCounter();
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
			World::scene()->navSelect(Direction::up, mouseLast);
		if (jhat.value & SDL_HAT_DOWN)
			World::scene()->navSelect(Direction::down, mouseLast);
		if (jhat.value & SDL_HAT_LEFT)
			World::scene()->navSelect(Direction::left, mouseLast);
		if (jhat.value & SDL_HAT_RIGHT)
			World::scene()->navSelect(Direction::right, mouseLast);
	}
}

void InputSys::eventJoystickAxis(const SDL_JoyAxisEvent& jaxis) {
	if (jaxis.axis >= joyAxisMax || SDL_GameControllerFromInstanceID(jaxis.which) || !axisDown(jaxis.value, jaxis.axis, lastJAxisOn, lastJAxes))
		return;

	if (World::scene()->capture)
		World::scene()->capture->onJAxis(jaxis.axis);
}

void InputSys::eventGamepadButton(const SDL_ControllerButtonEvent& gbutton) {
	if (World::scene()->capture)
		World::scene()->capture->onGButton(SDL_GameControllerButton(gbutton.button));
	else switch (gbutton.button) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		World::scene()->navSelect(Direction::up, mouseLast);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		World::scene()->navSelect(Direction::down, mouseLast);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		World::scene()->navSelect(Direction::left, mouseLast);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		World::scene()->navSelect(Direction::right, mouseLast);
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		World::state()->eventFavor(Favor::hasten);
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		World::state()->eventFavor(Favor::assault);
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		World::state()->eventEndTurn();
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		World::state()->eventCameraReset();
		break;
	case SDL_CONTROLLER_BUTTON_X:
		World::state()->eventEngage();
		break;
	case SDL_CONTROLLER_BUTTON_START:
		World::state()->eventDestroy(Switch::toggle);
		break;
	case SDL_CONTROLLER_BUTTON_GUIDE:
		World::state()->eventSurrender();
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		World::program()->eventToggleChat();
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		World::program()->eventCycleFrameCounter();
		break;
	case SDL_CONTROLLER_BUTTON_A:
		World::scene()->onConfirm();
		break;
	case SDL_CONTROLLER_BUTTON_B:
		World::scene()->onCancel();
	}
}

void InputSys::eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis) {
	if (!axisDown(gaxis.value, gaxis.axis, lastGAxisOn, lastGAxes))
		return;

	if (World::scene()->capture)
		World::scene()->capture->onGAxis(SDL_GameControllerAxis(gaxis.axis));
	else switch (gaxis.axis) {
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		World::state()->eventFavor(Favor::conspire);
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		World::state()->eventFavor(Favor::deceive);
	}
}

void InputSys::eventFingerMove(const SDL_TouchFingerEvent& fin) {
	vec2 size = World::window()->getScreenView();
	eventMouseMotion({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LMASK, int(fin.x * size.x), int(fin.y * size.y), int(fin.dx * size.x), int(fin.dy * size.y) }, false);
}

void InputSys::eventFingerGesture(const SDL_MultiGestureEvent& ges) {
	if (dynamic_cast<ProgMatch*>(World::state()) && ges.numFingers == 2 && std::abs(ges.dDist) > FLT_EPSILON)
		World::scene()->camera.zoom(int(ges.dDist * float(World::window()->getScreenView().y)));
}

void InputSys::eventFingerDown(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::window()->getScreenView());
	eventMouseButtonDown({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_PRESSED, 1, 0, pos.x, pos.y }, false);
}

void InputSys::eventFingerUp(const SDL_TouchFingerEvent& fin) {
	ivec2 pos = vec2(fin.x, fin.y) * vec2(World::window()->getScreenView());
	eventMouseButtonUp({ fin.type, fin.timestamp, 0, SDL_TOUCH_MOUSEID, SDL_BUTTON_LEFT, SDL_RELEASED, 1, 0, pos.x, pos.y }, false);
	World::scene()->updateSelect(ivec2(-1));
}

void InputSys::tick(float dSec) {
	if (World::scene()->capture)
		return;

	if (int16 ax = lastAxisValue(jaxisLX, SDL_CONTROLLER_AXIS_LEFTX), ay = lastAxisValue(jaxisLY, SDL_CONTROLLER_AXIS_LEFTY); ax || ay) {
		vec2 rot(axisToFloat(-ay) * dSec, axisToFloat(ax) * dSec);
		World::scene()->camera.rotate(rot, dynamic_cast<ProgMatch*>(World::state()) ? rot.y : 0.f);
	}
	if (int16 ax = lastAxisValue(jaxisRX, SDL_CONTROLLER_AXIS_RIGHTX), ay = lastAxisValue(jaxisRY, SDL_CONTROLLER_AXIS_RIGHTY); ax || ay)
		World::state()->eventSecondaryAxis(ivec2(ax, ay), dSec);
}

void InputSys::resetControllers() {
	for (Controller& it : controllers)
		it.close();
	controllers.clear();

	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (Controller ctr; ctr.open(i))
			controllers.push_back(ctr);
		else
			ctr.close();
	}
}

bool InputSys::axisDown(int16 val, uint8 id, uint8& lastOn, int16* vrec) {
	bool curOn = std::abs(int(val)) > int(World::sets()->deadzone);
	vrec[id] = curOn ? val : 0;
	uint8 flg = 1 << id;
	if (curOn == bool(lastOn & flg))
		return false;
	lastOn = curOn ? lastOn | flg : lastOn & ~flg;
	return curOn;
}

void InputSys::simulateMouseMove() {
	if (ivec2 pos; mouseLast) {
		uint32 state = SDL_GetMouseState(&pos.x, &pos.y);
		eventMouseMotion({ SDL_MOUSEMOTION, SDL_GetTicks(), 0, 0, state, pos.x, pos.y, 0, 0 }, mouseLast);
	} else
		eventMouseMotion({ SDL_FINGERMOTION, SDL_GetTicks(), 0, SDL_TOUCH_MOUSEID, 0, -1, -1, 0, 0 }, mouseLast);
}
