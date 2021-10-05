#pragma once

#include "alias.h"

enum class JoystickButton : uint8 {
	y,
	b,
	a,
	x,
	lb,
	rb,
	lt,
	rt,
	back,
	start,
	ls,
	rs,
	guide
};

enum class JoystickAxis : uint8 {
	lx,
	ly,
	rx,
	ry
};

class AsgJoystick {
public:
	enum Type : uint8 {
		button,
		hat,
		axisPos,
		axisNeg
	};

private:
	uint8 jct;	// button/hat/axis id
	uint8 hvl;	// hat value
	Type asg;

public:
	AsgJoystick(JoystickButton but);
	AsgJoystick(uint8 hid, uint8 val);
	AsgJoystick(JoystickAxis axis, bool positive);

	uint8 getJct() const;
	uint8 getHvl() const;
	Type getAsg() const;
	bool operator==(AsgJoystick aj) const;
};

inline uint8 AsgJoystick::getJct() const {
	return jct;
}

inline uint8 AsgJoystick::getHvl() const {
	return hvl;
}

inline AsgJoystick::Type AsgJoystick::getAsg() const {
	return asg;
}

inline bool AsgJoystick::operator==(AsgJoystick aj) const {
	return jct == aj.jct && hvl == aj.hvl && asg == aj.asg;
}

class AsgGamepad {
public:
	enum Type : uint8 {
		button,
		axisPos,
		axisNeg
	};

private:
	uint8 gct;	// button/axis id
	Type asg;

public:
	AsgGamepad(SDL_GameControllerButton but);
	AsgGamepad(SDL_GameControllerAxis axis, bool positive = true);

	uint8 getGct() const;
	SDL_GameControllerButton getButton() const;
	SDL_GameControllerAxis getAxis() const;
	Type getAsg() const;
	bool operator==(AsgGamepad ag) const;
};

inline uint8 AsgGamepad::getGct() const {
	return gct;
}

inline SDL_GameControllerButton AsgGamepad::getButton() const {
	return SDL_GameControllerButton(gct);
}

inline SDL_GameControllerAxis AsgGamepad::getAxis() const {
	return SDL_GameControllerAxis(gct);
}

inline AsgGamepad::Type AsgGamepad::getAsg() const {
	return asg;
}

inline bool AsgGamepad::operator==(AsgGamepad ag) const {
	return gct == ag.gct && asg == ag.asg;
}

namespace std {

template <>
struct hash<AsgJoystick> {
	sizet operator()(AsgJoystick aj) const {
		return hash<uint32>()(uint32(aj.getJct()) | (uint32(aj.getHvl()) << 8) | (uint32(aj.getAsg()) << 16));
	}
};

template <>
struct hash<AsgGamepad> {
	sizet operator()(AsgGamepad ag) const {
		return hash<uint16>()(uint16(ag.getGct()) | (uint16(ag.getAsg()) << 8));
	}
};

}

struct Binding {
	enum class Type : uint8 {
		up,
		down,
		left,
		right,
		textBackspace,
		textDelete,
		textHome,	// single press bindings start here
		textEnd,
		textPaste,
		textCopy,
		textCut,
		textRevert,
		confirm,
		cancel,
		finish,
		hasten,
		assault,
		conspire,
		deceive,
		moveCamera,
		resetCamera,
		engage,
		sdelete,
		destroyHold,
		destroyToggle,
		surrender,
		chat,
		config,
		settings,
		frameCounter,
		selectNext,
		selectPrev,
		select0,
		select1,
		select2,
		select3,
		select4,
		select5,
		select6,
		select7,
		select8,
		select9,
		cameraUp,	// hold down bindings start here
		cameraDown,
		cameraLeft,
		cameraRight,
		scrollUp,
		scrollDown
	};
	static constexpr array<const char*, sizet(Type::scrollDown)+1> names = {
		"up",
		"down",
		"left",
		"right",
		"text_backspace",
		"text_delete",
		"text_home",
		"text_end",
		"text_paste",
		"text_copy",
		"text_cut",
		"text_revert",
		"confirm",
		"cancel",
		"finish",
		"hasten",
		"assault",
		"conspire",
		"deceive",
		"move_camera",
		"reset_camera",
		"engage",
		"delete",
		"destroy_hold",
		"destroy_toggle",
		"surrender",
		"chat",
		"config",
		"settings",
		"frame_counter",
		"select_next",
		"select_prev",
		"select_0",
		"select_1",
		"select_2",
		"select_3",
		"select_4",
		"select_5",
		"select_6",
		"select_7",
		"select_8",
		"select_9",
		"camera_up",
		"camera_down",
		"camera_left",
		"camera_right",
		"scroll_up",
		"scroll_down"
	};
	enum class Accept : uint8 {
		keyboard,
		joystick,
		gamepad,
		any
	};
	static constexpr array<const char*, sizet(Accept::any)> acceptNames = {
		"keyboard",
		"joystick",
		"gamepad"
	};

