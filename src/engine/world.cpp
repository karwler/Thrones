#include "world.h"

template <class C>
void World::setArgs(int argc, const C* const* argv) {
#ifdef NDEBUG
	args.setArgs(argc, argv, {}, { Settings::argExternal, Settings::argCompositor });
#else
	args.setArgs(argc, argv, { Settings::argConsole, Settings::argCompositor, Settings::argSetup }, { Settings::argExternal });
#endif
}

#ifndef IS_TEST_LIBRARY
#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(cstow(lpCmdLine).c_str(), &argc)) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
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
