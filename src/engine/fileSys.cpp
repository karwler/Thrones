#include "fileSys.h"
#include "inputSys.h"
#include "utils/objects.h"
#include <iostream>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

// INI LINE

IniLine::IniLine(Type what, string&& property, string&& keyval, string&& value) :
	type(what),
	prp(std::move(property)),
	key(std::move(keyval)),
	val(std::move(value))
{}

IniLine::Type IniLine::read(const string& str) {
	sizet i0 = str.find_first_of('=');
	sizet i1 = str.find_first_of('[');
	sizet i2 = str.find_first_of(']', i1);
	if (i0 != string::npos) {
		val = str.substr(i0 + 1);
		if (i2 < i0) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return type = prpKeyVal;
		}
		prp = trim(str.substr(0, i0));
		key.clear();
		return type = prpVal;
	}
	if (i2 != string::npos) {
		prp = str.substr(i1 + 1, i2 - i1 - 1);
		key.clear();
		val.clear();
		return type = title;
	}
	prp.clear();
	val.clear();
	key.clear();
	return type = empty;
}

// FILE SYS

void FileSys::init(const Arguments& args) {
#ifdef __ANDROID__
	if (const char* path = SDL_AndroidGetExternalStoragePath())
		dirConfig = path + string("/");
#elif defined(EMSCRIPTEN)
	dirBase = "/";
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
		dirBase = path;
#ifdef _WIN32
		std::replace(dirBase.begin(), dirBase.end(), '\\', '/');
#elif !defined(__APPLE__)
		dirBase = parentPath(dirBase);
#endif
		SDL_free(path);
	}

#ifdef EXTERNAL
	if (const char* eopt = args.getOpt(Settings::argExternal); !eopt || stob(eopt)) {
#else
	if (const char* eopt = args.getOpt(Settings::argExternal); eopt && stob(eopt)) {
#endif
#ifdef _WIN32
		if (const wchar* path = _wgetenv(L"AppData")) {
			dirConfig = cwtos(path) + "/Thrones/";
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
		dirConfig = dirBase;
#endif
}

Settings FileSys::loadSettings(InputSys* input) {
	Settings sets;
	input->clearBindings();
	void (*reader)(void*, IniLine&) = readSetting;
	void* data = &sets;
	for (IniLine il : readTextLines(readFile(configPath(fileSettings)))) {
		if (il.type == IniLine::title) {
			bool bindings = il.prp == iniTitleBindings;
			reader = bindings ? readBinding : readSetting;
			data = bindings ? static_cast<void*>(input) : &sets;
		} else if (il.type != IniLine::empty)
			reader(data, il);
	}
	if (std::all_of(input->getBindings().begin(), input->getBindings().end(), [](const Binding& it) -> bool { return it.keys.empty() && it.joys.empty() && it.gpds.empty(); }))
		input->resetBindings();
	return sets;
}

void FileSys::readSetting(void* settings, IniLine& il) {
	if (il.type != IniLine::prpVal)
		return;

	Settings& sets = *static_cast<Settings*>(settings);
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDisplay))
		sets.display = uint8(std::min(sstoul(il.val), ulong(SDL_GetNumVideoDisplays())));
	else
#endif
#ifndef __ANDROID__
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordScreen))
		sets.screen = strToEnum(Settings::screenNames, il.val, Settings::defaultScreen);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSize))
		sets.size = stoiv<ivec2>(il.val.c_str(), strtoul);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMode))
		sets.mode = strToDisp(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVersionLookup))
		readVersionLookup(il.val.c_str(), sets);
	else
#endif
#ifndef OPENGLES
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMsamples))
		sets.msamples = uint8(std::min(sstoul(il.val), 8ul));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordShadows))
		readShadows(il.val.c_str(), sets);
	else
