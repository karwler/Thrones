#pragma once

#include "settings.h"
#include "utils.h"
#include "prog/types.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// vertex data that's shared between objects
class Mesh {
public:
	static constexpr GLenum elemType = GL_UNSIGNED_SHORT;

private:
	GLuint vao = 0, vbo = 0, ebo = 0;
	uint16 ecnt = 0;	// number of elements
	uint8 shape = 0;	// GLenum for how to handle elements

public:
	Mesh() = default;
	Mesh(const vector<Vertex>& vertices, const vector<GLushort>& elements, uint8 type = GL_TRIANGLES);

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

// 3D object with triangles
class Object : public Interactable {
public:
	const Mesh* mesh;
	const Material* matl;
	GLuint tex;
	bool show;
	bool rigid;
private:
	vec3 pos, scl;
	quat rot;
	mat4 trans;
	mat3 normat;

public:
	Object() = default;
	Object(const Object&) = default;
	Object(Object&&) = default;
	Object(const vec3& position, const vec3& rotation = vec3(0.f), const vec3& scale = vec3(1.f), const Mesh* model = nullptr, const Material* material = nullptr, GLuint texture = 0, bool visible = true, bool interactive = false);
	~Object() override = default;

	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

#ifndef OPENGLES
	virtual void drawDepth() const;
	virtual void drawTopDepth() const {}
#endif
	void draw() const;

	const vec3& getPos() const;
	void setPos(const vec3& vec);
	const quat& getRot() const;
	void setRot(const quat& qut);
	const vec3& getScl() const;
	void setScl(const vec3& vec);
protected:
	const mat4& getTrans() const;
	const mat3& getNormat() const;

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
		EMI_DIM = 1,
		EMI_SEL = 2,
		EMI_HIGH = 4
	};

	float alphaFactor;
	GCall hgcall = nullptr;
	GCall ulcall = nullptr;
	GCall urcall = nullptr;

	static constexpr float noEngageAlpha = 0.6f;
protected:
	static constexpr float topYpos = 0.1f;
	static constexpr vec4 moveColorFactor = { 1.f, 1.f, 1.f, 9.9f };

private:
	float diffuseFactor = 1.f;
	Emission emission = EMI_NONE;
protected:
	bool leftDrag;

public:
	BoardObject() = default;
	BoardObject(const BoardObject&) = default;
	BoardObject(BoardObject&&) = default;
	BoardObject(const vec3& position, float rotation, float size, const Mesh* model, const Material* material, GLuint texture, bool visible, float alpha);
	~BoardObject() override = default;

	BoardObject& operator=(const BoardObject&) = default;
	BoardObject& operator=(BoardObject&&) = default;

	void draw() const;
	void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	void onKeyDown(const SDL_KeyboardEvent& key) override;
	void onKeyUp(const SDL_KeyboardEvent& key) override;
	void onJButtonDown(uint8 but) override;
	void onJButtonUp(uint8 but) override;
	void onJHatDown(uint8 hat, uint8 val) override;
	void onJHatUp(uint8 hat, uint8 val) override;
	void onJAxisDown(uint8 axis, bool positive) override;
	void onJAxisUp(uint8 axis, bool positive) override;
	void onGButtonDown(SDL_GameControllerButton but) override;
	void onGButtonUp(SDL_GameControllerButton but) override;
	void onGAxisDown(SDL_GameControllerAxis axis, bool positive) override;
	void onGAxisUp(SDL_GameControllerAxis axis, bool positive) override;
	void onNavSelect(Direction dir) override;
	void startKeyDrag(uint8 mBut);

	Emission getEmission() const;
	virtual void setEmission(Emission emi);
protected:
#ifndef OPENGLES
	void drawTopMeshDepth(float ypos) const;
#endif
	void drawTopMesh(float ypos, const Mesh* tmesh, const Material& tmatl, GLuint ttexture) const;
private:
	void onInputDown(Binding::Type bind);
	void onInputUp(Binding::Type bind);
};

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
	TileType type = TileType::empty;
	bool breached = false;	// only for fortress

public:
	Tile() = default;
	Tile(const Tile&) = default;
	Tile(Tile&&) = default;
	Tile(const vec3& position, float size, bool visible);
	~Tile() final = default;

	Tile& operator=(const Tile&) = default;
	Tile& operator=(Tile&&) = default;

