#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(const string& str, int height, int margin) :
	text(str),
	length(strLength(str, height, margin)),
	height(height)
{}

int ProgState::Text::strLength(const string& str, int height, int margin) {
	return World::winSys()->textLength(str, height) + margin * 2;
}

int ProgState::findMaxLength(const vector<vector<string>*>& lists, int height, int margin) {
	int width = 0;
	for (vector<string>* it : lists)
		if (int len = findMaxLength(it->begin(), it->end(), height, margin); len > width)
			width = len;
	return width;
}

// PROGRAM STATE

void ProgState::eventEnter() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
}

bool ProgState::tryClosePopup() {
	if (World::scene()->getPopup()) {
		World::program()->eventClosePopup();
		return true;
	}
	return false;
}

Layout* ProgState::createLayout() {
	return nullptr;
}

Popup* ProgState::createPopupMessage(const string& msg, BCall ccal, const string& ctxt) {
	Text ok(ctxt, superHeight);
	Text ms(msg, superHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, ok.text, ccal, nullptr, nullptr, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, false, 0)
	};
	return new Popup(vec2s(ms.length, superHeight * 2 + Layout::defaultItemSpacing), con);
}

Popup* ProgState::createPopupChoice(const string& msg, BCall kcal, BCall ccal) {
	Text ms(msg, superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, nullptr, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, nullptr, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, false, 0)
	};
	return new Popup(vec2s(ms.length, superHeight * 2 + Layout::defaultItemSpacing), con);
}

// PROG MENU

void ProgMenu::eventEscape() {
	if (!tryClosePopup())
		World::winSys()->close();
}

Layout* ProgMenu::createLayout() {
	// server input
	Text srvt("Server:", superHeight);
	vector<Widget*> srv = {
		new Label(srvt.length, srvt.text),
		new LabelEdit(1.f, World::sets()->address, &Program::eventUpdateAddress, nullptr, &Program::eventConnectServer)
	};

	// port input and connect button
	vector<Widget*> prt = {
		new Label(srvt.length, "Port:"),
		new LabelEdit(1.f, to_string(World::sets()->port), &Program::eventUpdatePort, nullptr, &Program::eventConnectServer, LabelEdit::TextType::uInt)
	};

	// middle buttons
	vector<Widget*> buts = {
		new Widget(),
		new Layout(superHeight, srv, false),
		new Layout(superHeight, prt, false),
		new Label(superHeight, "Connect", &Program::eventConnectServer, nullptr, nullptr, Label::Alignment::center),
		new Widget(0),
		new Label(superHeight, "Settings", &Program::eventOpenSettings, nullptr, nullptr, Label::Alignment::center),
		new Label(superHeight, "Exit", &Program::eventExit, nullptr, nullptr, Label::Alignment::center),
		new Widget()
	};

	// root layout
	vector<Widget*> cont = {
		new Widget(),
		new Layout(World::winSys()->textLength(srvt.text + "999.999.999.999", superHeight) + Layout::defaultItemSpacing + Label::defaultTextMargin * 4, buts, true, false, superSpacing),
		new Widget()
	};
	return new Layout(1.f, cont, false, false, 0);
}

// PROG SETUP

ProgSetup::ProgSetup() :
	stage(Stage::tiles),
	enemyReady(false),
	midCnt({1, 1, 1, 1})
{
	memcpy(tileCnt.data(), Tile::amounts.data(), tileCnt.size());
	memcpy(pieceCnt.data(), Piece::amounts.data(), pieceCnt.size());

	setTicons();
	setMicons();
	setPicons();
}

void ProgSetup::eventEscape() {
	if (!tryClosePopup())
		stage < Stage::ready ? World::program()->eventSetupBack() : World::scene()->setPopup(createPopupChoice("Exit game?", &Program::eventExitGame, &Program::eventShowWaitPopup));
}

Layout* ProgSetup::createLayout() {
	// center piece
	vector<Widget*> midl = {
		new Widget(),
		message = new Label(superHeight, emptyStr, nullptr, nullptr, nullptr, Label::Alignment::center, nullptr, false, 0),
		new Widget(),
		stage == Stage::tiles ? ticons : stage == Stage::middles ? micons : picons
	};

	// root layout
	int sideLength;
	vector<Widget*> cont = {
		createSidebar(sideLength),
		new Layout(1.f, midl, true, false, 0),
		new Widget(sideLength)
	};
	return new Layout(1.f, cont, false, false, 0);
}

