#include "fileSys.h"
#include "inputSys.h"
#include "server/server.h"
#include "utils/objects.h"
#ifdef __APPLE__
#include <SDL2_ttf/SDL_ttf.h>
#else
#if defined(__ANDROID__) || defined(_WIN32)
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif
#endif
#ifdef __ANDROID__
#include <jni.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
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

IniLine::Type IniLine::read(string_view str) {
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

vector<IniLine> IniLine::readLines(string_view text) {
	vector<IniLine> lines;
	constexpr char nl[] = "\n\r";
	for (sizet e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); sizet len = e - p)
			lines.emplace_back(string_view(text.data() + p, len));
	return lines;
}

void IniLine::write(SDL_RWops* ofh, string_view tit) {
	SDL_RWwrite(ofh, "[", sizeof(char), 1);
	SDL_RWwrite(ofh, tit.data(), sizeof(*tit.data()), tit.length());
	SDL_RWwrite(ofh, "]", sizeof(char), 1);
	SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());
}

void IniLine::write(SDL_RWops* ofh, string_view prp, string_view val) {
	SDL_RWwrite(ofh, prp.data(), sizeof(*prp.data()), prp.length());
	SDL_RWwrite(ofh, "=", sizeof(char), 1);
	SDL_RWwrite(ofh, val.data(), sizeof(*val.data()), val.length());
	SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());
}

void IniLine::write(SDL_RWops* ofh, string_view prp, string_view key, string_view val) {
	SDL_RWwrite(ofh, prp.data(), sizeof(*prp.data()), prp.length());
	SDL_RWwrite(ofh, "[", sizeof(char), 1);
	SDL_RWwrite(ofh, key.data(), sizeof(*key.data()), key.length());
	SDL_RWwrite(ofh, "]=", sizeof(char), 2);
	SDL_RWwrite(ofh, val.data(), sizeof(*val.data()), val.length());
	SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());
}

// FILE SYS

void FileSys::init(const Arguments& args) {
#ifdef __ANDROID__
	if (const char* path = SDL_AndroidGetExternalStoragePath())
		dirConfig = path + "/"s;
#elif defined(__EMSCRIPTEN__)
	dirBase = "/";
	dirConfig = dirBase;
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
	if (const char* eopt = args.getOpt(Settings::argExternal); !eopt || toBool(eopt)) {
#else
	if (const char* eopt = args.getOpt(Settings::argExternal); eopt && toBool(eopt)) {
#endif
#ifdef _WIN32
		if (const wchar* path = _wgetenv(L"AppData")) {
			dirConfig = swtos(path) + "/Thrones/";
			std::replace(dirConfig.begin(), dirConfig.end(), '\\', '/');
		}
#elif defined(__APPLE__)
		if (const char* path = getenv("HOME"))
			dirConfig = path + "/Library/Preferences/Thrones/"s;
#else
		if (const char* path = getenv("HOME"))
			dirConfig = path + "/.config/thrones/"s;
#endif
	} else
		dirConfig = dataPath();

	if (args.hasFlag(Settings::argLog)) {
		if (string logDir = dirConfig + "logs/"; createDirectories(logDir)) {
			if (logFile = SDL_RWFromFile((logDir + "thrones_" + DateTime::now().toString() + ".txt").c_str(), defaultWriteMode); logFile) {
				SDL_LogSetOutputFunction(logWrite, logFile);
				logInfo("start v", Com::commonVersion);
			} else
				logError("failed to create log file: ", SDL_GetError());
		} else
			logError("failed to create log directory");
	}
#endif
}

void FileSys::close() {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	if (logFile) {
		logInfo("close");
		SDL_RWclose(logFile);
	}
#endif
}

Settings FileSys::loadSettings(InputSys* input) {
	Settings sets;
	input->clearBindings();
	void (*reader)(void*, IniLine&) = readSetting;
	void* data = &sets;
	for (IniLine& il : IniLine::readLines(loadFile(dirConfig + fileSettings))) {
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
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDisplay))
		sets.display = std::min(toNum<uint8>(il.val), uint8(SDL_GetNumVideoDisplays()));
	else
#endif
#ifndef __ANDROID__
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordScreen))
		sets.screen = strToEnum(Settings::screenNames, il.val, Settings::defaultScreen);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSize))
		sets.size = toVec<ivec2, uint>(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMode))
		sets.mode = strToDisp(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVersionLookup))
		readVersionLookup(il.val.c_str(), sets);
	else
