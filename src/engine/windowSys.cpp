#include "windowSys.h"

// FONT SET

FontSet::FontSet() :
	fontData(readFile<vector<uint8>>(fileFont))
{
	if (!(logFont = TTF_OpenFontRW(SDL_RWFromMem(fontData.data(), int(fontData.size())), SDL_TRUE, fontTestHeight)))
		throw std::runtime_error(TTF_GetError());
	int size;	// get approximate height scale factor
	heightScale = !TTF_SizeUTF8(logFont, fontTestString, nullptr, &size) ? float(fontTestHeight) / float(size) : fallbackScale;
	if (TTF_CloseFont(logFont); !(logFont = TTF_OpenFontRW(SDL_RWFromMem(fontData.data(), int(fontData.size())), SDL_TRUE, logSize)))
		throw std::runtime_error(TTF_GetError());
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

int FontSet::length(const string& text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

void FontSet::writeLog(string&& text, vec2i res) {
	logLines.push_back(std::move(text));
	if (uint maxl = uint(res.y / int(float(logSize) / heightScale)); logLines.size() > maxl)
		logLines.erase(logLines.begin(), logLines.begin() + pdift(logLines.size() - maxl));

	string str;
	for (const string& it : logLines)
		str += it + '\n';
	str.pop_back();
	logTex.close();
	if (logTex = TTF_RenderUTF8_Blended_Wrapped(logFont, str.c_str(), logColor, uint(res.x)); logTex.valid()) {
		glBindTexture(GL_TEXTURE_2D, logTex.getID());
		glColor4f(1.f, 1.f, 1.f, 1.f);

		glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex2i(0, 0);
		glTexCoord2i(1, 0);
		glVertex2i(logTex.getRes().x, 0);
		glTexCoord2i(1, 1);
		glVertex2i(logTex.getRes().x, logTex.getRes().y);
		glTexCoord2i(0, 1);
		glVertex2i(0, logTex.getRes().y);
		glEnd();
	}
}

void FontSet::closeLog() {
	logTex.close();
	TTF_CloseFont(logFont);
	logLines.clear();
}

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

	GLint result;
	if (glGetProgramiv(program, GL_LINK_STATUS, &result); result == GL_FALSE) {
		GLint infoLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);

		string err;
		err.resize(sizet(infoLen));
		glGetProgramInfoLog(program, infoLen, nullptr, err.data());
		throw std::runtime_error(err);
	}
}

GLuint Shader::loadShader(const string& source, GLenum type) {
	const char* src = source.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint result;
	if (glGetShaderiv(shader, GL_COMPILE_STATUS, &result); result == GL_FALSE) {
		GLint infoLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		string err;
		err.resize(sizet(infoLen));
		glGetShaderInfoLog(shader, infoLen, nullptr, err.data());
		throw std::runtime_error(err);
	}
	return shader;
}

ShaderScene::ShaderScene(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag)
{
	glUseProgram(program);
	pview = glGetUniformLocation(program, "pview");
	model = glGetUniformLocation(program, "model");
	normat = glGetUniformLocation(program, "normat");
	vertex = glGetAttribLocation(program, "vertex");
	uvloc = glGetAttribLocation(program, "uvloc");
	normal = glGetAttribLocation(program, "normal");
	materialDiffuse = glGetUniformLocation(program, "material.diffuse");
	materialEmission = glGetUniformLocation(program, "material.emission");
	materialSpecular = glGetUniformLocation(program, "material.specular");
	materialShininess = glGetUniformLocation(program, "material.shininess");
	materialAlpha = glGetUniformLocation(program, "material.alpha");
	texsamp = glGetUniformLocation(program, "texsamp");
	viewPos = glGetUniformLocation(program, "viewPos");
	lightPos = glGetUniformLocation(program, "light.pos");
	lightColor = glGetUniformLocation(program, "light.color");
	lightLinear = glGetUniformLocation(program, "light.linear");
	lightQuadratic = glGetUniformLocation(program, "light.quadratic");
	glUniform1i(texsamp, 0);
}

