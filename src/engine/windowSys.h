#pragma once

#include "utils/widgets.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#if defined(__ANDROID__) || defined(_WIN32)
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif
#endif

// for drawing
class Shader {
public:
	static constexpr GLuint vpos = 0, normal = 1, uvloc = 2, tangent = 3;
	static constexpr GLuint model0 = 4, model1 = 5, model2 = 6, model3 = 7;
	static constexpr GLuint normat0 = 8, normat1 = 9, normat2 = 10;
	static constexpr GLuint diffuse = 11, specShine = 12, texid = 13, show = 14;

protected:
	GLuint program;

public:
	Shader(const string& srcVert, const string& srcFrag, const char* name);
	~Shader();

	operator GLuint() const;

protected:
	static pairStr splitGlobMain(const string& src);

private:
	static GLuint loadShader(const string& source, GLenum type, const char* name);
	template <class C, class I> static void checkStatus(GLuint id, GLenum stat, C check, I info, const string& name);
};

inline Shader::~Shader() {
	glDeleteProgram(program);
}

inline Shader::operator GLuint() const {
	return program;
}

class ShaderGeom : public Shader {
public:
	static constexpr char fileVert[] = "geom.vert";
	static constexpr char fileFrag[] = "geom.frag";

	GLint proj, view;

	ShaderGeom(const string& srcVert, const string& srcFrag);
};

class ShaderDepth : public Shader {
public:
	static constexpr char fileVert[] = "depth.vert";
	static constexpr char fileFrag[] = "depth.frag";

	GLint pvTrans, pvId;
	GLint lightPos, farPlane;

	ShaderDepth(const string& srcVert, const string& srcFrag);
};

class ShaderSsao : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "ssao.frag";
	static constexpr GLenum noiseTexa = GL_TEXTURE11;

	GLint proj, noiseScale, samples;
	GLint vposMap, normMap, noiseMap;
private:
	GLuint texNoise;

	static constexpr int sampleSize = 64;
	static constexpr uint noiseSize = 16;

public:
	ShaderSsao(const string& srcVert, const string& srcFrag);
	~ShaderSsao();
};

inline ShaderSsao::~ShaderSsao() {
	glDeleteTextures(1, &texNoise);
}

class ShaderBlur : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "blur.frag";

	GLint colorMap;

	ShaderBlur(const string& srcVert, const string& srcFrag);
};

class ShaderLight : public Shader {
public:
	static constexpr char fileVert[] = "light.vert";
	static constexpr char fileFrag[] = "light.frag";
	static constexpr GLenum depthTexa = GL_TEXTURE10;

	GLint pview, viewPos;
	GLint screenSize, farPlane;
	GLint lightPos, lightAmbient, lightDiffuse, lightLinear, lightQuadratic;
	GLint colorMap, normaMap, depthMap, ssaoMap;

	ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets);

private:
	static string editSource(const string& src, const Settings* sets);
};

class ShaderGauss : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "gauss.frag";

	GLuint colorMap, horizontal;

	ShaderGauss(const string& srcVert, const string& srcFrag);
};

class ShaderFinal : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "final.frag";

	GLuint sceneMap, bloomMap;

	ShaderFinal(const string& srcVert, const string& srcFrag);
};

class ShaderSkybox : public Shader {
public:
	static constexpr char fileVert[] = "skybox.vert";
	static constexpr char fileFrag[] = "skybox.frag";

	GLint pview, viewPos;
	GLuint skyMap;

	ShaderSkybox(const string& srcVert, const string& srcFrag, const Settings* sets);

private:
	static string editSource(const string& src, const Settings* sets);
};

class ShaderGui : public Shader {
public:
	static constexpr char fileVert[] = "gui.vert";
	static constexpr char fileFrag[] = "gui.frag";

	GLint pview, rect, uvrc, zloc;
	GLint color, texsamp;
	Quad wrect;

	ShaderGui(const string& srcVert, const string& srcFrag);
};

// loads different font sizes from one font and handles basic log display
class FontSet {
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
	static constexpr float fallbackScale = 0.9f;
	static constexpr SDL_Color textColor = { 255, 193, 37, 255 };

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

		enum class State : uint8 {
			start,
			audio,
			objects,
			textures,
			program,
			done
		};

		string logStr;
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
	ShaderGauss* gauss;
	ShaderFinal* sfinal;
	ShaderSkybox* skybox;
	ShaderGui* gui;
	FontSet* fonts;
	ivec2 screenView, guiView;
	uint32 oldTime;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue
	uint8 cursorHeight;
#ifdef __EMSCRIPTEN__
	uptr<Loader> loader;
	void (WindowSys::*loopFunc)();
#endif

public:
	int start(const Arguments& args);
	void close();

	ivec2 getScreenView() const;
	ivec2 getGuiView() const;
	uint8 getCursorHeight() const;
	vector<ivec2> windowSizes() const;
	vector<SDL_DisplayMode> displayModes() const;
	int displayID() const;
	void setTextCapture(bool on);
	void setScreen();
	void setVsync(Settings::VSync vsync);
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
	const ShaderGauss* getGauss() const;
	const ShaderFinal* getSfinal() const;
	const ShaderSkybox* getSkybox() const;
	const ShaderGui* getGui() const;
	SDL_Window* getWindow();
	float getDeltaSec() const;

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
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void setSwapInterval();
	bool trySetSwapInterval();
	void setWindowMode();
	void updateView();
	bool checkCurDisplay();
	template <class T> static void checkResolution(T& val, const vector<T>& modes);
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