void ProgSetup::setTicons() {
	vector<Widget*> tbot = {
		new Widget(),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::plains)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::forest)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::mountain)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::water)]),
		new Widget()
	};
	ticons = new Layout(iconSize, tbot);
}

void ProgSetup::setMicons() {
	vector<Widget*> mbot = {
		new Widget(),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::plains)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::forest)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::mountain)]),
		new Draglet(iconSize, &Program::eventPlaceTile, nullptr, nullptr, Tile::colors[uint8(Tile::Type::water)]),
		new Widget()
	};
	micons = new Layout(iconSize, mbot);
}

void ProgSetup::setPicons() {
	vector<Widget*> pbot = {
		new Widget(),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::ranger)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::spearman)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::crossbowman)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::catapult)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::trebuchet)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::lancer)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::warhorse)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::elephant)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::dragon)])),
		new Draglet(iconSize, &Program::eventPlacePiece, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::throne)])),
		new Widget()
	};
	picons = new Layout(iconSize, pbot);
}

Layout* ProgSetup::createSidebar(int& sideLength) const {
	vector<string> sidt = stage == Stage::tiles ? vector<string>({"Exit", "Next"}) : vector<string>({"Exit", "Back", "Next"});
	sideLength = findMaxLength(sidt.begin(), sidt.end());
	return new Layout(sideLength, stage == Stage::tiles ? vector<Widget*>({new Label(lineHeight, popBack(sidt), &Program::eventExitGame), new Label(lineHeight, popBack(sidt), &Program::eventSetupNext)}) : vector<Widget*>({new Label(lineHeight, popBack(sidt), &Program::eventExitGame), new Label(lineHeight, popBack(sidt), &Program::eventSetupBack), new Label(lineHeight, popBack(sidt), &Program::eventSetupNext)}));
}

// PROG MATCH

void ProgMatch::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventExitGame();
}

Layout* ProgMatch::createLayout() {
	// sidebar
	Text exit("Exit", lineHeight);
	vector<Widget*> left = {
		new Label(lineHeight, exit.text, &Program::eventExitGame)
	};

	// dragon icon
	if (World::game()->getPieces(Piece::Type::dragon)->getPos().has(INT8_MIN)) {
		vector<Widget*> dora = {
			new Draglet(iconSize, &Program::eventPlaceDragon, nullptr, nullptr, {128, 128, 128, 255}, false, World::winSys()->texture(Piece::names[uint8(Piece::Type::dragon)]))
		};
		left.push_back(dragonIcon = new Layout(iconSize, dora, false, false, 0));
	}

	// root layout
	vector<Widget*> cont = {
		new Layout(exit.length, left, true),
		new Widget()
	};
	return new Layout(1.f, cont, false, false, 0);
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventOpenMainMenu();
}

void ProgSettings::eventResized() {
	resolution->setText(World::sets()->resolution.toString());
}

Layout* ProgSettings::createLayout() {
	// side bar
	vector<string> tps = {
		"Reset",
		"Info",
		"Back"
	};
	int optLength = findMaxLength(tps.begin(), tps.end());
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(tps), &Program::eventOpenMainMenu),
		new Label(lineHeight, popBack(tps), &Program::eventOpenInfo),
		new Widget(lineHeight / 2 - Layout::defaultItemSpacing),
		new Label(lineHeight, popBack(tps), &Program::eventResetSettings)
	};

	// setting buttons, labels and action fields for labels
	vector<string> txs = {
		"VSync",
		"Resolution",
		"Fullscreen"
	};
	sizet lnc = txs.size();
	int descLength = findMaxLength(txs.begin(), txs.end());
	vector<Widget*> lx[] = { {
		new Label(descLength, popBack(txs)),
		new CheckBox(lineHeight, World::sets()->fullscreen, &Program::eventSetFullscreen)
	}, {
		new Label(descLength, popBack(txs)),
		resolution = new LabelEdit(1.f, World::sets()->resolution.toString(), &Program::eventSetResolution, nullptr, nullptr, LabelEdit::TextType::uIntSpaced)
	}, {
		new Label(descLength, popBack(txs)),
		new SwitchBox(1.f, Settings::vsyncNames.data(), Settings::vsyncNames.size(), Settings::vsyncNames[uint8(World::sets()->vsync)], &Program::eventSetVsync)
	} };
	vector<Widget*> lns(lnc);
	for (sizet i = 0; i < lnc; i++)
		lns[i] = new Layout(lineHeight, lx[i], false);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, lft, true),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, false, false, superSpacing);
}