#endif
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMsamples))
		sets.antiAliasing = strToEnum(Settings::antiAliasingNames, il.val, Settings::AntiAliasing::none);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAnisotropy))
		sets.anisotropy = strToEnum(Settings::anisotropyNames, il.val, Settings::Anisotropy::none);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordShadows))
		readShadows(il.val.c_str(), sets);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSsao))
		sets.ssao = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordBloom))
		sets.bloom = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSsr))
		sets.ssr = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVsync))
		sets.vsync = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTexScale))
		sets.texScale = std::clamp(toNum<uint8>(il.val), 1_ub, 100_ub);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordGamma))
		sets.gamma = std::clamp(toNum<float>(il.val), 0.f, Settings::gammaMax);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFov))
		sets.fov = std::clamp(toNum<double>(il.val), Settings::fovLimit.x, Settings::fovLimit.y);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAVolume))
		sets.avolume = std::min(toNum<uint8>(il.val), uint8(SDL_MIX_MAXVOLUME));
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordColors))
		readColors(il.val.c_str(), sets);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordScales))
		readScales(il.val.c_str(), sets);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAutoVictoryPoints))
		sets.autoVictoryPoints = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTooltips))
		sets.tooltips = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordChatLines))
		sets.chatLines = std::min(toNum<uint16>(il.val), Settings::chatLinesMax);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDeadzone))
		sets.deadzone = std::clamp(toNum<uint16>(il.val), Settings::deadzoneLimit.x, Settings::deadzoneLimit.y);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordResolveFamily))
		sets.resolveFamily = strToEnum(Settings::familyNames, il.val, Settings::defaultFamily);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFont))
		sets.font = il.val;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordHinting))
		sets.hinting = strToEnum(Settings::hintingNames, il.val, Settings::defaultHinting);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordInvertWheel))
		sets.invertWheel = toBool(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordAddress))
		sets.address = std::move(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPort))
		sets.port = std::move(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPlayerName))
		sets.playerName = std::move(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordLastConfig))
		sets.lastConfig = std::move(il.val);
}

void FileSys::readShadows(const char* str, Settings& sets) {
	if (string_view word = readWord(str); !word.empty())
		sets.shadowRes = std::min(toNum<uint16>(word), uint16(1 << Settings::shadowBitMax));
	if (string_view word = readWord(str); !word.empty())
		sets.softShadows = toBool(word);
}

void FileSys::readColors(const char* str, Settings& sets) {
	sets.colorAlly = strToEnum(Settings::colorNames, readWord(str), Settings::defaultAlly);
	sets.colorEnemy = strToEnum(Settings::colorNames, readWord(str), Settings::defaultEnemy);
}

