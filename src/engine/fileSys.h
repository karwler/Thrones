#pragma once

#include "utils/utils.h"

// reads/writes an INI line
struct IniLine {
	enum Type : uint8 {
		empty,
		prpVal,		// property, value, no key, no title
		prpKeyVal,	// property, key, value, no title
		title		// title, no everything else
	} type;
	string prp;	// property, aka. the thing before the equal sign/brackets (can also be the title)
	string key;	// the thing between the brackets (empty if there are no brackets)
	string val;	// value, aka. the thing after the equal sign

	IniLine(string_view line);
	IniLine(Type what, string&& property = string(), string&& keyval = string(), string&& value = string());

	Type read(string_view str);
	static vector<IniLine> readLines(string_view text);
	static void write(SDL_RWops* ofh, string_view tit);
	static void write(SDL_RWops* ofh, string_view prp, string_view val);
	static void write(SDL_RWops* ofh, string_view prp, string_view key, string_view val);
};

inline IniLine::IniLine(string_view line) {
	read(line);
}

// handles all filesystem interactions
class FileSys {
public:
	static constexpr int fontTestHeight = 100;

	static constexpr char fileRules[] = "rules.html";
	static constexpr char fileDocs[] = "docs.html";
private:
	static constexpr char fileMaterials[] = "materials.dat";
	static constexpr char fileConfigs[] = "game.ini";
	static constexpr char fileSettings[] = "setting.ini";
	static constexpr char fileSetups[] = "setup.ini";

	static constexpr char iniTitleSettings[] = "Settings";
	static constexpr char iniTitleBindings[] = "Bindings";
	static constexpr char iniKeywordDisplay[] = "display";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordSize[] = "size";
	static constexpr char iniKeywordMode[] = "mode";
	static constexpr char iniKeywordVsync[] = "vsync";
	static constexpr char iniKeywordMsamples[] = "samples";
	static constexpr char iniKeywordAnisotropy[] = "anisotropy";
	static constexpr char iniKeywordTexScale[] = "textures";
	static constexpr char iniKeywordShadows[] = "shadows";
	static constexpr char iniKeywordSsao[] = "ssao";
	static constexpr char iniKeywordBloom[] = "bloom";
	static constexpr char iniKeywordSsr[] = "ssr";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordFov[] = "fov";
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordColors[] = "colors";
	static constexpr char iniKeywordScales[] = "scales";
	static constexpr char iniKeywordAutoVictoryPoints[] = "auto_vp";
	static constexpr char iniKeywordTooltips[] = "tooltips";
	static constexpr char iniKeywordChatLines[] = "chat_lines";
	static constexpr char iniKeywordDeadzone[] = "deadzone";
	static constexpr char iniKeywordResolveFamily[] = "resolve_family";
	static constexpr char iniKeywordFont[] = "font";
	static constexpr char iniKeywordHinting[] = "hinting";
	static constexpr char iniKeywordInvertWheel[] = "invert_wheel";
	static constexpr char iniKeywordVersionLookup[] = "version_lookup";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";
	static constexpr char iniKeywordPlayerName[] = "player_name";
	static constexpr char iniKeywordLastConfig[] = "last_config";

	static constexpr char iniKeyKey[] = "K";
	static constexpr char iniKeyJoy[] = "J";
	static constexpr char iniKeyGpd[] = "G";
	static constexpr char iniVkeyButton = 'B';
	static constexpr char iniVkeyHat = 'H';
	static constexpr char iniVkeyAxis = 'A';

	static constexpr char iniKeywordVictoryPoints[] = "victory_points";
	static constexpr char iniKeywordPorts[] = "ports";
	static constexpr char iniKeywordRowBalancing[] = "row_balancing";
	static constexpr char iniKeywordHomefront[] = "homefront";
	static constexpr char iniKeywordSetPieceBattle[] = "piece_battle";
	static constexpr char iniKeywordBoardSize[] = "size";
	static constexpr char iniKeywordBattlePass[] = "battle_win";
	static constexpr char iniKeywordFavorLimit[] = "favor_limit";
	static constexpr char iniKeywordFirstTurnEngage[] = "first_engage";
	static constexpr char iniKeywordTerrainRules[] = "terrain_rules";
	static constexpr char iniKeywordDragonLate[] = "dragon_late";
	static constexpr char iniKeywordDragonStraight[] = "dragon_straight";
	static constexpr char iniKeywordTile[] = "tile";
	static constexpr char iniKeywordMiddle[] = "middle";
	static constexpr char iniKeywordPiece[] = "piece";
	static constexpr char iniKeywordWinFortress[] = "win_fortresses";
	static constexpr char iniKeywordWinThrone[] = "win_thrones";
	static constexpr char iniKeywordCapturers[] = "capturers";

