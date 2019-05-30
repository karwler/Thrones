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
		World::prun(World::scene()->getPopup()->ccall, nullptr);
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
		new Label(ok.length, ok.text, ccal, nullptr, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, 0)
	};
	return new Popup(cvec2<Size>(ms.length, superHeight * 2 + Layout::defaultItemSpacing), con, ccal, ccal);
}

Popup* ProgState::createPopupChoice(const string& msg, BCall kcal, BCall ccal) {
	Text ms(msg, superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, 0)
	};
	return new Popup(cvec2<Size>(ms.length, superHeight * 2 + Layout::defaultItemSpacing), con, kcal, ccal);
}

pair<Popup*, Widget*> ProgState::createPopupInput(const string& msg, BCall kcal) {
	LabelEdit* ledit = new LabelEdit(1.f, emptyStr, kcal, &Program::eventClosePopup, kcal);
	vector<Widget*> bot = {
		new Label(1.f, "Ok", kcal, nullptr, Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, msg),
		ledit,
		new Layout(1.f, bot, false, 0)
	};
	return pair(new Popup(cvec2<Size>(500, superHeight * 3 + Layout::defaultItemSpacing * 2), con, kcal, &Program::eventClosePopup), ledit);
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

	// net buttons
	vector<Widget*> con = {
		new Label(srvt.length, "Host", &Program::eventOpenHostMenu, nullptr, Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, Label::Alignment::center)
	};

	// middle buttons
	vector<Widget*> buts = {
		new Widget(),
		new Layout(superHeight, srv, false),
		new Layout(superHeight, prt, false),
		new Layout(superHeight, con, false),
		new Widget(0),
		new Label(superHeight, "Settings", &Program::eventOpenSettings, nullptr, Label::Alignment::center),
		new Label(superHeight, "Exit", &Program::eventExit, nullptr, Label::Alignment::center),
		new Widget()
	};

	// root layout
	vector<Widget*> cont = {
		new Widget(),
		new Layout(World::winSys()->textLength(srvt.text + "000.000.000.000", superHeight) + Layout::defaultItemSpacing + Label::defaultTextMargin * 4, buts, true, superSpacing),
		new Widget()
	};
	return new Layout(1.f, cont, false, 0);
}

// PROG HOST

ProgHost::ProgHost() :
	confs(Com::loadConfs(Com::defaultConfigFile))
{
	if (curConf = sizet(std::find_if(confs.begin(), confs.end(), [](const Com::Config& cf) -> bool { return cf.name == Com::Config::defaultName; }) - confs.begin()); curConf >= confs.size())
		curConf = 0;
}

void ProgHost::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventExitGame();
}