	static constexpr Type singles = Type::textHome;
	static constexpr Type holders = Type::cameraUp;

	union {
		SBCall bcall;
		SACall acall;
	};
	SBCall ucall;
	vector<SDL_Scancode> keys;
	vector<AsgJoystick> joys;
	vector<AsgGamepad> gpds;

	void init(Type type);
	void reset(Type type);
};

struct Settings {
	enum class Screen : uint8 {
		window,
		borderless,
		fullscreen,
		desktop
	};
	static constexpr array<const char*, sizet(Screen::desktop)+1> screenNames = {
		"window",
		"borderless",
		"fullscreen",
		"desktop"
	};

	enum class VSync : int8 {
		adaptive = -1,
		immediate,
		synchronized
	};
	static constexpr array<const char*, sizet(VSync::synchronized)+2> vsyncNames = {	// add 1 to vsync value to get name
		"adaptive",
		"immediate",
		"synchronized"
	};

	enum class Color : uint8 {
		black,
		blue,
		brass,
		bronze,
		copper,
		tin,
		obsidian,
		perl,
		red,
		white
	};
	static constexpr array<const char*, sizet(Color::white)+1> colorNames = {
		"black",
		"blue",
		"brass",
		"bronze",
		"copper",
		"tin",
		"obsidian",
		"perl",
		"red",
		"white"
	};

	enum class Family : uint8 {
		any,
		v4,
		v6
	};
	static constexpr array<const char*, sizet(Family::v6)+1> familyNames = {
		"any",
		"IPv4",
		"IPv6"
	};

	static constexpr char defaultAddress[] = "94.16.112.206";
	static constexpr uint8 playerNameLimit = 31;
	static constexpr char defaultVersionLocation[] = "https://github.com/karwler/Thrones/releases";
	static constexpr char defaultVersionRegex[] = R"r(<a\s+href="/karwler/Thrones/releases/tag/v(\d+\.\d+\.\d+(\.\d+)?)">)r";
	static constexpr char defaultFont[] = "Romanesque";
#ifdef __ANDROID__
	static constexpr Screen defaultScreen = Screen::desktop;
#else
	static constexpr Screen defaultScreen = Screen::window;
#endif
	static constexpr VSync defaultVSync = VSync::synchronized;
	static constexpr uint8 shadowBitMax = 13;
	static constexpr float gammaMax = 2.f;
	static constexpr uint16 chatLinesMax = 8191;
	static constexpr svec2 deadzoneLimit = { 256, INT16_MAX - 1 };
	static constexpr Family defaultFamily = Family::v4;
	static constexpr Color defaultAlly = Color::brass;
	static constexpr Color defaultEnemy = Color::tin;

	static constexpr char argExternal = 'e';
#ifndef NDEBUG
	static constexpr char argConsole = 'c';
	static constexpr char argSetup = 's';
#endif

	string address;
	string port;
	string playerName;
	string lastConfig;
	string versionLookupUrl;
	string versionLookupRegex;
	string font;
	SDL_DisplayMode mode;
	ivec2 size;
	float gamma;
	uint16 shadowRes;
	uint16 chatLines;
	uint16 deadzone;
	uint8 display;
	Screen screen;
	VSync vsync;
	uint8 texScale;
	uint8 msamples;
	bool softShadows;
	bool ssao;
	bool bloom;
	uint8 avolume;
	Color colorAlly, colorEnemy;
	bool scaleTiles, scalePieces;
	bool autoVictoryPoints;
	bool tooltips;
	Family resolveFamily;
	bool invertWheel;

	Settings();

	int getFamily() const;
};
