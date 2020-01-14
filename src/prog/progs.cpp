#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(string str, int height) :
	text(std::move(str)),
	length(strLen(text, height)),
	height(height)
{}

int ProgState::Text::strLen(const string& str, int height) {
	return World::fonts()->length(str, height) + Label::textMargin * 2;
}

template <class T>
int ProgState::Text::maxLen(T pos, T end, int height) {
	int width = 0;
	for (; pos != end; pos++)
		if (int len = strLen(*pos, height); len > width)
			width = len;
	return width;
}

int ProgState::Text::maxLen(const vector<vector<string>*>& lists, int height) {
	int width = 0;
	for (vector<string>* it : lists)
		if (int len = maxLen(it->begin(), it->end(), height); len > width)
			width = len;
	return width;
}

// PROGRAM STATE

ProgState::ProgState() :
	chatBox(nullptr)
{
	eventResize();
}

void ProgState::eventCameraReset() {
	World::scene()->getCamera()->setPos(Camera::posSetup, Camera::latSetup);
}

void ProgState::eventResize() {
	smallHeight = World::window()->screenView().y / 36;	// 20p	(values are in relation to 720p height)
	lineHeight = World::window()->screenView().y / 24;		// 30p
	superHeight = World::window()->screenView().y / 18;	// 40p
	tooltipHeight = World::window()->screenView().y / 45;	// 16p
	lineSpacing = World::window()->screenView().y / 144;	// 5p
	superSpacing = World::window()->screenView().y / 72;	// 10p
	iconSize = World::window()->screenView().y / 11;		// 64p
	tooltipLimit = World::window()->screenView().x / 2;
}

uint8 ProgState::switchButtons(uint8 but) {
	return but;
}

Overlay* ProgState::createOverlay() {
	return nullptr;
}

Popup* ProgState::createPopupMessage(string msg, BCall ccal, string ctxt) const {
	Text ok(std::move(ctxt), superHeight);
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, ok.text, ccal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), ccal, ccal, true, lineSpacing);
}

Popup* ProgState::createPopupChoice(string msg, BCall kcal, BCall ccal) const {
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), kcal, ccal, true, lineSpacing);
}

pair<Popup*, Widget*> ProgState::createPopupInput(string msg, BCall kcal, uint limit) const {
	LabelEdit* ledit = new LabelEdit(1.f, string(), kcal, &Program::eventClosePopup, nullptr, nullptr, Texture(), 1.f, limit);
	vector<Widget*> bot = {
		new Label(1.f, "Ok", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg)),
		ledit,
		new Layout(1.f, std::move(bot), false, 0)
	};
	return pair(new Popup(pair(500, superHeight * 3 + lineSpacing * 2), std::move(con), kcal, &Program::eventClosePopup, true, lineSpacing), ledit);
}

Popup* ProgState::createPopupConfig(const Com::Config& cfg) {
	ConfigIO wio;
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new ScrollArea(1.f, createConfigList(wio, cfg, false), true, lineSpacing),
		new Layout(lineHeight, std::move(bot), false, 0)
	};
	return new Popup(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventClosePopup, true, lineSpacing);
}

Texture ProgState::makeTooltip(const string& text) const {
	return World::fonts()->render(text, tooltipHeight, uint(tooltipLimit));
}

Texture ProgState::makeTooltipL(const vector<string>& lines) const {
	int width = 0;
	for (const string& str : lines)
		if (int len = Text::strLen(str, tooltipHeight); len > width) {
			if (len < tooltipLimit)
				width = len;
			else {
				width = tooltipLimit;
				break;
			}
		}
	string text = lines[0];
	for (sizet i = 1; i < lines.size(); i++)
		text += linend + lines[i];
	return World::fonts()->render(text, tooltipHeight, uint(width));
}

vector<Widget*> ProgState::createChat(bool overlay) {
	return {
		chatBox = new TextBox(1.f, smallHeight, "", nullptr, &Program::eventClearLabel, Texture(), 1.f, World::sets()->chatLines, false, true, 0, Widget::colorDimmed),
		new LabelEdit(smallHeight, "", nullptr, nullptr, &Program::eventSendMessage, overlay ? &Program::eventCloseOverlay : &Program::eventHideChat, Texture(), 1.f, UINT16_MAX - Com::dataHeadSize, true)
	};
}

