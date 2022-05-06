#include "utils.h"
#include "engine/shaders.h"
#include <numeric>
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

void checkFramebufferStatus(string_view name) {
	switch (GLenum rc = glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
	case GL_FRAMEBUFFER_UNDEFINED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNDEFINED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"s);
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"s);
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNSUPPORTED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"s);
#endif
	default:
		if (rc != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error(name + ": unknown framebuffer error"s);
	}
}

pair<SDL_Surface*, GLenum> pickPixFormat(SDL_Surface* img) {
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
#ifndef OPENGLES
	case SDL_PIXELFORMAT_BGR24:
		return pair(img, GL_BGR);
	case SDL_PIXELFORMAT_RGB24:
		return pair(img, GL_RGB);
#endif
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

void invertImage(SDL_Surface* img) {
	for (sizet a = 0, b = sizet(img->h - 1) * img->pitch; a < b; a += img->pitch, b -= img->pitch) {
		uint8* ap = static_cast<uint8*>(img->pixels) + a;
		std::swap_ranges(ap, ap + img->pitch, static_cast<uint8*>(img->pixels) + b);
	}
}

SDL_Surface* takeScreenshot(ivec2 res) {
	SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, res.x, res.y, 32, TextureCol::shotImgFormat);
	if (img) {
		glReadPixels(0, 0, res.x, res.y, TextureCol::textPixFormat, GL_UNSIGNED_BYTE, img->pixels);
		invertImage(img);
	} else
		logError("failed to create screenshot surface");
	return img;
}

// TEXTURE COL

TextureCol::Element::Element(string&& texName, SDL_Surface* img, GLenum pformat) :
	name(std::move(texName)),
	image(img),
	format(pformat)
{}

TextureCol::Find::Find(uint pg, uint id, const Rect& posize) :
	page(pg),
	index(id),
	rect(posize)
{}

umap<string, TexLoc> TextureCol::init(vector<Element>&& imgs, int recommendSize, int minUiIcon) {
	int maxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	for (const Element& it : imgs)
		res = std::min(std::max(recommendSize, std::max(it.image->w, it.image->h)), maxSize);
	pages = { { Rect(0, 0, minUiIcon, minUiIcon) } };
	umap<string, TexLoc> refs;
	refs.reserve(imgs.size());
	refs.emplace(string(), TexLoc(0, pages[0][0]));
	vector<Find> fres(imgs.size());
	for (sizet i = 0; i < imgs.size(); ++i) {
		fres[i] = findLocation(ivec2(imgs[i].image->w, imgs[i].image->h));
		if (fres[i].page < pages.size())
			pages[fres[i].page].insert(pages[fres[i].page].begin() + fres[i].index, fres[i].rect);
		else
			pages.push_back({ fres[i].rect });
		refs.emplace(std::move(imgs[i].name), TexLoc(fres[i].page, pages[fres[i].page][fres[i].index]));
	}
	calcPageReserve();

	vector<uint32> pixd(res * res * pageReserve, 0);
	glActiveTexture(Shader::wgtTexa);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, pageReserve, 0, textPixFormat, GL_UNSIGNED_BYTE, pixd.data());

	pixd.assign(pages[0][0].w * pages[0][0].h, 0xFFFFFFFF);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, pages[0][0].x, pages[0][0].y, 0, pages[0][0].w, pages[0][0].h, 1, textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
	for (sizet i = 0; i < imgs.size(); ++i) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH, imgs[i].image->pitch / imgs[i].image->format->BytesPerPixel);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, fres[i].rect.x, fres[i].rect.y, fres[i].page, fres[i].rect.w, fres[i].rect.h, 1, imgs[i].format, GL_UNSIGNED_BYTE, imgs[i].image->pixels);
		SDL_FreeSurface(imgs[i].image);
	}
#ifdef OPENGLES
	constexpr GLenum none = GL_NONE;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffers(1, &none);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#endif
	return refs;
}

void TextureCol::free() {
#ifdef OPENGLES
	glDeleteFramebuffers(1, &fbo);
#endif
	glDeleteTextures(1, &tex);
	pages.clear();
}

