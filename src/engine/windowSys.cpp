#include "windowSys.h"

// FONT SET

FontSet::~FontSet() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
}

void FontSet::init(const string& path) {
	clear();
	file = path;
	TTF_Font* tmp = TTF_OpenFont(path.c_str(), fontTestHeight);
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
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.emplace(size, font);
	else
		std::cerr << TTF_GetError() << std::endl;
	return font;
}

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
	try {	// load font if it hasn't been loaded yet
		return fonts.at(height);
	} catch (const std::out_of_range&) {}
	return addSize(height);
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
	} catch (...) {
		std::cerr << "unknown error" << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "unknown error", window);
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
	fonts.init(fileFont);
	createWindow();
	scene.reset(new Scene);
	program.reset(new Program);
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

	// create new window
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	setResolution(sets->resolution);
	if (!(window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, windowFlags | (sets->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))))
		throw std::runtime_error(string("failed to create window:\n") + SDL_GetError());

	SDL_SetWindowMinimumSize(window, minWindowSize.x, minWindowSize.y);
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
	glEnable(GL_MULTISAMPLE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// load default textures with colors
	for (const string& file : FileSys::listDir(FileSys::dirTexs, FTYPE_REG)) {
		try {
			texes.emplace(delExt(file), FileSys::dirTexs + file);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
		}
	}
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
		scene->onMouseDown(vec2i(event.button.x, event.button.y), event.button.button, event.button.clicks);
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
	case SDL_WINDOWEVENT_RESIZED:
		if (uint32 flags = SDL_GetWindowFlags(window); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && !(sets->maximized = flags & SDL_WINDOW_MAXIMIZED))	// update settings if needed
			SDL_GetWindowSize(window, &sets->resolution.x, &sets->resolution.y);
		updateViewport();
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		updateViewport();
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
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
	if (SDL_GL_SetSwapInterval(sets->vsyncToInterval()))
		std::cerr << "swap interval " << sets->vsyncToInterval() << " not supported" << std::endl;
}

Texture WindowSys::renderText(const string& text, int height) {
	try {
		return !text.empty() ? Texture(TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colorText), text, false) : Texture();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
	}
	return Texture();
}

const Texture* WindowSys::texture(const string& name) const {
	try {
		return &texes.at(name);
	} catch (const std::out_of_range&) {
		std::cerr << "texture " << name << " doesn't exist" << std::endl;
	}
	return nullptr;
}

void WindowSys::setFullscreen(bool on) {
	sets->fullscreen = on;
	SDL_SetWindowFullscreen(window, on ? SDL_GetWindowFlags(window) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(window) & uint32(~SDL_WINDOW_FULLSCREEN_DESKTOP));
}

void WindowSys::setResolution(const string& line) {
	setResolution(vec2i::get(line, strtoul, 0));
	SDL_SetWindowSize(window, sets->resolution.x, sets->resolution.y);
}

void WindowSys::setResolution(const vec2i& res) {
	sets->resolution = res.clamp(minWindowSize, displayResolution());
	sets->resolution.x = clampLow(sets->resolution.x, sets->resolution.y);
}

void WindowSys::setVsync(Settings::VSync vsync) {
	sets->vsync = vsync;
	setSwapInterval();
}

void WindowSys::resetSettings() {
	sets.reset(new Settings);
	createWindow();
	scene->resetLayouts();
}