#endif
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTexScale))
		sets.texScale = uint8(std::clamp(sstoul(il.val), 1ul, 100ul));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVsync))
		sets.vsync = strToEnum(Settings::vsyncNames, il.val, Settings::defaultVSync + 1) - 1;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordGamma))
		sets.gamma = std::clamp(sstof(il.val), 0.f, Settings::gammaMax);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAVolume))
		sets.avolume = uint8(std::min(sstoul(il.val), ulong(SDL_MIX_MAXVOLUME)));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordColors))
		readColors(il.val.c_str(), sets);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordScales))
		readScales(il.val.c_str(), sets);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAutoVictoryPoints))
		sets.autoVictoryPoints = stob(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTooltips))
		sets.tooltips = stob(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordChatLines))
		sets.chatLines = uint16(std::min(stoul(il.val), ulong(Settings::chatLinesMax)));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDeadzone))
		sets.deadzone = uint16(std::clamp(stoul(il.val), ulong(Settings::deadzoneLimit.x), ulong(Settings::deadzoneLimit.y)));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordResolveFamily))
		sets.resolveFamily = strToEnum(Settings::familyNames, il.val, Settings::defaultFamily);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFont))
		sets.font = il.val;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordInvertWheel))
		sets.invertWheel = stob(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAddress))
		sets.address = std::move(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPort))
		sets.port = std::move(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordLastConfig))
		sets.lastConfig = std::move(il.val);
}

void FileSys::readShadows(const char* str, Settings& sets) {
	if (string word = readWord(str); !word.empty())
		sets.shadowRes = uint16(std::min(sstoul(word), ulong(Settings::shadowResMax)));
	if (string word = readWord(str); !word.empty())
		sets.softShadows = stob(word);
}

void FileSys::readColors(const char* str, Settings& sets) {
	sets.colorAlly = strToEnum(Settings::colorNames, readWord(str), Settings::defaultAlly);
	sets.colorEnemy = strToEnum(Settings::colorNames, readWord(str), Settings::defaultEnemy);
}

void FileSys::readScales(const char* str, Settings& sets) {
	if (string word = readWord(str); !word.empty())
		sets.scaleTiles = stob(word);
	if (string word = readWord(str); !word.empty())
		sets.scalePieces = stob(word);
}

void FileSys::readVersionLookup(const char* str, Settings& sets) {
	if (sets.versionLookupUrl = strUnenclose(str); sets.versionLookupUrl.empty())
		sets.versionLookupUrl = Settings::defaultVersionLocation;
	if (sets.versionLookupRegex = strUnenclose(str); sets.versionLookupRegex.empty())
		sets.versionLookupRegex = Settings::defaultVersionRegex;
}

void FileSys::readBinding(void* inputSys, IniLine& il) {
	if (il.type != IniLine::Type::prpKeyVal)
		return;
	Binding::Type bid = strToEnum<Binding::Type>(Binding::names, il.prp);
	if (uint8(bid) >= Binding::names.size())
		return;

	InputSys* input = static_cast<InputSys*>(inputSys);
	const char* pos = il.val.c_str();
	if (!SDL_strcasecmp(il.key.c_str(), iniKeyKey)) {
		while (*pos)
			if (SDL_Scancode cde = SDL_GetScancodeFromName(strUnenclose(pos).c_str()); cde != SDL_SCANCODE_UNKNOWN)
				input->addBinding(bid, cde);
	} else if (!SDL_strcasecmp(il.key.c_str(), iniKeyJoy)) {
		while (*pos) {
			for (; isSpace(*pos); ++pos);
			switch (*pos++) {
			case iniVkeyButton:
				input->addBinding(bid, JoystickButton(readNumber<uint8>(pos, strtoul, 0)));
				break;
			case iniVkeyHat:
				if (uint8 id = readNumber<uint8>(pos, strtoul, 0), val = strToVal<uint8>(InputSys::hatNames, readWord(++pos), SDL_HAT_CENTERED); val != SDL_HAT_CENTERED)
					input->addBinding(bid, AsgJoystick(id, val));
				break;
			case iniVkeyAxis:
				if (char sign = *pos++; sign == '+' || sign == '-')
					input->addBinding(bid, AsgJoystick(JoystickAxis(readNumber<uint8>(pos, strtoul, 0)), sign == '+'));
				else if (!sign)
					--pos;
				break;
			case '\0':
				--pos;
			}
		}
	} else if (!SDL_strcasecmp(il.key.c_str(), iniKeyGpd))
		while (*pos) {
			for (; isSpace(*pos); ++pos);
			switch (*pos++) {
			case iniVkeyButton:
				if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(InputSys::gbuttonNames, readWord(pos)); cid < SDL_CONTROLLER_BUTTON_MAX)
					input->addBinding(bid, cid);
				break;
			case iniVkeyAxis:
				if (char sign = *pos++; sign == '+' || sign == '-') {
					if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(InputSys::gaxisNames, readWord(pos)); cid < SDL_CONTROLLER_AXIS_MAX)
						input->addBinding(bid, AsgGamepad(cid, sign == '+'));
				} else if (!sign)
					--pos;
				break;
			case '\0':
				--pos;
			}
		}
}

