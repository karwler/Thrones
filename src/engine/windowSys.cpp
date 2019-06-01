#include "windowSys.h"

// FONT SET

FontSet::~FontSet() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
}

void FontSet::init() {
	clear();
	TTF_Font* tmp = TTF_OpenFont(fileFont, fontTestHeight);
	if (!tmp)
		throw std::runtime_error(TTF_GetError());

	int size;	// get approximate height scale factor
	TTF_SizeUTF8(tmp, fontTestString, nullptr, &size);
	heightScale = float(fontTestHeight) / float(size);
	TTF_CloseFont(tmp);
}

void FontSet::clear() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
	fonts.clear();
}

TTF_Font* FontSet::addSize(int size) {
	TTF_Font* font = TTF_OpenFont(fileFont, size);
	if (font)
		fonts.emplace(size, font);
	else
		std::cerr << TTF_GetError() << std::endl;
	return font;
}

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
	umap<int, TTF_Font*>::iterator it = fonts.find(height);
	return it != fonts.end() ? it->second : addSize(height);	// load font if it hasn't been loaded yet
}

int FontSet::length(const string& text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

// WINDOW SYS

WindowSys::WindowSys() :
	window(nullptr),
	context(nullptr),
	dSec(0.f),
	run(true)
{}

int WindowSys::start() {
	try {
		init();
		exec();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), window);
#ifdef NDEBUG
	} catch (...) {
		std::cerr << "unknown error" << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "unknown error", window);
#endif
	}
	cleanup();
	return 0;
}

void WindowSys::init() {
	if (SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(string("failed to initialize video:\n") + SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("failed to initialize fonts:\n") + TTF_GetError());
	if (SDLNet_Init())
		throw std::runtime_error(string("failed to initialize networking:\n") + SDLNet_GetError());
	SDL_StopTextInput();	// for some reason TextInput is on

	fileSys.reset(new FileSys);
	sets.reset(fileSys->loadSettings());
	fonts.init();
	createWindow();
	scene.reset(new Scene);
	program.reset(new Program);

	for (const string& file : FileSys::listDir(FileSys::dirTexs, FTYPE_REG, "bmp")) {
		try {
			texes.emplace(delExt(file), appDsep(FileSys::dirTexs) + file);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
		}
	}
}

void WindowSys::exec() {
	program->start();
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		setDSec(oldTime);

		scene->draw();
		SDL_GL_SwapWindow(window);

		scene->tick(dSec);
		program->getGame()->tick();

		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		for (SDL_Event event; SDL_PollEvent(&event) && SDL_GetTicks() < timeout;)
			handleEvent(event);
	}
	fileSys->saveSettings(sets.get());
}

void WindowSys::cleanup() {
	for (auto& [name, tex] : texes)
		tex.close();

	program.reset();
	scene.reset();
	destroyWindow();
	fonts.clear();
	fileSys.reset();

	SDLNet_Quit();
	TTF_Quit();
	SDL_Quit();
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

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

	// create new window
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sets->samples != 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sets->samples);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	if (!(window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->size.x, sets->size.y, flags)))
		throw std::runtime_error(string("failed to create window:\n") + SDL_GetError());

	curDisplay = SDL_GetWindowDisplayIndex(window);
	checkResolution(sets->size, displaySizes());
	checkResolution(sets->mode, displayModes());
	if (sets->screen <= Settings::Screen::borderless)
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
	else if (sets->screen == Settings::Screen::fullscreen)
		SDL_SetWindowDisplayMode(window, &sets->mode);
	setGamma(sets->gamma);
	SDL_SetWindowBrightness(window, sets->brightness);

	if (SDL_Surface* icon = SDL_LoadBMP(fileIcon)) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}

	// create context and set up rendering
	if (!(context = SDL_GL_CreateContext(window)))
		throw std::runtime_error(string("failed to create context:\n") + SDL_GetError());
	setSwapInterval();

	updateViewport();
	glClearColor(colorClear[0], colorClear[1], colorClear[2], colorClear[3]);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glShadeModel(GL_SMOOTH);
	sets->samples ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
	updateSmooth();
}

