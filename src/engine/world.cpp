#include "world.h"
#include "audioSys.h"

void World::play(const string& name) {
	if (windowSys.getAudio())
		windowSys.getAudio()->play(name, windowSys.getSets()->avolume);
}

#ifndef IS_TEST_LIBRARY
#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
		World::setArgs(argc, argv);
		LocalFree(argv);
	}
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv);
#endif
#ifdef __ANDROID__
	exit(World::window()->start(World::args));
#else
	return World::window()->start(World::args);
#endif
}
#endif
