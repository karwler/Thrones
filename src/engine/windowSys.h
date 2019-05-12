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

	float heightScale;	// for scaling down font size to fit requested height
	string file;
	umap<int, TTF_Font*> fonts;

public:
	~FontSet();

	void init(const string& path);
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
	static constexpr char fileFont[] = "Merriweather.otf";
	static constexpr char fileIcon[] = "icon.ico";
	static constexpr vec2i defaultWindowPos = { SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED };
	static constexpr vec2i minWindowSize = { 640, 480 };
	static constexpr uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
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
	int displayID() const;
	void setFullscreen(bool on);
	void setResolution(const string& line);
	void setVsync(Settings::VSync vsync);
	void setSmooth(Settings::Smooth smooth);
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

	void updateViewport() const;
	void updateSmooth() const;
	vec2i displayResolution() const;
	void setResolution(const vec2i& res);
};

inline void WindowSys::close() {
	run = false;
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

inline vec2i WindowSys::displayResolution() const {
	SDL_DisplayMode mode;
	return !SDL_GetDesktopDisplayMode(window ? SDL_GetWindowDisplayIndex(window) : 0, &mode) ? vec2i(mode.w, mode.h) : INT_MAX;
}

inline void WindowSys::setResolution(const vec2i& res) {
	sets->resolution = res.clamp(minWindowSize, displayResolution());
}
