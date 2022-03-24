#include "windowSys.h"
#include "audioSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "shaders.h"
#include "prog/program.h"
#include "prog/progs.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

// LOADER

void Loader::init(ShaderStartup* stlog, WindowSys* win) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(Shader::vpos);
	glVertexAttribPointer(Shader::vpos, stride, GL_FLOAT, GL_FALSE, stride * sizeof(*vertices), reinterpret_cast<void*>(uptrt(0)));

	shader = stlog;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	if (win->getTitleBarHeight()) {
		glActiveTexture(Shader::stlogTexa);
		blank.init(Widget::colorDark);
		title.init(win->getFonts()->render((string("Thrones v") + Com::commonVersion + " loading...").c_str(), win->getTitleBarHeight(), win->getScreenView().x), win->getScreenView());
		glActiveTexture(Shader::texa);
	}
#endif
}

void Loader::free() {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	title.free();
	blank.free();
#endif
	delete shader;

	glBindVertexArray(vao);
	glDisableVertexAttribArray(Shader::vpos);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void Loader::addLine(const string& str, WindowSys* win) {
	ivec2 wres = win->getScreenView();
	logStr += str + '\n';
	if (uint lines = std::count(logStr.begin(), logStr.end(), '\n'), maxl = (wres.y - logMargin.y * 2) / logSize; lines > maxl) {
		sizet end = 0;
		for (uint i = lines - maxl; i; --i)
			end = logStr.find_first_of('\n', end) + 1;
		logStr.erase(0, end);
	}

	glActiveTexture(Shader::stlogTexa);
	if (Texture tex; tex.init(win->getFonts()->render(logStr.c_str(), logSize, wres.x), wres)) {
		glUseProgram(*shader);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(vao);
		draw(Rect(logMargin, tex.getRes()), tex);

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		if (title) {
			vec2 hres = vec2(wres) / 2.f;
			glUniform2f(shader->pview, hres.x, float(win->getTitleBarHeight()) / 2.f);
			glViewport(0, wres.y, wres.x, win->getTitleBarHeight());
			draw(Rect(0, 0, wres.x, win->getTitleBarHeight()), blank);
			draw(Rect((wres.x - title.getRes().x) / 2, 0, title.getRes()), title);
			glUniform2fv(shader->pview, 1, glm::value_ptr(hres));
			glViewport(0, 0, wres.x, wres.y);
		}
#endif
		SDL_GL_SwapWindow(win->getWindow());
		tex.free();
	}
	glActiveTexture(Shader::texa);
}

void Loader::draw(const Rect& rect, GLuint tex) {
	glUniform4f(shader->rect, float(rect.x), float(rect.y), float(rect.w), float(rect.h));
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, corners);
}

// FONT SET

string FontSet::init(string name, Settings::Hinting hint) {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
#endif
	TTF_Font* tmp = load(name);	// just in case it's an absolute path
	if (!tmp) {
		vector<string> available = FileSys::listFonts();
		if (tmp = findFile(name, available); !tmp) {
			name = Settings::defaultFont;
			if (tmp = findFile(name, available); !tmp) {
				for (const string& it : available)
					if (tmp = load(it); tmp) {
						name = firstUpper(delExt(it));
						break;
					}
				if (!tmp)
					throw std::runtime_error(string("failed to find a font:") + linend + TTF_GetError());
			}
		}
	}
	TTF_SetFontHinting(tmp, int(hint));
	int size;	// get approximate height scale factor
	heightScale = !TTF_SizeUTF8(tmp, fontTestString, nullptr, &size) ? float(FileSys::fontTestHeight) / float(size) : fallbackScale;
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	font = tmp;
#else
	hinting = hint;
	TTF_CloseFont(tmp);
#endif
	return name;
}

TTF_Font* FontSet::findFile(const string& name, const vector<string>& available) {
	vector<string>::const_iterator it = std::find_if(available.begin(), available.end(), [name](const string& ent) -> bool { return !SDL_strcasecmp(name.c_str(), delExt(ent).c_str()); });
	return it != available.end() ? load(*it) : nullptr;
}

