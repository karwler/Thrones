#pragma once

#include "utils.h"

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	float shine;

	Material(const vec4& ambient = vec4(0.f, 0.f, 0.f, 1.f), const vec4& diffuse = vec4(0.f, 0.f, 0.f, 1.f), const vec4& specular = vec4(0.f, 0.f, 0.f, 1.f), const vec4& emission = vec4(0.f, 0.f, 0.f, 1.f), float shine = 0.f);

	void updateColor() const;
	static void updateColor(const vec4& ambient, const vec4& diffuse, const vec4& specular, const vec4& emission, float shine);
};

inline void Material::updateColor() const {
	updateColor(ambient, diffuse, specular, emission, shine);
}

// indices of vertex, uv, normal for a point
struct Vertex {
	static constexpr uint8 size = 3;

	ushort v, t, n;

	Vertex() = default;
	Vertex(ushort v, ushort t, ushort n);

	ushort& operator[](uint8 i);
	ushort operator[](uint8 i) const;
	ushort* begin();
	const ushort* begin() const;
	ushort* end();
	const ushort* end() const;
};

inline ushort& Vertex::operator[](uint8 i) {
	return reinterpret_cast<ushort*>(this)[i];
}

inline ushort Vertex::operator[](uint8 i) const {
	return reinterpret_cast<const ushort*>(this)[i];
}

inline ushort* Vertex::begin() {
	return reinterpret_cast<ushort*>(this);
}

inline const ushort* Vertex::begin() const {
	return reinterpret_cast<const ushort*>(this);
}

inline ushort* Vertex::end() {
	return reinterpret_cast<ushort*>(this) + size;
}

inline const ushort* Vertex::end() const {
	return reinterpret_cast<const ushort*>(this) + size;
}

// vertex data that's shared between objects
struct Blueprint {
	vector<vec3> verts;
	vector<vec2> tuvs;		// must contain at least one dummy element
	vector<vec3> norms;		// ^
	vector<Vertex> elems;	// size must be a multiple of 3 if not INFO_LINES, otherwise 2

	Blueprint() = default;
	Blueprint(vector<vec3> verts, vector<vec2> tuvs, vector<vec3> norms, vector<Vertex> elems);

	void draw(GLenum mode = GL_TRIANGLES) const;
	bool empty() const;
};

inline bool Blueprint::empty() const {
	return elems.empty() || verts.empty() || tuvs.empty() || norms.empty();
}

#define DCLASS_CONSTRUCT(Class, Base) \
	Class() = default; \
	Class(const Class& o) : \
		Base() \
	{ \
		std::copy_n(reinterpret_cast<const uint8*>(&o), sizeof(Class), reinterpret_cast<uint8*>(this)); \
	} \
	Class(Class&& o) : \
		Base() \
	{ \
		std::copy_n(reinterpret_cast<const uint8*>(&o), sizeof(Class), reinterpret_cast<uint8*>(this)); \
	} \
	Class& operator=(const Class& o) { \
		return reinterpret_cast<Class*>(std::copy_n(reinterpret_cast<const uint8*>(&o), sizeof(Class), reinterpret_cast<uint8*>(this)))[-1]; \
	} \
	Class& operator=(Class&& o) { \
		return reinterpret_cast<Class*>(std::copy_n(reinterpret_cast<const uint8*>(&o), sizeof(Class), reinterpret_cast<uint8*>(this)))[-1]; \
	} \
	virtual ~Class() override = default;

// 3D object with triangles
class Object : public Interactable {
public:
	enum Info : uint8 {
		INFO_NONE    = 0x0,
		INFO_SHOW    = 0x1,	// draw at all
		INFO_TEXTURE = 0x2,	// draw texture
		INFO_LINES   = 0x4,	// draw lines
		INFO_RAYCAST = 0x8,	// be detected by raycast
		INFO_FILL    = INFO_SHOW | INFO_TEXTURE
	};

	static const vec4 defaultColor;

	const Blueprint* bpr;
	const Material* mat;
	const Texture* tex;
	vec3 pos, rot, scl;
	Info mode;

public:
	DCLASS_CONSTRUCT(Object, Interactable)
	Object(const vec3& pos, const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const Blueprint* bpr = nullptr, const Material* mat = nullptr, const Texture* tex = nullptr, Info mode = INFO_FILL);

	virtual void draw() const;

protected:
	void updateTransform() const;
	static void updateTransform(const vec3& pos, const vec3& rot, const vec3& scl);
	void updateTexture() const;
	static void updateTexture(const Texture* tex, Info mode);
};
ENUM_OPERATIONS(Object::Info, uint8)

inline void Object::updateTransform() const {
	updateTransform(pos, rot, scl);
}

inline void Object::updateTexture() const {
	updateTexture(tex, mode);
}

// square object on a single plane
class BoardObject : public Object {
public:
	static constexpr float upperPoz = 0.001f;

	float diffuseFactor;
	float emissionFactor;
	OCall hgcall, ulcall, urcall;

private:
	static const vec4 moveIconColor, fireIconColor;
	static const vec4 emissionSelect;

	enum class DragState : uint8 {
		none,
		move,
		fire
	} dragState;

public:
	DCLASS_CONSTRUCT(BoardObject, Object)
	BoardObject(const vec3& pos, float size = 1.f, OCall hgcall = nullptr, OCall ulcall = nullptr, OCall urcall = nullptr, const Material* mat = nullptr, const Texture* tex = nullptr, Info mode = INFO_FILL | INFO_RAYCAST);

