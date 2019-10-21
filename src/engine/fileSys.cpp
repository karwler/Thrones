#include "world.h"

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
#ifdef __ANDROID__
	screen(Screen::desktop),
#else
	screen(Screen::window),
#endif
	vsync(VSync::synchronized),
	msamples(4),
	gamma(1.f),
	size(1280, 720),
	mode({ SDL_PIXELFORMAT_RGB888, 1920, 1080, 60, nullptr }),
	address(loopback),
	port(Com::defaultPort)
{}

// SETUP

void Setup::clear() {
	tiles.clear();
	mids.clear();
	pieces.clear();
}

// FILE SYS

string FileSys::dirData, FileSys::dirConfig;

void FileSys::init() {
#ifdef __ANDROID__
	if (const char* path = SDL_AndroidGetExternalStoragePath())
		dirConfig = path + string("/");
#else
	const char* eopt = World::args.getOpt(World::argExternal);
#ifdef EXTERNAL
	bool external = eopt ? stob(eopt) : true;
#else
	bool external = eopt ? stob(eopt) : false;
#endif
	if (char* path = SDL_GetBasePath()) {
#ifdef __APPLE__
		dirData = path;
#else
		dirData = path + string("data/");
#endif
		if (SDL_free(path); !external)
			dirConfig = dirData;
	}
#ifdef _WIN32
	if (const char* path = SDL_getenv("AppData"); external && path) {
		dirConfig = path + string("/Thrones/");
		std::replace(dirConfig.begin(), dirConfig.end(), '\\', '/');
		createDir(dirConfig);
	}
#elif defined(__APPLE__)
	if (const char* path = SDL_getenv("HOME"); external && path) {
		dirConfig = path + string("/Library/Preferences/Thrones/");
		createDir(dirConfig);
	}
#else
	if (const char* path = SDL_getenv("HOME"); external && path) {
		dirConfig = path + string("/.config/thrones/");
		createDir(dirConfig);
	}
#endif
#endif
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (const string& line : readFileLines(configPath(fileSettings))) {
		pairStr il = readIniLine(line);
#ifndef __ANDROID__
		if (il.first == iniKeywordMaximized)
			sets->maximized = stob(il.second);
		else if (il.first == iniKeywordDisplay)
			sets->display = std::clamp(uint8(sstoul(il.second)), uint8(0), uint8(SDL_GetNumVideoDisplays()));
		else if (il.first == iniKeywordScreen)
			sets->screen = strToEnum<Settings::Screen>(Settings::screenNames, il.second);
		else if (il.first == iniKeywordSize)
			sets->size = stoiv<ivec2>(il.second.c_str(), strtoul);
		else if (il.first == iniKeywordMode)
			sets->mode = strToDisp(il.second);
		else
#endif
#ifndef OPENGLES
		if (il.first == iniKeywordMsamples)
			sets->msamples = uint8(sstoul(il.second));
		else
#endif
		if (il.first == iniKeywordVsync)
			sets->vsync = Settings::VSync(strToEnum<int8>(Settings::vsyncNames, il.second) - 1);
		else if (il.first == iniKeywordGamma)
			sets->gamma = std::clamp(sstof(il.second), 0.f, Settings::gammaMax);
		else if (il.first == iniKeywordAVolume)
			sets->avolume = std::clamp(uint8(sstoul(il.second)), uint8(0), uint8(SDL_MIX_MAXVOLUME));
		else if (il.first == iniKeywordAddress)
			sets->address = il.second;
		else if (il.first == iniKeywordPort)
			sets->port = uint16(sstoul(il.second));
	}
	return sets;
}

bool FileSys::saveSettings(const Settings* sets) {
	string text;
#ifndef __ANDROID__
	text += makeIniLine(iniKeywordMaximized, btos(sets->maximized));
	text += makeIniLine(iniKeywordDisplay, toStr(sets->display));
	text += makeIniLine(iniKeywordScreen, Settings::screenNames[uint8(sets->screen)]);
	text += makeIniLine(iniKeywordSize, toStr(sets->size));
	text += makeIniLine(iniKeywordMode, dispToStr(sets->mode));
#endif
#ifndef OPENGLES
	text += makeIniLine(iniKeywordMsamples, toStr(sets->msamples));
#endif
	text += makeIniLine(iniKeywordVsync, Settings::vsyncNames[uint8(int8(sets->vsync)+1)]);
	text += makeIniLine(iniKeywordGamma, toStr(sets->gamma));
	text += makeIniLine(iniKeywordAVolume, toStr(sets->avolume));
	text += makeIniLine(iniKeywordAddress, sets->address);
	text += makeIniLine(iniKeywordPort, toStr(sets->port));
	return writeFile(configPath(fileSettings), text);
}

