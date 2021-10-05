#include "windowSys.h"
#include "audioSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "prog/program.h"
#include <iostream>
#include <regex>
#ifdef _WIN32
#include <winsock2.h>
#endif

// SHADERS

Shader::Shader(const string& srcVert, const string& srcFrag, const char* name) {
	GLuint vert = loadShader(srcVert, GL_VERTEX_SHADER, name);
	GLuint frag = loadShader(srcFrag, GL_FRAGMENT_SHADER, name);

	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	checkStatus(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog, name + string(" prog"));
	glUseProgram(program);
}

GLuint Shader::loadShader(const string& source, GLenum type, const char* name) {
	const char* src = source.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkStatus(shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + string(type == GL_VERTEX_SHADER ? " vert" : type == GL_FRAGMENT_SHADER ? " frag" : ""));
	return shader;
}

template <class C, class I>
void Shader::checkStatus(GLuint id, GLenum stat, C check, I info, const string& name) {
	int res;
	if (check(id, stat, &res); res == GL_FALSE) {
		string err;
		if (check(id, GL_INFO_LOG_LENGTH, &res); res) {
			err.resize(res);
			info(id, res, nullptr, err.data());
			err = trim(err);
		}
		err = !err.empty() ? name + ':' + linend + err : name + ": unknown error";
		std::cerr << err << std::endl;
		throw std::runtime_error(err);
	}
}

pairStr Shader::splitGlobMain(const string& src) {
	sizet pmain = src.find("main()");
	return pair(src.substr(0, pmain), src.substr(pmain));
}

