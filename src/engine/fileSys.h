#pragma once

#include "utils/objects.h"
#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

class Settings {
public:
	static constexpr char loopback[] = "127.0.0.1";

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

	enum class Smooth : uint8 {
		off,
		fast,
		nice
	};
	static const array<string, sizet(Smooth::nice)+1> smoothNames;

	bool maximized;
	Screen screen;
	VSync vsync;
	Smooth smooth;
	vec2i size;
	SDL_DisplayMode mode;
	string address;
	uint16 port;

public:
	Settings();
};

enum FileType : uint8 {
	FTYPE_NON = 0x0,
	FTYPE_REG = 0x1,
	FTYPE_DIR = 0x2,
	FTYPE_OTH = 0x4,
	FTYPE_STD = FTYPE_REG | FTYPE_DIR
};

inline constexpr FileType operator~(FileType a) {
	return FileType(~uint8(a));
}

inline constexpr FileType operator&(FileType a, FileType b) {
	return FileType(uint8(a) & uint8(b));
}

inline constexpr FileType operator&=(FileType& a, FileType b) {
	return a = FileType(uint8(a) & uint8(b));
}

inline constexpr FileType operator^(FileType a, FileType b) {
	return FileType(uint8(a) ^ uint8(b));
}

inline constexpr FileType operator^=(FileType& a, FileType b) {
	return a = FileType(uint8(a) ^ uint8(b));
}

inline constexpr FileType operator|(FileType a, FileType b) {
	return FileType(uint8(a) | uint8(b));
}

inline constexpr FileType operator|=(FileType& a, FileType b) {
	return a = FileType(uint8(a) | uint8(b));
}

// handles all filesystem interactions
class FileSys {
public:
	static constexpr char dirTexs[] = "textures";
	static constexpr char extIni[] = ".ini";
private:
	static constexpr char fileSettings[] = "settings.ini";

	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordSize[] = "size";
	static constexpr char iniKeywordMode[] = "mode";
	static constexpr char iniKeywordVsync[] = "vsync";
	static constexpr char iniKeywordSmooth[] = "smooth";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";

#ifdef _WIN32		// os's font directories
	array<string, 2> dirFonts;
#else
	array<string, 3> dirFonts;
#endif
public:
	FileSys();

	Settings* loadSettings();
	bool saveSettings(const Settings* sets);
	static Object loadObj(const string& file);

	static vector<string> listDir(const string& drc, FileType filter = FTYPE_STD);
	static bool createDir(const string& path);
	static FileType fileType(const string& file, bool readLink = true);

private:
	static array<int, 9> readFace(const char* str);	// returns IDs of 3 * { vertex, uv, normal }
	template <class T> static T resolveObjId(int id, const vector<T>& vec);

	static int setWorkingDir();
#ifdef _WIN32
	static bool atrcmp(DWORD attrs, FileType filter);
#else
	static FileType stmtoft(const string& file, int (*statfunc)(const char*, struct stat*));
	static bool dtycmp(const string& drc, const dirent* entry, FileType filter, bool readLink);
#endif
};

template <class T>
T FileSys::resolveObjId(int id, const vector<T>& vec) {
	if (sizet pid = sizet(id - 1); id > 0 && pid < vec.size())
		return vec[pid];
	if (sizet eid = vec.size() + sizet(id); id < 0 && eid < vec.size())
		return vec[eid];
	return T(0.f);
}

inline bool FileSys::createDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}
#ifdef _WIN32
inline bool FileSys::atrcmp(DWORD attrs, FileType filter) {
	return attrs & FILE_ATTRIBUTE_DIRECTORY ? filter & FTYPE_DIR : filter & FTYPE_REG;
}
#else
inline FileType FileSys::fileType(const string& file, bool readLink) {
	return stmtoft(file, readLink ? stat : lstat);
}
#endif