umap<string, Com::Config> FileSys::loadConfigs(const char* file) {
	umap<string, Com::Config> confs;
	Com::Config* cit = nullptr;
	for (const string& line : readFileLines(configPath(file))) {
		if (string title = readIniTitle(line); !title.empty())
			cit = &confs.emplace(std::move(title), Com::Config()).first->second;
		else if (cit) {
			if (pairStr it = readIniLine(line); !strcicmp(it.first, iniKeywordBoardSize))
				cit->homeSize = stoiv<nvec2>(it.second.c_str(), strtoul);
			else if (!strcicmp(it.first, iniKeywordSurvival))
				cit->survivalPass = uint8(sstoul(it.second));
			else if (!strcicmp(it.first, iniKeywordFavors))
				cit->favorLimit = uint8(sstoul(it.second));
			else if (!strcicmp(it.first, iniKeywordDragonDist))
				cit->dragonDist = uint8(sstoul(it.second));
			else if (!strcicmp(it.first, iniKeywordDragonDiag))
				cit->dragonDiag = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordMultistage))
				cit->multistage = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordSurvivalKill))
				cit->survivalKill = stob(it.second);
			else if (sizet len = strlen(iniKeywordTile); !strncicmp(it.first, iniKeywordTile, len))
				readAmount(it, len, Com::tileNames, cit->tileAmounts);
			else if (len = strlen(iniKeywordMiddle); !strncicmp(it.first, iniKeywordMiddle, len))
				readAmount(it, len, Com::tileNames, cit->middleAmounts);
			else if (len = strlen(iniKeywordPiece); !strncicmp(it.first, iniKeywordPiece, len))
				readAmount(it, len, Com::pieceNames, cit->pieceAmounts);
			else if (!strcicmp(it.first, iniKeywordWinFortress))
				cit->winFortress = uint16(sstol(it.second));
			else if (!strcicmp(it.first, iniKeywordWinThrone))
				cit->winThrone = uint16(sstol(it.second));
			else if (!strcicmp(it.first, iniKeywordCapturers))
				cit->readCapturers(it.second);
			else if (!strcicmp(it.first, iniKeywordShift))
				readShift(it.second, *cit);
		}
	}
	return !confs.empty() ? confs : umap<string, Com::Config>{ pair(Com::Config::defaultName, Com::Config()) };
}

void FileSys::readShift(const string& line, Com::Config& conf) {
	for (const char* pos = line.c_str(); *pos;) {
		if (string word = readWordM(pos); !strcicmp(word, iniKeywordLeft))
			conf.shiftLeft = true;
		else if (!strcicmp(word, iniKeywordRight))
			conf.shiftLeft = false;
		else if (!strcicmp(word, iniKeywordNear))
			conf.shiftNear = true;
		else if (!strcicmp(word, iniKeywordFar))
			conf.shiftNear = false;
	}
}

bool FileSys::saveConfigs(const umap<string, Com::Config>& confs, const char* file) {
	string text;
	for (const pair<const string, Com::Config>& it : confs) {
		text += makeIniLine(it.first);
		text += makeIniLine(iniKeywordBoardSize, toStr(it.second.homeSize));
		text += makeIniLine(iniKeywordSurvival, toStr(it.second.survivalPass));
		text += makeIniLine(iniKeywordFavors, toStr(it.second.favorLimit));
		text += makeIniLine(iniKeywordDragonDist, toStr(it.second.dragonDist));
		text += makeIniLine(iniKeywordDragonDiag, btos(it.second.dragonDiag));
		text += makeIniLine(iniKeywordMultistage, btos(it.second.multistage));
		text += makeIniLine(iniKeywordSurvivalKill, btos(it.second.survivalKill));
		writeAmounts(text, iniKeywordTile, Com::tileNames, it.second.tileAmounts);
		writeAmounts(text, iniKeywordMiddle, Com::tileNames, it.second.middleAmounts);
		writeAmounts(text, iniKeywordPiece, Com::pieceNames, it.second.pieceAmounts);
		text += makeIniLine(iniKeywordWinFortress, toStr(it.second.winFortress));
		text += makeIniLine(iniKeywordWinThrone, toStr(it.second.winThrone));
		text += makeIniLine(iniKeywordCapturers, it.second.capturersString());
		text += makeIniLine(iniKeywordShift, string(it.second.shiftLeft ? iniKeywordLeft : iniKeywordRight) + ' ' + (it.second.shiftNear ? iniKeywordNear : iniKeywordFar));
		text += linend;
	}
	return writeFile(configPath(file), text);
}

umap<string, Setup> FileSys::loadSetups() {
	umap<string, Setup> sets;
	umap<string, Setup>::iterator sit;
	for (const string& line : readFileLines(configPath(fileSetups))) {
		if (string title = readIniTitle(line); !title.empty())
			sit = sets.emplace(std::move(title), Setup()).first;
		else if (pairStr it = readIniLine(line); !sets.empty()) {
			if (sizet len = strlen(iniKeywordTile); !strncicmp(it.first, iniKeywordTile, len))
				sit->second.tiles.emplace(stoiv<svec2>(it.first.c_str() + len, strtol), strToEnum<Com::Tile>(Com::tileNames, it.second));
			else if (len = strlen(iniKeywordMiddle); !strncicmp(it.first, iniKeywordMiddle, len))
				sit->second.mids.emplace(uint16(sstoul(it.first.substr(len))), strToEnum<Com::Tile>(Com::tileNames, it.second));
			else if (len = strlen(iniKeywordPiece); !strncicmp(it.first, iniKeywordPiece, len))
				sit->second.pieces.emplace(stoiv<svec2>(it.first.c_str() + len, strtol), strToEnum<Com::Piece>(Com::pieceNames, it.second));
		}
	}
	return sets;
}

