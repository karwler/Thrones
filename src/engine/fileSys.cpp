#include "world.h"
#ifndef _WIN32
#include <unistd.h>
#endif

// SETTINGS

const array<string, Settings::screenNames.size()> Settings::screenNames = {
	"window",
	"borderless",
	"fullscreen",
	"desktop"
};

const array<string, Settings::vsyncNames.size()> Settings::vsyncNames = {
	"adaptive",
	"immediate",
	"synchronized"
};

Settings::Settings() :
	maximized(false),
	avolume(0),
	display(0),
	screen(Screen::window),
	vsync(VSync::synchronized),
	msamples(4),
	gamma(1.f),
	size(1280, 720),
	mode({ SDL_PIXELFORMAT_RGB888, 1920, 1080, 60, nullptr }),
	address(loopback),
	port(Com::defaultPort)
{}

// FILE SYS

int FileSys::setWorkingDir() {
	char* path = SDL_GetBasePath();
	if (!path)
		return 1;
#ifdef _WIN32
	int err = _wchdir(stow(path).c_str());
#else
	int err = chdir(path);
#endif
	SDL_free(path);
	return err;
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (const string& line : readFileLines(fileSettings)) {
		if (pairStr il = readIniLine(line); il.first == iniKeywordMaximized)
			sets->maximized = stob(il.second);
		else if (il.first == iniKeywordDisplay)
			sets->display = clampHigh(uint8(sstoul(il.second)), uint8(SDL_GetNumVideoDisplays()));
		else if (il.first == iniKeywordScreen)
			sets->screen = strToEnum<Settings::Screen>(Settings::screenNames, il.second);
		else if (il.first == iniKeywordSize)
			sets->size = vec2i::get(il.second, strtoul, 0);
		else if (il.first == iniKeywordMode)
			sets->mode = strToDisp(il.second);
		else if (il.first == iniKeywordVsync)
			sets->vsync = Settings::VSync(strToEnum<int8>(Settings::vsyncNames, il.second) - 1);
		else if (il.first == iniKeywordMsamples)
			sets->msamples = uint8(sstoul(il.second));
		else if (il.first == iniKeywordGamma)
			sets->gamma = std::clamp(sstof(il.second), 0.f, Settings::gammaMax);
		else if (il.first == iniKeywordAVolume)
			sets->avolume = clampHigh(uint8(sstoul(il.second)), uint8(SDL_MIX_MAXVOLUME));
		else if (il.first == iniKeywordAddress)
			sets->address = il.second;
		else if (il.first == iniKeywordPort)
			sets->port = uint16(sstoul(il.second));
	}
	return sets;
}

bool FileSys::saveSettings(const Settings* sets) {
	string text;
	text += makeIniLine(iniKeywordMaximized, btos(sets->maximized));
	text += makeIniLine(iniKeywordDisplay, toStr(sets->display));
	text += makeIniLine(iniKeywordScreen, Settings::screenNames[uint8(sets->screen)]);
	text += makeIniLine(iniKeywordSize, sets->size.toString());
	text += makeIniLine(iniKeywordMode, dispToStr(sets->mode));
	text += makeIniLine(iniKeywordVsync, Settings::vsyncNames[uint8(int8(sets->vsync)+1)]);
	text += makeIniLine(iniKeywordMsamples, toStr(sets->msamples));
	text += makeIniLine(iniKeywordGamma, toStr(sets->gamma));
	text += makeIniLine(iniKeywordAVolume, toStr(sets->avolume));
	text += makeIniLine(iniKeywordAddress, sets->address);
	text += makeIniLine(iniKeywordPort, toStr(sets->port));
	return writeFile(fileSettings, text);
}

umap<string, Sound> FileSys::loadAudios(const SDL_AudioSpec& spec) {
	World::window()->writeLog("loading audio");
	FILE* ifh = fopen(fileAudios, defaultReadMode);
	if (!ifh) {
		std::cerr << errorAudios << std::endl;
		World::window()->writeLog(errorAudios);
		return {};
	}
	uint16 size;
	fread(&size, sizeof(size), 1, ifh);
	umap<string, Sound> auds(size);

	uint8 ibuf[audioHeaderSize];
	uint32* up = reinterpret_cast<uint32*>(ibuf + 2);
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 6);
	string name;
	Sound sound;
	for (uint16 i = 0; i < size; i++) {
		fread(ibuf, sizeof(*ibuf), audioHeaderSize, ifh);
		name.resize(ibuf[0]);
		sound.channels = ibuf[1];
		sound.length = up[0];
		sound.frequency = sp[0];
		sound.format = sp[1];
		sound.samples = sp[2];

		sound.data = static_cast<uint8*>(SDL_malloc(sound.length));
		fread(name.data(), sizeof(*name.data()), name.length(), ifh);
		fread(sound.data, sizeof(*sound.data), sound.length, ifh);
		if (sound.convert(spec))
			auds.emplace(std::move(name), sound);
		else {
			sound.free();
			std::cerr << errorFile << name << std::endl;
			World::window()->writeLog(errorFile + name);
		}
	}
	fclose(ifh);
	return auds;
}

