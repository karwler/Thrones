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

void Texture::loadFile(const string& file) {
	SDL_Surface* img = SDL_LoadBMP(file.c_str());
	if (!img || img->format->format != SDL_PIXELFORMAT_ARGB8888) {
		SDL_FreeSurface(img);
		res = 0;
		throw std::runtime_error("failed to load texture " + file + '\n' + SDL_GetError());
	}

	for (int i = 0, s = img->w * img->h; i < s; i++) {
		uint32* pix = static_cast<uint32*>(img->pixels) + i;
		*pix = (*pix << 8) | (*pix >> 24);
	}
	loadGl(img, delExt(file), GL_RGBA);
}

void Texture::loadText(SDL_Surface* img) {
	if (img && img->format->format == SDL_PIXELFORMAT_BGRA32)
		loadGl(img, emptyStr, GL_BGRA);
	else {
		SDL_FreeSurface(img);
		res = 0;
	}
}

void Texture::loadGl(SDL_Surface* img, const string& text, GLenum format) {
	name = text;
	res = vec2i(img->w, img->h);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, format, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	SDL_FreeSurface(img);
}

void Texture::close() {
	if (valid()) {
		glDeleteTextures(1, &id);
		res = 0;
	}
}

// INTERACTABLE

void Interactable::onClick(const vec2i&, uint8) {}

// A STAR

Astar::Node::Node(uint8 id, uint8 parent, uint16 gCost, float fCost) :
	id(id),
	parent(parent),
	gCost(gCost),
	fCost(fCost)
{}

Astar::Astar(bool (*stepable)(uint8, void*), void* data) :
	stepable(stepable),
	data(data)
{}

float Astar::distance(uint8 id, uint8 dst) {
	int dx = id % Com::boardLength - dst % Com::boardLength;
	int dy = id / Com::boardLength - dst / Com::boardLength;
	return std::sqrt(float(dx * dx + dy * dy));
}

vector<uint8> Astar::travelPath(uint8 src, uint8 dst) {
	int ret = travelDist(src, dst);
	if (ret == -1)
		return {};

	vector<uint8> path(sizet(ret) + 1);
	for (uint8 id = dst; ret; id = grid[id].parent, ret--)
		path[sizet(ret)] = id;
	path[sizet(ret)] = src;
	return path;
}

int Astar::travelDist(uint8 src, uint8 dst) {
	if (src == dst)
		return 0;
	if (!isValid(dst))
		return -1;

	for (uint8 i = 0; i < Com::boardSize; i++)
		grid[i] = Node(i, UINT8_MAX, UINT16_MAX, FLT_MAX);
	grid[src] = Node(src, src, 0, 0.f);

	bool closed[Com::boardSize];
	memset(closed, 0, Com::boardSize * sizeof(*closed));
	for (sset<Node> open = { grid[src] }; !open.empty() && open.size() < Com::boardSize;) {
		sset<Node>::iterator nid = std::find_if(open.begin(), open.end(), [this](const Node& it) -> bool { return isValid(it.id); });
		Node node = *nid;
		open.erase(open.begin(), next(nid));
		closed[node.id] = true;

		for (uint8 (*mov)(uint8) : Com::adjacentStraight)	// TODO: test
			if (uint8 ni = mov(node.id); isValid(ni)) {
				if (ni == dst) {
					grid[ni].parent = node.id;
					return node.gCost + 1;
				}
				if (!closed[ni]) {
					uint16 g = node.gCost + 1;
					float f = float(g) + distance(ni, dst);

					if (grid[ni].fCost > f) {
						grid[ni].parent = node.id;
						grid[ni].gCost = g;
						grid[ni].fCost = f;
						open.insert(grid[ni]);
					}
				}
			}
	}
	return -1;
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