// PROG INFO

const array<string, ProgInfo::powerNames.size()> ProgInfo::powerNames = {
	"UNKNOWN",
	"ON_BATTERY",
	"NO_BATTERY",
	"CHARGING",
	"CHARGED"
};
const string ProgInfo::infinity = "\u221E";

void ProgInfo::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventOpenSettings();
}

Layout* ProgInfo::createLayout() {
	// side bar
	vector<string> tps = {
		"Back",
		"Menu"
	};
	int optLength = findMaxLength(tps.begin(), tps.end());
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(tps), &Program::eventOpenMainMenu),
		new Label(lineHeight, popBack(tps), &Program::eventOpenSettings)
	};

	// information block
	vector<string> titles = {
		WindowSys::title,
		"CPU",
		"RAM",
		"Audio",
		"Video",
		"Display",
		"Power",
		"Playback",
		"Recording",
		"Audio Drivers",
		"Video Drivers",
		"Video Displays",
		"Render Drivers"
	};
	std::reverse(titles.begin(), titles.end());
	vector<string> args = {
		"Platform",			// program
		"Path",
		"SDL Version",
		"Logical cores",	// CPU
		"L1 line size",
		"Features",
		"Size",				// RAM
		"Current",			// audio
		"Current",			// video
		"State",			// power
		"Time"
	};
	std::reverse(args.begin(), args.end());
	vector<string> dispArgs = {
		"Index",
		"Name",
		"Resolution",
		"Refresh rate",
		"Pixel format",
		"DPI diagonal",
		"DPI horizontal",
		"DPI vertical",
		"Modes"
	};
	vector<string> rendArgs = {
		"Index",
		"Name",
		"Flags",
		"Max tex size",
		"Tex formats"
	};
	int argWidth = findMaxLength({&args, &dispArgs, &rendArgs});

	vector<Widget*> lines;
	appendProgram(lines, argWidth, args, titles);
	appendCPU(lines, argWidth, args, titles);
	appendRAM(lines, argWidth, args, titles);
	appendCurrent(lines, argWidth, args, titles, SDL_GetCurrentAudioDriver);
	appendCurrent(lines, argWidth, args, titles, SDL_GetCurrentVideoDriver);
	appendCurrentDisplay(lines, argWidth, dispArgs, titles);
	appendPower(lines, argWidth, args, titles);
	appendDevices(lines, argWidth, titles, SDL_GetNumAudioDevices, SDL_GetAudioDeviceName, SDL_FALSE);
	appendDevices(lines, argWidth, titles, SDL_GetNumAudioDevices, SDL_GetAudioDeviceName, SDL_TRUE);
	appendDrivers(lines, argWidth, titles, SDL_GetNumAudioDrivers, SDL_GetAudioDriver);
	appendDrivers(lines, argWidth, titles, SDL_GetNumVideoDrivers, SDL_GetVideoDriver);
	appendDisplays(lines, argWidth, findMaxLength(dispArgs.begin(), dispArgs.end()), dispArgs, titles);
	appendRenderers(lines, argWidth, rendArgs, titles);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, lft, true),
		new ScrollArea(1.f, lines)
	};
	return new Layout(1.f, cont, false, false, superSpacing);
}