void ProgState::toggleChatEmbedShow() {
	if (chatBox->getParent()->relSize.usePix) {
		chatBox->getParent()->relSize = chatEmbedSize;
		chatBox->getParent()->getParent()->setSpacing(chatBox->getParent()->relSize.usePix ? 0 : lineSpacing);
		static_cast<LabelEdit*>(chatBox->getParent()->getWidget(chatBox->getID() + 1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
	} else {
		static_cast<LabelEdit*>(chatBox->getParent()->getWidget(chatBox->getID() + 1))->cancel();
		hideChatEmbed();
	}
}

void ProgState::hideChatEmbed() {
	chatBox->getParent()->relSize = 0;
	chatBox->getParent()->getParent()->setSpacing(chatBox->getParent()->relSize.usePix ? 0 : lineSpacing);
}

vector<Widget*> ProgState::createConfigList(ConfigIO& wio, const Com::Config& cfg, bool active) {
	BCall update = active ? &Program::eventUpdateConfig : nullptr;
	vector<string> txs = {
		"Homeland size",
		"Survival check pass",
		"Survival check mode",
		"Fate's Favor limit total",
		"Fate's Favor maximum",
		"Dragon move limit",
		"Dragon move straight",
		"Dragon move diagonal",
		"Fortresses captured",
		"Thrones killed",
		"Capturers",
		"Shift left",
		"Shift near"
	};
	std::reverse(txs.begin(), txs.end());
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);
	vector<string> tips = {
		"Board tile columns",
		"Board tile rows minus one divided by two",
		"Total amount of fate's favors is limited",
		"Maximum amount of fate's favors per player",
		"Maximum movement steps of a dragon",
		"Dragon moves in single directions",
		"Dragon can take diagonal steps",
		"Pieces that are able to capture fortresses",
		"Shift middle tiles to the left of the first player",
		"Shift middle tiles to the closest free space"
	};
	std::reverse(tips.begin(), tips.end());
	string scTip = "Chance of passing the survival check";
	vector<string> survivalTip = {
		"Effect of a failed non-attack survival check:",
		"- DISABLE the acting piece",
		"- FINISH turn",
		"- KILL the acting piece"
	};
	string tileTips[Com::tileLim];
	for (uint8 i = 0; i < Com::tileLim; i++)
		tileTips[i] = "Number of " + Com::tileNames[i] + " tiles per homeland";
	string fortressTip = "(calculated automatically to fill remaining free tiles)";
	string middleTips[Com::tileLim];
	for (uint8 i = 0; i < Com::tileLim; i++)
		middleTips[i] = "Number of " + Com::tileNames[i] + " tiles in middle row per player";
	string pieceTips[Com::pieceMax];
	for (uint8 i = 0; i < Com::pieceMax; i++)
		pieceTips[i] = "Number of " + Com::pieceNames[i] + " pieces per player";
	string fortTip = "Fortresses that need to be captured in order to win";
	string throneTip = "Thrones that need to be killed in order to win";
	Text width("width:", lineHeight);
	Text height("height:", lineHeight);
	Text aleft(arrowLeft, lineHeight);
	Text aright(arrowRight, lineHeight);
	int smWidth = Text::maxLen(Com::Config::survivalNames.begin(), Com::Config::survivalNames.end(), lineHeight);
	Size amtWidth = update ? Size(Text::strLen(toStr(UINT16_MAX), lineHeight) + LabelEdit::caretWidth) : Size(1.f);

	vector<vector<Widget*>> lines0 = { {
		new Label(descLength, popBack(txs)),
		new Label(width.length, std::move(width.text)),
		wio.width = new LabelEdit(1.f, toStr(cfg.homeSize.x), update, nullptr, nullptr, nullptr, makeTooltip(popBack(tips))),
		new Label(height.length, std::move(height.text)),
		wio.height = new LabelEdit(1.f, toStr(cfg.homeSize.y), update, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.survivalLE = new LabelEdit(amtWidth, toStr(cfg.survivalPass) + '%', update, nullptr, nullptr, nullptr, makeTooltip(scTip))
	}, {
		new Label(descLength, popBack(txs)),
		wio.survivalMode = new SwitchBox(active ? Size(smWidth) : Size(1.f), active ? vector<string>(Com::Config::survivalNames.begin(), Com::Config::survivalNames.end()) : vector<string>{ Com::Config::survivalNames[uint8(cfg.survivalMode)] }, Com::Config::survivalNames[uint8(cfg.survivalMode)], update, makeTooltipL(survivalTip), 1.f, active ? Label::Alignment::center : Label::Alignment::left)
	}, {
		new Label(descLength, popBack(txs)),
		wio.favorLimit = new CheckBox(lineHeight, cfg.favorLimit, update, update, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.favorMax = new LabelEdit(1.f, toStr(cfg.favorMax), update, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.dragonDist = new LabelEdit(1.f, toStr(cfg.dragonDist), update, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.dragonSingle = new CheckBox(lineHeight, cfg.dragonSingle, update, update, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.dragonDiag = new CheckBox(lineHeight, cfg.dragonDiag, update, update, makeTooltip(popBack(tips)))
	} };
	if (active) {
		lines0[1].insert(lines0[1].begin() + 1, wio.survivalSL = new Slider(1.f, cfg.survivalPass, 0, Com::Config::randomLimit, update, &Program::eventPrcSliderUpdate, makeTooltip(scTip)));
		lines0[2].insert(lines0[2].begin() + 1, new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltipL(survivalTip)));
		lines0[2].insert(lines0[2].begin() + 3, new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltipL(survivalTip)));
	}

	vector<vector<Widget*>> lines1(Com::tileMax);
	for (uint8 i = 0; i < Com::tileLim; i++)
		lines1[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.tiles[i] = new LabelEdit(amtWidth, toStr(cfg.tileAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(tileTips[i]) : makeTooltipL({ tileTips[i], fortressTip }))
		};
	lines1.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.tileFortress = new Label(1.f, tileFortressString(cfg), nullptr, nullptr, makeTooltipL({ "Number of " + Com::tileNames[uint8(Com::Tile::fortress)] + " tiles per homeland", fortressTip }))
	};
	if (active) {
		uint16 rest = cfg.countFreeTiles();
		for (uint8 i = 0; i < Com::tileLim; i++)
			lines1[i].insert(lines1[i].begin() + 1, new Slider(1.f, cfg.tileAmounts[i], cfg.homeSize.y, cfg.tileAmounts[i] + rest, update, &Program::eventTileSliderUpdate, makeTooltip(tileTips[i])));
	}

	vector<vector<Widget*>> lines2(Com::tileMax);
	for (uint8 i = 0; i < Com::tileLim; i++)
		lines2[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.middles[i] = new LabelEdit(amtWidth, toStr(cfg.middleAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(middleTips[i]) : makeTooltipL({ tileTips[i], fortressTip }))
	};
	lines2.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.middleFortress = new Label(1.f, middleFortressString(cfg), nullptr, nullptr, makeTooltipL({ "Number of " + Com::tileNames[uint8(Com::Tile::fortress)] + " tiles in middle row", fortressTip }))
	};
	if (active) {
		uint16 rest = cfg.countFreeMiddles();
		for (uint8 i = 0; i < lines2.size() - 1; i++)
			lines2[i].insert(lines2[i].begin() + 1, new Slider(1.f, cfg.middleAmounts[i], 0, cfg.middleAmounts[i] + rest, update, &Program::eventMiddleSliderUpdate, makeTooltip(middleTips[i])));
	}

	vector<vector<Widget*>> lines3(Com::pieceMax + 1);
	for (uint8 i = 0; i < Com::pieceMax; i++)
		lines3[i] = {
			new Label(descLength, firstUpper(Com::pieceNames[i])),
			wio.pieces[i] = new LabelEdit(amtWidth, toStr(cfg.pieceAmounts[i]), update, nullptr, nullptr, nullptr, makeTooltip(pieceTips[i]))
		};
	lines3.back() = {
		new Label(descLength, "Total"),
		wio.pieceTotal = new Label(1.f, pieceTotalString(cfg), nullptr, nullptr, makeTooltip("Total amount of pieces out of the maximum possible number of pieces per player"))
	};
	if (active) {
		uint16 rest = cfg.countFreePieces();
		for (uint8 i = 0; i < lines3.size() - 1; i++)
			lines3[i].insert(lines3[i].begin() + 1, new Slider(1.f, cfg.pieceAmounts[i], 0, cfg.pieceAmounts[i] + rest, update, &Program::eventPieceSliderUpdate, makeTooltip(pieceTips[i])));
	}

	vector<vector<Widget*>> lines4 = { {
		new Label(descLength, popBack(txs)),
		wio.winFortress = new LabelEdit(amtWidth, toStr(cfg.winFortress), update, nullptr, nullptr, nullptr, makeTooltip(fortTip))
	}, {
		new Label(descLength, popBack(txs)),
		wio.winThrone = new LabelEdit(amtWidth, toStr(cfg.winThrone), update, nullptr, nullptr, nullptr, makeTooltip(throneTip))
	}, {
		new Label(descLength, popBack(txs)),
		wio.capturers = new LabelEdit(1.f, cfg.capturersString(), update, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	} };
	if (active) {
		lines4[0].insert(lines4[0].begin() + 1, new Slider(1.f, cfg.winFortress, 0, cfg.countFreeTiles(), update, &Program::eventSLUpdateLE, makeTooltip(fortTip)));
		lines4[1].insert(lines4[1].begin() + 1, new Slider(1.f, cfg.winThrone, 0, cfg.pieceAmounts[uint8(Com::Piece::throne)], update, &Program::eventSLUpdateLE, makeTooltip(throneTip)));
	}

	vector<vector<Widget*>> lines5 = { {
		new Label(descLength, popBack(txs)),
		wio.shiftLeft = new CheckBox(lineHeight, cfg.shiftLeft, update, update, makeTooltip(popBack(tips))),
		new Widget()
	}, {
		new Label(descLength, popBack(txs)),
		wio.shiftNear = new CheckBox(lineHeight, cfg.shiftNear, update, update, makeTooltip(popBack(tips))),
		new Widget()
	} };

	sizet id = 0;
	vector<Widget*> menu(lines0.size() + lines1.size() + lines2.size() + lines3.size() + lines4.size() + lines5.size() + 5);	// 5 title bars
	setConfigLines(menu, lines0, id);
	setConfigTitle(menu, "Tile amounts", id);
	setConfigLines(menu, lines1, id);
	setConfigTitle(menu, "Middle amounts", id);
	setConfigLines(menu, lines2, id);
	setConfigTitle(menu, "Piece amounts", id);
	setConfigLines(menu, lines3, id);
	setConfigTitle(menu, "Winning conditions", id);
	setConfigLines(menu, lines4, id);
	setConfigTitle(menu, "Middle row rearranging", id);
	setConfigLines(menu, lines5, id);
	return menu;
}

void ProgState::setConfigLines(vector<Widget*>& menu, vector<vector<Widget*>>& lines, sizet& id) {
	for (vector<Widget*>& it : lines)
		menu[id++] = new Layout(lineHeight, std::move(it), false, lineSpacing);
}

void ProgState::setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) {
	int tlen = Text::strLen(title, lineHeight);
	vector<Widget*> line = {
		new Label(tlen, std::move(title)),
		new Widget()
	};
	menu[id++] = new Layout(lineHeight, std::move(line), false, lineSpacing);
}

// PROG MENU

void ProgMenu::eventEscape() {
	World::window()->close();
}

void ProgMenu::eventEnter() {
	World::program()->eventConnectServer();
}

RootLayout* ProgMenu::createLayout() {
	// server input
	Text srvt("Server:", superHeight);
	vector<Widget*> srv = {
		new Label(srvt.length, srvt.text),
		new LabelEdit(1.f, World::sets()->address, &Program::eventUpdateAddress, &Program::eventResetAddress, &Program::eventConnectServer)
	};

	// port input and connect button
	vector<Widget*> prt = {
		new Label(srvt.length, "Port:"),
		new LabelEdit(1.f, toStr(World::sets()->port), &Program::eventUpdatePort, &Program::eventResetPort, &Program::eventConnectServer, nullptr, Texture())
	};

	// net buttons
	vector<Widget*> con = {
		new Label(srvt.length, "Host", &Program::eventOpenHostMenu, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, Texture(), 1.f, Label::Alignment::center)
	};

	// title
	int width = World::fonts()->length(srvt.text + "000.000.000.000", superHeight) + lineSpacing + Label::textMargin * 4;
	const Texture* title = World::scene()->getTex("title");
	vector<Widget*> tit = {
		new Widget(),
		new Button(title->getRes().y * width / title->getRes().x, nullptr, nullptr, Texture(), 1.f, title->getID(), vec4(1.f))
	};

	// middle buttons
	vector<Widget*> buts = {
		new Layout(1.f, std::move(tit), true, superSpacing),
		new Widget(0),
		new Layout(superHeight, std::move(srv), false, lineSpacing),
		new Layout(superHeight, std::move(prt), false, lineSpacing),
		new Layout(superHeight, std::move(con), false, lineSpacing),
		new Widget(0),
		new Label(superHeight, "Settings", &Program::eventOpenSettings, nullptr, Texture(), 1.f, Label::Alignment::center),
#ifndef EMSCRIPTEN
		new Label(superHeight, "Exit", &Program::eventExit, nullptr, Texture(), 1.f, Label::Alignment::center),
#endif
		new Widget(0.8f)
	};

	// menu and root layout
	vector<Widget*> menu = {
		new Widget(),
		new Layout(width, std::move(buts), true, superSpacing),
		new Widget()
	};
	vector<Widget*> cont = {
		new Layout(1.f, std::move(menu), false, 0),
		new Label(lineHeight, commonVersion, nullptr, nullptr, Texture(), 1.f, Label::Alignment::right, false),
	};
	return new RootLayout(1.f, std::move(cont), true, 0);
}

// PROG LOBBY

ProgLobby::ProgLobby(vector<pair<string, bool>>&& rooms) :
	ProgState(),
	rooms(rooms)
{}

void ProgLobby::eventEscape() {
	World::program()->eventExitLobby();
}

void ProgLobby::eventEnter() {
	World::program()->eventHostRoomInput();
}

RootLayout* ProgLobby::createLayout() {
	// side bar
	vector<string> sidt = {
		"Host",
		"Back"
	};
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(sidt), &Program::eventExitLobby),
		new Label(lineHeight, popBack(sidt), &Program::eventHostRoomInput)
	};

	// room list
	vector<Widget*> lns(rooms.size());
	for (sizet i = 0; i < rooms.size(); i++)
		lns[i] = createRoom(rooms[i].first, rooms[i].second);

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, std::move(lft), true, lineSpacing),
		rlist = new ScrollArea(1.f, std::move(lns), true, lineSpacing)
	};
	return new RootLayout(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

Label* ProgLobby::createRoom(string name, bool open) {
	return new Label(lineHeight, std::move(name), open ? &Program::eventJoinRoomRequest : nullptr, nullptr, Texture(), open ? 1.f : defaultDim);
}

void ProgLobby::addRoom(string&& name) {
	vector<pair<string, bool>>::iterator it = std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return strnatless(rm.first, name); });
	rlist->insertWidget(sizet(it - rooms.begin()), createRoom(name, true));
	rooms.insert(it, pair(std::move(name), true));
}

