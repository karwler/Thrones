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
	static constexpr uint8 jbutL1 = 4;
	static constexpr uint8 jbutL2 = 6;
	static constexpr uint8 jbutR1 = 5;
	static constexpr uint8 jbutR2 = 7;
	static constexpr uint8 jbutLS = 10;
	static constexpr uint8 jbutRS = 11;
	static constexpr uint8 jbutBack = 8;
	static constexpr uint8 jbutStart = 9;
	static constexpr uint8 jbutGuide = 12;
	static constexpr uint8 jaxisLX = 0;
	static constexpr uint8 jaxisLY = 1;
	static constexpr uint8 jaxisRX = 2;
	static constexpr uint8 jaxisRY = 3;
private:
	static constexpr uint32 moveTimeout = 50;
	static constexpr uint8 joyAxisMax = 4;
	static constexpr float axisLimit = SHRT_MAX;

	struct Controller {
		SDL_GameController* gamepad;
		SDL_Joystick* joystick;

		bool open(int id);
		void close();
	};

	vector<Controller> controllers;	// currently connected game controllers
	ivec2 mouseMove;				// last recorded cursor position difference
	uint32 moveTime;				// timestamp of last recorded mouseMove
	int16 lastJAxes[joyAxisMax], lastGAxes[SDL_CONTROLLER_AXIS_MAX];	// last values of controllers' axes
	uint8 lastJAxisOn, lastGAxisOn;	// flag states of joystick/gamepad axes as buttons

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
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButton(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);
	void eventFingerMove(const SDL_TouchFingerEvent& fin);
	void eventFingerGesture(const SDL_MultiGestureEvent& ges);
	void eventFingerDown(const SDL_TouchFingerEvent& fin);
	void eventFingerUp(const SDL_TouchFingerEvent& fin);
	void tick(float dSec);

	void resetControllers();
	ivec2 getMouseMove() const;
	void simulateMouseMove();

private:
	int16 lastAxisValue(uint8 jid, SDL_GameControllerAxis gid) const;
	static bool axisDown(int16 val, uint8 id, uint8& lastOn, int16* vrec);
	static float axisToFloat(int axisValue);
};

inline ivec2 InputSys::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : ivec2(0);
}

inline int16 InputSys::lastAxisValue(uint8 jid, SDL_GameControllerAxis gid) const {
	return lastGAxes[gid] ? lastGAxes[gid] : lastJAxes[jid];
}

inline float InputSys::axisToFloat(int axisValue) {
	return float(axisValue) / axisLimit;
}
