#include "oven.h"
#include "utils/text.h"
#include <regex>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

constexpr string_view mtlKeywordNewmtl = "newmtl";
constexpr char argJson = 'j';
constexpr char argMaterial = 'm';
constexpr char argObject = 'o';
constexpr char argShader = 's';
constexpr char argShaderE = 'S';
constexpr char argTexture = 't';
constexpr char argScale = 'd';
constexpr char messageUsage[] = "usage: oven <-m|-o|-s|-S|-t> [-d texture scale] <destination file/directory> <input files>";

#ifdef NDEBUG
static std::regex versionRgx(R"r(#version\s+\d+\s+\w*\s+)r", std::regex::optimize);
#else
static std::regex versionRgx(R"r(#version\s+\d+\s+\w*)r", std::regex::optimize);
#endif
static std::regex noperspectiveRgx(R"r(noperspective\s+)r", std::regex::optimize);

static void writeTextFile(const string& file, const string& dest, const string& text, const char* desc) {
	if (string_view name = filename(file); !text.empty()) {
		if (SDL_RWops* ofh = SDL_RWFromFile((dest + name).c_str(), defaultWriteMode)) {
			SDL_RWwrite(ofh, text.c_str(), sizeof(*text.c_str()), text.length());
			SDL_RWclose(ofh);
		} else
			logError("failed to write ", desc, ' ', name);
	} else
		logError("no text for ", desc, ' ', name);
}

static void processJson(const string& file, const string& dest) {
	string text = loadFile(file);
#ifdef NDEBUG
	for (string::iterator it = text.begin(); it != text.end();) {
		for (bool quotes = false; notSpace(*it) || (quotes && it != text.end()); ++it) {
			if (*it == '"')
				quotes = !quotes;
			else if (*it == '\\' && it + 1 != text.end())
				++it;
		}
		string::iterator et = it;
		for (; et != text.end() && isSpace(*et); ++et);
		text.erase(it, et);
	}
#endif
	writeTextFile(file, dest, text, "JSON");
}

static void processMaterials(const vector<string>& files, const string& dest) {
	vector<pair<string, Material>> mtls = { pair(string(), Material()) };
	for (const string& file : files) {
		vector<string> lines = readTextLines(loadFile(file));
		if (lines.empty()) {
			logError("failed to read ", file);
			continue;
		}

		for (const string& line : lines) {
			if (!SDL_strncasecmp(line.c_str(), mtlKeywordNewmtl.data(), mtlKeywordNewmtl.length())) {
				if (pair<string, Material> next(trim(string_view(line).substr(mtlKeywordNewmtl.length())), Material()); mtls.back().first.empty())
					mtls.back() = std::move(next);
				else
					mtls.push_back(std::move(next));
			} else if (char c0 = char(toupper(line[0])); c0 == 'K') {
				if (char c1 = char(toupper(line[1])); c1 == 'A' || c1 == 'D')	// ambient and diffuse always have the same value
					mtls.back().second.color = toVec<vec4>(string_view(line).substr(2), 1.f);
				else if (c1 == 'S')
					mtls.back().second.spec = toVec<vec3>(string_view(line).substr(2), 1.f);
			} else if (c0 == 'N') {
				if (toupper(line[1]) == 'S')
					mtls.back().second.shine = toNum<float>(string_view(line).substr(2)) / 1000.f * 128.f;
			} else if (c0 == 'D')
				mtls.back().second.color.a = toNum<float>(string_view(line).substr(1));
			else if (c0 == 'T' && toupper(line[1]) == 'R')
				mtls.back().second.color.a = 1.f - toNum<float>(string_view(line).substr(2));	// inverse of d
			else if (c0 == 'R') {
				if (char c1 = char(toupper(line[1])); c1 == 'L')
					mtls.back().second.reflect = toNum<float>(string_view(line).substr(2));
				else if (c1 == 'H')
					mtls.back().second.rough = toNum<float>(string_view(line).substr(2));
			}
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
	string_view name = filename(delExt(file));
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
			string dname = !mesh->mName.length || !SDL_strcasecmp(mesh->mName.C_Str(), "defaultobject") ? string(name) : mesh->mName.C_Str() + string(std::find_if_not(name.rbegin(), name.rend(), isdigit).base(), name.end());
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
	string text(trim(loadFile(file)));
#ifdef NDEBUG
	bool keepLF = false;
	for (sizet i = 0; i < text.length(); ++i) {
		if (checkSpace(text[i])) {
			sizet e = i;
			while (checkSpace(text[++e]));
			text.erase(i, e - i);
			if (checkText(text[i - 1]) && checkText(text[i]))
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
			if (text[i + 1] == '/') {
				text.erase(i, text.find_first_of('\n', i + 2) - i);
				--i;
			} else if (text[i + 1] == '*') {
				sizet e = text.find("*/", i);
				text.erase(i, e != string::npos ? e - i + 2 : e);
				--i;
			}
		}
	}
#endif
	if (gles) {
		text = std::regex_replace(text, versionRgx, "#version 300 es\nprecision highp float;precision highp int;precision highp sampler2D;precision highp sampler2DArray;precision highp samplerCube;");
		text = std::regex_replace(text, noperspectiveRgx, "");
	}
	writeTextFile(file, dest, text, "shader");
}

static void processTexture(const string& file, const string& dest, int scale) {
	SDL_Surface* img = IMG_Load(file.c_str());
	if (!img) {
		logError("failed to load ", file, ": ", IMG_GetError());
		return;
	}
	if (scale >= 2)
		if (img = scaleSurfaceReplace(img, ivec2(img->w, img->h) / scale); !img)
			return;

	if (string_view name = filename(delExt(file)); img->format->BytesPerPixel == 4 ? IMG_SavePNG(img, (dest + name + ".png").c_str()) : IMG_SaveJPG(img, (dest + name + ".jpg").c_str(), 100))
		logError("failed to write texture ", name, ": ", IMG_GetError());
	SDL_FreeSurface(img);
}

template <class F, class... A>
void process(const string& dest, const vector<string>& files, F proc, A&&... args) {
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
	if (SDL_Init(0) || IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) != (IMG_INIT_JPG | IMG_INIT_PNG)) {
		logError(SDL_GetError());
		return EXIT_FAILURE;
	}

	if (Arguments arg(argc, argv, {}, { argJson, argMaterial, argObject, argShader, argShaderE, argTexture, argScale }); arg.getVals().empty())
		logInfo("no input files", linend, messageUsage);
	else if (const char* dest = arg.getOpt(argJson); dest)
		process(dest, arg.getVals(), processJson);
	else if (dest = arg.getOpt(argMaterial); dest)
		processMaterials(arg.getVals(), dest);
	else if (dest = arg.getOpt(argObject); dest)
		process(dest, arg.getVals(), processObject);
	else if (dest = arg.getOpt(argShader); dest)
		process(dest, arg.getVals(), processShader, false);
	else if (dest = arg.getOpt(argShaderE); dest)
		process(dest, arg.getVals(), processShader, true);
	else if (dest = arg.getOpt(argTexture); dest) {
		const char* scl = arg.getOpt(argScale);
		process(dest, arg.getVals(), processTexture, scl ? toNum<uint>(scl) : 0);
	} else
		logInfo("invalid mode", linend, messageUsage);

	IMG_Quit();
	SDL_Quit();
	return EXIT_SUCCESS;
}