ShaderGeom::ShaderGeom(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader geometry"),
	proj(glGetUniformLocation(program, "proj")),
	view(glGetUniformLocation(program, "view"))
{}

ShaderDepth::ShaderDepth(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader depth"),
	pvTrans(glGetUniformLocation(program, "pvTrans")),
	pvId(glGetUniformLocation(program, "pvId")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	farPlane(glGetUniformLocation(program, "farPlane"))
{}

ShaderSsao::ShaderSsao(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader SSAO"),
	proj(glGetUniformLocation(program, "proj")),
	noiseScale(glGetUniformLocation(program, "noiseScale")),
	samples(glGetUniformLocation(program, "samples")),
	vposMap(glGetUniformLocation(program, "vposMap")),
	normMap(glGetUniformLocation(program, "normMap")),
	noiseMap(glGetUniformLocation(program, "noiseMap"))
{
	glUniform1i(vposMap, FrameSet::vposTexa - GL_TEXTURE0);
	glUniform1i(normMap, FrameSet::normTexa - GL_TEXTURE0);
	glUniform1i(noiseMap, noiseTexa - GL_TEXTURE0);

	std::uniform_real_distribution<float> realDist(0.f, 1.f);
	std::default_random_engine generator(Com::generateRandomSeed());
	vec3 kernel[sampleSize];
	for (int i = 0; i < sampleSize; ++i) {
		vec3 sample = glm::normalize(vec3(realDist(generator) * 2.f - 1.f, realDist(generator) * 2.f - 1.f, realDist(generator))) * realDist(generator);
		float scale = float(i) / float(sampleSize);
		kernel[i] = sample * glm::mix(0.1f, 1.f, scale * scale);
	}
	glUniform3fv(samples, sampleSize, reinterpret_cast<float*>(kernel));

	vec3 noise[noiseSize];
	for (uint i = 0; i < noiseSize; ++i)
		noise[i] = glm::normalize(vec3(realDist(generator) * 2.f - 1.f, realDist(generator) * 2.f - 1.f, 0.f));

	glActiveTexture(noiseTexa);
	glGenTextures(1, &texNoise);
	glBindTexture(GL_TEXTURE_2D, texNoise);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glActiveTexture(Texture::texa);
}

ShaderBlur::ShaderBlur(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader blur"),
	colorMap(glGetUniformLocation(program, "colorMap"))
{
	glUniform1i(colorMap, FrameSet::ssaoTexa - GL_TEXTURE0);
}

ShaderLight::ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, editSource(srcFrag, sets), "Shader light"),
	pview(glGetUniformLocation(program, "pview")),
	viewPos(glGetUniformLocation(program, "viewPos")),
	screenSize(glGetUniformLocation(program, "screenSize")),
	farPlane(glGetUniformLocation(program, "farPlane")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	lightAmbient(glGetUniformLocation(program, "lightAmbient")),
	lightDiffuse(glGetUniformLocation(program, "lightDiffuse")),
	lightLinear(glGetUniformLocation(program, "lightLinear")),
	lightQuadratic(glGetUniformLocation(program, "lightQuadratic")),
	colorMap(glGetUniformLocation(program, "colorMap")),
	normaMap(glGetUniformLocation(program, "normaMap")),
	depthMap(glGetUniformLocation(program, "depthMap")),
	ssaoMap(glGetUniformLocation(program, "ssaoMap"))
{
	glUniform1i(colorMap, TextureSet::colorTexa - GL_TEXTURE0);
	glUniform1i(normaMap, TextureSet::normalTexa - GL_TEXTURE0);
	glUniform1i(depthMap, depthTexa - GL_TEXTURE0);
	glUniform1i(ssaoMap, FrameSet::blurTexa - GL_TEXTURE0);
}

string ShaderLight::editSource(const string& src, const Settings* sets) {
	auto [out, ins] = splitGlobMain(src);
	if (sets->shadowRes)
		ins = std::regex_replace(ins, std::regex(R"r((float\s+shadow\s*=\s*)[\d\.]+(\s*;))r"), sets->softShadows ? "$1calcShadowSoft()$2" : "$1calcShadowHard()$2");
	if (sets->ssao)
		ins = std::regex_replace(ins, std::regex(R"r((float\s+ssao\s*=\s*)[\d\.]+(\s*;))r"), "$1getSsao()$2");
	if (!sets->bloom) {
		out = std::regex_replace(out, std::regex(R"r(layout\s*(\s*location\s*=\s*\d+\s*)\s*out\s+vec\d\s+brightColor\s*;)r"), "");
		ins = std::regex_replace(ins, std::regex(R"r(brightColor\s*=.+?;)r"), "");
	}
	if (out += ins; (sets->shadowRes || sets->ssao || !sets->bloom) && out == src)
		throw std::runtime_error("failed to edit shader light frag");
	return out;
}

ShaderGauss::ShaderGauss(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader gauss"),
	colorMap(glGetUniformLocation(program, "colorMap")),
	horizontal(glGetUniformLocation(program, "horizontal"))
{
	glUniform1i(colorMap, FrameSet::brightTexa - GL_TEXTURE0);
}

ShaderFinal::ShaderFinal(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader final"),
	sceneMap(glGetUniformLocation(program, "sceneMap")),
	bloomMap(glGetUniformLocation(program, "bloomMap"))
{
	glUniform1i(sceneMap, FrameSet::colorTexa - GL_TEXTURE0);
	glUniform1i(bloomMap, FrameSet::brightTexa - GL_TEXTURE0);
}

ShaderSkybox::ShaderSkybox(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, editSource(srcFrag, sets), "Shader skybox"),
	pview(glGetUniformLocation(program, "pview")),
	viewPos(glGetUniformLocation(program, "viewPos")),
	skyMap(glGetUniformLocation(program, "skyMap"))
{
	glUniform1i(skyMap, TextureSet::skyboxTexa - GL_TEXTURE0);
}

string ShaderSkybox::editSource(const string& src, const Settings* sets) {
	auto [out, ins] = splitGlobMain(src);
	if (!sets->bloom) {
		out = std::regex_replace(out, std::regex(R"r(layout\s*(\s*location\s*=\s*\d+\s*)\s*out\s+vec\d\s+brightColor\s*;)r"), "");
		ins = std::regex_replace(ins, std::regex(R"r(brightColor\s*=.+?;)r"), "");
	}
	if (out += ins; !sets->bloom && out == src)
		throw std::runtime_error("failed to edit shader light frag");
	return out;
}

ShaderGui::ShaderGui(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader GUI"),
	pview(glGetUniformLocation(program, "pview")),
	rect(glGetUniformLocation(program, "rect")),
	uvrc(glGetUniformLocation(program, "uvrc")),
	zloc(glGetUniformLocation(program, "zloc")),
	color(glGetUniformLocation(program, "color")),
	texsamp(glGetUniformLocation(program, "texsamp"))
{
	glUniform1i(texsamp, Texture::texa - GL_TEXTURE0);
}

// FONT SET

string FontSet::init(string name) {
	clear();
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
					throw std::runtime_error(string("Failed to find a font:") + linend + TTF_GetError());
			}
		}
	}
	int size;	// get approximate height scale factor
	heightScale = !TTF_SizeUTF8(tmp, fontTestString, nullptr, &size) ? float(FileSys::fontTestHeight) / float(size) : fallbackScale;
	TTF_CloseFont(tmp);
	return name;
}

TTF_Font* FontSet::findFile(const string& name, const vector<string>& available) {
	vector<string>::const_iterator it = std::find_if(available.begin(), available.end(), [name](const string& ent) -> bool { return !SDL_strcasecmp(name.c_str(), delExt(ent).c_str()); });
	return it != available.end() ? load(*it) : nullptr;
}

TTF_Font* FontSet::load(const string& name) {
	fontData = FileSys::loadFile(FileSys::dataPath() + name);
	return TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, FileSys::fontTestHeight);
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

	TTF_Font* font = TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, height);
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

bool FontSet::hasGlyph(uint16 ch) {
	if (!fonts.empty())
		return TTF_GlyphIsProvided(fonts.begin()->second, ch);

	if (TTF_Font* tmp = TTF_OpenFontRW(SDL_RWFromConstMem(fontData.data(), fontData.size()), SDL_TRUE, FileSys::fontTestHeight)) {
		bool yes = TTF_GlyphIsProvided(tmp, ch);
		TTF_CloseFont(tmp);
		return yes;
	}
	return false;
}

// LOADER

void WindowSys::Loader::addLine(const string& str, WindowSys* win) {
	logStr += str + '\n';
	if (uint lines = std::count(logStr.begin(), logStr.end(), '\n'), maxl = win->getScreenView().y / logSize; lines > maxl) {
		sizet end = 0;
		for (uint i = lines - maxl; i; --i)
			end = logStr.find_first_of('\n', end) + 1;
		logStr.erase(0, end);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (Texture tex = win->getFonts()->render(logStr.c_str(), logSize, win->getScreenView().x)) {
		glUseProgram(*win->getGui());
		glBindVertexArray(win->getGui()->wrect.getVao());
		Quad::draw(Rect(0, 0, tex.getRes()), vec4(1.f), tex);
		tex.free();
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
	cursorHeight = 18;

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
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), window);
		rc = EXIT_FAILURE;
#ifdef NDEBUG
	} catch (...) {
		std::cerr << "unknown error" << std::endl;
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

#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_SetHint(SDL_HINT_APP_NAME, "Thrones");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
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
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
#if SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_EventState(SDL_DISPLAYEVENT, SDL_DISABLE);
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
#endif
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
		sets->font = fonts->init(sets->font);
		createWindow();
		loader->addLine("loading audio", this);
		break;
	case Loader::State::audio:
		try {
			audio = new AudioSys(sets->avolume);
		} catch (const std::runtime_error& err) {
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
#ifdef __EMSCRIPTEN__
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
#if OPENGLES == 32
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#else
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sets->msamples != 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sets->msamples);
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
#ifndef __ANDROID__
	checkCurDisplay();
	checkResolution(sets->size, wsizes);
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
	}
#endif

	// create context and set up rendering
	if (context = SDL_GL_CreateContext(window); !context)
		throw std::runtime_error(string("failed to create context:") + linend + SDL_GetError());
	setSwapInterval();
#ifdef OPENGLES
	Texture::bgraSupported = SDL_GL_ExtensionSupported("GL_EXT_texture_format_BGRA8888");
#elif !defined(__APPLE__)
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
	if (glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, gvals); gvals[0] < largestObjTexture)
		throw std::runtime_error("texture size is limited to " + toStr(gvals[0]));
	if (glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, gvals); gvals[0] < 1 << Settings::shadowBitMax)
		throw std::runtime_error("cubemap size is limited to " + toStr(gvals[0]));
	if (glGetIntegerv(GL_MAX_VIEWPORT_DIMS, gvals); gvals[0] < std::max(wsizes.back().x, 1 << Settings::shadowBitMax) || gvals[1] < std::max(wsizes.back().y, 1 << Settings::shadowBitMax))
		throw std::runtime_error("viewport size is limited to " + toStr(gvals[0]) + 'x' + toStr(gvals[1]));
	if (glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, gvals); gvals[0] < std::max(wsizes.back().x, wsizes.back().y))
		throw std::runtime_error("renderbuffer size is limited to " + toStr(gvals[0]));
#ifndef OPENGLES
	if (glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, gvals); gvals[0] < std::max(wsizes.back().x, wsizes.back().y))
		throw std::runtime_error("texture size is limited to " + toStr(gvals[0]));
