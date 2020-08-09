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
	Mesh(const vector<Vertex>& vertices, const vector<uint16>& elements, uint8 type = GL_TRIANGLES);

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
	bool show;		// if drawn
	bool rigid;		// if affected by raycast
private:
	vec3 pos, scl;
	quat rot;
	mat4 trans;
	mat3 normat;

public:
	Object() = default;
	Object(const Object&) = default;
	Object(Object&&) = default;
	Object(const vec3& position, const vec3& rotation = vec3(0.f), const vec3& scale = vec3(1.f), const Mesh* model = nullptr, const Material* material = nullptr, GLuint texture = 0, bool interactive = false, bool visible = true);
	virtual ~Object() override = default;

	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

#ifndef OPENGLES
	void drawDepth() const;
	virtual void drawTopDepth() const {}
#endif
	virtual void draw() const;
	virtual void drawTop() const {}

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

	GCall hgcall, ulcall, urcall;
	float alphaFactor;

	static constexpr float noEngageAlpha = 0.6f;
protected:
	static constexpr float topYpos = 0.1f;
	static constexpr vec4 moveColorFactor = { 1.f, 1.f, 1.f, 9.9f };

private:
	float diffuseFactor;
	Emission emission;

public:
	BoardObject() = default;
	BoardObject(const BoardObject&) = default;
	BoardObject(BoardObject&&) = default;
	BoardObject(const vec3& position, float rotation, float size, const Mesh* model, const Material* material, GLuint texture, bool visible, float alpha);
	virtual ~BoardObject() override = default;

	BoardObject& operator=(const BoardObject&) = default;
	BoardObject& operator=(BoardObject&&) = default;

	virtual void draw() const override;
	virtual void onDrag(const ivec2& mPos, const ivec2& mMov) override;
	virtual void onKeypress(const SDL_KeyboardEvent& key) override;
	virtual void onKeyrelease(const SDL_KeyboardEvent& key) override;
	virtual void onJButton(uint8 but) override;
	virtual void onJHat(uint8 hat) override;
	virtual void onGButton(SDL_GameControllerButton but) override;
	virtual void onNavSelect(Direction dir) override;
	virtual void cancelDrag() {}
	void startKeyDrag(uint8 mBut);

	Emission getEmission() const;
	virtual void setEmission(Emission emi);
protected:
#ifndef OPENGLES
	void drawTopMeshDepth(float ypos, const Mesh* tmesh) const;
#endif
	void drawTopMesh(float ypos, const Mesh* tmesh, const Material& tmatl, GLuint ttexture) const;
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
	Com::Tile type;
	bool breached;	// only for fortress

public:
	Tile() = default;
	Tile(const Tile&) = default;
	Tile(Tile&&) = default;
	Tile(const vec3& position, float size, bool visible);
	virtual ~Tile() override = default;

	Tile& operator=(const Tile&) = default;
	Tile& operator=(Tile&&) = default;

#ifndef OPENGLES
	virtual void drawTopDepth() const override;
#endif
	virtual void drawTop() const override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;
	virtual void cancelDrag() override;

	Com::Tile getType() const;
	void setType(Com::Tile newType);
	bool isBreachedFortress() const;
	bool isUnbreachedFortress() const;
	bool getBreached() const;
	void setBreached(bool yes);
	void setInteractivity(Interact lvl, bool dim = false);
	virtual void setEmission(Emission emi) override;
private:
	const char* pickMesh();
};

inline const char* Tile::pickMesh() {
	return type != Com::Tile::fortress ? "tile" : breached ? "breached" : "fortress";
}

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

// player on tiles
class Piece : public BoardObject {
public:
	uint16 lastFortress;	// index of last visited fortress (only relevant to throne)
private:
	Com::Piece type;
	bool drawTopSelf;

public:
	Piece() = default;
	Piece(const Piece&) = default;
	Piece(Piece&&) = default;
	Piece(const vec3& position, float rotation, float size, const Material* material);
	virtual ~Piece() override = default;

	Piece& operator=(const Piece&) = default;
	Piece& operator=(Piece&&) = default;

#ifndef OPENGLES
	virtual void drawTopDepth() const override;
#endif
	virtual void drawTop() const override;
	virtual void onHold(const ivec2& mPos, uint8 mBut) override;
	virtual void onUndrag(uint8 mBut) override;
	virtual void onHover() override;
	virtual void onUnhover() override;
	virtual void cancelDrag() override;

	Com::Piece getType() const;
	void setType(Com::Piece newType);
	pair<uint8, uint8> firingArea() const;	// 0 if non-firing piece
	void setActive(bool on);
	void updatePos(svec2 bpos = svec2(UINT16_MAX), bool forceRigid = false);
	bool getDrawTopSelf() const;
	void setInteractivity(bool on, bool dim, GCall holdCall, GCall leftCall, GCall rightCall);
private:
	float selfTopYpos(const Interactable* occupant) const;
};

inline Com::Piece Piece::getType() const {
	return type;
}

inline void Piece::setActive(bool on) {
	show = rigid = on;
}

inline bool Piece::getDrawTopSelf() const {
	return drawTopSelf;
}

inline float Piece::selfTopYpos(const Interactable* occupant) const {
	return dynamic_cast<const Piece*>(occupant) && occupant != this ? 1.1f * getScl().y : 0.01f;
}
