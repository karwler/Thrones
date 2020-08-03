#include "windowSys.h"

// SETTINGS

Settings::Settings() :
	address(defaultAddress),
	port(Com::defaultPort),
	versionLookup(defaultVersionLocation, defaultVersionRegex),
	mode{ SDL_PIXELFORMAT_RGB888, 1920, 1080, 60, nullptr },
	size(1280, 720),
	gamma(1.f),
#ifdef OPENGLES
	shadowRes(0),
#else
	shadowRes(1024),
#endif
	chatLines(511),
	deadzone(12000),
	display(0),
#ifdef __ANDROID__
	screen(Screen::desktop),
#else
	screen(defaultScreen),
#endif
	vsync(defaultVSync),
	msamples(4),
	texScale(100),
	softShadows(true),
	avolume(0),
	colorAlly(defaultAlly),
	colorEnemy(defaultEnemy),
	scaleTiles(true),
	scalePieces(false),
	tooltips(true),
	resolveFamily(defaultFamily),
	fontRegular(true)
{}

// SETUP

void Setup::clear() {
	tiles.clear();
	mids.clear();
	pieces.clear();
}

// FILE SYS

void FileSys::init(const Arguments& args) {
#ifdef __ANDROID__
	if (const char* path = SDL_AndroidGetExternalStoragePath())
		dirConfig = path + string("/");
#elif defined(EMSCRIPTEN)
	dirData = "/";
	dirConfig = "/data/";
	EM_ASM(
		FS.mkdir('/data');
		FS.mount(IDBFS, {}, '/data');
		Module.syncdone = 0;
		FS.syncfs(true, function (err) {
			Module.syncdone = 1;
		});
	);
#else
	if (char* path = SDL_GetBasePath()) {
#ifdef __APPLE__
		dirData = path;
#else
		dirData = path + string("data/");
#ifdef _WIN32
		std::replace(dirData.begin(), dirData.end(), '\\', '/');
#endif
#endif
		SDL_free(path);
	}

#ifdef EXTERNAL
	if (const char* eopt = args.getOpt(Settings::argExternal); eopt ? stob(eopt) : true) {
#else
	if (const char* eopt = args.getOpt(Settings::argExternal); eopt ? stob(eopt) : false) {
#endif
#ifdef _WIN32
		if (const wchar* path = _wgetenv(L"AppData")) {
			dirConfig = wtos(path) + string("/Thrones/");
			std::replace(dirConfig.begin(), dirConfig.end(), '\\', '/');
		}
#elif defined(__APPLE__)
		if (const char* path = getenv("HOME"))
			dirConfig = path + string("/Library/Preferences/Thrones/");
#else
		if (const char* path = getenv("HOME"))
			dirConfig = path + string("/.config/thrones/");
#endif
		createDirectories(dirConfig);
	} else
		dirConfig = dirData;
#endif
}

Settings FileSys::loadSettings() {
	Settings sets;
	for (const string& line : readTextLines(loadFile(configPath(fileSettings)))) {
		pairStr il = readIniLine(line);
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		if (!strcicmp(il.first, iniKeywordDisplay))
			sets.display = uint8(std::min(sstoul(il.second), ulong(SDL_GetNumVideoDisplays())));
		else
#endif
#ifndef __ANDROID__
		if (!strcicmp(il.first, iniKeywordScreen))
			sets.screen = strToEnum(Settings::screenNames, il.second, Settings::defaultScreen);
		else if (!strcicmp(il.first, iniKeywordSize))
			sets.size = stoiv<ivec2>(il.second.c_str(), strtoul);
		else if (!strcicmp(il.first, iniKeywordMode))
			sets.mode = strToDisp(il.second);
		else
#endif
#ifndef OPENGLES
		if (!strcicmp(il.first, iniKeywordMsamples))
			sets.msamples = uint8(std::min(sstoul(il.second), 8ul));
		else if (!strcicmp(il.first, iniKeywordShadowRes))
			sets.shadowRes = uint16(std::min(sstoul(il.second), ulong(Settings::shadowResMax)));
		else if (!strcicmp(il.first, iniKeywordSoftShadows))
			sets.softShadows = stob(il.second);
		else
#endif
		if (!strcicmp(il.first, iniKeywordTexScale))
			sets.texScale = uint8(std::clamp(sstoul(il.second), 1ul, 100ul));
		else if (!strcicmp(il.first, iniKeywordVsync))
			sets.vsync = strToEnum(Settings::vsyncNames, il.second, Settings::defaultVSync + 1) - 1;
		else if (!strcicmp(il.first, iniKeywordGamma))
			sets.gamma = std::clamp(sstof(il.second), 0.f, Settings::gammaMax);
		else if (!strcicmp(il.first, iniKeywordAVolume))
			sets.avolume = uint8(std::min(sstoul(il.second), ulong(SDL_MIX_MAXVOLUME)));
		else if (!strcicmp(il.first, iniKeywordColorAlly))
			sets.colorAlly = strToEnum(Settings::colorNames, il.second, Settings::defaultAlly);
		else if (!strcicmp(il.first, iniKeywordColorEnemy))
			sets.colorEnemy = strToEnum(Settings::colorNames, il.second, Settings::defaultEnemy);
		else if (!strcicmp(il.first, iniKeywordScaleTiles))
			sets.scaleTiles = stob(il.second);
		else if (!strcicmp(il.first, iniKeywordScalePieces))
			sets.scalePieces = stob(il.second);
		else if (!strcicmp(il.first, iniKeywordTooltips))
			sets.tooltips = stob(il.second);
		else if (!strcicmp(il.first, iniKeywordChatLines))
			sets.chatLines = uint16(std::min(stoul(il.second), ulong(Settings::chatLinesMax)));
		else if (!strcicmp(il.first, iniKeywordDeadzone))
			sets.deadzone = uint16(std::min(stoul(il.second), SHRT_MAX + 1ul));
		else if (!strcicmp(il.first, iniKeywordResolveFamily))
			sets.resolveFamily = strToEnum(Com::familyNames, il.second, Settings::defaultFamily);
		else if (!strcicmp(il.first, iniKeywordFontRegular))
			sets.fontRegular = stob(il.second);
#ifdef WEBUTILS
		else if (!strcicmp(il.first, iniKeywordVersionLookup))
			readVersionLookup(il.second.c_str(), sets.versionLookup);
#endif
		else if (!strcicmp(il.first, iniKeywordAddress))
			sets.address = std::move(il.second);
		else if (!strcicmp(il.first, iniKeywordPort))
			sets.port = std::move(il.second);
	}
	return sets;
}

void FileSys::readVersionLookup(const char* str, pair<string, string>& vl) {
	if (vl.first = strUnenclose(str); vl.first.empty())
		vl.first = Settings::defaultVersionLocation;
	if (vl.second = strUnenclose(str); vl.second.empty())
		vl.second = Settings::defaultVersionRegex;
}

void FileSys::saveSettings(const Settings* sets) {
	string text;
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
	writeIniLine(text, iniKeywordDisplay, toStr(sets->display));
#endif
#ifndef __ANDROID__
	writeIniLine(text, iniKeywordScreen, Settings::screenNames[uint8(sets->screen)]);
	writeIniLine(text, iniKeywordSize, toStr(sets->size));
	writeIniLine(text, iniKeywordMode, toStr(sets->mode.w) + ' ' + toStr(sets->mode.h) + ' ' + toStr(sets->mode.refresh_rate) + ' ' + toStr(sets->mode.format));
#endif
#ifndef OPENGLES
	writeIniLine(text, iniKeywordMsamples, toStr(sets->msamples));
	writeIniLine(text, iniKeywordShadowRes, toStr(sets->shadowRes));
	writeIniLine(text, iniKeywordSoftShadows, btos(sets->softShadows));
#endif
	writeIniLine(text, iniKeywordTexScale, toStr(sets->texScale));
	writeIniLine(text, iniKeywordVsync, Settings::vsyncNames[uint8(sets->vsync+1)]);
	writeIniLine(text, iniKeywordGamma, toStr(sets->gamma));
	writeIniLine(text, iniKeywordAVolume, toStr(sets->avolume));
	writeIniLine(text, iniKeywordColorAlly, Settings::colorNames[uint8(sets->colorAlly)]);
	writeIniLine(text, iniKeywordColorEnemy, Settings::colorNames[uint8(sets->colorEnemy)]);
	writeIniLine(text, iniKeywordScaleTiles, btos(sets->scaleTiles));
	writeIniLine(text, iniKeywordScalePieces, btos(sets->scalePieces));
	writeIniLine(text, iniKeywordTooltips, btos(sets->tooltips));
	writeIniLine(text, iniKeywordChatLines, toStr(sets->chatLines));
	writeIniLine(text, iniKeywordDeadzone, toStr(sets->deadzone));
	writeIniLine(text, iniKeywordResolveFamily, Com::familyNames[uint8(sets->resolveFamily)]);
	writeIniLine(text, iniKeywordFontRegular, btos(sets->fontRegular));
#ifdef WEBUTILS
	writeIniLine(text, iniKeywordVersionLookup, strEnclose(sets->versionLookup.first) + ' ' + strEnclose(sets->versionLookup.second));
#endif
	writeIniLine(text, iniKeywordAddress, sets->address);
	writeIniLine(text, iniKeywordPort, sets->port);
	writeFile(configPath(fileSettings), text);
}

umap<string, Com::Config> FileSys::loadConfigs() {
	umap<string, Com::Config> confs;
	Com::Config* cit = nullptr;
	for (const string& line : readTextLines(loadFile(configPath(fileConfigs)))) {
		if (string title = readIniTitle(line); !title.empty())
			cit = &confs.emplace(std::move(title), Com::Config()).first->second;
		else if (cit) {
			if (pairStr it = readIniLine(line); !strcicmp(it.first, iniKeywordGameType))
				cit->gameType = strToEnum(Com::Config::gameTypeNames, it.second, Com::Config::defaultGameType);
			else if (!strcicmp(it.first, iniKeywordPorts))
				cit->ports = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordRowBalancing))
				cit->rowBalancing = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordSetPieceBattle))
				readPiecePool(it.second.c_str(), cit);
			else if (!strcicmp(it.first, iniKeywordBoardSize))
				cit->homeSize = stoiv<svec2>(it.second.c_str(), strtol);
			else if (!strcicmp(it.first, iniKeywordBattlePass))
				cit->battlePass = uint8(sstol(it.second));
			else if (!strcicmp(it.first, iniKeywordFavorTotal))
				cit->favorTotal = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordFavorLimit))
				cit->favorLimit = uint8(sstol(it.second));
			else if (!strcicmp(it.first, iniKeywordDragonLate))
				cit->dragonLate = stob(it.second);
			else if (!strcicmp(it.first, iniKeywordDragonStraight))
				cit->dragonStraight = stob(it.second);
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
		}
	}
	return !confs.empty() ? confs : umap<string, Com::Config>{ pair(Com::Config::defaultName, Com::Config()) };
}