void FileSys::readScales(const char* str, Settings& sets) {
	if (string_view word = readWord(str); !word.empty())
		sets.scaleTiles = toBool(word);
	if (string_view word = readWord(str); !word.empty())
		sets.scalePieces = toBool(word);
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
				input->addBinding(bid, JoystickButton(readNumber<uint8>(pos)));
				break;
			case iniVkeyHat:
				if (uint8 id = readNumber<uint8>(pos), val = strToVal<uint8>(InputSys::hatNames, readWord(++pos), SDL_HAT_CENTERED); val != SDL_HAT_CENTERED)
					input->addBinding(bid, AsgJoystick(id, val));
				break;
			case iniVkeyAxis:
				if (char sign = *pos++; sign == '+' || sign == '-')
					input->addBinding(bid, AsgJoystick(JoystickAxis(readNumber<uint8>(pos)), sign == '+'));
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
	SDL_RWops* ofh = startWriteUserFile(dirConfig, fileSettings);
	if (!ofh)
		return;

	IniLine::write(ofh, iniTitleSettings);
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	IniLine::write(ofh, iniKeywordDisplay, toStr(sets.display));
#endif
#ifndef __ANDROID__
	IniLine::write(ofh, iniKeywordScreen, Settings::screenNames[uint8(sets.screen)]);
	IniLine::write(ofh, iniKeywordSize, toStr(sets.size));
	IniLine::write(ofh, iniKeywordMode, toStr(sets.mode.w) + ' ' + toStr(sets.mode.h) + ' ' + toStr(sets.mode.refresh_rate) + ' ' + toStr(sets.mode.format));
	IniLine::write(ofh, iniKeywordVersionLookup, strEnclose(sets.versionLookupUrl) + ' ' + strEnclose(sets.versionLookupRegex));
#endif
	IniLine::write(ofh, iniKeywordMsamples, Settings::antiAliasingNames[uint8(sets.antiAliasing)]);
	IniLine::write(ofh, iniKeywordAnisotropy, Settings::anisotropyNames[uint8(sets.anisotropy)]);
	IniLine::write(ofh, iniKeywordShadows, toStr(sets.shadowRes) + ' ' + toStr(sets.softShadows));
	IniLine::write(ofh, iniKeywordSsao, toStr(sets.ssao));
	IniLine::write(ofh, iniKeywordBloom, toStr(sets.bloom));
	IniLine::write(ofh, iniKeywordSsr, toStr(sets.ssr));
	IniLine::write(ofh, iniKeywordVsync, toStr(sets.vsync));
	IniLine::write(ofh, iniKeywordTexScale, toStr(sets.texScale));
	IniLine::write(ofh, iniKeywordGamma, toStr(sets.gamma));
	IniLine::write(ofh, iniKeywordFov, toStr(sets.fov));
	IniLine::write(ofh, iniKeywordAVolume, toStr(sets.avolume));
	IniLine::write(ofh, iniKeywordColors, Settings::colorNames[uint8(sets.colorAlly)] + " "s + Settings::colorNames[uint8(sets.colorEnemy)]);
	IniLine::write(ofh, iniKeywordScales, toStr(sets.scaleTiles) + " "s + toStr(sets.scalePieces));
	IniLine::write(ofh, iniKeywordAutoVictoryPoints, toStr(sets.autoVictoryPoints));
	IniLine::write(ofh, iniKeywordTooltips, toStr(sets.tooltips));
	IniLine::write(ofh, iniKeywordChatLines, toStr(sets.chatLines));
	IniLine::write(ofh, iniKeywordDeadzone, toStr(sets.deadzone));
	IniLine::write(ofh, iniKeywordResolveFamily, Settings::familyNames[uint8(sets.resolveFamily)]);
	IniLine::write(ofh, iniKeywordFont, sets.font);
	IniLine::write(ofh, iniKeywordHinting, Settings::hintingNames[uint8(sets.hinting)]);
	IniLine::write(ofh, iniKeywordInvertWheel, toStr(sets.invertWheel));
	IniLine::write(ofh, iniKeywordAddress, sets.address);
	IniLine::write(ofh, iniKeywordPort, sets.port);
	IniLine::write(ofh, iniKeywordPlayerName, sets.playerName);
	IniLine::write(ofh, iniKeywordLastConfig, sets.lastConfig);
	SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());

	IniLine::write(ofh, iniTitleBindings);
	for (uint8 i = 0; i < Binding::names.size(); ++i) {
		if (!input->getBinding(Binding::Type(i)).keys.empty())
			IniLine::write(ofh, Binding::names[i], iniKeyKey, strJoin(input->getBinding(Binding::Type(i)).keys, [](SDL_Scancode sc) -> string { return strEnclose(SDL_GetScancodeName(sc)); }));
		if (!input->getBinding(Binding::Type(i)).joys.empty())
			IniLine::write(ofh, Binding::names[i], iniKeyJoy, strJoin(input->getBinding(Binding::Type(i)).joys, [](AsgJoystick aj) -> string {
				switch (aj.getAsg()) {
				case AsgJoystick::button:
					return iniVkeyButton + toStr(aj.getJct());
				case AsgJoystick::hat:
					return iniVkeyHat + toStr(aj.getJct()) + '_' + InputSys::hatNames.at(aj.getHvl());
				case AsgJoystick::axisPos:
					return iniVkeyAxis + "+"s + toStr(aj.getJct());
				case AsgJoystick::axisNeg:
					return iniVkeyAxis + "-"s + toStr(aj.getJct());
				}
				return string();
			}));
		if (!input->getBinding(Binding::Type(i)).gpds.empty())
			IniLine::write(ofh, Binding::names[i], iniKeyGpd, strJoin(input->getBinding(Binding::Type(i)).gpds, [](AsgGamepad ag) -> string {
				switch (ag.getAsg()) {
				case AsgGamepad::button:
					return iniVkeyButton + string(InputSys::gbuttonNames[ag.getButton()]);
				case AsgGamepad::axisPos:
					return iniVkeyAxis + "+"s + InputSys::gaxisNames[ag.getAxis()];
				case AsgGamepad::axisNeg:
					return iniVkeyAxis + "-"s + InputSys::gaxisNames[ag.getAxis()];
				}
				return string();
			}));
	}
	endWriteUserFile(ofh);
}

