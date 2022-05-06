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

// startup loading information and rendering
class Loader {
public:
	enum class State : uint8 {
		start,
		audio,
		objects,
		textures,
		program,
		done
	};

	State state = State::start;

private:
	static constexpr int logSize = 18;
	static constexpr ivec2 logMargin = ivec2(8, 4);

	string logStr;
	ShaderStartup* shader;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	GLuint blank = 0, title = 0;
	ivec2 titleRes = ivec2(0);
#endif
	GLuint text = 0;
	GLuint vao = 0;

public:
	void init(ShaderStartup* stlog, WindowSys* win);
	void free();
	void addLine(string_view str, WindowSys* win);
private:
	void draw(const Rect& rect, GLuint tex);
	template <GLenum format, GLenum type> static void uploadTexture(GLuint tex, const void* pixels, ivec2 res, int rowLen);
};

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

public:
	~FontSet();

	string init(string name, Settings::Hinting hint);	// returns name or a chosen fallback name
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	void clear();
#endif
	int length(const char* text, int height);
	int length(const string& text, int height);
	int length(char* text, int height, sizet length);
	bool hasGlyph(uint16 ch);
	void setHinting(Settings::Hinting hinting);
	SDL_Surface* render(const char* text, int height);
	SDL_Surface* render(const string& text, int height);
	SDL_Surface* render(const char* text, int height, uint length);
	SDL_Surface* render(const string& text, int height, uint length);

private:
	TTF_Font* load(const string& name);
	TTF_Font* findFile(const string& name, const vector<string>& available);
	TTF_Font* getFont(int height);
};

inline FontSet::~FontSet() {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
#endif
}

inline int FontSet::length(const string& text, int height) {
	return length(text.c_str(), height);
}

inline SDL_Surface* FontSet::render(const char* text, int height) {
	return TTF_RenderUTF8_Blended(getFont(height), text, textColor);
}

inline SDL_Surface* FontSet::render(const string& text, int height) {
	return TTF_RenderUTF8_Blended(getFont(height), text.c_str(), textColor);
}

inline SDL_Surface* FontSet::render(const char* text, int height, uint length) {
	return TTF_RenderUTF8_Blended_Wrapped(getFont(height), text, textColor, length);
}

inline SDL_Surface* FontSet::render(const string& text, int height, uint length) {
	return TTF_RenderUTF8_Blended_Wrapped(getFont(height), text.c_str(), textColor, length);
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
	static constexpr IMG_InitFlags imgInitFlags = IMG_INIT_JPG | IMG_INIT_PNG;
	static constexpr int largestObjTexture = 1024;
#else
	static constexpr IMG_InitFlags imgInitFlags = IMG_INIT_PNG;
	static constexpr int largestObjTexture = 2048;
#endif

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
	ShaderBlur* mblur;
	ShaderSsr* ssr;
	ShaderSsrColor* ssrColor;
	ShaderBlur* cblur;
	ShaderLight* light;
	ShaderBrights* brights;
	ShaderGauss* gauss;
	ShaderFinal* sfinal;
	ShaderSkybox* skybox;
	ShaderGui* gui;
	FontSet* fonts;
#ifdef OPENVR
	VrSys* vrSys;
#endif
	array<GLuint, 2> samplers;	// 0: 2d frame, 1: 3d texture
	ivec2 screenView, guiView, windowView;
	int titleBarHeight;
	uint maxMsamples;
	uint cursorHeight;
	uint32 oldTime;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue
#ifdef __EMSCRIPTEN__
	uptr<Loader> loader;
	void (WindowSys::*loopFunc)();
#endif

public:
	int start(const Arguments& args);
	void close();

	ivec2 getScreenView() const;
	ivec2 getGuiView() const;
	ivec2 getWindowView() const;
	int getTitleBarHeight() const;
	uint getCursorHeight() const;
	ivec2 mousePos();
	int displayID() const;
	void setTextCapture(Rect* field);
	void setScreen();
	void setSwapInterval();
	void setMultisampling();
	void setGamma(float gamma);
	void setShaderOptions();
	void resetSettings();

	AudioSys* getAudio();
	FontSet* getFonts();
	InputSys* getInput();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();
#ifdef OPENVR
	VrSys* getVr();
#else
	constexpr VrSys* getVr() const;
#endif
	const ShaderGeom* getGeom() const;
	const ShaderDepth* getDepth() const;
	const ShaderSsao* getSsao() const;
	const ShaderBlur* getMblur() const;
	const ShaderSsr* getSsr() const;
	const ShaderSsrColor* getSsrColor() const;
	const ShaderBlur* getCblur() const;
	const ShaderLight* getLight() const;
	const ShaderBrights* getBrights() const;
	const ShaderGauss* getGauss() const;
	const ShaderFinal* getSfinal() const;
	const ShaderSkybox* getSkybox() const;
	const ShaderGui* getGui() const;
	SDL_Window* getWindow();
	float getDeltaSec() const;
	Settings::AntiAliasing maxAntiAliasing() const;

private:
	void init(const Arguments& args);
	void exec();
#ifdef __EMSCRIPTEN__
	void load();
#else
	void load(Loader* loader);
#endif
	ShaderStartup* createWindow();
	void destroyWindow();
	uint32 getWindowFlags() const;
	template <GLint min, GLint mag, GLint wrap, int lod = 1000> static void setSampler(GLuint sampler);
	void handleEvent(const SDL_Event& event);
	void eventWindow(const SDL_WindowEvent& winEvent);
	static SDL_HitTestResult SDLCALL eventWindowHit(SDL_Window* win, const SDL_Point* area, void* data);
	static SDL_HitTestResult SDLCALL eventWindowHitStartup(SDL_Window* win, const SDL_Point* area, void* data);
	bool trySetSwapInterval();
	void updateView();
	void setTitleBarHeight();	// must be followed by updateView() to update the viewport
	void setTitleBarHitTest();
	template <class T> static void checkResolution(T& val, const vector<T>& modes);
	static uint estimateSystemCursorSize();
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

inline ivec2 WindowSys::getGuiView() const {
	return guiView;
}

inline ivec2 WindowSys::getWindowView() const {
	return windowView;
}

inline int WindowSys::getTitleBarHeight() const {
	return titleBarHeight;
}

inline uint WindowSys::getCursorHeight() const {
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

#ifdef OPENVR
inline VrSys* WindowSys::getVr() {
	return vrSys;
#else
constexpr VrSys* WindowSys::getVr() const {
	return nullptr;
#endif
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

inline const ShaderBlur* WindowSys::getMblur() const {
	return mblur;
}

inline const ShaderSsr* WindowSys::getSsr() const {
	return ssr;
}

inline const ShaderSsrColor* WindowSys::getSsrColor() const {
	return ssrColor;
}

inline const ShaderBlur* WindowSys::getCblur() const {
	return cblur;
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

inline SDL_Window* WindowSys::getWindow() {
	return window;
}

inline float WindowSys::getDeltaSec() const {
	return dSec;
}

inline int WindowSys::displayID() const {
	return SDL_GetWindowDisplayIndex(window);
}
