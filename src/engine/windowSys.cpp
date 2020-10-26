#include "windowSys.h"
#include "audioSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "prog/program.h"
#include <iostream>
#ifdef _WIN32
#include <winsock2.h>
#endif

// SHADERS

Shader::Shader(const string& srcVert, const string& srcFrag) {
	GLuint vert = loadShader(srcVert, GL_VERTEX_SHADER);
	GLuint frag = loadShader(srcFrag, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	checkStatus(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog);
	glUseProgram(program);
}

#ifndef OPENGLES
Shader::Shader(const string& srcVert, const string& srcGeom, const string& srcFrag) {
	GLuint vert = loadShader(srcVert, GL_VERTEX_SHADER);
	GLuint geom = loadShader(srcGeom, GL_GEOMETRY_SHADER);
	GLuint frag = loadShader(srcFrag, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, geom);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, geom);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(geom);
	glDeleteShader(frag);
	checkStatus(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog);
	glUseProgram(program);
}
#endif

GLuint Shader::loadShader(const string& source, GLenum type) {
	const char* src = source.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkStatus(shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog);
	return shader;
}

template <class C, class I>
void Shader::checkStatus(GLuint id, GLenum stat, C check, I info) {
	GLint res;
	string err;
	if (check(id, GL_INFO_LOG_LENGTH, &res); res) {
		err.resize(sizet(res));
		info(id, res, nullptr, err.data());
		std::cerr << err << std::endl;
	}
	if (check(id, stat, &res); res == GL_FALSE)
		throw std::runtime_error(err);
}

ShaderGeometry::ShaderGeometry(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, editShadowAlg(srcFrag, sets->shadowRes, sets->softShadows)),
	pview(glGetUniformLocation(program, "pview")),
	model(glGetUniformLocation(program, "model")),
	normat(glGetUniformLocation(program, "normat")),
	viewPos(glGetUniformLocation(program, "viewPos")),
	farPlane(glGetUniformLocation(program, "farPlane")),
	texsamp(glGetUniformLocation(program, "texsamp")),
	depthMap(glGetUniformLocation(program, "depthMap")),
	materialDiffuse(glGetUniformLocation(program, "material.diffuse")),
	materialSpecular(glGetUniformLocation(program, "material.specular")),
	materialShininess(glGetUniformLocation(program, "material.shininess")),
	lightPos(glGetUniformLocation(program, "light.pos")),
	lightAmbient(glGetUniformLocation(program, "light.ambient")),
	lightDiffuse(glGetUniformLocation(program, "light.diffuse")),
	lightLinear(glGetUniformLocation(program, "light.linear")),
	lightQuadratic(glGetUniformLocation(program, "light.quadratic"))
{
	glUniform1i(texsamp, 0);
	glUniform1i(depthMap, Light::depthTexa - GL_TEXTURE0);
}

string ShaderGeometry::editShadowAlg(string src, bool calc, bool soft) {
	constexpr char svar[] = "float shadow=";
	if (sizet pos = src.find(svar, src.find("void main()")); pos >= src.length())
		std::cerr << "failed to set shadow quality" << std::endl;
	else if (pos += strlen(svar); calc)
		src.replace(pos, src.find(';', pos) - pos, soft ? "calcShadowSoft()" : "calcShadowHard()");
	return src;
}

#ifndef OPENGLES
ShaderDepth::ShaderDepth(const string& srcVert, const string& srcGeom, const string& srcFrag) :
	Shader(srcVert, srcGeom, srcFrag),
	model(glGetUniformLocation(program, "model")),
	shadowMats(glGetUniformLocation(program, "shadowMats")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	farPlane(glGetUniformLocation(program, "farPlane"))
{}
#endif

ShaderGui::ShaderGui(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag),
	pview(glGetUniformLocation(program, "pview")),
	rect(glGetUniformLocation(program, "rect")),
	uvrc(glGetUniformLocation(program, "uvrc")),
	zloc(glGetUniformLocation(program, "zloc")),
	color(glGetUniformLocation(program, "color")),
	texsamp(glGetUniformLocation(program, "texsamp"))
{
	glUniform1i(texsamp, 0);
}

// FONT SET

FontSet::FontSet(bool regular) :
	fontData(FileSys::loadFile(FileSys::dataPath(regular ? fileFont : fileFontAlt)))
{
	TTF_Font* tmp = TTF_OpenFontRW(SDL_RWFromMem(fontData.data(), int(fontData.size())), SDL_TRUE, fontTestHeight);
	if (!tmp)
		throw std::runtime_error(TTF_GetError());
	int size;	// get approximate height scale factor
	heightScale = !TTF_SizeUTF8(tmp, fontTestString, nullptr, &size) ? float(fontTestHeight) / float(size) : fallbackScale;
	TTF_CloseFont(tmp);
}

void FontSet::clear() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
	fonts.clear();
}

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
	if (umap<int, TTF_Font*>::iterator it = fonts.find(height); it != fonts.end())
		return it->second;

	TTF_Font* font = TTF_OpenFontRW(SDL_RWFromMem(fontData.data(), int(fontData.size())), SDL_TRUE, height);
	if (font)
		fonts.emplace(height, font);
	else
		std::cerr << TTF_GetError() << std::endl;
	return font;
}