TTF_Font* FontSet::load(const string& name) {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	return TTF_OpenFont((FileSys::fontPath() + name).c_str(), FileSys::fontTestHeight);
#else
	fontData = loadFile<vector<uint8>>(FileSys::fontPath() + name);
	return TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, FileSys::fontTestHeight);
#endif
}

#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
void FontSet::clear() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
	fonts.clear();
}
#endif

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	if (TTF_SetFontSize(font, height)) {
		logError(TTF_GetError());
		return nullptr;
	}
#else
	if (umap<int, TTF_Font*>::iterator it = fonts.find(height); it != fonts.end())
		return it->second;

	TTF_Font* font = TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, height);
	if (font) {
		TTF_SetFontHinting(font, int(hinting));
		fonts.emplace(height, font);
	} else
		logError(TTF_GetError());
#endif
	return font;
}

int FontSet::length(const char* text, int height) {
	int len = 0;
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	return len;
}

int FontSet::length(char* text, int height, sizet length) {
	int len = 0;
	char tmp = text[length];
	text[length] = '\0';
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	text[length] = tmp;
	return len;
}

bool FontSet::hasGlyph(uint16 ch) {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	return TTF_GlyphIsProvided(font, ch);
#else
	if (!fonts.empty())
		return TTF_GlyphIsProvided(fonts.begin()->second, ch);

	if (TTF_Font* tmp = TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, FileSys::fontTestHeight)) {
		bool yes = TTF_GlyphIsProvided(tmp, ch);
		TTF_CloseFont(tmp);
		return yes;
	}
	return false;
#endif
}

void FontSet::setHinting(Settings::Hinting hint) {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_SetFontHinting(font, int(hint));
#else
	hinting = hint;
	for (auto [size, font] : fonts)
		TTF_SetFontHinting(font, int(hinting));
#endif
}

// WINDOW SYS

int WindowSys::start(const Arguments& args) {
	audio = nullptr;
	inputSys = nullptr;
	program = nullptr;
	scene = nullptr;
	sets = nullptr;
	window = nullptr;
	context = nullptr;
	geom = nullptr;
	depth = nullptr;
	ssao = nullptr;
	blur = nullptr;
	light = nullptr;
	gauss = nullptr;
	sfinal = nullptr;
	skybox = nullptr;
	gui = nullptr;
	fonts = nullptr;
	run = true;

	int rc = EXIT_SUCCESS;
	try {
		init(args);
#ifdef __EMSCRIPTEN__
		emscripten_set_main_loop_arg([](void* data) {
			WindowSys* w = static_cast<WindowSys*>(data);
			(w->*(w->loopFunc))();
		}, this, 0, true);
#else
		for (; run; exec());
#endif
	} catch (const std::runtime_error& e) {
		logError(e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), window);
		rc = EXIT_FAILURE;
#ifdef NDEBUG
	} catch (...) {
		logError("unknown error");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "unknown error", window);
		rc = EXIT_FAILURE;
#endif
	}
	delete program;
	delete scene;
	delete audio;
	destroyWindow();
	delete fonts;
	delete sets;
	delete inputSys;

#ifdef _WIN32
	WSACleanup();
#endif
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	return rc;
}

void WindowSys::exec() {
	uint32 newTime = SDL_GetTicks();
	dSec = float(newTime - oldTime) / ticksPerSec;
	oldTime = newTime;

	scene->draw();
	SDL_GL_SwapWindow(window);

	inputSys->tick();
	scene->tick(dSec);
	program->tick(dSec);

	SDL_Event event;
	uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
	do {
		if (!SDL_PollEvent(&event))
			break;
		handleEvent(event);
	} while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout));
}

