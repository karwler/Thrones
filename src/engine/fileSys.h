#pragma once

#include "utils/objects.h"

struct Settings {
	static constexpr char loopback[] = "localhost";
	static constexpr uint16 shadowResMax = 1 << 14;
	static constexpr float gammaMax = 2.f;
	static constexpr uint16 chatLinesMax = 8191;

	enum class Screen : uint8 {
		window,
		borderless,
		fullscreen,
		desktop
	};
	static constexpr array<const char*, sizet(Screen::desktop)+1> screenNames = {
		"window",
		"borderless",
		"fullscreen",
		"desktop"
	};

	enum class VSync : int8 {
		adaptive = -1,
		immediate,
		synchronized
	};
	static constexpr array<const char*, sizet(VSync::synchronized)+2> vsyncNames = {	// add 1 to vsync value to get name
	   "adaptive",
	   "immediate",
	   "synchronized"
   };

	string address;
	SDL_DisplayMode mode;
	ivec2 size;
	float gamma;
	uint16 port;
	uint16 shadowRes;
	uint16 chatLines;
	uint8 display;
	Screen screen;
	VSync vsync;
	uint8 msamples;
	uint8 texScale;
	bool softShadows;
	uint8 avolume;
	bool scaleTiles, scalePieces;
	Com::Family resolveFamily;
	bool fontRegular;

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
	inline static string dirData, dirConfig;

	static constexpr char fileAudios[] = "audio.dat";
	static constexpr char fileMaterials[] = "materials.dat";
	static constexpr char fileObjects[] = "objects.dat";
	static constexpr char fileShaders[] = "shaders.dat";
	static constexpr char fileTextures[] = "textures.dat";
	static constexpr char fileConfigs[] = "game.ini";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileSetups[] = "setup.ini";

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
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordScaleTiles[] = "scale_tiles";
	static constexpr char iniKeywordScalePieces[] = "scale_pieces";
	static constexpr char iniKeywordChatLines[] = "chat_lines";
	static constexpr char iniKeywordResolveFamily[] = "resolve_family";
	static constexpr char iniKeywordFontRegular[] = "font_regular";
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
	static void saveSettings(const Settings* sets);
	static umap<string, Com::Config> loadConfigs();
	static void saveConfigs(const umap<string, Com::Config>& confs);
	static umap<string, Setup> loadSetups();
	static void saveSetups(const umap<string, Setup>& sets);
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static umap<string, Material> loadMaterials();
	static umap<string, Mesh> loadObjects();
	static umap<string, string> loadShaders();
	static umap<string, Texture> loadTextures();
	static void reloadTextures(umap<string, Texture>& texs);

#ifdef EMSCRIPTEN
	static bool canRead();
#endif
	static string dataPath(const char* file);
private:
	static string configPath(const char* file);

	template <sizet N, sizet S> static void readAmount(const pairStr& it, sizet wlen, const array<const char*, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const string& word, const array<const char*, N>& names, const array<uint16, S>& amts);
	static void readShift(const string& line, Com::Config& conf);
	static void loadTextures(umap<string, Texture>& texs, void (*inset)(umap<string, Texture>&, string&&, SDL_Surface*, GLint, GLenum));
	static void writeFile(const char* file, const string& text);
};

inline string FileSys::dataPath(const char* file) {
	return dirData + file;
}

inline string FileSys::configPath(const char* file) {
	return dirConfig + file;
}

inline void FileSys::reloadTextures(umap<string, Texture>& texs) {
	loadTextures(texs, [](umap<string, Texture>& texs, string&& name, SDL_Surface* img, GLint iform, GLenum pform) { texs[name].reload(img, iform, pform); });
}