void WindowSys::destroyWindow() {
	if (window) {
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

void WindowSys::handleEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_MOUSEMOTION:
		scene->onMouseMove(vec2i(event.motion.x, event.motion.y), vec2i(event.motion.xrel, event.motion.yrel));
		break;
	case SDL_MOUSEBUTTONDOWN:
		scene->onMouseDown(vec2i(event.button.x, event.button.y), event.button.button);
		break;
	case SDL_MOUSEBUTTONUP:
		scene->onMouseUp(vec2i(event.button.x, event.button.y), event.button.button);
		break;
	case SDL_MOUSEWHEEL:
		scene->onMouseWheel(vec2i(event.wheel.x, -event.wheel.y));
		break;
	case SDL_KEYDOWN:
		scene->onKeyDown(event.key);
		break;
	case SDL_KEYUP:
		scene->onKeyUp(event.key);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
	case SDL_QUIT:
		close();
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		updateViewport();
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
	case SDL_WINDOWEVENT_MOVED:
		if (int newDisp = SDL_GetWindowDisplayIndex(window); curDisplay != newDisp) {
			if (curDisplay = newDisp; sets->screen <= Settings::Screen::borderless) {
				if (!checkResolution(sets->size, displaySizes()))
					SDL_SetWindowSize(window, sets->size.x, sets->size.y);
			} else if (sets->screen == Settings::Screen::fullscreen && !checkResolution(sets->mode, displayModes()))
				SDL_SetWindowDisplayMode(window, &sets->mode);
		}
		break;
	case SDL_WINDOWEVENT_CLOSE:
		close();
	}
}

void WindowSys::setDSec(uint32& oldTicks) {
	uint32 newTime = SDL_GetTicks();
	dSec = float(newTime - oldTicks) / ticksPerSec;
	oldTicks = newTime;
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

void WindowSys::updateSmooth() const {
	GLenum hmode = sets->smooth == Settings::Smooth::nice ? GL_NICEST : GL_FASTEST;
	if (sets->smooth == Settings::Smooth::off) {
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
	} else {
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, hmode);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, hmode);
		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, hmode);
	}
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, hmode);
}

const Texture* WindowSys::texture(const string& name) const {
	umap<string, Texture>::const_iterator it = texes.find(name);
	return it != texes.end() ? &it->second : nullptr;
}

void WindowSys::setScreen(Settings::Screen screen, vec2i size, const SDL_DisplayMode& mode, uint8 samples) {
	sets->size = size;
	checkResolution(sets->size, displaySizes());
	sets->mode = mode;
	checkResolution(sets->mode, displayModes());
	if (sets->screen = screen; samples != sets->samples) {
		sets->samples = samples;
		createWindow();
	} else if (sets->screen <= Settings::Screen::borderless) {
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowBordered(window, SDL_bool(sets->screen == Settings::Screen::window));
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED);
	} else  if (sets->screen == Settings::Screen::fullscreen) {
		SDL_SetWindowDisplayMode(window, &sets->mode);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	updateViewport();
	scene->onResize();
}

void WindowSys::setVsync(Settings::VSync vsync) {
	sets->vsync = vsync;
	setSwapInterval();
}

void WindowSys::setSmooth(Settings::Smooth smooth) {
	sets->smooth = smooth;
	updateSmooth();
}

void WindowSys::setGamma(float gamma) {
	sets->gamma = clampHigh(gamma, Settings::gammaMax);
	uint16 ramp[256];
	SDL_CalculateGammaRamp(sets->gamma, ramp);
	SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
}

void WindowSys::setBrightness(float bright) {
	sets->brightness = clampHigh(bright, 1.f);
	SDL_SetWindowBrightness(window, sets->brightness);
}

void WindowSys::resetSettings() {
	sets.reset(new Settings);
	createWindow();
	scene->resetLayouts();
}

vector<vec2i> WindowSys::displaySizes() const {
	vector<vec2i> sizes;
	for (int im = 0; im < SDL_GetNumDisplayModes(curDisplay); im++)
		if (SDL_DisplayMode mode; !SDL_GetDisplayMode(curDisplay, im, &mode))
			sizes.emplace_back(mode.w, mode.h);
	return uniqueSort(sizes);
}

vector<SDL_DisplayMode> WindowSys::displayModes() const {
	vector<SDL_DisplayMode> mods;
	for (int im = 0; im < SDL_GetNumDisplayModes(curDisplay); im++)
		if (SDL_DisplayMode mode; !SDL_GetDisplayMode(curDisplay, im, &mode))
			mods.push_back(mode);
	return uniqueSort(mods);
}
