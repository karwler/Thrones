#pragma once

#include "utils/objects.h"
#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

struct Settings {
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

	enum class Color : uint8 {
		black,
		blue,
		brass,
		bronze,
		copper,
		tin,
		obsidian,
		perl,
		red,
		white
	};
	static constexpr array<const char*, sizet(Color::white)+1> colorNames = {
		"black",
		"blue",
		"brass",
		"bronze",
		"copper",
		"tin",
		"obsidian",
		"perl",
		"red",
		"white"
	};

	static constexpr char defaultAddress[] = "94.16.112.206";
	static constexpr char defaultVersionLocation[] = "https://github.com/Karwler/Thrones/releases";
	static constexpr char defaultVersionRegex[] = "<a\\s+href=\"/Karwler/Thrones/releases/tag/v(\\d+\\.\\d+\\.\\d+(\\.\\d+)?)\">";
	static constexpr Screen defaultScreen = Screen::window;
	static constexpr VSync defaultVSync = VSync::synchronized;
	static constexpr uint16 shadowResMax = 1 << 14;
	static constexpr float gammaMax = 2.f;
	static constexpr uint16 chatLinesMax = 8191;
	static constexpr Com::Family defaultFamily = Com::Family::v4;
	static constexpr Color defaultAlly = Color::brass;
	static constexpr Color defaultEnemy = Color::tin;

	static constexpr char argExternal = 'e';
	static constexpr char argSetup = 'd';

	string address;
	string port;
	pair<string, string> versionLookup;
	SDL_DisplayMode mode;
	ivec2 size;
	float gamma;
	uint16 shadowRes;
	uint16 chatLines;
	uint16 deadzone;
	uint8 display;
	Screen screen;
	VSync vsync;
	uint8 msamples;
	uint8 texScale;
	bool softShadows;
	uint8 avolume;
	Color colorAlly, colorEnemy;
	bool scaleTiles, scalePieces;
	bool autoVictoryPoints;
	bool tooltips;
	Com::Family resolveFamily;
	bool fontRegular;

	Settings();
};

struct Setup {
	vector<pair<svec2, Com::Tile>> tiles;
	vector<pair<uint16, Com::Tile>> mids;
	vector<pair<svec2, Com::Piece>> pieces;

	void clear();
};

// handles all filesystem interactions
class FileSys {
private:
	static inline string dirData, dirConfig;

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
	static constexpr char iniKeywordTexScale[] = "textures";
	static constexpr char iniKeywordShadows[] = "shadows";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordColors[] = "colors";
	static constexpr char iniKeywordScales[] = "scales";
	static constexpr char iniKeywordAutoVictoryPoints[] = "auto_vp";
	static constexpr char iniKeywordTooltips[] = "tooltips";
	static constexpr char iniKeywordChatLines[] = "chat_lines";
	static constexpr char iniKeywordDeadzone[] = "deadzone";
	static constexpr char iniKeywordResolveFamily[] = "resolve_family";
	static constexpr char iniKeywordFontRegular[] = "font_regular";
	static constexpr char iniKeywordVersionLookup[] = "version_lookup";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";

	static constexpr char iniKeywordVictoryPoints[] = "victory_points";
	static constexpr char iniKeywordPorts[] = "ports";
	static constexpr char iniKeywordRowBalancing[] = "row_balancing";
	static constexpr char iniKeywordHomefront[] = "homefront";
	static constexpr char iniKeywordSetPieceBattle[] = "piece_battle";
	static constexpr char iniKeywordBoardSize[] = "size";
	static constexpr char iniKeywordBattlePass[] = "battle_win";
	static constexpr char iniKeywordFavorLimit[] = "favor_limit";
	static constexpr char iniKeywordDragonLate[] = "dragon_late";
	static constexpr char iniKeywordDragonStraight[] = "dragon_straight";
	static constexpr char iniKeywordTile[] = "tile_";
	static constexpr char iniKeywordMiddle[] = "middle_";
	static constexpr char iniKeywordPiece[] = "piece_";
	static constexpr char iniKeywordWinFortress[] = "win_fortresses";
	static constexpr char iniKeywordWinThrone[] = "win_thrones";
	static constexpr char iniKeywordCapturers[] = "capturers";

public:
	static void init(const Arguments& args);

	static Settings loadSettings();
	static void saveSettings(const Settings* sets);
	static umap<string, Com::Config> loadConfigs();
	static void saveConfigs(const umap<string, Com::Config>& confs);
	static umap<string, Setup> loadSetups();
	static void saveSetups(const umap<string, Setup>& sets);
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static umap<string, Material> loadMaterials();
	static umap<string, Mesh> loadObjects(const ShaderGeometry* geom);
	static umap<string, string> loadShaders();
	static umap<string, Texture> loadTextures(int scale);
	static void reloadTextures(umap<string, Texture>& texs, int scale);

#ifdef EMSCRIPTEN
	static bool canRead();
#endif
	static string dataPath(const char* file);
private:
	static string configPath(const char* file);

	static void readShadows(const char* str, Settings& sets);
	static void readColors(const char* str, Settings& sets);
	static void readScales(const char* str, Settings& sets);
	static void readVersionLookup(const char* str, pair<string, string>& vl);
	static void readVictoryPoints(const char* str, Com::Config* cfg);
	static void readSetPieceBattle(const char* str, Com::Config* cfg);
	static void readFavorLimit(const char* str, Com::Config* cfg);
	template <sizet N, sizet S> static void readAmount(const pairStr& it, sizet wlen, const array<const char*, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const string& word, const array<const char*, N>& names, const array<uint16, S>& amts);
	static uint16 readCapturers(const string& line);
	static void writeCapturers(string& text, uint16 capturers);
	static void writeIniLine(string& text, const string& title);
	static void writeIniLine(string& text, const string& key, const string& val);
	static void writeFile(const string& path, const string& text);
	static void loadTextures(umap<string, Texture>& texs, void (*inset)(umap<string, Texture>&, string&&, SDL_Surface*, GLint, GLenum), int scale);
};

inline string FileSys::dataPath(const char* file) {
	return dirData + file;
}

inline string FileSys::configPath(const char* file) {
	return dirConfig + file;
}

inline void FileSys::writeIniLine(string& text, const string& title) {
	text += '[' + title + ']' + linend;
}

inline void FileSys::writeIniLine(string& text, const string& key, const string& val) {
	text += key + '=' + val + linend;
}

inline void FileSys::reloadTextures(umap<string, Texture>& texs, int scale) {
	loadTextures(texs, [](umap<string, Texture>& txv, string&& name, SDL_Surface* img, GLint iform, GLenum pform) { txv[name].reload(img, iform, pform); }, scale);
}
