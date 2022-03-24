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
#endif
	case SDL_PIXELFORMAT_RGB24:
		return pair(img, GL_RGB);
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

SDL_Surface* takeScreenshot(ivec2 res) {
	SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, res.x, res.y, 32, SDL_PIXELFORMAT_RGBA32);
	if (img) {
		glReadPixels(0, 0, res.x, res.y, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
		for (sizet a = 0, b = sizet(img->h - 1) * img->pitch; a < b; a += img->pitch, b -= img->pitch) {
			uint8* ap = static_cast<uint8*>(img->pixels) + a;
			std::swap_ranges(ap, ap + img->pitch, static_cast<uint8*>(img->pixels) + b);
		}
	} else
		logError("failed to create screenshot surface");
	return img;
}

// TEXTURE

void Texture::init(const vec4& color) {
	res = ivec2(2);
	upload(array<vec4, 4>{ color, color, color, color }.data(), 0, GL_RGBA, GL_FLOAT);
}

bool Texture::init(SDL_Surface* img, ivec2 limit) {
	if (img) {
		res = glm::min(ivec2(img->w, img->h), limit);
		upload(img->pixels, img->pitch / img->format->BytesPerPixel, textPixFormat, GL_UNSIGNED_BYTE);
		SDL_FreeSurface(img);
	}
	return img;
}

void Texture::upload(void* pixels, int rowLen, GLint format, GLenum type) {
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLen);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, format, type, pixels);
}

// TEXTURE COL

TextureCol::Element::Element(string&& texName, SDL_Surface* img, GLenum pformat, bool freeImage, bool minimizeSize) :
	name(std::move(texName)),
	image(img),
	format(pformat),
	freeImg(freeImage),
	minSize(minimizeSize)
{}

TextureCol::Find::Find(uint pg, uint id, const Rect& posize) :
	page(pg),
	index(id),
	rect(posize)
{}

umap<string, TexLoc> TextureCol::init(vector<Element>&& imgs, int recommendSize) {
	int maxSize;
	if (glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize); maxSize < recommendSize)
		throw std::runtime_error("texture size is limited to " + toStr(maxSize));
	int minIcon = recommendSize;
	for (const Element& it : imgs)
		if (!it.minSize) {
			minIcon = std::min(minIcon, it.image->h);
			res = std::min(std::max(recommendSize, std::max(it.image->w, it.image->h)), maxSize);
		}
	pages = { { Rect(0, 0, minIcon, minIcon) } };
	umap<string, TexLoc> refs;
	refs.reserve(imgs.size());
	refs.emplace(string(), TexLoc(0, pages[0][0]));
	vector<Find> fres(imgs.size());
	for (sizet i = 0; i < imgs.size(); ++i) {
		if (imgs[i].minSize)
			if (SDL_Surface* tscl = scaleSurfaceCopy(imgs[i].image, ivec2(minIcon)); tscl && tscl != imgs[i].image) {
				if (imgs[i].freeImg)
					SDL_FreeSurface(imgs[i].image);
				else
					imgs[i].freeImg = true;
				imgs[i].image = tscl;
			}

		fres[i] = findLocation(ivec2(imgs[i].image->w, imgs[i].image->h));
		if (fres[i].page < pages.size())
			pages[fres[i].page].insert(pages[fres[i].page].begin() + fres[i].index, fres[i].rect);
		else
			pages.push_back({ fres[i].rect });
		refs.emplace(std::move(imgs[i].name), TexLoc(fres[i].page, pages[fres[i].page][fres[i].index]));
	}
	calcPageReserve();

	vector<uint32> pixd(res * res * pageReserve, 0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, pageReserve, 0, Texture::textPixFormat, GL_UNSIGNED_BYTE, pixd.data());

	pixd.assign(pages[0][0].w * pages[0][0].h, 0xFFFFFFFF);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, pages[0][0].x, pages[0][0].y, 0, pages[0][0].w, pages[0][0].h, 1, Texture::textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
	for (sizet i = 0; i < imgs.size(); ++i) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH, imgs[i].image->pitch / imgs[i].image->format->BytesPerPixel);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, fres[i].rect.x, fres[i].rect.y, fres[i].page, fres[i].rect.w, fres[i].rect.h, 1, imgs[i].format, GL_UNSIGNED_BYTE, imgs[i].image->pixels);
		if (imgs[i].freeImg)
			SDL_FreeSurface(imgs[i].image);
	}
	return refs;
}

void TextureCol::free() {
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
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x, loc.y, page, sampRes.x, sampRes.y, 1, Texture::textPixFormat, GL_UNSIGNED_BYTE, static_cast<uint8*>(img->pixels) + offset.y * img->pitch + offset.x * img->format->BytesPerPixel);
	if (sampRes.x < loc.w) {
		int missing = loc.w - sampRes.x;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x + sampRes.x, loc.y, page, missing, sampRes.y, 1, Texture::textPixFormat, GL_UNSIGNED_BYTE, vector<uint32>(sizet(missing) * sizet(sampRes.y), 0).data());
	}
	if (sampRes.y < loc.y) {
		int missing = loc.h - sampRes.y;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, loc.x, loc.y + sampRes.y, page, loc.w, missing, 1, Texture::textPixFormat, GL_UNSIGNED_BYTE, vector<uint32>(sizet(loc.w) * sizet(missing), 0).data());
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
				if (int endy = i ? page[i-1].end().y : 0; page[i].y - endy >= size.y) {
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
		vector<uint32> pixd(res * res * pageReserve);
		if (pageReserve > oldSize)
			std::fill(pixd.begin() + res * res * oldSize, pixd.end(), 0);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, Texture::textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, pageReserve, 0, Texture::textPixFormat, GL_UNSIGNED_BYTE, pixd.data());
	}
}

