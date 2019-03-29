#pragma once

// stuff that's used pretty much everywhere
#include "server/server.h"
#include "utils/cvec2.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <array>
#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifndef _WIN32
#include <strings.h>
#endif

// get rid of SDL's main
#ifdef main
#undef main
#endif

// to make life easier
using std::array;
using std::queue;
using std::pair;
using std::vector;

template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uptr = std::unique_ptr<T...>;
template <class... T> using uset = std::unordered_set<T...>;

using pairStr = pair<string, string>;

using vec2b = cvec2<char>;
using vec2i = cvec2<int>;
using vec2f = cvec2<float>;
using vec2d = cvec2<double>;
using vec2t = cvec2<sizet>;

// forward declaraions
class BoardObject;
class Button;
class Layout;
class Program;
class ProgState;

using BCall = void (Program::*)(Button*);
using OCall = void (Program::*)(BoardObject*);

// global constants
#ifdef _WIN32
const char dsep = '\\';
const char dseps[] = "\\";
#else
const char dsep = '/';
const char dseps[] = "/";
#endif
const string emptyStr = "";
const string invalidStr = "invalid";

// general wrappers

inline vec2i mousePos() {
	vec2i p;
	SDL_GetMouseState(&p.x, &p.y);
	return p;
}

inline bool operator==(SDL_Color a, SDL_Color b) {
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline const char* pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

// SDL_Rect wrapper

struct Rect : SDL_Rect {
	Rect() = default;
	constexpr Rect(int n);
	constexpr Rect(int x, int y, int w, int h);
	constexpr Rect(const vec2i& pos, const vec2i& size);

	vec2i& pos();
	constexpr vec2i pos() const;
	vec2i& size();
	constexpr vec2i size() const;
	constexpr vec2i end() const;

	bool contain(const vec2i& point) const;
	vec4 crop(const Rect& rect);				// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off (left, top, right, bottom)
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify the rect
};

inline constexpr Rect::Rect(int n) :
	SDL_Rect({n, n, n, n})
{}

inline constexpr Rect::Rect(int x, int y, int w, int h) :
	SDL_Rect({x, y, w, h})
{}

inline constexpr Rect::Rect(const vec2i& pos, const vec2i& size) :
	SDL_Rect({pos.x, pos.y, size.w, size.h})
{}

inline vec2i& Rect::pos() {
	return *reinterpret_cast<vec2i*>(this);
}

inline constexpr vec2i Rect::pos() const {
	return vec2i(x, y);
}

inline vec2i& Rect::size() {
	return reinterpret_cast<vec2i*>(this)[1];
}

inline constexpr vec2i Rect::size() const {
	return vec2i(w, h);
}

inline constexpr vec2i Rect::end() const {
	return pos() + size();
}

inline bool Rect::contain(const vec2i& point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// OpenGL texture wrapper

class Texture {
private:
	GLuint id;
	vec2i res;
	GLenum format;
	string name;

public:
	Texture(GLuint id = GLuint(-1), const vec2i& res = 0, GLenum format = 0);
	Texture(const string& file);
	Texture(SDL_Surface* img, const string& file, bool setFilename = true);

	void load(const string& file);
	void load(SDL_Surface* img, const string& file, bool setFilename = true);
	void close();

	GLuint getID() const;
	const vec2i getRes() const;
	bool valid() const;
	const string& getName() const;
};

inline Texture::Texture(GLuint tex, const vec2i& res, GLenum format) :
	id(tex),
	res(res),
	format(format)
{}

inline Texture::Texture(const string& file) {
	load(file);
}

inline Texture::Texture(SDL_Surface* img, const string& file, bool setFilename) {
	load(img, file, setFilename);
}

inline void Texture::load(const string& file) {
	load(SDL_LoadBMP(file.c_str()), file);
}

inline GLuint Texture::getID() const {
	return id;
}

inline const vec2i Texture::getRes() const {
	return res;
}

inline bool Texture::valid() const {
	return format;
}

inline const string& Texture::getName() const {
	return name;
}

// for Object and Widget

class Interactable {
public:
	virtual ~Interactable() = default;

	virtual void onClick(const vec2i&, uint8) {}
	virtual void onDoubleClick(const vec2i&, uint8) {}
	virtual void onMouseMove(const vec2i&, const vec2i&) {}
	virtual void onHold(const vec2i&, uint8) {}
	virtual void onDrag(const vec2i&, const vec2i&) {}	// mouse move while left button down
	virtual void onUndrag(uint8) {}						// get's called on mouse button up if instance is Scene's capture
	virtual void onScroll(const vec2i&) {}				// on mouse wheel y movement
	virtual void onKeypress(const SDL_Keysym&) {}
	virtual void onText(const string&) {}
};

// files and strings

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

template <class T, class F, class... A>
vector<T> stov(const char* str, sizet len, F strtox, T fill = T(0), A... args) {
	for (; isSpace(*str); str++);

	sizet i = 0;
	vector<T> vec(len);
	while (*str && i < len) {
		char* end;
		if (T num = T(strtox(str, &end, args...)); end != str) {
			vec[i++] = num;
			for (str = end; isSpace(*str); str++);
		} else
			str++;
	}
	while (i < len)
		vec[i++] = fill;
	return vec;
}

// geometry?

template <class T>
bool outRange(const T& val, const T& min, const T& max) {
	return val < min || val > max;
}

template <class T>
bool outRange(const cvec2<T>& val, const cvec2<T>& min, const cvec2<T>& max) {
	return outRange(val.x, min.x, min.y) || outRange(val.y, min.y, max.y);
}

template <class T>
const T& clampLow(const T& val, const T& min) {
	return (val >= min) ? val : min;
}

template <class T>
cvec2<T> clampLow(const cvec2<T>& val, const cvec2<T>& min) {
	return cvec2<T>(clampLow(val.x, min.x), clampLow(val.y, min.y));
}

template <class T>
const T& clampHigh(const T& val, const T& max) {
	return (val <= max) ? val : max;
}

template <class T>
cvec2<T> clampHigh(const cvec2<T>& val, const cvec2<T>& max) {
	return cvec2<T>(clampHigh(val.x, max.x), clampHigh(val.y, max.y));
}

// conversions

#ifdef _WIN32
string wtos(const wchar* wstr);
wstring stow(const string& str);
#endif

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

template <class V, class T>
V vtog(const vector<T>& vec) {
	V gvn;
	for (sizet i = 0, end = sizet(gvn.length()) <= vec.size() ? sizet(gvn.length()) : vec.size(); i < end; i++)
		gvn[int(i)] = vec[i];
	return gvn;
}

// container stuff

template <class T>
T popBack(vector<T>& vec) {
	T t = vec.back();
	vec.pop_back();
	return t;
}

template <class T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}
