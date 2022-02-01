#pragma once

#include "utils/settings.h"
#include "utils/utils.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#if defined(__ANDROID__) || defined(_WIN32)
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif
#endif

// vertex data for widgets
class Quad {
public:
	static constexpr uint corners = 4;
private:
	static constexpr uint stride = 2;
	static constexpr float vertices[corners * stride] = {
		0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,
		1.f, 1.f
	};

	GLuint vao, vbo;

public:
	void init();
	void free();

	GLuint getVao() const;
	static void draw(const ShaderGui* sgui, const Rect& rect, const vec4& color, GLuint tex, float z = 0.f);
	static void draw(const ShaderGui* sgui, const Rect& rect, const Rect& frame, const vec4& color, GLuint tex, float z = 0.f);
	static void draw(const ShaderGui* sgui, const Rect& rect, const vec4& uvrect, const vec4& color, GLuint tex, float z = 0.f);
};

inline GLuint Quad::getVao() const {
	return vao;
}

inline void Quad::draw(const ShaderGui* sgui, const Rect& rect, const vec4& color, GLuint tex, float z) {
	draw(sgui, rect, vec4(0.f, 0.f, 1.f, 1.f), color, tex, z);
}

inline void Quad::draw(const ShaderGui* sgui, const Rect& rect, const Rect& frame, const vec4& color, GLuint tex, float z) {
	Rect isct = rect.intersect(frame);
	draw(sgui, isct, vec4(float(isct.x - rect.x) / float(rect.w), float(isct.y - rect.y) / float(rect.h), float(isct.w) / float(rect.w), float(isct.h) / float(rect.h)), color, tex, z);
}

// loads different font sizes from one font and handles basic log display
class FontSet {
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
	static constexpr float fallbackScale = 0.9f;
#ifdef OPENGLES
	static constexpr SDL_Color textColor = { 37, 193, 255, 255 };	// don't wanna deal with OpenGL ES's pixel format bullshit and extension checks are fucked on Android, thus simply invert that shit
#else
	static constexpr SDL_Color textColor = { 255, 193, 37, 255 };
#endif

#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_Font* font = nullptr;
#else
	umap<int, TTF_Font*> fonts;
	vector<uint8> fontData;
	Settings::Hinting hinting;
#endif
	float heightScale;	// for scaling down font size to fit requested height
	int maxTexSize;

public:
	~FontSet();

	string init(string name, Settings::Hinting hint);	// returns name or a chosen fallback name
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	void clear();
#endif
	int length(const char* text, int height);
	int length(char* text, int height, sizet length);
	bool hasGlyph(uint16 ch);
	void setHinting(Settings::Hinting hinting);
	int getMaxTexSize() const;
	void setMaxTexSize(int texLimit, int winLimit);
	Texture render(const char* text, int height);
	Texture render(const char* text, int height, SDL_Surface*& img, ivec2 offset);
	Texture render(const char* text, int height, uint length);
	Texture render(const char* text, int height, uint length, SDL_Surface*& img, ivec2 offset);

private:
	TTF_Font* load(const string& name);
	TTF_Font* findFile(const string& name, const vector<string>& available);
	TTF_Font* getFont(int height);
	Texture finishRender(SDL_Surface* img);
};

inline FontSet::~FontSet() {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
#endif
}

inline int FontSet::getMaxTexSize() const {
	return maxTexSize;
}

inline void FontSet::setMaxTexSize(int texLimit, int winLimit) {
	maxTexSize = std::min(texLimit, int(ceilPower2(winLimit)));
}

inline Texture FontSet::render(const char* text, int height) {
	return finishRender(TTF_RenderUTF8_Blended(getFont(height), text, textColor));
}

inline Texture FontSet::render(const char* text, int height, uint length) {
	return finishRender(TTF_RenderUTF8_Blended_Wrapped(getFont(height), text, textColor, length));
}

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "Thrones";
private:
	static constexpr char fileIcon[] = "thrones.png";
	static constexpr char fileCursor[] = "cursor.png";

	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;
#ifdef OPENGLES
	static constexpr int largestObjTexture = 1024;
#else
	static constexpr int largestObjTexture = 2048;
