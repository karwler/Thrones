#pragma once

// stuff that's used pretty much everywhere
#include "oven/oven.h"
#include "server/server.h"
#include <memory>
#include <queue>
#include <set>

// forward declaraions and aliases
class BoardObject;
class Button;
class Layout;
class Program;
class ProgState;
class ShaderGUI;

template <class... T> using uptr = std::unique_ptr<T...>;
template <class... T> using sset = std::set<T...>;

using BCall = void (Program::*)(Button*);
using GCall = void (Program::*)(BoardObject*, uint8);

// general wrappers

constexpr float PI = float(M_PI);

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b);

inline bool operator==(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	return a.format == b.format && a.w == b.w && a.h == b.h && a.refresh_rate == b.refresh_rate;
}

namespace glm {
template <class T, qualifier Q>
bool operator<(const vec<2, T, Q>& a, const vec<2, T, Q>& b) {
	return a.y < b.y || (a.y == b.y && a.x < b.x);
}
}

inline ivec2 mousePos() {
	ivec2 p;
	SDL_GetMouseState(&p.x, &p.y);
	return p;
}

#ifdef OPENGLES
inline void glClearDepth(double d) {
	glClearDepthf(float(d));
}
#endif

// SDL_Rect wrapper

struct Rect : SDL_Rect {
	Rect() = default;
	constexpr Rect(int n);
	constexpr Rect(int x, int y, int w, int h);
	constexpr Rect(const ivec2& pos, const ivec2& size);

	ivec2& pos();
	ivec2 pos() const;
	ivec2& size();
	ivec2 size() const;
	ivec2 end() const;

	bool contain(const ivec2& point) const;
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify the rect
};

inline constexpr Rect::Rect(int n) :
	SDL_Rect({ n, n, n, n })
{}

inline constexpr Rect::Rect(int x, int y, int w, int h) :
	SDL_Rect({ x, y, w, h })
{}

inline constexpr Rect::Rect(const ivec2& pos, const ivec2& size) :
	SDL_Rect({ pos.x, pos.y, size.x, size.y })
{}

inline ivec2& Rect::pos() {
	return *reinterpret_cast<ivec2*>(this);
}

inline ivec2 Rect::pos() const {
	return ivec2(x, y);
}

inline ivec2& Rect::size() {
	return reinterpret_cast<ivec2*>(this)[1];
}

inline ivec2 Rect::size() const {
	return ivec2(w, h);
}

inline ivec2 Rect::end() const {
	return pos() + size();
}

inline bool Rect::contain(const ivec2& point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// OpenGL texture wrapper

class Texture {
private:
#ifdef OPENGLES
	static constexpr GLenum defaultFormat3 = GL_RGB;
	static constexpr GLenum defaultFormat4 = GL_RGBA;
#else
	static constexpr GLenum defaultFormat3 = GL_BGR;
	static constexpr GLenum defaultFormat4 = GL_BGRA;
#endif

	GLuint id;
	ivec2 res;

public:
	Texture(GLuint id = 0, const ivec2& res = ivec2(0));
	Texture(SDL_Surface* img);											// for text
	Texture(ivec2 size, GLint iform, GLenum pform, const uint8* pix);	// for image

	void close();
	void free();
	GLuint getID() const;
	ivec2 getRes() const;
	bool valid() const;

	static Texture loadBlank();
private:
	static GLuint loadGL(const ivec2& size, GLint iformat, GLenum pformat, GLenum type, const void* pix, GLint wrap, GLint filter);
};

inline Texture::Texture(GLuint id, const ivec2& res) :
	id(id),
	res(res)
{}

inline void Texture::free() {
	glDeleteTextures(1, &id);
}

inline GLuint Texture::getID() const {
	return id;
}

inline ivec2 Texture::getRes() const {
	return res;
}

inline bool Texture::valid() const {
	return res.x && res.y;
}

inline Texture Texture::loadBlank() {
	constexpr uint8 color[3] = { 255, 255, 255 };
	return Texture(loadGL(ivec2(1), GL_RGB8, defaultFormat3, GL_UNSIGNED_BYTE, color, GL_CLAMP_TO_EDGE, GL_NEAREST), ivec2(1));
}

// for Object and Widget

class Interactable {
public:
	Interactable() = default;
	virtual ~Interactable() = default;

	virtual void drawTop() const {}
	virtual void onClick(const ivec2& mPos, uint8 mBut);	// dummy function to have an out-of-line virtual function
	virtual void onHold(const ivec2&, uint8) {}
	virtual void onDrag(const ivec2&, const ivec2&) {}		// mouse move while left button down
	virtual void onUndrag(uint8) {}							// get's called on mouse button up if instance is Scene's capture
	virtual void onHover() {}
	virtual void onUnhover() {}
	virtual void onKeypress(const SDL_Keysym&) {}
	virtual void onText(const char*) {}
};

// for travel distance on game board

class Dijkstra {
private:
	struct Node {
		uint16 id;
		uint16 dst;

		Node() = default;
		constexpr Node(uint16 id, uint16 dst);
	};

	struct Adjacent {
		uint8 cnt;
		uint16 adj[8];
	};

	struct Comp {
		bool operator()(Node a, Node b);
	};

public:
	static vector<uint16> travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, svec2), uint8 movSize);
};

inline constexpr Dijkstra::Node::Node(uint16 id, uint16 dst) :
	id(id),
	dst(dst)
{}

inline bool Dijkstra::Comp::operator()(Node a, Node b) {
	return a.dst > b.dst;
}

// geometry?

template <class T>
bool inRange(const T& val, const T& min, const T& max) {
	return val >= min && val <= max;
}

template <class T, glm::qualifier Q>
bool inRange(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min, const glm::vec<2, T, Q>& max) {
	return inRange(val.x, min.x, max.x) && inRange(val.y, min.y, max.y);
}

template <class T>
bool outRange(const T& val, const T& min, const T& max) {
	return val < min || val > max;
}

template <class T, glm::qualifier Q>
bool outRange(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min, const glm::vec<2, T, Q>& max) {
	return outRange(val.x, min.x, max.x) || outRange(val.y, min.y, max.y);
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> swap(const T& x, const T& y, bool swap) {
	return swap ? glm::vec<2, T, Q>(y, x) : glm::vec<2, T, Q>(x, y);
}

template <class T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
T swapBits(T n, uint8 i, uint8 j) {
	T x = ((n >> i) & 1) ^ ((n >> j) & 1);
	return n ^ ((x << i) | (x << j));
}

template <class T>
uint8 numDigits(T num) {
	return num ? uint8(std::log10(float(num > 0 ? num : -num))) + 1 : 1;
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
	T t = std::move(vec.back());
	vec.pop_back();
	return t;
}

template <class T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}
