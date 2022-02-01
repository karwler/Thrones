#include "utils.h"
#include "engine/shaders.h"
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

uint32 ceilPower2(uint32 val) {
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 8;
	val |= val >> 4;
	val |= val >> 16;
	return val + 1;
}

SDL_Surface* takeScreenshot(ivec2 res) {
	SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, res.x, res.y, 32, SDL_PIXELFORMAT_RGBA32);
	if (img) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glReadPixels(0, 0, res.x, res.y, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

		for (sizet a = 0, b = sizet(img->h - 1) * img->pitch; a < b; a += img->pitch, b -= img->pitch) {
			uint8* ap = static_cast<uint8*>(img->pixels) + a;
			std::swap_ranges(ap, ap + img->pitch, static_cast<uint8*>(img->pixels) + b);
		}
	}
	return img;
}

// TEXTURE

Texture::Texture(const SDL_Surface* img, GLenum pform) :
	res(img->w, img->h)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, pform, GL_UNSIGNED_BYTE, img->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(const SDL_Surface* img, int limit, ivec2 offset) {
	if (img) {
		res = glm::min(ivec2(img->w, img->h), limit);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, textPixFormat, GL_UNSIGNED_BYTE, static_cast<uint8*>(img->pixels) + offset.y * img->pitch + offset.x * img->format->BytesPerPixel);
	}
}

Texture::Texture(array<uint8, 4> color) :
	res(1)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, color.data());
}

void Texture::close() {
	if (id) {
		free();
		*this = Texture();
	}
}

void Texture::update(const SDL_Surface* img, ivec2 offset) {
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
	glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, res.x, res.y, textPixFormat, GL_UNSIGNED_BYTE, static_cast<uint8*>(img->pixels) + offset.y * img->pitch + offset.x * img->format->BytesPerPixel);
}

pair<SDL_Surface*, GLenum> Texture::pickPixFormat(SDL_Surface* img) {
	if (!img)
		return pair(img, GL_NONE);

	switch (img->format->format) {
#ifndef OPENGLES
	case SDL_PIXELFORMAT_BGRA32: case SDL_PIXELFORMAT_XRGB8888:
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
		if (ivec2(it.color->w, it.color->h) != imp.cres)
			it.color = scaleSurface(it.color, imp.cres);
		if (ivec2(it.normal->w, it.normal->h) != imp.nres)
			it.normal = scaleSurface(it.normal, imp.nres);

		if (!(it.color && it.normal)) {
			SDL_FreeSurface(it.color);
			SDL_FreeSurface(it.normal);
			imp.imgs.erase(imp.imgs.begin() + i--);
		}
	}
	loadTexArray(clr, imp, imp.cres, &Element::color, &Element::cformat, { 255, 255, 255, 255 }, Shader::colorTexa);
	loadTexArray(nrm, imp, imp.nres, &Element::normal, &Element::nformat, { 128, 128, 255, 255 }, Shader::normalTexa);

	refs.reserve(imp.imgs.size() + 1);
	refs.emplace(string(), 0);
	for (size_t i = 0; i < imp.imgs.size(); ++i)
		refs.emplace(std::move(imp.imgs[i].name), i + 1);

	glActiveTexture(Shader::skyboxTexa);
	glGenTextures(1, &sky);
	glBindTexture(GL_TEXTURE_CUBE_MAP, sky);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	if (std::all_of(imp.sky.begin(), imp.sky.end(), [](const pair<SDL_Surface*, GLenum>& it) -> bool { return it.first; })) {
		for (sizet i = 0; i < imp.sky.size(); ++i) {
			auto [img, format] = imp.sky[i];
			glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
			SDL_FreeSurface(img);
		}
	} else {
		uint8 pixels[4] = { 0, 0, 0, 255 };
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		for (sizet i = 0; i < imp.sky.size(); ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(Shader::texa);
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
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#ifdef OPENGLES
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res.x, res.y, imp.imgs.size() + 1);
#else
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res.x, res.y, imp.imgs.size() + 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, res.x, res.y, 1, GL_RGBA,  GL_UNSIGNED_BYTE, pixels.data());

	for (sizet i = 0; i < imp.imgs.size(); ++i) {
		SDL_Surface* it = imp.imgs[i].*img;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, it->pitch / it->format->BytesPerPixel);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i + 1, it->w, it->h, 1, imp.imgs[i].*format,  GL_UNSIGNED_BYTE, it->pixels);
		SDL_FreeSurface(it);
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

// SOUND

bool Sound::convert(const SDL_AudioSpec& srcs, const SDL_AudioSpec& dsts) {
	SDL_AudioCVT cvt;
	if (!SDL_BuildAudioCVT(&cvt, srcs.format, srcs.channels, srcs.freq, dsts.format, dsts.channels, dsts.freq))
		return true;	// nothing to convert
	if (cvt.needed != 1)
		return false;	// not convertible

	cvt.len = length;
	cvt.buf = static_cast<uint8*>(SDL_malloc(sizet(cvt.len) * sizet(cvt.len_mult)));
	if (SDL_memcpy(cvt.buf, data, length); SDL_ConvertAudio(&cvt)) {
		SDL_free(cvt.buf);
		return false;	// failed to convert
	}
	free();
	length = cvt.len_cvt;
	data = static_cast<uint8*>(SDL_malloc(length));
	SDL_memcpy(data, cvt.buf, length);
	SDL_free(cvt.buf);
	return true;
}
