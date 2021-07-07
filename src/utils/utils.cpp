#include "utils.h"

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	if (a.h != b.h)
		return a.h < b.h;
	if (a.w != b.w)
		return a.w < b.w;
	if (a.refresh_rate != b.refresh_rate)
		return a.refresh_rate < b.refresh_rate;
	return a.format < b.format;
}

#ifndef OPENGLES
GLuint makeCubemap(GLsizei res, GLenum active) {
	GLuint cmap;
	glGenTextures(1, &cmap);
	loadCubemap(cmap, res, active);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return cmap;
}

void loadCubemap(GLuint tex, GLsizei res, GLenum active) {
	glActiveTexture(active);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	for (uint i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, res, res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}

GLuint makeFramebufferDepth(GLuint tex) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);	// necessary in case shadows are disabled
	return fbo;
}
#endif

// TEXTURE

Texture::Texture(SDL_Surface* img) {
	if (img) {
		load(img, GL_RGBA8, defaultFormat4, GL_CLAMP_TO_EDGE, GL_NEAREST);
		SDL_FreeSurface(img);
	} else
		*this = Texture();
}

Texture::Texture(SDL_Surface* img, GLint iform, GLenum pform) {
	load(img, iform, pform, GL_REPEAT, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(img);
}

Texture::Texture(array<uint8, 3> color) {
	SDL_Surface surf;
	surf.w = surf.h = 1;
	surf.pixels = color.data();
	load(&surf, GL_RGB8, defaultFormat3, GL_CLAMP_TO_EDGE, GL_NEAREST);
}

void Texture::close() {
	if (id) {
		free();
		*this = Texture();
	}
}

void Texture::load(const SDL_Surface* img, GLint iformat, GLenum pformat, GLint wrap, GLint filter) {
	glGenTextures(1, &id);
	upload(img, iformat, pformat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void Texture::upload(const SDL_Surface* img, GLint iformat, GLenum pformat) {
	res = ivec2(img->w, img->h);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, iformat, res.x, res.y, 0, pformat, GL_UNSIGNED_BYTE, img->pixels);
}