void ProgInfo::appendProgram(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, SDL_GetPlatform())}, false)
	});

	if (char* basePath = SDL_GetBasePath()) {
		lines.emplace_back(new Layout(lineHeight, {new Label(width, args.back()), new Label(1.f, basePath)}, false));
		SDL_free(basePath);
	}
	args.pop_back();

	SDL_version ver;
	SDL_GetVersion(&ver);
	Text vernum(to_string(ver.major) + '.' + to_string(ver.minor) + '.' + to_string(ver.patch));
	Text revnum(to_string(SDL_GetRevisionNumber()));

	lines.insert(lines.end(), {
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(vernum.length, vernum.text), new Label(revnum.length, revnum.text), new Label(1.f, SDL_GetRevision())}, false),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendCPU(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	vector<string> features;
	if (SDL_Has3DNow())
		features.emplace_back("3DNow");
	if (SDL_HasAVX())
		features.emplace_back("AVX");
	if (SDL_HasAVX2())
		features.emplace_back("AVX2");
	if (SDL_HasAltiVec())
		features.emplace_back("AltiVec");
	if (SDL_HasMMX())
		features.emplace_back("MMX");
	if (SDL_HasRDTSC())
		features.emplace_back("RDTSC");
	if (SDL_HasSSE())
		features.emplace_back("SSE");
	if (SDL_HasSSE2())
		features.emplace_back("SSE2");
	if (SDL_HasSSE3())
		features.emplace_back("SSE3");
	if (SDL_HasSSE41())
		features.emplace_back("SSE41");
	if (SDL_HasSSE42())
		features.emplace_back("SSE42");

	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetCPUCount()))}, false),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetCPUCacheLineSize()) + 'B')}, false)
	});
	if (!features.empty()) {
		lines.emplace_back(new Layout(lineHeight, {new Label(width, args.back()), new Label(1.f, features[0])}, false));
		for (sizet i = 1; i < features.size(); i++)
			lines.emplace_back(new Layout(lineHeight, {new Widget(width), new Label(1.f, features[i])}, false));
		features.clear();
	} else
		lines.emplace_back(new Layout(lineHeight, {new Label(width, args.back()), new Widget()}, false));
	lines.emplace_back(new Widget(lineHeight));
	args.pop_back();
}