void WindowSys::init(const Arguments& args) {
#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_SetHint(SDL_HINT_APP_NAME, "Thrones");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	if (args.hasFlag(Settings::argCompositor))
		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(string("failed to initialize systems:") + linend + SDL_GetError());
	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
		throw std::runtime_error(string("failed to initialize textures:") + linend + IMG_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("failed to initialize fonts:") + linend + TTF_GetError());
#ifdef _WIN32
	if (WSADATA wsad; WSAStartup(MAKEWORD(2, 2), &wsad))
		throw std::runtime_error(Com::msgWinsockFail);
#endif
	SDL_EventState(SDL_APP_LOWMEMORY, SDL_DISABLE);
	SDL_EventState(SDL_APP_WILLENTERBACKGROUND, SDL_DISABLE);
	SDL_EventState(SDL_APP_DIDENTERBACKGROUND, SDL_DISABLE);
	SDL_EventState(SDL_APP_WILLENTERFOREGROUND, SDL_DISABLE);
	SDL_EventState(SDL_APP_DIDENTERFOREGROUND, SDL_DISABLE);
	SDL_EventState(SDL_LOCALECHANGED, SDL_DISABLE);
	SDL_EventState(SDL_DISPLAYEVENT, SDL_DISABLE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
	SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADDOWN, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERSENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARRECORD, SDL_DISABLE);
	SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DROPFILE, SDL_DISABLE);
	SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
	SDL_StopTextInput();

	FileSys::init(args);
#ifdef __EMSCRIPTEN__
	loader = std::make_unique<Loader>();
	loopFunc = &WindowSys::load;
#else
	for (Loader loader; loader.state < Loader::State::done; load(&loader));
#endif
}