umap<string, Config> FileSys::loadConfigs() {
	umap<string, Config> confs;
	Config* cit = nullptr;
	for (IniLine& il : IniLine::readLines(loadFile(dirConfig + fileConfigs)))
		switch (il.type) {
		case IniLine::title:
			if (il.prp.length() > Config::maxNameLength)
				il.prp.resize(Config::maxNameLength);
			cit = &confs.emplace(std::move(il.prp), Config()).first->second;
			break;
		case IniLine::prpVal:
			if (cit)
				readConfigPrpVal(il, *cit);
			break;
		case IniLine::prpKeyVal:
			if (cit)
				readConfigPrpKeyVal(il, *cit);
		}
	return !confs.empty() ? confs : umap<string, Config>{ pair(Settings::defaultConfigName, Config()) };
}

void FileSys::readConfigPrpVal(const IniLine& il, Config& cfg) {
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordVictoryPoints))
		readVictoryPoints(il.val.c_str(), cfg);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPorts))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::ports : cfg.opts & ~Config::ports;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordRowBalancing))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::rowBalancing : cfg.opts & ~Config::rowBalancing;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordHomefront))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::homefront : cfg.opts & ~Config::homefront;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordSetPieceBattle))
		readSetPieceBattle(il.val.c_str(), cfg);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordBoardSize))
		cfg.homeSize = toVec<svec2, int>(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordBattlePass))
		cfg.battlePass = std::min(toNum<uint8>(il.val), Config::randomLimit);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFavorLimit))
		readFavorLimit(il.val.c_str(), cfg);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordFirstTurnEngage))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::firstTurnEngage : cfg.opts & ~Config::firstTurnEngage;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTerrainRules))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::terrainRules : cfg.opts & ~Config::terrainRules;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDragonLate))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::dragonLate : cfg.opts & ~Config::dragonLate;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordDragonStraight))
		cfg.opts = toBool(il.val) ? cfg.opts | Config::dragonStraight : cfg.opts & ~Config::dragonStraight;
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordWinFortress))
		cfg.winFortress = toNum<int>(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordWinThrone))
		cfg.winThrone = toNum<int>(il.val);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordCapturers))
		cfg.capturers = readCapturers(il.val.c_str());
}

void FileSys::readConfigPrpKeyVal(const IniLine& il, Config& cfg) {
	if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTile))
		readAmount(il, tileNames, cfg.tileAmounts);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMiddle))
		readAmount(il, tileNames, cfg.middleAmounts);
	else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPiece))
		readAmount(il, pieceNames, cfg.pieceAmounts);
}

void FileSys::readVictoryPoints(const char* str, Config& cfg) {
	if (string_view word = readWord(str); !word.empty())
		cfg.opts = toBool(word) ? cfg.opts | Config::victoryPoints : cfg.opts & ~Config::victoryPoints;
	if (string_view word = readWord(str); !word.empty())
		cfg.victoryPointsNum = toNum<int>(word);
	if (string_view word = readWord(str); !word.empty())
		cfg.opts = toBool(word) ? cfg.opts | Config::victoryPointsEquidistant : cfg.opts & ~Config::victoryPointsEquidistant;
}

void FileSys::readSetPieceBattle(const char* str, Config& cfg) {
	if (string_view word = readWord(str); !word.empty())
		cfg.opts = toBool(word) ? cfg.opts | Config::setPieceBattle : cfg.opts & ~Config::setPieceBattle;
	if (string_view word = readWord(str); !word.empty())
		cfg.setPieceBattleNum = toNum<uint16>(word);
}

void FileSys::readFavorLimit(const char* str, Config& cfg) {
	if (string_view word = readWord(str); !word.empty())
		cfg.favorLimit = toNum<int8>(word);
	if (string_view word = readWord(str); !word.empty())
		cfg.opts = toBool(word) ? cfg.opts | Config::favorTotal : cfg.opts & ~Config::favorTotal;
}

uint16 FileSys::readCapturers(const char* str) {
	uint16 capturers = 0;
	while (*str)
		if (uint8 id = strToEnum<uint8>(pieceNames, readWord(str)); id < pieceNames.size())
			capturers |= 1 << id;
	return capturers;
}

void FileSys::saveConfigs(const umap<string, Config>& confs) {
	if (SDL_RWops* ofh = startWriteUserFile(dirConfig, fileConfigs)) {
		for (auto& [name, cfg] : confs) {
			IniLine::write(ofh, name);
			writeConfig(ofh, cfg);
		}
		endWriteUserFile(ofh);
	}
}