#endif
	static constexpr float minimumRatio = 1.4f;
	static constexpr array<ivec2, 26> resolutions = {
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

	struct Loader {
		static constexpr int logSize = 18;
		static constexpr ivec2 logMargin = ivec2(8, 4);

		enum class State : uint8 {
			start,
			audio,
			objects,
			textures,
			program,
			done
		};

		string logStr;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		Texture blank, title;
#endif
		State state = State::start;

		void addLine(const string& str, WindowSys* win);
	};

	AudioSys* audio;
	InputSys* inputSys;
	Program* program;
	Scene* scene;
	Settings* sets;
	SDL_Window* window;
	SDL_GLContext context;
	ShaderGeom* geom;
	ShaderDepth* depth;
	ShaderSsao* ssao;
	ShaderBlur* blur;
	ShaderLight* light;
	ShaderBrights* brights;
	ShaderGauss* gauss;
	ShaderFinal* sfinal;
	ShaderSkybox* skybox;
	ShaderGui* gui;
	FontSet* fonts;
	Quad wrect;
	ivec2 screenView;
	int windowHeight, titleBarHeight;
	uint32 oldTime;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue
	uint8 cursorHeight;
	uint8 maxMsamples;
#ifdef __EMSCRIPTEN__
	uptr<Loader> loader;
	void (WindowSys::*loopFunc)();
#endif

public:
	int start(const Arguments& args);
	void close();

	ivec2 getScreenView() const;
	int getTitleBarHeight() const;
	uint8 getCursorHeight() const;
	ivec2 mousePos();
	vector<ivec2> windowSizes() const;
	vector<SDL_DisplayMode> displayModes() const;
	int displayID() const;
	void setTextCapture(Rect* field);
	void setScreen();
	void setSwapInterval();
	void setMultisampling();
	void setGamma(float gamma);
	void resetSettings();
	void reloadVaryingShaders();

	AudioSys* getAudio();
	FontSet* getFonts();
	InputSys* getInput();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();
	const ShaderGeom* getGeom() const;
	const ShaderDepth* getDepth() const;
	const ShaderSsao* getSsao() const;
	const ShaderBlur* getBlur() const;
	const ShaderLight* getLight() const;
	const ShaderBrights* getBrights() const;
	const ShaderGauss* getGauss() const;
	const ShaderFinal* getSfinal() const;
	const ShaderSkybox* getSkybox() const;
	const ShaderGui* getGui() const;
	const Quad* getWrect() const;
	SDL_Window* getWindow();
	float getDeltaSec() const;
	uint8 getMaxMsamples() const;

private:
	void init(const Arguments& args);
	void exec();
#ifdef __EMSCRIPTEN__
	void load();
#else
	void load(Loader* loader);
#endif
	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);
	void eventWindow(const SDL_WindowEvent& winEvent);
	static SDL_HitTestResult SDLCALL eventWindowHit(SDL_Window* win, const SDL_Point* area, void* data);
	bool trySetSwapInterval();
	void setWindowMode();
	void updateView();
	void setTitleBarHeight();	// must be followed by updateView() to update the viewport
	bool checkCurDisplay();
	template <class T> static void checkResolution(T& val, const vector<T>& modes);
	static int estimateSystemCursorSize();
#if !defined(NDEBUG) && !defined(__APPLE__) && (!defined(OPENGLES) || OPENGLES == 32)
#ifdef _WIN32
	static void __stdcall debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
#else
	static void debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
#endif
#endif
};

inline void WindowSys::close() {
	run = false;
}

inline ivec2 WindowSys::getScreenView() const {
	return screenView;
}

inline int WindowSys::getTitleBarHeight() const {
	return titleBarHeight;
}

inline uint8 WindowSys::getCursorHeight() const {
	return cursorHeight;
}

inline AudioSys* WindowSys::getAudio() {
	return audio;
}

inline FontSet* WindowSys::getFonts() {
	return fonts;
}

inline InputSys* WindowSys::getInput() {
	return inputSys;
}

inline Program* WindowSys::getProgram() {
	return program;
}

inline Scene* WindowSys::getScene() {
	return scene;
}

inline Settings* WindowSys::getSets() {
	return sets;
}

inline const ShaderGeom* WindowSys::getGeom() const {
	return geom;
}

inline const ShaderDepth* WindowSys::getDepth() const {
	return depth;
}

inline const ShaderSsao* WindowSys::getSsao() const {
	return ssao;
}

inline const ShaderBlur* WindowSys::getBlur() const {
	return blur;
}

inline const ShaderLight* WindowSys::getLight() const {
	return light;
}

inline const ShaderBrights* WindowSys::getBrights() const {
	return brights;
}

inline const ShaderGauss* WindowSys::getGauss() const {
	return gauss;
}

inline const ShaderFinal* WindowSys::getSfinal() const {
	return sfinal;
}

inline const ShaderSkybox* WindowSys::getSkybox() const {
	return skybox;
}

inline const ShaderGui* WindowSys::getGui() const {
	return gui;
}

inline const Quad* WindowSys::getWrect() const {
	return &wrect;
}

inline SDL_Window* WindowSys::getWindow() {
	return window;
}

inline float WindowSys::getDeltaSec() const {
	return dSec;
}

inline uint8 WindowSys::getMaxMsamples() const {
	return maxMsamples;
}

inline int WindowSys::displayID() const {
	return SDL_GetWindowDisplayIndex(window);
}
