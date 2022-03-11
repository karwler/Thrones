#pragma once

#include "text.h"
#include "oven/oven.h"

enum class UserCode : int32 {
	versionFetch
};

// general wrappers

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b);
void checkFramebufferStatus(const string& name);

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
void setPtrVec(vector<T*>& vec, vector<T*>&& set = vector<T*>()) {
	std::for_each(vec.begin(), vec.end(), [](T* it) { delete it; });
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
	constexpr Rect(int px, int py, ivec2 size);
	constexpr Rect(ivec2 pos, int sw, int sh);
	constexpr Rect(ivec2 pos, ivec2 size);

	ivec2& pos();
	ivec2 pos() const;
	ivec2& size();
	ivec2 size() const;
	ivec2 end() const;

	bool contain(ivec2 point) const;
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify the rect
};

constexpr Rect::Rect(int n) :
	SDL_Rect{ n, n, n, n }
{}

constexpr Rect::Rect(int px, int py, int sw, int sh) :
	SDL_Rect{ px, py, sw, sh }
{}

constexpr Rect::Rect(int px, int py, ivec2 size) :
	SDL_Rect{ px, py, size.x, size.y }
{}

constexpr Rect::Rect(ivec2 pos, int sw, int sh) :
	SDL_Rect{ pos.x, pos.y, sw, sh }
{}

constexpr Rect::Rect(ivec2 pos, ivec2 size) :
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

inline bool Rect::contain(ivec2 point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// OpenGL texture wrapper for widgets

class Texture {
public:
	static constexpr GLenum texa = GL_TEXTURE0;

#ifdef OPENGLES
	static inline bool bgraSupported = false;
#endif
private:
	GLuint id = 0;
	ivec2 res = ivec2(0, 0);

public:
	Texture() = default;
	Texture(SDL_Surface* img, GLenum pform);	// for images
	Texture(SDL_Surface* img);					// for text
	Texture(array<uint8, 4> color);

	void close();
	void free();

	operator GLuint() const;
	ivec2 getRes() const;
	static pair<SDL_Surface*, GLenum> pickPixFormat(SDL_Surface* img);

	void reload(const SDL_Surface* img, GLenum pform);
private:
	void load(const SDL_Surface* img, GLenum pform, GLint wrap, GLint filter);
	void upload(const SDL_Surface* img, GLenum pform);
};

inline void Texture::free() {
	glDeleteTextures(1, &id);
}

inline Texture::operator GLuint() const {
	return id;
}

inline ivec2 Texture::getRes() const {
	return res;
}

// OpenGL texture wrapper for 3D objects

class TextureSet {
public:
	struct Element {
		SDL_Surface* color;
		SDL_Surface* normal;
		string name;
		GLenum cformat, nformat;

		Element(SDL_Surface* iclr, SDL_Surface* inrm, string&& texName, GLenum cfmt, GLenum nfmt);
	};

	struct Import {
		vector<Element> imgs;
		array<pair<SDL_Surface*, GLenum>, 6> sky;	// image and format
		ivec2 cres = ivec2(0), nres = ivec2(0);
	};

	static constexpr GLenum colorTexa = GL_TEXTURE1;
	static constexpr GLenum normalTexa = GL_TEXTURE2;
	static constexpr GLenum skyboxTexa = GL_TEXTURE3;

private:
	GLuint clr = 0, nrm = 0, sky = 0;
	umap<string, int> refs;

public:
	TextureSet() = default;
	TextureSet(Import&& imp);

	void free();
	operator GLuint() const;
	int get(const string& name = string()) const;

private:
	static void loadTexArray(GLuint& tex, Import& imp, ivec2 res, SDL_Surface* Element::*img, GLenum Element::*format, array<uint8, 4> blank, GLenum active);
};

inline TextureSet::operator GLuint() const {
	return clr;
}

inline int TextureSet::get(const string& name) const {
	return refs.at(name);
}

// Render data group

class FrameSet {
public:
	static constexpr GLenum vposTexa = GL_TEXTURE4;
	static constexpr GLenum normTexa = GL_TEXTURE5;
	static constexpr GLenum ssaoTexa = GL_TEXTURE6;
	static constexpr GLenum blurTexa = GL_TEXTURE7;
	static constexpr GLenum colorTexa = GL_TEXTURE8;
	static constexpr GLenum brightTexa = GL_TEXTURE9;

	GLuint fboGeom = 0, texPosition = 0, texNormal = 0, rboGeom = 0;
	GLuint fboSsao = 0, texSsao = 0;
	GLuint fboBlur = 0, texBlur = 0;
	GLuint fboLight = 0, texColor = 0, texBright = 0, rboLight = 0;
	array<GLuint, 2> fboGauss{}, texGauss{};

	FrameSet(const Settings* sets, ivec2 res);

	void free();

private:
	static GLuint makeTexture(ivec2 res, GLint iform, GLenum pform, GLenum active, GLint filter = GL_NEAREST);
};

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

constexpr Direction::Direction(Dir direction) :
	dir(direction)
{}

constexpr Direction::operator Dir() const {
	return dir;
}

constexpr bool Direction::vertical() const {
	return dir <= down;
}

constexpr bool Direction::horizontal() const {
	return dir >= left;
}

constexpr bool Direction::positive() const {
	return dir & 1;
}

constexpr bool Direction::negative() const {
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
	virtual void onClick(ivec2, uint8) {}
	virtual void onHold(ivec2, uint8) {}
	virtual void onDrag(ivec2, ivec2) {}	// mouse move while left button down
	virtual void onUndrag(uint8) {}						// gets called on mouse button up if instance is Scene's capture
	virtual void onHover() {}
	virtual void onUnhover() {}
	virtual void onScroll(ivec2) {}
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

template <class U, class S, std::enable_if_t<std::is_unsigned_v<U> && std::is_signed_v<S>, int> = 0>
U cycle(U pos, U siz, S mov) {
	U rst = pos + U(mov % S(siz));
	return rst < siz ? rst : mov >= S(0) ? rst - siz : siz + rst;
}
