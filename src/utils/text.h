#pragma once

#ifdef DEBUG
#define MALLOC_CHECK_ 2
#endif
#include <SDL2/SDL.h>
#ifdef main
#undef main
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
namespace Win {
#include <windows.h>	// needs to be here to prevent conflicts with SDL_net
}
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
using std::pair;
using std::string;
using std::wstring;
using std::vector;

template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uset = std::unordered_set<T...>;
using pairStr = pair<string, string>;

using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#ifdef _WIN32
constexpr char linend[] = "\r\n";
#else
constexpr char linend[] = "\n";
#endif
constexpr char defaultReadMode[] = "rb";
constexpr char defaultWriteMode[] = "wb";

// utility

#define ENUM_OPERATIONS(EType, IType) \
	inline constexpr EType operator~(EType a) { \
		return EType(~IType(a)); \
	} \
	inline constexpr EType operator&(EType a, EType b) { \
		return EType(IType(a) & IType(b)); \
	} \
	inline constexpr EType operator&=(EType& a, EType b) { \
		return a = EType(IType(a) & IType(b)); \
	} \
	inline constexpr EType operator|(EType a, EType b) { \
		return EType(IType(a) | IType(b)); \
	} \
	inline constexpr EType operator|=(EType& a, EType b) { \
		return a = EType(IType(a) | IType(b)); \
	} \
	inline constexpr EType operator^(EType a, EType b) { \
		return EType(IType(a) ^ IType(b)); \
	} \
	inline constexpr EType operator^=(EType& a, EType b) { \
		return a = EType(IType(a) ^ IType(b)); \
	}

bool hasExt(const string& path, const string& ext);
string filename(const string& path);
string readWordM(const char*& pos);
int strnatcmp(const char* a, const char* b);	// natural string compare

inline string readWord(const char* pos) {
	return readWordM(pos);
}

inline bool strnatless(const string& a, const string& b) {
	return strnatcmp(a.c_str(), b.c_str()) < 0;
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
	string::const_iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string(pos, std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base());
}

inline string delExt(const string& path) {
	string::const_reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() && *it == '.' ? string(path.begin(), it.base() - 1) : string();
}

inline bool isDotName(const string& str) {
	return str[0] == '.' && (str[1] == '\0' || (str[1] == '.' && str[2] == '\0'));
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

inline const char* pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

inline string toStr(float num) {
	string str = std::to_string(num);
	sizet id = str.find_last_not_of('0');
	return str.substr(0, str[id] == '.' ? id : id + 1);
}

template <class T>
string toStr(T num) {
	return std::to_string(num);
}

inline string dispToStr(const SDL_DisplayMode& mode) {
	return toStr(mode.w) + ' ' + toStr(mode.h) + ' ' + toStr(mode.refresh_rate) + ' ' + toStr(mode.format);
}

inline bool stob(const string& str) {
	return str == "true" || str == "yes" || str == "1";
}

inline string btos(bool b) {
	return b ? "true" : "false";
}

template <class T, sizet N>
T strToEnum(const array<string, N>& names, const string& str) {
	return T(std::find_if(names.begin(), names.end(), [str](const string& it) -> bool { return !strcicmp(it, str); }) - names.begin());
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

template <class T>
string ntosPadded(T num, uint pad) {
	string str = toStr(num);
	return str.length() < pad ? string(pad - str.length(), '0') + str : str;
}

template <class U, class S>	// U has to be an unsigned type and S has to be the signed equivalent of U
U cycle(U pos, U siz, S mov) {
	U rst = pos + U(mov % S(siz));
	return rst < siz ? rst : mov >= S(0) ? rst - siz : siz + rst;
}

template <class T>
vector<string> sortNames(const umap<string, T>& vmap) {
	vector<string> names(vmap.size());
	vector<string>::iterator nit = names.begin();
	for (const pair<const string, T>& cit : vmap)
		*nit++ = cit.first;
	std::sort(names.begin(), names.end(), strnatless);
	return names;
}

// files

vector<string> readFileLines(const string& file);
bool writeFile(const string& file, const string& text);
string readIniTitle(const string& line);
pairStr readIniLine(const string& line);

inline string makeIniLine(const string& title) {
	return '[' + title + ']' + linend;
}

inline string makeIniLine(const string& key, const string& val) {
	return key + '=' + val + linend;
}

template <class T = string>
T readFile(const string& file) {
	T data;
	FILE* ifh = fopen(file.c_str(), defaultReadMode);
	if (!ifh)
		return data;
	fseek(ifh, 0, SEEK_END);
	ulong len = ulong(ftell(ifh));
	fseek(ifh, 0, SEEK_SET);

	data.resize(len);
	if (sizet read = fread(data.data(), sizeof(*data.data()), data.size(), ifh); read != len)
		data.resize(read);
	fclose(ifh);
	return data;
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
	Arguments(Win::PWSTR pCmdLine, const uset<char>& flg, const uset<char>& opt);
	Arguments(int argc, wchar** argv, const uset<char>& flg, const uset<char>& opt);
#endif
	Arguments(int argc, char** argv, const uset<char>& flg, const uset<char>& opt);

#ifdef _WIN32
	void setArgs(Win::PWSTR pCmdLine, const uset<char>& flg, const uset<char>& opt);
#endif
	template <class C, class F> void setArgs(int argc, C** argv, F conv, const uset<char>& flg, const uset<char>& opt);

	const vector<string>& getVals() const;
	bool hasFlag(char key) const;
	const char* getOpt(char key) const;
};
#ifdef _WIN32
inline Arguments::Arguments(Win::PWSTR pCmdLine, const uset<char>& flg, const uset<char>& opt) {
	setArgs(pCmdLine, flg, opt);
}

inline Arguments::Arguments(int argc, wchar** argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, wtos, flg, opt);
}
#endif
inline Arguments::Arguments(int argc, char** argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, stos, flg, opt);
}

template <class C, class F>
void Arguments::setArgs(int argc, C** argv, F conv, const uset<char>& flg, const uset<char>& opt) {
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
