#include "oven.h"

constexpr char mtlKeywordNewmtl[] = "newmtl";
constexpr char argAudio = 'a';
constexpr char argMaterial = 'm';
constexpr char argObject = 'o';
constexpr char argShader = 's';
constexpr char argShaderE = 'S';
constexpr char argTexture = 't';
constexpr char argTextureR = 'T';
constexpr char messageUsage[] = "usage: tobjtobob <-a|-m|-o|-s|-t> <destination file> <input files>";

// OBJ FILE DATA

struct Element {
	uint p, t, n;

	Element() = default;
	Element(uint p, uint t, uint n);

	uint& operator[](uint i);
	uint* begin();
	uint* end();
};

Element::Element(uint p, uint t, uint n) :
	p(p),
	t(t),
	n(n)
{}

uint& Element::operator[](uint i) {
	return reinterpret_cast<uint*>(this)[i];
}

uint* Element::begin() {
	return reinterpret_cast<uint*>(this);
}

uint* Element::end() {
	return reinterpret_cast<uint*>(this) + 3;
}

struct Blueprint {
	string name;
	vector<uint16> elems;
	vector<Vertex> data;

	Blueprint(Blueprint&& bpr);
	Blueprint(string&& name);

	Blueprint& operator=(Blueprint&& bpr);
	bool empty() const;
};

Blueprint::Blueprint(Blueprint&& bpr) :
	name(std::move(bpr.name)),
	elems(std::move(bpr.elems)),
	data(std::move(bpr.data))
{}

Blueprint::Blueprint(string&& name) :
	name(std::move(name))
{}

Blueprint& Blueprint::operator=(Blueprint&& bpr) {
	name = std::move(bpr.name);
	elems = std::move(bpr.elems);
	data = std::move(bpr.data);
	return *this;
}

bool Blueprint::empty() const {
	return elems.empty() || data.empty();
}

// BMP FILE DATA

struct Image {
	string name;
	vector<uint8> data;
	GLint iformat;
	GLenum pformat;

	Image(string&& fname, vector<uint8>&& data, uint16 iformat, uint16 pformat);
};

Image::Image(string&& fname, vector<uint8>&& data, uint16 iformat, uint16 pformat) :
	name(std::move(fname)),
	data(std::move(data)),
	iformat(iformat),
	pformat(pformat)
{}

// AUDIOS

static void loadWav(const char* file, vector<pair<string, Sound>>& auds) {
	Sound sound;
	if (SDL_AudioSpec spec; SDL_LoadWAV(file, &spec, &sound.data, &sound.length)) {
		if (sound.set(spec); sound.convert(Sound::defaultSpec))
			auds.emplace_back(filename(delExt(file)), sound);
		else {
			sound.free();
			std::cerr << "error: couldn't convert " << file << std::endl;
		}
	} else
		std::cerr << "error: couldn't read " << file << linend << SDL_GetError() << std::endl;
}

static void writeAudios(const char* file, vector<pair<string, Sound>>& auds) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[audioHeaderSize];
	uint32* up = reinterpret_cast<uint32*>(obuf + 2);
	uint16* sp = reinterpret_cast<uint16*>(obuf + 6);
	*sp = uint16(auds.size());
	SDL_RWwrite(ofh, sp, sizeof(*sp), 1);

	for (auto& [name, sound] : auds) {
		obuf[0] = uint8(name.length());
		obuf[1] = sound.channels;
		up[0] = sound.length;
		sp[0] = sound.frequency;
		sp[1] = sound.format;
		sp[2] = sound.samples;

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), audioHeaderSize);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, sound.data, sizeof(*sound.data), sound.length);
		sound.free();
	}
	SDL_RWclose(ofh);
}

// MATERIALS

static void loadMtl(const char* file, vector<pair<string, Material>>& mtls) {
	vector<string> lines = readFileLines(file);
	if (lines.empty()) {
		std::cerr << "error: couldn't read " << file << std::endl;
		return;
	}
	mtls.emplace_back();

	for (const string& line : lines) {
		if (sizet len = strlen(mtlKeywordNewmtl); !strncicmp(line, mtlKeywordNewmtl, len)) {
			if (pair<string, Material> next(trim(line.substr(len)), Material()); mtls.back().first.empty())
				mtls.back() = std::move(next);
			else
				mtls.push_back(std::move(next));
		} else if (char c0 = char(toupper(line[0])); c0 == 'K') {
			if (char c1 = char(toupper(line[1])); c1 == 'A' || c1 == 'D')	// ambient and diffuse always have the same value
				mtls.back().second.diffuse = stofv<vec4>(&line[2], strtof, 1.f);
			else if (c1 == 'S')
				mtls.back().second.specular = stofv<vec3>(&line[2], strtof, 1.f);
		} else if (c0 == 'N') {
			if (toupper(line[1]) == 'S')
				mtls.back().second.shininess = sstof(&line[2]) / 1000.f * 128.f;
		} else if (c0 == 'D')
			mtls.back().second.diffuse.a = sstof(&line[1]);
		else if (c0 == 'T' && toupper(line[1]) == 'R')
			mtls.back().second.diffuse.a = 1.f - sstof(&line[2]);	// inverse of d
	}
	if (mtls.back().first.empty())
		mtls.pop_back();
}