Layout* ProgHost::createLayout() {
	vector<string> cfgNames(confs.size());
	for (sizet i = 0; i < confs.size(); i++)
		cfgNames[i] = confs[i].name;

	Text back("Back", superHeight);
	Text cfgt("Configuration: ", lineHeight);
	Text copy("Copy");
	Text newc("New");
	Text port("Port:", lineHeight);
	vector<Widget*> top0 = {
		new Label(back.length, back.text, &Program::eventExitHost),
		new Label(1.f, "Open", &Program::eventHostServer, nullptr, Label::Alignment::center)
	};
	vector<Widget*> top1 = {
		new Label(cfgt.length, cfgt.text),
		new SwitchBox(1.f, cfgNames.data(), cfgNames.size(), cfgNames[curConf], &Program::eventSwitchConfig, Label::Alignment::center),
		new Label(copy.length, copy.text, &Program::eventConfigCopyInput),
		new Label(newc.length, newc.text, &Program::eventConfigNewInput),
		new Widget(superSpacing),
		new Label(port.length, port.text),
		new LabelEdit(World::winSys()->textLength("00000", lineHeight), to_string(World::sets()->port), &Program::eventUpdatePort, nullptr, nullptr, LabelEdit::TextType::uInt),
	};
	if (confs.size() > 1) {
		Text dele("Del");
		top1.insert(top1.begin() + 2, new Label(dele.length, dele.text, &Program::eventConfigDeleteInput));
	}

	Text reset("Reset");
	vector<string> txs = {
		"Board width",
		"Homeland height",
		"Fortresses captured",
		"Thrones killed",
		"Capturers",
		"Shift left",
		"Shift near"
	};
	std::reverse(txs.begin(), txs.end());
	int descLength = findMaxLength(txs.begin(), txs.end());

	vector<vector<Widget*>> lines0 = { {
		new Label(descLength, popBack(txs)),
		inWidth = new LabelEdit(1.f, to_string(confs[curConf].homeWidth), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::sInt)
	}, {
		new Label(descLength, popBack(txs)),
		inHeight = new LabelEdit(1.f, to_string(confs[curConf].homeHeight), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::sInt)
	} };

	vector<vector<Widget*>> lines1(Com::tileMax);
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		lines1[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			inTiles[i] = new LabelEdit(1.f, to_string(confs[curConf].tileAmounts[i]), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::uInt)
		};
	lines1.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		outTileFortress = new Label(1.f, tileFortressString(confs[curConf]))
	};

	vector<vector<Widget*>> lines2(Com::tileMax);
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		lines2[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			inMiddles[i] = new LabelEdit(1.f, to_string(confs[curConf].middleAmounts[i]), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::uInt)
	};
	lines2.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		outMiddleFortress = new Label(1.f, middleFortressString(confs[curConf]))
	};

	vector<vector<Widget*>> lines3(Com::pieceMax + 1);
	for (uint8 i = 0; i < Com::pieceMax; i++)
		lines3[i] = {
			new Label(descLength, firstUpper(Com::pieceNames[i])),
			inPieces[i] = new LabelEdit(1.f, to_string(confs[curConf].pieceAmounts[i]), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::uInt)
		};
	lines3.back() = {
		new Label(descLength, "Total"),
		outPieceTotal = new Label(1.f, pieceTotalString(confs[curConf]))
	};

	vector<vector<Widget*>> lines4 = { {
		new Label(descLength, popBack(txs)),
		inWinFortress = new LabelEdit(1.f, to_string(confs[curConf].winFortress), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::sInt)
	}, {
		new Label(descLength, popBack(txs)),
		inWinThrone = new LabelEdit(1.f, to_string(confs[curConf].winThrone), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig, LabelEdit::TextType::sInt)
	}, {
		new Label(descLength, popBack(txs)),
		inCapturers = new LabelEdit(1.f, confs[curConf].capturersString(), &Program::eventUpdateConfig, nullptr, &Program::eventUpdateConfig)
	} };

	vector<vector<Widget*>> lines5 = { {
		new Label(descLength, popBack(txs)),
		inShiftLeft = new CheckBox(lineHeight, confs[curConf].shiftLeft, &Program::eventUpdateConfig),
		new Widget()
	}, {
		new Label(descLength, popBack(txs)),
		inShiftNear = new CheckBox(lineHeight, confs[curConf].shiftNear, &Program::eventUpdateConfig),
		new Widget()
	} };

	vector<vector<Widget*>> lineR = { {
		new Label(reset.length, reset.text, &Program::eventUpdateReset),
		new Widget()
	} };

	sizet id = 0;
	vector<Widget*> menu(lines0.size() + lines1.size() + lines2.size() + lines3.size() + lines4.size() + lines5.size() + lineR.size() + 5);
	setLines(menu, lines0, id);
	setTitle(menu, "Tile amounts", id);
	setLines(menu, lines1, id);
	setTitle(menu, "Middle amounts", id);
	setLines(menu, lines2, id);
	setTitle(menu, "Piece amounts", id);
	setLines(menu, lines3, id);
	setTitle(menu, "Winning conditions", id);
	setLines(menu, lines4, id);
	setTitle(menu, "Middle row rearranging", id);
	setLines(menu, lines5, id);
	setLines(menu, lineR, id);

	vector<Widget*> cont = {
		new Layout(superHeight, top0, false),
		new Layout(lineHeight, top1, false),
		new ScrollArea(1.f, menu)
	};
	return new Layout(1.f, cont, true, superSpacing);
}

void ProgHost::setLines(vector<Widget*>& menu, const vector<vector<Widget*>>& lines, sizet& id) {
	for (const vector<Widget*>& it : lines)
		menu[id++] = new Layout(lineHeight, it, false);
}

void ProgHost::setTitle(vector<Widget*>& menu, const string& title, sizet& id) {
	vector<Widget*> line = {
		new Label(Text::strLength(title), title),
		new Widget()
	};
	menu[id++] = new Layout(lineHeight, line, false);
}