vector<SDL_Surface*> TextureCol::peekMemory() const {
	Rect irc(0, 0, res, res);
	vector<SDL_Surface*> imgs(pages.size());
	for (sizet i = 0; i < pages.size(); ++i) {
		if (imgs[i] = SDL_CreateRGBSurfaceWithFormat(0, res, res, 32, SDL_PIXELFORMAT_RGB24); !imgs[i]) {
			logError("failed to create memory peek surface ", i);
			continue;
		}
		vector<Rect> borders(pages[i].size() * 2);
		for (sizet b = 0; b < pages[i].size(); ++b) {
			borders[b] = Rect(pages[i][b].end().x - 1, pages[i][b].y, 1, pages[i][b].h - 1);
			borders[pages[i].size()+b] = Rect(pages[i][b].x, pages[i][b].end().y - 1, pages[i][b].w, 1);
		}
		SDL_FillRect(imgs[i], &irc, 0);
		SDL_FillRects(imgs[i], pages[i].data(), pages[i].size(), SDL_MapRGB(imgs[i]->format, 255, 0, 0));
		SDL_FillRects(imgs[i], borders.data(), borders.size(), SDL_MapRGB(imgs[i]->format, 0, 0, 255));
	}
	return imgs;
}

vector<SDL_Surface*> TextureCol::peekTexture() const {
	sizet psiz = sizet(res) * sizet(res);
	vector<uint32> pixd(psiz * pageReserve);
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixd.data());

	vector<SDL_Surface*> imgs(pages.size());
	for (sizet i = 0; i < pages.size(); ++i) {
		if (imgs[i] = SDL_CreateRGBSurfaceWithFormat(0, res, res, 32, SDL_PIXELFORMAT_RGBA32); imgs[i])
			std::copy_n(pixd.data() + i * psiz, psiz, static_cast<uint32*>(imgs[i]->pixels));
		else
			logError("failed to create memory peek surface ", i);
	}
	return imgs;
}

// TEXTURE SET

TextureSet::Element::Element(SDL_Surface* iclr, SDL_Surface* inrm, string&& texName, GLenum cfmt, GLenum nfmt) :
	color(iclr),
	normal(inrm),
	name(std::move(texName)),
	cformat(cfmt),
	nformat(nfmt)
{}

void TextureSet::init(Import&& imp) {
	for (sizet i = 0; i < imp.imgs.size(); ++i) {
		Element& it = imp.imgs[i];
		if (it.color && ivec2(it.color->w, it.color->h) != ivec2(imp.cres))
			it.color = scaleSurface(it.color, ivec2(imp.cres));
		if (it.normal && ivec2(it.normal->w, it.normal->h) != ivec2(imp.nres))
			it.normal = scaleSurface(it.normal, ivec2(imp.nres));
	}
	loadTexArray(clr, imp, imp.cres, &Element::color, &Element::cformat, { 255, 255, 255, 255 }, Shader::colorTexa);
	loadTexArray(nrm, imp, imp.nres, &Element::normal, &Element::nformat, { 128, 128, 255, 255 }, Shader::normalTexa);

	refs.reserve(imp.imgs.size() + 1);
	refs.emplace(string(), uvec2(0));
	uvec2 tid(0);
	for (Element& it : imp.imgs)
		refs.emplace(std::move(it.name), uvec2(it.color ? tid.x += 1 : 0, it.normal ? tid.y += 1 : 0));

	glActiveTexture(Shader::skyboxTexa);
	glGenTextures(1, &sky);
	glBindTexture(GL_TEXTURE_CUBE_MAP, sky);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
	glActiveTexture(Shader::texa);
}

void TextureSet::free() {
	glDeleteTextures(1, &sky);
	glDeleteTextures(1, &nrm);
	glDeleteTextures(1, &clr);
	refs.clear();
}

void TextureSet::loadTexArray(GLuint& tex, Import& imp, int res, SDL_Surface* Element::*img, GLenum Element::*format, array<uint8, 4> blank, GLenum active) {
	vector<uint8> pixels(sizet(res) * sizet(res) * blank.size());
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
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, imp.imgs.size() + 1);
#else
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, res, res, imp.imgs.size() + 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, res, res, 1, GL_RGBA,  GL_UNSIGNED_BYTE, pixels.data());

	for (sizet i = 0, t = 1; i < imp.imgs.size(); ++i)
		if (SDL_Surface* it = imp.imgs[i].*img) {
			glPixelStorei(GL_UNPACK_ROW_LENGTH, it->pitch / it->format->BytesPerPixel);
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, t++, res, res, 1, imp.imgs[i].*format,  GL_UNSIGNED_BYTE, it->pixels);
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