void ProgLobby::openRoom(const string& name, bool open) {
	Label* le = static_cast<Label*>(rlist->getWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin())));
	le->lcall = open ? &Program::eventJoinRoomRequest : nullptr;
	le->setDim(open ? 1.f : defaultDim);
}

// PROG ROOM

ProgRoom::ProgRoom() :
	ProgState()
{
	if (World::program()->info & Program::INF_HOST) {
		confs = FileSys::loadConfigs();
		umap<string, Com::Config>::iterator it = confs.find(Com::Config::defaultName);
		World::program()->curConfig = it != confs.end() ? it->first : sortNames(confs).front();
		confs[World::program()->curConfig].checkValues();
	}
}

void ProgRoom::eventEscape() {
	World::program()->info & Program::INF_UNIQ ? World::program()->eventOpenMainMenu() : World::program()->eventExitRoom();
}

void ProgRoom::eventEnter() {
	if (startButton->lcall)
		World::program()->eventHostServer();
}

RootLayout* ProgRoom::createLayout() {
	Text back("Back", lineHeight);
	vector<Widget*> top0 = {
		new Label(back.length, back.text, World::program()->info & Program::INF_UNIQ ? &Program::eventOpenMainMenu : &Program::eventExitRoom),
		startButton = new Label(1.f, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	if (World::program()->info & Program::INF_UNIQ) {
		Text setup("Setup", lineHeight);
		Text port("Port:", lineHeight);
		top0.insert(top0.begin() + 1, new Label(setup.length, std::move(setup.text), &Program::eventOpenSetup));
#ifndef EMSCRIPTEN
		top0.insert(top0.end(), {
			new Label(port.length, std::move(port.text)),
			new LabelEdit(Text::strLen("00000", lineHeight) + LabelEdit::caretWidth, toStr(World::sets()->port), &Program::eventUpdatePort)
		});
#endif
	} else if (World::program()->info & Program::INF_HOST) {
		Text kick("Kick", lineHeight);
		top0.push_back(kickButton = new Label(kick.length, std::move(kick.text), nullptr, nullptr, Texture(), defaultDim));
	}

	vector<Widget*> topb = { new Layout(lineHeight, std::move(top0), false, lineSpacing) };
	vector<Widget*> menu = createConfigList(wio, World::program()->info & Program::INF_HOST ? confs[World::program()->curConfig] : World::game()->getConfig(), World::program()->info & Program::INF_HOST);
	if (World::program()->info & Program::INF_HOST) {
		Text cfgt("Configuration:", lineHeight);
		Text aleft(arrowLeft, lineHeight);
		Text aright(arrowRight, lineHeight);
		Text copy("Copy", lineHeight);
		Text newc("New", lineHeight);
		vector<Widget*> top1 = {
			new Label(cfgt.length, cfgt.text),
			new Label(aleft.length, aleft.text, &Program::eventSBPrev),
			new SwitchBox(1.f, sortNames(confs), World::program()->curConfig, &Program::eventSwitchConfig, Texture(), 1.f, Label::Alignment::center),
			new Label(aright.length, aright.text, &Program::eventSBNext),
			new Label(copy.length, copy.text, &Program::eventConfigCopyInput),
			new Label(newc.length, newc.text, &Program::eventConfigNewInput)
		};
		if (confs.size() > 1) {
			Text dele("Del", lineHeight);
			top1.insert(top1.begin() + 4, new Label(dele.length, dele.text, &Program::eventConfigDelete));
		}
		topb.push_back(new Layout(lineHeight, std::move(top1), false, lineSpacing));

		Text reset("Reset", lineHeight);
		vector<Widget*> lineR = {
			new Label(reset.length, reset.text, &Program::eventUpdateReset, nullptr, makeTooltip("Reset current config")),
			new Widget()
		};
		menu.push_back(new Layout(lineHeight, std::move(lineR), false, lineSpacing));
	}

	int topHeight = lineHeight * int(topb.size()) + lineSpacing * int(topb.size() - 1);
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(topb), true, lineSpacing),
		new ScrollArea(1.f, std::move(menu), true, lineSpacing)
	};
	vector<Widget*> rwgt = {
		new Layout(1.f, std::move(cont), true, superSpacing)
	};
	if (!(World::program()->info & Program::INF_UNIQ))
		rwgt.push_back(new Layout(chatEmbedSize, createChat(false)));
	RootLayout* root = new RootLayout(1.f, std::move(rwgt), false, lineSpacing, RootLayout::uniformBgColor);
	updateStartButton();
	return root;
}