#ifndef OPENGLES
	void drawDepth() const final;
	void drawTopDepth() const final;
#endif
	void drawTop() const final;
	void onHold(const ivec2& mPos, uint8 mBut) final;
	void onUndrag(uint8 mBut) final;
	void onHover() final;
	void onUnhover() final;
	void onCancelCapture() final;

	TileType getType() const;
	void setType(TileType newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interact lvl, bool dim = false);
	void setEmission(Emission emi) final;
private:
	const char* pickMesh();
};

inline const char* Tile::pickMesh() {
	return type != TileType::fortress ? "tile" : breached ? "breached" : "fortress";
}

inline TileType Tile::getType() const {
	return type;
}

inline bool Tile::isBreachedFortress() const {
	return type == TileType::fortress && breached;
}

inline bool Tile::isUnbreachedFortress() const {
	return type == TileType::fortress && !breached;
}

inline bool Tile::getBreached() const {
	return breached;
}

// tile container
class TileCol {
private:
	uptr<Tile[]> tl;
	uint16 home = 0;	// number of home tiles
	uint16 extra = 0;	// home + board width
	uint16 size = 0;	// all tiles

public:
	void update(const Config& conf);
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
	return tl.get();
}

inline const Tile* TileCol::begin() const {
	return tl.get();
}

inline Tile* TileCol::end() {
	return &tl[size];
}

inline const Tile* TileCol::end() const {
	return &tl[size];
}

inline Tile* TileCol::ene(pdift i) {
	return &tl[i];
}

inline const Tile* TileCol::ene(pdift i) const {
	return &tl[i];
}

inline Tile* TileCol::mid(pdift i) {
	return &tl[home+i];
}

inline const Tile* TileCol::mid(pdift i) const {
	return &tl[home+i];
}

inline Tile* TileCol::own(pdift i) {
	return &tl[extra+i];
}

inline const Tile* TileCol::own(pdift i) const {
	return &tl[extra+i];
}

// player on tiles
class Piece : public BoardObject {
public:
	uint16 lastFortress = UINT16_MAX;	// index of last visited fortress (only relevant to throne)
private:
	PieceType type;

public:
	Piece() = default;
	Piece(const Piece&) = default;
	Piece(Piece&&) = default;
	Piece(const vec3& position, float rotation, float size, const Material* material);
	~Piece() final = default;

	Piece& operator=(const Piece&) = default;
	Piece& operator=(Piece&&) = default;

#ifndef OPENGLES
	void drawTopDepth() const final;
#endif
	void drawTop() const final;
	void onHold(const ivec2& mPos, uint8 mBut) final;
	void onUndrag(uint8 mBut) final;
	void onHover() final;
	void onUnhover() final;
	void onCancelCapture() final;

	PieceType getType() const;
	void setType(PieceType newType);
	pair<uint8, uint8> firingArea() const;	// 0 if non-firing piece
	void setActive(bool on);
	void updatePos(svec2 bpos = svec2(UINT16_MAX), bool forceRigid = false);
	void setInteractivity(bool on, bool dim, GCall holdCall, GCall leftCall, GCall rightCall);
private:
	float selfTopYpos(const Interactable* occupant) const;
};

inline PieceType Piece::getType() const {
	return type;
}

inline void Piece::setActive(bool on) {
	show = rigid = on;
}

inline float Piece::selfTopYpos(const Interactable* occupant) const {
	return dynamic_cast<const Piece*>(occupant) && occupant != this ? 1.1f * getScl().y : 0.01f;
}

// piece container
class PieceCol {
private:
	uptr<Piece[]> pc;
	uint16 num = 0;		// number of one player's pieces, i.e. size / 2
	uint16 size = 0;	// all pieces

public:
	void update(const Config& conf, bool regular);
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
	return pc.get();
}

inline const Piece* PieceCol::begin() const {
	return pc.get();
}

inline Piece* PieceCol::end() {
	return &pc[size];
}

inline const Piece* PieceCol::end() const {
	return &pc[size];
}

inline Piece* PieceCol::own(pdift i) {
	return &pc[i];
}

inline const Piece* PieceCol::own(pdift i) const {
	return &pc[i];
}

inline Piece* PieceCol::ene(pdift i) {
	return &pc[num+i];
}

inline const Piece* PieceCol::ene(pdift i) const {
	return &pc[num+i];
}
