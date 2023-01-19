#pragma once

#include "settings.h"
#include "utils.h"
#include "prog/types.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// vertex data that's shared between objects
class Mesh {
public:
	struct Instance {
		mat4 model;
		mat3 normat;
		vec4 diffuse;
		uvec3 texid;	// x = color, y = normal, z = material	// TODO: move z to x
		int show;
	};

private:
	struct Top {
		Instance data;
		GLuint vao = 0, ibo = 0;
	};

	vector<Instance> instanceData;
	uptr<Top> top;
	GLuint vao = 0, vbo = 0, ibo = 0, ebo = 0;
	uint16 ecnt = 0;	// number of elements

public:
	void init(const vector<Vertex>& vertices, const vector<GLushort>& elements);
	void free();
	void draw();
	void drawTop();
	void allocate(uint size, bool setTop = false);
	void allocateTop(bool yes);
	uint insert(const Instance& data);
	Instance erase(uint id);
	Instance& getInstance(uint id);
	void updateInstance(uint id, iptrt loffs = 0, sizet size = sizeof(Instance));
	void updateInstanceData();
	bool hasTop() const;
	Instance& getInstanceTop();
	void updateInstanceTop(iptrt loffs = 0, sizet size = sizeof(Instance));
	void updateInstanceDataTop();

private:
	void initTop();
	void freeTop();
	void setVertexAttrib();
	void setInstanceAttrib();
	void disableAttrib();
};

inline Mesh::Instance& Mesh::getInstance(uint id) {
	return instanceData[id];
}

inline bool Mesh::hasTop() const {
	return bool(top);
}

inline Mesh::Instance& Mesh::getInstanceTop() {
	return top->data;
}

// 3D object with triangles
class Object : public Interactable {
protected:
	Mesh* mesh = nullptr;
	uint matl;
private:
	vec3 pos, scl;
	quat rot;
public:
	uint meshIndex;
	bool rigid;

	Object() = default;
	Object(const Object&) = default;
	Object(Object&&) = default;
	Object(const vec3& position, const vec3& rotation = vec3(0.f), const vec3& scale = vec3(1.f), bool interactive = false);
	~Object() override = default;

	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

	void init(Mesh* model, uint id, uint material, uvec2 texture, bool visible = true);
	static void init(Mesh* model, uint id, const vec3& pos, const quat& rot, const vec3& scl, uint material, uvec2 texture, bool visible = true);

	const vec3& getPos() const;
	void setPos(const vec3& vec);
	const quat& getRot() const;
	void setRot(const quat& qut);
	const vec3& getScl() const;
	void setScl(const vec3& vec);
	static mat4 getTransform(const vec3& pos, const quat& rot, const vec3& scl);
	void setTrans(const vec3& position, const quat& rotation);
	void setTrans(const vec3& position, const vec3& scale);
	void setTrans(const quat& rotation, const vec3& scale);
	void setTrans(const vec3& position, const quat& rotation, const vec3& scale);

	Mesh* getMesh();
	void setMaterial(uint material);
	void setTexture(uvec2 texture);
	bool getShow() const;
	void setShow(bool visible);
protected:
	void updateTransform();
	void getTransform(mat4& model, mat3& normat);
};

inline const vec3& Object::getPos() const {
	return pos;
}

inline const quat& Object::getRot() const {
	return rot;
}

inline const vec3& Object::getScl() const {
	return scl;
}

inline mat4 Object::getTransform(const vec3& pos, const glm::quat& rot, const vec3& scl) {
	return glm::scale(glm::translate(mat4(1.f), pos) * glm::mat4_cast(rot), scl);
}

inline Mesh* Object::getMesh() {
	return mesh;
}

