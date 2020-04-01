#pragma once

#include "utils/text.h"

// handles input events and contains controls settings
class InputSys {
public:
	bool mouseLast;		// last input was mouse or touch

	static constexpr uint8 jbutA = 2;
	static constexpr uint8 jbutB = 1;
	static constexpr uint8 jbutX = 3;
	static constexpr uint8 jbutY = 0;
	static constexpr uint8 jbutLS = 4;
	static constexpr uint8 jbutRS = 5;
	static constexpr uint8 jbutBack = 8;
private:
	static constexpr uint32 moveTimeout = 50;
	static constexpr float axisLimit = SHRT_MAX;
	static constexpr int deadzone = 4096;

	struct Controller {
		SDL_GameController* gamepad;	// for xinput
		SDL_Joystick* joystick;			// for direct input
		int index;

		Controller(int id);

		void close();
	};

	vector<Controller> controllers;	// currently connected game controllers
	ivec2 mouseMove;	// last recorded cursor position difference
	uint32 moveTime;	// timestamp of last recorded mouseMove

public:
	InputSys();
	~InputSys();

	void eventMouseMotion(const SDL_MouseMotionEvent& motion, bool mouse = true);
	void eventMouseButtonDown(const SDL_MouseButtonEvent& button, bool mouse = true);
	void eventMouseButtonUp(const SDL_MouseButtonEvent& button, bool mouse = true);
	void eventMouseWheel(const SDL_MouseWheelEvent& wheel);
	void eventKeyDown(const SDL_KeyboardEvent& key);
	void eventKeyUp(const SDL_KeyboardEvent& key);
	void eventJoystickButton(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventFingerMove(const SDL_TouchFingerEvent& fin);
	void eventFingerGesture(const SDL_MultiGestureEvent& ges);
	void eventFingerDown(const SDL_TouchFingerEvent& fin);
	void eventFingerUp(const SDL_TouchFingerEvent& fin);
	void tick(float dSec);

	void addController(int id);
	void removeController(int id);
	ivec2 getMouseMove() const;
	void simulateMouseMove();

private:
	static int getAxisValue(const Controller& ctr, uint8 jid, SDL_GameControllerAxis gid);
	static float axisToFloat(int axisValue);
};

inline ivec2 InputSys::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : ivec2(0);
}

inline float InputSys::axisToFloat(int axisValue) {
	return float(axisValue) / axisLimit;
}