bool FileSys::saveSetups(const umap<string, Setup>& sets) {
	string text;
	for (const pair<const string, Setup>& it : sets) {
		text += makeIniLine(it.first);
		for (const pair<svec2, Com::Tile>& ti : it.second.tiles)
			text += makeIniLine(iniKeywordTile + toStr(ti.first, "_"), Com::tileNames[uint8(ti.second)]);
		for (const pair<uint16, Com::Tile>& mi : it.second.mids)
			text += makeIniLine(iniKeywordMiddle + toStr(mi.first), Com::tileNames[uint8(mi.second)]);
		for (const pair<svec2, Com::Piece>& pi : it.second.pieces)
			text += makeIniLine(iniKeywordPiece + toStr(pi.first, "_"), Com::pieceNames[uint8(pi.second)]);
		text += linend;
	}
	return writeFile(configPath(fileSetups), text);
}

umap<string, Sound> FileSys::loadAudios(const SDL_AudioSpec& spec) {
	World::window()->writeLog("loading audio");
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileAudios).c_str(), defaultReadMode);
	if (!ifh) {
		constexpr char errorAudios[] = "failed to load sounds";
		std::cerr << errorAudios << std::endl;
		World::window()->writeLog(errorAudios);
		return {};
	}
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Sound> auds(size);

	uint8 ibuf[audioHeaderSize];
	uint32* up = reinterpret_cast<uint32*>(ibuf + 2);
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 6);
	string name;
	Sound sound;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), audioHeaderSize);
		name.resize(ibuf[0]);
		sound.channels = ibuf[1];
		sound.length = up[0];
		sound.frequency = sp[0];
		sound.format = sp[1];
		sound.samples = sp[2];

		sound.data = static_cast<uint8*>(SDL_malloc(sound.length));
		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, sound.data, sizeof(*sound.data), sound.length);
		if (sound.convert(spec))
			auds.emplace(std::move(name), sound);
		else {
			sound.free();
			constexpr char errorFile[] = "failed to load ";
			std::cerr << errorFile << name << std::endl;
			World::window()->writeLog(errorFile + name);
		}
	}
	SDL_RWclose(ifh);
	return auds;
}

umap<string, Material> FileSys::loadMaterials() {
	World::window()->writeLog("loading materials");
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileMaterials).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load materials");

	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Material> mtls(size + 1);
	mtls.emplace();

	uint8 len;
	string name;
	Material matl;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, &len, sizeof(len), 1);
		name.resize(len);
		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, &matl, sizeof(matl), 1);
		mtls.emplace(std::move(name), matl);
	}
	SDL_RWclose(ifh);
	return mtls;
}

umap<string, GMesh> FileSys::loadObjects() {
	World::window()->writeLog("loading objects");
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileObjects).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load objects");

	glUseProgram(World::geom()->program);
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, GMesh> mshs(size + 1);
	mshs.emplace();

	uint8 ibuf[objectHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	string name;
	vector<Vertex> verts;
	vector<uint16> elems;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), objectHeaderSize);
		name.resize(ibuf[0]);
		elems.resize(sp[0]);
		verts.resize(sp[1]);

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, elems.data(), sizeof(*elems.data()), elems.size());
		SDL_RWread(ifh, verts.data(), sizeof(*verts.data()), verts.size());
		mshs.emplace(std::move(name), GMesh(verts, elems));
	}
	SDL_RWclose(ifh);
	return mshs;
}

umap<string, string> FileSys::loadShaders() {
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileShaders).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load shaders");

	uint8 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, string> shds(size);

	uint8 ibuf[shaderHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	string name, text;
	for (uint8 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), shaderHeaderSize);
		name.resize(*ibuf);
		text.resize(*sp);

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, text.data(), sizeof(*text.data()), text.length());
		shds.emplace(std::move(name), std::move(text));
	}
	SDL_RWclose(ifh);
	return shds;
}

umap<string, Texture> FileSys::loadTextures() {
	World::window()->writeLog("loading textures");
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileTextures).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load textures");

	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Texture> texs(size + 1);
	texs.emplace(string(), Texture::loadBlank());

	uint8 ibuf[textureHeaderSize];
	uint16* sp = reinterpret_cast<uint16*>(ibuf + 1);
	vector<uint8> pixels;
	string name;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), textureHeaderSize);
		name.resize(ibuf[0]);
		ivec2 res(sp[0], sp[1]);
		pixels.resize(uint(int(sp[2]) * res.y));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, pixels.data(), sizeof(*pixels.data()), pixels.size());
		texs.emplace(std::move(name), Texture(res, sp[3], sp[4], pixels.data()));
	}
	SDL_RWclose(ifh);
	return texs;
}
