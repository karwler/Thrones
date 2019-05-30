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
	void updateColorDiffuse(const vec4& difu) const;
	void updateColorEmission(const vec4& emis) const;
	static void updateColor(const vec4& ambient, const vec4& diffuse, const vec4& specular, const vec4& emission, float shine);
};

inline void Material::updateColor() const {
	updateColor(ambient, diffuse, specular, emission, shine);
}

inline void Material::updateColorDiffuse(const vec4& difu) const {
	updateColor(ambient, difu, specular, emission, shine);
}

inline void Material::updateColorEmission(const vec4& emis) const {
	updateColor(ambient, diffuse, specular, emis, shine);
}

// indices of vertex, uv, normal for a point
struct Vertex {
	static constexpr uint8 size = 3;

	ushort v, t, n;

	Vertex() = default;
	Vertex(ushort v, ushort t, ushort n);

	ushort& operator[](uint8 i);
	ushort operator[](uint8 i) const;
};

inline ushort& Vertex::operator[](uint8 i) {
	return reinterpret_cast<ushort*>(this)[i];
}

inline ushort Vertex::operator[](uint8 i) const {
	return reinterpret_cast<const ushort*>(this)[i];
}

// vertex data that's shared between objects
struct Blueprint {
	vector<vec3> verts;
	vector<vec2> tuvs;		// must contain at least one dummy element
	vector<vec3> norms;		// ^
	vector<Vertex> elems;	// size must be a multiple of 3 if not INFO_LINES, otherwise 2

	Blueprint() = default;
	Blueprint(const vector<vec3>& verts, const vector<vec2>& tuvs, const vector<vec3>& norms, const vector<Vertex>& elems);

	void draw(GLenum mode) const;
	bool empty() const;
};

inline bool Blueprint::empty() const {
	return elems.empty() || verts.empty() || tuvs.empty() || norms.empty();
}

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
	Object(const vec3& pos = vec3(0.f), const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const Blueprint* bpr = nullptr, const Material* mat = nullptr, const Texture* tex = nullptr, Info mode = INFO_FILL);
	virtual ~Object() override = default;

	virtual void draw() const;

protected:
	static void updateTransform(const vec3& pos, const vec3& rot, const vec3& scl);
	static void updateTexture(const Texture* tex, Info mode);
};

inline constexpr Object::Info operator~(Object::Info a) {
	return Object::Info(~uint8(a));
}

inline constexpr Object::Info operator&(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) & uint8(b));
}

inline constexpr Object::Info operator&=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) & uint8(b));
}

inline constexpr Object::Info operator^(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) ^ uint8(b));
}

inline constexpr Object::Info operator^=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) ^ uint8(b));
}

inline constexpr Object::Info operator|(Object::Info a, Object::Info b) {
	return Object::Info(uint8(a) | uint8(b));
}

inline constexpr Object::Info operator|=(Object::Info& a, Object::Info b) {
	return a = Object::Info(uint8(a) | uint8(b));
}

// square object on a single plane
class BoardObject : public Object {
public:
	enum class DragState : uint8 {
		none,
		move,
		fire
	} dragState;
	OCall clcall, crcall, hgcall, ulcall, urcall;

	static constexpr float upperPoz = 0.001f;
private:
	static const vec4 moveIconColor, fireIconColor;

public:
	BoardObject(const vec3& pos = vec3(0.f), float size = 1.f, OCall clcall = nullptr, OCall crcall = nullptr, OCall hgcall = nullptr, OCall ulcall = nullptr, OCall urcall = nullptr, const Material* mat = nullptr, const Texture* tex = nullptr, Info mode = INFO_FILL | INFO_RAYCAST);
	virtual ~BoardObject() override = default;

	virtual void draw() const override;
	virtual void drawTop() const override;
	virtual void onClick(const vec2i& mPos, uint8 mBut) override;
	virtual void onHold(const vec2i& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;

	void setModeByInteract(bool on);
};

inline void BoardObject::setModeByInteract(bool on) {
	on ? mode |= INFO_RAYCAST : mode &= ~INFO_RAYCAST;
}

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Interactivity : uint8 {
		none,
		raycast,
		tiling,
		piecing
	};

private:
	Com::Tile type;
	bool breached;	// only for fortress

public:
	Tile() = default;
	Tile(const vec3& pos, float size, Com::Tile type, OCall clcall, OCall crcall, OCall hgcall, OCall ulcall, OCall urcall, Info mode);
	virtual ~Tile() override = default;

	virtual void onText(const string& str) override;	// dummy function to have an out-of-line virtual function

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setCalls(Interactivity lvl);
private:
	static Info getModeByType(Info mode, Com::Tile type);
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

inline bool Tile::getBreached() const {
	return breached;
}

inline Object::Info Tile::getModeByType(Info mode, Com::Tile type) {
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
	Piece() = default;
	Piece(const vec3& pos, float size, Com::Piece type, OCall clcall, OCall crcall, OCall hgcall, OCall ulcall, OCall urcall, const Material* mat, Info mode);
	virtual ~Piece() override = default;

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