	virtual void draw() const override;
	virtual void drawTop() const override;
	virtual void onHold(vec2i mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	void setRaycast(bool on, bool dim = false);
};

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Interactivity : uint8 {
		ignore,
		recognize,
		interact
	};

private:
	Com::Tile type;
	bool breached;	// only for fortress

public:
	DCLASS_CONSTRUCT(Tile, BoardObject)
	Tile(const vec3& pos, float size, Com::Tile type, OCall hgcall, OCall ulcall, OCall urcall, Info mode);

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool isPenetrable() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interactivity lvl, bool dim = false);
private:
	static Info getModeShow(Info mode, Com::Tile type);
};

inline Com::Tile Tile::getType() const {
	return type;
}

inline bool Tile::isBreachedFortress() const {
	return type == Com::Tile::fortress && breached;
}

inline bool Tile::isUnbreachedFortress() const {
	return type == Com::Tile::fortress && !breached;
}

inline bool Tile::isPenetrable() const {
	return type != Com::Tile::fortress || breached;
}

inline bool Tile::getBreached() const {
	return breached;
}

inline Object::Info Tile::getModeShow(Info mode, Com::Tile type) {
	return type != Com::Tile::empty ? mode | INFO_SHOW : mode & ~INFO_SHOW;
}

// tiles on a board
class TileCol {
private:
	Tile* tl;
	uint16 home, extra, size;

public:
	TileCol() = default;
	TileCol(const Com::Config& conf);
	~TileCol();

	void update(const Com::Config& conf);

	Tile& operator[](uint16 i);
	const Tile& operator[](uint16 i) const;
	Tile* begin();
	const Tile* begin() const;
	Tile* end();
	const Tile* end() const;
	Tile* ene(pdift i = 0);
	const Tile* ene(pdift i = 0) const;
	Tile* mid(pdift i = 0);
	const Tile* mid(pdift i = 0) const;
	Tile* own(pdift i = 0);
	const Tile* own(pdift i = 0) const;
};

inline TileCol::~TileCol() {
	delete[] tl;
}

inline Tile& TileCol::operator[](uint16 i) {
	return tl[i];
}

inline const Tile& TileCol::operator[](uint16 i) const {
	return tl[i];
}

inline Tile* TileCol::begin() {
	return tl;
}

inline const Tile* TileCol::begin() const {
	return tl;
}

inline Tile* TileCol::end() {
	return tl + size;
}

inline const Tile* TileCol::end() const {
	return tl + size;
}

inline Tile* TileCol::ene(pdift i) {
	return tl + i;
}

inline const Tile* TileCol::ene(pdift i) const {
	return tl + i;
}

inline Tile* TileCol::mid(pdift i) {
	return tl + home + i;
}

inline const Tile* TileCol::mid(pdift i) const {
	return tl + home + i;
}

inline Tile* TileCol::own(pdift i) {
	return tl + extra + i;
}

inline const Tile* TileCol::own(pdift i) const {
	return tl + extra + i;
}

// player on tiles
class Piece : public BoardObject {
public:
	uint16 lastFortress;	// index of last visited fortress (only relevant to throne)
private:
	Com::Piece type;

public:
	DCLASS_CONSTRUCT(Piece, BoardObject)
	Piece(const vec3& pos, float size, Com::Piece type, OCall hgcall, OCall ulcall, OCall urcall, const Material* mat, Info mode);

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Com::Piece getType() const;
	void setType(Com::Piece newType);
	void setModeByOn(bool on);
	bool canFire() const;
	bool active() const;
	void enable(vec2s bpos);
	void disable();
};

inline Com::Piece Piece::getType() const {
	return type;
}

inline void Piece::setModeByOn(bool on) {
	on ? mode |= INFO_SHOW | INFO_RAYCAST : mode &= ~(INFO_SHOW | INFO_RAYCAST);
}

inline bool Piece::canFire() const {
	return type >= Com::Piece::crossbowman && type <= Com::Piece::trebuchet;
}

// pieces on a board
class PieceCol {
private:
	Piece* pc;
	uint16 num, size;

public:
	PieceCol() = default;
	PieceCol(const Com::Config& conf);
	~PieceCol();

	void update(const Com::Config& conf);

	Piece& operator[](uint16 i);
	const Piece& operator[](uint16 i) const;
	Piece* begin();
	const Piece* begin() const;
	Piece* end();
	const Piece* end() const;
	Piece* own(pdift i = 0);
	const Piece* own(pdift i = 0) const;
	Piece* ene(pdift i = 0);
	const Piece* ene(pdift i = 0) const;
};

inline PieceCol::~PieceCol() {
	delete[] pc;
}

inline Piece& PieceCol::operator[](uint16 i) {
	return pc[i];
}

inline const Piece& PieceCol::operator[](uint16 i) const {
	return pc[i];
}

inline Piece* PieceCol::begin() {
	return pc;
}

inline const Piece* PieceCol::begin() const {
	return pc;
}

inline Piece* PieceCol::end() {
	return pc + size;
}

inline const Piece* PieceCol::end() const {
	return pc + size;
}

inline Piece* PieceCol::own(pdift i) {
	return pc + i;
}

inline const Piece* PieceCol::own(pdift i) const {
	return pc + i;
}

inline Piece* PieceCol::ene(pdift i) {
	return pc + num + i;
}

inline const Piece* PieceCol::ene(pdift i) const {
	return pc + num + i;
}
