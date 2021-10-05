#include "settings.h"
#include "utils.h"
#include <stdexcept>

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	if (a.h != b.h)
		return a.h < b.h;
	if (a.w != b.w)
		return a.w < b.w;
	if (a.refresh_rate != b.refresh_rate)
		return a.refresh_rate < b.refresh_rate;
	return a.format < b.format;
}

void checkFramebufferStatus(const string& name) {
	switch (GLenum rc = glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
	case GL_FRAMEBUFFER_UNDEFINED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNDEFINED");
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNSUPPORTED");
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
#endif
	default:
		if (rc != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error(name + ": unknown framebuffer error");
	}
}

// TEXTURE

Texture::Texture(SDL_Surface* img, GLenum pform) {
	load(img, pform, GL_REPEAT, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(SDL_Surface* img) {
	if (auto [image, format] = pickPixFormat(img); image) {
		load(image, format, GL_CLAMP_TO_EDGE, GL_NEAREST);
		SDL_FreeSurface(image);
	}
}

Texture::Texture(array<uint8, 4> color) {
	SDL_Surface surf;
	surf.w = surf.h = 1;
	surf.pixels = color.data();
	load(&surf, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}

void Texture::close() {
	if (id) {
		free();
		*this = Texture();
	}
}

void Texture::load(const SDL_Surface* img, GLenum pform, GLint wrap, GLint filter) {
	glGenTextures(1, &id);
	upload(img, pform);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

void Texture::upload(const SDL_Surface* img, GLenum pform) {
	res = ivec2(img->w, img->h);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, pform, GL_UNSIGNED_BYTE, img->pixels);
}

void Texture::reload(const SDL_Surface* img, GLenum pform) {
	upload(img, pform);
	glGenerateMipmap(GL_TEXTURE_2D);
}

pair<SDL_Surface*, GLenum> Texture::pickPixFormat(SDL_Surface* img) {
	if (!img)
		return pair(img, GL_NONE);

	switch (img->format->format) {
#ifdef __EMSCRIPTEN__
	case SDL_PIXELFORMAT_BGRA32:
#else
	case SDL_PIXELFORMAT_BGRA32: case SDL_PIXELFORMAT_XRGB8888:
#endif
#ifdef OPENGLES
		if (Texture::bgraSupported)
			return pair(img, GL_BGRA);
		break;
#else
		return pair(img, GL_BGRA);
#endif
#ifdef __EMSCRIPTEN__
	case SDL_PIXELFORMAT_RGBA32:
#else
	case SDL_PIXELFORMAT_RGBA32: case SDL_PIXELFORMAT_XBGR8888:
#endif
		return pair(img, GL_RGBA);
	}
#ifdef OPENGLES
	SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
#else
	SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_BGRA32, 0);
#endif
	SDL_FreeSurface(img);
	img = dst;
#ifdef OPENGLES
	return pair(img, GL_RGBA);
#else
	return pair(img, GL_BGRA);
#endif
}

// TEXTURE SET

TextureSet::Element::Element(SDL_Surface* iclr, SDL_Surface* inrm, string&& texName, GLenum cfmt, GLenum nfmt) :
	color(iclr),
	normal(inrm),
	name(std::move(texName)),
	cformat(cfmt),
	nformat(nfmt)
{}

TextureSet::TextureSet(Import&& imp) {
	for (sizet i = 0; i < imp.imgs.size(); ++i) {
		Element& it = imp.imgs[i];
		if (ivec2(it.color->w, it.color->h) < imp.cres)
			it.color = scaleSurface(it.color, imp.cres);
		if (ivec2(it.normal->w, it.normal->h) < imp.nres)
			it.normal = scaleSurface(it.normal, imp.nres);

		if (!(it.color && it.normal)) {
			SDL_FreeSurface(it.color);
			SDL_FreeSurface(it.normal);
			imp.imgs.erase(imp.imgs.begin() + i--);
		}
	}
	loadTexArray(clr, imp, imp.cres, &Element::color, &Element::cformat, { 255, 255, 255, 255 }, colorTexa);
	loadTexArray(nrm, imp, imp.nres, &Element::normal, &Element::nformat, { 128, 128, 255, 255 }, normalTexa);

	refs.reserve(imp.imgs.size() + 1);
	refs.emplace(string(), 0);
	for (size_t i = 0; i < imp.imgs.size(); ++i)
		refs.emplace(std::move(imp.imgs[i].name), i + 1);

	glActiveTexture(skyboxTexa);
	glGenTextures(1, &sky);
	glBindTexture(GL_TEXTURE_CUBE_MAP, sky);
	if (std::all_of(imp.sky.begin(), imp.sky.end(), [](const pair<SDL_Surface*, GLenum>& it) -> bool { return it.first; })) {
		for (sizet i = 0; i < imp.sky.size(); ++i) {
			auto [img, format] = imp.sky[i];
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
			SDL_FreeSurface(img);
		}
	} else {
		uint8 pixels[4] = { 0, 0, 0, 255 };
		for (sizet i = 0; i < imp.sky.size(); ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(Texture::texa);
}

void TextureSet::free() {
	glDeleteTextures(1, &sky);
	glDeleteTextures(1, &nrm);
	glDeleteTextures(1, &clr);
}

void TextureSet::loadTexArray(GLuint& tex, Import& imp, ivec2 res, SDL_Surface* Element::*img, GLenum Element::*format, array<uint8, 4> blank, GLenum active) {
	vector<uint8> pixels(sizet(res.x) * sizet(res.y) * blank.size());
	for (sizet i = 0; i < pixels.size(); i += blank.size())
		std::copy(blank.begin(), blank.end(), pixels.begin() + i);

	glActiveTexture(active);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res.x, res.y, imp.imgs.size() + 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, res.x, res.y, 1, GL_RGBA,  GL_UNSIGNED_BYTE, pixels.data());

	for (sizet i = 0; i < imp.imgs.size(); ++i) {
		SDL_Surface* it = imp.imgs[i].*img;
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i + 1, it->w, it->h, 1, imp.imgs[i].*format,  GL_UNSIGNED_BYTE, it->pixels);
		SDL_FreeSurface(it);
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

// FRAME SET

FrameSet::FrameSet(const Settings* sets, ivec2 res) {
	constexpr GLenum attach[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	if (sets->ssao) {
		glGenFramebuffers(1, &fboGeom);
		glBindFramebuffer(GL_FRAMEBUFFER, fboGeom);
		texPosition = makeTexture(res, GL_RGBA32F, GL_RGBA, vposTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPosition, 0);
		texNormal = makeTexture(res, GL_RGBA32F, GL_RGBA, normTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texNormal, 0);
		glDrawBuffers(2, attach);
		glReadBuffer(GL_NONE);

		glGenRenderbuffers(1, &rboGeom);
		glBindRenderbuffer(GL_RENDERBUFFER, rboGeom);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboGeom);
		checkFramebufferStatus("geometry buffer");

		glGenFramebuffers(1, &fboSsao);
		glBindFramebuffer(GL_FRAMEBUFFER, fboSsao);
		texSsao = makeTexture(res, GL_R16F, GL_RED, ssaoTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texSsao, 0);
		glReadBuffer(GL_NONE);
		checkFramebufferStatus("SSAO buffer");

		glGenFramebuffers(1, &fboBlur);
		glBindFramebuffer(GL_FRAMEBUFFER, fboBlur);
		texBlur = makeTexture(res, GL_R16F, GL_RED, blurTexa);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlur, 0);
		glReadBuffer(GL_NONE);
		checkFramebufferStatus("blur buffer");
	}
	if (sets->bloom) {
		glGenFramebuffers(1, &fboLight);
		glBindFramebuffer(GL_FRAMEBUFFER, fboLight);
		texColor = makeTexture(res, GL_RGBA32F, GL_RGBA, colorTexa, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach[0], GL_TEXTURE_2D, texColor, 0);
		texBright = makeTexture(res, GL_RGBA32F, GL_RGBA, brightTexa, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attach[1], GL_TEXTURE_2D, texBright, 0);
		glDrawBuffers(2, attach);
		glReadBuffer(GL_NONE);

		glGenRenderbuffers(1, &rboLight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboLight);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, res.x, res.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboLight);
		checkFramebufferStatus("light buffer");

		for (sizet i = 0; i < fboGauss.size(); ++i) {
			glGenFramebuffers(1, &fboGauss[i]);
			glBindFramebuffer(GL_FRAMEBUFFER, fboGauss[i]);
			texGauss[i] = makeTexture(res, GL_RGBA32F, GL_RGBA, brightTexa, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attach[0], GL_TEXTURE_2D, texGauss[i], 0);
			glReadBuffer(GL_NONE);
			checkFramebufferStatus("gauss buffer " + toStr(i));
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(Texture::texa);
}

GLuint FrameSet::makeTexture(ivec2 res, GLint iform, GLenum pform, GLenum active, GLint filter) {
	GLuint tex;
	glActiveTexture(active);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, iform, res.x, res.y, 0, pform, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return tex;
}

void FrameSet::free() {
	glDeleteTextures(texGauss.size(), texGauss.data());
	glDeleteFramebuffers(fboGauss.size(), fboGauss.data());
	glDeleteRenderbuffers(1, &rboLight);
	glDeleteTextures(1, &texBright);
	glDeleteTextures(1, &texColor);
	glDeleteFramebuffers(1, &fboLight);
	glDeleteTextures(1, &texBlur);
	glDeleteFramebuffers(1, &fboBlur);
	glDeleteTextures(1, &texSsao);
	glDeleteFramebuffers(1, &fboSsao);
	glDeleteRenderbuffers(1, &rboGeom);
	glDeleteTextures(1, &texNormal);
	glDeleteTextures(1, &texPosition);
	glDeleteFramebuffers(1, &fboGeom);
}