static void writeMaterials(const char* file, vector<pair<string, Material>>& mtls) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint16 mamt = uint16(mtls.size());
	SDL_RWwrite(ofh, &mamt, sizeof(mamt), 1);

	for (const auto& [name, matl] : mtls) {
		uint8 nlen = uint8(name.length());
		SDL_RWwrite(ofh, &nlen, sizeof(nlen), 1);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, &matl, sizeof(matl), 1);
	}
	SDL_RWclose(ofh);
}

// OBJECTS

static uint resolveObjId(int id, uint size) {
	if (uint(id) < size)
		return uint(id);
	if (uint(-id) < size)
		return size + uint(id);
	return 0;
}

static void readFace(const char* str, vector<vec3>& verts, vector<vec2>& tuvs, vector<vec3>& norms, vector<pair<Element, uint16>>& elems, Blueprint& obj) {
	array<uint, 3> sizes = { uint(verts.size()), uint(tuvs.size()), uint(norms.size()) };
	array<Element, 3> face;
	uint8 v = 0, e = 0;

	for (char* end; *str && v < face.size();) {
		if (int n = int(strtol(str, &end, 0)); end != str) {
			face[v][e] = resolveObjId(n, sizes[e]);
			str = end;
			e++;
		} else if (*str == '/')
			face[v][e++] = 0;

		if (e && (*str != '/' || e >= sizes.size())) {
			std::fill(face[v].begin() + e, face[v].end(), 0);
			e = 0;
			v++;
		}
		if (*str)
			str++;
	}
	if (v < face.size())
		return;

	std::swap(face[1], face[2]);	// model is CW, need CCW
	for (v = 0; v < face.size(); v++) {
		if (!face[v].n) {
			vec3 normal = glm::normalize(glm::cross(verts[face[cycle(v, uint8(face.size()), int8(-1))][0]] - verts[face[v][0]], verts[face[cycle(v, uint8(face.size()), int8(-2))][0]] - verts[face[v][0]]));
			if (vector<vec3>::iterator it = std::find(norms.begin(), norms.end(), normal); it == norms.end()) {
				face[v].n = uint(norms.size());
				norms.push_back(normal);
			} else
				face[v].n = uint(it - norms.begin());
		}

		if (vector<pair<Element, uint16>>::iterator ei = std::find_if(elems.begin(), elems.end(), [&face, v](const pair<Element, uint16>& it) -> bool { return it.first.p == face[v].p && it.first.t == face[v].t && it.first.n == face[v].n; }); ei == elems.end()) {
			obj.elems.push_back(uint16(obj.data.size()));
			obj.data.emplace_back(verts[face[v].p], norms[face[v].n], vec2(tuvs[face[v].t].x, 1.f - tuvs[face[v].t].y));
			elems.emplace_back(face[v], obj.elems.back());
		} else
			obj.elems.push_back(ei->second);
	}
}

static void loadObj(const char* file, vector<Blueprint>& bprs) {
	vector<string> lines = readFileLines(file);
	if (lines.empty()) {
		std::cerr << "error: couldn't read " << file << std::endl;
		return;
	}
	vector<vec3> verts = { vec3(0.f) };
	vector<vec2> tuvs = { vec2(0.f, 1.f) };
	vector<vec3> norms = { vec3(0.f) };
	vector<pair<Element, uint16>> elems;	// mapping of current object's face ids to blueprint elements
	bprs.emplace_back(filename(delExt(file)));

	for (const string& line : lines) {
		if (char c0 = char(toupper(line[0])); c0 == 'V') {
			if (char c1 = char(toupper(line[1])); c1 == 'T')
				tuvs.push_back(stofv<vec2>(&line[2]));
			else if (c1 == 'N')
				norms.push_back(stofv<vec3>(&line[2]));
			else
				verts.push_back(stofv<vec3>(&line[1]));
		} else if (c0 == 'F')
			readFace(&line[1], verts, tuvs, norms, elems, bprs.back());
		else if (c0 == 'O') {
			if (Blueprint next(trim(line.substr(1))); bprs.back().empty())
				bprs.back() = std::move(next);
			else
				bprs.push_back(std::move(next));
			elems.clear();
		}
	}
	if (bprs.back().empty())
		bprs.pop_back();
}

