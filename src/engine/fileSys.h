#pragma once

#include "utils/objects.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif

struct Settings {
	static constexpr char loopback[] = "127.0.0.1";
	static constexpr uint16 shadowResMax = 1 << 15;
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
	uint8 texScale;
	uint16 shadowRes;
	bool softShadows;
	float gamma;
	ivec2 size;
	SDL_DisplayMode mode;
	bool scaleTiles, scalePieces;
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
	static constexpr char iniKeywordTexScale[] = "texture_scale";
	static constexpr char iniKeywordShadowRes[] = "shadow_resolution";
	static constexpr char iniKeywordSoftShadows[] = "soft_shadows";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordScaleTiles[] = "scale_tiles";
	static constexpr char iniKeywordScalePieces[] = "scale_pieces";
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";

	static constexpr char iniKeywordBoardSize[] = "size";
	static constexpr char iniKeywordSurvivalPass[] = "survival_pass";
	static constexpr char iniKeywordSurvivalMode[] = "survival_mode";
	static constexpr char iniKeywordFavorLimit[] = "favor_limit";
	static constexpr char iniKeywordFavorMax[] = "favor_max";
	static constexpr char iniKeywordDragonDist[] = "dragon_dist";
	static constexpr char iniKeywordDragonSingle[] = "dragon_single";
	static constexpr char iniKeywordDragonDiag[] = "dragon_diag";
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
	static umap<string, Mesh> loadObjects();
	static umap<string, string> loadShaders();
	static umap<string, Texture> loadTextures();
	static void reloadTextures(umap<string, Texture>& texs);

	static string dataPath(const char* file);
	static string configPath(const char* file);
private:
	static bool createDir(const string& path);

	template <sizet N, sizet S> static void readAmount(const pairStr& it, sizet wlen, const array<string, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const string& word, const array<string, N>& names, const array<uint16, S>& amts);
	static void readShift(const string& line, Com::Config& conf);
	static void loadTextures(umap<string, Texture>& texs, void (*inset)(umap<string, Texture>&, string&&, SDL_Surface*, GLint, GLenum));
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

inline void FileSys::reloadTextures(umap<string, Texture>& texs) {
	loadTextures(texs, [](umap<string, Texture>& texs, string&& name, SDL_Surface* img, GLint iform, GLenum pform) { texs[name].reload(img, iform, pform); });
}