void FileSys::readPiecePool(const char* str, Com::Config* cfg) {
	cfg->setPieceOn = stob(readWordM(str));
	cfg->setPieceNum = uint16(sstoul(str));
}

template <sizet N, sizet S>
void FileSys::readAmount(const pairStr& it, sizet wlen, const array<const char*, N>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, it.first.substr(wlen)); id < amts.size())
		amts[id] = uint16(sstol(it.second));
}

void FileSys::saveConfigs(const umap<string, Com::Config>& confs) {
	string text;
	for (auto& [name, cfg] : confs) {
		writeIniLine(text, name);
		//writeIniLine(text, iniKeywordGameType, Com::Config::gameTypeNames[uint8(cfg.gameType)]);
		writeIniLine(text, iniKeywordPorts, btos(cfg.ports));
		writeIniLine(text, iniKeywordRowBalancing, btos(cfg.rowBalancing));
		writeIniLine(text, iniKeywordSetPieceBattle, string(btos(cfg.setPieceOn)) + ' ' + toStr(cfg.setPieceNum));
		writeIniLine(text, iniKeywordBoardSize, toStr(cfg.homeSize));
		writeIniLine(text, iniKeywordBattlePass, toStr(cfg.battlePass));
		writeIniLine(text, iniKeywordFavorTotal, btos(cfg.favorTotal));
		writeIniLine(text, iniKeywordFavorLimit, toStr(cfg.favorLimit));
		writeIniLine(text, iniKeywordDragonLate, btos(cfg.dragonLate));
		writeIniLine(text, iniKeywordDragonStraight, btos(cfg.dragonStraight));
		writeAmounts(text, iniKeywordTile, Com::tileNames, cfg.tileAmounts);
		writeAmounts(text, iniKeywordMiddle, Com::tileNames, cfg.middleAmounts);
		writeAmounts(text, iniKeywordPiece, Com::pieceNames, cfg.pieceAmounts);
		writeIniLine(text, iniKeywordWinFortress, toStr(cfg.winFortress));
		writeIniLine(text, iniKeywordWinThrone, toStr(cfg.winThrone));
		writeIniLine(text, iniKeywordCapturers, cfg.capturersString());
		text += linend;
	}
	writeFile(configPath(fileConfigs), text);
}

