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

// RECT

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

// TEXTURE

Texture::Texture(const vec2i& size, const vec4& pos, const vec4& end, bool vertical) {
	vec4* pix = new vec4[uint(size.area())];
	if (vec2f last = size - 1; vertical) {
		for (int y = 0; y < size.y; y++)
			std::fill_n(pix + y * size.x, size.x, linearTransition(pos, end, float(y) / last.y));
	} else {
		for (int x = 0; x < size.x; x++)
			pix[x] = linearTransition(pos, end, float(x) / last.x);
		for (int y = 1; y < size.y; y++)
			std::copy_n(pix, size.x, pix + y * size.x);
	}
	loadGL(size, GL_RGBA, GL_FLOAT, pix);
	delete[] pix;
}

void Texture::close() {
	if (valid()) {
		glDeleteTextures(1, &id);
		*this = Texture();
	}
}

bool Texture::load(SDL_Surface* img) {
	if (img) {
		loadGL(vec2i(img->w, img->h), GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
		SDL_FreeSurface(img);
		return true;
	}
	*this = Texture();
	return false;
}

void Texture::loadGL(const vec2i& size, GLenum format, GLenum type, const void* pix) {
	res = size;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.x, res.y, 0, format, type, pix);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// INTERACTABLE

void Interactable::onClick(const vec2i&, uint8) {}

// DIJKSTRA

Dijkstra::Node::Node(uint16 id, uint16 dst) :
	id(id),
	dst(dst)
{}

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

// OTHER

mat4 makeTransform(const vec3& pos, const vec3& rot, const vec3& scl) {
	mat4 trans = glm::translate(mat4(1.f), pos);
	trans = glm::rotate(trans, rot.x, vec3(1.f, 0.f, 0.f));
	trans = glm::rotate(trans, rot.y, vec3(0.f, 1.f, 0.f));
	trans = glm::rotate(trans, rot.z, vec3(0.f, 0.f, 1.f));
	return trans = glm::scale(trans, vec3(scl.x, scl.y, scl.z));
}
