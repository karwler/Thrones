#include "prog/progs.h"
#include "server/server.h"

// ASG JOYSTICK

AsgJoystick::AsgJoystick(JoystickButton but) :
	jct(uint8(but)),
	hvl(0),
	asg(button)
{}

AsgJoystick::AsgJoystick(uint8 hid, uint8 val) :
	jct(hid),
	hvl(val),
	asg(hat)
{}

AsgJoystick::AsgJoystick(JoystickAxis axis, bool positive) :
	jct(uint8(axis)),
	hvl(0),
	asg(positive ? axisPos : axisNeg)
{}

// ASG GAMEPAD

AsgGamepad::AsgGamepad(SDL_GameControllerButton but) :
	gct(but),
	asg(button)
{}

AsgGamepad::AsgGamepad(SDL_GameControllerAxis axis, bool positive) :
	gct(axis),
	asg(positive ? axisPos : axisNeg)
{}

// BINDING

void Binding::init(Type type) {
	switch (type) {
	case Type::up:
		bcall = &ProgState::eventUp;
		ucall = nullptr;
		break;
	case Type::down:
		bcall = &ProgState::eventDown;
		ucall = nullptr;
		break;
	case Type::left:
		bcall = &ProgState::eventLeft;
		ucall = nullptr;
		break;
	case Type::right:
		bcall = &ProgState::eventRight;
		ucall = nullptr;
		break;
	case Type::textBackspace: case Type::textDelete: case Type::textHome: case Type::textEnd: case Type::textPaste: case Type::textCopy: case Type::textCut: case Type::textRevert:
		bcall = nullptr;
		ucall = nullptr;
		break;
	case Type::confirm:
		bcall = &ProgState::eventConfirm;
		ucall = nullptr;
		break;
	case Type::cancel:
		bcall = &ProgState::eventCancel;
		ucall = nullptr;
		break;
	case Type::finish:
		bcall = &ProgState::eventFinish;
		ucall = nullptr;
		break;
	case Type::hasten:
		bcall = &ProgState::eventHasten;
		ucall = nullptr;
		break;
	case Type::assault:
		bcall = &ProgState::eventAssault;
		ucall = nullptr;
		break;
	case Type::conspire:
		bcall = &ProgState::eventConspire;
		ucall = nullptr;
		break;
	case Type::deceive:
		bcall = &ProgState::eventDeceive;
		ucall = nullptr;
		break;
	case Type::moveCamera:
		bcall = &ProgState::eventStartCamera;
		ucall = &ProgState::eventStopCamera;
		break;
	case Type::resetCamera:
		bcall = &ProgState::eventCameraReset;
		ucall = nullptr;
		break;
	case Type::engage:
		bcall = &ProgState::eventEngage;
		ucall = nullptr;
		break;
	case Type::sdelete:
		bcall = &ProgState::eventDelete;
		ucall = nullptr;
		break;
	case Type::destroyHold:
		bcall = &ProgState::eventDestroyOn;
		ucall = &ProgState::eventDestroyOff;
		break;
	case Type::destroyToggle:
		bcall = &ProgState::eventDestroyToggle;
		ucall = nullptr;
		break;
	case Type::surrender:
		bcall = &ProgState::eventSurrender;
		ucall = nullptr;
		break;
	case Type::chat:
		bcall = &ProgState::eventChat;
		ucall = nullptr;
		break;
	case Type::config:
		bcall = &ProgState::eventOpenConfig;
		ucall = nullptr;
		break;
	case Type::settings:
		bcall = &ProgState::eventOpenSettings;
		ucall = nullptr;
		break;
	case Type::frameCounter:
		bcall = &ProgState::eventFrameCounter;
		ucall = nullptr;
		break;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	case Type::screenshot:
		bcall = &ProgState::eventScreenshot;
		ucall = nullptr;
		break;
	case Type::peekTextures:
		bcall = &ProgState::eventPeekUiTextures;
		ucall = nullptr;
		break;
#endif
	case Type::selectNext:
		bcall = &ProgState::eventSelectNext;
		ucall = nullptr;
		break;
	case Type::selectPrev:
		bcall = &ProgState::eventSelectPrev;
		ucall = nullptr;
		break;
	case Type::select0:
		bcall = &ProgState::eventSelect0;
		ucall = nullptr;
		break;
	case Type::select1:
		bcall = &ProgState::eventSelect1;
		ucall = nullptr;
		break;
	case Type::select2:
		bcall = &ProgState::eventSelect2;
		ucall = nullptr;
		break;
	case Type::select3:
		bcall = &ProgState::eventSelect3;
		ucall = nullptr;
		break;
	case Type::select4:
		bcall = &ProgState::eventSelect4;
		ucall = nullptr;
		break;
	case Type::select5:
		bcall = &ProgState::eventSelect5;
		ucall = nullptr;
		break;
	case Type::select6:
		bcall = &ProgState::eventSelect6;
		ucall = nullptr;
		break;
	case Type::select7:
		bcall = &ProgState::eventSelect7;
		ucall = nullptr;
		break;
	case Type::select8:
		bcall = &ProgState::eventSelect8;
		ucall = nullptr;
		break;
	case Type::select9:
		bcall = &ProgState::eventSelect9;
		ucall = nullptr;
		break;
	case Type::cameraUp:
		acall = &ProgState::eventCameraUp;
		ucall = nullptr;
		break;
	case Type::cameraDown:
		acall = &ProgState::eventCameraDown;
		ucall = nullptr;
		break;
	case Type::cameraLeft:
		acall = &ProgState::eventCameraLeft;
		ucall = nullptr;
		break;
	case Type::cameraRight:
		acall = &ProgState::eventCameraRight;
		ucall = nullptr;
		break;
	case Type::scrollUp:
		acall = &ProgState::eventScrollUp;
		ucall = nullptr;
		break;
	case Type::scrollDown:
		acall = &ProgState::eventScrollDown;
		ucall = nullptr;
	}
}

