#include "oven.h"
#ifdef __APPLE
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#endif

static const string mtlKeywordNewmtl = "newmtl";
static const string objKeywordOff = "off";
constexpr char argAudio = 'a';
constexpr char argMaterial = 'm';
constexpr char argObject = 'o';
constexpr char argShader = 's';
constexpr char argTexture = 't';
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
	SDL_Surface* image;
	GLint iformat;
	GLenum pformat;

	Image();
	Image(Image&& img);
	Image(string&& fname, SDL_Surface* image, uint16 iformat, uint16 pformat);

	static Image surfaceToImage(string&& fname, SDL_Surface* img);
};

Image::Image() :
	image(nullptr)
{}

Image::Image(Image&& img) :
	name(std::move(img.name)),
	image(img.image),
	iformat(img.iformat),
	pformat(img.pformat)
{}

Image::Image(string&& fname, SDL_Surface* image, uint16 iformat, uint16 pformat) :
	name(std::move(fname)),
	image(image),
	iformat(iformat),
	pformat(pformat)
{}

Image Image::surfaceToImage(string&& name, SDL_Surface* img) {
	if (img->format->format == SDL_PIXELFORMAT_BGR24 || img->format->format == SDL_PIXELFORMAT_BGRA32)
		return Image(std::move(name), img, img->format->format == SDL_PIXELFORMAT_BGR24 ? GL_RGB8 : GL_RGBA8, img->format->format == SDL_PIXELFORMAT_BGR24 ? GL_BGR : GL_BGRA);

	if (uint32 format = img->format->BytesPerPixel == 3 ? SDL_PIXELFORMAT_BGR24 : img->format->BytesPerPixel == 4 ? SDL_PIXELFORMAT_BGRA32 : SDL_PIXELFORMAT_UNKNOWN) {
		if (SDL_Surface* pic = SDL_ConvertSurfaceFormat(img, format, 0)) {
			SDL_FreeSurface(img);
			return Image(std::move(name), pic, pic->format->BytesPerPixel == 3 ? GL_RGB8 : GL_RGBA8, pic->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA);
		}
		std::cerr << "error: failed to convert " << name << " of format " << pixelformatName(img->format->format) << std::endl;
	} else
		std::cerr << "error: " << name << " has invalid format " << pixelformatName(img->format->format) << std::endl;

	SDL_FreeSurface(img);
	return Image();
}

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
	FILE* ofh = fopen(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[audioHeaderSize];
	uint32* up = reinterpret_cast<uint32*>(obuf + 2);
	uint16* sp = reinterpret_cast<uint16*>(obuf + 6);
	*sp = uint16(auds.size());
	fwrite(sp, sizeof(*sp), 1, ofh);

	for (auto& [name, sound] : auds) {
		obuf[0] = uint8(name.length());
		obuf[1] = sound.channels;
		up[0] = sound.length;
		sp[0] = sound.frequency;
		sp[1] = sound.format;
		sp[2] = sound.samples;

		fwrite(obuf, sizeof(*obuf), audioHeaderSize, ofh);
		fwrite(name.c_str(), sizeof(*name.c_str()), name.length(), ofh);
		fwrite(sound.data, sizeof(*sound.data), sound.length, ofh);
		sound.free();
	}
	fclose(ofh);
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
		if (!strncicmp(line, mtlKeywordNewmtl, mtlKeywordNewmtl.length())) {
			if (pair<string, Material> next(trim(line.substr(mtlKeywordNewmtl.length())), Material()); mtls.back().first.empty())
				mtls.back() = std::move(next);
			else
				mtls.push_back(std::move(next));
		} else if (char c0 = char(toupper(line[0])); c0 == 'K') {
			if (char c1 = char(toupper(line[1])); c1 == 'A' || c1 == 'D')	// ambient and diffuse always have the same value
				mtls.back().second.diffuse = stov<3>(&line[2], 1.f);
			else if (c1 == 'S')
				mtls.back().second.specular = stov<3>(&line[2], 1.f);
		} else if (c0 == 'N') {
			if (toupper(line[1]) == 'S')
				mtls.back().second.shininess = sstof(&line[2]) / 1000.f * 128.f;
		} else if (c0 == 'D')
			mtls.back().second.alpha = sstof(&line[1]);
		else if (c0 == 'T' && toupper(line[1]) == 'R')
			mtls.back().second.alpha = 1.f - sstof(&line[2]);	// inverse of d
	}
	if (mtls.back().first.empty())
		mtls.pop_back();
}