#ifdef __EMSCRIPTEN__
void WindowSys::load() {
#else
void WindowSys::load(Loader* loader) {
#endif
	switch (loader->state) {
	case Loader::State::start:
#ifdef __EMSCRIPTEN__
		if (FileSys::canRead())
			return;
#endif
		inputSys = new InputSys;
		sets = new Settings(FileSys::loadSettings(inputSys));
		fonts = new FontSet;
		sets->font = fonts->init(sets->font, sets->hinting);
		loader->init(createWindow(), this);
		loader->addLine("loading audio", this);
		break;
	case Loader::State::audio:
		try {
			audio = new AudioSys(sets->avolume);
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
		loader->addLine("loading objects", this);
		break;
	case Loader::State::objects:
		scene = new Scene;
		scene->loadObjects();
		loader->addLine("loading textures", this);
		break;
	case Loader::State::textures: {
		ivec2 maxWin = windowSizes().back();
		scene->loadTextures(ceilPower2(std::max(maxWin.x, maxWin.y)));
		loader->addLine("starting", this);
		loader->free();
		break; }
	case Loader::State::program:
#ifdef __EMSCRIPTEN__
		loader.reset();
		loopFunc = &WindowSys::exec;
#endif
		program = new Program;
		program->start();
		setTitleBarHitTest();
		oldTime = SDL_GetTicks();
	}
	++loader->state;
}

ShaderStartup* WindowSys::createWindow() {
	uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
	switch (sets->screen) {
	case Settings::Screen::borderlessWindow: case Settings::Screen::borderless:
		flags |= SDL_WINDOW_BORDERLESS;
		break;
	case Settings::Screen::fullscreen:
		flags |= SDL_WINDOW_FULLSCREEN;
		break;
	case Settings::Screen::desktop:
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	int winPos = SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display);

	// create new window
#ifdef OPENGLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
#if OPENGLES == 32
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef NDEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
#endif
	if (window = SDL_CreateWindow(title, winPos, winPos, sets->size.x, sets->size.y, flags); !window)
		throw std::runtime_error(string("failed to create window:") + linend + SDL_GetError());

	vector<ivec2> wsizes = windowSizes();
	setTitleBarHeight();
#ifndef __ANDROID__
	checkCurDisplay();
	checkResolution(sets->size, wsizes);
	checkResolution(sets->mode, displayModes());
	if (sets->screen <= Settings::Screen::borderless) {
		SDL_SetWindowSize(window, sets->size.x, sets->size.y + titleBarHeight);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		if (sets->screen == Settings::Screen::borderlessWindow)
			SDL_SetWindowHitTest(window, eventWindowHitStartup, this);
	} else if (sets->screen == Settings::Screen::fullscreen)
		SDL_SetWindowDisplayMode(window, &sets->mode);
#endif
	SDL_SetWindowBrightness(window, sets->gamma);

	// load icons
#ifndef __ANDROID__
#ifndef __EMSCRIPTEN__
	if (SDL_Surface* icon = IMG_Load((FileSys::dataPath() + fileIcon).c_str())) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
#endif
	if (SDL_Surface* icon = IMG_Load((FileSys::dataPath() + fileCursor).c_str())) {
		if (SDL_Cursor* cursor = SDL_CreateColorCursor(icon, 0, 0)) {
			cursorHeight = icon->h;
			SDL_SetCursor(cursor);
		}
		SDL_FreeSurface(icon);
	} else
		cursorHeight = estimateSystemCursorSize();
#endif

	// create context and set up rendering
	if (context = SDL_GL_CreateContext(window); !context)
		throw std::runtime_error(string("failed to create context:") + linend + SDL_GetError());
	setSwapInterval();
#if !defined(OPENGLES) && !defined(__APPLE__)
	glewExperimental = GL_TRUE;
	if (GLenum err = glewInit(); err != GLEW_OK)
		throw std::runtime_error(string("failed to initialize OpenGL extensions:") + linend + reinterpret_cast<const char*>(glewGetErrorString(err)));
#endif

	int gvals[2] = { 0, 0 };
#if !defined(NDEBUG) && !defined(__APPLE__) && (!defined(OPENGLES) || OPENGLES == 32)
#if OPENGLES == 32
	if (glGetIntegerv(GL_CONTEXT_FLAGS, gvals); gvals[0] & GL_CONTEXT_FLAG_DEBUG_BIT) {
#else
	if (glGetIntegerv(GL_CONTEXT_FLAGS, gvals); (gvals[0] & GL_CONTEXT_FLAG_DEBUG_BIT) && glDebugMessageCallback && glDebugMessageControl) {
#endif
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debugMessage, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif
	if (glGetIntegerv(GL_MAX_TEXTURE_SIZE, gvals); gvals[0] < std::max(wsizes.back().x, wsizes.back().y))
		throw std::runtime_error("texture size is limited to " + toStr(gvals[0]));
	if (glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, gvals); gvals[0] < largestObjTexture)
		throw std::runtime_error("texture size is limited to " + toStr(gvals[0]));
	if (glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, gvals); gvals[0] < 1 << Settings::shadowBitMax)
		throw std::runtime_error("cubemap size is limited to " + toStr(gvals[0]));
	if (glGetIntegerv(GL_MAX_VIEWPORT_DIMS, gvals); gvals[0] < std::max(wsizes.back().x, 1 << Settings::shadowBitMax) || gvals[1] < std::max(wsizes.back().y, 1 << Settings::shadowBitMax))
		throw std::runtime_error("viewport size is limited to " + toStr(gvals[0]) + 'x' + toStr(gvals[1]));
	if (glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, gvals); gvals[0] < std::max(wsizes.back().x, wsizes.back().y))
		throw std::runtime_error("renderbuffer size is limited to " + toStr(gvals[0]));

	glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, gvals);
	glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, gvals + 1);
	maxMsamples = std::min(gvals[0], gvals[1]);
	setMultisampling();

	updateView();
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	umap<string, string> sources = FileSys::loadShaders();
	geom = new ShaderGeom(sources.at(ShaderGeom::fileVert), sources.at(ShaderGeom::fileFrag));
	depth = new ShaderDepth(sources.at(ShaderDepth::fileVert), sources.at(ShaderDepth::fileFrag));
	ssao = new ShaderSsao(sources.at(ShaderSsao::fileVert), sources.at(ShaderSsao::fileFrag));
	blur = new ShaderBlur(sources.at(ShaderBlur::fileVert), sources.at(ShaderBlur::fileFrag));
	light = new ShaderLight(sources.at(ShaderLight::fileVert), sources.at(ShaderLight::fileFrag), sets);
	brights = new ShaderBrights(sources.at(ShaderBrights::fileVert), sources.at(ShaderBrights::fileFrag));
	gauss = new ShaderGauss(sources.at(ShaderGauss::fileVert), sources.at(ShaderGauss::fileFrag));
	sfinal = new ShaderFinal(sources.at(ShaderFinal::fileVert), sources.at(ShaderFinal::fileFrag), sets);
	skybox = new ShaderSkybox(sources.at(ShaderSkybox::fileVert), sources.at(ShaderSkybox::fileFrag));
	gui = new ShaderGui(sources.at(ShaderGui::fileVert), sources.at(ShaderGui::fileFrag));

	ShaderStartup* stlog = new ShaderStartup(sources.at(ShaderStartup::fileVert), sources.at(ShaderStartup::fileFrag));
	glUniform2f(stlog->pview, float(screenView.x) / 2.f, float(screenView.y) / 2.f);
	return stlog;
}