#endif
	if (glGetIntegerv(GL_MAX_TEXTURE_SIZE, gvals); gvals[0] < std::max(wsizes.back().x, wsizes.back().y))
		throw std::runtime_error("texture size is limited to " + toStr(gvals[0]));

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
	geom = new ShaderGeom(sources.at(ShaderGeom::fileVert), sources.at(ShaderGeom::fileFrag));
	depth = new ShaderDepth(sources.at(ShaderDepth::fileVert), sources.at(ShaderDepth::fileFrag));
	ssao = new ShaderSsao(sources.at(ShaderSsao::fileVert), sources.at(ShaderSsao::fileFrag));
	blur = new ShaderBlur(sources.at(ShaderBlur::fileVert), sources.at(ShaderBlur::fileFrag));
	light = new ShaderLight(sources.at(ShaderLight::fileVert), sources.at(ShaderLight::fileFrag), sets);
	gauss = new ShaderGauss(sources.at(ShaderGauss::fileVert), sources.at(ShaderGauss::fileFrag));
	sfinal = new ShaderFinal(sources.at(ShaderFinal::fileVert), sources.at(ShaderFinal::fileFrag));
	skybox = new ShaderSkybox(sources.at(ShaderSkybox::fileVert), sources.at(ShaderSkybox::fileFrag), sets);
	gui = new ShaderGui(sources.at(ShaderGui::fileVert), sources.at(ShaderGui::fileFrag));

	// init startup log
	glActiveTexture(Texture::texa);
	glUseProgram(*gui);
	glUniform2f(gui->pview, float(screenView.x) / 2.f, float(screenView.y) / 2.f);
}