template <sizet N, sizet S>
void FileSys::writeAmounts(string& text, const string& word, const array<const char*, N>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); i++)
		writeIniLine(text, word + names[i], toStr(amts[i]));
}

umap<string, Setup> FileSys::loadSetups() {
	umap<string, Setup> sets;
	umap<string, Setup>::iterator sit;
	for (const string& line : readTextLines(loadFile(configPath(fileSetups)))) {
		if (string title = readIniTitle(line); !title.empty())
			sit = sets.emplace(std::move(title), Setup()).first;
		else if (pairStr it = readIniLine(line); !sets.empty()) {
			if (sizet len = strlen(iniKeywordTile); !strncicmp(it.first, iniKeywordTile, len)) {
				if (Com::Tile t = strToEnum<Com::Tile>(Com::tileNames, it.second); t < Com::Tile::fortress)
					sit->second.tiles.emplace_back(stoiv<svec2>(it.first.c_str() + len, strtol), t);
			} else if (len = strlen(iniKeywordMiddle); !strncicmp(it.first, iniKeywordMiddle, len)) {
				if (Com::Tile t = strToEnum<Com::Tile>(Com::tileNames, it.second); t < Com::Tile::fortress)
					sit->second.mids.emplace_back(uint16(sstoul(it.first.substr(len))), t);
			} else if (len = strlen(iniKeywordPiece); !strncicmp(it.first, iniKeywordPiece, len))
				if (Com::Piece t = strToEnum<Com::Piece>(Com::pieceNames, it.second); t <= Com::Piece::throne)
					sit->second.pieces.emplace_back(stoiv<svec2>(it.first.c_str() + len, strtol), t);
		}
	}
	return sets;
}

