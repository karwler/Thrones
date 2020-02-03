#include "world.h"
#include <clocale>
#include <locale>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::setArgs(pCmdLine);
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv);
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(World::envLocale));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	World::window()->start();
#ifdef __ANDROID__
	exit(0);
#endif
	return 0;
}
