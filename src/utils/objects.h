#pragma once

#include "oven/oven.h"
#include "utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// vertex data that's shared between objects
class Mesh {
public:
	static constexpr GLenum elemType = GL_UNSIGNED_SHORT;
private:
	static constexpr uint psiz = 3, pofs = 0;
	static constexpr uint nsiz = 3, nofs = 3;
	static constexpr uint tsiz = 2, tofs = 6;

	GLuint vao, vbo, ebo;
	uint16 ecnt;	// number of elements
	uint8 shape;	// GLenum for how to handle elements

public:
	Mesh();
	Mesh(const vector<Vertex>& vertices, const vector<uint16>& elements, uint8 shape = GL_TRIANGLES);

	void free();
	GLuint getVao() const;
	uint16 getEcnt() const;
	uint8 getShape() const;
};

inline GLuint Mesh::getVao() const {
	return vao;
}

inline uint16 Mesh::getEcnt() const {
	return ecnt;
}

inline uint8 Mesh::getShape() const {
	return shape;
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
	const Mesh* mesh;
	const Material* matl;
	GLuint tex;
	bool show;		// if drawn
	bool rigid;		// if affected by raycast
private:
	vec3 pos, scl;
	quat rot;
	mat4 trans;
	mat3 normat;

public:
	DCLASS_CONSTRUCT(Object, Interactable)
	Object(const vec3& pos, const vec3& ert = vec3(0.f), const vec3& scl = vec3(1.f), const Mesh* mesh = nullptr, const Material* matl = nullptr, GLuint tex = 0, bool rigid = false, bool show = true);

	virtual void draw() const;

	const vec3& getPos() const;
	void setPos(const vec3& vec);
	const quat& getRot() const;
	void setRot(const quat& qut);
	const vec3& getScl() const;
	void setScl(const vec3& vec);
protected:
	const mat4& getTrans() const;
	const mat3& getNormat() const;

	static void updateColor(const vec3& diffuse, const vec3& specular, float shininess, float alpha, GLuint texture);
	static void updateTransform(const mat4& model, const mat3& norm);
	static void setTransform(mat4& model, const vec3& pos, const quat& rot, const vec3& scl);
	static void setTransform(mat4& model, mat3& norm, const vec3& pos, const quat& rot, const vec3& scl);
};

inline const vec3& Object::getPos() const {
	return pos;
}

inline void Object::setPos(const vec3& vec) {
	setTransform(trans, pos = vec, rot, scl);
}

inline const quat& Object::getRot() const {
	return rot;
}

inline void Object::setRot(const quat& qut) {
	setTransform(trans, normat, pos, rot = qut, scl);
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

inline const mat3& Object::getNormat() const {
	return normat;
}

inline void Object::setTransform(mat4& model, const vec3& pos, const quat& rot, const vec3& scl) {
	model = glm::scale(glm::translate(mat4(1.f), pos) * glm::mat4_cast(rot), scl);
}

// square object on a single plane
class BoardObject : public Object {
public:
	enum Emission : uint8 {
		EMI_NONE = 0,
		EMI_DIM  = 1,
		EMI_SEL  = 2,
		EMI_HIGH = 4
	};

	GCall hgcall, ulcall, urcall;

protected:
	static const vec3 moveIconColor;

private:
	float diffuseFactor;
	Emission emission;

public:
	DCLASS_CONSTRUCT(BoardObject, Object)
	BoardObject(const vec3& pos, float rot = 0.f, const vec3& scl = vec3(1.f), GCall hgcall = nullptr, GCall ulcall = nullptr, GCall urcall = nullptr, const Mesh* mesh = nullptr, const Material* matl = nullptr, GLuint tex = 0, bool rigid = true, bool show = true);

	virtual void draw() const override;

	Emission getEmission() const;
	void setEmission(Emission emi);
	void setRaycast(bool on, bool dim = false);
protected:
	void drawTopMesh(float ypos, const Mesh* tmesh, const vec3& tdiffuse, GLuint ttexture) const;
};
ENUM_OPERATIONS(BoardObject::Emission, uint8)

inline BoardObject::Emission BoardObject::getEmission() const {
	return emission;
}

// piece of terrain
class Tile : public BoardObject {
public:
	enum class Interact : uint8 {
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

	virtual void drawTop() const override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	void setTypeSilent(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interact lvl, bool dim = false);
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
	uint16 home, extra, size;	// home = number of home tiles, extra = home + board width, size = all tiles

public:
	TileCol();
	~TileCol();

	void update(const Com::Config& conf);
	uint16 getHome() const;
	uint16 getExtra() const;
	uint16 getSize() const;

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

inline uint16 TileCol::getHome() const {
	return home;
}

inline uint16 TileCol::getExtra() const {
	return extra;
}

inline uint16 TileCol::getSize() const {
	return size;
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
	bool drawTopSelf;

	static const vec3 fireIconColor, attackHorseColor;

public:
	DCLASS_CONSTRUCT(Piece, BoardObject)
	Piece(const vec3& pos, float rot, float size, Com::Piece type, GCall hgcall, GCall ulcall, GCall urcall, const Material* matl, bool rigid, bool show);

	virtual void drawTop() const override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;

	Com::Piece getType() const;
	uint8 firingDistance() const;	// 0 if non-firing piece
	void setActive(bool on);
	void updatePos(svec2 bpos = svec2(UINT16_MAX), bool active = false);
	bool getDrawTopSelf() const;
};

inline Com::Piece Piece::getType() const {
	return type;
}

inline uint8 Piece::firingDistance() const {
	return type >= Com::Piece::crossbowman && type <= Com::Piece::trebuchet ? uint8(type) - uint8(Com::Piece::crossbowman) + 1 : 0;
}

inline void Piece::setActive(bool on) {
	show = rigid = on;
}

inline bool Piece::getDrawTopSelf() const {
	return drawTopSelf;
}

// pieces on a board
class PieceCol {
private:
	Piece* pc;
	uint16 num, size;	// num = number of one player's pieces, i.e. size / 2

public:
	PieceCol();
	~PieceCol();

	void update(const Com::Config& conf);
	uint16 getNum() const;
	uint16 getSize() const;

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

inline uint16 PieceCol::getNum() const {
	return num;
}

inline uint16 PieceCol::getSize() const {
	return size;
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

enum class FavorAct : uint8 {
	off,
	on,
	now
};