void FileSys::saveSettings(const Settings& sets, const InputSys* input) {
	string text;
	IniLine::write(text, iniTitleSettings);
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
	IniLine::write(text, iniKeywordDisplay, toStr(sets.display));
#endif
#ifndef __ANDROID__
	IniLine::write(text, iniKeywordScreen, Settings::screenNames[uint8(sets.screen)]);
	IniLine::write(text, iniKeywordSize, toStr(sets.size));
	IniLine::write(text, iniKeywordMode, toStr(sets.mode.w) + ' ' + toStr(sets.mode.h) + ' ' + toStr(sets.mode.refresh_rate) + ' ' + toStr(sets.mode.format));
	IniLine::write(text, iniKeywordVersionLookup, strEnclose(sets.versionLookupUrl) + ' ' + strEnclose(sets.versionLookupRegex));
#endif
#ifndef OPENGLES
	IniLine::write(text, iniKeywordMsamples, toStr(sets.msamples));
	IniLine::write(text, iniKeywordShadows, toStr(sets.shadowRes) + ' ' + btos(sets.softShadows));
#endif
	IniLine::write(text, iniKeywordTexScale, toStr(sets.texScale));
	IniLine::write(text, iniKeywordVsync, Settings::vsyncNames[uint8(sets.vsync+1)]);
	IniLine::write(text, iniKeywordGamma, toStr(sets.gamma));
	IniLine::write(text, iniKeywordAVolume, toStr(sets.avolume));
	IniLine::write(text, iniKeywordColors, string(Settings::colorNames[uint8(sets.colorAlly)]) + ' ' + Settings::colorNames[uint8(sets.colorEnemy)]);
	IniLine::write(text, iniKeywordScales, string(btos(sets.scaleTiles)) + ' ' + btos(sets.scalePieces));
	IniLine::write(text, iniKeywordAutoVictoryPoints, btos(sets.autoVictoryPoints));
	IniLine::write(text, iniKeywordTooltips, btos(sets.tooltips));
	IniLine::write(text, iniKeywordChatLines, toStr(sets.chatLines));
	IniLine::write(text, iniKeywordDeadzone, toStr(sets.deadzone));
	IniLine::write(text, iniKeywordResolveFamily, Settings::familyNames[uint8(sets.resolveFamily)]);
	IniLine::write(text, iniKeywordFont, sets.font);
	IniLine::write(text, iniKeywordInvertWheel, btos(sets.invertWheel));
	IniLine::write(text, iniKeywordAddress, sets.address);
	IniLine::write(text, iniKeywordPort, sets.port);
	IniLine::write(text, iniKeywordLastConfig, sets.lastConfig);
	text += linend;

	IniLine::write(text, iniTitleBindings);
	for (uint8 i = 0; i < uint8(Binding::names.size()); ++i) {
		if (!input->getBinding(Binding::Type(i)).keys.empty())
			IniLine::write(text, Binding::names[i], iniKeyKey, strJoin(input->getBinding(Binding::Type(i)).keys, [](SDL_Scancode sc) -> string { return strEnclose(SDL_GetScancodeName(sc)); }));
		if (!input->getBinding(Binding::Type(i)).joys.empty())
			IniLine::write(text, Binding::names[i], iniKeyJoy, strJoin(input->getBinding(Binding::Type(i)).joys, [](AsgJoystick aj) -> string {
				switch (aj.getAsg()) {
				case AsgJoystick::button:
					return iniVkeyButton + toStr(aj.getJct());
				case AsgJoystick::hat:
					return iniVkeyHat + toStr(aj.getJct()) + '_' + InputSys::hatNames.at(aj.getHvl());
				case AsgJoystick::axisPos:
					return iniVkeyAxis + string(1, '+') + toStr(aj.getJct());
				case AsgJoystick::axisNeg:
					return iniVkeyAxis + string(1, '-') + toStr(aj.getJct());
				}
				return string();
			}));
		if (!input->getBinding(Binding::Type(i)).gpds.empty())
			IniLine::write(text, Binding::names[i], iniKeyGpd, strJoin(input->getBinding(Binding::Type(i)).gpds, [](AsgGamepad ag) -> string {
				switch (ag.getAsg()) {
				case AsgGamepad::button:
					return iniVkeyButton + string(InputSys::gbuttonNames[uint8(ag.getButton())]);
				case AsgGamepad::axisPos:
					return iniVkeyAxis + string(1, '+') + InputSys::gaxisNames[uint8((ag.getAxis()))];
				case AsgGamepad::axisNeg:
					return iniVkeyAxis + string(1, '-') + InputSys::gaxisNames[uint8((ag.getAxis()))];
				}
				return string();
			}));
	}
	writeFile(configPath(fileSettings), text);
}