// PROG SETUP

ProgSetup::ProgSetup() :
	enemyReady(false),
	selected(0)
{}

void ProgSetup::eventEscape() {
	if (!tryClosePopup())
		stage > Stage::tiles && stage < Stage::ready ? World::program()->eventSetupBack() : World::scene()->setPopup(createPopupChoice("Exit game?", &Program::eventExitGame, &Program::eventClosePopup));
}

void ProgSetup::eventWheel(int ymov) {
	setSelected(cycle(selected, uint8(counters.size()), int8(-ymov)));
}

bool ProgSetup::setStage(ProgSetup::Stage stg) {
	switch (stage = stg) {
	case Stage::tiles:
		World::game()->setOwnTilesInteract(Tile::Interactivity::tiling);
		World::game()->setMidTilesInteract(false);
		World::game()->setOwnPiecesVisible(false);
		counters = World::game()->countOwnTiles();
		break;
	case Stage::middles:
		World::game()->setOwnTilesInteract(Tile::Interactivity::none);
		World::game()->setMidTilesInteract(true);
		World::game()->setOwnPiecesVisible(false);
		counters = World::game()->countMidTiles();
		break;
	case Stage::pieces:
		World::game()->setOwnTilesInteract(Tile::Interactivity::piecing);
		World::game()->setMidTilesInteract(false);
		World::game()->setOwnPiecesVisible(true);
		counters = World::game()->countOwnPieces();
		break;
	case Stage::ready:
		World::game()->setOwnTilesInteract(Tile::Interactivity::none);
		World::game()->setMidTilesInteract(false);
		World::game()->setOwnPiecesInteract(false);
		counters.clear();
		return true;
	}
	World::scene()->resetLayouts();
	setSelected(0);
	for (uint8 i = 0; i < counters.size(); i++)
		switchIcon(i, counters[i], stage != Stage::pieces);
	return false;
}

void ProgSetup::setSelected(uint8 sel) {
	static_cast<Draglet*>(icons->getWidget(selected + 1))->setSelected(false);
	selected = sel;
	static_cast<Draglet*>(icons->getWidget(selected + 1))->setSelected(true);
}

Layout* ProgSetup::createLayout() {
	// center piece
	icons = stage == Stage::pieces ? getPicons() : getTicons();
	vector<Widget*> midl = {
		new Widget(1.f),
		message = new Label(superHeight, emptyStr, nullptr, nullptr, Label::Alignment::center, nullptr, Widget::colorNormal, false, 0),
		new Widget(5.f),
		icons
	};

	// root layout
	int sideLength;
	vector<Widget*> cont = {
		createSidebar(sideLength),
		new Layout(1.f, midl, true, 0),
		new Widget(sideLength)
	};
	return new Layout(1.f, cont, false, 0);
}

Layout* ProgSetup::getTicons() {
	vector<Widget*> tbot = { new Widget() };
	for (uint8 i = 0; i < 4; i++)
		tbot.push_back(new Draglet(iconSize, &Program::eventPlaceTileD, nullptr, true, World::scene()->material(Com::tileNames[i])->diffuse));
	tbot.push_back(new Widget());
	return new Layout(iconSize, tbot, false);
}

Layout* ProgSetup::getPicons() {
	vector<Widget*> pbot = { new Widget() };
	for (const string& it : Com::pieceNames)
		pbot.push_back(new Draglet(iconSize, &Program::eventPlacePieceD, nullptr, false, vec4(0.5f, 0.5f, 0.5f, 1.f), World::winSys()->texture(it)));
	pbot.push_back(new Widget());
	return new Layout(iconSize, pbot, false);
}

Layout* ProgSetup::createSidebar(int& sideLength) const {
	vector<string> sidt = stage == Stage::tiles ? vector<string>({ "Exit", "Next" }) : vector<string>({ "Exit", "Back", "Next" });
	sideLength = findMaxLength(sidt.begin(), sidt.end());
	return new Layout(sideLength, stage == Stage::tiles ? vector<Widget*>({ new Label(lineHeight, popBack(sidt), &Program::eventSetupNext), new Label(lineHeight, popBack(sidt), &Program::eventExitGame) }) : vector<Widget*>({ new Label(lineHeight, popBack(sidt), &Program::eventSetupNext), new Label(lineHeight, popBack(sidt), &Program::eventSetupBack), new Label(lineHeight, popBack(sidt), &Program::eventExitGame) }));
}