	enum class Texplace : uint8 {
		none,
		widget,
		object,
		both,
		cube
	};

	static inline string dirBase, dirConfig;
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	static inline SDL_RWops* logFile;
#endif

public:
	static void init(const Arguments& args);
	static void close();

	static Settings loadSettings(InputSys* input);
	static void saveSettings(const Settings& sets, const InputSys* input);
	static umap<string, Config> loadConfigs();
	static void readConfigPrpVal(const IniLine& il, Config& cfg);
	static void readConfigPrpKeyVal(const IniLine& il, Config& cfg);
	static void saveConfigs(const umap<string, Config>& confs);
	static void writeConfig(SDL_RWops* ofh, const Config& cfg);
	template <sizet N, sizet S> static void writeAmounts(SDL_RWops* ofh, const char* word, const array<const char*, N>& names, const array<uint16, S>& amts);
	template <sizet N, sizet S> static void readAmount(const IniLine& il, const array<const char*, N>& names, array<uint16, S>& amts);
	static umap<string, Setup> loadSetups();
	static void saveSetups(const umap<string, Setup>& sets);
	static vector<string> listFonts();
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static vector<Material> loadMaterials(umap<string, uint16>& refs);
	static vector<Mesh> loadObjects(umap<string, uint16>& refs);
	static umap<string, string> loadShaders();
	static TextureSet::Import loadTextures(vector<TextureCol::Element>& icns, int& minUiHeight, float scale);
	static TextureSet::Import reloadTextures(float scale);
#ifndef __ANDROID__
	static vector<string> listFiles(string_view dirPath);
#ifndef __EMSCRIPTEN__
	static void saveScreenshot(SDL_Surface* img, string_view desc = string_view());
#endif
#endif

#ifdef __EMSCRIPTEN__
	static bool canRead();
#endif
	static string dataPath();
	static string fontPath();
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	static string docPath();
#endif
	static const string& getDirConfig();
	static string recordPath();
private:
	static string texturePath();

	static void readSetting(void* settings, IniLine& il);
	static void readShadows(const char* str, Settings& sets);
	static void readColors(const char* str, Settings& sets);
	static void readScales(const char* str, Settings& sets);
	static void readVersionLookup(const char* str, Settings& sets);
	static void readBinding(void* inputSys, IniLine& il);

	static void readVictoryPoints(const char* str, Config& cfg);
	static void readSetPieceBattle(const char* str, Config& cfg);
	static void readFavorLimit(const char* str, Config& cfg);
	static uint16 readCapturers(const char* str);
	static void writeCapturers(SDL_RWops* ofh, uint16 capturers);
	static SDL_RWops* startWriteUserFile(const string& drc, const char* file);
	static void endWriteUserFile(SDL_RWops* ofh);
	static tuple<Texplace, sizet, string> beginTextureLoad(string_view dirPath, string& name, bool noWidget);
	static void loadObjectTexture(SDL_Surface* img, string_view dirPath, string& name, sizet num, GLenum ifmt, TextureSet::Import& imp, float scale, bool unique = true);
	static void loadSkyTextures(TextureSet::Import& imp, string& path, sizet dirPathLen, sizet num);
	template <class F> static void listOperateDirectory(string_view dirPath, F func);
	template <class T, class F> static string strJoin(const vector<T>& vec, F conv, char sep = ' ');
	static string::iterator fileLevelPos(string& str);
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	static void SDLCALL logWrite(void* userdata, int category, SDL_LogPriority priority, const char* message);
#endif

	friend bool operator&(Texplace a, Texplace b);
};

inline string FileSys::dataPath() {
#if defined(__APPLE__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
	return dirBase;
#else
	return dirBase + "share/thrones/";
#endif
}

inline string FileSys::fontPath() {
	return dataPath() + "fonts/";
}

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
inline string FileSys::docPath() {
#if defined(_WIN32) || defined(__APPLE__)
	return dirBase + "doc/";
#else
	return dirBase + "share/thrones/doc/";
#endif
}
#endif

inline string FileSys::recordPath() {
	return dirConfig + "records/";
}

inline string FileSys::texturePath() {
	return dataPath() + "textures/";
}

inline const string& FileSys::getDirConfig() {
	return dirConfig;
}

template <sizet N, sizet S>
void FileSys::writeAmounts(SDL_RWops* ofh, const char* word, const array<const char*, N>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); ++i)
		IniLine::write(ofh, word, names[i], toStr(amts[i]));
}

template <sizet N, sizet S>
void FileSys::readAmount(const IniLine& il, const array<const char*, N>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, il.key); id < amts.size())
		amts[id] = toNum<int>(il.val);
}

inline bool operator&(FileSys::Texplace a, FileSys::Texplace b) {
	return std::underlying_type_t<FileSys::Texplace>(a) & std::underlying_type_t<FileSys::Texplace>(b);
}