static void writeObjects(const char* file, vector<Blueprint>& bprs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[objectHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(obuf + 1);
	*sp = uint16(bprs.size());
	SDL_RWwrite(ofh, sp, sizeof(*sp), 1);

	for (const Blueprint& it : bprs) {
		if (it.data.size() > 64000)
			std::cout << "warning: " << it.name << "'s size of " << it.data.size() << " exceeds 8000 vertices" << std::endl;

		obuf[0] = uint8(it.name.length());
		sp[0] = uint16(it.elems.size());
		sp[1] = uint16(it.data.size());

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), objectHeaderSize);
		SDL_RWwrite(ofh, it.name.c_str(), sizeof(*it.name.c_str()), it.name.length());
		SDL_RWwrite(ofh, it.elems.data(), sizeof(*it.elems.data()), it.elems.size());
		SDL_RWwrite(ofh, it.data.data(), sizeof(*it.data.data()), it.data.size());
	}
	SDL_RWclose(ofh);
}

// SHADERS

static bool checkText(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static bool checkSpace(char c) {
	return isSpace(c) && c != '\n';
}

static void loadGlsl(const char* file, vector<pair<string, string>>& srcs, bool gles) {
	string text = trim(readFile(file));
	bool keepLF = false;
	for (sizet i = 0; i < text.length(); i++) {
		if (checkSpace(text[i])) {
			sizet e = i;
			while (checkSpace(text[++e]));
			text.erase(i, e - i);
			if (checkText(text[i-1]) && checkText(text[i]))
				text.insert(i, 1, ' ');
			else
				i--;
		} else if (text[i] == '\n') {
			if (keepLF)
				keepLF = false;
			else
				text.erase(i--, 1);
		} else if (text[i] == '#')
			keepLF = true;
		else if (text[i] == '/') {
			if (text[i+1] == '/') {
				text.erase(i, text.find_first_of('\n', i + 2) - i);
				i--;
			} else if (text[i+1] == '*') {
				sizet e = text.find("*/", i);
				text.erase(i, e != string::npos ? e - i + 2 : e);
				i--;
			}
		}
	}

	if (gles) {
		constexpr char verstr[] = "#version ";
		if (sizet p = text.find(verstr); p < text.length()) {
			p += strlen(verstr);
			text.replace(p, text.find('\n', p) - p + 1, "300 es\nprecision highp float;");
		}
	}
	if (!text.empty())
		srcs.emplace_back(filename(file), std::move(text));
}

static void writeShaders(const char* file, vector<pair<string, string>>& srcs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[shaderHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(obuf + 1);
	obuf[0] = uint8(srcs.size());
	SDL_RWwrite(ofh, obuf, sizeof(*obuf), 1);

	for (const auto& [name, text] : srcs) {
		*obuf = uint8(name.length());
		*sp = uint16(text.length());

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), shaderHeaderSize);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, text.c_str(), sizeof(*text.c_str()), text.length());
	}
	SDL_RWclose(ofh);
}

// TEXTURES

static bool checkFormat(uint32 format, bool regular) {
#ifndef OPENGLES
	if (regular && (format == SDL_PIXELFORMAT_BGR24 || format == SDL_PIXELFORMAT_BGRA32))
		return true;
#endif
	return format == SDL_PIXELFORMAT_RGB24 || format == SDL_PIXELFORMAT_RGBA32;
}

static uint32 getFormat(uint8 bpp, bool regular) {
#ifndef OPENGLES
	if (regular)
		return bpp == 3 ? SDL_PIXELFORMAT_BGR24 : bpp == 4 ? SDL_PIXELFORMAT_BGRA32 : SDL_PIXELFORMAT_UNKNOWN;
#endif
	return bpp == 3 ? SDL_PIXELFORMAT_RGB24 : bpp == 4 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_UNKNOWN;
}

static uint16 getIformat(uint8 bpp) {
	switch (bpp) {
	case 1:
		return GL_R8;
	case 2:
		return GL_RG8;
	case 3:
		return GL_RGB8;
	case 4:
		return GL_RGBA8;
	}
	return 0;
}

static uint16 getPformat(uint32 format) {
	switch (format) {
#ifndef OPENGLES
	case SDL_PIXELFORMAT_BGR24:
		return GL_BGR;
	case SDL_PIXELFORMAT_BGRA32:
		return GL_BGRA;
#endif
	case SDL_PIXELFORMAT_RGB24:
		return GL_RGB;
	case SDL_PIXELFORMAT_RGBA32:
		return GL_RGBA;
	}
	return 0;
}

