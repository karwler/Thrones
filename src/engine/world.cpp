#include "world.h"
#include <clocale>
#include <locale>

WindowSys World::windowSys;
Arguments World::arguments;

const string& World::getOpt(const string& key) {
	umap<string, string>::const_iterator it = arguments.opts.find(key);
	return it != arguments.opts.end() ? it->second : arguments.opts[""];
}
#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::setArgs(pCmdLine);
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv);
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	return World::winSys()->start();
}