TexLoc TextureCol::insert(SDL_Surface* img) {
	TexLoc loc = insert(img, img ? ivec2(img->w, img->h) : ivec2());
	SDL_FreeSurface(img);
	return loc;
}

TexLoc TextureCol::insert(const SDL_Surface* img, ivec2 size, ivec2 offset) {
	if (!img)
		return blank;

	glActiveTexture(Shader::wgtTexa);
	Find fres = findLocation(size);
	if (fres.page < pages.size())
		pages[fres.page].insert(pages[fres.page].begin() + fres.index, fres.rect);
	else {
		pages.push_back({ fres.rect });
		maybeResize();
	}
	uploadSubTex(img, offset, fres.rect, fres.page);
	return TexLoc(fres.page, fres.rect);
}

void TextureCol::replace(TexLoc& loc, SDL_Surface* img) {
	replace(loc, img, img ? ivec2(img->w, img->h) : ivec2());
	SDL_FreeSurface(img);
}

void TextureCol::replace(TexLoc& loc, const SDL_Surface* img, ivec2 size, ivec2 offset) {
	if (!img) {
		erase(loc);
		return;
	}
	if (loc.blank()) {
		loc = insert(img, size, offset);
		return;
	}

	vector<Rect>& page = pages[loc.tid];
	if (auto [rit, ok] = findReplaceable(loc, size); ok) {
		rit->w = size.x;
		loc.rct.size() = size;
		assertIntegrity();
		glActiveTexture(Shader::wgtTexa);
		uploadSubTex(img, offset, loc.rct, loc.tid);
	} else {
		if (rit != page.end())
			page.erase(rit);
		loc = insert(img, size, offset);
	}
}

void TextureCol::uploadSubTex(const SDL_Surface* img, ivec2 offset, const Rect& loc, uint page) {
	ivec2 sampRes = glm::min(ivec2(img->w, img->h) - offset, loc.size());
	glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x, loc.y, page, sampRes.x, sampRes.y, 1, textPixFormat, GL_UNSIGNED_BYTE, static_cast<uint8*>(img->pixels) + offset.y * img->pitch + offset.x * img->format->BytesPerPixel);
	if (sampRes.x < loc.w) {
		int missing = loc.w - sampRes.x;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x + sampRes.x, loc.y, page, missing, sampRes.y, 1, textPixFormat, GL_UNSIGNED_BYTE, vector<uint32>(sizet(missing) * sizet(sampRes.y), 0).data());
	}
	if (sampRes.y < loc.y) {
		int missing = loc.h - sampRes.y;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x, loc.y + sampRes.y, page, loc.w, missing, 1, textPixFormat, GL_UNSIGNED_BYTE, vector<uint32>(sizet(loc.w) * sizet(missing), 0).data());
	}
}

pair<vector<Rect>::iterator, bool> TextureCol::findReplaceable(const TexLoc& loc, ivec2& size) {
	vector<Rect>& page = pages[loc.tid];
	vector<Rect>::iterator rit = std::find_if(page.begin(), page.end(), [loc](const Rect& it) -> bool { return it.pos() == loc.rct.pos(); });
	if (size = glm::min(size, res); rit != page.end() && size.y == rit->h)
		if (vector<Rect>::iterator next = rit + 1; loc.rct.x + size.x <= (next != page.end() && next->y == rit->y ? next->x : res))
			return pair(rit, true);
	return pair(rit, false);
}

void TextureCol::erase(TexLoc& loc) {
	if (!loc.blank()) {
		vector<Rect>& page = pages[loc.tid];
		if (vector<Rect>::iterator rit = std::find_if(page.begin(), page.end(), [loc](const Rect& it) -> bool { return it.pos() == loc.rct.pos(); }); rit != page.end()) {
			if (page.erase(rit); loc.tid == pages.size() - 1 && page.empty()) {
				pages.pop_back();
				glActiveTexture(Shader::wgtTexa);
				maybeResize();
			}
			loc = blank;
		}
	}
}