void ProgRoom::updateStartButton() {
	if (World::program()->info & Program::INF_UNIQ) {
#ifndef EMSCRIPTEN
		startButton->setText("Open");
		startButton->lcall = &Program::eventHostServer;
#endif
	} else if (World::program()->info & Program::INF_HOST) {
		bool on = World::program()->info & Program::INF_GUEST_WAITING;
		startButton->setText(on ? "Start" : "Waiting for player...");
		startButton->lcall = on ? &Program::eventStartGame : nullptr;
		kickButton->lcall = on ? &Program::eventKickPlayer : nullptr;
		kickButton->setDim(on ? 1.f : defaultDim);
	} else
		startButton->setText("Waiting for start...");
}

void ProgRoom::updateConfigWidgets(const Com::Config& cfg) {
	wio.width->setText(toStr(cfg.homeSize.x));
	wio.height->setText(toStr(cfg.homeSize.y));
	if (World::program()->info & Program::INF_HOST)
		wio.survivalSL->setVal(cfg.survivalPass);
	wio.survivalLE->setText(toStr(cfg.survivalPass) + '%');
	if (World::program()->info & Program::INF_HOST)
		wio.survivalMode->setCurOpt(uint8(cfg.survivalMode));
	else
		wio.survivalMode->set({ Com::Config::survivalNames[uint8(cfg.survivalMode)] }, uint8(cfg.survivalMode));
	wio.favorLimit->on = cfg.favorLimit;
	wio.favorMax->setText(toStr(cfg.favorMax));
	wio.dragonDist->setText(toStr(cfg.dragonDist));
	wio.dragonSingle->on = cfg.dragonSingle;
	wio.dragonDiag->on = cfg.dragonDiag;

	for (uint8 i = 0; i < Com::tileLim; i++)
		wio.tiles[i]->setText(toStr(cfg.tileAmounts[i]));
	wio.tileFortress->setText(tileFortressString(cfg));
	updateAmtSliders(cfg.tileAmounts.data(), wio.tiles.data(), Com::tileLim, cfg.homeSize.y, cfg.countFreeTiles());
	for (uint8 i = 0; i < Com::tileLim; i++)
		wio.middles[i]->setText(toStr(cfg.middleAmounts[i]));
	wio.middleFortress->setText(middleFortressString(cfg));
	updateAmtSliders(cfg.middleAmounts.data(), wio.middles.data(), Com::tileLim, 0, cfg.countFreeMiddles());
	for (uint8 i = 0; i < Com::pieceMax; i++)
		wio.pieces[i]->setText(toStr(cfg.pieceAmounts[i]));
	wio.pieceTotal->setText(pieceTotalString(cfg));
	updateAmtSliders(cfg.pieceAmounts.data(), wio.pieces.data(), Com::pieceMax, 0, cfg.countFreePieces());

	wio.winFortress->setText(toStr(cfg.winFortress));
	updateWinSlider(wio.winFortress, cfg.winFortress, cfg.countFreeTiles());
	wio.winThrone->setText(toStr(cfg.winThrone));
	updateWinSlider(wio.winThrone, cfg.winThrone, cfg.pieceAmounts[uint8(Com::Piece::throne)]);
	wio.capturers->setText(cfg.capturersString());
	wio.shiftLeft->on = cfg.shiftLeft;
	wio.shiftNear->on = cfg.shiftNear;
}

void ProgRoom::updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest) {
	if (World::program()->info & Program::INF_HOST) {
		Layout* bline = wgts[0]->getParent();
		for (uint8 i = 0; i < cnt; i++)
			static_cast<Slider*>(static_cast<Layout*>(bline->getParent()->getWidget(bline->getID() + i))->getWidget(1))->set(amts[i], min, amts[i] + rest);
	}
}

void ProgRoom::updateWinSlider(Label* amt, uint16 val, uint16 max) {
	if (World::program()->info & Program::INF_HOST)
		static_cast<Slider*>(amt->getParent()->getWidget(amt->getID() - 1))->set(val, 0, max);
}

// PROG GAME

uint8 ProgGame::switchButtons(uint8 but) {
	if (bswapIcon->selected) {
		if (but == SDL_BUTTON_LEFT)
			return SDL_BUTTON_RIGHT;
		if (but ==SDL_BUTTON_RIGHT)
			return SDL_BUTTON_LEFT;
	}
	return but;
}

Overlay* ProgGame::createOverlay() {
	return World::netcp() ? new Overlay(pair(0.7f, 0.f), pair(0.3f, 1.f), createChat(), &Program::eventChatOpen, &Program::eventChatClose) : nullptr;
}

// PROG SETUP

ProgSetup::ProgSetup() :
	ProgGame(),
	enemyReady(false),
	selected(0),
	lastButton(0),
	lastHold(UINT16_MAX)
{}

void ProgSetup::eventEscape() {
	stage > Stage::tiles ? World::program()->eventSetupBack() : World::program()->eventAbortGame();
}

void ProgSetup::eventEnter() {
	if (stage < Stage::pieces || World::netcp())
		World::program()->eventSetupNext();
}

void ProgSetup::eventNumpress(uint8 num) {
	if (num < counters.size())
		setSelected(num);
}

void ProgSetup::eventWheel(int ymov) {
	setSelected(cycle(selected, uint8(counters.size()), int8(ymov)));
}

void ProgSetup::eventDrag(uint32 mStat) {
	if (bswapIcon->selected)
		mStat = swapBits(mStat, SDL_BUTTON_LEFT - 1, SDL_BUTTON_RIGHT - 1);

	uint8 curButton = mStat & SDL_BUTTON_LMASK ? SDL_BUTTON_LEFT : mStat & SDL_BUTTON_RMASK ? SDL_BUTTON_RIGHT : 0;
	BoardObject* bo = dynamic_cast<BoardObject*>(World::scene()->select);
	svec2 curHold = bo ? World::game()->ptog(bo->getPos()) : svec2(UINT16_MAX);
	if (bo && curButton && (curHold != lastHold || curButton != lastButton)) {
		if (stage <= Stage::middles)
			curButton == SDL_BUTTON_LEFT ? World::program()->eventPlaceTileH() : World::program()->eventClearTile();
		else if (stage == Stage::pieces)
			curButton == SDL_BUTTON_LEFT ? World::program()->eventPlacePieceH() : World::program()->eventClearPiece();
	}
	lastHold = curHold;
	lastButton = curButton;
}

void ProgSetup::eventUndrag() {
	lastHold = svec2(UINT16_MAX);
}

bool ProgSetup::setStage(ProgSetup::Stage stg) {
	stage = stg;
	setInteractivity();
	switch (stage) {
	case Stage::tiles:
		counters = World::game()->countOwnTiles();
		break;
	case Stage::middles:
		World::game()->fillInFortress();
		counters = World::game()->countMidTiles();
		break;
	case Stage::pieces:
		counters = World::game()->countOwnPieces();
		break;
	case Stage::ready:
		counters.clear();
		return true;
	}
	string txt = chatBox ? chatBox->moveText() : "";
	if (World::scene()->resetLayouts(); chatBox)
		chatBox->setText(std::move(txt));

	selected = 0;	// like setSelected but without crashing
	static_cast<Draglet*>(icons->getWidget(selected + 1))->selected = true;
	for (uint8 i = 0; i < counters.size(); i++)
		switchIcon(i, counters[i], stage != Stage::pieces);
	return false;
}

