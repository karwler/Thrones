#include "oven.h"
#include "utils/text.h"

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
	SDL_Surface* dst = scaleSurfaceCopy(img, res);
	if (dst != img)
		SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* scaleSurfaceCopy(SDL_Surface* img, ivec2 res) {
	if ((img->w == res.x && img->h == res.y) || !img)
		return img;

	if (SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, res.x, res.y, img->format->BitsPerPixel, img->format->format)) {
		if (!SDL_BlitScaled(img, nullptr, dst, nullptr))
			return dst;
		logError("failed to scale surface: ", SDL_GetError());
		SDL_FreeSurface(dst);
	} else
		logError("failed to create scaled surface: ", SDL_GetError());
	return nullptr;
}