int FontSet::length(const char* text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text, &len, nullptr);
	return len;
}

// LOADER

void WindowSys::Loader::addLine(const string& str, WindowSys* win) {
	logStr += str + '\n';
	if (uint lines = std::count(logStr.begin(), logStr.end(), '\n'), maxl = uint(win->getScreenView().y / logSize); lines > maxl) {
		sizet end = 0;
		for (uint i = lines - maxl; i; --i)
			end = logStr.find_first_of('\n', end) + 1;
		logStr.erase(0, end);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (Texture tex = win->getFonts()->render(logStr.c_str(), logSize, uint(win->getScreenView().x)); tex) {
		glUseProgram(*win->getGui());
		glBindVertexArray(win->getGui()->wrect.getVao());
		Quad::draw(Rect(0, 0, tex.getRes()), vec4(1.f), tex);
	}
	SDL_GL_SwapWindow(win->getWindow());
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
#ifndef OPENGLES
	depth = nullptr;
#endif
	gui = nullptr;
	fonts = nullptr;
	run = true;
	cursorHeight = 18;

	int rc = EXIT_SUCCESS;
	try {
		init(args);
#ifdef EMSCRIPTEN
		emscripten_set_main_loop_arg([](void* data) {
			WindowSys* w = static_cast<WindowSys*>(data);
			(w->*w->loopFunc)();
		}, this, 0, true);
#else
		for (; run; exec());
#endif
	} catch (const std::runtime_error& e) {
		rc = showError("Error", e.what());
#ifdef NDEBUG
	} catch (...) {
		rc = showError("Error", "unknown error");
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
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(string("failed to initialize systems:") + linend + SDL_GetError());
	if (IMG_Init(imgInitFlags) != imgInitFlags)
		throw std::runtime_error(string("failed to initialize textures:") + linend + IMG_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("failed to initialize fonts:") + linend + TTF_GetError());
#ifdef _WIN32
	if (WSADATA wsad; WSAStartup(MAKEWORD(2, 2), &wsad))
		throw std::runtime_error(Com::msgWinsockFail);
#endif

#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#if SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_EventState(SDL_DISPLAYEVENT, SDL_DISABLE);
#endif
	SDL_EventState(SDL_TEXTEDITING, SDL_DISABLE);
	SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARRECORD, SDL_DISABLE);
	SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DROPFILE, SDL_DISABLE);
	SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
#if SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
#endif
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
	SDL_StopTextInput();

	FileSys::init(args);
#ifdef EMSCRIPTEN
	loader = std::make_unique<Loader>();
	loopFunc = &WindowSys::load;
#else
	for (Loader loader; loader.state < Loader::State::done; load(&loader));
#endif
}

#ifdef EMSCRIPTEN
void WindowSys::load() {
#else
void WindowSys::load(Loader* loader) {
#endif
	switch (loader->state) {
	case Loader::State::start:
#ifdef EMSCRIPTEN
		if (FileSys::canRead())
			return;
#endif
		inputSys = new InputSys;
		sets = new Settings(FileSys::loadSettings(inputSys));
		fonts = new FontSet(sets->fontRegular);
		createWindow();
		loader->addLine("loading audio", this);
		break;
	case Loader::State::audio:
		try {
			audio = new AudioSys(this);
		} catch (const std::runtime_error& err) {
			delete audio;
			std::cerr << err.what() << std::endl;
		}
		loader->addLine("loading objects", this);
		break;
	case Loader::State::objects:
		scene = new Scene;
		scene->loadObjects();
		loader->addLine("loading textures", this);
		break;
	case Loader::State::textures:
		scene->loadTextures();
		loader->addLine("starting", this);
		break;
	case Loader::State::program:
#ifdef EMSCRIPTEN
		loader.reset();
		loopFunc = &WindowSys::exec;
#endif
		program = new Program;
		program->start();
		oldTime = SDL_GetTicks();
	}
	++loader->state;
}

void WindowSys::createWindow() {
	uint32 flags = SDL_WINDOW_OPENGL;
	switch (sets->screen) {
	case Settings::Screen::borderless:
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sets->msamples != 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sets->msamples);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, 0);
#endif
#endif
	if (!(window = SDL_CreateWindow(title, winPos, winPos, sets->size.x, sets->size.y, flags)))
		throw std::runtime_error(string("failed to create window:") + linend + SDL_GetError());
#ifndef __ANDROID__
	checkCurDisplay();
	checkResolution(sets->size, windowSizes());
	checkResolution(sets->mode, displayModes());
	if (sets->screen <= Settings::Screen::borderless) {
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
	} else if (sets->screen == Settings::Screen::fullscreen)
		SDL_SetWindowDisplayMode(window, &sets->mode);
#endif
	SDL_SetWindowBrightness(window, sets->gamma);

	// load icons
#ifndef __ANDROID__
#ifndef EMSCRIPTEN
	if (SDL_Surface* icon = IMG_Load(FileSys::dataPath(fileIcon).c_str())) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
#endif
	if (SDL_Surface* icon = IMG_Load(FileSys::dataPath(fileCursor).c_str())) {
		if (SDL_Cursor* cursor = SDL_CreateColorCursor(icon, 0, 0)) {
			cursorHeight = uint8(icon->h);
			SDL_SetCursor(cursor);
		}
		SDL_FreeSurface(icon);
	}
#endif

	// create context and set up rendering
	if (!(context = SDL_GL_CreateContext(window)))
		throw std::runtime_error(string("failed to create context:") + linend + SDL_GetError());
	setSwapInterval();
#if !defined(OPENGLES) && !defined(__APPLE__)
	glewExperimental = GL_TRUE;
	glewInit();
#endif
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
#ifndef OPENGLES
	sets->msamples ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
#endif

	umap<string, string> sources = FileSys::loadShaders();
	geom = new ShaderGeometry(sources.at(fileGeometryVert), sources.at(fileGeometryFrag), sets);
#ifndef OPENGLES
	depth = new ShaderDepth(sources.at(fileDepthVert), sources.at(fileDepthGeom), sources.at(fileDepthFrag));
#endif
	gui = new ShaderGui(sources.at(fileGuiVert), sources.at(fileGuiFrag));

	// init startup log
	glViewport(0, 0, screenView.x, screenView.y);
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(*gui);
	glUniform2f(gui->pview, float(screenView.x) / 2.f, float(screenView.y) / 2.f);
}

