#pragma once

#include "windowSys.h"
#include "prog/program.h"

// class that makes accessing stuff easier
class World {
public:
	static inline Arguments args;
private:
	static inline WindowSys windowSys;	// the thing on top of which everything runs

public:
	static WindowSys* window();
	static AudioSys* audio();
	static FontSet* fonts();
	static Game* game();
	static InputSys* input();
	static Netcp* netcp();
	static GuiGen* pgui();
	static Program* program();
	static ProgState* state();
	template <class T> static T* state();
	static Scene* scene();
	static Settings* sets();
#ifdef OPENVR
	static VrSys* vr();
#endif
	static const ShaderGeom* geom();
	static const ShaderDepth* depth();
	static const ShaderSsao* ssao();
	static const ShaderBlur* mblur();
	static const ShaderSsr* ssr();
	static const ShaderSsrColor* ssrColor();
	static const ShaderBlur* cblur();
	static const ShaderLight* light();
	static const ShaderBrights* brights();
	static const ShaderGauss* gauss();
	static const ShaderFinal* sfinal();
	static const ShaderSkybox* skybox();
	static const ShaderGui* sgui();

	template <class F, class... A> static void prun(F func, A&&... argv);
	template <class F, class... A> static void srun(F func, A&&... argv);
	template <class C> static void setArgs(int argc, const C* const* argv);
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

inline GuiGen* World::pgui() {
	return windowSys.getProgram()->getGui();
}

inline Program* World::program() {
	return windowSys.getProgram();
}

inline ProgState* World::state() {
	return windowSys.getProgram()->getState();
}

template <class T>
T* World::state() {
	return windowSys.getProgram()->getState<T>();
}

inline Scene* World::scene() {
	return windowSys.getScene();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

#ifdef OPENVR
inline VrSys* World::vr() {
	return windowSys.getVr();
}
#endif

inline const ShaderGeom* World::geom() {
	return windowSys.getGeom();
}

inline const ShaderDepth* World::depth() {
	return windowSys.getDepth();
}

inline const ShaderSsao* World::ssao() {
	return windowSys.getSsao();
}

inline const ShaderBlur* World::mblur() {
	return windowSys.getMblur();
}

inline const ShaderSsr* World::ssr() {
	return windowSys.getSsr();
}

inline const ShaderSsrColor* World::ssrColor() {
	return windowSys.getSsrColor();
}

inline const ShaderBlur* World::cblur() {
	return windowSys.getCblur();
}

inline const ShaderLight* World::light() {
	return windowSys.getLight();
}

inline const ShaderBrights* World::brights() {
	return windowSys.getBrights();
}

inline const ShaderGauss* World::gauss() {
	return windowSys.getGauss();
}

inline const ShaderFinal* World::sfinal() {
	return windowSys.getSfinal();
}

inline const ShaderSkybox* World::skybox() {
	return windowSys.getSkybox();
}

inline const ShaderGui* World::sgui() {
	return windowSys.getGui();
}

template <class F, class... A>
void World::prun(F func, A&&... argv) {
	if (func)
		(program()->*func)(std::forward<A>(argv)...);
}

template <class F, class... A>
void World::srun(F func, A&&... argv) {
	if (func)
		(state()->*func)(std::forward<A>(argv)...);
}