void FileSys::saveSetups(const umap<string, Setup>& sets) {
	string text;
	for (auto& [name, stp] : sets) {
		writeIniLine(text, name);
		for (auto& [pos, type] : stp.tiles)
			writeIniLine(text, iniKeywordTile + toStr(pos, "_"), Com::tileNames[uint8(type)]);
		for (auto& [pos, type] : stp.mids)
			writeIniLine(text, iniKeywordMiddle + toStr(pos), Com::tileNames[uint8(type)]);
		for (auto& [pos, type] : stp.pieces)
			writeIniLine(text, iniKeywordPiece + toStr(pos, "_"), Com::pieceNames[uint8(type)]);
		text += linend;
	}
	writeFile(configPath(fileSetups), text);
}

umap<string, Sound> FileSys::loadAudios(const SDL_AudioSpec& spec) {
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileAudios).c_str(), defaultReadMode);
	if (!ifh) {
		std::cerr << "failed to load sounds" << std::endl;
		return {};
	}
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Sound> auds(size);

	uint8 ibuf[audioHeaderSize];
	string name;
	Sound sound;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), audioHeaderSize);
		name.resize(ibuf[0]);
		sound.channels = ibuf[1];
		sound.length = readMem<uint32>(ibuf + 2);
		sound.frequency = readMem<uint16>(ibuf + 6);
		sound.format = readMem<uint16>(ibuf + 8);
		sound.samples = readMem<uint16>(ibuf + 10);

		sound.data = static_cast<uint8*>(SDL_malloc(sound.length));
		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, sound.data, sizeof(*sound.data), sound.length);
		if (sound.convert(spec))
			auds.emplace(std::move(name), sound);
		else {
			sound.free();
			std::cerr << "failed to load " << name << std::endl;
		}
	}
	SDL_RWclose(ifh);
	return auds;
}