umap<string, Config> FileSys::loadConfigs() {
	umap<string, Config> confs;
	Config* cit = nullptr;
	for (IniLine il : readTextLines(readFile(configPath(fileConfigs))))
		switch (il.type) {
		case IniLine::title:
			if (il.prp.length() > Config::maxNameLength)
				il.prp.resize(Config::maxNameLength);
			cit = &confs.emplace(std::move(il.prp), Config()).first->second;
			break;
		case IniLine::prpVal:
			if (!cit)
				break;
			if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVictoryPoints))
				readVictoryPoints(il.val.c_str(), cit);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPorts))
				cit->opts = stob(il.val) ? cit->opts | Config::ports : cit->opts & ~Config::ports;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordRowBalancing))
				cit->opts = stob(il.val) ? cit->opts | Config::rowBalancing : cit->opts & ~Config::rowBalancing;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordHomefront))
				cit->opts = stob(il.val) ? cit->opts | Config::homefront : cit->opts & ~Config::homefront;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSetPieceBattle))
				readSetPieceBattle(il.val.c_str(), cit);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordBoardSize))
				cit->homeSize = stoiv<svec2>(il.val.c_str(), strtol);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordBattlePass))
				cit->battlePass = uint8(sstol(il.val));
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFavorLimit))
				readFavorLimit(il.val.c_str(), cit);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFirstTurnEngage))
				cit->opts = stob(il.val) ? cit->opts | Config::firstTurnEngage : cit->opts & ~Config::firstTurnEngage;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTerrainRules))
				cit->opts = stob(il.val) ? cit->opts | Config::terrainRules : cit->opts & ~Config::terrainRules;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDragonLate))
				cit->opts = stob(il.val) ? cit->opts | Config::dragonLate : cit->opts & ~Config::dragonLate;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDragonStraight))
				cit->opts = stob(il.val) ? cit->opts | Config::dragonStraight : cit->opts & ~Config::dragonStraight;
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordWinFortress))
				cit->winFortress = uint16(sstol(il.val));
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordWinThrone))
				cit->winThrone = uint16(sstol(il.val));
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordCapturers))
				cit->capturers = readCapturers(il.val);
			break;
		case IniLine::prpKeyVal:
			if (!cit)
				break;
			if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTile))
				readAmount(il, tileNames, cit->tileAmounts);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMiddle))
				readAmount(il, tileNames, cit->middleAmounts);
			else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPiece))
				readAmount(il, pieceNames, cit->pieceAmounts);
		}
	return !confs.empty() ? confs : umap<string, Config>{ pair(Config::defaultName, Config()) };
}

