#pragma once

#include "oven/oven.h"
#include "utils.h"

// vertex data for raycast detection
class CMesh {
public:
	uint16* elems;
	vec3* verts;
	uint16 esiz;

private:
	static constexpr uint16 defaultElementSize = 6;
	static constexpr uint16 defaultVertexSize = 4;
	static constexpr uint16 defaultElements[defaultElementSize] = {
		0, 1, 2,
		0, 2, 3
	};
	static const vec3 defaultVertices[defaultVertexSize];

public:
	CMesh();
	CMesh(uint16 ec, uint16 vc);

	void free();
	static CMesh makeDefault(const vec3& ofs = vec3(0.f));
};

// vertex data that's shared between objects
class GMesh {
public:
	GLuint vao;
	uint16 ecnt;	// number of elements
	uint8 shape;	// GLenum for how to handle elements

	static constexpr GLenum elemType = GL_UNSIGNED_SHORT;
private:
	static constexpr uint psiz = 3, pofs = 0;
	static constexpr uint nsiz = 3, nofs = 3;
	static constexpr uint tsiz = 2, tofs = 6;

public:
	GMesh();
	GMesh(const vector<float>& vertices, const vector<uint16>& elements, uint8 shape = GL_TRIANGLES);

	void free();
	static void setVertex(float* verts, uint16 id, const vec3& pos, const vec3& nrm, const vec2& tuv);
};

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
	static const vec4 defaultColor;

	const GMesh* mesh;
	const Material* matl;
	const CMesh* coli;
	GLuint tex;
	bool show;		// if drawn
	bool rigid;		// if affected by raycast
private:
	vec3 pos, rot, scl;
	mat4 trans;
	mat3 normat;

public:
	DCLASS_CONSTRUCT(Object, Interactable)
	Object(const vec3& pos, const vec3& rot = vec3(0.f), const vec3& scl = vec3(1.f), const GMesh* mesh = nullptr, const Material* matl = nullptr, GLuint tex = 0, const CMesh* coli = nullptr, bool rigid = false, bool show = true);

	virtual void draw() const;

	const vec3& getPos() const;
	void setPos(const vec3& vec);
	const vec3& getRot() const;
	void setRot(const vec3& vec);
	const vec3& getScl() const;
	void setScl(const vec3& vec);
	const mat4& getTrans() const;

protected:
	void updateColor() const;
	static void updateColor(const vec3& diffuse, const vec3& specular, const vec3& emission, float shininess, float alpha, GLuint texture);
	void updateTransform() const;
	static void updateTransform(const vec3& pos, const vec3& rot, const vec3& scl);
	static void setTransform(mat4& model, mat3& norm, const vec3& pos, const vec3& rot, const vec3& scl);
};

inline const vec3& Object::getPos() const {
	return pos;
}

inline void Object::setPos(const vec3& vec) {
	setTransform(trans, normat, pos = vec, rot, scl);
}

inline const vec3& Object::getRot() const {
	return rot;
}

inline void Object::setRot(const vec3& vec) {
	setTransform(trans, normat, pos, rot = vec, scl);
}

inline const vec3& Object::getScl() const {
	return scl;
}

inline void Object::setScl(const vec3& vec) {
	setTransform(trans, normat, pos, rot, scl = vec);
}

inline const mat4& Object::getTrans() const {
	return trans;
}

inline void Object::updateColor() const {
	updateColor(matl->diffuse, matl->specular, matl->emission, matl->shininess, matl->alpha, tex);
}

// square object on a single plane
class BoardObject : public Object {
public:
	static constexpr float upperPoz = 0.001f;
	static constexpr float emissionSelect = 0.1f;

	float diffuseFactor;
	GCall hgcall, ulcall, urcall;
	float emissionFactor;

private:
	static const vec3 moveIconColor, fireIconColor;

	enum class DragState : uint8 {
		none,
		move,
		fire
	} dragState;

public:
	DCLASS_CONSTRUCT(BoardObject, Object)
	BoardObject(const vec3& pos, float rot = 0.f, float size = 1.f, GCall hgcall = nullptr, GCall ulcall = nullptr, GCall urcall = nullptr, const GMesh* mesh = nullptr, const Material* matl = nullptr, GLuint tex = 0, const CMesh* coli = nullptr, bool rigid = true, bool show = true);

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
	Tile(const vec3& pos, float size, Com::Tile type, GCall hgcall, GCall ulcall, GCall urcall, bool rigid, bool show);

	virtual void onHover() override;
	virtual void onUnhover() override;

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	void setTypeSilent(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool isPenetrable() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interactivity lvl, bool dim = false);
private:
	static bool getShow(bool show, Com::Tile type);
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

inline bool Tile::getShow(bool show, Com::Tile type) {
	return show && type != Com::Tile::empty;
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
	Piece(const vec3& pos, float rot, float size, Com::Piece type, GCall hgcall, GCall ulcall, GCall urcall, const Material* matl, bool rigid, bool show);

	virtual void onHover() override;
	virtual void onUnhover() override;

	Com::Piece getType() const;
	uint8 firingDistance() const;	// 0 if non-firing piece
	bool active() const;
	void setActive(bool on);
	void enable(vec2s bpos);
	void disable();
};

inline Com::Piece Piece::getType() const {
	return type;
}

inline uint8 Piece::firingDistance() const {
	return type >= Com::Piece::crossbowman && type <= Com::Piece::trebuchet ? uint8(type) - int16(Com::Piece::crossbowman) + 1 : 0;
}

inline void Piece::setActive(bool on) {
	show = rigid = on;
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