static void writeMaterials(const char* file, vector<pair<string, Material>>& mtls) {
	FILE* ofh = fopen(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint16 mamt = uint16(mtls.size());
	fwrite(&mamt, sizeof(mamt), 1, ofh);

	for (const auto& [name, matl] : mtls) {
		uint8 nlen = uint8(name.length());
		fwrite(&nlen, sizeof(nlen), 1, ofh);
		fwrite(name.c_str(), sizeof(*name.c_str()), name.length(), ofh);
		fwrite(&matl, sizeof(matl), 1, ofh);
	}
	fclose(ofh);
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
	uint sizes[3] = { uint(verts.size()), uint(tuvs.size()), uint(norms.size()) };
	array<Element, 3> face;
	uint8 v = 0, e = 0;

	for (char* end; *str && v < face.size();) {
		if (int n = int(strtol(str, &end, 0)); end != str) {
			face[v][e] = resolveObjId(n, sizes[e]);
			str = end;
			e++;
		} else if (*str == '/')
			face[v][e++] = 0;

		if (e && (*str != '/' || e >= 3)) {
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
	vector<vec2> tuvs = { vec2(0.f) };
	vector<vec3> norms = { vec3(0.f) };
	vector<pair<Element, uint16>> elems;	// mapping of current object's face ids to blueprint elements
	bprs.emplace_back(filename(delExt(file)));

	for (const string& line : lines) {
		if (char c0 = char(toupper(line[0])); c0 == 'V') {
			if (char c1 = char(toupper(line[1])); c1 == 'T')
				tuvs.push_back(stov<2>(&line[2]));
			else if (c1 == 'N')
				norms.push_back(stov<3>(&line[2]));
			else
				verts.push_back(stov<3>(&line[1]));
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
	FILE* ofh = fopen(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[objectHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(obuf + 1);
	*sp = uint16(bprs.size());
	fwrite(sp, sizeof(*sp), 1, ofh);

	for (const Blueprint& it : bprs) {
		if (it.data.size() > 64000)
			std::cout << "warning: " << it.name << "'s size of " << it.data.size() << " exceeds 8000 vertices" << std::endl;

		obuf[0] = uint8(it.name.length());
		sp[0] = uint16(it.elems.size());
		sp[1] = uint16(it.data.size());

		fwrite(obuf, sizeof(*obuf), objectHeaderSize, ofh);
		fwrite(it.name.c_str(), sizeof(*it.name.c_str()), it.name.length(), ofh);
		fwrite(it.elems.data(), sizeof(*it.elems.data()), it.elems.size(), ofh);
		fwrite(it.data.data(), sizeof(*it.data.data()), it.data.size(), ofh);
	}
	fclose(ofh);
}

// SHADERS

static bool checkText(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static bool checkSpace(char c) {
	return isSpace(c) && c != '\n';
}

static void loadGlsl(const char* file, vector<pair<string, string>>& srcs) {
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
	if (!text.empty())
		srcs.emplace_back(filename(file), text);
}

static void writeShaders(const char* file, vector<pair<string, string>>& srcs) {
	FILE* ofh = fopen(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[shaderHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(obuf + 1);
	obuf[0] = uint8(srcs.size());
	fwrite(obuf, sizeof(*obuf), 1, ofh);

	for (const auto& [name, text] : srcs) {
		*obuf = uint8(name.length());
		*sp = uint16(text.length());

		fwrite(obuf, sizeof(*obuf), shaderHeaderSize, ofh);
		fwrite(name.c_str(), sizeof(*name.c_str()), name.length(), ofh);
		fwrite(text.c_str(), sizeof(*text.c_str()), text.length(), ofh);
	}
	fclose(ofh);
}

// TEXTURES

static void loadBmp(const char* file, vector<Image>& imgs) {
	if (SDL_Surface* src = SDL_LoadBMP(file)) {
		if (Image img = Image::surfaceToImage(filename(delExt(file)), src); img.image)
			imgs.push_back(std::move(img));
	} else
		std::cerr << "error: couldn't read " << file << linend << SDL_GetError() << std::endl;
}

static void writeImages(const char* file, vector<Image>& imgs) {
	FILE* ofh = fopen(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[textureHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(obuf + 1);
	*sp = uint16(imgs.size());
	fwrite(sp, sizeof(*sp), 1, ofh);

	for (const Image& it : imgs) {
		obuf[0] = uint8(it.name.length());
		sp[0] = uint16(it.image->w);
		sp[1] = uint16(it.image->h);
		sp[2] = uint16(it.image->pitch);
		sp[3] = uint16(it.iformat);
		sp[4] = uint16(it.pformat);

		fwrite(obuf, sizeof(*obuf), textureHeaderSize, ofh);
		fwrite(it.name.c_str(), sizeof(*it.name.c_str()), it.name.length(), ofh);
		fwrite(it.image->pixels, sizeof(uint8), uint(it.image->pitch * it.image->h), ofh);
		SDL_FreeSurface(it.image);
	}
	fclose(ofh);
}

// PROGRAM

template <class T>
void process(const char* dest, const vector<string>& files, void (*loader)(const char*, vector<T>&), void (*writer)(const char*, vector<T>&)) {
	vector<T> dats;
	for (const string& it : files)
		loader(it.c_str(), dats);

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
	if (Arguments arg(argc, argv, {}, { argAudio, argMaterial, argObject, argShader, argTexture }); arg.getVals().empty())
		std::cout << "no input files" << linend << messageUsage << std::endl;
	else if (const char* dest = arg.getOpt(argAudio))
		process(dest, arg.getVals(), loadWav, writeAudios);
	else if (dest = arg.getOpt(argMaterial))
		process(dest, arg.getVals(), loadMtl, writeMaterials);
	else if (dest = arg.getOpt(argObject))
		process(dest, arg.getVals(), loadObj, writeObjects);
	else if (dest = arg.getOpt(argShader))
		process(dest, arg.getVals(), loadGlsl, writeShaders);
	else if (dest = arg.getOpt(argTexture))
		process(dest, arg.getVals(), loadBmp, writeImages);
	else {
		std::cout << "error: invalid mode" << linend << messageUsage << std::endl;
		return -1;
	}
	return 0;
}