void ProgSetup::setInteractivity() {
	switch (stage) {
	case Stage::tiles:
		World::game()->setOwnTilesInteract(Tile::Interact::interact);
		World::game()->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->setOwnPiecesVisible(false);
		break;
	case Stage::middles:
		World::game()->setOwnTilesInteract(Tile::Interact::ignore, true);
		World::game()->setMidTilesInteract(Tile::Interact::interact);
		World::game()->setOwnPiecesVisible(false);
		break;
	case Stage::pieces:
		World::game()->setOwnTilesInteract(Tile::Interact::recognize);
		World::game()->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->setOwnPiecesVisible(true);
		break;
	case Stage::ready:
		World::game()->setOwnTilesInteract(Tile::Interact::ignore);
		World::game()->setMidTilesInteract(Tile::Interact::ignore);
		World::game()->disableOwnPiecesInteract(false);
	}
}

void ProgSetup::setSelected(uint8 sel) {
	static_cast<Draglet*>(icons->getWidget(selected + 1))->selected = false;
	selected = sel;
	static_cast<Draglet*>(icons->getWidget(selected + 1))->selected = true;
}

uint8 ProgSetup::findNextSelect(bool fwd) {
	uint8 m = btom<uint8>(fwd);
	for (uint8 i = selected + m; i < counters.size(); i += m)
		if (counters[i])
			return i;
	for (uint8 i = fwd ? 0 : uint8(counters.size() - 1); i != selected; i += m)
		if (counters[i])
			return i;
	return selected;
}

RootLayout* ProgSetup::createLayout() {
	// sidebar
	vector<string> sidt = {
		"Exit",
		"Config",
		"Save",
		"Load",
		"Delete",
		"Back",
		stage < Stage::pieces || !World::netcp() ? "Next" : "Finish"
	};
	if (std::reverse(sidt.begin(), sidt.end()); World::netcp())
		sidt.insert(sidt.begin(), "Chat");
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);
	bool cnext = stage < Stage::pieces || World::netcp(), cback = stage > Stage::tiles;

	vector<Widget*> wgts = {
		new Label(lineHeight, popBack(sidt), &Program::eventAbortGame),
		new Label(lineHeight, popBack(sidt), &Program::eventShowConfig),
		new Label(lineHeight, popBack(sidt), &Program::eventOpenSetupSave),
		new Label(lineHeight, popBack(sidt), &Program::eventOpenSetupLoad),
		bswapIcon = new Draglet(lineHeight, &Program::eventSwitchGameButtons, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, 1.f, Texture(), popBack(sidt), Label::Alignment::left, false),
		new Label(lineHeight, popBack(sidt), cback ? &Program::eventSetupBack : nullptr, nullptr, Texture(), cback ? 1.f : defaultDim),
		new Label(lineHeight, popBack(sidt), cnext ? &Program::eventSetupNext : nullptr, nullptr, Texture(), cnext ? 1.f : defaultDim),
	};
	if (World::netcp())
		wgts.insert(wgts.begin() + 4, new Label(lineHeight, popBack(sidt), &Program::eventToggleChat));

	// center piece
	vector<Widget*> midl = {
		new Widget(1.f),
		message = new Label(superHeight, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center, false),
		new Widget(10.f),
		icons = stage == Stage::pieces ? makePicons() : makeTicons()
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, std::move(wgts), true, lineSpacing),
		new Layout(1.f, std::move(midl), true, 0),
		new Widget(sideLength)
	};
	return new RootLayout(1.f, std::move(cont), false, 0);
}

Layout* ProgSetup::makeTicons() {
	vector<Widget*> tbot(uint8(Com::Tile::fortress) + 2);
	tbot.front() = new Widget();
	for (uint8 i = 0; i < uint8(Com::Tile::fortress); i++)
		tbot[1+i] = new Draglet(iconSize, &Program::eventPlaceTileD, &Program::eventIconSelect, nullptr, World::scene()->texture(Com::tileNames[i]));
	tbot.back() = new Widget();
	return new Layout(iconSize, std::move(tbot), false, lineSpacing);
}

Layout* ProgSetup::makePicons() {
	vector<Widget*> pbot(Com::pieceNames.size() + 2);
	pbot.front() = new Widget();
	for (uint8 i = 0; i < Com::pieceNames.size(); i++)
		pbot[1+i] = new Draglet(iconSize, &Program::eventPlacePieceD, &Program::eventIconSelect, nullptr, World::scene()->texture(Com::pieceNames[i]));
	pbot.back() = new Widget();
	return new Layout(iconSize, std::move(pbot), false, lineSpacing);
}

void ProgSetup::incdecIcon(uint8 type, bool inc, bool isTile) {
	if (!inc && counters[type] == 1) {
		switchIcon(type, false, isTile);
		setSelected(findNextSelect(true));
	} else if (inc && !counters[type])
		switchIcon(type, true, isTile);
	counters[type] += btom<uint16>(inc);
}

void ProgSetup::switchIcon(uint8 type, bool on, bool isTile) {
	Draglet* ico = getIcon(type);
	ico->setDim(on ? 1.f : defaultDim);
	ico->lcall = on ? isTile ? &Program::eventPlaceTileD : &Program::eventPlacePieceD : nullptr;
}

Popup* ProgSetup::createPopupSaveLoad(bool save) {
	Text back("Back", lineHeight);
	Text tnew("New", lineHeight);
	vector<Widget*> top = {
		new Label(back.length, std::move(back.text), &Program::eventClosePopup)
	};
	if (save)
		top.insert(top.end(), {
			new LabelEdit(1.f, string(), nullptr, nullptr, &Program::eventSetupNew),
			new Label(tnew.length, std::move(tnew.text), &Program::eventSetupNew)
		});

	setups = FileSys::loadSetups();
	vector<string> names = sortNames(setups);
	vector<Widget*> saves(setups.size());
	for (sizet i = 0; i < setups.size(); i++)
		saves[i] = new Label(lineHeight, std::move(names[i]), save ? &Program::eventSetupSave : &Program::eventSetupLoad, &Program::eventSetupDelete);

	vector<Widget*> con = {
		new Layout(lineHeight, std::move(top), false, lineSpacing),
		new ScrollArea(1.f, std::move(saves), true, lineSpacing)
	};
	return new Popup(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventClosePopup, true, superSpacing);
}

// PROG MATCH

void ProgMatch::eventEscape() {
	World::program()->eventAbortGame();
}

void ProgMatch::eventEnter() {
	eventEndTurn();
}

void ProgMatch::eventWheel(int ymov) {
	World::scene()->getCamera()->zoom(ymov);
}

void ProgMatch::eventFavorize(FavorAct act) {
	if (favorIcon)
		if (selectFavorIcon(act, false); Piece* pce = dynamic_cast<Piece*>(World::scene()->capture))
			World::program()->eventFavorStart(pce);	// vaguely simulate what happens when holding down on a piece to refresh favor state and highlighted tiles (no need to take warhorse into account)
}

void ProgMatch::eventEndTurn() {
	if (turnIcon->lcall)
		World::program()->eventEndTurn();
}

void ProgMatch::eventCameraReset() {
	World::scene()->getCamera()->setPos(Camera::posMatch, Camera::latMatch);
}

FavorAct ProgMatch::favorIconSelect() const {
	if (favorIcon) {
		if (fnowIcon->selected)
			return FavorAct::now;
		if (favorIcon->selected)
			return FavorAct::on;
	}
	return FavorAct::off;
}

void ProgMatch::selectFavorIcon(FavorAct act, bool force) {
	if (favorIcon) {
		if (force || favorIcon->lcall)
			favorIcon->selected = act != FavorAct::off;
		if (force || fnowIcon->lcall)
			fnowIcon->selected = act == FavorAct::now;
	}
}