void ProgSetup::incdecIcon(uint8 type, bool inc, bool isTile) {
	if (!inc && counters[type] == 1)
		switchIcon(type, false, isTile);
	else if (inc && !counters[type])
		switchIcon(type, true, isTile);
	counters[type] += btom<uint16>(inc);
}

void ProgSetup::switchIcon(uint8 type, bool on, bool isTile) {
	if (Draglet* ico = getIcon(type); !on) {
		ico->setDim(0.5f);
		ico->setLcall(nullptr);
	} else {
		ico->setDim(1.f);
		if (isTile) {
			ico->color = World::scene()->material(Com::tileNames[type])->diffuse;
			ico->setLcall(&Program::eventPlaceTileD);
		} else {
			ico->color = Object::defaultColor;
			ico->setLcall(&Program::eventPlacePieceD);
		}
	}
}

// PROG MATCH

void ProgMatch::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventExitGame();
}

void ProgMatch::updateFavorIcon(bool on, uint8 cnt, uint8 tot) {
	favorIcon->setDim(on && cnt ? 1.f : 0.5f);
	favorIcon->setText("FF: " + to_string(cnt) + '/' + to_string(tot));
}

void ProgMatch::setDragonIcon(bool on) {
	if (dragonIcon) {
		if (Draglet* ico = static_cast<Draglet*>(dragonIcon->getWidget(0)); on) {
			ico->setLcall(&Program::eventPlaceDragon);
			ico->setDim(1.f);
		} else {
			ico->setLcall(nullptr);
			ico->setDim(0.5f);
		}
	}
}

void ProgMatch::deleteDragonIcon() {
	dragonIcon->getParent()->deleteWidget(dragonIcon->getID());
	dragonIcon = nullptr;
}

Layout* ProgMatch::createLayout() {
	// sidebar
	int sideLength = Text::strLength("FF: 0/0");
	vector<Widget*> left = {
		new Label(lineHeight, "Exit", &Program::eventExitGame),
		new Widget(0),
		favorIcon = new Draglet(lineHeight, nullptr, nullptr, true, World::scene()->material("gold")->diffuse)
	};
	
	if (World::game()->ptog(World::game()->getOwnPieces(Com::Piece::dragon)->pos).hasNot(INT16_MIN))
		dragonIcon = nullptr;
	else {
		left.push_back(dragonIcon = new Layout(iconSize, { new Draglet(iconSize, nullptr, nullptr, false, Object::defaultColor, World::winSys()->texture(Com::pieceNames[uint8(Com::Piece::dragon)])) }, false, 0));
		setDragonIcon(World::game()->getMyTurn());
	}

	// middle for message
	vector<Widget*> midl = {
		new Widget(),
		message = new Label(superHeight, World::game()->getMyTurn() ? Game::messageTurnGo : Game::messageTurnWait, nullptr, nullptr, Label::Alignment::center, nullptr, Widget::colorNormal, false, 0),
		new Widget(10)
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, left, true),
		new Layout(1.f, midl, true, 0),
		new Widget(sideLength)
	};
	return new Layout(1.f, cont, false, 0);
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventOpenMainMenu();
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
		new Widget(0),
		new Label(lineHeight, popBack(tps), &Program::eventResetSettings)
	};

	// resolution list
	vector<vec2i> sizes = World::winSys()->displaySizes();
	vector<SDL_DisplayMode> modes = World::winSys()->displayModes();
	pixelformats.clear();
	for (const SDL_DisplayMode& it : modes)
		pixelformats.emplace(pixelformatName(it.format), it.format);

	vector<string> winsiz(sizes.size()), dmodes(modes.size());
	std::transform(sizes.begin(), sizes.end(), winsiz.begin(), sizeToFstr);
	std::transform(modes.begin(), modes.end(), dmodes.begin(), dispToFstr);
	vector<string> samples = { "0", "1", "2", "4" };

	// setting buttons, labels and action fields for labels
	Text aptx("Apply", lineHeight);
	vector<string> txs = {
		"Screen",
		"Size",
		"Mode",
		"VSync",
		"Multisamples",
		"Smooth"
	};
	sizet lnc = txs.size();
	int descLength = findMaxLength(txs.begin(), txs.end());

	vector<Widget*> lx[] = { {
		new Label(descLength, txs[0]),
		screen = new SwitchBox(1.f, Settings::screenNames.data(), Settings::screenNames.size(), Settings::screenNames[uint8(World::sets()->screen)])
	}, {
		new Label(descLength, txs[1]),
		winSize = new SwitchBox(1.f, winsiz.data(), winsiz.size(), World::sets()->size.toString(rv2iSeparator))
	}, {
		new Label(descLength, txs[2]),
		dspMode = new SwitchBox(1.f, dmodes.data(), dmodes.size(), dispToFstr(World::sets()->mode))
	}, {
		new Label(descLength, txs[3]),
		new SwitchBox(1.f, Settings::vsyncNames.data(), Settings::vsyncNames.size(), Settings::vsyncNames[uint8(int8(World::sets()->vsync)+1)], &Program::eventSetVsync)
	}, {
		new Label(descLength, txs[4]),
		msample = new SwitchBox(1.f, samples.data(), samples.size(), to_string(World::sets()->samples))
	}, {
		new Label(descLength, txs[5]),
		new SwitchBox(1.f, Settings::smoothNames.data(), Settings::smoothNames.size(), Settings::smoothNames[uint8(World::sets()->smooth)], &Program::eventSetSmooth)
	}, };
	vector<Widget*> lns(lnc + 2);
	for (sizet i = 0; i < lnc; i++)
		lns[i] = new Layout(lineHeight, lx[i], false);
	lns[lnc] = new Widget(0);
	lns[lnc+1] = new Layout(lineHeight, { new Label(aptx.length, aptx.text, &Program::eventApplySettings) }, false);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, lft, true),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, false, superSpacing);
}

