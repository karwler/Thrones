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

int ProgState::Text::maxLen(const vector<vector<string>*>& lists, int height) {
	int width = 0;
	for (vector<string>* it : lists)
		if (int len = maxLen(it->begin(), it->end(), height); len > width)
			width = len;
	return width;
}

// PROGRAM STATE

void ProgState::eventCameraReset() {
	World::scene()->getCamera()->setPos(Camera::posSetup, Camera::latSetup);
}

void ProgState::eventResize() {
	int scale = World::window()->getView().y;	// following pixel values are in relation to 720p height
	lineHeight = scale / 24;	// 30p
	superHeight = scale / 18;	// 40p
	tooltipHeight = scale / 45;	// 16p
	lineSpacing = scale / 144;	// 5p
	superSpacing = scale / 72;	// 10p
	iconSize = scale / 11;		// 64p
}

RootLayout* ProgState::createLayout() {
	return new RootLayout;
}

Popup* ProgState::createPopupMessage(string msg, BCall ccal, string ctxt) {
	Text ok(std::move(ctxt), superHeight);
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, ok.text, ccal, nullptr, Texture(), Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Texture(), Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), ccal, ccal, true, lineSpacing);
}

Popup* ProgState::createPopupChoice(string msg, BCall kcal, BCall ccal) {
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, Texture(), Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, Texture(), Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, Texture(), Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), kcal, ccal, true, lineSpacing);
}

pair<Popup*, Widget*> ProgState::createPopupInput(string msg, BCall kcal, uint limit) {
	LabelEdit* ledit = new LabelEdit(1.f, string(), kcal, &Program::eventClosePopup, nullptr, Texture(), limit);
	vector<Widget*> bot = {
		new Label(1.f, "Ok", kcal, nullptr, Texture(), Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, Texture(), Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg)),
		ledit,
		new Layout(1.f, std::move(bot), false, 0)
	};
	return pair(new Popup(pair(500, superHeight * 3 + lineSpacing * 2), std::move(con), kcal, &Program::eventClosePopup, true, lineSpacing), ledit);
}

Popup* ProgState::createPopupConfig() {
	ConfigIO wio;
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, Texture(), Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new ScrollArea(1.f, createConfigList(wio, false), true, lineSpacing),
		new Layout(lineHeight, std::move(bot), false, 0)
	};
	return new Popup(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventClosePopup, true, lineSpacing);
}

Texture ProgState::makeTooltip(const string& text) {
	return World::fonts()->render(text, tooltipHeight, tooltipLimit);
}