void WindowSys::destroyWindow() {
	delete gui;
	delete skybox;
	delete sfinal;
	delete brights;
	delete gauss;
	delete light;
	delete blur;
	delete ssao;
	delete depth;
	delete geom;
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_FreeCursor(SDL_GetCursor());
}

void WindowSys::handleEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_QUIT: case SDL_APP_TERMINATING:
		close();
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
	case SDL_KEYDOWN:
		inputSys->eventKeyDown(event.key);
		break;
	case SDL_KEYUP:
		inputSys->eventKeyUp(event.key);
		break;
	case SDL_TEXTEDITING:
		scene->onCompose(event.edit.text);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
	case SDL_MOUSEMOTION:
		inputSys->eventMouseMotion(event.motion, titleBarHeight);
		break;
	case SDL_MOUSEBUTTONDOWN:
		inputSys->eventMouseButtonDown(event.button, titleBarHeight);
		break;
	case SDL_MOUSEBUTTONUP:
		inputSys->eventMouseButtonUp(event.button, titleBarHeight);
		break;
	case SDL_MOUSEWHEEL:
		inputSys->eventMouseWheel(event.wheel);
		break;
	case SDL_JOYAXISMOTION:
		inputSys->eventJoystickAxis(event.jaxis);
		break;
	case SDL_JOYHATMOTION:
		inputSys->eventJoystickHat(event.jhat);
		break;
	case SDL_JOYBUTTONDOWN:
		inputSys->eventJoystickButtonDown(event.jbutton);
		break;
	case SDL_JOYBUTTONUP:
		inputSys->eventJoystickButtonUp(event.jbutton);
		break;
	case SDL_JOYDEVICEADDED:
		inputSys->addController(event.jdevice.which);
		break;
	case SDL_JOYDEVICEREMOVED:
		inputSys->delController(event.jdevice.which);
		break;
	case SDL_CONTROLLERAXISMOTION:
		inputSys->eventGamepadAxis(event.caxis);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		inputSys->eventGamepadButtonDown(event.cbutton);
		break;
	case SDL_CONTROLLERBUTTONUP:
		inputSys->eventGamepadButtonUp(event.cbutton);
		break;
	case SDL_FINGERDOWN:
		inputSys->eventFingerDown(event.tfinger, titleBarHeight, windowHeight);
		break;
	case SDL_FINGERUP:
		inputSys->eventFingerUp(event.tfinger, titleBarHeight, windowHeight);
		break;
	case SDL_FINGERMOTION:
		inputSys->eventFingerMove(event.tfinger, titleBarHeight, windowHeight);
		break;
	case SDL_MULTIGESTURE:
		inputSys->eventFingerGesture(event.mgesture);
		break;
	case SDL_DROPTEXT:
		scene->onText(event.drop.file);
		SDL_free(event.drop.file);
		break;
	case SDL_USEREVENT:
		program->eventUser(event.user);
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_ENTER:
		scene->updateSelect();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		inputSys->eventMouseLeave();
		break;
	case SDL_WINDOWEVENT_MOVED:
		if (checkCurDisplay()) {
			setTitleBarHeight();
			updateView();
		}
	}
}

SDL_HitTestResult SDLCALL WindowSys::eventWindowHit(SDL_Window*, const SDL_Point* area, void* data) {
	WindowSys* ws = static_cast<WindowSys*>(data);
	return area->y >= ws->titleBarHeight || ws->scene->getTitleBarSelected(ivec2(area->x, area->y)) ? SDL_HITTEST_NORMAL : SDL_HITTEST_DRAGGABLE;
}

SDL_HitTestResult SDLCALL WindowSys::eventWindowHitStartup(SDL_Window*, const SDL_Point* area, void* data) {
	return area->y >= static_cast<WindowSys*>(data)->titleBarHeight ? SDL_HITTEST_NORMAL : SDL_HITTEST_DRAGGABLE;
}

ivec2 WindowSys::mousePos() {
	int x, y;
	SDL_GetMouseState(&x, &y);
	return ivec2(x, y - titleBarHeight);
}

