#include "utils.h"

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

bool Texture::loadGl(SDL_Surface* img) {
	if (!img) {
		res = 0;
		return false;
	}
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	res = vec2i(img->w, img->h);
	SDL_FreeSurface(img);
	return true;
}

void Texture::close() {
	if (valid()) {
		glDeleteTextures(1, &id);
		res = 0;
	}
}

// INTERACTABLE

void Interactable::onClick(const vec2i&, uint8) {}

// DIJKSTRA

Dijkstra::Node::Node(uint8 id, uint8 dst) :
	id(id),
	dst(dst)
{}

array<uint8, Com::boardSize> Dijkstra::travelDist(uint8 src, bool (*stepable)(uint8)) {
	// init graph
	array<Adjacent, Com::boardSize> grid;
	for (uint8 i = 0; i < Com::boardSize; i++)
		if (grid[i].cnt = 0; stepable(i) || i == src)	// ignore rules for starting point cause it can be a blocking piece
			for (uint8 (*mov)(uint8) : Com::adjacentStraight) {
				uint8 ni = mov(i);
				if (ni < Com::boardSize && stepable(ni))
					grid[i].adj[grid[i].cnt++] = ni;
			}

	array<uint8, Com::boardSize> dist;
	std::fill(dist.begin(), dist.end(), UINT8_MAX);
	dist[src] = 0;

	bool visited[Com::boardSize];
	std::fill_n(visited, Com::boardSize, false);

	// dijkstra
	std::priority_queue<Node, vector<Node>, Comp> nodes;
	nodes.emplace(src, 0);
	do {
		uint8 u = nodes.top().id;
		nodes.pop();
		if (!visited[u]) {	// maybe try "if (nodes.top().dst < dist[u])" instead of using bool matrix
			for (uint8 i = 0; i < grid[u].cnt; i++)
				if (uint8 v = grid[u].adj[i], du = dist[u] + 1; !visited[v] && du < dist[v]) {
					dist[v] = du;
					nodes.emplace(v, dist[v]);
				}
			visited[u] = true;
		}
	} while (!nodes.empty());
	return dist;
}

// STRINGS
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