void FileSys::writeConfig(SDL_RWops* ofh, const Config& cfg) {
	IniLine::write(ofh, iniKeywordVictoryPoints, toStr(bool(cfg.opts & Config::victoryPoints)) + " "s + toStr(cfg.victoryPointsNum) + ' ' + toStr(bool(cfg.opts & Config::victoryPointsEquidistant)));
	IniLine::write(ofh, iniKeywordPorts, toStr(bool(cfg.opts & Config::ports)));
	IniLine::write(ofh, iniKeywordRowBalancing, toStr(bool(cfg.opts & Config::rowBalancing)));
	IniLine::write(ofh, iniKeywordHomefront, toStr(bool(cfg.opts & Config::homefront)));
	IniLine::write(ofh, iniKeywordSetPieceBattle, toStr(bool(cfg.opts & Config::setPieceBattle)) + " "s + toStr(cfg.setPieceBattleNum));
	IniLine::write(ofh, iniKeywordBoardSize, toStr(cfg.homeSize));
	IniLine::write(ofh, iniKeywordBattlePass, toStr(cfg.battlePass));
	IniLine::write(ofh, iniKeywordFavorLimit, toStr(cfg.favorLimit) + ' ' + toStr(bool(cfg.opts & Config::favorTotal)));
	IniLine::write(ofh, iniKeywordFirstTurnEngage, toStr(bool(cfg.opts & Config::firstTurnEngage)));
	IniLine::write(ofh, iniKeywordTerrainRules, toStr(bool(cfg.opts & Config::terrainRules)));
	IniLine::write(ofh, iniKeywordDragonLate, toStr(bool(cfg.opts & Config::dragonLate)));
	IniLine::write(ofh, iniKeywordDragonStraight, toStr(bool(cfg.opts & Config::dragonStraight)));
	IniLine::write(ofh, iniKeywordWinFortress, toStr(cfg.winFortress));
	IniLine::write(ofh, iniKeywordWinThrone, toStr(cfg.winThrone));
	writeCapturers(ofh, cfg.capturers);
	writeAmounts(ofh, iniKeywordTile, tileNames, cfg.tileAmounts);
	writeAmounts(ofh, iniKeywordMiddle, tileNames, cfg.middleAmounts);
	writeAmounts(ofh, iniKeywordPiece, pieceNames, cfg.pieceAmounts);
	SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());
}

void FileSys::writeCapturers(SDL_RWops* ofh, uint16 capturers) {
	string str;
	for (sizet i = 0; i < pieceLim; ++i)
		if (capturers & (1 << i))
			str += pieceNames[i] + " "s;
	if (!str.empty())
		str.pop_back();
	IniLine::write(ofh, iniKeywordCapturers, str);
}

umap<string, Setup> FileSys::loadSetups() {
	umap<string, Setup> sets;
	umap<string, Setup>::iterator sit;
	for (const IniLine& il : IniLine::readLines(loadFile(dirConfig + fileSetups))) {
		if (il.type == IniLine::title)
			sit = sets.emplace(std::move(il.prp), Setup()).first;
		else if (il.type == IniLine::prpKeyVal && !sets.empty()) {
			if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordTile)) {
				if (TileType t = strToEnum<TileType>(tileNames, il.val); t < TileType::fortress)
					sit->second.tiles.emplace_back(toVec<svec2>(il.key), t);
			} else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordMiddle)) {
				if (TileType t = strToEnum<TileType>(tileNames, il.val); t < TileType::fortress)
					sit->second.mids.emplace_back(toNum<uint16>(il.key), t);
			} else if (!SDL_strcasecmp(il.prp.c_str(), iniKeywordPiece))
				if (PieceType t = strToEnum<PieceType>(pieceNames, il.val); t <= PieceType::throne)
					sit->second.pieces.emplace_back(toVec<svec2>(il.key), t);
		}
	}
	return sets;
}

void FileSys::saveSetups(const umap<string, Setup>& sets) {
	if (SDL_RWops* ofh = startWriteUserFile(dirConfig, fileSetups)) {
		for (auto& [name, stp] : sets) {
			IniLine::write(ofh, name);
			for (auto& [pos, type] : stp.tiles)
				IniLine::write(ofh, iniKeywordTile, toStr(pos), tileNames[uint8(type)]);
			for (auto& [pos, type] : stp.mids)
				IniLine::write(ofh, iniKeywordMiddle, toStr(pos), tileNames[uint8(type)]);
			for (auto& [pos, type] : stp.pieces)
				IniLine::write(ofh, iniKeywordPiece, toStr(pos), pieceNames[uint8(type)]);
			SDL_RWwrite(ofh, linend.data(), sizeof(*linend.data()), linend.length());
		}
		endWriteUserFile(ofh);
	}
}