void WindowSys::destroyWindow() {
	delete gui;
	delete skybox;
	delete sfinal;
	delete gauss;
	delete light;
	delete blur;
	delete ssao;
	delete depth;
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
	glViewport(0, 0, screenView.x, screenView.y);
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
	scene->reloadShader();
	scene->resetShadows();
	sets->font = fonts->init(sets->font);
	checkCurDisplay();
	setSwapInterval();
	SDL_SetWindowBrightness(window, sets->gamma);
	setScreen();
}

void WindowSys::reloadVaryingShaders() {
	delete skybox;
	delete light;
	umap<string, string> sources = FileSys::loadShaders();
	light = new ShaderLight(sources.at(ShaderLight::fileVert), sources.at(ShaderLight::fileFrag), sets);
	skybox = new ShaderSkybox(sources.at(ShaderSkybox::fileVert), sources.at(ShaderSkybox::fileFrag), sets);
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

#if !defined(NDEBUG) && !defined(__APPLE__) && (!defined(OPENGLES) || OPENGLES == 32)
#ifdef _WIN32
void APIENTRY WindowSys::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
#else
void WindowSys::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
#endif
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;
	std::cout << "Debug message " << id << ": " <<  message;
	if (size_t mlen = strlen(message); mlen && message[mlen-1] != '\n')
		std::cout << linend;

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		std::cout << "Source: API, ";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::cout << "Source: window system, ";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::cout << "Source: shader compiler, ";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::cout << "Source: third party, ";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::cout << "Source: application, ";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		std::cout << "Source: other, ";
		break;
	default:
		std::cout << "Source: unknown, ";
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "Type: error, ";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "Type: deprecated behavior, ";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "Type: undefined behavior, ";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "Type: portability, ";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "Type: performance, ";
		break;
	case GL_DEBUG_TYPE_MARKER:
		std::cout << "Type: marker, ";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::cout << "Type: push group, ";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::cout << "Type: pop group, ";
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "Type: other, ";
		break;
	default:
		std::cout << "Type: unknown, ";
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "Severity: high" << std::endl;
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "Severity: medium" << std::endl;
		break;
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "Severity: low" << std::endl;
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << "Severity: notification" << std::endl;
		break;
	default:
		std::cout << "Severity: unknown" << std::endl;
	}
}
#endif
