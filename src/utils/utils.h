#pragma once

// stuff that's used pretty much everywhere
#include "cvec2.h"
#include "server/server.h"
#ifdef __APPLE
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#endif
#include <memory>
#include <queue>
#include <set>

// to make life easier
using std::queue;

template <class... T> using uptr = std::unique_ptr<T...>;
template <class... T> using sset = std::set<T...>;

using vec2s = cvec2<int16>;
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

// general wrappers

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b);

inline bool operator==(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	return a.format == b.format && a.w == b.w && a.h == b.h && a.refresh_rate == b.refresh_rate;
}

inline vec2i mousePos() {
	vec2i p;
	SDL_GetMouseState(&p.x, &p.y);
	return p;
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
	SDL_Rect({ n, n, n, n })
{}

inline constexpr Rect::Rect(int x, int y, int w, int h) :
	SDL_Rect({ x, y, w, h })
{}

inline constexpr Rect::Rect(const vec2i& pos, const vec2i& size) :
	SDL_Rect({ pos.x, pos.y, size.w, size.h })
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

public:
	Texture();
	Texture(const string& file);	// throws if load fails
	Texture(SDL_Surface* img);		// ignores load fail
	Texture(const vec2i& size, const vec4& pos, const vec4& end, bool vertical);	// creates gradient

	void close();
	GLuint getID() const;
	const vec2i getRes() const;
	bool valid() const;

private:
	bool load(SDL_Surface* img);
	void loadGL(const vec2i& size, GLenum format, GLenum type, const void* pix);
};

inline Texture::Texture() :
	id(0),
	res(0)
{}

inline Texture::Texture(const string& file) {
	if (!load(SDL_LoadBMP(file.c_str())))
		throw std::runtime_error("failed to load texture " + file + '\n' + SDL_GetError());
}

inline Texture::Texture(SDL_Surface* img) {
	load(img);
}

inline GLuint Texture::getID() const {
	return id;
}

inline const vec2i Texture::getRes() const {
	return res;
}

inline bool Texture::valid() const {
	return res.hasNot(0);
}

// for Object and Widget

class Interactable {
public:
	virtual ~Interactable() = default;

	virtual void drawTop() const {}
	virtual void onClick(const vec2i& mPos, uint8 mBut);	// dummy function to have an out-of-line virtual function
	virtual void onMouseMove(const vec2i&, const vec2i&) {}
	virtual void onHold(const vec2i&, uint8) {}
	virtual void onDrag(const vec2i&, const vec2i&) {}		// mouse move while left button down
	virtual void onUndrag(uint8) {}							// get's called on mouse button up if instance is Scene's capture
	virtual void onKeypress(const SDL_Keysym&) {}
	virtual void onText(const string&) {}
};

// for travel distance on game board

class Dijkstra {
private:
	struct Node {
		uint16 id;
		uint16 dst;

		Node() = default;
		Node(uint16 id, uint16 dst);
	};

	struct Adjacent {
		uint8 cnt;
		uint16 adj[4];
	};

	struct Comp {
		bool operator()(const Node& a, const Node& b);
	};

public:
	static vector<uint16> travelDist(uint16 src, uint16 width, uint16 size, bool (*stepable)(uint16));
};

inline bool Dijkstra::Comp::operator()(const Node& a, const Node& b) {
	return a.dst > b.dst;
}

// geometry?

mat4 makeTransform(const vec3& pos, const vec3& rot, const vec3& scl);
vector<vec3> transformCopy(vector<vec3> vec, const mat4& trans);

template <class T>
bool inRange(const T& val, const T& min, const T& max) {
	return val >= min && val <= max;
}

template <class T>
bool outRange(const T& val, const T& min, const T& max) {
	return val < min || val > max;
}

template <class T>
bool inRange(const cvec2<T>& val, const cvec2<T>& min, const cvec2<T>& max) {
	return inRange(val.x, min.x, max.x) && inRange(val.y, min.y, max.y);
}

template <class T>
bool outRange(const cvec2<T>& val, const cvec2<T>& min, const cvec2<T>& max) {
	return outRange(val.x, min.x, max.x) || outRange(val.y, min.y, max.y);
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

template <class U, class S>	// U has to be an unsigned type and S has to be the signed equivalent of U
U cycle(U pos, U siz, S mov) {
	U rst = pos + U(mov);
	if (rst < siz)
		return rst;
	return mov >= S(0) ? (rst - siz) % siz : (siz - U(-mov)) % siz;
}

template <class T>
T linearTransition(const T& start, const T& end, float factor) {
	return start + (end - start) * factor;
}

// container stuff

template <class T>
vector<T>& uniqueSort(vector<T>& vec) {
	std::sort(vec.begin(), vec.end());
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
	return vec;
}

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