void Binding::reset(Binding::Type type) {
	switch (type) {
	case Type::up:
		keys = { SDL_SCANCODE_UP };
		joys = { AsgJoystick(0, SDL_HAT_UP) };
		gpds = { SDL_CONTROLLER_BUTTON_DPAD_UP };
		break;
	case Type::down:
		keys = { SDL_SCANCODE_DOWN };
		joys = { AsgJoystick(0, SDL_HAT_DOWN) };
		gpds = { SDL_CONTROLLER_BUTTON_DPAD_DOWN };
		break;
	case Type::left:
		keys = { SDL_SCANCODE_LEFT };
		joys = { AsgJoystick(0, SDL_HAT_LEFT) };
		gpds = { SDL_CONTROLLER_BUTTON_DPAD_LEFT };
		break;
	case Type::right:
		keys = { SDL_SCANCODE_RIGHT };
		joys = { AsgJoystick(0, SDL_HAT_RIGHT) };
		gpds = { SDL_CONTROLLER_BUTTON_DPAD_RIGHT };
		break;
	case Type::textBackspace:
		keys = { SDL_SCANCODE_BACKSPACE };
		joys = { JoystickButton::x };
		gpds = { SDL_CONTROLLER_BUTTON_X };
		break;
	case Type::textDelete:
		keys = { SDL_SCANCODE_DELETE };
		joys = { JoystickButton::y };
		gpds = { SDL_CONTROLLER_BUTTON_Y };
		break;
	case Type::textHome:
		keys = { SDL_SCANCODE_HOME };
		joys = { JoystickButton::start };
		gpds = { SDL_CONTROLLER_BUTTON_START };
		break;
	case Type::textEnd:
		keys = { SDL_SCANCODE_END };
		joys = { JoystickButton::back };
		gpds = { SDL_CONTROLLER_BUTTON_BACK };
		break;
	case Type::textPaste:
		keys = { SDL_SCANCODE_V };
		joys = { JoystickButton::rt };
		gpds = { SDL_CONTROLLER_AXIS_TRIGGERRIGHT };
		break;
	case Type::textCopy:
		keys = { SDL_SCANCODE_C };
		joys = { JoystickButton::rb };
		gpds = { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER };
		break;
	case Type::textCut:
		keys = { SDL_SCANCODE_X };
		joys = { JoystickButton::lb };
		gpds = { SDL_CONTROLLER_BUTTON_LEFTSHOULDER };
		break;
	case Type::textRevert:
		keys = { SDL_SCANCODE_Z };
		joys = { JoystickButton::lt };
		gpds = { SDL_CONTROLLER_AXIS_TRIGGERLEFT };
		break;
	case Type::confirm:
		keys = { SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER };
		joys = { JoystickButton::a };
		gpds = { SDL_CONTROLLER_BUTTON_A };
		break;
	case Type::cancel:
		keys = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_AC_BACK };
		joys = { JoystickButton::b };
		gpds = { SDL_CONTROLLER_BUTTON_B };
		break;
	case Type::finish:
		keys = { SDL_SCANCODE_SPACE };
		joys = { JoystickButton::y };
		gpds = { SDL_CONTROLLER_BUTTON_Y };
		break;
	case Type::hasten:
		keys = { SDL_SCANCODE_1, SDL_SCANCODE_KP_1 };
		joys = { JoystickButton::lb };
		gpds = { SDL_CONTROLLER_BUTTON_LEFTSHOULDER };
		break;
	case Type::assault:
		keys = { SDL_SCANCODE_2, SDL_SCANCODE_KP_2 };
		joys = { JoystickButton::rb };
		gpds = { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER };
		break;
	case Type::conspire:
		keys = { SDL_SCANCODE_3, SDL_SCANCODE_KP_3 };
		joys = { JoystickButton::lt };
		gpds = { SDL_CONTROLLER_AXIS_TRIGGERLEFT };
		break;
	case Type::deceive:
		keys = { SDL_SCANCODE_4, SDL_SCANCODE_KP_4 };
		joys = { JoystickButton::rt };
		gpds = { SDL_CONTROLLER_AXIS_TRIGGERRIGHT };
		break;
	case Type::moveCamera:
		keys = { SDL_SCANCODE_LALT };
		joys.clear();
		gpds.clear();
		break;
	case Type::resetCamera:
		keys = { SDL_SCANCODE_C };
		joys = { JoystickButton::ls };
		gpds = { SDL_CONTROLLER_BUTTON_LEFTSTICK };
		break;
	case Type::engage: case Type::sdelete:
		keys = { SDL_SCANCODE_X };
		joys = { JoystickButton::x };
		gpds = { SDL_CONTROLLER_BUTTON_X };
		break;
	case Type::destroyHold:
		keys = { SDL_SCANCODE_D };
		joys.clear();
		gpds.clear();
		break;
	case Type::destroyToggle:
		keys.clear();
		joys = { JoystickButton::start };
		gpds = { SDL_CONTROLLER_BUTTON_START };
		break;
	case Type::surrender:
		keys = { SDL_SCANCODE_S };
		joys = { JoystickButton::guide };
		gpds = { SDL_CONTROLLER_BUTTON_GUIDE };
		break;
	case Type::chat:
		keys = { SDL_SCANCODE_TAB };
		joys = { JoystickButton::back };
		gpds = { SDL_CONTROLLER_BUTTON_BACK };
		break;
	case Type::config:
		keys = { SDL_SCANCODE_I };
		joys.clear();
		gpds.clear();
		break;
	case Type::settings:
		keys = { SDL_SCANCODE_O };
		joys.clear();
		gpds.clear();
		break;
	case Type::frameCounter:
		keys = { SDL_SCANCODE_P };
		joys = { JoystickButton::rs };
		gpds = { SDL_CONTROLLER_BUTTON_RIGHTSTICK };
		break;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	case Type::screenshot:
		keys = { SDL_SCANCODE_F12 };
		joys.clear();
		gpds.clear();
		break;
	case Type::peekTextures:
		keys = { SDL_SCANCODE_F10 };
		joys.clear();
		gpds.clear();
		break;
#endif
	case Type::selectNext:
		keys.clear();
		joys = { JoystickButton::lb, JoystickButton::lt };
		gpds = { SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_AXIS_TRIGGERLEFT };
		break;
	case Type::selectPrev:
		keys.clear();
		joys = { JoystickButton::rb, JoystickButton::rt };
		gpds = { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_AXIS_TRIGGERRIGHT };
		break;
	case Type::select0:
		keys = { SDL_SCANCODE_1, SDL_SCANCODE_KP_1 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select1:
		keys = { SDL_SCANCODE_2, SDL_SCANCODE_KP_2 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select2:
		keys = { SDL_SCANCODE_3, SDL_SCANCODE_KP_3 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select3:
		keys = { SDL_SCANCODE_4, SDL_SCANCODE_KP_4 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select4:
		keys = { SDL_SCANCODE_5, SDL_SCANCODE_KP_5 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select5:
		keys = { SDL_SCANCODE_6, SDL_SCANCODE_KP_6 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select6:
		keys = { SDL_SCANCODE_7, SDL_SCANCODE_KP_7 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select7:
		keys = { SDL_SCANCODE_8, SDL_SCANCODE_KP_8 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select8:
		keys = { SDL_SCANCODE_9, SDL_SCANCODE_KP_9 };
		joys.clear();
		gpds.clear();
		break;
	case Type::select9:
		keys = { SDL_SCANCODE_0, SDL_SCANCODE_KP_0 };
		joys.clear();
		gpds.clear();
		break;
	case Type::cameraUp:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::ly, false) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_LEFTY, false) };
		break;
	case Type::cameraDown:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::ly, true) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_LEFTY, true) };
		break;
	case Type::cameraLeft:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::lx, true) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_LEFTX, true) };
		break;
	case Type::cameraRight:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::lx, false) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_LEFTX, false) };
		break;
	case Type::scrollUp:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::ry, false) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_RIGHTY, false) };
		break;
	case Type::scrollDown:
		keys.clear();
		joys = { AsgJoystick(JoystickAxis::ry, true) };
		gpds = { AsgGamepad(SDL_CONTROLLER_AXIS_RIGHTY, true) };
	}
}