umap<string, Material> FileSys::loadMaterials() {
	World::window()->writeLog("loading materials");
	FILE* ifh = fopen(fileMaterials, defaultReadMode);
	if (!ifh) {
		std::cerr << errorMaterials << std::endl;
		World::window()->writeLog(errorMaterials);
		return { pair("", Material()) };
	}
	uint16 size;
	fread(&size, sizeof(size), 1, ifh);
	umap<string, Material> mtls(size + 1);
	mtls.emplace();

	uint8 len;
	string name;
	Material matl;
	for (uint16 i = 0; i < size; i++) {
		fread(&len, sizeof(len), 1, ifh);
		name.resize(len);
		fread(name.data(), sizeof(*name.data()), name.length(), ifh);
		fread(&matl, sizeof(matl), 1, ifh);
		mtls.emplace(std::move(name), matl);
	}
	return mtls;
}

umap<string, GMesh> FileSys::loadObjects() {
	World::window()->writeLog("loading objects");
	FILE* ifh = fopen(fileObjects, defaultReadMode);
	if (!ifh) {
		std::cerr << errorObjects << std::endl;
		World::window()->writeLog(errorObjects);
		return umap<string, GMesh>({ pair("", GMesh()) });
	}
	glUseProgram(World::space()->program);
	uint16 size;
	fread(&size, sizeof(size), 1, ifh);
	umap<string, GMesh> mshs(size + 1);
	mshs.emplace();

	uint8 ibuf[objectHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	string name;
	vector<float> verts;
	vector<uint16> elems;
	for (uint16 i = 0; i < size; i++) {
		fread(ibuf, sizeof(*ibuf), objectHeaderSize, ifh);
		name.resize(ibuf[0]);
		elems.resize(sp[0]);
		verts.resize(sp[1]);

		fread(name.data(), sizeof(*name.data()), name.length(), ifh);
		fread(elems.data(), sizeof(*elems.data()), elems.size(), ifh);
		fread(verts.data(), sizeof(*verts.data()), verts.size(), ifh);
		mshs.emplace(std::move(name), GMesh(verts, elems));
	}
	fclose(ifh);
	return mshs;
}

umap<string, string> FileSys::loadShaders() {
	World::window()->writeLog("loading shaders");
	FILE* ifh = fopen(fileShaders, defaultReadMode);
	if (!ifh) {
		std::cerr << errorShaders << std::endl;
		World::window()->writeLog(errorShaders);
		return umap<string, string>({ pair("", "") });
	}
	uint8 size;
	fread(&size, sizeof(size), 1, ifh);
	umap<string, string> shds(size);

	uint8 ibuf[shaderHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	string name, text;
	for (uint8 i = 0; i < size; i++) {
		fread(ibuf, sizeof(*ibuf), shaderHeaderSize, ifh);
		name.resize(*ibuf);
		text.resize(*sp);

		fread(name.data(), sizeof(*name.data()), name.length(), ifh);
		fread(text.data(), sizeof(*text.data()), text.length(), ifh);
		shds.emplace(std::move(name), std::move(text));
	}
	return shds;
}

umap<string, Texture> FileSys::loadTextures() {
	World::window()->writeLog("loading textures");
	FILE* ifh = fopen(fileTextures, defaultReadMode);
	if (!ifh) {
		std::cerr << errorTextures << std::endl;
		World::window()->writeLog(errorTextures);
		return { pair("", Texture::loadBlank()) };
	}
	uint16 size;
	fread(&size, sizeof(size), 1, ifh);
	umap<string, Texture> texs(size + 1);
	texs.emplace("", Texture::loadBlank());

	uint8 ibuf[textureHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	vector<uint8> pixels;
	string name;
	for (uint16 i = 0; i < size; i++) {
		fread(ibuf, sizeof(*ibuf), textureHeaderSize, ifh);
		name.resize(ibuf[0]);
		vec2i res(sp[0], sp[1]);
		pixels.resize(uint(int(sp[2]) * res.y));

		fread(name.data(), sizeof(*name.data()), name.length(), ifh);
		fread(pixels.data(), sizeof(*pixels.data()), pixels.size(), ifh);
		texs.emplace(std::move(name), Texture(res, sp[3], sp[4], pixels.data()));
	}
	fclose(ifh);
	return texs;
}
