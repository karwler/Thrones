#include "oven.h"
#include "utils/text.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>

constexpr char mtlKeywordNewmtl[] = "newmtl";
constexpr char argAudio = 'a';
constexpr char argMaterial = 'm';
constexpr char argObject = 'o';
constexpr char argShader = 's';
constexpr char argShaderE = 'S';
constexpr char argTexture = 't';
constexpr char argTextureR = 'T';
constexpr char messageUsage[] = "usage: oven <-a|-m|-o|-s|-t> <destination file> <input files>";

// OBJ FILE DATA

struct Blueprint {
	string name;
	vector<GLushort> elems;
	vector<Vertex> data;
	uint8 level;

	Blueprint(string&& capt, vector<GLushort>&& elements, vector<Vertex>&& vertices, uint8 lvl);

	bool empty() const;
};

Blueprint::Blueprint(string&& capt, vector<GLushort>&& elements, vector<Vertex>&& vertices, uint8 lvl) :
	name(std::move(capt)),
	elems(std::move(elements)),
	data(std::move(vertices)),
	level(lvl)
{}

bool Blueprint::empty() const {
	return elems.empty() || data.empty();
}

// TEXTURE FILE DATA

struct PixelData {
	vector<uint8> data;
	ivec2 res = ivec2(0);
	SDL_PixelFormatEnum format;

	PixelData() = default;
	PixelData(vector<uint8>&& pdata, ivec2 resolution, SDL_PixelFormatEnum pixelformat);
};

PixelData::PixelData(vector<uint8>&& pdata, ivec2 resolution, SDL_PixelFormatEnum pixelformat) :
	data(std::move(pdata)),
	res(resolution),
	format(pixelformat)
{}

struct Image {
	string name;
	vector<PixelData> pdat;
	TexturePlacement place;

	Image(string&& fname, vector<PixelData>&& data, TexturePlacement tpl);
};

Image::Image(string&& fname, vector<PixelData>&& data, TexturePlacement tpl) :
	name(std::move(fname)),
	pdat(std::move(data)),
	place(tpl)
{}

// UTILITY

template <class T = string>
T loadFile(const string& path) {
	T data;
#ifdef _WIN32
	if (FILE* ifh = _wfopen(sstow(path).c_str(), L"rb")) {
#else
	if (FILE* ifh = fopen(path.c_str(), defaultReadMode)) {
#endif
		if (!fseek(ifh, 0, SEEK_END))
			if (long len = ftell(ifh); len != -1 && !fseek(ifh, 0, SEEK_SET)) {
				data.resize(ulong(len));
				if (sizet red = fread(data.data(), sizeof(*data.data()), data.size(), ifh); red < data.size())
					data.resize(red);
			}
		fclose(ifh);
	}
	return data;
}

// AUDIOS

static void loadWav(const char* file, vector<pair<string, Sound>>& auds) {
	Sound sound;
	if (SDL_AudioSpec spec; SDL_LoadWAV(file, &spec, &sound.data, &sound.length)) {
		if (sound.set(spec); sound.convert(Sound::defaultSpec))
			auds.emplace_back(filename(delExt(file)), sound);
		else {
			sound.free();
			std::cerr << "error: couldn't convert " << file << linend << SDL_GetError() << std::endl;
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
	writeMem(obuf, uint16(auds.size()));
	SDL_RWwrite(ofh, obuf, sizeof(uint16), 1);

	for (auto& [name, sound] : auds) {
		obuf[0] = name.length();
		obuf[1] = sound.channels;
		writeMem(obuf + 2, sound.length);
		writeMem(obuf + 6, sound.frequency);
		writeMem(obuf + 8, sound.format);
		writeMem(obuf + 10, sound.samples);

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), audioHeaderSize);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, sound.data, sizeof(*sound.data), sound.length);
		sound.free();
	}
	SDL_RWclose(ofh);
}

// MATERIALS

