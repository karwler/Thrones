#include "oven.h"
#include "utils/text.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

constexpr char mtlKeywordNewmtl[] = "newmtl";
constexpr char argMaterial = 'm';
constexpr char argObject = 'o';
constexpr char argShader = 's';
constexpr char argShaderE = 'S';
constexpr char argTexture = 't';
constexpr char argTextureR = 'T';
constexpr char messageUsage[] = "usage: oven <-m|-o|-s|-t> <destination file/directory> <input files>";
constexpr int maxWindowIconSize = 128;

static void processMaterials(const vector<string>& files, const string& dest) {
	vector<pair<string, Material>> mtls = { pair(string(), Material()) };
	for (const string& file : files) {
		vector<string> lines = readTextLines(loadFile(file));
		if (lines.empty()) {
			logError("failed to read ", file);
			continue;
		}

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
	if (mtls.size() > UINT16_MAX) {
		logError("the material count exceeds ", UINT16_MAX);
		return;
	}

	if (SDL_RWops* ofh = SDL_RWFromFile(dest.c_str(), defaultWriteMode)) {
		uint16 mamt = mtls.size();
		SDL_RWwrite(ofh, &mamt, sizeof(mamt), 1);
		for (const auto& [name, matl] : mtls) {
			uint16 nlen = name.length();
			SDL_RWwrite(ofh, &nlen, sizeof(nlen), 1);
			SDL_RWwrite(ofh, name.c_str(), sizeof(*name.c_str()), name.length());
			SDL_RWwrite(ofh, &matl, sizeof(float), sizeof(Material) / sizeof(float));
		}
		SDL_RWclose(ofh);
	} else
		logError("failed to write ", dest);
}

static void processObject(const string& file, const string& dest) {
	string name = filename(delExt(file));
	if (name.empty()) {
		logError("empty object name in ", file);
		return;
	}

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file, aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_PreTransformVertices | aiProcess_ImproveCacheLocality | aiProcess_FlipUVs | aiProcess_FlipWindingOrder);
	if (!(scene && scene->HasMeshes())) {
		logError("failed to read ", name, ": ", importer.GetErrorString());
		return;
	}

	for (uint m = 0; m < scene->mNumMeshes; ++m) {
		const aiMesh* mesh = scene->mMeshes[m];
		if (!(mesh->HasPositions() && mesh->HasFaces())) {
			logError("insufficient data in mesh ", name, "::", mesh->mName.C_Str());
			continue;
		}
		if (mesh->mNumVertices > UINT16_MAX) {
			logError("the vertex count of ", name, "::", mesh->mName.C_Str(), " exceeds ", UINT16_MAX);
			continue;
		}
		if (mesh->mNumFaces * 3 > UINT16_MAX) {
			logError("the element count of ", name, "::", mesh->mName.C_Str(), " exceeds ", UINT16_MAX);
			continue;
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
		uint i = 0;
		for (uint j = 0; i < mesh->mNumFaces; ++i, j += 3) {
			if (mesh->mFaces[i].mNumIndices == 3)
				std::copy_n(mesh->mFaces[i].mIndices, 3, elems.begin() + j);
			else {
				logError("invalid number of face indices ", mesh->mFaces[i].mNumIndices, " in ", name, "::", mesh->mName.C_Str());
				break;
			}
		}
		if (i == mesh->mNumFaces) {
			string dname = !mesh->mName.length || !SDL_strcasecmp(mesh->mName.C_Str(), "defaultobject") ? std::move(name) : mesh->mName.C_Str() + string(std::find_if_not(name.rbegin(), name.rend(), isdigit).base(), name.end());
			if (SDL_RWops* ofh = SDL_RWFromFile((dest + dname + ".dat").c_str(), defaultWriteMode)) {
				uint16 obuf[2] = { uint16(elems.size()), uint16(verts.size()) };
				SDL_RWwrite(ofh, obuf, sizeof(*obuf), 2);
				SDL_RWwrite(ofh, elems.data(), sizeof(*elems.data()), elems.size());
				SDL_RWwrite(ofh, verts.data(), sizeof(*verts.data()), verts.size());
				SDL_RWclose(ofh);
			} else
				logError("failed to write object ", dname);
		}
	}
}

#ifdef NDEBUG
static bool checkText(char c) {
	return isalnum(c) || c == '_';
}

static bool checkSpace(char c) {
	return isSpace(c) && c != '\n';
}
#endif

static void processShader(const string& file, const string& dest, bool gles) {
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

	if (string name = filename(file); !text.empty()) {
		if (SDL_RWops* ofh = SDL_RWFromFile((dest + name).c_str(), defaultWriteMode)) {
			SDL_RWwrite(ofh, text.c_str(), sizeof(*text.c_str()), text.length());
			SDL_RWclose(ofh);
		} else
			logError("failed to write shader ", name);
	} else
		logError("no text for shader ", name);
}

static void processTexture(const string& file, const string& dest, bool gles) {
	SDL_Surface* img = IMG_Load(file.c_str());
	if (!img) {
		logError("failed to load ", file, ": ", IMG_GetError());
		return;
	}

	if (gles) {
		if (img = scaleSurface(img, 2); !img)
			return;
	} else if (strciEndsWith(file, ".svg") && (img->w > maxWindowIconSize || img->h > maxWindowIconSize))
		if (img = scaleSurface(img, ivec2(maxWindowIconSize)); !img)
			return;

	string name = filename(delExt(file));
	if (img->format->BytesPerPixel != 4) {
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(img);
		if (!dst) {
			logError("failed to convert ", file, ": ", SDL_GetError());
			return;
		}
		img = dst;
	}
	if (IMG_SavePNG(img, (dest + name + ".png").c_str()))
		logError("failed to write write texture ", name, ": ", IMG_GetError());
	SDL_FreeSurface(img);
}

template <class F, class... A>
void process(const char* dest, const vector<string>& files, F proc, A... args) {
	if (createDirectories(dest)) {
		string dirPath = appDsep(dest);
		for (const string& it : files)
			proc(it, dirPath, std::forward<A>(args)...);
	} else
		logError("failed to create directory ", dest);

}

#if defined(_WIN32) && !defined(__MINGW32__)
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
#endif
	if (SDL_Init(0) || IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		logError(SDL_GetError());
		return -1;
	}

	if (Arguments arg(argc, argv, {}, { argMaterial, argObject, argShader, argShaderE, argTexture, argTextureR }); arg.getVals().empty())
		logInfo("no input files\n", messageUsage);
	else if (const char* dest = arg.getOpt(argMaterial); dest)
		processMaterials(arg.getVals(), dest);
	else if (dest = arg.getOpt(argObject); dest)
		process(dest, arg.getVals(), processObject);
	else if (dest = arg.getOpt(argShader); dest)
		process(dest, arg.getVals(), processShader, false);
	else if (dest = arg.getOpt(argShaderE); dest)
		process(dest, arg.getVals(), processShader, true);
	else if (dest = arg.getOpt(argTexture); dest)
		process(dest, arg.getVals(), processTexture, false);
	else if (dest = arg.getOpt(argTextureR); dest)
		process(dest, arg.getVals(), processTexture, true);
	else
		logInfo("invalid mode\n", messageUsage);

	IMG_Quit();
	SDL_Quit();
	return 0;
}