void WindowSys::setTextCapture(Rect* field) {
	if (field) {
		field->y += titleBarHeight;
		SDL_SetTextInputRect(field);
		SDL_StartTextInput();
	} else
		SDL_StopTextInput();
}

void WindowSys::setMultisampling() {
	if (sets->msamples = sets->msamples > 1 ? std::min(ceilPower2(sets->msamples), uint32(maxMsamples)) : 0; sets->msamples)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
}

void WindowSys::setSwapInterval() {
	switch (sets->vsync) {
	case Settings::VSync::adaptive:
		if (trySetSwapInterval())
			break;
		sets->vsync = Settings::VSync::synchronized;
	case Settings::VSync::synchronized:
		if (trySetSwapInterval())
			break;
		sets->vsync = Settings::VSync::immediate;
	case Settings::VSync::immediate:
		trySetSwapInterval();
	}
}

bool WindowSys::trySetSwapInterval() {
	if (SDL_GL_SetSwapInterval(int8(sets->vsync))) {
		logError("swap interval ", int(sets->vsync), " not supported");
		return false;
	}
	return true;
}

void WindowSys::setScreen() {
	if (sets->display >= SDL_GetNumVideoDisplays())
		sets->display = 0;
	checkResolution(sets->size, windowSizes());
	checkResolution(sets->mode, displayModes());
	setWindowMode();
	program->getGui()->makeTitleBar();
	program->getState()->updateTitleBar();
	scene->onResize();
}

void WindowSys::setWindowMode() {
#ifndef __ANDROID__
	setTitleBarHeight();
	switch (sets->screen) {
	case Settings::Screen::window: case Settings::Screen::borderlessWindow: case Settings::Screen::borderless:
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowBordered(window, SDL_bool(sets->screen == Settings::Screen::window));
		SDL_SetWindowSize(window, sets->size.x, sets->size.y + titleBarHeight);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		break;
	case Settings::Screen::fullscreen:
		SDL_SetWindowDisplayMode(window, &sets->mode);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		break;
	case Settings::Screen::desktop:
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	setTitleBarHitTest();
	updateView();
#endif
}

void WindowSys::updateView() {
#ifdef __ANDROID__
	SDL_Rect rect;
	SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(window), &rect);	// SDL_GL_GetDrawableSize don't work properly
	screenView = ivec2(rect.w, rect.h);
#else
	SDL_GL_GetDrawableSize(window, &screenView.x, &screenView.y);
#endif
	windowHeight = screenView.y;
	screenView.y -= titleBarHeight;
	glViewport(0, 0, screenView.x, screenView.y);
}

void WindowSys::setTitleBarHeight() {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
	titleBarHeight = 0;
#else
	if (sets->screen == Settings::Screen::borderlessWindow) {
		float dpi;
		titleBarHeight = !SDL_GetDisplayDPI(displayID(), nullptr, nullptr, &dpi) ?  int(dpi / 4.f) : 32;
	} else
		titleBarHeight = 0;
#endif
}

void WindowSys::setTitleBarHitTest() {
	if (sets->screen == Settings::Screen::borderlessWindow)
		SDL_SetWindowHitTest(window, eventWindowHit, this);
	else
		SDL_SetWindowHitTest(nullptr, nullptr, nullptr);
}

void WindowSys::setGamma(float gamma) {
	sets->gamma = std::clamp(gamma, 0.f, Settings::gammaMax);
	SDL_SetWindowBrightness(window, sets->gamma);
}

void WindowSys::resetSettings() {
	*sets = Settings();
	inputSys->resetBindings();
	scene->reloadTextures();
	scene->reloadShader();
	scene->resetShadows();
	sets->font = fonts->init(sets->font, sets->hinting);
	checkCurDisplay();
	setMultisampling();
	setSwapInterval();
	SDL_SetWindowBrightness(window, sets->gamma);
	setScreen();
}

void WindowSys::reloadVaryingShaders() {
	delete sfinal;
	delete light;
	umap<string, string> sources = FileSys::loadShaders();
	light = new ShaderLight(sources.at(ShaderLight::fileVert), sources.at(ShaderLight::fileFrag), sets);
	sfinal = new ShaderFinal(sources.at(ShaderFinal::fileVert), sources.at(ShaderFinal::fileFrag), sets);
}