static void loadMtl(const char* file, vector<pair<string, Material>>& mtls) {
	vector<string> lines = readTextLines(loadFile(file));
	if (lines.empty()) {
		std::cerr << "error: couldn't read " << file << std::endl;
		return;
	}
	mtls.emplace_back();

	for (const string& line : lines) {
		if (sizet len = strlen(mtlKeywordNewmtl); !SDL_strncasecmp(line.c_str(), mtlKeywordNewmtl, len)) {
			if (pair<string, Material> next(trim(line.substr(len)), Material()); mtls.back().first.empty())
				mtls.back() = std::move(next);
			else
				mtls.push_back(std::move(next));
		} else if (char c0 = char(toupper(line[0])); c0 == 'K') {
			if (char c1 = char(toupper(line[1])); c1 == 'A' || c1 == 'D')	// ambient and diffuse always have the same value
				mtls.back().second.color = stofv<vec4>(&line[2], strtof, 1.f);
			else if (c1 == 'S')
				mtls.back().second.spec = stofv<vec3>(&line[2], strtof, 1.f);
		} else if (c0 == 'N') {
			if (toupper(line[1]) == 'S')
				mtls.back().second.shine = sstof(&line[2]) / 1000.f * 128.f;
		} else if (c0 == 'D')
			mtls.back().second.color.a = sstof(&line[1]);
		else if (c0 == 'T' && toupper(line[1]) == 'R')
			mtls.back().second.color.a = 1.f - sstof(&line[2]);	// inverse of d
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
	uint16 mamt = mtls.size();
	SDL_RWwrite(ofh, &mamt, sizeof(mamt), 1);

	for (const auto& [name, matl] : mtls) {
		uint8 nlen = name.length();
		SDL_RWwrite(ofh, &nlen, sizeof(nlen), 1);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, &matl, sizeof(float), sizeof(Material) / sizeof(float));
	}
	SDL_RWclose(ofh);
}

// OBJECTS

static void loadObj(const char* file, vector<Blueprint>& bprs) {
	uint8 level = UINT8_MAX;
	string name = filename(delExt(file));
	if (name.empty()) {
		std::cerr << "error: empty object name in " << file << std::endl;
		return;
	}
	if (isdigit(name.back()) && name.length() > 1) {
		level = name.back() - '0';
		name.pop_back();
	}

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file, aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_PreTransformVertices | aiProcess_ImproveCacheLocality | aiProcess_FlipUVs | aiProcess_FlipWindingOrder);
	if (!(scene && scene->HasMeshes())) {
		std::cerr << "error: failed to read " << name << std::endl;
		return;
	}

	const aiMesh* mesh = scene->mMeshes[0];
	if (!(mesh->HasPositions() && mesh->HasFaces())) {
		std::cerr << "error: insufficient data in mesh " << name << std::endl;
		return;
	}

	vector<Vertex> verts(mesh->mNumVertices);
	for (uint i = 0; i < mesh->mNumVertices; ++i) {
		verts[i].pos = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		verts[i].nrm = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		if (mesh->HasTextureCoords(0) && mesh->HasTangentsAndBitangents()) {
			verts[i].tuv = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			verts[i].tng = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		} else {
			verts[i].tuv = vec2(0.f);
			verts[i].tng = vec3(0.f);
		}
	}

	vector<GLushort> elems(mesh->mNumFaces * 3);
	for (uint i = 0, j = 0; i < mesh->mNumFaces; ++i, j += 3) {
		if (mesh->mFaces[i].mNumIndices == 3)
			std::copy_n(mesh->mFaces[i].mIndices, 3, elems.begin() + j);
		else
			std::cerr << "invalid number of face indices " << mesh->mFaces[i].mNumIndices << " in " << name << std::endl;
	}
	bprs.emplace_back(std::move(name), std::move(elems), std::move(verts), level);
}

static void writeObjects(const char* file, vector<Blueprint>& bprs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	std::sort(bprs.begin(), bprs.end(), [](const Blueprint& a, const Blueprint& b) -> bool { return a.level < b.level; });
	uint8 obuf[objectHeaderSize];
	writeMem(obuf, uint16(bprs.size()));
	SDL_RWwrite(ofh, obuf, sizeof(uint16), 1);

	for (const Blueprint& it : bprs) {
		if (it.data.size() > 64000)
			std::cout << "warning: " << it.name << "'s size of " << it.data.size() << " exceeds 8000 vertices" << std::endl;

		obuf[0] = it.name.length();
		writeMem(obuf + 1, uint16(it.elems.size()));
		writeMem(obuf + 3, uint16(it.data.size()));

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), objectHeaderSize);
		SDL_RWwrite(ofh, it.name.c_str(), sizeof(*it.name.c_str()), it.name.length());
		SDL_RWwrite(ofh, it.elems.data(), sizeof(*it.elems.data()), it.elems.size());
		SDL_RWwrite(ofh, it.data.data(), sizeof(*it.data.data()), it.data.size());
	}
	SDL_RWclose(ofh);
}