SDL_DisplayMode ProgSettings::currentMode() const {
	SDL_DisplayMode mode = strToDisp(dspMode->getText());
	if (string::const_reverse_iterator sit = std::find_if(dspMode->getText().rbegin(), dspMode->getText().rend(), [](char c) -> bool { return isSpace(c); }); sit != dspMode->getText().rend())
		if (umap<string, uint32>::const_iterator pit = pixelformats.find(string(sit.base(), dspMode->getText().end())); pit != pixelformats.end())
			mode.format = pit->second;
	return mode;
}

// PROG INFO

const array<string, ProgInfo::powerNames.size()> ProgInfo::powerNames = {
	"UNKNOWN",
	"ON_BATTERY",
	"NO_BATTERY",
	"CHARGING",
	"CHARGED"
};

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
	int argWidth = findMaxLength({ &args, &dispArgs, &rendArgs });

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
	return new Layout(1.f, cont, false, superSpacing);
}

void ProgInfo::appendProgram(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, SDL_GetPlatform()) }, false)
	});

	if (char* basePath = SDL_GetBasePath()) {
		lines.emplace_back(new Layout(lineHeight, { new Label(width, args.back()), new Label(1.f, basePath) }, false));
		SDL_free(basePath);
	}
	args.pop_back();

	SDL_version ver;
	SDL_GetVersion(&ver);
	Text vernum(to_string(ver.major) + '.' + to_string(ver.minor) + '.' + to_string(ver.patch));
	Text revnum(to_string(SDL_GetRevisionNumber()));

	lines.insert(lines.end(), {
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(vernum.length, vernum.text), new Label(revnum.length, revnum.text), new Label(1.f, SDL_GetRevision()) }, false),
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
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetCPUCount())) }, false),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetCPUCacheLineSize()) + 'B') }, false)
	});
	if (!features.empty()) {
		lines.emplace_back(new Layout(lineHeight, { new Label(width, args.back()), new Label(1.f, features[0]) }, false));
		for (sizet i = 1; i < features.size(); i++)
			lines.emplace_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, features[i]) }, false));
		features.clear();
	} else
		lines.emplace_back(new Layout(lineHeight, { new Label(width, args.back()), new Widget() }, false));
	lines.emplace_back(new Widget(lineHeight));
	args.pop_back();
}