ShaderGUI::ShaderGUI(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag)
{
	glUseProgram(program);
	pview = glGetUniformLocation(program, "pview");
	rect = glGetUniformLocation(program, "rect");
	uvrc = glGetUniformLocation(program, "uvrc");
	zloc = glGetUniformLocation(program, "zloc");
	vertex = glGetAttribLocation(program, "vertex");
	uvloc = glGetAttribLocation(program, "uvloc");
	color = glGetUniformLocation(program, "color");
	texsamp = glGetUniformLocation(program, "texsamp");
	glUniform1i(texsamp, 0);
	wrect.init(this);
}

ShaderGUI::~ShaderGUI() {
	glUseProgram(program);
	wrect.free(this);
}

// WINDOW SYS

WindowSys::WindowSys() :
	window(nullptr),
	context(nullptr),
	dSec(0.f),
	run(true),
	cursor(nullptr)
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
	if (int err = FileSys::setWorkingDir())
		throw std::runtime_error("failed to set working directory (" + toStr(err) + ')');
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO))
		throw std::runtime_error(string("failed to initialize systems:") + linend + SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("failed to initialize fonts:") + linend + TTF_GetError());
	if (SDLNet_Init())
		throw std::runtime_error(string("failed to initialize networking:") + linend + SDLNet_GetError());
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_StopTextInput();	// for some reason TextInput is on

	sets.reset(FileSys::loadSettings());
	fonts.reset(new FontSet);
	createWindow();
	try {
		audio.reset(new AudioSys);
	} catch (const std::runtime_error& err) {
		std::cerr << err.what() << std::endl;
		writeLog(err.what());
		audio.reset();
	}
	scene.reset(new Scene);
	program.reset(new Program);
	fonts->closeLog();
}

void WindowSys::exec() {
	program->start();
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / ticksPerSec;
		oldTime = newTime;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene->draw();
		SDL_GL_SwapWindow(window);

		scene->tick(dSec);
		program->getGame()->tick();

		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		for (SDL_Event event; SDL_PollEvent(&event) && SDL_GetTicks() < timeout;)
			handleEvent(event);
	}
	FileSys::saveSettings(sets.get());
}

void WindowSys::cleanup() {
	program.reset();
	scene.reset();
	destroyWindow();
	audio.reset();
	fonts.reset();
	sets.reset();

	SDLNet_Quit();
	TTF_Quit();
	SDL_Quit();
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

	// create new window
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sets->msamples != 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sets->msamples);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	if (!(window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), sets->size.x, sets->size.y, flags)))
		throw std::runtime_error(string("failed to create window:") + linend + SDL_GetError());
	checkCurDisplay();
	checkResolution(sets->size, displaySizes());
	checkResolution(sets->mode, displayModes());
	if (sets->screen <= Settings::Screen::borderless) {
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
	} else if (sets->screen == Settings::Screen::fullscreen)
		SDL_SetWindowDisplayMode(window, &sets->mode);
	SDL_SetWindowBrightness(window, sets->gamma);

	// load icons
	if (SDL_Surface* icon = SDL_LoadBMP(fileIcon)) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	if (SDL_Surface* icon = SDL_LoadBMP(fileCursor)) {
		if (SDL_Cursor* cursor = SDL_CreateColorCursor(icon, 0, 0)) {
			cursorHeight = uint8(icon->h);
			SDL_SetCursor(cursor);
		} else
			cursorHeight = fallbackCursorSize;
		SDL_FreeSurface(icon);
	} else
		cursorHeight = fallbackCursorSize;

	// create context and set up rendering
	if (!(context = SDL_GL_CreateContext(window)))
		throw std::runtime_error(string("failed to create context:") + linend + SDL_GetError());
	setSwapInterval();
	glewExperimental = GL_TRUE;
	glewInit();
	updateViewport();

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glEnable(GL_TEXTURE_2D);
	sets->msamples ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);

	umap<string, string> sources = FileSys::loadShaders();
	space.reset(new ShaderScene(sources.at(fileSceneVert), sources.at(fileSceneFrag)));
	gui.reset(new ShaderGUI(sources.at(fileGuiVert), sources.at(fileGuiFrag)));
	glActiveTexture(GL_TEXTURE0);

	// for startup log
	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, curView.x, curView.y, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void WindowSys::destroyWindow() {
	space.reset();
	gui.reset();
	SDL_FreeCursor(cursor);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
}

