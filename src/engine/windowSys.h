#pragma once

#include "audioSys.h"
#include "scene.h"
#include "prog/program.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif

// loads different font sizes from one font and handles basic log display
class FontSet {
private:
	static constexpr char fileFont[] = "data/romanesque.ttf";
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
	static constexpr int fontTestHeight = 100;
	static constexpr int logSize = 18;
	static constexpr float fallbackScale = 0.9f;
	static constexpr SDL_Color textColor = { 255, 193, 37, 255 };
	static constexpr SDL_Color logColor = { 230, 220, 220, 220 };

	float heightScale;	// for scaling down font size to fit requested height
	umap<int, TTF_Font*> fonts;
	vector<uint8> fontData;

	TTF_Font* logFont;
	Texture logTex;
	vector<string> logLines;

public:
	FontSet();
	~FontSet();

	void clear();
	int length(const string& text, int height);
	Texture render(const string& text, int height);
	Texture render(const string& text, int height, uint length);

	void writeLog(string&& text, const ivec2& res);
	void closeLog();	// doesn't get called in destructor

private:
	TTF_Font* getFont(int height);
};

inline FontSet::~FontSet() {
	clear();
}

inline Texture FontSet::render(const string& text, int height) {
	return TTF_RenderUTF8_Blended(getFont(height), text.c_str(), textColor);
}

inline Texture FontSet::render(const string& text, int height, uint length) {
	return TTF_RenderUTF8_Blended_Wrapped(getFont(height), text.c_str(), textColor, length);
}

// for drawing
class Shader {
public:
	GLuint program;

public:
	Shader(const string& srcVert, const string& srcFrag);
	~Shader();

private:
	static GLuint loadShader(const string& source, GLenum type);
};

inline Shader::~Shader() {
	glDeleteProgram(program);
}

class ShaderGeometry : public Shader {
public:
	GLint pview, model, normat, vertex, uvloc, normal, texsamp, viewPos;
	GLint materialDiffuse, materialSpecular, materialShininess, materialAlpha;
	GLint lightPos, lightAmbient, lightDiffuse, lightLinear, lightQuadratic;

public:
	ShaderGeometry(const string& srcVert, const string& srcFrag);
};

class ShaderGUI : public Shader {
public:
	GLint pview, rect, uvrc, zloc, vertex, uvloc;
	GLint color, texsamp;
private:
	Shape wrect;

public:
	ShaderGUI(const string& srcVert, const string& srcFrag);
	~ShaderGUI();

	void bindRect() const;
};

inline void ShaderGUI::bindRect() const {
	glBindVertexArray(wrect.vao);
}

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "Thrones";
private:
	static constexpr char fileIcon[] = "data/thrones.bmp";
	static constexpr char fileCursor[] = "data/cursor.bmp";
	static constexpr char fileSceneVert[] = "geometry.vert";
	static constexpr char fileSceneFrag[] = "geometry.frag";
	static constexpr char fileGuiVert[] = "gui.vert";
	static constexpr char fileGuiFrag[] = "gui.frag";

	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;
	static constexpr uint8 fallbackCursorSize = 18;
	
	uptr<AudioSys> audio;
	uptr<Program> program;
	uptr<Scene> scene;
	uptr<Settings> sets;
	SDL_Window* window;
	SDL_GLContext context;
	uptr<ShaderGeometry> geom;
	uptr<ShaderGUI> gui;
	uptr<FontSet> fonts;
	ivec2 curView;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue
	SDL_Cursor* cursor;
	uint8 cursorHeight;

public:
	WindowSys();

	int start();
	void close();

	float getDSec() const;
	ivec2 getView() const;
	uint8 getCursorHeight() const;
	vector<ivec2> displaySizes() const;
	vector<SDL_DisplayMode> displayModes() const;
	int displayID() const;
	uint32 windowID() const;
	void writeLog(string&& text);
	void setScreen(uint8 display, Settings::Screen screen, const ivec2& size, const SDL_DisplayMode& mode);
	void setVsync(Settings::VSync vsync);
	void setGamma(float gamma);
	void resetSettings();

	AudioSys* getAudio();
	FontSet* getFonts();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();
	ShaderGeometry* getGeom();
	ShaderGUI* getGUI();

private:
	void init();
	void exec();
	void cleanup();

	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void setSwapInterval();
	bool trySetSwapInterval();
	void setWindowMode();

	void updateViewport();
	bool checkCurDisplay();
	template <class T> static bool checkResolution(T& val, const vector<T>& modes);
};

inline void WindowSys::close() {
	run = false;
}

inline float WindowSys::getDSec() const {
	return dSec;
}

inline ivec2 WindowSys::getView() const {
	return curView;
}

inline uint8 WindowSys::getCursorHeight() const {
	return cursorHeight;
}

inline AudioSys* WindowSys::getAudio() {
	return audio.get();
}

inline FontSet* WindowSys::getFonts() {
	return fonts.get();
}

inline Program* WindowSys::getProgram() {
	return program.get();
}

inline Scene* WindowSys::getScene() {
	return scene.get();
}

inline Settings* WindowSys::getSets() {
	return sets.get();
}

inline ShaderGeometry* WindowSys::getGeom() {
	return geom.get();
}

inline ShaderGUI* WindowSys::getGUI() {
	return gui.get();
}

inline int WindowSys::displayID() const {
	return SDL_GetWindowDisplayIndex(window);
}

inline uint32 WindowSys::windowID() const {
	return SDL_GetWindowID(window);
}

template <class T>
bool WindowSys::checkResolution(T& val, const vector<T>& modes) {
	typename vector<T>::const_iterator it;
	if (it = std::find(modes.begin(), modes.end(), val); it != modes.end() || modes.empty())
		return true;

	for (it = modes.begin(); it != modes.end() && *it < val; it++);
	val = it == modes.begin() ? *it : *(it - 1);
	return false;
}
