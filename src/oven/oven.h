#pragma once

#include "utils/alias.h"
#ifdef __APPLE__
#ifdef OPENGLES
#include <OpenGLES/ES3/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
#elif defined(OPENGLES)
#if OPENGLES == 32
#include <GLES3/gl32.h>
#else
#include <GLES3/gl3.h>
#endif
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#ifdef __APPLE__
#include <SDL2_image/SDL_image.h>
#elif defined(__ANDROID__) || defined(_WIN32)
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

constexpr uint audioHeaderSize = sizeof(uint8) + sizeof(uint32) + sizeof(uint16) * 3 + sizeof(uint8);
constexpr uint objectHeaderSize = sizeof(uint8) + sizeof(uint16) * 2;
constexpr uint shaderHeaderSize = sizeof(uint8) + sizeof(uint16);
constexpr uint textureHeaderSize = sizeof(uint8) * 2;

enum TexturePlacement : uint8 {
	TEXPLACE_NONE,
	TEXPLACE_WIDGET,
	TEXPLACE_OBJECT,
	TEXPLACE_BOTH,
	TEXPLACE_CUBE
};

struct Sound {
	static constexpr SDL_AudioSpec defaultSpec = { 22050, AUDIO_S16, 1, 0, 4096, 0, 0, nullptr, nullptr };

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
	vec4 color;
	vec3 spec;
	float shine;

	Material(const vec4& diffuse = vec4(1.f), const vec3& specular = vec3(1.f), float shininess = 32.f);
};

struct Vertex {
	vec3 pos;
	vec3 nrm;
	vec2 tuv;
	vec3 tng;

	Vertex() = default;
	Vertex(const vec3& position, const vec3& normal, vec2 texuv, const vec3& tangent);
};

// other functions

SDL_Surface* scaleSurface(SDL_Surface* img, ivec2 res);

inline SDL_Surface* scaleSurface(SDL_Surface* img, int div) {
	return div <= 1 || !img ? img : scaleSurface(img, ivec2(img->w, img->h) / div);
}