void ProgInfo::appendRAM(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, to_string(SDL_GetSystemRAM()) + "MB") }, false),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendCurrent(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles, const char* (*value)()) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	if (const char* name = value())
		lines.emplace_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, name) }, false));
	else
		lines.emplace_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Button() }, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendCurrentDisplay(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	if (int i = World::winSys()->displayID(); i >= 0)
		appendDisplay(lines, i, width, args);
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplay(vector<Widget*>& lines, int i, int width, const vector<string>& args) {
	lines.emplace_back(new Layout(lineHeight, { new Label(width, args[0]), new Label(1.f, to_string(i)) }, false));
	if (const char* name = SDL_GetDisplayName(i))
		lines.emplace_back(new Layout(lineHeight, { new Label(width, args[1]), new Label(1.f, name) }, false));
	if (SDL_DisplayMode mode; !SDL_GetDesktopDisplayMode(i, &mode))
		lines.insert(lines.end(), {
			new Layout(lineHeight, { new Label(width, args[2]), new Label(1.f, to_string(mode.w) + " x " + to_string(mode.h)) }, false),
			new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, to_string(mode.refresh_rate) + "Hz") }, false),
			new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, pixelformatName(mode.format)) }, false)
		});
	if (float ddpi, hdpi, vdpi; !SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi))
		lines.insert(lines.end(), {
			new Layout(lineHeight, { new Label(width, args[5]), new Label(1.f, to_string(ddpi)) }, false),
			new Layout(lineHeight, { new Label(width, args[6]), new Label(1.f, to_string(hdpi)) }, false),
			new Layout(lineHeight, { new Label(width, args[7]), new Label(1.f, to_string(vdpi)) }, false)
		});
}

void ProgInfo::appendPower(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	int secs, pct;
	SDL_PowerState power = SDL_GetPowerInfo(&secs, &pct);
	Text tprc((pct >= 0 ? to_string(pct) : u8"\u221E") + '%');

	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, powerNames[power]) }, false),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(tprc.length, tprc.text), new Label(1.f, (secs >= 0 ? to_string(secs) : u8"\u221E") + 's') }, false),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendDevices(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(int), const char* (*value)(int, int), int arg) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < limit(arg); i++)
		if (const char* name = value(i, arg))
			lines.emplace_back(new Layout(lineHeight, { new Label(width, to_string(i)), new Label(1.f, name) }, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int)) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < limit(); i++)
		if (const char* name = value(i))
			lines.emplace_back(new Layout(lineHeight, { new Label(width, to_string(i)), new Label(1.f, name) }, false));
	lines.emplace_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles) {
	lines.emplace_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		appendDisplay(lines, i, argWidth, args);

		lines.emplace_back(new Layout(lineHeight, { new Label(1.f, args[8]) }, false));
		for (int j = 0; j < SDL_GetNumDisplayModes(i); j++) {
			lines.emplace_back(new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[0]), new Label(1.f, to_string(j)) }, false));
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, j, &mode))
				lines.insert(lines.end(), {
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[2]), new Label(1.f, to_string(mode.w) + " x " + to_string(mode.h)) }, false),
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[3]), new Label(1.f, to_string(mode.refresh_rate)) }, false),
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[4]), new Label(1.f, pixelformatName(mode.format)) }, false)
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
		lines.emplace_back(new Layout(lineHeight, { new Label(width, args[0]), new Label(1.f, to_string(i)) }, false));
		if (SDL_RendererInfo info; !SDL_GetRenderDriverInfo(i, &info)) {
			lines.emplace_back(new Layout(lineHeight, { new Label(width, args[1]), new Label(1.f, info.name) }, false));

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
				lines.emplace_back(new Layout(lineHeight, { new Label(width, args[2]), new Label(1.f, flags[0]) }, false));
				for (sizet j = 1; j < flags.size(); j++)
					lines.emplace_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, flags[j]) }, false));
			} else
				lines.emplace_back(new Layout(lineHeight, { new Label(width, args[2]), new Widget() }, false));

			lines.emplace_back(new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, to_string(info.max_texture_width) + " x " + to_string(info.max_texture_height)) }, false));
			if (info.num_texture_formats) {
				lines.emplace_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, pixelformatName(info.texture_formats[0])) }, false));
				for (uint32 i = 1; i < info.num_texture_formats; i++)
					lines.emplace_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, pixelformatName(info.texture_formats[i])) }, false));
			} else
				lines.emplace_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, to_string(i)) }, false));
		}
		if (i < SDL_GetNumRenderDrivers() - 1)
			lines.emplace_back(new Widget(lineHeight / 2));
	}
}
