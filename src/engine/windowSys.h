#pragma once

#include "utils/widgets.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#if defined(__ANDROID__) || defined(_WIN32) || defined(APPIMAGE)
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif
#endif

// for drawing
class Shader {
public:
	static constexpr GLuint vpos = 0, normal = 1, uvloc = 2;

protected:
	GLuint program;

public:
	Shader(const string& srcVert, const string& srcFrag);
#ifndef OPENGLES
	Shader(const string& srcVert, const string& srcGeom, const string& srcFrag);
#endif
	~Shader();

	operator GLuint() const;

private:
	static GLuint loadShader(const string& source, GLenum type);
	template <class C, class I> static void checkStatus(GLuint id, GLenum stat, C check, I info);
};

inline Shader::~Shader() {
	glDeleteProgram(program);
}

inline Shader::operator GLuint() const {
	return program;
}

class ShaderGeometry : public Shader {
public:
	GLint pview, model, normat;
	GLint viewPos, farPlane, texsamp, depthMap;
	GLint materialDiffuse, materialSpecular, materialShininess;
	GLint lightPos, lightAmbient, lightDiffuse, lightLinear, lightQuadratic;

	ShaderGeometry(const string& srcVert, const string& srcFrag, const Settings* sets);

private:
	static string editShadowAlg(string src, bool calc, bool soft);
};

#ifndef OPENGLES
class ShaderDepth : public Shader {
public:
	GLint model;
	GLint shadowMats;
	GLint lightPos, farPlane;

	ShaderDepth(const string& srcVert, const string& srcGeom, const string& srcFrag);
};
#endif

class ShaderGui : public Shader {
public:
	GLint pview, rect, uvrc, zloc;
	GLint color, texsamp;
	Quad wrect;

	ShaderGui(const string& srcVert, const string& srcFrag);
};

// loads different font sizes from one font and handles basic log display
class FontSet {
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
	static constexpr int fontTestHeight = 100;
	static constexpr float fallbackScale = 0.9f;
#ifdef OPENGLES
	static constexpr SDL_Color textColor = { 37, 193, 255, 255 };	// R and B need to be switched
#else
	static constexpr SDL_Color textColor = { 255, 193, 37, 255 };
#endif

	umap<int, TTF_Font*> fonts;
	vector<uint8> fontData;
	float heightScale;	// for scaling down font size to fit requested height

public:
	~FontSet();

	string init(string name);	// returns name or a chosen fallback name
	void clear();
	int length(const char* text, int height);
	bool hasGlyph(uint16 ch);
	Texture render(const char* text, int height);
	Texture render(const char* text, int height, uint length);

private:
	TTF_Font* load(const string& name);
	TTF_Font* findFile(const string& name, const vector<string>& available);
	TTF_Font* getFont(int height);
};

inline FontSet::~FontSet() {
	clear();
}

inline Texture FontSet::render(const char* text, int height) {
	return TTF_RenderUTF8_Blended(getFont(height), text, textColor);
}

inline Texture FontSet::render(const char* text, int height, uint length) {
	return TTF_RenderUTF8_Blended_Wrapped(getFont(height), text, textColor, length);
}

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "Thrones";
private:
	static constexpr char fileCursor[] = "cursor.png";
	static constexpr char fileGeometryVert[] = "geometry.vert";
	static constexpr char fileGeometryFrag[] = "geometry.frag";
	static constexpr char fileDepthVert[] = "depth.vert";
	static constexpr char fileDepthGeom[] = "depth.geom";
	static constexpr char fileDepthFrag[] = "depth.frag";
	static constexpr char fileGuiVert[] = "gui.vert";
	static constexpr char fileGuiFrag[] = "gui.frag";

	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;
#ifdef OPENGLES
	static constexpr int imgInitFlags = imgInitFull;
#else
	static constexpr int imgInitFlags = IMG_INIT_PNG;
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

		enum class State : uint8 {
			start,
			audio,
			objects,
			textures,
			program,
			done
		};

		string logStr;
		State state;

		Loader();

		void addLine(const string& str, WindowSys* win);
	};

	AudioSys* audio;
	InputSys* inputSys;
	Program* program;
	Scene* scene;
	Settings* sets;
	SDL_Window* window;
	SDL_GLContext context;
	ShaderGeometry* geom;
#ifndef OPENGLES
	ShaderDepth* depth;
#endif
	ShaderGui* gui;
	FontSet* fonts;
	ivec2 screenView, guiView;
	uint32 oldTime;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue
	uint8 cursorHeight;
#ifdef EMSCRIPTEN
	uptr<Loader> loader;
	void (WindowSys::*loopFunc)();
#endif

public:
	int start(const Arguments& args);
	void close();

	const ivec2& getScreenView() const;
	const ivec2& getGuiView() const;
	uint8 getCursorHeight() const;
	vector<ivec2> windowSizes() const;
	vector<SDL_DisplayMode> displayModes() const;
	int displayID() const;
	void setTextCapture(bool on);
	void setScreen();
	void setVsync(Settings::VSync vsync);
	void setGamma(float gamma);
	void resetSettings();
	void reloadGeom();

	AudioSys* getAudio();
	FontSet* getFonts();
	InputSys* getInput();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();
	const ShaderGeometry* getGeom() const;
#ifndef OPENGLES
	const ShaderDepth* getDepth() const;
#endif
	const ShaderGui* getGui() const;
	SDL_Window* getWindow();
	float getDeltaSec() const;

private:
	void init(const Arguments& args);
	void exec();
#ifdef EMSCRIPTEN
	void load();
#else
	void load(Loader* loader);
#endif
	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void setSwapInterval();
	bool trySetSwapInterval();
	void setWindowMode();
	void updateView();
	bool checkCurDisplay();
	template <class T> static void checkResolution(T& val, const vector<T>& modes);
};

inline WindowSys::Loader::Loader() :
	state(State::start)
{}

inline void WindowSys::close() {
	run = false;
}

inline const ivec2& WindowSys::getScreenView() const {
	return screenView;
}

inline const ivec2& WindowSys::getGuiView() const {
	return guiView;
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

inline const ShaderGeometry* WindowSys::getGeom() const {
	return geom;
}

#ifndef OPENGLES
inline const ShaderDepth* WindowSys::getDepth() const {
	return depth;
}
#endif

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