vector<string> FileSys::listFonts() {
	string dirPath = fontPath();
	vector<string> entries;
	listOperateDirectory(dirPath, [&dirPath, &entries](string&& name) {
		if (TTF_Font* font = TTF_OpenFont((dirPath + name).c_str(), fontTestHeight)) {
			TTF_CloseFont(font);
			entries.push_back(std::move(name));
		}
	});
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}

umap<string, Sound> FileSys::loadAudios(const SDL_AudioSpec& spec) {
	string dirPath = dataPath() + "audio/";
	umap<string, Sound> audios;
	listOperateDirectory(dirPath, [&dirPath, &audios, spec](string&& name) {
		Sound sound;
		if (SDL_AudioSpec aspec; SDL_LoadWAV((dirPath + name).c_str(), &aspec, &sound.data, &sound.length) && sound.convert(aspec, spec))
			audios.emplace(delExt(name), sound);
		else {
			sound.free();
			logError("failed to load audio ", name);
		}
	});
	return audios;
}

vector<Material> FileSys::loadMaterials(umap<string, uint16>& refs) {
	SDL_RWops* ifh = SDL_RWFromFile((dataPath() + fileMaterials).c_str(), defaultReadMode);
	if (!ifh)
		throw std::runtime_error("failed to load materials");

	uint16 size;
	SDL_RWread(ifh, &size, sizeof(size), 1);
	refs.reserve(size + 1);
	refs.emplace(string(), 0);
	vector<Material> mtls(size + 1);

	uint16 len;
	Material matl;
	for (uint16 i = 0; i < size; ++i) {
		SDL_RWread(ifh, &len, sizeof(len), 1);
		string name;
		name.resize(len);

		SDL_RWread(ifh, name.data(), sizeof(*name.data()), name.length());
		SDL_RWread(ifh, &mtls[i + 1], sizeof(*mtls.data()), 1);
		refs.emplace(std::move(name), i + 1);
	}
	SDL_RWclose(ifh);
	return mtls;
}

vector<Mesh> FileSys::loadObjects(umap<string, uint16>& refs) {
	string dirPath = dataPath() + "objects/";
	vector<tuple<string, uint, uint>> names;
	listOperateDirectory(dirPath, [&names](string&& name) {
		string::iterator num = fileLevelPos(name);
		uint len = num - name.begin();
		uint val = num != name.begin() && num != name.end() ? toNum<uint>(&*num) : UINT_MAX;
		names.emplace_back(std::move(name), len, val);
	});
	std::sort(names.begin(), names.end(), [](const tuple<string, uint, uint>& a, const tuple<string, uint, uint>& b) -> int { return std::get<2>(a) < std::get<2>(b); });

	vector<Mesh> mshs(names.size() + 1);
	refs.reserve(mshs.size());
	sizet m = 0;
	for (const auto& [name, num, level] : names) {
		SDL_RWops* ifh = SDL_RWFromFile((dirPath + name).c_str(), defaultReadMode);
		if (!ifh) {
			logError("failed to load object ", name);
			continue;
		}
		uint16 ibuf[2];
		SDL_RWread(ifh, ibuf, sizeof(*ibuf), 2);

		vector<GLushort> elems(ibuf[0]);
		vector<Vertex> verts(ibuf[1]);
		SDL_RWread(ifh, elems.data(), sizeof(*elems.data()), elems.size());
		SDL_RWread(ifh, verts.data(), sizeof(*verts.data()), verts.size());
		SDL_RWclose(ifh);

		mshs[m].init(verts, elems);
		refs.emplace(name.substr(0, num), m);
		++m;
	}
	mshs.resize(m);
	return mshs;
}

umap<string, string> FileSys::loadShaders() {
	string dirPath = dataPath() + "shaders/";
	umap<string, string> shds;
	listOperateDirectory(dirPath, [&dirPath, &shds](string&& name) {
		if (string txt = loadFile(dirPath + name); !txt.empty())
			shds.emplace(std::move(name), std::move(txt));
		else
			logError("failed to load shader ", name);
	});
	return shds;
}

