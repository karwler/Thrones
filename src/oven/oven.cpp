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
