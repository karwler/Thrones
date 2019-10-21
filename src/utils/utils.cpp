#include "utils.h"

bool operator<(const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
	if (a.h < b.h)
		return true;
	if (a.h > b.h)
		return false;

	if (a.w < b.w)
		return true;
	if (a.w > b.w)
		return false;

	if (a.refresh_rate < b.refresh_rate)
		return true;
	if (a.refresh_rate > b.refresh_rate)
		return false;
	return a.format < b.format;
}

// TEXTURE

Texture::Texture(SDL_Surface* img) {
	if (img) {
		id = loadGL(res = ivec2(img->w, img->h), GL_RGBA8, defaultFormat4, GL_UNSIGNED_BYTE, img->pixels, GL_CLAMP_TO_EDGE, GL_NEAREST);
		SDL_FreeSurface(img);
	} else
		*this = Texture();
}

Texture::Texture(ivec2 size, GLint iform, GLenum pform, const uint8* pix) :
	id(loadGL(size, iform, pform, GL_UNSIGNED_BYTE, pix, GL_REPEAT, GL_LINEAR)),
	res(size)
{
	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::close() {
	if (valid()) {
		free();
		*this = Texture();
	}
}

GLuint Texture::loadGL(const ivec2& size, GLint iformat, GLenum pformat, GLenum type, const void* pix, GLint wrap, GLint filter) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, iformat, size.x, size.y, 0, pformat, type, pix);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	return id;
}

// INTERACTABLE

void Interactable::onClick(const ivec2&, uint8) {}

// DIJKSTRA

vector<uint16> Dijkstra::travelDist(uint16 src, uint16 dlim, uint16 width, uint16 size, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, uint16), uint8 movSize) {
	// init graph
	vector<Adjacent> grid(size);
	for (uint16 i = 0; i < size; i++)
		if (grid[i].cnt = 0; stepable(i) || i == src)	// ignore rules for starting point cause it can be a blocking piece
			for (uint8 m = 0; m < movSize; m++)
				if (uint16 ni = vmov[m](i, width); ni < size && stepable(ni))
					grid[i].adj[grid[i].cnt++] = ni;

	vector<bool> visited(size, false);
	vector<uint16> dist(size, UINT16_MAX);
	dist[src] = 0;

	// dijkstra
	std::priority_queue<Node, vector<Node>, Comp> nodes;
	nodes.emplace(src, 0);
	do {
		uint16 u = nodes.top().id;
		nodes.pop();
		if (dist[u] < dlim && !visited[u]) {
			for (uint8 i = 0; i < grid[u].cnt; i++)
				if (uint16 v = grid[u].adj[i], du = dist[u] + 1; !visited[v] && du < dist[v]) {
					dist[v] = du;
					nodes.emplace(v, dist[v]);
				}
			visited[u] = true;
		}
	} while (!nodes.empty());
	return dist;
}