TextureSet::Import FileSys::loadTextures(vector<TextureCol::Element>& icns, int& minUiIcon, float scale) {
	string dirPath = texturePath();
	TextureSet::Import imp;
	vector<pair<string, string>> svgUiFiles;
	listOperateDirectory(dirPath, [&dirPath, &imp, &icns, &svgUiFiles, &minUiIcon, scale](string&& name) {
		if (auto [place, num, path] = beginTextureLoad(dirPath, name, false); place != Texplace::none) {
			if (place != Texplace::cube) {
				if (!strciEndsWith(path, ".svg")) {
					if (auto [img, pform] = pickPixFormat(IMG_Load(path.c_str())); img) {
						if (place & Texplace::object)
							loadObjectTexture(img, dirPath, name, num, pform, imp, scale, place == Texplace::object);
						if (place & Texplace::widget) {
							minUiIcon = std::min(minUiIcon, img->h);
							icns.emplace_back(name.substr(0, num), img, pform);
						}
					} else
						logError("failed to load texture ", path);
				} else {
					if (place & Texplace::object) {
						if (auto [img, pform] = pickPixFormat(IMG_Load(path.c_str())); img)
							loadObjectTexture(img, dirPath, name, num, pform, imp, scale);
						else
							logError("failed to load texture ", path);
					}
					if (place & Texplace::widget)
						svgUiFiles.emplace_back(std::move(path), name.substr(0, num));
				}
			} else
				loadSkyTextures(imp, path, dirPath.length(), num);
		}
	});
	for (pair<string, string>& it : svgUiFiles) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		if (SDL_RWops* fh = SDL_RWFromFile(it.first.c_str(), defaultReadMode)) {
			if (auto [img, pform] = pickPixFormat(IMG_LoadSizedSVG_RW(fh, minUiIcon, minUiIcon)); img)
				icns.emplace_back(std::move(it.second), img, pform);
			else
				logError("failed to load texture ", it.first);
			SDL_RWclose(fh);
		} else
			logError("failed to open texture ", it.first);
#else
		if (auto [img, pform] = pickPixFormat(IMG_Load(it.first.c_str())); img)
			icns.emplace_back(std::move(it.second), scaleSurfaceTry(img, ivec2(minUiIcon)), pform);
		else
			logError("failed to load texture ", it.first);
#endif
	}
	return imp;
}

TextureSet::Import FileSys::reloadTextures(float scale) {
	string dirPath = texturePath();
	TextureSet::Import imp;
	listOperateDirectory(dirPath, [&dirPath, &imp, scale](string&& name) {
		if (auto [place, num, path] = beginTextureLoad(dirPath, name, true); place != Texplace::none) {
			if (place != Texplace::cube) {
				if (auto [img, pform] = pickPixFormat(IMG_Load(path.c_str())); img)
					loadObjectTexture(img, dirPath, name, num, pform, imp, scale);
				else
					logError("failed to load texture ", path);
			} else
				loadSkyTextures(imp, path, dirPath.length(), num);
		}
	});
	return imp;
}

tuple<FileSys::Texplace, sizet, string> FileSys::beginTextureLoad(string_view dirPath, string& name, bool noWidget) {
	string::iterator num = fileLevelPos(name);
	if (num == name.begin() || num == name.end())
		return tuple(Texplace::none, string::npos, string());
	Texplace place = Texplace(*num - '0');
	if ((noWidget && place == Texplace::widget) || place > Texplace::cube || (place == Texplace::cube && *(num + 1) != '0'))
		return tuple(Texplace::none, string::npos, string());
	return tuple(place, num - name.begin(), dirPath + name);
}

void FileSys::loadObjectTexture(SDL_Surface* img, string_view dirPath, string& name, sizet num, GLenum ifmt, TextureSet::Import& imp, float scale, bool unique) {
	name[num] = 'N';
	string path = dirPath + name;
	auto [nrm, nfmt] = pickPixFormat(IMG_Load(path.c_str()));
	if (nrm)
		imp.nres = std::max(imp.nres, int(float(std::max(nrm->w, nrm->h)) * scale));
	imp.cres = std::max(imp.cres, int(float(std::max(img->w, img->h)) * scale));
	imp.imgs.emplace_back(img, nrm, name.substr(0, num), ifmt, nfmt, unique);
}

void FileSys::loadSkyTextures(TextureSet::Import& imp, string& path, sizet dirPathLen, sizet num) {
	sizet ofs = num + 1 + dirPathLen;
	for (sizet s = 0; s < imp.sky.size(); ++s) {
		path[ofs] = '0' + s;
		if (imp.sky[s] = pickPixFormat(IMG_Load(path.c_str())); !imp.sky[s].first) {
			std::for_each(imp.sky.begin(), imp.sky.begin() + s, [](pair<SDL_Surface*, GLenum>& it) { SDL_FreeSurface(it.first); });
			logError("failed to load cubemap side ", path);
			break;
		}
	}
}