// SETTINGS

Settings::Settings() :
	address(defaultAddress),
	port(Com::defaultPort),
	lastConfig(Config::defaultName),
	versionLookupUrl(defaultVersionLocation),
	versionLookupRegex(defaultVersionRegex),
	font(defaultFont),
	fov(35.0),
	mode{ SDL_PIXELFORMAT_RGB888, 1920, 1080, 60, nullptr },
	size(1280, 720),
	gamma(1.f),
#ifdef OPENGLES
	shadowRes(0),
#else
	shadowRes(1024),
#endif
	chatLines(511),
	deadzone(12000),
	display(0),
	screen(defaultScreen),
	vsync(defaultVSync),
	texScale(100),
#ifdef OPENGLES
	antiAliasing(AntiAliasing::none),
	softShadows(false),
	ssao(false),
	bloom(false),
	ssr(false),
#else
	antiAliasing(AntiAliasing::msaa4),
	softShadows(true),
	ssao(true),
	bloom(true),
	ssr(true),
#endif
	hinting(defaultHinting),
	avolume(0),
	colorAlly(defaultAlly),
	colorEnemy(defaultEnemy),
	scaleTiles(true),
	scalePieces(false),
	autoVictoryPoints(true),
	tooltips(true),
	resolveFamily(defaultFamily),
	invertWheel(false)
{}