void WindowSys::handleEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_MOUSEMOTION:
		scene->onMouseMove(vec2i(event.motion.x, event.motion.y), vec2i(event.motion.xrel, event.motion.yrel), event.motion.state, event.motion.timestamp);
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
	case SDL_WINDOWEVENT_RESIZED:
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		updateViewport();
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
	case SDL_WINDOWEVENT_MOVED:	// doesn't work on windows for some reason
		if (checkCurDisplay() && sets->screen <= Settings::Screen::borderless && !checkResolution(sets->size, displaySizes())) {
			SDL_SetWindowSize(window, sets->size.x, sets->size.y);
			updateViewport();
			scene->onResize();
		}
		break;
	case SDL_WINDOWEVENT_CLOSE:
		close();
	}
}

void WindowSys::writeLog(string&& text) {
	glUseProgram(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	fonts->writeLog(std::move(text), curView);
	SDL_GL_SwapWindow(window);
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

void WindowSys::setScreen(uint8 display, Settings::Screen screen, vec2i size, const SDL_DisplayMode& mode) {
	sets->display = clampHigh(display, uint8(SDL_GetNumVideoDisplays()));
	sets->screen = screen;
	sets->size = size;
	checkResolution(sets->size, displaySizes());
	sets->mode = mode;
	checkResolution(sets->mode, displayModes());
	setWindowMode();
	scene->onResize();
}

void WindowSys::setWindowMode() {
	if (sets->screen <= Settings::Screen::borderless) {
		SDL_SetWindowFullscreen(window, 0);
		SDL_SetWindowBordered(window, SDL_bool(sets->screen == Settings::Screen::window));
		SDL_SetWindowSize(window, sets->size.x, sets->size.y);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
	} else  if (sets->screen == Settings::Screen::fullscreen) {
		SDL_SetWindowDisplayMode(window, &sets->mode);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->display));
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	updateViewport();
}

void WindowSys::setGamma(float gamma) {
	sets->gamma = std::clamp(gamma, 0.f, Settings::gammaMax);
	SDL_SetWindowBrightness(window, sets->gamma);
}

void WindowSys::resetSettings() {
	sets.reset(new Settings);
	checkCurDisplay();
	setWindowMode();
	setSwapInterval();
	SDL_SetWindowBrightness(window, sets->gamma);
	scene->resetLayouts();
}

bool WindowSys::checkCurDisplay() {
	if (int disp = SDL_GetWindowDisplayIndex(window); disp >= 0 && sets->display != disp) {
		sets->display = uint8(disp);
		return true;
	}
	return false;
}

vector<vec2i> WindowSys::displaySizes() const {
	vector<vec2i> sizes;
	for (int im = 0; im < SDL_GetNumDisplayModes(sets->display); im++)
		if (SDL_DisplayMode mode; !SDL_GetDisplayMode(sets->display, im, &mode))
			if (float(mode.w) / float(mode.h) >= resolutionRatioLimit)
				sizes.emplace_back(mode.w, mode.h);
	return uniqueSort(sizes);
}

vector<SDL_DisplayMode> WindowSys::displayModes() const {
	vector<SDL_DisplayMode> mods;
	for (int im = 0; im < SDL_GetNumDisplayModes(sets->display); im++)
		if (SDL_DisplayMode mode; !SDL_GetDisplayMode(sets->display, im, &mode))
			mods.push_back(mode);
	return uniqueSort(mods);
}
