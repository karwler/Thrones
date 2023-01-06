#pragma once

#include "alias.h"
#include <charconv>
#include <sstream>

#ifdef _WIN32
constexpr string_view linend = "\r\n";
#else
constexpr string_view linend = "\n";
#endif

// utility

string_view readWord(const char*& pos);
string strEnclose(string_view str);
string strUnenclose(const char*& str);
int strnatcmp(const char* a, const char* b);	// natural string compare
uint8 u8clen(char c);
vector<string> readTextLines(string_view text);
bool createDirectories(const string& path);

inline bool strnatless(string_view a, string_view b) {
	return strnatcmp(a.data(), b.data()) < 0;
}

template <class T>
string operator+(std::basic_string<T>&& a, std::basic_string_view<T> b) {
	sizet alen = a.length();
	a.resize(alen + b.length());
	std::copy(b.begin(), b.end(), a.begin() + alen);
	return a;
}

template <class T>
string operator+(const std::basic_string<T>& a, std::basic_string_view<T> b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	std::copy(a.begin(), a.end(), r.begin());
	std::copy(b.begin(), b.end(), r.begin() + a.length());
	return r;
}

template <class T>
string operator+(std::basic_string_view<T> a, std::basic_string<T>&& b) {
	sizet blen = b.length();
	b.resize(a.length() + blen);
	std::move_backward(b.begin(), b.begin() + blen, b.begin() + a.length());
	std::copy(a.begin(), a.end(), b.begin());
	return b;
}

template <class T>
string operator+(std::basic_string_view<T> a, const std::basic_string<T>& b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	std::copy(a.begin(), a.end(), r.begin());
	std::copy(b.begin(), b.end(), r.begin() + a.length());
	return r;
}

inline bool isDsep(char c) {
#ifdef _WIN32
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}

inline string appDsep(string_view str) {
	return !str.empty() && isDsep(str.back()) ? string(str) : string(str) + '/';
}

inline bool isSpace(char c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(char c) {
	return uchar(c) > ' ' && c != 0x7F;
}

inline string firstUpper(string_view str) {
	string txt(str);
	txt[0] = toupper(txt[0]);
	return txt;
}

inline string_view trim(string_view str) {
	string_view::iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string_view(str.data() + (pos - str.begin()), std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base() - pos);
}

inline string_view delExt(string_view path) {
	string_view::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() ? *it == '.' && it + 1 != path.rend() && !isDsep(it[1]) ? string_view(path.data(), it.base() - 1 - path.begin()) : path : string_view();
}

inline string_view filename(string_view path) {
	string_view::reverse_iterator end = std::find_if_not(path.rbegin(), path.rend(), isDsep);
	string_view::iterator pos = std::find_if(end, path.rend(), isDsep).base();
	return string_view(&*pos, end.base() - pos);
}

inline string_view parentPath(string_view path) {
	string_view::iterator fin = std::find_if(std::find_if_not(path.rbegin(), path.rend(), isDsep), path.rend(), isDsep).base();
#ifdef _WIN32
	return string_view(path.data(), fin - path.begin());
#else
	return string_view(path.data(), (isDsep(*fin) ? fin + 1 : fin) - path.begin());
#endif
}

inline bool strciEndsWith(string_view str, string_view end) {
	return str.length() >= end.length() ? !SDL_strcasecmp(str.data() + str.length() - end.length(), end.data()) : false;
}

inline const char* pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

template <class... A>
void logInfo(A&&... args) {
	std::ostringstream ss;
	(ss << ... << std::forward<A>(args));
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s%s", ss.str().c_str(), linend.data());
}

template <class... A>
void logError(A&&... args) {
	std::ostringstream ss;
	(ss << ... << std::forward<A>(args));
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s%s", ss.str().c_str(), linend.data());
}

// conversions

#ifdef _WIN32
string swtos(wstring_view wstr);
wstring sstow(string_view str);
string lastErrorMessage();
#endif
SDL_DisplayMode strToDisp(string_view str);

inline const char* toStr(bool b) {
	return b ? "true" : "false";
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(T num) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num, base);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);
	return string(buf.data(), res.ptr);
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(T num, uint8 pad) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num, base);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);

	uint8 len = res.ptr - buf.data();
	if (len >= pad)
		return string(buf.data(), res.ptr);

	string str;
	str.resize(pad);
	pad -= len;
	std::fill_n(str.begin(), pad, '0');
	std::copy(buf.data(), res.ptr, str.begin() + pad);
	return str;
}

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
string toStr(T num) {
	return std::to_string(num);
}
#else
template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
constexpr sizet recommendedCharBufferSize() {
	if constexpr (std::is_same_v<T, float>)
		return 16;
	if constexpr (std::is_same_v<T, double>)
		return 24;
	return 30;
}

