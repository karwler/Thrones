#pragma once

#include "fileSys.h"
#include "scene.h"
#include "prog/program.h"
#include "utils/layouts.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif

// loads different font sizes from one file
class FontSet {
public:
	static constexpr int fontTestHeight = 100;
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
	static constexpr char fileFont[] = "Merriweather.otf";

	float heightScale;	// for scaling down font size to fit requested height
	umap<int, TTF_Font*> fonts;

public:
	~FontSet();

	void init();
	void clear();

	TTF_Font* getFont(int height);
	int length(const string& text, int height);
private:
	TTF_Font* addSize(int size);
};

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "Thrones";
private:
	static constexpr char fileIcon[] = "icon.ico";
	static constexpr vec2i defaultWindowPos = { SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED };
	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;
	static constexpr GLclampf colorClear[4] = { 0.f, 0.f, 0.f, 1.f };
	static constexpr SDL_Color colorText = { 220, 220, 220, 255 };
	
	uptr<FileSys> fileSys;
	uptr<Program> program;
	uptr<Scene> scene;
	uptr<Settings> sets;

	SDL_Window* window;
	SDL_GLContext context;
	FontSet fonts;
	umap<string, Texture> texes;	// name, texture data
	int curDisplay;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue

public:
	WindowSys();

	int start();
	void close();

	Texture renderText(const string& text, int height);
	int textLength(const string& text, int height);
	void clearFonts();

	float getDSec() const;
	const Texture* texture(const string& name) const;
	vec2i windowSize() const;
	vector<vec2i> displaySizes() const;
	vector<SDL_DisplayMode> displayModes() const;
	int displayID() const;
	void setScreen(Settings::Screen screen, vec2i size, const SDL_DisplayMode& mode, uint8 samples);
	void setVsync(Settings::VSync vsync);
	void setSmooth(Settings::Smooth smooth);
	void setGamma(float gamma);
	void resetSettings();

	FileSys* getFileSys();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();

private:
	void init();
	void exec();
	void cleanup();

	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void setDSec(uint32& oldTicks);
	void setSwapInterval();
	bool trySetSwapInterval();

	void updateViewport() const;
	void updateSmooth() const;
	template <class T> static bool checkResolution(T& val, const vector<T>& modes);
};

inline void WindowSys::close() {
	run = false;
}

inline Texture WindowSys::renderText(const string& text, int height) {
	return TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colorText);
}

inline int WindowSys::textLength(const string& text, int height) {
	return fonts.length(text, height);
}

inline void WindowSys::clearFonts() {
	fonts.clear();
}

inline float WindowSys::getDSec() const {
	return dSec;
}

inline FileSys* WindowSys::getFileSys() {
	return fileSys.get();
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

inline void WindowSys::updateViewport() const {
	vec2i res = windowSize();
	glViewport(0, 0, res.x, res.y);
}

inline vec2i WindowSys::windowSize() const {
	vec2i res;
	SDL_GetWindowSize(window, &res.x, &res.y);
	return res;
}

inline int WindowSys::displayID() const {
	return SDL_GetWindowDisplayIndex(window);
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
