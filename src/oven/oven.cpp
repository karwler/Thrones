#include "oven.h"

// SOUND

void Sound::set(const SDL_AudioSpec& spec) {
	frequency = uint16(spec.freq);
	format = spec.format;
	channels = spec.channels;
	samples = spec.samples;
}

bool Sound::convert(const SDL_AudioSpec& dsts) {
	SDL_AudioCVT cvt;
	if (!SDL_BuildAudioCVT(&cvt, format, channels, frequency, dsts.format, dsts.channels, dsts.freq))
		return true;	// nothing to convert
	if (cvt.needed != 1)
		return false;	// not convertible

	cvt.len = int(length);
	cvt.buf = static_cast<uint8*>(SDL_malloc(sizet(cvt.len * cvt.len_mult)));
	if (SDL_memcpy(cvt.buf, data, length);  SDL_ConvertAudio(&cvt)) {
		SDL_free(cvt.buf);
		return false;	// failed to convert
	}
	free();
	set(dsts);
	length = uint32(cvt.len_cvt);
	data = static_cast<uint8*>(SDL_malloc(length));
	SDL_memcpy(data, cvt.buf, length);
	SDL_free(cvt.buf);
	return true;
}

// MATERIAL

Material::Material(const vec3& diffuse, const vec3& specular, float shininess, float alpha) :
	diffuse(diffuse),
	specular(specular),
	shininess(shininess),
	alpha(alpha)
{}

// VERTEX

Vertex::Vertex(const vec3& pos, const vec3& nrm, const vec2& tuv) :
	pos(pos),
	nrm(nrm),
	tuv(tuv)
{}

// FUNCTIONS

SDL_Surface* scaleSurface(SDL_Surface* img, int div) {
	if (div > 1 && img) {
		if (SDL_Surface* dst = SDL_CreateRGBSurface(img->flags, img->w / div, img->h / div, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask)) {
			if (SDL_Rect rect = { 0, 0, dst->w, dst->h }; !SDL_BlitScaled(img, nullptr, dst, &rect)) {
				SDL_FreeSurface(img);
				return dst;
			}
			std::cerr << "failed to scale surface: " << SDL_GetError() << std::endl;
			SDL_FreeSurface(dst);
		} else
			std::cerr << "failed to create scaled surface: " << SDL_GetError() << std::endl;
	}
	return img;
}
