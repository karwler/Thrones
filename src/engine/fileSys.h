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

	IniLine(const string& line);
	IniLine(Type what, string&& property = string(), string&& keyval = string(), string&& value = string());

	Type read(const string& str);
	static void write(string& text, const string& title);
	static void write(string& text, const string& prp, const string& val);
	static void write(string& text, const string& prp, const string& key, const string& val);
};

inline IniLine::IniLine(const string& line) {
	read(line);
}

inline void IniLine::write(string& text, const string& title) {
	text += '[' + title + ']' + linend;
}

inline void IniLine::write(string& text, const string& prp, const string& val) {
	text += prp + '=' + val + linend;
}

inline void IniLine::write(string& text, const string& prp, const string& key, const string& val) {
	text += prp + '[' + key + "]=" + val + linend;
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
	static constexpr char iniKeywordTexScale[] = "textures";
	static constexpr char iniKeywordShadows[] = "shadows";
	static constexpr char iniKeywordSsao[] = "ssao";
	static constexpr char iniKeywordBloom[] = "bloom";
	static constexpr char iniKeywordGamma[] = "gamma";
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

public:
	static void init(const Arguments& args);

	static Settings loadSettings(InputSys* input);
	static void saveSettings(const Settings& sets, const InputSys* input);
	static umap<string, Config> loadConfigs();
	static void saveConfigs(const umap<string, Config>& confs);
	static umap<string, Setup> loadSetups();
	static void saveSetups(const umap<string, Setup>& sets);
	static vector<string> listFonts();
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static umap<string, Material> loadMaterials();
	static vector<Mesh> loadObjects(umap<string, uint16>& refs);	// geometry shader must be in use
	static umap<string, string> loadShaders();
	static umap<string, TexLoc> loadTextures(TextureSet& tset, TextureCol& tcol, float scale, int recomTexSize);
	static void reloadTextures(TextureSet& tset, float scale);
	static void saveScreenshot(SDL_Surface* img, const string& desc = string());

#ifdef __EMSCRIPTEN__
	static bool canRead();
#endif
	static string dataPath();
	static string fontPath();
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	static string docPath();
#endif
	static const string& getDirConfig();
private:
	static string texturePath();

	static void readSetting(void* settings, IniLine& il);
	static void readShadows(const char* str, Settings& sets);
	static void readColors(const char* str, Settings& sets);
	static void readScales(const char* str, Settings& sets);
	static void readVersionLookup(const char* str, Settings& sets);
	static void readBinding(void* inputSys, IniLine& il);

	static void readVictoryPoints(const char* str, Config* cfg);
	static void readSetPieceBattle(const char* str, Config* cfg);
	static void readFavorLimit(const char* str, Config* cfg);
	static uint16 readCapturers(const string& line);
	static void writeCapturers(string& text, uint16 capturers);
	template <sizet N, sizet S> static void readAmount(const IniLine& il, const array<const char*, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const char* word, const array<const char*, N>& names, const array<uint16, S>& amts);
	static void saveUserFile(const string& drc, const char* file, const string& text);
	static tuple<Texplace, sizet, string> beginTextureLoad(const string& dirPath, string& name, bool noWidget);
	static void loadObjectTexture(SDL_Surface* img, const string& dirPath, string& name, sizet num, GLenum ifmt, TextureSet::Import& imp, float scale);
	static void loadSkyTextures(TextureSet::Import& imp, string& path, const string& dirPath, sizet num);
	template <class F> static void listOperateDirectory(const string& dirPath, F func);
	template <class T, class F> static string strJoin(const vector<T>& vec, F conv, char sep = ' ');
	static string::iterator fileLevelPos(string& str);

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

inline string FileSys::texturePath() {
	return dataPath() + "textures/";
}

inline const string& FileSys::getDirConfig() {
	return dirConfig;
}

inline bool operator&(FileSys::Texplace a, FileSys::Texplace b) {
	return std::underlying_type_t<FileSys::Texplace>(a) & std::underlying_type_t<FileSys::Texplace>(b);
}