Texture ProgState::makeTooltip(const vector<string>& lines) {
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

vector<Widget*> ProgState::createConfigList(ConfigIO& wio, bool active) {
	BCall update = active ? &Program::eventUpdateConfig : nullptr;
	vector<string> txs = {
		"Board width",
		"Homeland height",
		"Survival pass",
		"Fate's Favors limit",
		"Dragon move limit",
		"Dragon step diagonal",
		"Multistage turns",
		"Survival kill",
		"Fortresses captured",
		"Thrones killed",
		"Capturers",
		"Shift left",
		"Shift near"
	};
	vector<string> tips = {
		"Amount of tiles per row",
		"Amount of tiles per column minus one divided by two",
		"Maximum amount of fate's favors per player",
		"Maximum movement steps of a dragon",
		"Whether a dragon can make diagonal steps",
		"Whether a firing piece can move and fire in one turn",
		"Whether a failed survival check kills the piece",
		"Amount of enemy fortresses that need to be captured in order to win",
		"Amount of enemy thrones that need to be killed in order to win",
		"Pieces that are able to capture fortresses",
		"Whether to shift middle tiles to the left of the first player",
		"Whether to shift middle tiles to the closest free space"
	};
	string scTip = "Chance of passing the survival check";
	std::reverse(txs.begin(), txs.end());
	std::reverse(tips.begin(), tips.end());
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);

	vector<vector<Widget*>> lines0 = { {
		new Label(descLength, popBack(txs)),
		wio.width = new LabelEdit(1.f, toStr(World::game()->config.homeSize.x), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.height = new LabelEdit(1.f, toStr(World::game()->config.homeSize.y), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.survivalLE = new LabelEdit(update ? Size(Text::strLen("100%", lineHeight) + LabelEdit::caretWidth) : Size(1.f), toStr(World::game()->config.survivalPass) + '%', update, nullptr, nullptr, makeTooltip(scTip))
	}, {
		new Label(descLength, popBack(txs)),
		wio.favors = new LabelEdit(1.f, toStr(World::game()->config.favorLimit), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.dragonDist = new LabelEdit(1.f, toStr(World::game()->config.dragonDist), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.dragonDiag = new CheckBox(lineHeight, World::game()->config.dragonDiag, update, update, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.multistage = new CheckBox(lineHeight, World::game()->config.multistage, update, update, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.survivalKill = new CheckBox(lineHeight, World::game()->config.survivalKill, update, update, makeTooltip(popBack(tips)))
	} };
	if (active)
		lines0[2].insert(lines0[2].begin() + 1, wio.survivalSL = new Slider(1.f, World::game()->config.survivalPass, 0, 100, update, &Program::eventPrcSliderUpdate, makeTooltip(scTip)));

	vector<vector<Widget*>> lines1(Com::tileMax);
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		lines1[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.tiles[i] = new LabelEdit(1.f, toStr(World::game()->config.tileAmounts[i]), update, nullptr, nullptr, makeTooltip("Number of " + Com::tileNames[i] + " tiles per homeland"))
		};
	lines1.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.tileFortress = new Label(1.f, tileFortressString(World::game()->config), nullptr, nullptr, makeTooltip({ "Number of " + Com::tileNames[uint8(Com::Tile::fortress)] + " tiles per homeland", "(calculated automatically to fill remaining free tiles)" }))
	};

	vector<vector<Widget*>> lines2(Com::tileMax);
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		lines2[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.middles[i] = new LabelEdit(1.f, toStr(World::game()->config.middleAmounts[i]), update, nullptr, nullptr, makeTooltip("Number of " + Com::tileNames[i] + " tiles in middle row per player"))
	};
	lines2.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.middleFortress = new Label(1.f, middleFortressString(World::game()->config), nullptr, nullptr, makeTooltip({ "Number of " + Com::tileNames[uint8(Com::Tile::fortress)] + " tiles in middle row", "(calculated automatically to fill remaining free tiles)" }))
	};

	vector<vector<Widget*>> lines3(Com::pieceMax + 1);
	for (uint8 i = 0; i < Com::pieceMax; i++)
		lines3[i] = {
			new Label(descLength, firstUpper(Com::pieceNames[i])),
			wio.pieces[i] = new LabelEdit(1.f, toStr(World::game()->config.pieceAmounts[i]), update, nullptr, nullptr, makeTooltip("Number of " + Com::pieceNames[i] + " pieces per player"))
		};
	lines3.back() = {
		new Label(descLength, "Total"),
		wio.pieceTotal = new Label(1.f, pieceTotalString(World::game()->config), nullptr, nullptr, makeTooltip("Total amount of pieces out of the maximum possible number of pieces per player"))
	};

	vector<vector<Widget*>> lines4 = { {
		new Label(descLength, popBack(txs)),
		wio.winFortress = new LabelEdit(1.f, toStr(World::game()->config.winFortress), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.winThrone = new LabelEdit(1.f, toStr(World::game()->config.winThrone), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, {
		new Label(descLength, popBack(txs)),
		wio.capturers = new LabelEdit(1.f, World::game()->config.capturersString(), update, nullptr, nullptr, makeTooltip(popBack(tips)))
	} };

	vector<vector<Widget*>> lines5 = { {
		new Label(descLength, popBack(txs)),
		wio.shiftLeft = new CheckBox(lineHeight, World::game()->config.shiftLeft, update, update, makeTooltip(popBack(tips))),
		new Widget()
	}, {
		new Label(descLength, popBack(txs)),
		wio.shiftNear = new CheckBox(lineHeight, World::game()->config.shiftNear, update, update, makeTooltip(popBack(tips))),
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
		new LabelEdit(1.f, toStr(World::sets()->port), &Program::eventUpdatePort, &Program::eventResetPort, &Program::eventConnectServer, Texture())
	};

	// net buttons
	vector<Widget*> con = {
		new Label(srvt.length, "Host", &Program::eventOpenHostMenu, nullptr, Texture(), Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, Texture(), Label::Alignment::center)
	};

	// title
	int width = World::fonts()->length(srvt.text + "000.000.000.000", superHeight) + lineSpacing + Label::textMargin * 4;
	const Texture* title = World::scene()->getTex("title");
	vector<Widget*> tit = {
		new Widget(),
		new Button(title->getRes().y * width / title->getRes().x, nullptr, nullptr, Texture(), title->getID(), vec4(1.f))
	};

	// middle buttons
	vector<Widget*> buts = {
		new Layout(1.f, std::move(tit), true, superSpacing),
		new Widget(0),
		new Layout(superHeight, std::move(srv), false, lineSpacing),
		new Layout(superHeight, std::move(prt), false, lineSpacing),
		new Layout(superHeight, std::move(con), false, lineSpacing),
		new Widget(0),
		new Label(superHeight, "Settings", &Program::eventOpenSettings, nullptr, Texture(), Label::Alignment::center),
		new Label(superHeight, "Exit", &Program::eventExit, nullptr, Texture(), Label::Alignment::center),
		new Widget(0.9f)
	};

	// root layout
	vector<Widget*> cont = {
		new Widget(),
		new Layout(width, std::move(buts), true, superSpacing),
		new Widget()
	};
	return new RootLayout(1.f, std::move(cont), false, 0);
}

// PROG LOBBY

ProgLobby::ProgLobby(vector<pair<string, bool>>&& rooms) :
	ProgState(),
	rooms(rooms)
{}

void ProgLobby::eventEscape() {
	World::program()->eventExitLobby();
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
	return new Label(lineHeight, std::move(name), &Program::eventJoinRoomRequest, nullptr, Texture(), Label::Alignment::left, true, 0, Widget::colorNormal * (open ? 1.f : 0.5f));
}

void ProgLobby::addRoom(string&& name) {
	vector<pair<string, bool>>::iterator it = std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return strnatless(rm.first, name); });
	rlist->insertWidget(sizet(it - rooms.begin()), createRoom(name, true));
	rooms.insert(it, pair(std::move(name), true));
}

// PROG ROOM

ProgRoom::ProgRoom() :
	ProgState()
{
	if (World::program()->info & Program::INF_HOST) {
		confs = FileSys::loadConfigs();
		umap<string, Com::Config>::iterator it = confs.find(Com::Config::defaultName);
		World::game()->configName = it != confs.end() ? it->first : sortNames(confs).front();
		World::game()->config = confs[World::game()->configName].checkValues();
	}
}

void ProgRoom::eventEscape() {
	World::program()->info & Program::INF_UNIQ ? World::program()->eventOpenMainMenu() : World::program()->eventExitRoom();
}

RootLayout* ProgRoom::createLayout() {
	Text back("Back", lineHeight);
	vector<Widget*> top0 = {
		new Label(back.length, back.text, World::program()->info & Program::INF_UNIQ ? &Program::eventOpenMainMenu : &Program::eventExitRoom),
		startButton = new Label(1.f, string(), nullptr, nullptr, Texture(), Label::Alignment::center)
	};
	if (World::program()->info & Program::INF_UNIQ) {
		Text setup("Setup", lineHeight);
		Text port("Port:", lineHeight);
		top0.insert(top0.begin() + 1, new Label(setup.length, std::move(setup.text), &Program::eventOpenSetup));
		top0.insert(top0.end(), {
			new Label(port.length, std::move(port.text)),
			new LabelEdit(Text::strLen("00000", lineHeight) + LabelEdit::caretWidth, toStr(World::sets()->port), &Program::eventUpdatePort)
		});
	}

	vector<Widget*> topb = { new Layout(lineHeight, std::move(top0), false, lineSpacing) };
	vector<Widget*> menu = createConfigList(wio, World::program()->info & Program::INF_HOST);
	if (World::program()->info & Program::INF_HOST) {
		Text cfgt("Configuration: ", lineHeight);
		Text aleft(arrowLeft, lineHeight);
		Text aright(arrowRight, lineHeight);
		Text copy("Copy", lineHeight);
		Text newc("New", lineHeight);
		vector<Widget*> top1 = {
			new Label(cfgt.length, cfgt.text),
			new Label(aleft.length, aleft.text, &Program::eventSBPrev),
			new SwitchBox(1.f, sortNames(confs), World::game()->configName, &Program::eventSwitchConfig, Texture(), Label::Alignment::center),
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
	RootLayout* root = new RootLayout(1.f, std::move(cont), true, superSpacing, RootLayout::uniformBgColor);
	updateStartButton();
	return root;
}

void ProgRoom::updateStartButton() {
	if (World::program()->info & Program::INF_UNIQ) {
		startButton->setText("Open");
		startButton->setLcall(&Program::eventHostServer);
	} else if (World::program()->info & Program::INF_HOST) {
		startButton->setText(World::program()->info & Program::INF_GUEST_WAITING ? "Start" : "Waiting for player...");
		startButton->setLcall(World::program()->info & Program::INF_GUEST_WAITING ? &Program::eventStartGame : nullptr);
	} else
		startButton->setText("Waiting for start...");
}

void ProgRoom::updateConfigWidgets() {
	wio.width->setText(toStr(World::game()->config.homeSize.x));
	wio.height->setText(toStr(World::game()->config.homeSize.y));
	if (World::program()->info & Program::INF_HOST)
		wio.survivalSL->setVal(World::game()->config.survivalPass);
	wio.survivalLE->setText(toStr(World::game()->config.survivalPass) + '%');
	wio.favors->setText(toStr(World::game()->config.favorLimit));
	wio.dragonDist->setText(toStr(World::game()->config.dragonDist));
	wio.dragonDiag->on = World::game()->config.dragonDiag;
	wio.multistage->on = World::game()->config.multistage;
	wio.survivalKill->on = World::game()->config.survivalKill;
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		wio.tiles[i]->setText(toStr(World::game()->config.tileAmounts[i]));
	wio.tileFortress->setText(tileFortressString(World::game()->config));
	for (uint8 i = 0; i < Com::tileMax - 1; i++)
		wio.middles[i]->setText(toStr(World::game()->config.middleAmounts[i]));
	wio.middleFortress->setText(middleFortressString(World::game()->config));
	for (uint8 i = 0; i < Com::pieceMax; i++)
		wio.pieces[i]->setText(toStr(World::game()->config.pieceAmounts[i]));
	wio.pieceTotal->setText(pieceTotalString(World::game()->config));
	wio.winFortress->setText(toStr(World::game()->config.winFortress));
	wio.winThrone->setText(toStr(World::game()->config.winThrone));
	wio.capturers->setText(World::game()->config.capturersString());
	wio.shiftLeft->on = World::game()->config.shiftLeft;
	wio.shiftNear->on = World::game()->config.shiftNear;
}

// PROG SETUP

ProgSetup::ProgSetup() :
	ProgState(),
	enemyReady(false),
	selected(0),
	lastButton(0),
	lastHold(INT16_MIN)
{}

void ProgSetup::eventEscape() {
	stage > Stage::tiles ? World::program()->eventSetupBack() : World::program()->eventAbortGame();
}

void ProgSetup::eventNumpress(uint8 num) {
	if (num < counters.size())
		setSelected(num);
}

void ProgSetup::eventWheel(int ymov) {
	setSelected(cycle(selected, uint8(counters.size()), int8(ymov)));
}

void ProgSetup::eventDrag(uint32 mStat) {
	if (getDeleteLock())
		mStat = swapBits(mStat, SDL_BUTTON_LEFT - 1, SDL_BUTTON_RIGHT - 1);

	uint8 curButton = mStat & SDL_BUTTON_LMASK ? SDL_BUTTON_LEFT : mStat & SDL_BUTTON_RMASK ? SDL_BUTTON_RIGHT : 0;
	BoardObject* bo = dynamic_cast<BoardObject*>(World::scene()->select);
	svec2 curHold = bo ? World::game()->ptog(bo->getPos()) : svec2(INT16_MIN);
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
	lastHold = svec2(INT16_MIN);
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
	World::scene()->resetLayouts();

	selected = 0;	// like setSelected but without crashing
	static_cast<Draglet*>(icons->getWidget(selected + 1))->selected = true;
	for (uint8 i = 0; i < counters.size(); i++)
		switchIcon(i, counters[i], stage != Stage::pieces);
	return false;
}

void ProgSetup::setInteractivity(bool dlock) {
	switch (stage) {
	case Stage::tiles:
		World::game()->setOwnTilesInteract(dlock ? Tile::Interact::recognize : Tile::Interact::interact);
		World::game()->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->setOwnPiecesVisible(false);
		break;
	case Stage::middles:
		World::game()->setOwnTilesInteract(Tile::Interact::ignore, true);
		World::game()->setMidTilesInteract(dlock ? Tile::Interact::recognize : Tile::Interact::interact);
		World::game()->setOwnPiecesVisible(false);
		break;
	case Stage::pieces:
		World::game()->setOwnTilesInteract(Tile::Interact::recognize);
		World::game()->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->setOwnPiecesVisible(true, !dlock);
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
		"Save",
		"Load",
		"Config",
		"Exit",
	};
	if (World::netcp() || stage < Stage::pieces)
		sidt.emplace_back("Next");
	if (stage > Stage::tiles)
		sidt.emplace_back("Back");
	std::reverse(sidt.begin(), sidt.end());
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);

	vector<Widget*> wgts = {
		new Label(lineHeight, popBack(sidt), &Program::eventOpenSetupSave),
		new Label(lineHeight, popBack(sidt), &Program::eventOpenSetupLoad),
		new Label(lineHeight, popBack(sidt), &Program::eventShowConfig),
		deleteIcon = new Draglet(lineHeight, &Program::eventSwitchSetupButtons, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, Texture(), "Delete", Label::Alignment::left, false),
		new Label(lineHeight, popBack(sidt), &Program::eventAbortGame)
	};
	if (World::netcp() || stage < Stage::pieces)
		wgts.insert(wgts.end() - 1, new Label(lineHeight, popBack(sidt), &Program::eventSetupNext));
	if (stage > Stage::tiles)
		wgts.insert(wgts.end() - 1, new Label(lineHeight, popBack(sidt), &Program::eventSetupBack));

	// center piece
	vector<Widget*> midl = {
		new Widget(1.f),
		message = new Label(superHeight, string(), nullptr, nullptr, Texture(), Label::Alignment::center, false),
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
	ico->setDim(on ? 1.f : 0.5f);
	ico->setLcall(on ? isTile ? &Program::eventPlaceTileD : &Program::eventPlacePieceD : nullptr);
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
		saves[i] = new Label(lineHeight, std::move(names[i]), save ? &Program::eventSetupSave : &Program::eventSetupLoad);

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

void ProgMatch::eventWheel(int ymov) {
	World::scene()->getCamera()->zoom(ymov);
}

void ProgMatch::eventFavorize(bool on) {
	selectFavorIcon(on);
	if (World::game()->favorState.use = on; Piece* pce = dynamic_cast<Piece*>(World::scene()->capture))
		World::program()->eventFavorStart(pce, pce->show ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);	// vaguely simulate what happens when holding down on a piece to refresh favor state and highlighted tiles (no need to take warhorse into account)
}

void ProgMatch::eventCameraReset() {
	World::scene()->getCamera()->setPos(Camera::posMatch, Camera::latMatch);
}

void ProgMatch::updateFavorIcon(bool on, uint8 cnt, uint8 tot) {
	if (favorIcon) {
		favorIcon->setLcall(on ? &Program::eventSwitchFavor : nullptr);
		favorIcon->setDim(on && cnt ? 1.f : 0.5f);
		favorIcon->setText("FF: " + toStr(cnt) + '/' + toStr(tot));
	}
}

void ProgMatch::updateTurnIcon(bool on) {
	turnIcon->setLcall(on ? &Program::eventEndTurn : nullptr);
	turnIcon->setDim(on ? 1.f : 0.5f);
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

void ProgMatch::decreaseDragonIcon() {
	if (!--unplacedDragons) {
		dragonIcon->getParent()->deleteWidget(dragonIcon->getID());
		dragonIcon = nullptr;
	}
}

RootLayout* ProgMatch::createLayout() {
	// sidebar
	int sideLength = Text::strLen("FF: 00/00", lineHeight);
	vector<Widget*> left = {
		new Label(lineHeight, "Exit", &Program::eventAbortGame),
		new Label(lineHeight, "Config", &Program::eventShowConfig)
	};
	if (World::game()->config.pieceAmounts[uint8(Com::Piece::throne)])
		left.push_back(favorIcon = new Draglet(lineHeight, nullptr, nullptr, nullptr, World::scene()->blank(), Widget::colorNormal, Texture(), "FF: 0/0", Label::Alignment::left, false));	// text is updated after the icon has a parent
	else
		favorIcon = nullptr;
	left.push_back(turnIcon = new Label(lineHeight, "End turn"));
	
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
		message = new Label(superHeight, string(), nullptr, nullptr, Texture(), Label::Alignment::center, false),
		new Widget(lineSpacing * 2)
	};
	RootLayout* root = new RootLayout(1.f, std::move(cont), true, 0);
	updateTurnIcon(World::game()->getMyTurn());
	updateFavorIcon(World::game()->getMyTurn(), World::game()->getFavorCount(), World::game()->getFavorTotal());
	return root;
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	World::program()->eventOpenMainMenu();
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
#ifndef __ANDROID__
		"Display",
		"Screen",
		"Size",
		"Mode",
		"Multisamples",
#endif
		"VSync",
		"Gamma",
		"Volume"
	};
	vector<string> tips = {
#ifndef __ANDROID__
		"Keep 0 cause it doesn't really work",
		"Window or fullscreen (\"desktop\" is native resolution)",
		"Window size",
		"Fullscreen display properties",
		"Anti-Aliasing multisamples (requires restart)",
#endif
		"Brightness",
		"Audio volume",
		"Apply \"Display\", \"Screen\", \"Size\" and \"Mode\""
	};
	vector<string> vsyncTip = { "Immediate: off", "Synchronized: on", "Adaptive: on and smooth (works on fewer computers)" };
	std::reverse(txs.begin(), txs.end());
	std::reverse(tips.begin(), tips.end());
	sizet lnc = txs.size();
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);
	int slleLength = Text::strLen("000", lineHeight) + LabelEdit::caretWidth;

	vector<Widget*> lx[] = { {
#ifndef __ANDROID__
		new Label(descLength, popBack(txs)),
		display = new LabelEdit(1.f, toStr(World::sets()->display), nullptr, nullptr, &Program::eventDummy, makeTooltip(popBack(tips)))
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
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(tips.back())),
		new SwitchBox(1.f, { "0", "1", "2", "4" }, toStr(World::sets()->msamples), &Program::eventSetSamples, makeTooltip(tips.back())),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(popBack(tips)))
	}, {
#endif
		new Label(descLength, popBack(txs)),
		new Label(aleft.length, aleft.text, &Program::eventSBPrev, nullptr, makeTooltip(vsyncTip)),
		new SwitchBox(1.f, vector<string>(Settings::vsyncNames.begin(), Settings::vsyncNames.end()), Settings::vsyncNames[uint8(int8(World::sets()->vsync)+1)], &Program::eventSetVsync, makeTooltip(vsyncTip)),
		new Label(aright.length, aright.text, &Program::eventSBNext, nullptr, makeTooltip(vsyncTip))
	}, {
		new Label(descLength, popBack(txs)),
		new Slider(1.f, int(World::sets()->gamma * gammaStepFactor), 0, int(Settings::gammaMax * gammaStepFactor), &Program::eventSaveSettings, &Program::eventSetGammaSL, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->gamma), &Program::eventSetGammaLE, nullptr, nullptr, makeTooltip(popBack(tips)))
	}, World::audio() ? vector<Widget*>{
		new Label(descLength, popBack(txs)),
		new Slider(1.f, World::sets()->avolume, 0, SDL_MIX_MAXVOLUME, &Program::eventSaveSettings, &Program::eventSetVolumeSL, makeTooltip(tips.back())),
		new LabelEdit(slleLength, toStr(World::sets()->avolume), &Program::eventSetVolumeLE, nullptr, nullptr, makeTooltip(popBack(tips)))
	} : vector<Widget*>{
		new Label(descLength, popBack(txs)),
		new Label(1.f, "missing audio device")
	}, };
#ifdef __ANDROID__
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

#ifndef __APPLE__
	GLint glverm, glvern;
	glGetIntegerv(GL_MAJOR_VERSION, &glverm);
	glGetIntegerv(GL_MINOR_VERSION, &glvern);
#endif
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
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, toStr(glverm) + '.' + toStr(glvern)) }, false, lineSpacing),
#ifndef OPENGLES
		new Layout(lineHeight, { new Label(width, popBack(args)), new Label(1.f, reinterpret_cast<const char*>(glewGetString(GLEW_VERSION))) }, false, lineSpacing),
#endif
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