void ProgMatch::updateFavorIcon(bool on, uint8 cnt, uint8 tot) {
	if (favorIcon) {
		on = on && cnt;
		favorIcon->lcall = on ? &Program::eventSwitchFavor : nullptr;
		favorIcon->setDim(on ? 1.f : defaultDim);
		favorIcon->setText("FF: " + toStr(cnt) + '/' + toStr(tot));
	}
}

void ProgMatch::updateFnowIcon(bool on, uint8 cnt) {
	if (favorIcon) {
		on = on && cnt;
		fnowIcon->lcall = on ? &Program::eventSwitchFavorNow : nullptr;
		fnowIcon->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::updateTurnIcon(bool on) {
	turnIcon->lcall = on ? &Program::eventEndTurn : nullptr;
	turnIcon->setDim(on ? 1.f : defaultDim);
}

void ProgMatch::setDragonIcon(bool on) {
	if (dragonIcon) {
		Draglet* ico = static_cast<Draglet*>(dragonIcon->getWidget(0));
		ico->lcall = on ? &Program::eventPlaceDragon : nullptr;
		ico->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::decreaseDragonIcon() {
	if (!--unplacedDragons) {
		dragonIcon->getParent()->deleteWidget(dragonIcon->getID());
		dragonIcon = nullptr;
	}
}

RootLayout* ProgMatch::createLayout() {
	// sidebar
	bool fon = World::game()->getConfig().favorMax && World::game()->getConfig().pieceAmounts[uint8(Com::Piece::throne)];
	vector<string> sidt = {
		"Exit",
		"Config",
		"Chat",
		"Fire",
		"Finish"
	};
	if (std::reverse(sidt.begin(), sidt.end()); fon)
		sidt.insert(sidt.begin(), { "FF: /" + string(numDigits10(World::game()->getConfig().favorMax) * 2, '0'), "FF now" });
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);

	vector<Widget*> left = {
		new Label(lineHeight, popBack(sidt), &Program::eventAbortGame),
		new Label(lineHeight, popBack(sidt), &Program::eventShowConfig),
		new Label(lineHeight, popBack(sidt), &Program::eventToggleChat),
		bswapIcon = new Draglet(lineHeight, &Program::eventSwitchGameButtons, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, 1.f, Texture(), popBack(sidt), Label::Alignment::left, false),
		turnIcon = new Label(lineHeight, popBack(sidt))
	};
	if (fon) {
		left.insert(left.end() - 1, {
			favorIcon = new Draglet(lineHeight, nullptr, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, 1.f, Texture(), "", Label::Alignment::left, false),	// text is updated after the icon has a parent
			fnowIcon = new Draglet(lineHeight, nullptr, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, 1.f, Texture(), popBack(sidt), Label::Alignment::left, false)
		});
	} else
		fnowIcon = favorIcon = nullptr;
	
	unplacedDragons = 0;
	for (Piece* pce = World::game()->getOwnPieces(Com::Piece::dragon); pce->getType() == Com::Piece::dragon; pce++)
		if (!World::game()->pieceOnBoard(pce))
			unplacedDragons++;
	if (unplacedDragons) {
		left.push_back(dragonIcon = new Layout(iconSize, { new Draglet(iconSize, nullptr, nullptr, nullptr, World::scene()->texture(Com::pieceNames[uint8(Com::Piece::dragon)]), vec4(1.f)) }, false, 0));
		setDragonIcon(World::game()->getMyTurn());
	} else
		dragonIcon = nullptr;

	// middle and root layout
	vector<Widget*> midl = {
		new Layout(sideLength, std::move(left), true, lineSpacing),
		new Widget(1.f)
	};
	vector<Widget*> cont = {
		new Layout(1.f, std::move(midl), false, 0),
		message = new Label(superHeight, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center, false),
		new Widget(lineSpacing * 2)
	};
	RootLayout* root = new RootLayout(1.f, std::move(cont), true, 0);
	updateTurnIcon(World::game()->getMyTurn());
	updateFavorIcon(World::game()->getMyTurn(), World::game()->getFavorCount(), World::game()->getFavorTotal());
	updateFnowIcon(World::game()->getMyTurn(), World::game()->getFavorCount());
	return root;
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	World::program()->eventOpenMainMenu();
}

void ProgSettings::eventEnter() {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
	World::program()->eventApplySettings();
#endif
}

RootLayout* ProgSettings::createLayout() {
	// side bar
	vector<string> tps = {
		"Reset",
		"Info",
		"Back"
	};
	int optLength = Text::maxLen(tps.begin(), tps.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(tps), &Program::eventOpenMainMenu),
		new Label(lineHeight, popBack(tps), &Program::eventOpenInfo),
		new Widget(0),
		new Label(lineHeight, popBack(tps), &Program::eventResetSettings)
	};

	// resolution list
	vector<ivec2> sizes = World::window()->displaySizes();
	vector<SDL_DisplayMode> modes = World::window()->displayModes();
	pixelformats.clear();
	for (const SDL_DisplayMode& it : modes)
		pixelformats.emplace(pixelformatName(it.format), it.format);
	vector<string> winsiz(sizes.size()), dmodes(modes.size());
	std::transform(sizes.begin(), sizes.end(), winsiz.begin(), [](ivec2 size) -> string { return toStr(size, rv2iSeparator); });
	std::transform(modes.begin(), modes.end(), dmodes.begin(), dispToFstr);

	// setting buttons, labels and action fields for labels
	Text aleft(arrowLeft, lineHeight);
	Text aright(arrowRight, lineHeight);
	Text aptx("Apply", lineHeight);
	vector<string> txs = {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		"Display",
		"Screen",
		"Size",
		"Mode",
#endif
#ifndef OPENGLES
		"Multisamples",
		"Shadow resolution",
		"Soft shadows",
#endif
		"Texture scale",
		"VSync",
		"Gamma",
		"Volume",
		"Scale tiles",
		"Scale pieces",
		"Chat line limit",
		"Regular font"
	};
	vector<string> tips = {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		"Keep 0 cause it doesn't really work",
		"Window or fullscreen (\"desktop\" is native resolution)",
		"Window size",
		"Fullscreen display properties",
#endif
#ifndef OPENGLES
		"Anti-Aliasing multisamples (requires restart)",
		"Width/height of shadow cubemap",
		"Soft shadow edges",
#endif
		"Scale factor of texture sizes",
		"Brightness",
		"Audio volume",
		"Scale tile amounts when resizing board",
		"Scale piece amounts when resizing board",
		"Line break limit of a chat box",
		"Use the standard Romanesque or Merriweather to support more characters",
		"Apply \"Display\", \"Screen\", \"Size\" and \"Mode\""
	};
	vector<string> vsyncTip = { "Immediate: off", "Synchronized: on", "Adaptive: on and smooth (works on fewer computers)" };
	std::reverse(txs.begin(), txs.end());
	std::reverse(tips.begin(), tips.end());
	sizet lnc = txs.size();
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);
	int slleLength = Text::strLen("000%", lineHeight) + LabelEdit::caretWidth;

	vector<Widget*> lx[] = { {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		new Label(descLength, popBack(txs)),
		display = new LabelEdit(1.f, toStr(World::sets()->display), nullptr, nullptr, &Program::eventDummy, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(tips.back())),
		screen = new SwitchBox(1.f, vector<string>(Settings::screenNames.begin(), Settings::screenNames.end()), Settings::screenNames[uint8(World::sets()->screen)], &Program::eventDummy, makeTooltip(tips.back())),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(tips.back())),
		winSize = new SwitchBox(1.f, std::move(winsiz), toStr(World::sets()->size, rv2iSeparator), &Program::eventDummy, makeTooltip(tips.back())),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(tips.back())),
		dspMode = new SwitchBox(1.f, std::move(dmodes), dispToFstr(World::sets()->mode), &Program::eventDummy, makeTooltip(tips.back())),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(popBack(tips)))
	}, {
#endif
#ifndef OPENGLES
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(tips.back())),
		new SwitchBox(1.f, { "0", "1", "2", "4" }, toStr(World::sets()->msamples), &Program::eventSetSamples, makeTooltip(tips.back())),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Slider(1.f, World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1, -1, 15, nullptr, &Program::eventSetShadowResSL, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->shadowRes), &Program::eventSetShadowResLE, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new CheckBox(lineHeight, World::sets()->softShadows, &Program::eventSetSoftShadows, &Program::eventSetSoftShadows, makeTooltip(popBack(tips)))
	}, {
#endif
		new Label(descLength, popBack(txs)),
		new Slider(1.f, World::sets()->texScale, 1, 100, &Program::eventSetTexturesScaleSL, &Program::eventPrcSliderUpdate, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->texScale) + '%', &Program::eventSetTextureScaleLE, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltipL(vsyncTip)),
		new SwitchBox(1.f, vector<string>(Settings::vsyncNames.begin(), Settings::vsyncNames.end()), Settings::vsyncNames[uint8(int8(World::sets()->vsync)+1)], &Program::eventSetVsync, makeTooltipL(vsyncTip)),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltipL(vsyncTip))
	}, {
		new Label(descLength, popBack(txs)),
		new Slider(1.f, int(World::sets()->gamma * gammaStepFactor), 0, int(Settings::gammaMax * gammaStepFactor), &Program::eventSaveSettings, &Program::eventSetGammaSL, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->gamma), &Program::eventSetGammaLE, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, World::audio() ? vector<Widget*>{
		new Label(descLength, popBack(txs)),
		new Slider(1.f, World::sets()->avolume, 0, SDL_MIX_MAXVOLUME, &Program::eventSaveSettings, &Program::eventSetVolumeSL, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->avolume), &Program::eventSetVolumeLE, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	} : vector<Widget*>{
		new Label(descLength, popBack(txs)),
		new Label(1.f, "missing audio device")
	}, {
		new Label(descLength, popBack(txs)),
		new CheckBox(lineHeight, World::sets()->scaleTiles, &Program::eventSetScaleTiles, &Program::eventSetScaleTiles, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new CheckBox(lineHeight, World::sets()->scalePieces, &Program::eventSetScalePieces, &Program::eventSetScalePieces, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new Slider(1.f, World::sets()->chatLines, 0, Settings::chatLinesMax, &Program::eventSetChatLineLimitSL, &Program::eventSLUpdateLE, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->chatLines), &Program::eventSetChatLineLimitLE, nullptr, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		new CheckBox(lineHeight, World::sets()->fontRegular, &Program::eventSetFontRegular, &Program::eventSetFontRegular, makeTooltip(popBack(tips)))
	}, };
#if defined(__ANDROID__) || defined(EMSCRIPTEN)
	vector<Widget*> lns(lnc);
#else
	vector<Widget*> lns(lnc + 2);
	lns[lnc] = new Widget(0);
	lns[lnc+1] = new Layout(lineHeight, { new Label(aptx.length, aptx.text, &Program::eventApplySettings, nullptr, makeTooltip(popBack(tips))) }, false, lineSpacing);
#endif
	for (sizet i = 0; i < lnc; i++)
		lns[i] = new Layout(lineHeight, std::move(lx[i]), false, lineSpacing);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		new ScrollArea(1.f, std::move(lns), true, lineSpacing)
	};
	return new RootLayout(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
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
	World::program()->eventOpenSettings();
}

void ProgInfo::eventEnter() {
	World::program()->eventOpenSettings();
}

RootLayout* ProgInfo::createLayout() {
	// side bar
	vector<string> tps = {
		"Back",
		"Menu"
	};
	int optLength = Text::maxLen(tps.begin(), tps.end(), lineHeight);
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
		"Version",			// program
		"Path",
		"Platform",
		"OpenGL",
		"Channels",
		"Framebuffer size",
		"Double buffer",
		"Depth buffer size",
		"Stencil buffer size",
		"Accumulator",
		"Stereo 3D",
		"Multisample buffers",
		"Multisamples",
		"Hardware acceleration",
		"Context version",
		"Context flags",
		"Context sharing",
		"Framebuffer sRGB",
		"Release Behaviour",
#if !defined(OPENGLES) && !defined(__APPLE__)
		"GLEW",
#endif
		"GLM",
		"SDL",
		"SDL_net",
		"SDL_ttf",
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
		"Max texture size",
		"Texture formats"
	};
	int argWidth = Text::maxLen({ &args, &dispArgs, &rendArgs }, lineHeight);

	vector<Widget*> lines;
	appendProgram(lines, argWidth, args, titles);
	appendCPU(lines, argWidth, args, titles);
	appendRAM(lines, argWidth, args, titles);
	appendCurrent(lines, argWidth, args, titles, SDL_GetCurrentAudioDriver);
	appendCurrent(lines, argWidth, args, titles, SDL_GetCurrentVideoDriver);
	appendCurrentDisplay(lines, argWidth, dispArgs, titles);
	appendPower(lines, argWidth, args, titles);
	appendAudioDevices(lines, argWidth, titles, SDL_FALSE);
	appendAudioDevices(lines, argWidth, titles, SDL_TRUE);
	appendDrivers(lines, argWidth, titles, SDL_GetNumAudioDrivers, SDL_GetAudioDriver);
	appendDrivers(lines, argWidth, titles, SDL_GetNumVideoDrivers, SDL_GetVideoDriver);
	appendDisplays(lines, argWidth, Text::maxLen(dispArgs.begin(), dispArgs.end(), lineHeight), dispArgs, titles);
	appendRenderers(lines, argWidth, rendArgs, titles);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		new ScrollArea(1.f, std::move(lines), true, lineSpacing)
	};
	return new RootLayout(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

void ProgInfo::appendProgram(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, commonVersion) }, false, lineSpacing)
	});
	if (char* basePath = SDL_GetBasePath()) {
		lines.push_back(new Layout(lineHeight, { new Label(width, std::move(args.back())), new Label(1.f, basePath) }, false, lineSpacing));
		SDL_free(basePath);
	}
	args.pop_back();

	int red, green, blue, alpha, buffer, dbuffer, depth, stencil, ared, agreen, ablue, aalpha, stereo, msbuffers, msamples, accvisual, vmaj, vmin, cflags, cprofile, swcc, srgb, crb;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha);
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &buffer);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &dbuffer);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &ared);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &agreen);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &ablue);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &aalpha);
	SDL_GL_GetAttribute(SDL_GL_STEREO, &stereo);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &msbuffers);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &msamples);
	SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &accvisual);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &vmaj);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &vmin);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &cflags);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &cprofile);
	SDL_GL_GetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, &swcc);
	SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &srgb);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, &crb);
