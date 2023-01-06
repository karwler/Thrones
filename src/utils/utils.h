#pragma once

#include "text.h"
#include "oven/oven.h"

enum class UserCode : int32 {
	versionFetch
};

// general wrappers

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b);
void checkFramebufferStatus(string_view name);
pair<SDL_Surface*, GLenum> pickPixFormat(SDL_Surface* img);
void invertImage(SDL_Surface* img);
SDL_Surface* takeScreenshot(ivec2 res);

inline bool operator==(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	return a.format == b.format && a.w == b.w && a.h == b.h && a.refresh_rate == b.refresh_rate;
}

namespace glm {

template <class T, qualifier Q>
bool operator<(const vec<2, T, Q>& a, const vec<2, T, Q>& b) {
	return a.y < b.y || (a.y == b.y && a.x < b.x);
}

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
	constexpr ivec2 pos() const;
	ivec2& size();
	constexpr ivec2 size() const;
	constexpr ivec2 end() const;

	bool contains(ivec2 point) const;
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

constexpr ivec2 Rect::pos() const {
	return ivec2(x, y);
}

inline ivec2& Rect::size() {
	return reinterpret_cast<ivec2*>(this)[1];
}

constexpr ivec2 Rect::size() const {
	return ivec2(w, h);
}

constexpr ivec2 Rect::end() const {
	return pos() + size();
}

inline bool Rect::contains(ivec2 point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// SDL_FRect wrapper
struct Rectf : SDL_FRect {
	Rectf() = default;
	constexpr Rectf(float n);
	constexpr Rectf(float px, float py, float sw, float sh);
	constexpr Rectf(float px, float py, vec2 size);
	constexpr Rectf(vec2 pos, float sw, float sh);
	constexpr Rectf(vec2 pos, vec2 size);
	constexpr Rectf(const Rect& rect);

	vec2& pos();
	constexpr vec2 pos() const;
	vec2& size();
	constexpr vec2 size() const;
	constexpr vec2 end() const;
	float* data();
	const float* data() const;
};

constexpr Rectf::Rectf(float n) :
	SDL_FRect{ n, n, n, n }
{}

constexpr Rectf::Rectf(float px, float py, float sw, float sh) :
	SDL_FRect{ px, py, sw, sh }
{}

constexpr Rectf::Rectf(float px, float py, vec2 size) :
	SDL_FRect{ px, py, size.x, size.y }
{}

constexpr Rectf::Rectf(vec2 pos, float sw, float sh) :
	SDL_FRect{ pos.x, pos.y, sw, sh }
{}

constexpr Rectf::Rectf(vec2 pos, vec2 size) :
	SDL_FRect{ pos.x, pos.y, size.x, size.y }
{}

constexpr Rectf::Rectf(const Rect& rect) :
	SDL_FRect{ float(rect.x), float(rect.y), float(rect.w), float(rect.h) }
{}

inline vec2& Rectf::pos() {
	return *reinterpret_cast<vec2*>(this);
}

constexpr vec2 Rectf::pos() const {
	return vec2(x, y);
}

inline vec2& Rectf::size() {
	return reinterpret_cast<vec2*>(this)[1];
}

constexpr vec2 Rectf::size() const {
	return vec2(w, h);
}

constexpr vec2 Rectf::end() const {
	return pos() + size();
}

inline float* Rectf::data() {
	return reinterpret_cast<float*>(this);
}

inline const float* Rectf::data() const {
	return reinterpret_cast<const float*>(this);
}

// texture location in TextureCol
struct TexLoc {
	uint tid;
	Rect rct;

	constexpr TexLoc(uint page = 0, const Rect& posize = Rect(0, 0, 2, 2));

	constexpr bool blank() const;
};

constexpr TexLoc::TexLoc(uint page, const Rect& posize) :
	tid(page),
	rct(posize)
{}

constexpr bool TexLoc::blank() const {
	return !(tid || rct.x || rct.y);
}

// texture collection for widgets
class TextureCol {
public:
#ifdef OPENGLES
	static constexpr GLenum textPixFormat = GL_RGBA;
	static constexpr SDL_PixelFormatEnum shotImgFormat = SDL_PIXELFORMAT_RGBA32;
#else
	static constexpr GLenum textPixFormat = GL_BGRA;
	static constexpr SDL_PixelFormatEnum shotImgFormat = SDL_PIXELFORMAT_BGRA32;
#endif
	static constexpr TexLoc blank = TexLoc(0, Rect(0, 0, 2, 2));

	struct Element {
		string name;
		SDL_Surface* image;
		GLenum format;

		Element(string&& texName, SDL_Surface* img, GLenum pformat);
	};

private:
	struct Less {
		bool operator()(const Rect& a, const Rect& b) const;
	};

	struct Find {
		uint page, index;
		Rect rect;

		Find() = default;
		Find(uint pg, uint id, const Rect& posize);
	};

	static constexpr uint pageNumStep = 4;

	GLuint tex = 0;
#ifdef OPENGLES
	GLuint fbo = 0;
#endif
	int res;
	uint pageReserve;
	vector<vector<Rect>> pages;

public:
	umap<string, TexLoc> init(vector<Element>&& imgs, int recommendSize, int minUiIcon);
	void free();

	int getRes() const;
	TexLoc insert(SDL_Surface* img);
	TexLoc insert(const SDL_Surface* img, ivec2 size, ivec2 offset = ivec2(0));
	void replace(TexLoc& loc, SDL_Surface* img);
	void replace(TexLoc& loc, const SDL_Surface* img, ivec2 size, ivec2 offset = ivec2(0));
	void erase(TexLoc& loc);

	vector<SDL_Surface*> peekMemory() const;
	vector<SDL_Surface*> peekTexture() const;
private:
	void uploadSubTex(const SDL_Surface* img, ivec2 offset, const Rect& loc, uint page);
	pair<vector<Rect>::iterator, bool> findReplaceable(const TexLoc& loc, ivec2& size);	// returns previous location and whether it can be replaced
	Find findLocation(ivec2 size) const;
	void maybeResize();
	void calcPageReserve();
	vector<uint32> texturePixels() const;

	void assertIntegrity();	// TODO: remove
	static void assertIntegrity(const vector<Rect>& page, const Rect& nrect);	// TODO: remove
};

inline bool TextureCol::Less::operator()(const Rect& a, const Rect& b) const {
	return a.y < b.y || (a.y == b.y && a.x < b.x);
}

inline int TextureCol::getRes() const {
	return res;
}

inline void TextureCol::calcPageReserve() {
	pageReserve = (pages.size() / pageNumStep + 1) * pageNumStep;
}

// texture wrapper for 3D objects
class TextureSet {
public:
	struct Element {
		SDL_Surface* color;
		SDL_Surface* normal;
		string name;
		GLenum cformat, nformat;
		bool unique;

		Element(SDL_Surface* iclr, SDL_Surface* inrm, string&& texName, GLenum cfmt, GLenum nfmt, bool owned);

		void clear();
	};

	struct Import {
		vector<Element> imgs;
		array<pair<SDL_Surface*, GLenum>, 6> sky;	// image and format
		int cres = 2, nres = 2;
	};

private:
	GLuint clr = 0, nrm = 0, sky = 0;
	umap<string, uvec2> refs;

public:
	void init(Import&& imp);
	void free();

	uvec2 get(const string& name = string()) const;

private:
	template <GLenum active> static void loadTexArray(GLuint& tex, const Import& imp, int res, uint num, SDL_Surface* Element::*img, GLenum Element::*format, array<uint8, 4> blank);
};

inline uvec2 TextureSet::get(const string& name) const {
	return refs.at(name);
}

// size of a widget
struct Size {
	enum Mode : uint8 {
		rela,	// relative to parent size
		abso,	// relative to widow height
		pref,	// reference to variable pixel value
		pixv,	// absolute pixel value (mainly for 0)
		calc	// use the calculation function
	};

	union {
		float rel;
		const int* ptr;
		int pix;
		int (*cfn)(const Widget*);
	};
	Mode mod;

	constexpr Size(float size = 1.f, Mode mode = rela);
	constexpr Size(const int* iptr);
	constexpr Size(int pixa);
	constexpr Size(int (*cfunc)(const Widget*));

	int operator()(const Widget* wgt) const;
};

constexpr Size::Size(float size, Mode mode) :
	rel(size),
	mod(mode)
{}

constexpr Size::Size(const int* iptr) :
	ptr(iptr),
	mod(pref)
{}

constexpr Size::Size(int pixa) :
	pix(pixa),
	mod(pixv)
{}

constexpr Size::Size(int (*cfunc)(const Widget*)) :
	cfn(cfunc),
	mod(calc)
{}

inline int Size::operator()(const Widget* wgt) const {
	return cfn(wgt);
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

// base for user interactable widgets and objects
class Interactable {
public:
	Interactable() = default;
	Interactable(const Interactable&) = default;
	Interactable(Interactable&&) = default;
	virtual ~Interactable() = default;

	Interactable& operator=(const Interactable&) = default;
	Interactable& operator=(Interactable&&) = default;

	virtual void tick(float) {}
	virtual void onClick(ivec2, uint8) {}
	virtual void onHold(ivec2, uint8) {}
	virtual void onDrag(ivec2, ivec2) {}	// mouse move while left button down
	virtual void onUndrag(ivec2, uint8) {}	// gets called on mouse button up if instance is Scene's capture
	virtual void onHover() {}
	virtual void onUnhover() {}
	virtual void onScroll(ivec2, ivec2) {}
	virtual void onKeyDown(const SDL_KeyboardEvent&) {}
	virtual void onKeyUp(const SDL_KeyboardEvent&) {}
	virtual void onCompose(string_view, sizet) {}
	virtual void onText(string_view, sizet) {}
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

// audio file data
struct Sound {
	uint8* data;
	uint32 length;

	bool convert(const SDL_AudioSpec& srcs, const SDL_AudioSpec& dsts);
	void free();
};

inline void Sound::free() {
	SDL_FreeWAV(data);
}

// math

uint32 ceilPower2(uint32 val);

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
