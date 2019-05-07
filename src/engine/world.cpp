#include "world.h"
#include <clocale>
#include <locale>

WindowSys World::windowSys;
vector<string> World::vals;
uset<string> World::flags;
umap<string, string> World::opts;

const string& World::getOpt(const string& key) {
	try {
		return opts.at(key);
	} catch (const std::out_of_range&) {}
	return emptyStr;
}
#ifdef _WIN32
void World::setArgs(PWSTR pCmdLine) {
	int argc;
	LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);
	if (!argv)
		return;
	wchar* flg;
#else
void World::setArgs(int argc, char** argv) {
	char* flg;
#endif
	for (int i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			flg = argv[i] + (argv[i][1] == '-' ? 2 : 1);
			if (int ni = i + 1; ni < argc && argv[ni][0] != '-')
				opts.emplace(wtos(flg), wtos(argv[++i]));
			else
				flags.emplace(wtos(flg));
		} else
			vals.emplace_back(wtos(argv[i]));
	}
#ifdef _WIN32
	LocalFree(argv);
#endif
}
#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::setArgs(pCmdLine);
#else
int main(int argc, char** argv) {
	World::setArgs(argc - 1, argv + 1);
#endif
	std::setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	return World::winSys()->start();
}
