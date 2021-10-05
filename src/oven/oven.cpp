#include "oven.h"
#include <iostream>

// SOUND

void Sound::set(const SDL_AudioSpec& spec) {
	frequency = spec.freq;
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

	cvt.len = length;
	cvt.buf = static_cast<uint8*>(SDL_malloc(sizet(cvt.len) * sizet(cvt.len_mult)));
	if (SDL_memcpy(cvt.buf, data, length); SDL_ConvertAudio(&cvt)) {
		SDL_free(cvt.buf);
		return false;	// failed to convert
	}
	free();
	set(dsts);
	length = cvt.len_cvt;
	data = static_cast<uint8*>(SDL_malloc(length));
	SDL_memcpy(data, cvt.buf, length);
	SDL_free(cvt.buf);
	return true;
}

// MATERIAL

Material::Material(const vec4& diffuse, const vec3& specular, float shininess) :
	color(diffuse),
	spec(specular),
	shine(shininess)
{}

// VERTEX

Vertex::Vertex(const vec3& position, const vec3& normal, vec2 texuv, const vec3& tangent) :
	pos(position),
	nrm(normal),
	tuv(texuv),
	tng(tangent)
{}

// FUNCTIONS

SDL_Surface* scaleSurface(SDL_Surface* img, ivec2 res) {
	if (!img || (img->w == res.x && img->h == res.y))
		return img;

	if (SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, res.x, res.y, img->format->BitsPerPixel, img->format->format)) {
		if (!SDL_BlitScaled(img, nullptr, dst, nullptr)) {
			SDL_FreeSurface(img);
			return dst;
		}
		std::cerr << "failed to scale surface: " << SDL_GetError() << std::endl;
		SDL_FreeSurface(dst);
	} else
		std::cerr << "failed to create scaled surface: " << SDL_GetError() << std::endl;
	SDL_FreeSurface(img);
	return nullptr;
}