template <class F>
void FileSys::listOperateDirectory(string_view dirPath, F func) {
#ifdef __ANDROID__
	JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
	jclass clazz(env->GetObjectClass(activity));
	jmethodID methodId = env->GetMethodID(clazz, "listAssets", "(Ljava/lang/String;)[Ljava/lang/String;");

	jstring path = env->NewStringUTF(string(dirPath).c_str());
	jobjectArray files = static_cast<jobjectArray>(env->CallObjectMethod(activity, methodId, path));
	for (jsize i = 0, size = env->GetArrayLength(files); i < size; ++i) {
		jstring name = static_cast<jstring>(env->GetObjectArrayElement(files, i));
		const char* ncvec = env->GetStringUTFChars(name, 0);
		func(ncvec);
		env->ReleaseStringUTFChars(name, ncvec);
		env->DeleteLocalRef(name);
	}
	env->DeleteLocalRef(files);
	env->DeleteLocalRef(path);
	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);
#elif defined(_WIN32)
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(dirPath) + L'*').c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				func(swtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(string(dirPath).c_str())) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && (entry->d_type == DT_REG || entry->d_type == DT_LNK))
				func(entry->d_name);
		closedir(directory);
	}
#endif
}

#ifndef __ANDROID__
vector<string> FileSys::listFiles(string_view dirPath) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(dirPath) + L'*').c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				entries.push_back(swtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (string dirc(dirPath); DIR* directory = opendir(dirc.c_str())) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
				if (entry->d_type == DT_REG)
					entries.emplace_back(entry->d_name);
				else if (entry->d_type == DT_LNK)
					if (struct stat ps; !stat((dirc + entry->d_name).c_str(), &ps) && (ps.st_mode & S_IFMT) == S_IFREG)
						entries.emplace_back(entry->d_name);
			}
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}
#endif

SDL_RWops* FileSys::startWriteUserFile(const string& drc, const char* file) {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	if (!createDirectories(drc)) {
		logError("failed to create directory ", drc);
		return nullptr;
	}
#endif

	string path = drc + file;
	SDL_RWops* ofh = SDL_RWFromFile(path.c_str(), defaultWriteMode);
	if (!ofh)
		logError("failed to write ", path);
	return ofh;
}

void FileSys::endWriteUserFile(SDL_RWops* ofh) {
	SDL_RWclose(ofh);
#ifdef __EMSCRIPTEN__
	EM_ASM(
		Module.syncdone = 0;
		FS.syncfs(function(err) {
			Module.syncdone = 1;
		});
	);
#endif
}

#ifdef __EMSCRIPTEN__
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

string::iterator FileSys::fileLevelPos(string& str) {
	string::reverse_iterator dot = std::find(str.rbegin(), str.rend(), '.');
	return std::find_if_not(dot + (dot != str.rend()), str.rend(), isdigit).base();
}

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
void FileSys::saveScreenshot(SDL_Surface* img, string_view desc) {
	if (img) {
		if (string path = dirConfig + "screenshots/"; createDirectories(path)) {
			if (IMG_SavePNG(img, (path + "thrones_" + (desc.empty() ? DateTime::now().toString() : string(desc)) + ".png").c_str()))
				logError("failed to save screenshot: ", IMG_GetError());
		} else
			logError("failed to create directory ", path);
		SDL_FreeSurface(img);
	}
}

void SDLCALL FileSys::logWrite(void* userdata, int, SDL_LogPriority priority, const char* message) {
	string prefix = DateTime::now().timeString() + ' ';
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
		prefix += "VERBOSE: ";
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		prefix += "DEBUG: ";
		break;
	case SDL_LOG_PRIORITY_INFO:
		prefix += "INFO: ";
		break;
	case SDL_LOG_PRIORITY_WARN:
		prefix += "WARN: ";
		break;
	case SDL_LOG_PRIORITY_ERROR:
		prefix += "ERROR: ";
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		prefix += "CRITICAL: ";
	}

	SDL_RWops* file = static_cast<SDL_RWops*>(userdata);
	SDL_RWwrite(file, prefix.c_str(), sizeof(*prefix.c_str()), prefix.length());
	SDL_RWwrite(file, message, sizeof(*message), strlen(message));
	SDL_RWwrite(file, linend.data(), sizeof(*linend.data()), linend.length());
}
#endif