umap<string, Material> FileSys::loadMaterials() {
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

umap<string, Mesh> FileSys::loadObjects(const ShaderGeometry* geom) {
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileObjects).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load objects");

	glUseProgram(*geom);
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Mesh> mshs(size + 1);
	mshs.emplace();

	uint8 ibuf[objectHeaderSize];
	string name;
	vector<Vertex> verts;
	vector<uint16> elems;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), objectHeaderSize);
		name.resize(ibuf[0]);
		elems.resize(readMem<uint16>(ibuf + 1));
		verts.resize(readMem<uint16>(ibuf + 3));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, elems.data(), sizeof(*elems.data()), elems.size());
		SDL_RWread(ifh, verts.data(), sizeof(*verts.data()), verts.size());
		mshs.emplace(std::move(name), Mesh(verts, elems));
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
	string name, text;
	for (uint8 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), shaderHeaderSize);
		name.resize(ibuf[0]);
		text.resize(readMem<uint16>(ibuf + 1));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, text.data(), sizeof(*text.data()), text.length());
		shds.emplace(std::move(name), std::move(text));
	}
	SDL_RWclose(ifh);
	return shds;
}

umap<string, Texture> FileSys::loadTextures(int scale) {
	umap<string, Texture> texs = { pair(string(), Texture({ 255, 255, 255 })) };
	loadTextures(texs, [](umap<string, Texture>& txv, string&& name, SDL_Surface* img, GLint iform, GLenum pform) { txv.emplace(std::move(name), Texture(img, iform, pform)); }, scale);
	return texs;
}

void FileSys::loadTextures(umap<string, Texture>& texs, void (*inset)(umap<string, Texture>&, string&&, SDL_Surface*, GLint, GLenum), int scale) {
	SDL_RWops* ifh = SDL_RWFromFile(dataPath(fileTextures).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load textures");

	scale = 100 / scale;
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);

	uint8 ibuf[textureHeaderSize];
	vector<uint8> imgd;
	string name;
	for (uint16 i = 0; i < size; i++) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), textureHeaderSize);
		name.resize(ibuf[0]);
		imgd.resize(readMem<uint32>(ibuf + 1));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, imgd.data(), sizeof(*imgd.data()), imgd.size());
		if (SDL_Surface* img = scaleSurface(IMG_Load_RW(SDL_RWFromMem(imgd.data(), int(imgd.size())), SDL_TRUE), scale))
			inset(texs, std::move(name), img, readMem<uint16>(ibuf + 5), readMem<uint16>(ibuf + 7));
		else
			std::cerr << "failed to load " << name << ": " << SDL_GetError() << std::endl;
	}
	SDL_RWclose(ifh);
}

void FileSys::writeFile(const string& path, const string& text) {
	SDL_RWops* ofh = SDL_RWFromFile(path.c_str(), defaultWriteMode);
	if (!ofh) {
		std::cerr << "failed to write " << path << std::endl;
		return;
	}
	SDL_RWwrite(ofh, text.c_str(), sizeof(*text.c_str()), text.length());
	SDL_RWclose(ofh);
#ifdef EMSCRIPTEN
	EM_ASM(
		Module.syncdone = 0;
		FS.syncfs(function(err) {
			Module.syncdone = 1;
		});
	);
#endif
}

#ifdef EMSCRIPTEN
bool FileSys::canRead() {
	return emscripten_run_script_int("Module.syncdone");
}
#endif
