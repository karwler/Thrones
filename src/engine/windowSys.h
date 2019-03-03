#pragma once

#include "fileSys.h"
#include "scene.h"
#include "prog/program.h"
#include "utils/layouts.h"
#include <SDL2/SDL_ttf.h>

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
	static constexpr char fileFont[] = "font.otf";
	static constexpr char fileIcon[] = "icon.ico";
	static constexpr uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;
	static constexpr SDL_Color colorText = {220, 220, 220, 255};
	static const vec2i minWindowSize;

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
	void setFullscreen(bool on);
	void setResolution(const string& line);
	void setVsync(Settings::VSync vsync);
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
	void eventWindow(const SDL_WindowEvent& window);
	void setDSec(uint32& oldTicks);
	void setSwapInterval();

	void updateViewport() const;
	vec2i displayResolution() const;
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
	int x, y;
	SDL_GetWindowSize(window, &x, &y);
	return vec2i(x, y);
}

inline vec2i WindowSys::displayResolution() const {
	SDL_DisplayMode mode;
	return !SDL_GetDesktopDisplayMode(window ? SDL_GetWindowDisplayIndex(window) : 0, &mode) ? vec2i(mode.w, mode.h) : INT_MAX;
}
