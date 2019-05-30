#pragma once

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#endif

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using wchar = wchar_t;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using sizet = size_t;
using pdift = ptrdiff_t;

using std::array;
using std::initializer_list;
using std::pair;
using std::string;
using std::wstring;
using std::to_string;
using std::vector;

template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uset = std::unordered_set<T...>;
using pairStr = pair<string, string>;

#ifdef _WIN32
constexpr char dsep = '\\';
constexpr char dseps[] = "\\";
constexpr char linend[] = "\r\n";
#else
constexpr char dsep = '/';
constexpr char dseps[] = "/";
constexpr char linend[] = "\n";
#endif
const string emptyStr = "";
const string invalidStr = "invalid";

// utility

bool hasExt(const string& path, const string& ext);
string readWordM(const char*& pos);

inline string readWord(const char* pos) {
	return readWordM(pos);
}

inline int strcicmp(const string& a, const string& b) {	// case insensitive check if strings are equal
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

inline int strncicmp(const string& a, const string& b, sizet n) {	// case insensitive check if strings are equal
#ifdef _WIN32
	return _strnicmp(a.c_str(), b.c_str(), n);
#else
	return strncasecmp(a.c_str(), b.c_str(), n);
#endif
}
#ifdef _WIN32
inline bool isDriveLetter(const string& path) {
	return path.length() >= 2 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':' && std::all_of(path.begin() + 2, path.end(), [](char c) -> bool { return c == dsep; });
}
#endif
inline bool isSpace(char c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(char c) {
	return uchar(c) > ' ' && c != 0x7F;
}

inline bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

inline bool notDigit(char c) {
	return c < '0' || c > '9';
}

inline string firstUpper(string str) {
	str[0] = char(toupper(str[0]));
	return str;
}

inline string trim(const string& str) {
	string::const_iterator pos = std::find_if(str.begin(), str.end(), [](char c) -> bool { return uchar(c) > ' '; });
	return string(pos, std::find_if(str.rbegin(), std::make_reverse_iterator(pos), [](char c) -> bool { return uchar(c) > ' '; }).base());
}

inline string trimZero(const string& str) {
	sizet id = str.find_last_not_of('0');
	return str.substr(0, str[id] == '.' ? id : id + 1);
}

inline string delExt(const string& path) {
	string::const_reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || c == dsep; });
	return it != path.rend() && *it == '.' ? string(path.begin(), it.base() - 1) : emptyStr;
}

inline string appDsep(const string& path) {
	return !path.empty() && path.back() == dsep ? path : path + dsep;
}
#ifdef _WIN32
inline wstring appDsep(const wstring& path) {
	return !path.empty() && path.back() == dsep ? path : path + wchar(dsep);
}
#endif
inline string childPath(const string& parent, const string& child) {
#ifdef _WIN32
	return std::all_of(parent.begin(), parent.end(), [](char c) -> bool { return c == dsep; }) && isDriveLetter(child) ? child : appDsep(parent) + child;
#else
	return appDsep(parent) + child;
#endif
}

template <class T>
bool isDotName(const T& str) {
	return str[0] == '.' && (str[1] == '\0' || (str[1] == '.' && str[2] == '\0'));
}

// conversions

#ifdef _WIN32
string wtos(const wchar* wstr);
wstring stow(const string& str);
#else
inline string stos(const char* str) {	// dummy function for ARguments::setArgs
	return str;
}
#endif
SDL_DisplayMode strToDisp(const string& str);

inline const char* pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

inline string dispToStr(const SDL_DisplayMode& mode) {
	return to_string(mode.w) + ' ' + to_string(mode.h) + ' ' + to_string(mode.refresh_rate) + ' ' + to_string(mode.format);
}

inline bool stob(const string& str) {
	return str == "true" || str == "1";
}

inline string btos(bool b) {
	return b ? "true" : "false";
}

template <class T, sizet N>
T strToEnum(const array<string, N>& names, const string& str) {
	return T(std::find_if(names.begin(), names.end(), [str](const string& it) -> bool { return !strcicmp(it, str); }) - names.begin());
}

template <class T, sizet N, class V>
T valToEnum(const array<V, N>& arr, const V& val) {
	return T(std::find(arr.begin(), arr.end(), val) - arr.begin());
}

inline long sstol(const string& str, int base = 0) {
	return strtol(str.c_str(), nullptr, base);
}

inline llong sstoll(const string& str, int base = 0) {
	return strtoll(str.c_str(), nullptr, base);
}

inline ulong sstoul(const string& str, int base = 0) {
	return strtoul(str.c_str(), nullptr, base);
}

inline ullong sstoull(const string& str, int base = 0) {
	return strtoull(str.c_str(), nullptr, base);
}

inline float sstof(const string& str) {
	return strtof(str.c_str(), nullptr);
}

inline double sstod(const string& str) {
	return strtod(str.c_str(), nullptr);
}

inline ldouble sstold(const string& str) {
	return strtold(str.c_str(), nullptr);
}

template <class T>
T btom(bool fwd) {
	return T(fwd) * 2 - 1;
}

template <class T, class F, class... A>
T readNumber(const char*& pos, F strtox, A... args) {
	T num = T(0);
	for (char* end; *pos; pos++)
		if (num = T(strtox(pos, &end, args...)); pos != end) {
			pos = end;
			break;
		}
	return num;
}

template <glm::length_t L>
glm::vec<L, float, glm::highp> stov(const char* str, float fill = 0.f) {
	glm::length_t i = 0;
	glm::vec<L, float, glm::highp> gvn;
	while (*str && i < L)
		gvn[i++] = readNumber<float>(str, strtof);
	while (i < L)
		gvn[i++] = fill;
	return gvn;
}

// files

vector<string> readTextFile(const string& file);
bool writeTextFile(const string& file, const string& text);
string readIniTitle(const string& line);
pairStr readIniLine(const string& line);

inline string makeIniLine(const string& key, const string& val) {
	return key + '=' + val + linend;
}

// command line arguments
struct Arguments {
	vector<string> vals;
	uset<string> flags;
	umap<string, string> opts;

	Arguments() = default;
#ifdef _WIN32
	Arguments(PWSTR pCmdLine);
	Arguments(int argc, wchar** argv);
#else
	Arguments(int argc, char** argv);
#endif

#ifdef _WIN32
	void setArgs(PWSTR pCmdLine);
#endif
	template <class C, class F> void setArgs(int argc, C** argv, F conv);
};
#ifdef _WIN32
inline Arguments::Arguments(PWSTR pCmdLine) {
	setArgs(pCmdLine);
}

inline Arguments::Arguments(int argc, wchar** argv) {
	setArgs(argc - 1, argv + 1, wtos);
}
#else
inline Arguments::Arguments(int argc, char** argv) {
	setArgs(argc - 1, argv + 1, stos);
}
#endif
template <class C, class F>
void Arguments::setArgs(int argc, C** argv, F conv) {
	for (int i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			C* flg = argv[i] + (argv[i][1] == '-' ? 2 : 1);
			if (int ni = i + 1; ni < argc && argv[ni][0] != '-')
				opts.emplace(conv(flg), conv(argv[++i]));
			else
				flags.emplace(conv(flg));
		} else
			vals.emplace_back(conv(argv[i]));
	}
}
