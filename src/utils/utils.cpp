#include "utils.h"

bool Rect::overlap(const Rect& rect, vec2i& sback, vec2i& rback) const {
	if (w <= 0 || h <= 0 || rect.w <= 0 || rect.h <= 0)	// idfk
		return false;

	sback = back();
	rback = rect.back();
	return x < rback.x && y < rback.y && sback.x >= rect.x && sback.y >= rect.y;
}

vec4 Rect::crop(const Rect& frame) {
	vec2i rback, fback;
	if (!overlap(frame, rback, fback)) {
		*this = Rect(0);
		return vec4(0.f);
	}

	float width = float(w), height = float(h);
	vec4 crop(0.f, 0.f, 1.f, 1.f);	// uv corners
	if (x < frame.x) {			// left
		int cl = frame.x - x;
		x = frame.x;
		w -= cl;
		crop.x = float(cl) / width;
	}
	if (rback.x > fback.x) {	// right
		int cr = rback.x - fback.x;
		w -= cr;
		crop.z -= float(cr) / width;
	}
	if (y < frame.y) {			// top
		int ct = frame.y - y;
		y = frame.y;
		h -= ct;
		crop.y = float(ct) / height;
	}
	if (rback.y > fback.y) {	// bottom
		int cb = rback.y - fback.y;
		h -= cb;
		crop.w -= float(cb) / height;
	}
	return crop;
}

Rect Rect::getOverlap(const Rect& frame) const {
	Rect rect = *this;
	vec2i rback, fback;
	if (!rect.overlap(frame, rback, fback))
		return Rect(0);

	// crop rect if it's boundaries are out of frame
	if (rect.x < frame.x) {	// left
		rect.w -= frame.x - rect.x;
		rect.x = frame.x;
	}
	if (rback.x > fback.x)	// right
		rect.w -= rback.x - fback.x;
	if (rect.y < frame.y) {	// top
		rect.h -= frame.y - rect.y;
		rect.y = frame.y;
	}
	if (rback.y > fback.y)	// bottom
		rect.h -= rback.y - fback.y;
	return rect;
}

void Texture::load(SDL_Surface* img, const string& file, bool setFilename) {
	if (!img) {
		*this = Texture();
		throw std::runtime_error("failed to load texture " + file + '\n' + IMG_GetError());
	}

	switch (img->format->BytesPerPixel) {
	case 4:
		format = img->format->Rmask == 0x000000FF ? GL_RGBA : GL_BGRA;
		break;
	case 3:
		format = img->format->Rmask == 0x000000FF ? GL_RGB : GL_BGR;
		break;
	default:
		SDL_FreeSurface(img);
		*this = Texture();
		throw std::runtime_error(string("invalid texture pixel format ") + SDL_GetPixelFormatName(img->format->format) + ' ' + file);
	}
	name = setFilename ? delExt(file) : "";
	res = vec2i(img->w, img->h);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, img->format->BytesPerPixel, img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(img);
}

void Texture::close() {
	glDeleteTextures(1, &id);
	*this = Texture();
}

string parentPath(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return dseps;
#endif
	sizet end = path.find_last_not_of(dsep);	// skip initial separators
	if (end == string::npos)					// if the entire path is separators, return root
		return dseps;

	sizet pos = path.find_last_of(dsep, end);	// skip to separators between parent and child
	if (pos == string::npos)					// if there are none, just cut off child
		return path.substr(0, end + 1);

	pos = path.find_last_not_of(dsep, pos);		// skip separators to get to the parent entry
	return pos == string::npos ? dseps : path.substr(0, pos + 1);	// cut off child
}

string filename(const string& path) {
	if (path.empty() || path == dseps)
		return emptyStr;

	sizet end = path.back() == dsep ? path.length() - 1 : path.length();
	sizet pos = path.find_last_of(dsep, end - 1);
	return pos == string::npos ? path.substr(0, end) : path.substr(pos + 1, end-pos - 1);
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

string wtos(const wstring& src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return emptyStr;

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len, nullptr, nullptr);
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