inline bool Object::getShow() const {
	return mesh->getInstance(meshIndex).show;
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

	static constexpr float noEngageAlpha = 0.6f;
protected:
	static constexpr vec4 moveColorFactor = vec4(1.f, 1.f, 1.f, 0.9f);

	bool leftDrag;
	bool topSolid;
private:
	Emission emission = EMI_NONE;

public:
	GCall hgcall = nullptr;
	GCall ulcall = nullptr;
	GCall urcall = nullptr;

	BoardObject() = default;
	BoardObject(const BoardObject&) = default;
	BoardObject(BoardObject&&) = default;
	BoardObject(const vec3& position, float rotation, float size);
	~BoardObject() override = default;

	BoardObject& operator=(const BoardObject&) = default;
	BoardObject& operator=(BoardObject&&) = default;

	void init(Mesh* model, uint id, uint material, uvec2 texture, bool visible = true, float alpha = 1.f);
	virtual float getTopY();

	void onDrag(ivec2 mPos, ivec2 mMov) override;
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

	bool getTopSolid() const;
	Emission getEmission() const;
	virtual void setEmission(Emission emi);
	void setAlphaFactor(float alpha);

protected:
	void setTop(vec2 mPos, const mat3& normat, Mesh* tmesh, uint material, const vec4& colorFactor, uvec2 texture, bool solid = true);

private:
	float emissionFactor();

	void onInputDown(Binding::Type bind);
	void onInputUp(Binding::Type bind);
};

inline BoardObject::BoardObject(const vec3& position, float rotation, float size) :
	Object(position, vec3(0.f, rotation, 0.f), vec3(size))
{}

inline bool BoardObject::getTopSolid() const {
	return topSolid;
}

inline BoardObject::Emission BoardObject::getEmission() const {
	return emission;
}

inline float BoardObject::emissionFactor() {
	return (emission & EMI_DIM ? 0.6f : 1.f) + (emission & EMI_SEL ? 0.2f : 0.f) + (emission & EMI_HIGH ? 0.2f : 0.f);
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
	Tile(const vec3& position, float size);
	~Tile() override = default;

	Tile& operator=(const Tile&) = default;
	Tile& operator=(Tile&&) = default;

	void onHold(ivec2 mPos, uint8 mBut) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;
	void onHover() override;
	void onUnhover() override;
	void onCancelCapture() override;

	TileType getType() const;
	void setType(TileType newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interact lvl, bool dim = false);
	void setEmission(Emission emi) override;
private:
	void swapMesh();
};

inline Tile::Tile(const vec3& position, float size) :
	BoardObject(position, 0.f, size)
{}

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
	return &tl[home + i];
}

inline const Tile* TileCol::mid(pdift i) const {
	return &tl[home + i];
}

inline Tile* TileCol::own(pdift i) {
	return &tl[extra + i];
}

inline const Tile* TileCol::own(pdift i) const {
	return &tl[extra + i];
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
	Piece(const vec3& position, float rotation, float size);
	~Piece() override = default;

	Piece& operator=(const Piece&) = default;
	Piece& operator=(Piece&&) = default;

	void init(Mesh* model, uint id, uint material, uvec2 texture, bool visible, PieceType iniType);
	float getTopY() override;

	void onHold(ivec2 mPos, uint8 mBut) override;
	void onUndrag(ivec2 mPos, uint8 mBut) override;
	void onHover() override;
	void onUnhover() override;
	void onCancelCapture() override;

	PieceType getType() const;
	pair<uint8, uint8> firingArea() const;	// 0 if non-firing piece
	void setActive(bool on);
	void updatePos(svec2 bpos = svec2(UINT16_MAX), bool forceRigid = false);
	void setInteractivity(bool on, bool dim, GCall holdCall, GCall leftCall, GCall rightCall);
};

inline Piece::Piece(const vec3& position, float rotation, float size) :
	BoardObject(position, rotation, size)
{}

inline PieceType Piece::getType() const {
	return type;
}

inline void Piece::setActive(bool on) {
	setShow(rigid = on);
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
	return &pc[num + i];
}

inline const Piece* PieceCol::ene(pdift i) const {
	return &pc[num + i];
}
