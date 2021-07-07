#pragma once

#include "utils/settings.h"

class Controller {
public:
	static int16 axisValue(int16 value);	// check deadzone in axis value
	static float axisToFloat(int16 value);
protected:
	static optional<bool> axisDown(int16 val, uint8 id, bool* vrec);	// returns null if there's no state change
};

inline float Controller::axisToFloat(int16 value) {
	return float(value) / float(INT16_MAX);
}

class Joystick : public Controller {
private:
	SDL_Joystick* ctr;
	uptr<uint8[]> hats;	// last read hat states
	uptr<bool[]> axes;		// last read axis values

public:
	Joystick(SDL_Joystick* joy);

	void close();
	SDL_Joystick* getCtr() const;
	uint8 hatDown(uint8 val, uint8 id);	// returns the previous state
	optional<bool> axisDown(int16 val, uint8 id);
private:
	template<class T> static uptr<T[]> initRecord(int num, T fill);
};

inline SDL_Joystick* Joystick::getCtr() const {
	return ctr;
}

inline optional<bool> Joystick::axisDown(int16 val, uint8 id) {
	return Controller::axisDown(val, id, axes.get());
}

class Gamepad : public Controller {
private:
	SDL_GameController* ctr;
	bool axes[SDL_CONTROLLER_AXIS_MAX+2]{};	// last read axis values as booleans (add 2 because there's padding anyway)

public:
	Gamepad(SDL_GameController* pad);

	void close();
	SDL_GameController* getCtr() const;
	optional<bool> axisDown(int16 val, uint8 id);
};

inline Gamepad::Gamepad(SDL_GameController* pad) :
	ctr(pad)
{}

inline void Gamepad::close() {
	SDL_GameControllerClose(ctr);
}

inline SDL_GameController* Gamepad::getCtr() const {
	return ctr;
}

inline optional<bool> Gamepad::axisDown(int16 val, uint8 id) {
	return Controller::axisDown(val, id, axes);
}

// handles input events and contains controls settings
class InputSys {
public:
	static inline const umap<uint8, const char*> hatNames = {
		pair(SDL_HAT_UP, "Up"),
		pair(SDL_HAT_RIGHT, "Right"),
		pair(SDL_HAT_DOWN, "Down"),
		pair(SDL_HAT_LEFT, "Left")
	};
	static constexpr array<const char*, SDL_CONTROLLER_BUTTON_MAX> gbuttonNames = {
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Guide",
		"Start",
		"LS",
		"RS",
		"LB",
		"RB",
		"Up",
		"Down",
		"Left",
		"Right"
	};
	static constexpr array<const char*, SDL_CONTROLLER_AXIS_MAX> gaxisNames = {
		"LX",
		"LY",
		"RX",
		"RY",
		"LT",
		"RT"
	};

	bool mouseLast = false;		// last input was mouse or touch
private:
	umap<SDL_JoystickID, Joystick> joysticks;	// currently connected joysticks
	umap<SDL_JoystickID, Gamepad> gamepads;		// currently connected game controllers
	array<Binding, Binding::names.size()> bindings;
	mumap<SDL_Scancode, Binding::Type> keymap;
	mumap<AsgJoystick, Binding::Type> joymap;
	mumap<AsgGamepad, Binding::Type> padmap;
	ivec2 mouseMove = { 0, 0 };	// last recorded cursor position difference
	uint32 moveTime = 0;		// timestamp of last recorded mouseMove

	static constexpr uint32 moveTimeout = 50;

public:
	InputSys();
	~InputSys();