TextureCol::Find TextureCol::findLocation(ivec2 size) const {
	size = glm::min(size, res);
	for (sizet p = pages.size() - 1; p < pages.size(); --p) {
		const vector<Rect>& page = pages[p];
		for (sizet i = 0; i < page.size();) {
			if (ivec2 pos(0, page[i].y); page[i].h == size.y) {
				for (; i < page.size() && page[i].y == pos.y; ++i) {
					if (page[i].x - pos.x >= size.x) {
						assertIntegrity(page, Rect(pos, size));
						return Find(p, i, Rect(pos, size));
					}
					pos.x = page[i].end().x;
				}
				if (res - pos.x >= size.x) {
					assertIntegrity(page, Rect(pos, size));
					return Find(p, i, Rect(pos, size));
				}
			} else
				for (; i < page.size() && page[i].y == pos.y; ++i);

			if (i < page.size())
				if (int endy = i ? page[i - 1].end().y : 0; page[i].y - endy >= size.y) {
					assertIntegrity(page, Rect(0, endy, size));
					return Find(p, i, Rect(0, endy, size));
				}
		}
		if (int endy = !page.empty() ? page.back().end().y : 0; res - endy >= size.y) {
			assertIntegrity(page, Rect(0, endy, size));
			return Find(p, page.size(), Rect(0, endy, size));
		}
	}
	return Find(pages.size(), 0, Rect(0, 0, size));
}

void TextureCol::assertIntegrity() {
	for (const vector<Rect>& page : pages)
		for (sizet i = 0; i < page.size(); ++i)
			for (sizet j = 0; j < page.size(); ++j)
				assert(i == j || !SDL_HasIntersection(&page[i], &page[j]));
}

void TextureCol::assertIntegrity(const vector<Rect>& page, const Rect& nrect) {
	for (sizet i = 0; i < page.size(); ++i)
		assert(!SDL_HasIntersection(&nrect, &page[i]));
}

void TextureCol::maybeResize() {
	if (pages.size() > pageReserve || pageReserve - pages.size() > pageNumStep) {
		uint oldSize = pageReserve;
		calcPageReserve();
		vector<uint32> pixd = texturePixels();
		if (pageReserve > oldSize)
			std::fill(pixd.begin() + res * res * oldSize, pixd.end(), 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, pageReserve, 0, textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
	}
}

vector<SDL_Surface*> TextureCol::peekMemory() const {
	Rect irc(0, 0, res, res);
	vector<SDL_Surface*> imgs(pages.size());
	for (sizet i = 0; i < pages.size(); ++i) {
		if (imgs[i] = SDL_CreateRGBSurfaceWithFormat(0, res, res, 24, SDL_PIXELFORMAT_RGB24); !imgs[i]) {
			logError("failed to create memory peek surface ", i);
			continue;
		}
		vector<Rect> borders(pages[i].size() * 2);
		for (sizet b = 0; b < pages[i].size(); ++b) {
			borders[b] = Rect(pages[i][b].end().x - 1, pages[i][b].y, 1, pages[i][b].h - 1);
			borders[pages[i].size() + b] = Rect(pages[i][b].x, pages[i][b].end().y - 1, pages[i][b].w, 1);
		}
		SDL_FillRect(imgs[i], &irc, 0);
		SDL_FillRects(imgs[i], pages[i].data(), pages[i].size(), SDL_MapRGB(imgs[i]->format, 255, 0, 0));
		SDL_FillRects(imgs[i], borders.data(), borders.size(), SDL_MapRGB(imgs[i]->format, 0, 0, 255));
	}
	return imgs;
}

vector<SDL_Surface*> TextureCol::peekTexture() const {
	sizet psiz = sizet(res) * sizet(res);
	glActiveTexture(Shader::wgtTexa);
	vector<uint32> pixd = texturePixels();
	vector<SDL_Surface*> imgs(pages.size());
	for (sizet i = 0; i < pages.size(); ++i) {
		if (imgs[i] = SDL_CreateRGBSurfaceWithFormat(0, res, res, 32, shotImgFormat); imgs[i])
			std::copy_n(pixd.data() + i * psiz, psiz, static_cast<uint32*>(imgs[i]->pixels));
		else
			logError("failed to create memory peek surface ", i);
	}
	return imgs;
}

vector<uint32> TextureCol::texturePixels() const {
	sizet psiz = sizet(res) * sizet(res);
	vector<uint32> pixd(psiz * pageReserve);
#ifdef OPENGLES
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (uint i = 0; i < pageReserve; ++i) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0, i);
		glReadPixels(0, 0, res, res, Texture::textPixFormat, GL_UNSIGNED_BYTE, pixd.data() + i * psiz);
	}