void WindowSys::destroyWindow() {
	delete gui;
#ifndef OPENGLES
	delete depth;
#endif
	delete geom;
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
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
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
	case SDL_MOUSEMOTION:
		inputSys->eventMouseMotion(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
		inputSys->eventMouseButtonDown(event.button);
		break;
	case SDL_MOUSEBUTTONUP:
		inputSys->eventMouseButtonUp(event.button);
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
		inputSys->eventFingerDown(event.tfinger);
		break;
	case SDL_FINGERUP:
		inputSys->eventFingerUp(event.tfinger);
		break;
	case SDL_FINGERMOTION:
		inputSys->eventFingerMove(event.tfinger);
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
	case SDL_WINDOWEVENT_MOVED:	// doesn't work on windows for some reason
		checkCurDisplay();
	}
}

void WindowSys::setTextCapture(bool on) {
	on ? SDL_StartTextInput() : SDL_StopTextInput();
#ifdef __ANDROID__
	guiView.y = on ? guiView.y / 2 : screenView.y;
	scene->onResize();
#endif
}

void WindowSys::setVsync(Settings::VSync vsync) {
	sets->vsync = vsync;
	setSwapInterval();
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
		std::cerr << "swap interval " << int(sets->vsync) << " not supported" << std::endl;
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
	scene->onResize();
	scene->resetLayouts();	// necessary to reset widgets' pixel sizes
}

void WindowSys::setWindowMode() {
#ifndef __ANDROID__
	switch (sets->screen) {
	case Settings::Screen::window: case Settings::Screen::borderless:
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowBordered(window, SDL_bool(sets->screen == Settings::Screen::window));
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
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
	guiView = screenView;
#ifdef __ANDROID__
	if (SDL_IsTextInputActive())
		guiView.y /= 2;
#endif
}

void WindowSys::setGamma(float gamma) {
	sets->gamma = std::clamp(gamma, 0.f, Settings::gammaMax);
	SDL_SetWindowBrightness(window, sets->gamma);
}

void WindowSys::resetSettings() {
	*sets = Settings();
	inputSys->resetBindings();
	scene->reloadTextures();
	scene->resetShadows();
	scene->reloadShader();
	reloadFont(sets->fontRegular);
	checkCurDisplay();
	setSwapInterval();
	SDL_SetWindowBrightness(window, sets->gamma);
	setScreen();
}

void WindowSys::reloadGeom() {
	delete geom;
	umap<string, string> sources = FileSys::loadShaders();
	geom = new ShaderGeometry(sources.at(fileGeometryVert), sources.at(fileGeometryFrag), sets);
}

void WindowSys::reloadFont(bool regular) {
	delete fonts;
	fonts = new FontSet(sets->fontRegular = regular);
}

bool WindowSys::checkCurDisplay() {
	if (int disp = SDL_GetWindowDisplayIndex(window); disp >= 0 && sets->display != disp) {
		sets->display = uint8(disp);
		return true;
	}
	return false;
}

template <class T>
void WindowSys::checkResolution(T& val, const vector<T>& modes) {
#if !defined(__ANDROID__) && !defined(__APPLE__)
	if (std::none_of(modes.begin(), modes.end(), [val](const T& m) -> bool { return m == val; })) {
		if (modes.empty())
			throw std::runtime_error("No window modes available");

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
	std::copy_if(resolutions.begin(), resolutions.end(), std::back_inserter(sizes), [&max](const ivec2& it) -> bool { return it.x <= max.w && it.y <= max.h; });
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

int WindowSys::showError(const char* caption, const char* message) {
	std::cerr << message << std::endl;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, message, window);
	return EXIT_FAILURE;
}