// SHADERS

#ifdef NDEBUG
static bool checkText(char c) {
	return isalnum(c) || c == '_';
}

static bool checkSpace(char c) {
	return isSpace(c) && c != '\n';
}
#endif

static void loadGlsl(const char* file, vector<pairStr>& srcs, bool gles) {
	string text = trim(loadFile(file));
#ifdef NDEBUG
	bool keepLF = false;
	for (sizet i = 0; i < text.length(); ++i) {
		if (checkSpace(text[i])) {
			sizet e = i;
			while (checkSpace(text[++e]));
			text.erase(i, e - i);
			if (checkText(text[i-1]) && checkText(text[i]))
				text.insert(i, 1, ' ');
			else
				--i;
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
				--i;
			} else if (text[i+1] == '*') {
				sizet e = text.find("*/", i);
				text.erase(i, e != string::npos ? e - i + 2 : e);
				--i;
			}
		}
	}
#endif
	if (gles) {
		constexpr char verstr[] = "#version ";
		if (sizet p = text.find(verstr); p < text.length()) {
			p += strlen(verstr);
			text.replace(p, text.find('\n', p) - p + 1, "300 es\nprecision highp float;precision highp int;precision highp sampler2D;precision highp sampler2DArray;precision highp samplerCube;");
		}
	}
	if (!text.empty())
		srcs.emplace_back(filename(file), std::move(text));
}

static void writeShaders(const char* file, vector<pairStr>& srcs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << "error: couldn't write " << file << std::endl;
		return;
	}
	uint8 obuf[shaderHeaderSize] = { uint8(srcs.size()) };
	SDL_RWwrite(ofh, obuf, sizeof(*obuf), 1);

	for (const auto& [name, text] : srcs) {
		obuf[0] = name.length();
		writeMem(obuf + 1, uint16(text.length()));

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), shaderHeaderSize);
		SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
		SDL_RWwrite(ofh, text.c_str(), sizeof(*text.c_str()), text.length());
	}
	SDL_RWclose(ofh);
}

// TEXTURES

static PixelData loadImageFile(const char* file, const string& name, bool gles) {
	vector<uint8> data = loadFile<vector<uint8>>(file);
	SDL_Surface* img = IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE);
	if (!img) {
		std::cerr << "failed to load " << name << ": " << IMG_GetError() << std::endl;
		return PixelData();
	}

	bool renew = gles;
	if (renew)
		if (img = scaleSurface(img, 2); !img)
			return PixelData();
	if (img->format->BytesPerPixel < 4) {
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(img);
		if (!dst) {
			std::cerr << "failed to convert " << name << ": " << SDL_GetError() << std::endl;
			return PixelData();
		}
		img = dst;
		renew = true;
	}

	if (renew) {
		data.resize(sizet(img->pitch) * sizet(img->h) * 2);	// double the size just in case it ends up big
		SDL_RWops* dw = SDL_RWFromMem(data.data(), data.size());
		if (IMG_SavePNG_RW(img, dw, SDL_FALSE)) {
			SDL_RWclose(dw);
			SDL_FreeSurface(img);
			std::cerr << "failed to save " << name << ": " << IMG_GetError() << std::endl;
			return PixelData();
		}
		data.resize(SDL_RWtell(dw));
		SDL_RWclose(dw);
	}
	PixelData pret(std::move(data), ivec2(img->w, img->h), SDL_PixelFormatEnum(img->format->format));
	SDL_FreeSurface(img);
	return pret;
}