template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
string toStr(T num) {
	array<char, recommendedCharBufferSize<T>()> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num);
	return string(buf.data(), res.ptr);
}
#endif

template <glm::length_t L, class T, glm::qualifier Q, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L - 1]);
}

template <class T, sizet N, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, int> = 0>
T strToEnum(const array<const char*, N>& names, string_view str, T defaultValue = T(N)) {
	typename array<const char*, N>::const_iterator p = std::find_if(names.begin(), names.end(), [str](const char* it) -> bool {  return !SDL_strncasecmp(it, str.data(), str.length()) && !it[str.length()]; });
	return p != names.end() ? T(p - names.begin()) : defaultValue;
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>, int> = 0>
T strToVal(const umap<T, const char*>& names, string_view str, T defaultValue = T(0)) {
	typename umap<T, const char*>::const_iterator it = std::find_if(names.begin(), names.end(), [str](const pair<const T, const char*>& nit) -> bool { return !SDL_strncasecmp(nit.second, str.data(), str.length()) && !nit.second[str.length()]; });
	return it != names.end() ? it->first : defaultValue;
}

inline bool toBool(string_view str) {
	return (str.length() >= 4 && !SDL_strncasecmp(str.data(), "true", 4)) || (str.length() >= 2 && !SDL_strncasecmp(str.data(), "on", 2)) || (!str.empty() && !SDL_strncasecmp(str.data(), "y", 1)) || std::any_of(str.begin(), str.end(), [](char c) -> bool { return c >= '1' && c <= '9'; });
}

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
T toNum(string_view str) {
	string tmp(str);
	if constexpr (std::is_same_v<T, float>)
		return strtof(tmp.c_str(), nullptr);
	if constexpr (std::is_same_v<T, double>)
		return strtod(tmp.c_str(), nullptr);
	return strtold(tmp.c_str(), nullptr);
}

template <class T, class... A, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
#else
template <class T, class... A, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
#endif
T toNum(string_view str, A... args) {
	T val = T(0);
	sizet i = 0;
	for (; i < str.length() && isSpace(str[i]); ++i);
	std::from_chars(str.data() + i, str.data() + str.length(), val, args...);
	return val;
}

template <class T, class... A, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
T readNumber(string_view& str, T num = T(0), A... args) {
	sizet i = 0;
	for (; i < str.length() && isSpace(str[i]); ++i);
	for (; i < str.length(); ++i)
		if (std::from_chars_result res = std::from_chars(str.data() + i, str.data() + str.length(), num, args...); res.ec == std::errc()) {
			i = res.ptr - str.data();
			break;
		}
	str = string_view(str.data() + i, str.length() - i);
	return num;
}

template <class T, class... A, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
T readNumber(const char*& str, T num = T(0), A... args) {
	string_view pos = str;
	num = readNumber<T>(pos, num, args...);
	str = pos.data();
	return num;
}

template <class T, class N = typename T::value_type, class... A, std::enable_if_t<(std::is_integral_v<N> && !std::is_same_v<N, bool>) || std::is_floating_point_v<N>, int> = 0>
T toVec(string_view str, typename T::value_type fill = typename T::value_type(0), A... args) {
	T vec(fill);
	for (glm::length_t i = 0; !str.empty() && i < vec.length(); ++i)
		vec[i] = readNumber<N>(str, vec[i], args...);
	return vec;
}

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
uint8 numDigits(T num, uint8 base = 10) {
	return num ? uint8(log(num) / log(base)) + 1 : 1;
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

// struct tm wrapper
struct DateTime {
	uint8 sec, min, hour;
	uint8 day, month;
	uint8 wday;
	uint16 year;

	DateTime() = default;
	DateTime(uint8 second, uint8 minute, uint8 dhour, uint8 mday, uint8 ymonth, uint16 tyear, uint8 weekDay);

	static DateTime now();
	string timeString(char ts = '-') const;
	string dateString(char ds = '-') const;
	string toString(char ts = '-', char sep = '_', char ds = '-') const;
	bool datecmp(const DateTime& date) const;
};

inline string DateTime::timeString(char ts) const {
	return toStr(hour, 2) + ts + toStr(min, 2) + ts + toStr(sec, 2);
}

inline string DateTime::dateString(char ds) const {
	return toStr(year) + ds + toStr(month, 2) + ds + toStr(day, 2);
}

inline string DateTime::toString(char ts, char sep, char ds) const {
	return dateString(ds) + sep + timeString(ts);
}

inline bool DateTime::datecmp(const DateTime& date) const {
	return day == date.day && month == date.month && year == date.year;
}