bool WindowSys::checkCurDisplay() {
	if (int disp = SDL_GetWindowDisplayIndex(window); disp >= 0 && sets->display != disp) {
		sets->display = disp;
		return true;
	}
	return false;
}

template <class T>
void WindowSys::checkResolution(T& val, const vector<T>& modes) {
#ifndef __ANDROID__
	if (std::none_of(modes.begin(), modes.end(), [val](const T& m) -> bool { return m == val; })) {
		if (modes.empty())
			throw std::runtime_error("no window modes available");

		typename vector<T>::const_iterator it;
		for (it = modes.begin(); it != modes.end() && *it < val; ++it);
		val = it == modes.begin() ? *it : *(it - 1);
	}
#endif
}

vector<ivec2> WindowSys::windowSizes() const {
	SDL_Rect max;
	if (SDL_GetDisplayBounds(sets->display, &max))
		max.w = max.h = INT_MAX;

	vector<ivec2> sizes;
	std::copy_if(resolutions.begin(), resolutions.end(), std::back_inserter(sizes), [&max](ivec2 it) -> bool { return it.x <= max.w && it.y <= max.h; });
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
		for (int im = 0; im < SDL_GetNumDisplayModes(i); ++im)
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, im, &mode) && mode.w <= max.w && mode.h <= max.h && float(mode.w) / float(mode.h) >= minimumRatio)
				sizes.emplace_back(mode.w, mode.h);
	return uniqueSort(sizes);
}

vector<SDL_DisplayMode> WindowSys::displayModes() const {
	vector<SDL_DisplayMode> mods;
	for (int im = 0; im < SDL_GetNumDisplayModes(sets->display); ++im)
		if (SDL_DisplayMode mode; !SDL_GetDisplayMode(sets->display, im, &mode) && float(mode.w) / float(mode.h) >= minimumRatio)
			mods.push_back(mode);
	return uniqueSort(mods);
}

int WindowSys::estimateSystemCursorSize() {
    int size = 32;
#ifdef _WIN32
    if (HICON icon = LoadIconW(nullptr, IDC_ARROW)) {
        if (ICONINFO info = { sizeof(info) }; GetIconInfo(icon, &info)) {
            if (info.hbmColor) {
                if (BITMAP bmp; GetObjectW(info.hbmColor, sizeof(bmp), &bmp) > 0)
                    size = bmp.bmHeight;
            } else if (info.hbmMask)
                if (BITMAP bmp; GetObjectW(info.hbmMask, sizeof(bmp), &bmp) > 0)
                    size = bmp.bmHeight / 2;
            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);
        }
        DestroyIcon(icon);
    }
#endif
    return size;
}

#if !defined(NDEBUG) && !defined(__APPLE__) && (!defined(OPENGLES) || OPENGLES == 32)
#ifdef _WIN32
void APIENTRY WindowSys::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
#else
void WindowSys::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
#endif
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;
	std::ostringstream ss("Debug message ");
	ss << id << ": " <<  message;
	if (sizet mlen = strlen(message); mlen && message[mlen-1] != '\n')
		ss << '\n';

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		ss << "Source: API, ";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		ss << "Source: window system, ";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		ss << "Source: shader compiler, ";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		ss << "Source: third party, ";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		ss << "Source: application, ";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		ss << "Source: other, ";
		break;
	default:
		ss << "Source: unknown, ";
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		ss << "Type: error, ";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		ss << "Type: deprecated behavior, ";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		ss << "Type: undefined behavior, ";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		ss << "Type: portability, ";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		ss << "Type: performance, ";
		break;
	case GL_DEBUG_TYPE_MARKER:
		ss << "Type: marker, ";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		ss << "Type: push group, ";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		ss << "Type: pop group, ";
		break;
	case GL_DEBUG_TYPE_OTHER:
		ss << "Type: other, ";
		break;
	default:
		ss << "Type: unknown, ";
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		logError(ss.str(), "Severity: high");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		logError(ss.str(), "Severity: medium");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		logError(ss.str(), "Severity: low");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		logInfo(ss.str(), "Severity: notification");
		break;
	default:
		logError(ss.str(), "Severity: unknown");
	}
}
#endif
