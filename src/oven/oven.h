#pragma once

#include "utils/text.h"
#ifdef __APPLE__
#ifdef OPENGLES
#include <OpenGLES/ES3/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#elif defined(OPENGLES)
#include <GLES3/gl3.h>
#else
#ifdef _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#endif

constexpr uint audioHeaderSize = sizeof(uint8) + sizeof(uint32) + sizeof(uint16) * 3 + sizeof(uint8);
constexpr uint objectHeaderSize = sizeof(uint8) + sizeof(uint16) * 2;
constexpr uint shaderHeaderSize = sizeof(uint8) + sizeof(uint16);
constexpr uint textureHeaderSize = sizeof(uint8) + sizeof(uint16) * 5;

struct Sound {
	static constexpr SDL_AudioSpec defaultSpec = { 48000, AUDIO_F32, 2, 0, 4096, 0, 0, nullptr, nullptr };

	uint8* data;
	uint32 length;
	uint16 frequency;
	SDL_AudioFormat format;
	uint8 channels;
	uint16 samples;

	void set(const SDL_AudioSpec& spec);
	bool convert(const SDL_AudioSpec& dsts);
	void free();
};

inline void Sound::free() {
	SDL_FreeWAV(data);
}

struct Material {
	vec3 diffuse;
	vec3 specular;
	float shininess;
	float alpha;

	Material(const vec3& diffuse = vec3(1.f), const vec3& specular = vec3(1.f), float shininess = 32.f, float alpha = 1.f);
};

struct Vertex {
	vec3 pos;
	vec3 nrm;
	vec2 tuv;

	Vertex() = default;
	Vertex(const vec3& pos, const vec3& nrm, const vec2& tuv);
};

#ifdef __APPLE__
inline void glBindVertexArray(GLuint arr) {
	glBindVertexArrayAPPLE(arr);
}

inline void glGenVertexArrays(GLsizei n, GLuint* arrs) {
	glGenVertexArraysAPPLE(n, arrs);
}

inline void glDeleteVertexArrays(GLsizei n, const GLuint* arrs) {
	glDeleteVertexArraysAPPLE(n, arrs);
}
#endif