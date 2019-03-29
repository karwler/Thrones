#include "utils.h"

vec4 Rect::crop(const Rect& rect) {
	Rect isct;
	if (!SDL_IntersectRect(this, &rect, &isct)) {
		*this = Rect(0);
		return vec4(0.f);
	}

	float width = float(w), height = float(h);
	vec2i te = end(), ie = isct.end();
	vec4 crop;
	crop.x = isct.x > x ? float(isct.x - x) / width : 0.f;
	crop.y = isct.y > y ? float(isct.y - y) / height : 0.f;
	crop.z = ie.x < te.x ? float(w - (te.x - ie.x)) / width : 1.f;
	crop.a = ie.y < te.y ? float(h - (te.y - ie.y)) / height : 1.f;
	*this = isct;
	return crop;
}

void Texture::load(SDL_Surface* img, const string& file, bool setFilename) {
	if (!img) {
		*this = Texture();
		throw std::runtime_error("failed to load texture " + file + '\n' + SDL_GetError());
	}

	switch (img->format->BytesPerPixel) {
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	default:
		SDL_FreeSurface(img);
		*this = Texture();
		throw std::runtime_error(string("invalid texture pixel format ") + SDL_GetPixelFormatName(img->format->format) + ' ' + file);
	}
	name = setFilename ? delExt(file) : emptyStr;
	res = vec2i(img->w, img->h);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GLint(format), img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(img);
}

void Texture::close() {
	glDeleteTextures(1, &id);
	*this = Texture();
}
#ifdef _WIN32
string wtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return emptyStr;
	len--;
	
	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return L"";

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
	return dst;
}
#endif
