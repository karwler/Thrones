#pragma once

#include "text.h"
#include "oven/oven.h"

enum class UserCode : int32 {
	versionFetch
};

// general wrappers

constexpr float PI = float(M_PI);

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b);
#ifndef OPENGLES
GLuint makeCubemap(GLsizei res, GLenum active);
void loadCubemap(GLuint tex, GLsizei res, GLenum active);
GLuint makeFramebufferDepth(GLuint tex);
#endif

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

inline void pushEvent(UserCode code, void* data1 = nullptr, void* data2 = nullptr) {
	SDL_Event event;
	event.user = { SDL_USEREVENT, SDL_GetTicks(), 0, int32(code), data1, data2 };
	SDL_PushEvent(&event);
}

#ifdef OPENGLES
inline void glClearDepth(double d) {
	glClearDepthf(float(d));
}
#endif

template <class T>
void setPtrVec(vector<T*>& vec, vector<T*>&& set) {
	for (T* it : vec)
		delete it;
	vec = std::move(set);
}

template <class T, class... A>
vector<T>& uniqueSort(vector<T>& vec, A... sorter) {
	std::sort(vec.begin(), vec.end(), sorter...);
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
	return vec;
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

// SDL_Rect wrapper

struct Rect : SDL_Rect {
	Rect() = default;
	constexpr Rect(int n);
	constexpr Rect(int px, int py, int sw, int sh);
	constexpr Rect(int px, int py, const ivec2& size);
	constexpr Rect(const ivec2& pos, int sw, int sh);
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
	SDL_Rect{ n, n, n, n }
{}

inline constexpr Rect::Rect(int px, int py, int sw, int sh) :
	SDL_Rect{ px, py, sw, sh }
{}

inline constexpr Rect::Rect(int px, int py, const ivec2& size) :
	SDL_Rect{ px, py, size.x, size.y }
{}

inline constexpr Rect::Rect(const ivec2& pos, int sw, int sh) :
	SDL_Rect{ pos.x, pos.y, sw, sh }
{}

inline constexpr Rect::Rect(const ivec2& pos, const ivec2& size) :
	SDL_Rect{ pos.x, pos.y, size.x, size.y }
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
	Texture();
	Texture(SDL_Surface* img);								// for text
	Texture(SDL_Surface* img, GLint iform, GLenum pform);	// for image
	Texture(array<uint8, 3> color);							// load blank

	void close();
	void free();
	operator GLuint() const;
	const ivec2& getRes() const;
	void reload(SDL_Surface* img, GLint iformat, GLenum pformat);
private:
	void load(SDL_Surface* img, GLint iformat, GLenum pformat, GLint wrap, GLint filter);
	void upload(SDL_Surface* img, GLint iformat, GLenum pformat);
};

inline Texture::Texture() :
	id(0),
	res(0)
{}

inline void Texture::free() {
	glDeleteTextures(1, &id);
}

inline Texture::operator GLuint() const {
	return id;
}

inline const ivec2& Texture::getRes() const {
	return res;
}

inline void Texture::reload(SDL_Surface* img, GLint iformat, GLenum pformat) {
	upload(img, iformat, pformat);
	glGenerateMipmap(GL_TEXTURE_2D);
}

// for Object and Widget

class Direction {
public:
	enum Dir : uint8 {
		up,
		down,
		left,
		right
	};

private:
	Dir dir;

public:
	constexpr Direction(Dir direction);

	constexpr operator Dir() const;
	constexpr bool vertical() const;
	constexpr bool horizontal() const;
	constexpr bool positive() const;
	constexpr bool negative() const;
};

inline constexpr Direction::Direction(Dir direction) :
	dir(direction)
{}

inline constexpr Direction::operator Dir() const {
	return dir;
}

inline constexpr bool Direction::vertical() const {
	return dir <= down;
}

inline constexpr bool Direction::horizontal() const {
	return dir >= left;
}

inline constexpr bool Direction::positive() const {
	return dir & 1;
}

inline constexpr bool Direction::negative() const {
	return !positive();
}

class Interactable {
public:
	Interactable() = default;
	Interactable(const Interactable&) = default;
	Interactable(Interactable&&) = default;
	virtual ~Interactable() = default;

	Interactable& operator=(const Interactable&) = default;
	Interactable& operator=(Interactable&&) = default;

	virtual void drawTop() const {}
	virtual void tick(float) {}
	virtual void onClick(const ivec2&, uint8) {}
	virtual void onHold(const ivec2&, uint8) {}
	virtual void onDrag(const ivec2&, const ivec2&) {}	// mouse move while left button down
	virtual void onUndrag(uint8) {}						// gets called on mouse button up if instance is Scene's capture
	virtual void onHover() {}
	virtual void onUnhover() {}
	virtual void onScroll(const ivec2&) {}
	virtual void onKeyDown(const SDL_KeyboardEvent&) {}
	virtual void onKeyUp(const SDL_KeyboardEvent&) {}
	virtual void onText(const char*) {}
	virtual void onJButtonDown(uint8) {}
	virtual void onJButtonUp(uint8) {}
	virtual void onJHatDown(uint8, uint8) {}
	virtual void onJHatUp(uint8, uint8) {}
	virtual void onJAxisDown(uint8, bool) {}
	virtual void onJAxisUp(uint8, bool) {}
	virtual void onGButtonDown(SDL_GameControllerButton) {}
	virtual void onGButtonUp(SDL_GameControllerButton) {}
	virtual void onGAxisDown(SDL_GameControllerAxis, bool) {}
	virtual void onGAxisUp(SDL_GameControllerAxis, bool) {}
	virtual void onNavSelect(Direction dir);
	virtual void onCancelCapture() {}	// should only be called by the Scene when resetting the capture
};

// geometry?

template <class T>
T btom(bool fwd) {
	return T(fwd) * T(2) - T(1);
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
bool inRange(T val, T min, T lim) {
	return val >= min && val < lim;
}

template <class T, glm::qualifier Q, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
bool inRange(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min, const glm::vec<2, T, Q>& lim) {
	return inRange(val.x, min.x, lim.x) && inRange(val.y, min.y, lim.y);
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
bool outRange(T val, T min, T lim) {
	return val < min || val >= lim;
}

template <class T, glm::qualifier Q, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
bool outRange(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min, const glm::vec<2, T, Q>& lim) {
	return outRange(val.x, min.x, lim.x) || outRange(val.y, min.y, lim.y);
}

template <class T, glm::qualifier Q = glm::defaultp, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
glm::vec<2, T, Q> swap(T x, T y, bool swap) {
	return swap ? glm::vec<2, T, Q>(y, x) : glm::vec<2, T, Q>(x, y);
}

template <glm::length_t L, class T, glm::qualifier Q = glm::defaultp, std::enable_if_t<std::is_signed_v<T>, int> = 0>
glm::vec<L, T, Q> deltaSingle(glm::vec<L, T, Q> v) {
	for (glm::length_t i = 0; i < L; ++i)
		if (v[i])
			v[i] = v[i] > T(0) ? T(1) : T(-1);
	return v;
}

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
T swapBits(T n, uint8 i, uint8 j) {
	T x = ((n >> i) & 1) ^ ((n >> j) & 1);
	return n ^ ((x << i) | (x << j));
}