void FileSys::readVictoryPoints(const char* str, Config* cfg) {
	if (string word = readWord(str); !word.empty())
		cfg->opts = stob(word) ? cfg->opts | Config::victoryPoints : cfg->opts & ~Config::victoryPoints;
	if (string word = readWord(str); !word.empty())
		cfg->victoryPointsNum = uint16(sstol(word));
	if (string word = readWord(str); !word.empty())
		cfg->opts = stob(word) ? cfg->opts | Config::victoryPointsEquidistant : cfg->opts & ~Config::victoryPointsEquidistant;
}

void FileSys::readSetPieceBattle(const char* str, Config* cfg) {
	if (string word = readWord(str); !word.empty())
		cfg->opts = stob(word) ? cfg->opts | Config::setPieceBattle : cfg->opts & ~Config::setPieceBattle;
	if (string word = readWord(str); !word.empty())
		cfg->setPieceBattleNum = uint16(sstol(word));
}

void FileSys::readFavorLimit(const char* str, Config* cfg) {
	if (string word = readWord(str); !word.empty())
		cfg->favorLimit = uint16(sstol(word));
	if (string word = readWord(str); !word.empty())
		cfg->opts = stob(word) ? cfg->opts | Config::favorTotal : cfg->opts & ~Config::favorTotal;
}

uint16 FileSys::readCapturers(const string& line) {
	uint16 capturers = 0;
	for (const char* pos = line.c_str(); *pos;)
		if (uint8 id = strToEnum<uint8>(pieceNames, readWord(pos)); id < pieceNames.size())
			capturers |= 1 << id;
	return capturers;
}

template <sizet N, sizet S>
void FileSys::readAmount(const IniLine& il, const array<const char*, N>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, il.key); id < amts.size())
		amts[id] = uint16(sstol(il.val));
}

void FileSys::saveConfigs(const umap<string, Config>& confs) {
	string text;
	for (auto& [name, cfg] : confs) {
		IniLine::write(text, name);
		IniLine::write(text, iniKeywordVictoryPoints, string(btos(cfg.opts & Config::victoryPoints)) + ' ' + toStr(cfg.victoryPointsNum) + ' ' + btos(cfg.opts & Config::victoryPointsEquidistant));
		IniLine::write(text, iniKeywordPorts, btos(cfg.opts & Config::ports));
		IniLine::write(text, iniKeywordRowBalancing, btos(cfg.opts & Config::rowBalancing));
		IniLine::write(text, iniKeywordHomefront, btos(cfg.opts & Config::homefront));
		IniLine::write(text, iniKeywordSetPieceBattle, string(btos(cfg.opts & Config::setPieceBattle)) + ' ' + toStr(cfg.setPieceBattleNum));
		IniLine::write(text, iniKeywordBoardSize, toStr(cfg.homeSize));
		IniLine::write(text, iniKeywordBattlePass, toStr(cfg.battlePass));
		IniLine::write(text, iniKeywordFavorLimit, toStr(cfg.favorLimit) + ' ' + btos(cfg.opts & Config::favorTotal));
		IniLine::write(text, iniKeywordFirstTurnEngage, btos(cfg.opts & Config::firstTurnEngage));
		IniLine::write(text, iniKeywordTerrainRules, btos(cfg.opts & Config::terrainRules));
		IniLine::write(text, iniKeywordDragonLate, btos(cfg.opts & Config::dragonLate));
		IniLine::write(text, iniKeywordDragonStraight, btos(cfg.opts & Config::dragonStraight));
		IniLine::write(text, iniKeywordWinFortress, toStr(cfg.winFortress));
		IniLine::write(text, iniKeywordWinThrone, toStr(cfg.winThrone));
		writeCapturers(text, cfg.capturers);
		writeAmounts(text, iniKeywordTile, tileNames, cfg.tileAmounts);
		writeAmounts(text, iniKeywordMiddle, tileNames, cfg.middleAmounts);
		writeAmounts(text, iniKeywordPiece, pieceNames, cfg.pieceAmounts);
		text += linend;
	}
	writeFile(configPath(fileConfigs), text);
}

