#include "world.h"
#include <clocale>
#include <locale>

Arguments World::args;
WindowSys World::windowSys;

#ifdef _WIN32
inline void World::setArgs(Win::PWSTR pCmdLine) {
#else
inline void World::setArgs(int argc, char** argv) {
#endif
	uset<string> flags = { argAddress, argConnect, argPort, argSetup };
	uset<string> opts = { argAddress, argPort };
#ifdef _WIN32
	args.setArgs(pCmdLine, flags, opts);
#else
	args.setArgs(argc, argv, stos, flags, opts);
#endif
}
#ifdef _WIN32
int WINAPI wWinMain(Win::HINSTANCE hInstance, Win::HINSTANCE hPrevInstance, Win::PWSTR pCmdLine, int nCmdShow) {
	World::setArgs(pCmdLine);
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv);
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	return World::window()->start();
}
