#pragma once

#if !(defined(_DEBUG) || defined(DEBUG)) && !defined(NDEBUG)
#define NDEBUG
#elif defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif
#ifdef DEBUG
#define MALLOC_CHECK_ 2
#endif
#if (defined(__ANDROID__) || defined(EMSCRIPTEN)) && !defined(OPENGLES)
#define OPENGLES
#endif

#ifndef __ANDROID__
#define SDL_MAIN_HANDLED
#endif
#if defined(__ANDROID__) || defined(_WIN32)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
using std::pair;
using std::string;
using std::tuple;
using std::wstring;
using std::vector;

template <class T> using initlist = std::initializer_list<T>;
template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uset = std::unordered_set<T...>;
using pairStr = pair<string, string>;

using glm::mat3;
using glm::mat4;
using glm::quat;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;
using glm::uvec4;
using svec2 = glm::vec<2, uint16, glm::defaultp>;
using mvec2 = glm::vec<2, sizet, glm::defaultp>;

#ifdef _WIN32
constexpr char linend[] = "\r\n";
#else
constexpr char linend[] = "\n";
#endif
constexpr char defaultReadMode[] = "rb";
constexpr char defaultWriteMode[] = "wb";

// utility

string filename(const string& path);
string readWord(const char*& pos);
string strEnclose(string str);
string strUnenclose(const char*& str);
int strnatcmp(const char* a, const char* b);	// natural string compare
uint8 u8clen(char c);
vector<string> readTextLines(const string& text);
string readIniTitle(const string& line);
pairStr readIniLine(const string& line);
void createDirectories(const string& path);

template <class T>
T readMem(const void* data) {
	T val;
	std::copy_n(static_cast<const uint8*>(data), sizeof(T), reinterpret_cast<uint8*>(&val));
	return val;
}

template <class T>
void* writeMem(void* data, const T& val) {
	return std::copy_n(reinterpret_cast<const uint8*>(&val), sizeof(T), static_cast<uint8*>(data));
}

inline bool strnatless(const string& a, const string& b) {
	return strnatcmp(a.c_str(), b.c_str()) < 0;
}

inline int strcicmp(const char* a, const char* b) {	// case insensitive string compare
#ifdef _WIN32
	return _stricmp(a, b);
#else
	return strcasecmp(a, b);
#endif
}

inline int strcicmp(const string& a, const char* b) {
	return strcicmp(a.c_str(), b);
}

inline int strncicmp(const char* a, const char* b, sizet n) {	// case insensitive string compare
#ifdef _WIN32
	return _strnicmp(a, b, n);
#else
	return strncasecmp(a, b, n);
#endif
}

inline int strncicmp(const string& a, const char* b, sizet n) {
	return strncicmp(a.c_str(), b, n);
}

inline bool isDsep(char c) {
#ifdef _WIN32
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}