void FileSys::writeCapturers(string& text, uint16 capturers) {
	string str;
	for (sizet i = 0; i < pieceLim; ++i)
		if (capturers & (1 << i))
			str += string(pieceNames[i]) + ' ';
	if (!str.empty())
		str.pop_back();
	IniLine::write(text, iniKeywordCapturers, str);
}

template <sizet N, sizet S>
void FileSys::writeAmounts(string& text, const char* word, const array<const char*, N>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); ++i)
		IniLine::write(text, word, names[i], toStr(amts[i]));
}

umap<string, Setup> FileSys::loadSetups() {
	umap<string, Setup> sets;
	umap<string, Setup>::iterator sit;
	for (IniLine il : readTextLines(readFile(configPath(fileSetups)))) {
		if (il.type == IniLine::title)
			sit = sets.emplace(std::move(il.prp), Setup()).first;
		else if (il.type == IniLine::prpKeyVal && !sets.empty()) {
			if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTile)) {
				if (TileType t = strToEnum<TileType>(tileNames, il.val); t < TileType::fortress)
					sit->second.tiles.emplace_back(stoiv<svec2>(il.key.c_str(), strtol), t);
			} else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMiddle)) {
				if (TileType t = strToEnum<TileType>(tileNames, il.val); t < TileType::fortress)
					sit->second.mids.emplace_back(uint16(sstoul(il.key)), t);
			} else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPiece))
				if (PieceType t = strToEnum<PieceType>(pieceNames, il.val); t <= PieceType::throne)
					sit->second.pieces.emplace_back(stoiv<svec2>(il.key.c_str(), strtol), t);
		}
	}
	return sets;
}

void FileSys::saveSetups(const umap<string, Setup>& sets) {
	string text;
	for (auto& [name, stp] : sets) {
		IniLine::write(text, name);
		for (auto& [pos, type] : stp.tiles)
			IniLine::write(text, iniKeywordTile, toStr(pos), tileNames[uint8(type)]);
		for (auto& [pos, type] : stp.mids)
			IniLine::write(text, iniKeywordMiddle, toStr(pos), tileNames[uint8(type)]);
		for (auto& [pos, type] : stp.pieces)
			IniLine::write(text, iniKeywordPiece, toStr(pos), pieceNames[uint8(type)]);
		text += linend;
	}
	writeFile(configPath(fileSetups), text);
}

