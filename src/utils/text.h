#pragma once

#include "alias.h"

#ifdef _WIN32
constexpr char linend[] = "\r\n";
#else
constexpr char linend[] = "\n";
#endif
constexpr char defaultReadMode[] = "rb";
constexpr char defaultWriteMode[] = "wb";

// utility

string parentPath(const string& path);
string filename(const string& path);
string readWord(const char*& pos);
string strEnclose(string str);
string strUnenclose(const char*& str);
int strnatcmp(const char* a, const char* b);	// natural string compare
uint8 u8clen(char c);
vector<string> readTextLines(const string& text);
void createDirectories(const string& path);

inline bool strnatless(const string& a, const string& b) {
	return strnatcmp(a.c_str(), b.c_str()) < 0;
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

inline string filename(const string& path) {
	string::const_reverse_iterator end = std::find_if_not(path.rbegin(), path.rend(), isDsep);
	return string(std::find_if(end, path.rend(), isDsep).base(), end.base());
}

// conversions

#ifdef _WIN32
string cwtos(const wchar* wstr);
string swtos(const wstring& wstr);
wstring cstow(const char* str);
wstring sstow(const string& str);
string lastErrorMessage();
#endif
SDL_DisplayMode strToDisp(const string& str);

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
	return !SDL_strncasecmp(str.c_str(), "true", 4) || !SDL_strncasecmp(str.c_str(), "on", 2) || !SDL_strncasecmp(str.c_str(), "y", 1) || std::any_of(str.begin(), str.end(), [](char c) -> bool { return c >= '1' && c <= '9'; });
}

inline const char* btos(bool b) {
	return b ? "true" : "false";
}

template <class T, sizet N>
T strToEnum(const array<const char*, N>& names, const string& str, T defaultValue = T(N)) {
	typename array<const char*, N>::const_iterator p = std::find_if(names.begin(), names.end(), [str](const char* it) -> bool { return !SDL_strcasecmp(it, str.c_str()); });
	return p != names.end() ? T(p - names.begin()) : defaultValue;
}

template <class T>
T strToVal(const umap<T, const char*>& names, const string& str, T defaultValue = T(0)) {
	typename umap<T, const char*>::const_iterator it = std::find_if(names.begin(), names.end(), [str](const pair<T, const char*>& nit) -> bool { return !SDL_strcasecmp(nit.second, str.c_str()); });
	return it != names.end() ? it->first : defaultValue;
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
	for (char* end; *pos; ++pos)
		if (num = T(strtox(pos, &end, args...)); pos != end) {
			pos = end;
			break;
		}
	return num;
}

template <class T, class F, class... A>
T stoxv(const char* str, F strtox, typename T::value_type fill, A... args) {
	T vec(fill);
	for (glm::length_t i = 0; *str && i < vec.length(); ++i)
		vec[i] = readNumber<typename T::value_type>(str, strtox, args...);
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
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L-1]);
}

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
uint8 numDigits(T num, uint8 base = 10) {
	return num ? log(num) / log(base) + 1 : 1;
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
	Arguments(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt);
#endif
	Arguments(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt);

	const vector<string>& getVals() const;
	bool hasFlag(char key) const;
	const char* getOpt(char key) const;

#ifdef _WIN32
	void setArgs(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt);
#endif
	void setArgs(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt);
private:
	template <class C, class F> void setArgs(int argc, const C* const* argv, F conv, const uset<char>& flg, const uset<char>& opt);
};

#ifdef _WIN32
inline Arguments::Arguments(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc, argv, flg, opt);
}
#endif

inline Arguments::Arguments(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc, argv, flg, opt);
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