#ifndef __APPLE__
	GLint gvmaj, gvmin;
	glGetIntegerv(GL_MAJOR_VERSION, &gvmaj);
	glGetIntegerv(GL_MINOR_VERSION, &gvmin);
#ifdef OPENGLES
	string glvers = "ES " + toStr(gvmaj) + '.' + toStr(gvmin);
#else
	string glvers = toStr(gvmaj) + '.' + toStr(gvmin);
#endif
#endif
	string cvers = toStr(vmaj) + '.' + toStr(vmin);
	if (cprofile & SDL_GL_CONTEXT_PROFILE_CORE)
		cvers += " Core";
	if (cprofile & SDL_GL_CONTEXT_PROFILE_ES)
		cvers += " ES";
	if (cprofile & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
		cvers += " Compatibility";

	string sflags;
	if (cflags & SDL_GL_CONTEXT_DEBUG_FLAG)
		sflags += "Debug ";
	if (cflags & SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG)
		sflags += "ForwardCompatible ";
	if (cflags & SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG)
		sflags += "RobustAccess ";
	if (cflags & SDL_GL_CONTEXT_RESET_ISOLATION_FLAG)
		sflags += "ResetIsolation ";
	if (sflags.empty())
		sflags = "None";
	else
		sflags.pop_back();

	SDL_version bver, ncver, tcver;
	SDL_GetVersion(&bver);
	Text slv(versionText(bver), lineHeight);
	Text slr(toStr(SDL_GetRevisionNumber()), lineHeight);
	SDL_VERSION(&bver)
	Text scv(versionText(bver), lineHeight);
	const SDL_version* nlver = SDLNet_Linked_Version();
	SDL_NET_VERSION(&ncver)
	const SDL_version* tlver = TTF_Linked_Version();
	SDL_TTF_VERSION(&tcver)
	lines.insert(lines.end(), {
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, SDL_GetPlatform()) }, false, lineSpacing),
#ifndef __APPLE__
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, std::move(glvers)) }, false, lineSpacing),
#endif
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, 'R' + toStr(red) + " G" + toStr(green) + " B" + toStr(blue) + " A" + toStr(alpha)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(buffer)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(dbuffer)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(depth)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(stencil)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, 'R' + toStr(ared) + " G" + toStr(agreen) + " B" + toStr(ablue) + " A" + toStr(aalpha)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(stereo)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(msbuffers)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(msamples)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(accvisual)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, std::move(cvers)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, std::move(sflags)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(swcc)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(srgb)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, ibtos(crb)) }, false, lineSpacing),
#if !defined(OPENGLES) && !defined(__APPLE__)
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, reinterpret_cast<const char*>(glewGetString(GLEW_VERSION))) }, false, lineSpacing),
#endif
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(GLM_VERSION_MAJOR) + '.' + toStr(GLM_VERSION_MINOR) + '.' + toStr(GLM_VERSION_PATCH) + '.' + toStr(GLM_VERSION_REVISION)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(slv.length, std::move(slv.text)), new Label(slr.length, std::move(slr.text)), new Label(1.f, SDL_GetRevision()), new Label(scv.length, std::move(scv.text)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, versionText(*nlver)), new Label(1.f, versionText(ncver)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, versionText(*tlver)), new Label(1.f, versionText(tcver)) }, false, lineSpacing),
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
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(SDL_GetCPUCount())) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(SDL_GetCPUCacheLineSize()) + 'B') }, false, lineSpacing)
	});
	if (!features.empty()) {
		lines.push_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, std::move(features[0])) }, false, lineSpacing));
		for (sizet i = 1; i < features.size(); i++)
			lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, std::move(features[i])) }, false, lineSpacing));
		features.clear();
	} else
		lines.push_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Widget() }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendRAM(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(SDL_GetSystemRAM()) + "MB") }, false, lineSpacing),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendCurrent(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles, const char* (*value)()) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	if (const char* name = value())
		lines.push_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, name) }, false, lineSpacing));
	else
		lines.push_back(new Layout(lineHeight, { new Label(width, popBack(args)), new Button() }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendCurrentDisplay(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	if (int i = World::window()->displayID(); i >= 0)
		appendDisplay(lines, i, width, args);
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplay(vector<Widget*>& lines, int i, int width, const vector<string>& args) {
	lines.push_back(new Layout(lineHeight, { new Label(width, args[0]), new Label(1.f, toStr(i)) }, false, lineSpacing));
	if (const char* name = SDL_GetDisplayName(i))
		lines.push_back(new Layout(lineHeight, { new Label(width, args[1]), new Label(1.f, name) }, false, lineSpacing));
	if (SDL_DisplayMode mode; !SDL_GetDesktopDisplayMode(i, &mode))
		lines.insert(lines.end(), {
			new Layout(lineHeight, { new Label(width, args[2]), new Label(1.f, toStr(mode.w) + " x " + toStr(mode.h)) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, toStr(mode.refresh_rate) + "Hz") }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, pixelformatName(mode.format)) }, false, lineSpacing)
		});
	if (float ddpi, hdpi, vdpi; !SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi))
		lines.insert(lines.end(), {
			new Layout(lineHeight, { new Label(width, args[5]), new Label(1.f, toStr(ddpi)) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[6]), new Label(1.f, toStr(hdpi)) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[7]), new Label(1.f, toStr(vdpi)) }, false, lineSpacing)
		});
}