static void loadImg(const char* file, vector<Image>& imgs, bool regular) {
	string name = filename(delExt(file));
	vector<uint8> data = readFile<vector<uint8>>(file);
	SDL_Surface* img;
	if (data.empty() || !(img = IMG_Load_RW(SDL_RWFromMem(data.data(), int(data.size())), SDL_TRUE))) {
		std::cerr << "failed to load " << name << ": " << IMG_GetError() << std::endl;
		return;
	}
	bool renew = !regular;
	if (renew)
		img = scaleSurface(img, 2);

	uint8 bpp = img->format->BytesPerPixel;
	if (!checkFormat(img->format->format, regular)) {
		uint32 format = getFormat(bpp, regular);
		if (!format) {
			SDL_FreeSurface(img);
			std::cerr << "invalid pixel size " << bpp << " of " << name << std::endl;
			return;
		}
		if (SDL_Surface* pic = SDL_ConvertSurfaceFormat(img, format, 0)) {
			SDL_FreeSurface(img);
			img = pic;
			renew = true;
		} else {
			SDL_FreeSurface(img);
			std::cerr << SDL_GetError();
			return;
		}
	}

	if (renew) {
		data.resize(sizet(img->pitch * img->h));
		SDL_RWops* dw = SDL_RWFromMem(data.data(), int(data.size()));
		if (bpp == 3 ? IMG_SaveJPG_RW(img, dw, SDL_FALSE, 100) : IMG_SavePNG_RW(img, dw, SDL_FALSE)) {	// png don't work with 3 byte pixels
			SDL_RWclose(dw);
			SDL_FreeSurface(img);
			std::cerr << "failed to save " << name << ": " << IMG_GetError() << std::endl;
			return;
		}
		data.resize(sizet(SDL_RWtell(dw)));
		SDL_RWclose(dw);
	}
	imgs.emplace_back(std::move(name), std::move(data), getIformat(bpp), getPformat(img->format->format));
	SDL_FreeSurface(img);
}

static void writeImages(const char* file, vector<Image>& imgs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << SDL_GetError() << std::endl;
		return;
	}
	uint8 obuf[textureHeaderSize];
	uint32* wp = reinterpret_cast<uint32*>(obuf + 1);
	uint16* sp = reinterpret_cast<uint16*>(obuf + 5);
	*sp = uint16(imgs.size());
	SDL_RWwrite(ofh, sp, sizeof(*sp), 1);

	vector<uint32> sizes(imgs.size());
	for (uint16 i = 0; i < imgs.size(); i++) {
		obuf[0] = uint8(imgs[i].name.length());
		wp[0] = uint32(imgs[i].data.size());
		sp[0] = uint16(imgs[i].iformat);
		sp[1] = uint16(imgs[i].pformat);

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), textureHeaderSize);
		SDL_RWwrite(ofh, imgs[i].name.c_str(), sizeof(*imgs[i].name.c_str()), imgs[i].name.length());
		SDL_RWwrite(ofh, imgs[i].data.data(), sizeof(*imgs[i].data.data()), imgs[i].data.size());
	}
	SDL_RWclose(ofh);
}

// PROGRAM

template <class T, class F, class... A>
void process(const char* dest, const vector<string>& files, F loader, void (*writer)(const char*, vector<T>&), A... args) {
	vector<T> dats;
	for (const string& it : files)
		loader(it.c_str(), dats, args...);

	if (!dats.empty())
		writer(dest, dats);
	else
		std::cout << "nothing to write" << std::endl;
}

#ifdef _WIN32
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
#endif
	if (SDL_Init(0) || IMG_Init(imgInitFull) != imgInitFull) {
		std::cerr << SDL_GetError() << std::endl;
		return -1;
	}

	if (Arguments arg(argc, argv, {}, { argAudio, argMaterial, argObject, argShader, argShaderE, argTexture, argTextureR }); arg.getVals().empty())
		std::cout << "no input files" << linend << messageUsage << std::endl;
	else if (const char* dest = arg.getOpt(argAudio))
		process(dest, arg.getVals(), loadWav, writeAudios);
	else if (dest = arg.getOpt(argMaterial))
		process(dest, arg.getVals(), loadMtl, writeMaterials);
	else if (dest = arg.getOpt(argObject))
		process(dest, arg.getVals(), loadObj, writeObjects);
	else if (dest = arg.getOpt(argShader))
		process(dest, arg.getVals(), loadGlsl, writeShaders, false);
	else if (dest = arg.getOpt(argShaderE))
		process(dest, arg.getVals(), loadGlsl, writeShaders, true);
	else if (dest = arg.getOpt(argTexture))
		process(dest, arg.getVals(), loadImg, writeImages, true);
	else if (dest = arg.getOpt(argTextureR))
		process(dest, arg.getVals(), loadImg, writeImages, false);
	else
		std::cout << "error: invalid mode" << linend << messageUsage << std::endl;

	IMG_Quit();
	SDL_Quit();
	return 0;
}
