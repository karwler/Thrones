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

#ifndef SDL_IMAGE_VERSION_ATLEAST	// emscripten doesn't have this
#define SDL_IMAGE_VERSION_ATLEAST(X, Y, Z) ((SDL_IMAGE_MAJOR_VERSION >= X) && (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION >= Y) && (SDL_IMAGE_MAJOR_VERSION > X || SDL_IMAGE_MINOR_VERSION > Y || SDL_IMAGE_PATCHLEVEL >= Z))
#endif

constexpr char defaultReadMode[] = "rb";
constexpr char defaultWriteMode[] = "wb";

struct Material {
	vec4 color;
	float ao;
	float metallic;
	float roughness;
	float reflective;

	Material(const vec4& diffuse = vec4(1.f), float ambient = 0.8f, float metal = 0.f, float rough = 0.f, float reflect = 0.f);
};

struct Vertex {
	vec3 pos;
	vec3 nrm;
	vec2 tuv;
	vec3 tng;

	Vertex() = default;
	Vertex(const vec3& position, const vec3& normal, vec2 texuv, const vec3& tangent);
};

// functions

SDL_Surface* scaleSurfaceReplace(SDL_Surface* img, ivec2 res);
SDL_Surface* scaleSurfaceTry(SDL_Surface* img, ivec2 res);
SDL_Surface* scaleSurfaceCopy(SDL_Surface* img, ivec2 res);

template <class T = string>
T loadFile(const string& file) {
	T data;
	if (SDL_RWops* ifh = SDL_RWFromFile(file.c_str(), defaultReadMode)) {
		if (int64 len = SDL_RWsize(ifh); len != -1) {
			data.resize(len);
			if (sizet read = SDL_RWread(ifh, data.data(), sizeof(*data.data()), data.size()); read < data.size())
				data.resize(read);
		}
		SDL_RWclose(ifh);
	}
	return data;
}