bool Settings::trySetDisplay(int dip) {
	if (dip >= 0 && display != dip) {
		display = dip;
		return true;
	}
	return false;
}

int Settings::getFamily() const {
	switch (resolveFamily) {
	case Family::v4:
		return AF_INET;
	case Family::v6:
		return AF_INET6;
	}
	return AF_UNSPEC;
}

vector<ivec2> Settings::windowSizes() const {
	SDL_Rect max;
	if (SDL_GetDisplayBounds(display, &max))
		max.w = max.h = INT_MAX;

	initlist<ivec2> resolutions = {
		ivec2(640, 360),
		ivec2(768, 432),
		ivec2(800, 450),
		ivec2(848, 480),
		ivec2(854, 480),
		ivec2(960, 540),
		ivec2(1024, 576),
		ivec2(1280, 720),
		ivec2(1366, 768),
		ivec2(1280, 800),
		ivec2(1440, 900),
		ivec2(1600, 900),
		ivec2(1680, 1050),
		ivec2(1920, 1080),
		ivec2(2048, 1152),
		ivec2(1920, 1200),
		ivec2(2560, 1440),
		ivec2(2560, 1600),
		ivec2(2880, 1620),
		ivec2(3200, 1800),
		ivec2(3840, 2160),
		ivec2(4096, 2304),
		ivec2(3840, 2400),
		ivec2(5120, 2880),
		ivec2(7680, 4320),
		ivec2(15360, 8640)
	};
	vector<ivec2> sizes;
	std::copy_if(resolutions.begin(), resolutions.end(), std::back_inserter(sizes), [&max](ivec2 it) -> bool { return it.x <= max.w && it.y <= max.h; });
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
		for (int im = 0; im < SDL_GetNumDisplayModes(i); ++im)
			if (SDL_DisplayMode dm; !SDL_GetDisplayMode(i, im, &dm) && dm.w <= max.w && dm.h <= max.h && float(dm.w) / float(dm.h) >= minimumRatio)
				sizes.emplace_back(dm.w, dm.h);
	return uniqueSort(sizes);
}

vector<SDL_DisplayMode> Settings::displayModes() const {
	vector<SDL_DisplayMode> mods;
	for (int im = 0; im < SDL_GetNumDisplayModes(display); ++im)
		if (SDL_DisplayMode dm; !SDL_GetDisplayMode(display, im, &dm) && float(dm.w) / float(dm.h) >= minimumRatio)
			mods.push_back(dm);
	return uniqueSort(mods);
}