vector<string> FileSys::listFonts() {
#if defined(__ANDROID__) || defined(EMSCRIPTEN)
	return { Settings::defaultFont, "Merriweather" };
#else
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(sstow(fontPath() + '*').c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && !(data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
			entries.push_back(cwtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(fontPath().c_str());
	if (!directory)
		return entries;

	while (dirent* entry = readdir(directory))
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
			switch (entry->d_type) {
			case DT_REG:
				entries.emplace_back(entry->d_name);
				break;
			case DT_LNK:
				if (struct stat info; !stat(entry->d_name, &info) && S_ISREG(info.st_mode))
					entries.emplace_back(entry->d_name);
			}
	closedir(directory);
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
#endif
}

umap<string, Sound> FileSys::loadAudios(const SDL_AudioSpec& spec) {
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileAudios).c_str(), defaultReadMode);
	if (!ifh) {
		std::cerr << "failed to load sounds" << std::endl;
		return {};
	}
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Sound> auds(size);

	uint8 ibuf[audioHeaderSize];
	Sound sound;
	for (uint16 i = 0; i < size; ++i) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), audioHeaderSize);
		string name;
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
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileMaterials).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load materials");

	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Material> mtls(size + 1);
	mtls.emplace();

	uint8 len;
	Material matl;
	for (uint16 i = 0; i < size; ++i) {
		SDL_RWread(ifh, &len, sizeof(len), 1);
		string name;
		name.resize(len);

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, &matl, sizeof(matl), 1);
		mtls.emplace(std::move(name), matl);
	}
	SDL_RWclose(ifh);
	return mtls;
}

umap<string, Mesh> FileSys::loadObjects() {
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileObjects).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load objects");

	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, Mesh> mshs(size + 1);
	mshs.emplace();

	uint8 ibuf[objectHeaderSize];
	for (uint16 i = 0; i < size; ++i) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), objectHeaderSize);
		string name;
		name.resize(ibuf[0]);
		vector<GLushort> elems(readMem<uint16>(ibuf + 1));
		vector<Vertex> verts(readMem<uint16>(ibuf + 3));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, elems.data(), sizeof(*elems.data()), elems.size());
		SDL_RWread(ifh, verts.data(), sizeof(*verts.data()), verts.size());
		mshs.emplace(std::move(name), Mesh(verts, elems));
	}
	SDL_RWclose(ifh);
	return mshs;
}

umap<string, string> FileSys::loadShaders() {
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileShaders).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load shaders");

	uint8 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	umap<string, string> shds(size);

	uint8 ibuf[shaderHeaderSize];
	for (uint8 i = 0; i < size; ++i) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), shaderHeaderSize);
		string name, text;
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
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileTextures).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load textures");

	scale = 100 / scale;
	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);

	uint8 ibuf[textureHeaderSize];
	for (uint16 i = 0; i < size; ++i) {
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), textureHeaderSize);
		string name;
		name.resize(ibuf[0]);
		vector<uint8> imgd(readMem<uint32>(ibuf + 1));

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, imgd.data(), sizeof(*imgd.data()), imgd.size());
		if (SDL_Surface* img = scaleSurface(IMG_Load_RW(SDL_RWFromMem(imgd.data(), int(imgd.size())), SDL_TRUE), scale))
			inset(texs, std::move(name), img, readMem<uint16>(ibuf + 5), readMem<uint16>(ibuf + 7));
		else
			std::cerr << "failed to load " << name << ": " << SDL_GetError() << std::endl;
	}
	SDL_RWclose(ifh);
}

void FileSys::reloadTextures(umap<string, Texture>& texs, int scale) {
	loadTextures(texs, [](umap<string, Texture>& txv, string&& name, SDL_Surface* img, GLint iform, GLenum pform) { txv[name].reload(img, iform, pform); }, scale);
}

vector<uint8> FileSys::loadFile(const string& file) {
	return readFile<vector<uint8>>(file);
}

template <class T>
T FileSys::readFile(const string& file) {
	T data;
	if (SDL_RWops* ifh = SDL_RWFromFile(file.c_str(), defaultReadMode)) {
		if (int64 len = SDL_RWsize(ifh); len != -1) {
			data.resize(sizet(len));
			if (sizet read = SDL_RWread(ifh, data.data(), sizeof(*data.data()), data.size()); read < data.size())
				data.resize(read);
		}
		SDL_RWclose(ifh);
	}
	return data;
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

template <class T, class F>
string FileSys::strJoin(const vector<T>& vec, F conv, char sep) {
	string str;
	for (const T& it : vec)
		str += conv(it) + sep;
	if (!str.empty())
		str.pop_back();
	return str;
}