void ProgInfo::appendPower(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles) {
	int secs, pct;
	SDL_PowerState power = SDL_GetPowerInfo(&secs, &pct);
	Text tprc(pct >= 0 ? toStr(pct) + '%' : "inf", lineHeight);

	lines.insert(lines.end(), {
		new Label(superHeight, popBack(titles)),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, powerNames[power]) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(tprc.length, std::move(tprc.text)), new Label(1.f, secs >= 0 ? toStr(secs) + 's' : "inf") }, false, lineSpacing),
		new Widget(lineHeight)
	});
}

void ProgInfo::appendAudioDevices(vector<Widget*>& lines, int width, vector<string>& titles, int iscapture) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumAudioDevices(iscapture); i++)
		if (const char* name = SDL_GetAudioDeviceName(i, iscapture))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int)) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < limit(); i++)
		if (const char* name = value(i))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		appendDisplay(lines, i, argWidth, args);

		lines.push_back(new Layout(lineHeight, { new Label(1.f, args[8]) }, false, lineSpacing));
		for (int j = 0; j < SDL_GetNumDisplayModes(i); j++) {
			lines.push_back(new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[0]), new Label(1.f, toStr(j)) }, false, lineSpacing));
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, j, &mode))
				lines.insert(lines.end(), {
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[2]), new Label(1.f, toStr(mode.w) + " x " + toStr(mode.h)) }, false, lineSpacing),
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[3]), new Label(1.f, toStr(mode.refresh_rate)) }, false, lineSpacing),
					new Layout(lineHeight, { new Widget(argWidth / 2), new Label(dispWidth, args[4]), new Label(1.f, pixelformatName(mode.format)) }, false, lineSpacing)
				});
			if (j < SDL_GetNumDisplayModes(i) - 1)
				lines.push_back(new Widget(lineHeight / 2));
		}
		if (i < SDL_GetNumVideoDisplays() - 1)
			lines.push_back(new Widget(lineHeight / 2));
	}
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendRenderers(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles) {
	lines.push_back(new Label(superHeight, popBack(titles)));
	for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
		lines.push_back(new Layout(lineHeight, { new Label(width, args[0]), new Label(1.f, toStr(i)) }, false, lineSpacing));
		if (SDL_RendererInfo info; !SDL_GetRenderDriverInfo(i, &info)) {
			lines.push_back(new Layout(lineHeight, { new Label(width, args[1]), new Label(1.f, info.name) }, false, lineSpacing));

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
				lines.push_back(new Layout(lineHeight, { new Label(width, args[2]), new Label(1.f, std::move(flags[0])) }, false, lineSpacing));
				for (sizet j = 1; j < flags.size(); j++)
					lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, std::move(flags[j])) }, false, lineSpacing));
			} else
				lines.push_back(new Layout(lineHeight, { new Label(width, args[2]), new Widget() }, false, lineSpacing));

			lines.push_back(new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, toStr(info.max_texture_width) + " x " + toStr(info.max_texture_height)) }, false, lineSpacing));
			if (info.num_texture_formats) {
				lines.push_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, pixelformatName(info.texture_formats[0])) }, false, lineSpacing));
				for (uint32 i = 1; i < info.num_texture_formats; i++)
					lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, pixelformatName(info.texture_formats[i])) }, false, lineSpacing));
			} else
				lines.push_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, toStr(i)) }, false, lineSpacing));
		}
		if (i < SDL_GetNumRenderDrivers() - 1)
			lines.push_back(new Widget(lineHeight / 2));
	}
}

string ProgInfo::ibtos(int val) {
	if (!val)
		return "off";
	if (val == 1)
		return "on";
	return toStr(val);
}
