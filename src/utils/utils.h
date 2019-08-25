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

// forward declaraions and aliases
class BoardObject;
class Button;
class Layout;
class Program;
class ProgState;
class ShaderGUI;

template <class... T> using uptr = std::unique_ptr<T...>;
template <class... T> using sset = std::set<T...>;

using vec2s = cvec2<int16>;
using vec2i = cvec2<int>;
using vec2f = cvec2<float>;
using vec2d = cvec2<double>;
using vec2t = cvec2<sizet>;

using BCall = void (Program::*)(Button*);
using GCall = void (Program::*)(BoardObject*);

// general wrappers

constexpr float PI = float(M_PI);

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
	constexpr Rect(vec2i pos, vec2i size);

	vec2i& pos();
	constexpr vec2i pos() const;
	vec2i& size();
	constexpr vec2i size() const;
	constexpr vec2i end() const;

	bool contain(vec2i point) const;
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify the rect
};

inline constexpr Rect::Rect(int n) :
	SDL_Rect({ n, n, n, n })
{}

inline constexpr Rect::Rect(int x, int y, int w, int h) :
	SDL_Rect({ x, y, w, h })
{}

inline constexpr Rect::Rect(vec2i pos, vec2i size) :
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

inline bool Rect::contain(vec2i point) const {
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
	Texture(GLuint id = 0, vec2i res = 0);
	Texture(SDL_Surface* img);											// for text
	Texture(vec2i size, GLint iform, GLenum pform, const uint8* pix);	// for image

	void close();
	void free();
	GLuint getID() const;
	vec2i getRes() const;
	bool valid() const;

	static Texture loadBlank(const vec3& color = vec3(1.f));
private:
	static GLuint loadGL(vec2i size, GLint iformat, GLenum pformat, GLenum type, const void* pix, GLint wrap, GLint filter);
};

inline Texture::Texture(GLuint id, vec2i res) :
	id(id),
	res(res)
{}

inline void Texture::free() {
	glDeleteTextures(1, &id);
}

inline GLuint Texture::getID() const {
	return id;
}

inline vec2i Texture::getRes() const {
	return res;
}

inline bool Texture::valid() const {
	return res.hasNot(0);
}

inline Texture Texture::loadBlank(const vec3& color) {
	return Texture(loadGL(1, GL_RGB8, GL_BGR, GL_FLOAT, &color, GL_CLAMP_TO_EDGE, GL_NEAREST), 1);
}

// for Object and Widget

class Interactable {
public:
	Interactable() = default;
	virtual ~Interactable() = default;

	virtual void drawTop() const {}
	virtual void onClick(vec2i mPos, uint8 mBut);		// dummy function to have an out-of-line virtual function
	virtual void onHold(vec2i, uint8) {}
	virtual void onDrag(vec2i, vec2i) {}	// mouse move while left button down
	virtual void onUndrag(uint8) {}						// get's called on mouse button up if instance is Scene's capture
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
	static vector<uint16> travelDist(uint16 src, uint16 dlim, uint16 width, uint16 size, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, uint16), uint8 movSize);
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

template <class T>
T lerp(const T& start, const T& end, float factor) {
	return start + (end - start) * factor;
}

inline glm::quat makeQuat(const vec3& rot) {
	return glm::quat(vec3(rot.x, 0.f, 0.f)) * glm::quat(vec3(0.f, rot.y, 0.f)) *glm::quat(vec3(0.f, 0.f, rot.z));	// for unknown reasons the glm::quat euler constructor doesn't work as expected
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
