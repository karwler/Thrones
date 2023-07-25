#include "world.h"

template <class C>
void World::setArgs(int argc, const C* const* argv) {
#ifdef NDEBUG
	args.setArgs(argc, argv, { Settings::argCompositor, Settings::argLog }, { Settings::argExternal });
#else
	args.setArgs(argc, argv, { Settings::argCompositor, Settings::argLog, Settings::argConsole, Settings::argSetup }, { Settings::argExternal });
#endif
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	if (lpCmdLine && lpCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).c_str(), &argc)) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
	if (pCmdLine && pCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
			World::setArgs(argc, argv);
			LocalFree(argv);
		}
#else
int main(int argc, char** argv) {
	World::setArgs(argc - 1, argv + 1);
#endif
#ifdef __ANDROID__
	exit(World::window()->start(World::args));
#else
	return World::window()->start(World::args);
#endif
}