void ProgInfo::appendRAM(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetSystemRAM()) + "MB")}, false),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendCurrent(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles, const char* (*value)()) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	if (const char* name = value())
		lines.emplace_back(new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, name)}, false));
	else
		lines.emplace_back(new Layout(lineHeight, {new Label(width, popBack(args)), new Button()}, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendCurrentDisplay(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	if (int i = World::winSys()->displayID(); i >= 0)
		appendDisplay(lines, i, width, args);
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplay(vector<Widget*>& lines, int i, int width, const vector<string>& args) {
	lines.emplace_back(new Layout(lineHeight, {new Label(width, args[0]), new Label(1.f, to_string(i))}, false));
	if (const char* name = SDL_GetDisplayName(i))
		lines.emplace_back(new Layout(lineHeight, {new Label(width, args[1]), new Label(1.f, name)}, false));
	if (SDL_DisplayMode mode; !SDL_GetDesktopDisplayMode(i, &mode))
		lines.insert(lines.end(), {
			new Layout(lineHeight, {new Label(width, args[2]), new Label(1.f, to_string(mode.w) + " x " + to_string(mode.h))}, false),
			new Layout(lineHeight, {new Label(width, args[3]), new Label(1.f, to_string(mode.refresh_rate) + "Hz")}, false),
			new Layout(lineHeight, {new Label(width, args[4]), new Label(1.f, pixelformatName(mode.format))}, false)
		});
	if (float ddpi, hdpi, vdpi; !SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi))
		lines.insert(lines.end(), {
			new Layout(lineHeight, {new Label(width, args[5]), new Label(1.f, to_string(ddpi))}, false),
			new Layout(lineHeight, {new Label(width, args[6]), new Label(1.f, to_string(hdpi))}, false),
			new Layout(lineHeight, {new Label(width, args[7]), new Label(1.f, to_string(vdpi))}, false)
		});
}

void ProgInfo::appendPower(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	int secs, pct;
	SDL_PowerState power = SDL_GetPowerInfo(&secs, &pct);
	Text tprc((pct >= 0 ? to_string(pct) : infinity) + '%');

	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(1.f, powerNames[power])}, false),
		new Layout(lineHeight, {new Label(width, popBack(args)), new Label(tprc.length, tprc.text), new Label(1.f, (secs >= 0 ? to_string(secs) : infinity) + 's')}, false),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendDevices(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(int), const char* (*value)(int, int), int arg) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < limit(arg); i++)
		if (const char* name = value(i, arg))
			lines.emplace_back(new Layout(lineHeight, {new Label(width, to_string(i)), new Label(1.f, name)}, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int)) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < limit(); i++)
		if (const char* name = value(i))
			lines.emplace_back(new Layout(lineHeight, {new Label(width, to_string(i)), new Label(1.f, name)}, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		appendDisplay(lines, i, argWidth, args);

		lines.emplace_back(new Layout(lineHeight, {new Label(1.f, args[8])}, false));
		for (int j = 0; j < SDL_GetNumDisplayModes(i); j++) {
			lines.emplace_back(new Layout(lineHeight, {new Widget(argWidth / 2), new Label(dispWidth, args[0]), new Label(1.f, to_string(j))}, false));
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, j, &mode))
				lines.insert(lines.end(), {
					new Layout(lineHeight, {new Widget(argWidth / 2), new Label(dispWidth, args[2]), new Label(1.f, to_string(mode.w) + " x " + to_string(mode.h))}, false),
					new Layout(lineHeight, {new Widget(argWidth / 2), new Label(dispWidth, args[3]), new Label(1.f, to_string(mode.refresh_rate))}, false),
					new Layout(lineHeight, {new Widget(argWidth / 2), new Label(dispWidth, args[4]), new Label(1.f, pixelformatName(mode.format))}, false)
				});
			if (j < SDL_GetNumDisplayModes(i) - 1)
				lines.emplace_back(new Widget(lineHeight / 2));
		}
		if (i < SDL_GetNumVideoDisplays() - 1)
			lines.emplace_back(new Widget(lineHeight / 2));
	}
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendRenderers(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
		lines.emplace_back(new Layout(lineHeight, {new Label(width, args[0]), new Label(1.f, to_string(i))}, false));
		if (SDL_RendererInfo info; !SDL_GetRenderDriverInfo(i, &info)) {
			lines.emplace_back(new Layout(lineHeight, {new Label(width, args[1]), new Label(1.f, info.name)}, false));

			vector<string> flags;
			if (info.flags & SDL_RENDERER_SOFTWARE)
				flags.emplace_back("SOFTWARE");
			if (info.flags & SDL_RENDERER_ACCELERATED)
				flags.emplace_back("ACCELERATED");
			if (info.flags & SDL_RENDERER_PRESENTVSYNC)
				flags.emplace_back("PRESENTVSYNC");
			if (info.flags & SDL_RENDERER_TARGETTEXTURE)
				flags.emplace_back("TARGETTEXTURE");

			if (!flags.empty()) {
				lines.emplace_back(new Layout(lineHeight, {new Label(width, args[2]), new Label(1.f, flags[0])}, false));
				for (sizet j = 1; j < flags.size(); j++)
					lines.emplace_back(new Layout(lineHeight, {new Widget(width), new Label(1.f, flags[j])}, false));
			} else
				lines.emplace_back(new Layout(lineHeight, {new Label(width, args[2]), new Widget()}, false));

			lines.emplace_back(new Layout(lineHeight, {new Label(width, args[3]), new Label(1.f, to_string(info.max_texture_width) + " x " + to_string(info.max_texture_height))}, false));
			if (info.num_texture_formats) {
				lines.emplace_back(new Layout(lineHeight, {new Label(width, args[4]), new Label(1.f, pixelformatName(info.texture_formats[0]))}, false));
				for (uint32 i = 1; i < info.num_texture_formats; i++)
					lines.emplace_back(new Layout(lineHeight, {new Widget(width), new Label(1.f, pixelformatName(info.texture_formats[i]))}, false));
			} else
				lines.emplace_back(new Layout(lineHeight, {new Label(width, args[4]), new Label(1.f, to_string(i))}, false));
		}
		if (i < SDL_GetNumRenderDrivers() - 1)
			lines.emplace_back(new Widget(lineHeight / 2));
	}
}