inline bool isSpace(char c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(char c) {
	return uchar(c) > ' ' && c != 0x7F;
}

inline string firstUpper(string str) {
	str[0] = char(toupper(str[0]));
	return str;
}

inline string trim(const string& str) {
	string::const_iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string(pos, std::find_if(str.rbegin(), string::const_reverse_iterator(pos), notSpace).base());
}

inline string delExt(const string& path) {
	string::const_reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() ? *it == '.' && it + 1 != path.rend() && !isDsep(it[1]) ? string(path.begin(), it.base() - 1) : path : string();
}

template <class T = string>
T loadFile(const string& file) {
	T data;
	if (SDL_RWops* ifh = SDL_RWFromFile(file.c_str(), defaultReadMode)) {
		if (int64 len = SDL_RWsize(ifh); len != -1) {
			data.resize(sizet(len));
			if (sizet read = SDL_RWread(ifh, data.data(), sizeof(*data.data()), data.size()); read < data.size())
				data.resize(read);
		}
		SDL_RWclose(ifh);
	}
	return data;
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator++(T& a) {
	return a = T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(1));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator++(T& a, int) {
	T r = a;
	a = T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(1));
	return r;
}

template <class T, class U, std::enable_if_t<std::is_enum_v<T> && std::is_integral_v<U>, int> = 0>
constexpr T operator+(T a, U b) {
	return T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator--(T& a) {
	return a = T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(1));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator--(T& a, int) {
	T r = a;
	a = T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(1));
	return r;
}

template <class T, class U, std::enable_if_t<std::is_enum_v<T> && std::is_integral_v<U>, int> = 0>
constexpr T operator-(T a, U b) {
	return T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator~(T a) {
	return T(~std::underlying_type_t<T>(a));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&(T a, T b) {
	return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|(T a, T b) {
	return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^(T a, T b) {
	return T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

// conversions

#ifdef _WIN32
string wtos(const wchar* wstr);
wstring stow(const string& str);
#endif
SDL_DisplayMode strToDisp(const string& str);

inline string stos(const char* str) {	// dummy function for Arguments::setArgs
	return str;
}

template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
string toStr(T num) {
	return std::to_string(num);
}

template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
inline string toStr(T num, sizet len) {
	string str = std::to_string(num);
	return len > str.length() ? string(len - str.length(), '0') + str : str;
}

template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
string toStr(T num) {
	string str = std::to_string(num);
	sizet id = str.find_last_not_of('0');
	return str.substr(0, str[id] == '.' ? id : id + 1);
}

inline bool stob(const string& str) {
	return !strncicmp(str, "true", 4) || !strncicmp(str, "on", 2) || !strncicmp(str, "y", 1) || std::any_of(str.begin(), str.end(), [](char c) -> bool { return c >= '1' && c <= '9'; });
}

inline const char* btos(bool b) {
	return b ? "true" : "false";
}

template <class T, sizet N>
T strToEnum(const array<const char*, N>& names, const string& str, T defaultValue = T(N)) {
	typename array<const char*, N>::const_iterator p = std::find_if(names.begin(), names.end(), [str](const char* it) -> bool { return !strcicmp(it, str.c_str()); });
	return p != names.end() ? T(p - names.begin()) : defaultValue;
}

inline long sstol(const char* str, int base = 0) {
	return strtol(str, nullptr, base);
}

inline long sstol(const string& str, int base = 0) {
	return sstol(str.c_str(), base);
}

inline llong sstoll(const char* str, int base = 0) {
	return strtoll(str, nullptr, base);
}

inline llong sstoll(const string& str, int base = 0) {
	return sstoll(str.c_str(), base);
}

inline ulong sstoul(const char* str, int base = 0) {
	return strtoul(str, nullptr, base);
}

inline ulong sstoul(const string& str, int base = 0) {
	return sstoul(str.c_str(), base);
}

inline ullong sstoull(const char* str, int base = 0) {
	return strtoull(str, nullptr, base);
}

inline ullong sstoull(const string& str, int base = 0) {
	return sstoull(str.c_str(), base);
}

inline float sstof(const char* str) {
	return strtof(str, nullptr);
}

inline float sstof(const string& str) {
	return sstof(str.c_str());
}

inline double sstod(const char* str) {
	return strtod(str, nullptr);
}

inline double sstod(const string& str) {
	return sstod(str.c_str());
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

template <class T, class F, class... A>
T stoxv(const char* str, F strtox, typename T::value_type fill, A... args) {
	T vec(fill);
	for (glm::length_t i = 0; *str && i < vec.length();)
		vec[i++] = readNumber<typename T::value_type>(str, strtox, args...);
	return vec;
}

template <class T, class F = decltype(strtof)>
T stofv(const char* str, F strtox = strtof, typename T::value_type fill = typename T::value_type(0)) {
	return stoxv<T>(str, strtox, fill);
}

template <class T, class F>
T stoiv(const char* str, F strtox, typename T::value_type fill = typename T::value_type(0), int base = 0) {
	return stoxv<T>(str, strtox, fill, base);
}

template <glm::length_t L, class T, glm::qualifier Q, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; i++)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L-1]);
}

template <class U, class S, std::enable_if_t<std::is_unsigned_v<U> && std::is_signed_v<S>, int> = 0>
U cycle(U pos, U siz, S mov) {
	U rst = pos + U(mov % S(siz));
	return rst < siz ? rst : mov >= S(0) ? rst - siz : siz + rst;
}

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
uint8 numDigits(T num, uint8 base = 10) {
	return uint8(log(float(num)) / log(float(base))) + 1;
}

template <class T>
vector<string> sortNames(const umap<string, T>& vmap) {
	vector<string> names(vmap.size());
	vector<string>::iterator nit = names.begin();
	for (auto& [name, vt] : vmap)
		*nit++ = name;
	std::sort(names.begin(), names.end(), strnatless);
	return names;
}

// command line arguments
class Arguments {
private:
	vector<string> vals;
	uset<char> flags;
	umap<char, string> opts;

public:
	Arguments() = default;
#ifdef _WIN32
	Arguments(const wchar* pCmdLine, const uset<char>& flg, const uset<char>& opt);
	Arguments(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt);
#endif
	Arguments(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt);

#ifdef _WIN32
	void setArgs(const wchar* pCmdLine, const uset<char>& flg, const uset<char>& opt);
#endif
	template <class C, class F> void setArgs(int argc, const C* const* argv, F conv, const uset<char>& flg, const uset<char>& opt);

	const vector<string>& getVals() const;
	bool hasFlag(char key) const;
	const char* getOpt(char key) const;
};

#ifdef _WIN32
inline Arguments::Arguments(const wchar* pCmdLine, const uset<char>& flg, const uset<char>& opt) {
	setArgs(pCmdLine, flg, opt);
}

inline Arguments::Arguments(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, wtos, flg, opt);
}
#endif

inline Arguments::Arguments(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, stos, flg, opt);
}

template <class C, class F>
void Arguments::setArgs(int argc, const C* const* argv, F conv, const uset<char>& flg, const uset<char>& opt) {
	for (int i = 0; i < argc; i++) {
		if (char key; argv[i][0] == '-') {
			for (int j = 1; key = char(argv[i][j]); j++) {
				if (!argv[i][j+1] && i + 1 < argc && opt.count(key)) {
					opts.emplace(key, conv(argv[++i]));
					break;
				}
				if (flg.count(key))
					flags.insert(key);
			}
		} else
			vals.push_back(conv(argv[i] + (argv[i][0] == '\\' && (argv[i][1] == '-' || argv[i][1] == '\\'))));
	}
	vals.shrink_to_fit();
}

inline const vector<string>& Arguments::getVals() const {
	return vals;
}

inline bool Arguments::hasFlag(char key) const {
	return flags.count(key);
}

inline const char* Arguments::getOpt(char key) const {
	umap<char, string>::const_iterator it = opts.find(key);
	return it != opts.end() ? it->second.c_str() : nullptr;
}
