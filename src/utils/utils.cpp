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
		load(img, GL_RGBA8, defaultFormat4, GL_CLAMP_TO_EDGE, GL_NEAREST);
		SDL_FreeSurface(img);
	} else
		*this = Texture();
}

Texture::Texture(SDL_Surface* img, GLint iform, GLenum pform) {
	load(img, iform, pform, GL_REPEAT, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(img);
}

Texture::Texture(array<uint8, 3> color) {
	SDL_Surface surf;
	surf.w = surf.h = 1;
	surf.pixels = color.data();
	load(&surf, GL_RGB8, defaultFormat3, GL_CLAMP_TO_EDGE, GL_NEAREST);
}

void Texture::close() {
	if (valid()) {
		free();
		*this = Texture();
	}
}

void Texture::upload(SDL_Surface* img, GLint iformat, GLenum pformat, GLint wrap, GLint filter) {
	res = ivec2(img->w, img->h);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, iformat, res.x, res.y, 0, pformat, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

// INTERACTABLE

void Interactable::onClick(const ivec2&, uint8) {}

// DIJKSTRA

vector<uint16> Dijkstra::travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16), uint16 (*const* vmov)(uint16, svec2), uint8 movSize) {
	// init graph
	uint16 area = size.x * size.y;
	vector<Adjacent> grid(area);
	for (uint16 i = 0; i < area; i++)
		if (grid[i].cnt = 0; stepable(i) || i == src)	// ignore rules for starting point cause it can be a blocking piece
			for (uint8 m = 0; m < movSize; m++)
				if (uint16 ni = vmov[m](i, size); ni < area && stepable(ni))
					grid[i].adj[grid[i].cnt++] = ni;

	vector<bool> visited(area, false);
	vector<uint16> dist(area, UINT16_MAX);
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