static void loadImg(const char* file, vector<Image>& imgs, bool gles) {
	string name = filename(delExt(file));
	string::iterator num = std::find_if_not(name.rbegin(), name.rend(), isdigit).base();
	if (num == name.begin() || num == name.end()) {
		std::cerr << "invalid texture name: " << name << std::endl;
		return;
	}
	TexturePlacement place = TexturePlacement(*num - '0');
	if (place > TEXPLACE_CUBE || (place == TEXPLACE_CUBE ? *(num + 1) < '0' || *(num + 1) > '5' || num + 2 != name.end() : num + 1 != name.end())) {
		std::cerr << "texture placement out of bounds: " << name << std::endl;
		return;
	}
	name.erase(num, name.end());

	vector<PixelData> pdat(place != TEXPLACE_CUBE ? place & TEXPLACE_OBJECT ? 2 : 1 : 6);
	if (place != TEXPLACE_CUBE) {
		pdat[0] = loadImageFile(file, name, gles);
		if (pdat[0].data.empty())
			return;

		if (place & TEXPLACE_OBJECT) {
			string normalFile = file;
			normalFile[normalFile.rfind('.')-1] = 'N';
			pdat[1] = loadImageFile(normalFile.c_str(), name, gles);
		}
	} else {
		string cubeFile = file;
		sizet idid = cubeFile.rfind('.') - 1;
		for (uint8 i = 0; i < pdat.size(); ++i) {
			cubeFile[idid] = '0' + i;
			if (pdat[i] = loadImageFile(cubeFile.c_str(), name, gles); pdat[i].data.empty())
				return;
		}
	}
	imgs.emplace_back(std::move(name), std::move(pdat), place);
}

static void writeImages(const char* file, vector<Image>& imgs) {
	SDL_RWops* ofh = SDL_RWFromFile(file, defaultWriteMode);
	if (!ofh) {
		std::cerr << SDL_GetError() << std::endl;
		return;
	}
	uint8 obuf[textureHeaderSize];
	writeMem(obuf, uint16(imgs.size()));
	SDL_RWwrite(ofh, obuf, sizeof(uint16), 1);

	for (Image& it : imgs) {
		obuf[0] = it.place;
		obuf[1] = it.name.length();

		SDL_RWwrite(ofh, obuf, sizeof(*obuf), textureHeaderSize);
		SDL_RWwrite(ofh, it.name.c_str(), sizeof(*it.name.c_str()), it.name.length());
		for (const PixelData& pd : it.pdat) {
			uint32 dataSize = pd.data.size();
			SDL_RWwrite(ofh, &dataSize, sizeof(dataSize), 1);
			SDL_RWwrite(ofh, pd.data.data(), sizeof(*pd.data.data()), pd.data.size());
		}
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
		std::cout << "nothing to write for " << dest << std::endl;
}

#if defined(_WIN32) && !defined(__MINGW32__)
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
#endif
	if (SDL_Init(0) || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		std::cerr << SDL_GetError() << std::endl;
		return -1;
	}

	if (Arguments arg(argc, argv, {}, { argAudio, argMaterial, argObject, argShader, argShaderE, argTexture, argTextureR }); arg.getVals().empty())
		std::cout << "no input files" << linend << messageUsage << std::endl;
	else if (const char* dest = arg.getOpt(argAudio))
		process(dest, arg.getVals(), loadWav, writeAudios);
	else if (dest = arg.getOpt(argMaterial); dest)
		process(dest, arg.getVals(), loadMtl, writeMaterials);
	else if (dest = arg.getOpt(argObject); dest)
		process(dest, arg.getVals(), loadObj, writeObjects);
	else if (dest = arg.getOpt(argShader); dest)
		process(dest, arg.getVals(), loadGlsl, writeShaders, false);
	else if (dest = arg.getOpt(argShaderE); dest)
		process(dest, arg.getVals(), loadGlsl, writeShaders, true);
	else if (dest = arg.getOpt(argTexture); dest)
		process(dest, arg.getVals(), loadImg, writeImages, false);
	else if (dest = arg.getOpt(argTextureR); dest)
		process(dest, arg.getVals(), loadImg, writeImages, true);
	else
		std::cout << "error: invalid mode" << linend << messageUsage << std::endl;

	IMG_Quit();
	SDL_Quit();
	return 0;
}
