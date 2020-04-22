#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
public:
	static inline Arguments args;
private:
	static inline WindowSys windowSys;		// the thing on top of which everything runs

public:
	static WindowSys* window();
	static AudioSys* audio();
	static FontSet* fonts();
	static Game* game();
	static InputSys* input();
	static Netcp* netcp();
	static Program* program();
	static ProgState* state();
	template <class T> static T* state();
	static Settings* sets();
	static Scene* scene();
	static const ShaderGeometry* geom();
#ifndef OPENGLES
	static const ShaderDepth* depth();
#endif
	static const ShaderGui* gui();

#ifdef _WIN32
	static void setArgs(wchar* pCmdLine);
#endif
	static void setArgs(int argc, char** argv);

	template <class F, class... A> static void prun(F func, A... argv);
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

inline InputSys* World::input() {
	return windowSys.getInput();
}

inline Netcp* World::netcp() {
	return windowSys.getProgram()->getNetcp();
}

inline Program* World::program() {
	return windowSys.getProgram();
}

inline ProgState* World::state() {
	return windowSys.getProgram()->getState();
}

template <class T>
T* World::state() {
	return static_cast<T*>(windowSys.getProgram()->getState());
}

inline Scene* World::scene() {
	return windowSys.getScene();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

inline const ShaderGeometry* World::geom() {
	return windowSys.getGeom();
}

#ifndef OPENGLES
inline const ShaderDepth* World::depth() {
	return windowSys.getDepth();
}
#endif

inline const ShaderGui* World::gui() {
	return windowSys.getGui();
}

#ifdef _WIN32
inline void World::setArgs(wchar* pCmdLine) {
	args.setArgs(pCmdLine, { Settings::argSetup }, { Settings::argExternal });
}
#endif

inline void World::setArgs(int argc, char** argv) {
	args.setArgs(argc, argv, stos, { Settings::argSetup }, { Settings::argExternal });
}

template <class F, class... A>
void World::prun(F func, A... argv) {
	if (func)
		(program()->*func)(argv...);
}

inline void World::play(const string& name) {
	if (windowSys.getAudio())
		windowSys.getAudio()->play(name, windowSys.getSets()->avolume);
}
