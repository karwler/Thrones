#include "world.h"
#include <clocale>
#include <locale>

WindowSys World::windowSys;

#ifdef _WIN32
#ifdef _DEBUG
int wmain(int, wchar**) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
#endif
#else
int main(int, char**) {
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	return World::winSys()->start();
}
