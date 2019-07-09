#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
public:
	static constexpr char argAddress[] = "s";
	static constexpr char argConnect[] = "c";
	static constexpr char argPort[] = "p";
	static constexpr char argSetup[] = "d";

	static Arguments args;
private:
	static WindowSys windowSys;		// the thing ontop of which everything runs

public:
	static WindowSys* window();
	static AudioSys* audio();
	static FontSet* fonts();
	static Game* game();
	static Program* program();
	static ProgState* state();
	static Settings* sets();
	static Scene* scene();
	static ShaderScene* space();
	static ShaderGUI* gui();

#ifdef _WIN32
	static void setArgs(Win::PWSTR pCmdLine);
#else
	static void setArgs(int argc, char** argv);
#endif
	template <class F, class... A> static void prun(F func, A... args);
	static void play(const string& name);
};

inline WindowSys* World::window() {
	return &windowSys;
}

inline AudioSys* World::audio() {
	return windowSys.getAudio();
}

inline FontSet* World::fonts() {
	return windowSys.getFonts();
}

inline Game* World::game() {
	return windowSys.getProgram()->getGame();
}

inline Program* World::program() {
	return windowSys.getProgram();
}

inline ProgState* World::state() {
	return windowSys.getProgram()->getState();
}

inline Scene* World::scene() {
	return windowSys.getScene();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

inline ShaderScene* World::space() {
	return windowSys.getSpace();
}

inline ShaderGUI* World::gui() {
	return windowSys.getGUI();
}

template <class F, class... A>
void World::prun(F func, A... args) {
	if (func)
		(program()->*func)(args...);
}

inline void World::play(const string& name) {
	if (windowSys.getAudio())
		windowSys.getAudio()->play(name);
}