	void eventMouseMotion(const SDL_MouseMotionEvent& motion, bool mouse = true);
	void eventMouseButtonDown(const SDL_MouseButtonEvent& button, bool mouse = true);
	void eventMouseButtonUp(const SDL_MouseButtonEvent& button, bool mouse = true);
	void eventMouseWheel(const SDL_MouseWheelEvent& wheel);
	void eventKeyDown(const SDL_KeyboardEvent& key);
	void eventKeyUp(const SDL_KeyboardEvent& key);
	void eventJoystickButtonDown(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickButtonUp(const SDL_JoyButtonEvent& jbutton);
	void eventJoystickHat(const SDL_JoyHatEvent& jhat);
	void eventJoystickAxis(const SDL_JoyAxisEvent& jaxis);
	void eventGamepadButtonDown(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadButtonUp(const SDL_ControllerButtonEvent& gbutton);
	void eventGamepadAxis(const SDL_ControllerAxisEvent& gaxis);
	void eventFingerMove(const SDL_TouchFingerEvent& fin);
	void eventFingerGesture(const SDL_MultiGestureEvent& ges);
	void eventFingerDown(const SDL_TouchFingerEvent& fin);
	void eventFingerUp(const SDL_TouchFingerEvent& fin);
	void eventMouseLeave();
	void simulateMouseMove();
	void tick();

	vector<SDL_Joystick*> listJoysticks() const;
	void addController(int id);
	void delController(SDL_JoystickID id);
	ivec2 getMouseMove() const;
	const array<Binding, Binding::names.size()>& getBindings() const;
	const Binding& getBinding(Binding::Type type) const;
	template <class F> void callBindings(const SDL_KeyboardEvent& key, F func) const;
	template <class F> void callBindings(AsgJoystick aj, F func) const;
	template <class F> void callBindings(AsgGamepad ag, F func) const;
	void setBinding(Binding::Type bid, sizet kid, SDL_Scancode sc);
	void setBinding(Binding::Type bid, sizet kid, AsgJoystick aj);
	void setBinding(Binding::Type bid, sizet kid, AsgGamepad ag);
	sizet addBinding(Binding::Type bid, SDL_Scancode sc);
	sizet addBinding(Binding::Type bid, AsgJoystick aj);
	sizet addBinding(Binding::Type bid, AsgGamepad ag);
	void delBindingK(Binding::Type bid, sizet kid);
	void delBindingJ(Binding::Type bid, sizet kid);
	void delBindingG(Binding::Type bid, sizet kid);
	void clearBindings();
	void resetBindings();

private:
	template <class K, class T, class M> void checkInput(const mumap<K, Binding::Type>& kmap, T key, M Binding::*call, Binding::Type lim = Binding::holders) const;
	int16 isPressed(const Binding& bind) const;		// returns the axis value or 0/INT16_MAX for binary inputs
	int16 getAxis(uint8 jaxis) const;				// check if any of the joysticks' axis value is greater than 0
	int16 getAxis(SDL_GameControllerAxis gaxis) const;	// check if any of the gamepads' axis value is greater than 0
	template <class T> static bool tryDelController(umap<SDL_JoystickID, T>& ctrls, SDL_JoystickID id);
	template <class T, class F> static void callBindings(T key, const mumap<T, Binding::Type>& kmap, Binding::Type lim, F func);
	template <class T, class M> static void setBinding(Binding::Type bid, mumap<T, Binding::Type>& kmap, T key, M& kref);
	template <class T, class M> sizet addBinding(Binding::Type bid, T key, mumap<T, Binding::Type>& kmap, M bkey);	// returns kid or SIZE_MAX if failed
	template <class T, class M> void delBinding(Binding::Type bid, sizet kid, M bkey, mumap<T, Binding::Type>& kmap);
	template <class T> static typename mumap<T, Binding::Type>::iterator findMapBind(Binding::Type bid, T key, mumap<T, Binding::Type>& kmap);
};

inline const Binding& InputSys::getBinding(Binding::Type type) const {
	return bindings[uint8(type)];
}

inline ivec2 InputSys::getMouseMove() const {
	return SDL_GetTicks() - moveTime < moveTimeout ? mouseMove : ivec2(0);
}

inline const array<Binding, Binding::names.size()>& InputSys::getBindings() const {
	return bindings;
}

template <class F>
void InputSys::callBindings(const SDL_KeyboardEvent& key, F func) const {
	return callBindings(key.keysym.scancode, keymap, key.repeat ? Binding::singles : Binding::holders, func);
}

template <class F>
void InputSys::callBindings(AsgJoystick aj, F func) const {
	return callBindings(aj, joymap, Binding::holders, func);
}

template <class F>
void InputSys::callBindings(AsgGamepad ag, F func) const {
	return callBindings(ag, padmap, Binding::holders, func);
}

template <class T, class F>
void InputSys::callBindings(T key, const mumap<T, Binding::Type>& kmap, Binding::Type lim, F func) {
	for (auto [pos, end] = kmap.equal_range(key); pos != end; ++pos)
		if (pos->second < lim)
			func(pos->second);
}
