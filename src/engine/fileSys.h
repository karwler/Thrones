#pragma once

#include "utils/objects.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif

struct Settings {
	static constexpr char loopback[] = "127.0.0.1";
	static constexpr float gammaMax = 2.f;

	enum class Screen : uint8 {
		window,
		borderless,
		fullscreen,
		desktop
	};
	static const array<string, sizet(Screen::desktop)+1> screenNames;

	enum class VSync : int8 {
		adaptive = -1,
		immediate,
		synchronized
	};
	static const array<string, 3> vsyncNames;	// add 1 to vsync value to get name

	bool maximized;
	uint8 avolume;
	uint8 display;
	Screen screen;
	VSync vsync;
	uint8 msamples;
	float gamma;
	ivec2 size;
	SDL_DisplayMode mode;
	string address;
	uint16 port;

	Settings();
};

struct Setup {
	sset<pair<svec2, Com::Tile>> tiles;
	sset<pair<uint16, Com::Tile>> mids;
	sset<pair<svec2, Com::Piece>> pieces;

	void clear();
};

// handles all filesystem interactions
class FileSys {
private:
	static string dirData, dirConfig;
	static constexpr char fileAudios[] = "audio.dat";
	static constexpr char fileMaterials[] = "materials.dat";
	static constexpr char fileObjects[] = "objects.dat";
	static constexpr char fileShaders[] = "shaders.dat";
	static constexpr char fileTextures[] = "textures.dat";
	static constexpr char fileConfigs[] = "game.ini";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileSetups[] = "setup.ini";

	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordDisplay[] = "display";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordSize[] = "size";
	static constexpr char iniKeywordMode[] = "mode";
	static constexpr char iniKeywordVsync[] = "vsync";
	static constexpr char iniKeywordMsamples[] = "samples";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";

	static constexpr char iniKeywordBoardSize[] = "size";
	static constexpr char iniKeywordSurvival[] = "survival";
	static constexpr char iniKeywordFavors[] = "favors";
	static constexpr char iniKeywordDragonDist[] = "dragon_dist";
	static constexpr char iniKeywordDragonDiag[] = "dragon_diag";
	static constexpr char iniKeywordMultistage[] = "multistage";
	static constexpr char iniKeywordSurvivalKill[] = "survival_kill";
	static constexpr char iniKeywordTile[] = "tile_";
	static constexpr char iniKeywordMiddle[] = "middle_";
	static constexpr char iniKeywordPiece[] = "piece_";
	static constexpr char iniKeywordWinFortress[] = "win_fortresses";
	static constexpr char iniKeywordWinThrone[] = "win_thrones";
	static constexpr char iniKeywordCapturers[] = "capturers";
	static constexpr char iniKeywordShift[] = "middle_shift";
	static constexpr char iniKeywordLeft[] = "left";
	static constexpr char iniKeywordRight[] = "right";
	static constexpr char iniKeywordNear[] = "near";
	static constexpr char iniKeywordFar[] = "far";

public:
	static void init();

	static Settings* loadSettings();
	static bool saveSettings(const Settings* sets);
	static umap<string, Com::Config> loadConfigs(const char* file = fileConfigs);
	static bool saveConfigs(const umap<string, Com::Config>& confs, const char* file = fileConfigs);
	static umap<string, Setup> loadSetups();
	static bool saveSetups(const umap<string, Setup>& sets);
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static umap<string, Material> loadMaterials();
	static umap<string, GMesh> loadObjects();
	static umap<string, string> loadShaders();
	static umap<string, Texture> loadTextures();

	static string dataPath(const char* file);
	static string configPath(const char* file);
private:
	static bool createDir(const string& path);

	template <sizet N, sizet S> static void readAmount(const pairStr& it, sizet wlen, const array<string, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const string& word, const array<string, N>& names, const array<uint16, S>& amts);
	static void readShift(const string& line, Com::Config& conf);
};

inline string FileSys::dataPath(const char* file) {
	return dirData + file;
}

inline string FileSys::configPath(const char* file) {
	return dirConfig + file;
}

inline bool FileSys::createDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

template <sizet N, sizet S>
void FileSys::readAmount(const pairStr& it, sizet wlen, const array<string, N>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, it.first.substr(wlen)); id < amts.size())
		amts[id] = uint16(sstol(it.second));
}

template <sizet N, sizet S>
void FileSys::writeAmounts(string& text, const string& word, const array<string, N>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); i++)
		text += makeIniLine(word + names[i], toStr(amts[i]));
}