#else
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
#endif
	return pixd;
}

// TEXTURE SET

TextureSet::Element::Element(SDL_Surface* iclr, SDL_Surface* inrm, string&& texName, GLenum cfmt, GLenum nfmt, bool owned) :
	color(iclr),
	normal(inrm),
	name(std::move(texName)),
	cformat(cfmt),
	nformat(nfmt),
	unique(owned)
{}

void TextureSet::Element::clear() {
	if (unique)
		SDL_FreeSurface(color);
	SDL_FreeSurface(normal);
}

void TextureSet::init(Import&& imp) {
	uint numColors = 1, numNormals = 1;
	for (Element& it : imp.imgs) {
		if (SDL_Surface* tscl = scaleSurfaceCopy(it.color, ivec2(imp.cres))) {
			if (tscl != it.color) {
				if (it.unique)
					SDL_FreeSurface(it.color);
				it.color = tscl;
				it.unique = true;
			}
			++numColors;

			it.normal = scaleSurfaceReplace(it.normal, ivec2(imp.nres));
			numNormals += bool(it.normal);
		} else {
			it.clear();
			it.color = nullptr;
		}
	}
	loadTexArray<Shader::colorTexa>(clr, imp, imp.cres, numColors, &Element::color, &Element::cformat, { 255, 255, 255, 255 });
	loadTexArray<Shader::normalTexa>(nrm, imp, imp.nres, numNormals, &Element::normal, &Element::nformat, { 128, 128, 255, 255 });

	refs.reserve(numColors);
	refs.emplace(string(), uvec2(0));
	uvec2 tid(0);
	for (Element& it : imp.imgs)
		if (it.color) {
			refs.emplace(std::move(it.name), uvec2(tid.x += 1, it.normal ? tid.y += 1 : 0));
			it.clear();
		}

	glActiveTexture(Shader::skyboxTexa);
	glGenTextures(1, &sky);
	glBindTexture(GL_TEXTURE_CUBE_MAP, sky);
	for (sizet i = 0; i < imp.sky.size(); ++i) {
		if (auto [img, format] = imp.sky[i]; img) {
			glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
			SDL_FreeSurface(img);
		} else {
			uint8 pixels[4] = { 0, 0, 0, 255 };
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		}
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void TextureSet::free() {
	glDeleteTextures(1, &sky);
	glDeleteTextures(1, &nrm);
	glDeleteTextures(1, &clr);
	refs.clear();
}

template <GLenum active>
void TextureSet::loadTexArray(GLuint& tex, const Import& imp, int res, uint num, SDL_Surface* Element::*img, GLenum Element::*format, array<uint8, 4> blank) {
	vector<uint8> pixels(sizet(res) * sizet(res) * blank.size());
	for (sizet i = 0; i < pixels.size(); i += blank.size())
		std::copy(blank.begin(), blank.end(), pixels.begin() + i);

	glActiveTexture(active);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, num, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, res, res, 1, GL_RGBA,  GL_UNSIGNED_BYTE, pixels.data());

	for (sizet i = 0, t = 1; i < imp.imgs.size(); ++i)
		if (SDL_Surface* it = imp.imgs[i].*img) {
			glPixelStorei(GL_UNPACK_ROW_LENGTH, it->pitch / it->format->BytesPerPixel);
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, t++, res, res, 1, imp.imgs[i].*format,  GL_UNSIGNED_BYTE, it->pixels);
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

// MATH

uint32 ceilPower2(uint32 val) {
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 8;
	val |= val >> 4;
	val |= val >> 16;
	return val + 1;
}
