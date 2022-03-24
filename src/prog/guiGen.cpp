#include "board.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"

template <class T>
int GuiGen::txtMaxLen(T pos, T end, float hfac) {
	int height = int(hfac * float(World::window()->getScreenView().y));
	int width = 0;
	for (; pos != end; ++pos)
		if (int len = Label::txtLen(*pos, height); len > width)
			width = len;
	return width;
}

int GuiGen::txtMaxLen(const initlist<initlist<const char*>>& lists, float hfac) {
	int width = 0;
	for (initlist<const char*> it : lists)
		if (int len = txtMaxLen(it.begin(), it.end(), hfac); len > width)
			width = len;
	return width;
}

void GuiGen::initSizes() {
	assignSizeFunc(SizeRef::scrollBarWidth, []() -> int { return int(scrollBarWidth * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::caretWidth, []() -> int { return int(caretWidth * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::popupMargin, []() -> int { return int(popupMargin * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::smallHeight, []() -> int { return int(smallHeight * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::lineHeight, []() -> int { return int(lineHeight * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::superHeight, []() -> int { return int(superHeight * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::iconSize, []() -> int { return int(iconSize * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::lineSpacing, []() -> int { return int(lineSpacing * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::superSpacing, []() -> int { return int(superSpacing * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::tooltipHeight, []() -> int { return int(tooltipHeight * float(World::window()->getScreenView().y)); });
	assignSizeFunc(SizeRef::tooltipLimit, []() -> int { return int(tooltipLimit * float(World::window()->getScreenView().x)); });
	assignSizeFunc(SizeRef::chatEmbed, []() -> int { return int(chatEmbed * float(World::window()->getScreenView().x)); });
}

void GuiGen::calsSizes() {
	for (auto& [size, func] : sizes)
		size = func();
}

void GuiGen::openPopupMessage(string msg, BCall ccal, string&& ctxt) const {
	vector<Widget*> bot = {
		new Widget(),
		new Label(std::move(ctxt), ccal, nullptr, string(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg), nullptr, nullptr, string(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};

	Size width([](const Widget* self) -> int { return Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight); });
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 2.f + lineSpacing), std::move(con), ccal, ccal, true, getSize(SizeRef::lineSpacing)));
}

void GuiGen::openPopupChoice(string&& msg, BCall kcal, BCall ccal) const {
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, string(), 1.f, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, string(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg), nullptr, nullptr, string(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, getSize(SizeRef::lineSpacing))
	};

	Size width([](const Widget* self) -> int { return Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight); });
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 2.f + lineSpacing), std::move(con), kcal, ccal, true, getSize(SizeRef::lineSpacing)));
}

void GuiGen::openPopupInput(string&& msg, string text, BCall kcal, uint16 limit) const {
	LabelEdit* ledit = new LabelEdit(1.f, std::move(text), nullptr, nullptr, kcal, &Program::eventClosePopup, string(), 1.f, limit);
	vector<Widget*> bot = {
		new Label(1.f, "Okay", kcal, nullptr, string(), 1.f, Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, string(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg)),
		ledit,
		new Layout(1.f, std::move(bot), false, 0)
	};

	Size width([](const Widget* self) -> int {
		ivec2 res = World::window()->getScreenView();
		return std::max(res.x - int(lineSpacing * 2.f * float(res.y)), Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight));
	});
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 3.f + lineSpacing * 2.f), std::move(con), kcal, &Program::eventClosePopup, true, getSize(SizeRef::lineSpacing)), ledit);
}

void GuiGen::makeTitleBar() const {
	if (World::window()->getTitleBarHeight()) {
		int width = World::window()->getTitleBarHeight();
		vector<Widget*> con = {
			new Button(width, nullptr, nullptr, "Settings", GuiGen::defaultDim, World::scene()->wgtTex("wrench"), Widget::colorLight),
			new Button(width, nullptr, nullptr, "Config", GuiGen::defaultDim, World::scene()->wgtTex("cog"), Widget::colorLight),
			new Label(1.f, string("Thrones v") + Com::commonVersion, nullptr, nullptr, string(), 1.f, Label::Alignment::center, false),
			new Label(width, "_", &Program::eventMinimize, nullptr, string(), 1.f, Label::Alignment::center),
			new Label(width, "X", &Program::eventExit, nullptr, string(), 1.f, Label::Alignment::center)
		};
		World::scene()->setTitleBar(std::make_unique<TitleBar>(std::move(con), false, getSize(SizeRef::lineSpacing)));
	} else
		World::scene()->setTitleBar(nullptr);
}

vector<Widget*> GuiGen::createChat(TextBox*& chatBox, bool overlay) const {
	return {
		chatBox = new TextBox(1.f, getSize(SizeRef::smallHeight), string(), &Program::eventFocusChatLabel, &Program::eventClearLabel, string(), 1.f, World::sets()->chatLines, false, true, 0, Widget::colorDimmed),
		new LabelEdit(getSize(SizeRef::smallHeight), string(), nullptr, nullptr, &Program::eventSendMessage, overlay ? &Program::eventCloseChat : &Program::eventHideChat, string(), 1.f, chatCharLimit, true)
	};
}

Overlay* GuiGen::createNotification(Overlay*& notification) const {
	Size posy([](const Widget* self) -> int { return World::window()->getScreenView().x - Label::txtLen(static_cast<const Overlay*>(self)->getWidget<Label>(0)->getText(), superHeight); });
	Size width([](const Widget* self) -> int { return Label::txtLen(static_cast<const Overlay*>(self)->getWidget<Label>(0)->getText(), superHeight); });
	Label* msg = new Label(1.f, "<i>", &Program::eventToggleChat, nullptr, "New message", 1.f, Label::Alignment::left, true, 0, Widget::colorDark);
	return notification = new Overlay(pair(posy, 0.f), pair(width, getSize(SizeRef::superHeight)), { msg }, nullptr, nullptr, true, false, true, getSize(SizeRef::lineSpacing), 0, vec4(0.f));
}

Overlay* GuiGen::createFpsCounter(Label*& fpsText) const {
	Size width([](const Widget* self) -> int { return Label::txtLen(static_cast<const Overlay*>(self)->getWidget<Label>(0)->getText(), lineHeight); });
	fpsText = new Label(1.f, makeFpsText(World::window()->getDeltaSec()), nullptr, nullptr, string(), 1.f, Label::Alignment::left, false);
	return new Overlay(pair(0, 1.f - lineHeight), pair(width, getSize(SizeRef::lineHeight)), { fpsText }, nullptr, nullptr, true, World::program()->ftimeMode != Program::FrameTime::none, false, getSize(SizeRef::lineSpacing), 0, Widget::colorDark);
}

string GuiGen::makeFpsText(float dSec) const {
	switch (World::program()->ftimeMode) {
	case Program::FrameTime::frames:
		return toStr(uint(std::round(1.f / dSec)));
	case Program::FrameTime::seconds:
		return toStr(uint(dSec * 1000.f)) + " ms";
	}
	return string();
}

// MAIN MENU

uptr<RootLayout> GuiGen::makeMainMenu(Interactable*& selected, LabelEdit*& pname, Label*& versionNotif) {
	initSizes();
	assignSizeFunc(SizeRef::menuSideWidth, []() -> int { return Label::txtLen("Server:", superHeight); });
	assignSizeFunc(SizeRef::menuMenuWidth, [this]() -> int {
		return Label::txtLen("Server:", superHeight) + Label::txtLen("000.000.000.000", superHeight) + *getSize(SizeRef::lineSpacing);
	});
	assignSizeFunc(SizeRef::menuTitleWidth, [this]() -> int {
		ivec2 tres = World::scene()->wgtTex("title").rct.size();
		return tres.y * (Label::txtLen("Server:", superHeight) + Label::txtLen("000.000.000.000", superHeight) + *getSize(SizeRef::lineSpacing)) / tres.x;
	});

	// server input
	vector<Widget*> srv = {
		new Label(getSize(SizeRef::menuSideWidth), "Server:"),
		new LabelEdit(1.f, World::sets()->address, &Program::eventUpdateAddress, &Program::eventResetAddress)
	};

	// port input and connect button
	vector<Widget*> prt = {
		new Label(getSize(SizeRef::menuSideWidth), "Port:"),
		new LabelEdit(1.f, World::sets()->port, &Program::eventUpdatePort, &Program::eventResetPort)
	};

	vector<Widget*> pnm = {
		new Label(getSize(SizeRef::menuSideWidth), "Name:"),
		pname = new LabelEdit(1.f, World::sets()->playerName, &Program::eventUpdatePlayerName, &Program::eventResetPlayerName, nullptr, nullptr, string(), 1.f, Settings::playerNameLimit)
	};

	// net buttons
	vector<Widget*> con = {
		new Label(getSize(SizeRef::menuSideWidth), "Host", &Program::eventOpenHostMenu, nullptr, string(), 1.f, Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, string(), 1.f, Label::Alignment::center)
	};
	selected = con.back();

	// middle buttons
	vector<Widget*> buts = {
		new Widget(0.35f),
		new Button(getSize(SizeRef::menuTitleWidth), nullptr, nullptr, string(), 1.f, World::scene()->wgtTex("title"), vec4(1.f)),
		new Widget(0),
		new Layout(getSize(SizeRef::superHeight), std::move(srv), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::superHeight), std::move(prt), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::superHeight), std::move(pnm), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::superHeight), std::move(con), false, getSize(SizeRef::lineSpacing)),
		new Widget(0),
		new Label(getSize(SizeRef::superHeight), "Settings", &Program::eventOpenSettings, nullptr, string(), 1.f, Label::Alignment::center),
#ifndef __EMSCRIPTEN__
		new Label(getSize(SizeRef::superHeight), "Exit", &Program::eventExit, nullptr, string(), 1.f, Label::Alignment::center),
#endif
		new Widget(0.65f)
	};

	// menu and root layout
	vector<Widget*> menu = {
		new Widget(),
		new Layout(getSize(SizeRef::menuMenuWidth), std::move(buts), true, getSize(SizeRef::superSpacing)),
		new Widget()
	};
	vector<Widget*> vers = {
		versionNotif = new Label(1.f, World::program()->getLatestVersion(), nullptr, nullptr, string(), 1.f, Label::Alignment::left, false),
		new Label(Com::commonVersion, nullptr, nullptr, string(), 1.f, Label::Alignment::right, false)
	};
	vector<Widget*> cont = {
		new Layout(1.f, std::move(menu), false),
		new Layout(getSize(SizeRef::lineHeight), std::move(vers), false)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont));
}

// LOBBY

uptr<RootLayout> GuiGen::makeLobby(Interactable*& selected, TextBox*& chatBox, ScrollArea*& rooms, vector<pair<string, bool>>& roomBuff) {
	initlist<const char*> sidt = {
		"Back",
		"Host"
	};

	initSizes();
	assignSizeFunc(SizeRef::lobbySideWidth, [sidt]() -> int { return txtMaxLen(sidt.begin(), sidt.end(), lineHeight); });

	// side bar
	initlist<const char*>::iterator isidt = sidt.begin();
	vector<Widget*> lft = {
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventExitLobby),
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventHostRoomInput)
	};
	selected = lft.back();

	// room list
	vector<Widget*> lns(roomBuff.size());
	for (sizet i = 0; i < roomBuff.size(); ++i)
		lns[i] = createRoom(std::move(roomBuff[i].first), roomBuff[i].second);
	roomBuff.clear();

	// main layout
	vector<Widget*> cont = {
		new Layout(getSize(SizeRef::lobbySideWidth), std::move(lft), true, getSize(SizeRef::lineSpacing)),
		rooms = new ScrollArea(1.f, std::move(lns), true, getSize(SizeRef::lineSpacing))
	};

	// root layout
	vector<Widget*> root = {
		new Layout(1.f, std::move(cont), false, getSize(SizeRef::superSpacing)),
		new Layout(getSize(SizeRef::chatEmbed), { createChat(chatBox, false) })
	};
	return std::make_unique<RootLayout>(1.f, std::move(root), true, getSize(SizeRef::superSpacing), lineSpacing, RootLayout::uniformBgColor);
}

Label* GuiGen::createRoom(string&& name, bool open) const {
	return new Label(getSize(SizeRef::lineHeight), std::move(name), open ? &Program::eventJoinRoomRequest : nullptr, nullptr, string(), open ? 1.f : defaultDim);
}

// ROOM

uptr<RootLayout> GuiGen::makeRoom(Interactable*& selected, ConfigIO& wio, RoomIO& rio, TextBox*& chatBox, ComboBox*& configName, const umap<string, Config>& confs, const string& startConfig) {
	initSizes();
	assignSizeFunc(SizeRef::roomPortWidth, [this]() -> int { return Label::txtLen(string(numDigits(uint(UINT16_MAX)), '0'), lineHeight) + *getSize(GuiGen::SizeRef::caretWidth); });

	bool unique = World::program()->info & Program::INF_UNIQ, host = World::program()->info & Program::INF_HOST;
	vector<Widget*> top0;
	if (unique) {
#ifndef __EMSCRIPTEN__
		top0 = {
			new Label("Back", &Program::eventOpenMainMenu),
			new Label("Setup", &Program::eventOpenSetup),
			rio.start = new Label(1.f, "Open", &Program::eventHostServer, nullptr, string(), 1.f, Label::Alignment::center),
			new Label("Port:"),
			new LabelEdit(getSize(SizeRef::roomPortWidth), World::sets()->port, &Program::eventUpdatePort)
		};
#endif
	} else if (host) {
		top0 = {
			new Label("Back", &Program::eventExitRoom),
			rio.start = new Label(1.f, "Waiting for player...", nullptr, nullptr, string(), 1.f, Label::Alignment::center),
			rio.host = new Label("Host", nullptr, nullptr, string(), defaultDim),
			rio.kick = new Label("Kick", nullptr, nullptr, string(), defaultDim)
		};
	} else {
		top0 = {
			new Label("Back", &Program::eventExitRoom),
			rio.start = new Label(1.f, "Waiting for start...", nullptr, nullptr, string(), 1.f, Label::Alignment::center)
		};
	}
	selected = top0[0];

	vector<Widget*> menu = createConfigList(wio, host ? confs.at(startConfig) : World::game()->board->config, host, false);
	vector<Widget*> top1 = {
		new Label("Configuration:"),
		configName = new ComboBox(1.f, startConfig, host ? sortNames(confs) : vector<string>{ startConfig }, host ? &Program::eventSwitchConfig : nullptr),
		new Label("Copy", &Program::eventConfigCopyInput)
	};
	if (host) {
		bool delOn = confs.size() > 1;
		top1.insert(top1.end(), {
			new Label("New", &Program::eventConfigNewInput),
			rio.del = new Label("Del", delOn ? &Program::eventConfigDelete : nullptr, nullptr, string(), delOn ? 1.f : defaultDim)
		});

		vector<Widget*> lineR = {
			new Label("Reset", &Program::eventUpdateReset, nullptr, "Reset current configuration"),
			new Widget()
		};
		menu.push_back(new Layout(getSize(SizeRef::lineHeight), std::move(lineR), false, getSize(SizeRef::lineSpacing)));
	}

	vector<Widget*> cont = {
		new Layout(getSize(SizeRef::lineHeight), std::move(top0), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), std::move(top1), false, getSize(SizeRef::lineSpacing)),
		new Widget(0),
		new ScrollArea(1.f, std::move(menu), true, getSize(SizeRef::lineSpacing))
	};
	vector<Widget*> rwgt = {
		new Layout(1.f, std::move(cont), true, getSize(SizeRef::lineSpacing))
	};
	if (!unique)
		rwgt.push_back(new Layout(host ? Size(0) : getSize(SizeRef::chatEmbed), createChat(chatBox, false)));
	return std::make_unique<RootLayout>(1.f, std::move(rwgt), false, getSize(SizeRef::lineSpacing), lineSpacing, RootLayout::uniformBgColor);
}

vector<Widget*> GuiGen::createConfigList(ConfigIO& wio, const Config& cfg, bool active, bool match) {
	BCall update = active ? &Program::eventUpdateConfig : nullptr;
	BCall iupdate = active ? &Program::eventUpdateConfigI : nullptr;
	BCall vupdate = active ? &Program::eventUpdateConfigV : nullptr;
	initlist<const char*> txs = {
		"Victory points",
		"Ports",
		"Row balancing",
		"Homefront",
		"Set piece battle",
		"Homeland size",
		"Battle win chance",
		"Fate's favor maximum",
		"Fate's favor total",
		"First turn engage",
		"Terrain rules",
		"Dragon place late",
		"Dragon move straight",
		"Capturers",
		"Fortresses captured",
		"Thrones killed"
	};
	initlist<const char*>::iterator itxs = txs.begin();
	assignSizeFunc(SizeRef::configDescWidth, [txs]() -> int { return txtMaxLen(txs.begin(), txs.end(), lineHeight); });
	assignSizeFunc(SizeRef::configAmtWidth, [this]() -> int { return Label::txtLen(string(numDigits(uint(UINT16_MAX)), '0'), lineHeight) + *getSize(GuiGen::SizeRef::caretWidth); });

	string scTip = "Chance of breaching a " + string(tileNames[uint8(TileType::fortress)]);
	string tileTips[tileLim];
	for (uint8 i = 0; i < tileLim; ++i)
		tileTips[i] = "Number of " + string(tileNames[i]) + " tiles per homeland";
	constexpr char fortressTip[] = "(calculated automatically to fill the remaining free tiles)";
	string middleTips[tileLim];
	for (uint8 i = 0; i < tileLim; ++i)
		middleTips[i] = "Number of " + string(tileNames[i]) + " tiles in the middle row per player";
	string pieceTips[pieceLim];
	for (uint8 i = 0; i < pieceLim; ++i)
		pieceTips[i] = "Number of " + string(pieceNames[i]) + " pieces per player";
	constexpr char fortTip[] = "Number of fortresses that need to be captured in order to win";
	constexpr char throneTip[] = "Number of thrones that need to be killed in order to win";
	Size amtWidth = update ? getSize(SizeRef::configAmtWidth) : Size(1.f);

	vector<vector<Widget*>> lines0 = { {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.victoryPoints = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::victoryPoints, vupdate, vupdate, "Use the victory points game variant"),
		wio.victoryPointsNum = new LabelEdit(1.f, toStr(cfg.victoryPointsNum), update, nullptr, nullptr, nullptr, "Number of victory points required to win"),
		new Label("centered:"),
		wio.vpEquidistant = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::victoryPointsEquidistant, update, update, "Place middle row fortresses in the center")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.ports = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::ports, update, update, "Use the ports game variant")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.rowBalancing = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::rowBalancing, update, update, "Have at least one of each tile type in each row")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.homefront = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::homefront, update, update, "Use the homefront game variant")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.setPieceBattle = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::setPieceBattle, update, update, "Use the set piece battle game variant"),
		wio.setPieceBattleNum = new LabelEdit(1.f, toStr(cfg.setPieceBattleNum), update, nullptr, nullptr, nullptr, "Number of pieces per player to be chosen"),
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		new Label("width:"),
		wio.width = new LabelEdit(1.f, toStr(cfg.homeSize.x), update, nullptr, nullptr, nullptr, "Number of the board's columns of tiles"),
		new Label("height:"),
		wio.height = new LabelEdit(1.f, toStr(cfg.homeSize.y), update, nullptr, nullptr, nullptr, "Number of the board's rows of tiles minus one divided by two")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.battleLE = new LabelEdit(amtWidth, toStr(cfg.battlePass) + '%', update, nullptr, nullptr, nullptr, string(scTip))
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.favorLimit = new LabelEdit(1.f, toStr(cfg.favorLimit), update, nullptr, nullptr, nullptr, "Maximum amount of fate's favors per type")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.favorTotal = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::favorTotal, update, update, "Limit the total amount of fate's favors for the entire match")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.firstTurnEngage = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::firstTurnEngage, update, update, "Allow attacking and firing at pieces during each player's first turn")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.terrainRules = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::terrainRules, update, update, "Use terrain related rules")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.dragonLate = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::dragonLate, update, update, firstUpper(pieceNames[uint8(PieceType::dragon)]) + " can be placed later during the match on a homeland " + tileNames[uint8(TileType::fortress)])
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.dragonStraight = new CheckBox(getSize(SizeRef::lineHeight), cfg.opts & Config::dragonStraight, update, update, firstUpper(pieceNames[uint8(PieceType::dragon)]) + " moves in a straight line")
	}, {
		new Label(getSize(SizeRef::configDescWidth), *itxs++)
	} };
	lines0.back().resize(pieceLim + 1);
	for (uint8 i = 0; i < pieceLim; ++i)
		lines0.back()[i+1] = wio.capturers[i] = new Icon(getSize(SizeRef::lineHeight), string(), iupdate, iupdate, nullptr, firstUpper(pieceNames[i]) + " can capture fortresses", 1.f, Label::Alignment::left, true, World::scene()->wgtTex(pieceNames[i]), vec4(1.f), cfg.capturers & (1 << i));
	if (active)
		lines0[6].insert(lines0[6].begin() + 1, wio.battleSL = new Slider(1.f, cfg.battlePass, 0, Config::randomLimit, 10, update, &Program::eventPrcSliderUpdate, std::move(scTip)));

	vector<vector<Widget*>> lines1(tileLim + 2);
	for (uint8 i = 0; i < tileLim; ++i)
		lines1[i] = {
			new Label(getSize(SizeRef::configDescWidth), firstUpper(tileNames[i])),
			wio.tiles[i] = new LabelEdit(amtWidth, toStr(cfg.tileAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? string(tileTips[i]) : tileTips[i] + '\n' + fortressTip)
		};
	lines1[tileLim] = {
		new Label(getSize(SizeRef::configDescWidth), firstUpper(tileNames[uint8(TileType::fortress)])),
		wio.tileFortress = new Label(1.f, tileFortressString(cfg), nullptr, nullptr, "Number of " + string(tileNames[uint8(TileType::fortress)]) + " tiles per homeland\n" + fortressTip)
	};
	lines1[tileLim+1] = {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.winFortress = new LabelEdit(amtWidth, toStr(cfg.winFortress), update, nullptr, nullptr, nullptr, fortTip)
	};
	if (active) {
		uint16 rest = cfg.countFreeTiles();
		for (uint8 i = 0; i < tileLim; ++i)
			lines1[i].insert(lines1[i].begin() + 1, new Slider(1.f, cfg.tileAmounts[i], cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, cfg.tileAmounts[i] + rest, 1, update, &Program::eventTileSliderUpdate, string(tileTips[i])));
		lines1[tileLim+1].insert(lines1[tileLim+1].begin() + 1, new Slider(1.f, cfg.winFortress, 0, cfg.countFreeTiles(), 1, update, &Program::eventSLUpdateLE, fortTip));
	}

	vector<vector<Widget*>> lines2(tileLim + 1);
	for (uint8 i = 0; i < tileLim; ++i)
		lines2[i] = {
			new Label(getSize(SizeRef::configDescWidth), firstUpper(tileNames[i])),
			wio.middles[i] = new LabelEdit(amtWidth, toStr(cfg.middleAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? string(middleTips[i]) : tileTips[i] + '\n' + fortressTip)
	};
	lines2.back() = {
		new Label(getSize(SizeRef::configDescWidth), firstUpper(tileNames[uint8(TileType::fortress)])),
		wio.middleFortress = new Label(1.f, middleFortressString(cfg), nullptr, nullptr, "Number of " + string(tileNames[uint8(TileType::fortress)]) + " tiles in the middle row\n" + fortressTip)
	};
	if (active) {
		uint16 rest = cfg.countFreeMiddles();
		for (uint8 i = 0; i < tileLim; ++i)
			lines2[i].insert(lines2[i].begin() + 1, new Slider(1.f, cfg.middleAmounts[i], 0, cfg.middleAmounts[i] + rest, 1, update, &Program::eventMiddleSliderUpdate, std::move(middleTips[i])));
	}

	vector<vector<Widget*>> lines3(pieceLim + 2);
	for (uint8 i = 0; i < pieceLim; ++i)
		lines3[i] = {
			new Label(getSize(SizeRef::configDescWidth), firstUpper(pieceNames[i])),
			wio.pieces[i] = new LabelEdit(amtWidth, toStr(cfg.pieceAmounts[i]), update, nullptr, nullptr, nullptr, string(pieceTips[i]))
		};
	lines3[pieceLim] = {
		new Label(getSize(SizeRef::configDescWidth), "Total"),
		wio.pieceTotal = new Label(1.f, pieceTotalString(cfg), nullptr, nullptr, "Total amount of pieces out of the maximum possible number of pieces per player")
	};
	lines3.back() = {
		new Label(getSize(SizeRef::configDescWidth), *itxs++),
		wio.winThrone = new LabelEdit(amtWidth, toStr(cfg.winThrone), update, nullptr, nullptr, nullptr, throneTip)
	};
	if (active) {
		uint16 rest = cfg.countFreePieces();
		for (uint8 i = 0; i < pieceLim; ++i)
			lines3[i].insert(lines3[i].begin() + 1, new Slider(1.f, cfg.pieceAmounts[i], 0, cfg.pieceAmounts[i] + rest, 1, update, &Program::eventPieceSliderUpdate, std::move(pieceTips[i])));
		lines3.back().insert(lines3.back().begin() + 1, new Slider(1.f, cfg.winThrone, 0, cfg.pieceAmounts[uint8(PieceType::throne)], 1, update, &Program::eventSLUpdateLE, throneTip));
	} else if (World::netcp() && (World::game()->board->config.opts & Config::setPieceBattle) && World::game()->board->config.setPieceBattleNum < World::game()->board->config.countPieces())
		for (uint8 i = 0; i < pieceLim; ++i)
			if (lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board->ownPieceAmts[i]), nullptr, nullptr, "Number of ally " + string(pieceNames[i]) + " pieces")); match)
				lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board->enePieceAmts[i]), nullptr, nullptr, "Number of enemy " + string(pieceNames[i]) + " pieces"));

	sizet id = 0;
	vector<Widget*> menu(lines0.size() + lines1.size() + lines2.size() + lines3.size() + 3);	// 3 title bars
	setConfigLines(menu, lines0, id);
	setConfigTitle(menu, "Homeland tiles", id);
	setConfigLines(menu, lines1, id);
	setConfigTitle(menu, "Middle tiles", id);
	setConfigLines(menu, lines2, id);
	setConfigTitle(menu, "Pieces", id);
	setConfigLines(menu, lines3, id);
	return menu;
}

void GuiGen::setConfigLines(vector<Widget*>& menu, vector<vector<Widget*>>& lines, sizet& id) const {
	for (vector<Widget*>& it : lines)
		menu[id++] = new Layout(getSize(SizeRef::lineHeight), std::move(it), false, getSize(SizeRef::lineSpacing));
}

void GuiGen::setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const {
	vector<Widget*> line = {
		new Label(std::move(title)),
		new Widget()
	};
	menu[id++] = new Layout(getSize(SizeRef::lineHeight), std::move(line), false, getSize(SizeRef::lineSpacing));
}

// SETUP

uptr<RootLayout> GuiGen::makeSetup(Interactable*& selected, SetupIO& sio, Icon*& bswapIcon, Navigator*& planeSwitch) {
	initlist<const char*> sidt = {
		"Exit",
		"Save",
		"Load",
		"Delete",
		"Back",
		"Chat",
		"Settings",
		"Config",
		"Finish"	// width placeholder just in case for when setting nextIcon's text later
	};
	initlist<const char*>::iterator isidt = sidt.begin();

	initSizes();
	assignSizeFunc(SizeRef::setupSideWidth, [sidt]() -> int { return txtMaxLen(sidt.begin(), sidt.end(), lineHeight); });

	// sidebar
	vector<Widget*> wgts = {
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventAbortGame, nullptr, "Exit the game"),
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventOpenSetupSave, nullptr, "Save current setup"),
		sio.load = new Label(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, "Load a setup"),
		bswapIcon = new Icon(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, nullptr, "Switch between placing and deleting a tile/piece"),
		sio.back = new Label(getSize(SizeRef::lineHeight), *isidt++),
		sio.next = new Label(getSize(SizeRef::lineHeight))
	};
	selected = wgts.back();
	if (World::netcp())
		wgts.insert(wgts.begin() + 3, new Label(getSize(SizeRef::lineHeight), *isidt, &Program::eventToggleChat, nullptr, "Toggle chat"));
	++isidt;
	if (!World::window()->getTitleBarHeight()) {
		wgts.insert(wgts.begin() + 1, {
			new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventShowSettings, nullptr, "Open settings"),
			new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventShowConfig, nullptr, "Show current game configuration"),
		});
	}

	vector<Widget*> side = {
		new Layout(getSize(SizeRef::setupSideWidth), std::move(wgts), true, getSize(SizeRef::lineSpacing)),
		planeSwitch = new Navigator()
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(1.f, std::move(side), false),
		sio.icons = new Layout(getSize(SizeRef::iconSize), {}, false, getSize(SizeRef::lineSpacing)),
		new Widget(getSize(SizeRef::superSpacing))
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont), true, 0, lineSpacing);
}

vector<Widget*> GuiGen::createBottomIcons(bool tiles) const {
	uint8 size = tiles ? tileLim : pieceLim;
	const char* const* names = tiles ? tileNames.data() : pieceNames.data();
	vector<Widget*> ibot(size + 2);
	ibot.front() = new Widget();
	for (uint8 i = 0; i < size; ++i)
		ibot[1+i] = new Icon(getSize(SizeRef::iconSize), string(), &Program::eventIconSelect, nullptr, nullptr, firstUpper(names[i]), 1.f, Label::Alignment::center, true, World::scene()->wgtTex(names[i]), vec4(1.f));
	ibot.back() = new Widget();
	return ibot;
}

Overlay* GuiGen::createGameMessage(Label*& message, bool setup) const {
	message = new Label(1.f, string(), nullptr, nullptr, string(), 1.f, Label::Alignment::center, false);
	return new Overlay(setup ? pair(0.1f, 0.08f) : pair(0.f, 0.92f), pair(setup ? 0.8f : 1.f, getSize(SizeRef::superHeight)), { message }, nullptr, nullptr, true, true, false, getSize(SizeRef::lineSpacing));
}

Overlay* GuiGen::createGameChat(TextBox*& chatBox) const {
	return new Overlay(pair(0.7f, 0.f), pair(0.3f, 1.f), createChat(chatBox, true), &Program::eventChatOpen, &Program::eventChatClose, true, false, true, getSize(SizeRef::lineSpacing));
}

void GuiGen::openPopupFavorPick(uint16 availableFF) const {
	Widget* defSel = nullptr;
	vector<Widget*> bot(favorMax + 2);
	bot.front() = new Widget();
	for (uint8 i = 0; i < favorMax; ++i) {
		bool on = World::game()->favorsCount[i] < World::game()->favorsLeft[i];
		bot[1+i] = new Button(getSize(SizeRef::superHeight), on ? &Program::eventPickFavor : nullptr, nullptr, firstUpper(favorNames[i]), on ? 1.f : defaultDim, World::scene()->wgtTex(favorNames[i]), vec4(1.f));
		if (on && !defSel)
			defSel = bot[1+i];
	}
	bot.back() = new Widget();

	Size width([](const Widget* self) -> int {
		return std::max(Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight), int((float(favorMax) * superHeight + float(favorMax + 1) * lineSpacing) * float(World::window()->getScreenView().y)));
	});
	vector<Widget*> con = {
		new Label(1.f, msgFavorPick + (availableFF > 1 ? " (" + toStr(availableFF) + ')' : string()), nullptr, nullptr, string(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, getSize(SizeRef::lineSpacing))
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 2.f + lineSpacing), std::move(con), nullptr, nullptr, true, getSize(SizeRef::lineSpacing), 0.f, defSel));
}

void GuiGen::openPopupConfig(const string& configName, const Config& cfg, ScrollArea*& configList, bool match) {
	ConfigIO wio;
	vector<Widget*> top = {
		new Label(1.f, configName, nullptr, nullptr, string(), 1.f, Label::Alignment::center),
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		new Label("Rules", &Program::eventOpenRules, nullptr, string(), 1.f, Label::Alignment::center),
		new Label("Docs", &Program::eventOpenDocs, nullptr, string(), 1.f, Label::Alignment::center)
#endif
	};
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, string(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Layout(getSize(SizeRef::lineHeight), std::move(top), false, getSize(SizeRef::lineSpacing)),
		configList = new ScrollArea(1.f, createConfigList(wio, cfg, false, match), true, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), std::move(bot), false)
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventCloseScrollingPopup, true, getSize(SizeRef::lineSpacing), 0.f, nullptr, Popup::Type::config));
}

void GuiGen::openPopupSaveLoad(const umap<string, Setup>& setups, bool save) const {
	Label* blbl = new Label("Back", &Program::eventClosePopup);
	vector<Widget*> top = {
		blbl
	};
	if (save)
		top.insert(top.end(), {
			new LabelEdit(1.f, string(), nullptr, nullptr, &Program::eventSetupNew),
			new Label("New", &Program::eventSetupNew)
		});

	vector<string> names = sortNames(setups);
	vector<Widget*> saves(setups.size());
	for (sizet i = 0; i < setups.size(); ++i)
		saves[i] = new Label(getSize(SizeRef::lineHeight), std::move(names[i]), save ? &Program::eventSetupSave : &Program::eventSetupLoad, &Program::eventSetupDelete);

	vector<Widget*> con = {
		new Layout(getSize(SizeRef::lineHeight), std::move(top), false, getSize(SizeRef::lineSpacing)),
		new ScrollArea(1.f, std::move(saves), true, getSize(SizeRef::lineSpacing))
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventCloseScrollingPopup, true, getSize(SizeRef::superSpacing), 0.f, blbl));
}

void GuiGen::openPopupPiecePicker(uint16 piecePicksLeft) const {
	Widget* defSel = nullptr;
	vector<Widget*> bot[2] = { vector<Widget*>(5), vector<Widget*>(5) };
	for (uint8 r = 0, t = 0; r < 2; ++r)
		for (uint8 i = 0; i < 5; ++i, ++t) {
			bool more = World::game()->board->ownPieceAmts[t] < World::game()->board->config.pieceAmounts[t];
			bot[r][i] = new Button(getSize(SizeRef::superHeight), more ? &Program::eventSetupPickPiece : nullptr, nullptr, firstUpper(pieceNames[t]), more ? 1.f : defaultDim, World::scene()->wgtTex(pieceNames[t]), vec4(1.f));
			if (more && !defSel)
				defSel = bot[r][i];
		}

	vector<Widget*> mid = {
		new Layout(getSize(SizeRef::superHeight), std::move(bot[0]), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::superHeight), std::move(bot[1]), false, getSize(SizeRef::lineSpacing))
	};
	vector<Widget*> cont = {
		new Widget(),
		new Layout(Size(superHeight * 5.f + lineSpacing * 4.f, Size::abso), std::move(mid), true, getSize(SizeRef::lineSpacing)),
		new Widget()
	};

	Size width([](const Widget* self) -> int {
		return std::max(Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight), int((superHeight * 5.f + lineSpacing * 4.f) * float(World::window()->getScreenView().y)));
	});
	vector<Widget*> con = {
		new Label(getSize(SizeRef::superHeight), msgPickPiece + toStr(piecePicksLeft) + ')', nullptr, nullptr, string(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(cont), false, 0)
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 3.f + lineSpacing * 2.f), std::move(con), nullptr, nullptr, true, getSize(SizeRef::lineSpacing), 0.f, defSel));
}

// MATCH

uptr<RootLayout> GuiGen::makeMatch(Interactable*& selected, MatchIO& mio, Icon*& bswapIcon, Navigator*& planeSwitch, uint16& unplacedDragons) {
	initlist<const char*> sidt = {
		"Exit",
		"Surrender",
		"Chat",
		"Engage",
		"Destroy",
		"Finish",
		"Settings",
		"Config",
		"Establish",
		"Rebuild",
		"Spawn"
	};
	initlist<const char*>::iterator isidt = sidt.begin();

	initSizes();
	assignSizeFunc(SizeRef::matchSideWidth, [sidt]() -> int {
		return std::max(txtMaxLen(sidt.begin(), sidt.end(), lineHeight), int((iconSize * 2.f + lineSpacing * 2.f) * float(World::window()->getScreenView().y)));	// second argument is length of FF line
	});
	assignSizeFunc(SizeRef::matchPointsLen, []() -> int { return Label::txtLen(string(numDigits(World::game()->board->config.victoryPointsNum), '0'), lineHeight); });

	// sidebar
	vector<Widget*> left = {
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventAbortGame, nullptr, "Exit the game"),
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventSurrender, nullptr, "Surrender and quit the match"),
		new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventToggleChat, nullptr, "Toggle chat"),
		bswapIcon = new Icon(getSize(SizeRef::lineHeight), *isidt++, &Program::eventSwitchGameButtons, nullptr, nullptr, "Switch between engaging and moving a piece"),
		mio.destroy = new Icon(getSize(SizeRef::lineHeight), *isidt++, &Program::eventSwitchDestroy, nullptr, nullptr, "Destroy a tile when moving off of it"),
		mio.turn = new Label(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, "Finish current turn")
	};
	selected = mio.turn;
	if (!World::window()->getTitleBarHeight()) {
		left.insert(left.begin() + 2, {
			new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventShowSettings, nullptr, "Open settings"),
			new Label(getSize(SizeRef::lineHeight), *isidt++, &Program::eventShowConfig, nullptr, "Show current game configuration"),
		});
	} else
		isidt += 2;

	if (World::game()->board->config.opts & Config::homefront) {
		left.insert(left.end() - 1, {
			mio.establish = new Icon(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, nullptr, "Respawn a piece"),
			mio.rebuild = new Icon(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, nullptr, "Rebuild a fortress or farm"),
			mio.spawn = new Icon(getSize(SizeRef::lineHeight), *isidt++, nullptr, nullptr, nullptr, "Establish a farm or city")
		});
	} else
		mio.rebuild = mio.establish = mio.spawn = nullptr;
	if (World::game()->board->config.favorLimit && World::game()->board->ownPieceAmts[uint8(PieceType::throne)]) {
		for (sizet i = 0; i < mio.favors.size(); ++i)
			mio.favors[i] = new Icon(getSize(SizeRef::iconSize), string(), nullptr, nullptr, nullptr, firstUpper(favorNames[i]), 1.f, Label::Alignment::center, true, World::scene()->wgtTex(favorNames[i]), vec4(1.f));
		left.push_back(new Widget(0));
		for (sizet i = 0; i < mio.favors.size(); i += 2)
			left.push_back(new Layout(getSize(SizeRef::iconSize), { new Widget(0), mio.favors[i], mio.favors[i+1] }, false, getSize(SizeRef::lineSpacing)));
	} else
		std::fill(mio.favors.begin(), mio.favors.end(), nullptr);

	unplacedDragons = 0;
	for (Piece* pce = World::game()->board->getOwnPieces(PieceType::dragon); pce->getType() == PieceType::dragon; ++pce)
		if (!pce->getShow())
			++unplacedDragons;
	if (unplacedDragons) {
		mio.dragon = new Icon(getSize(SizeRef::iconSize), string(), nullptr, nullptr, nullptr, string("Place ") + pieceNames[uint8(PieceType::dragon)], 1.f, Label::Alignment::left, true, World::scene()->wgtTex(pieceNames[uint8(PieceType::dragon)]), vec4(1.f));
		left.push_back(new Layout(getSize(SizeRef::iconSize), { mio.dragon }, false, 0));
	} else
		mio.dragon = nullptr;

	// root layout
	vector<Widget*> cont = {
		new Layout(getSize(SizeRef::matchSideWidth), std::move(left), true, getSize(SizeRef::lineSpacing)),
		nullptr
	};
	if (World::game()->board->config.opts & Config::victoryPoints) {
		vector<Widget*> topb = {
			new Widget(),
			mio.vpOwn = new Label(getSize(SizeRef::matchPointsLen), "0", nullptr, nullptr, string(), 1.f, Label::Alignment::center),
			new Label("Victory Points", nullptr, nullptr, string(), 1.f, Label::Alignment::center, false),
			mio.vpEne = new Label(getSize(SizeRef::matchPointsLen), "0", nullptr, nullptr, string(), 1.f, Label::Alignment::center),
			new Widget(),
			new Widget(getSize(SizeRef::matchSideWidth))
		};
		vector<Widget*> midf = {
			new Widget(getSize(SizeRef::superSpacing)),
			new Layout(getSize(SizeRef::lineHeight), std::move(topb), false, getSize(SizeRef::lineSpacing)),
			planeSwitch = new Navigator()
		};
		cont.back() = new Layout(1.f, std::move(midf));
	} else {
		mio.vpOwn = mio.vpEne = nullptr;
		cont.back() = planeSwitch = new Navigator();
	}
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, 0, lineSpacing);	// interactive icons need to be updated once the game match has been set up
}

void GuiGen::openPopupSpawner() const {
	Widget* defSel = nullptr;
	vector<Widget*> bot[2] = { vector<Widget*>(4), vector<Widget*>(4) };
	for (uint8 r = 0, t = 0; r < 2; ++r)
		for (uint8 i = 0; i < 4; ++i, ++t) {
			bool on = World::game()->board->pieceSpawnable(PieceType(t));
			bot[r][i] = new Button(getSize(SizeRef::superHeight), on ? BCall(&Program::eventSpawnPiece) : nullptr, nullptr, firstUpper(pieceNames[t]), on ? 1.f : defaultDim, World::scene()->wgtTex(pieceNames[t]), vec4(1.f));
			if (on && !defSel)
				defSel = bot[r][i];
		}

	vector<Widget*> mid = {
		new Layout(getSize(SizeRef::superHeight), std::move(bot[0]), false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::superHeight), std::move(bot[1]), false, getSize(SizeRef::lineSpacing))
	};
	vector<Widget*> cont = {
		new Widget(),
		new Layout(Size(superHeight * 4.f + lineSpacing * 3.f, Size::abso), std::move(mid)),
		new Widget()
	};

	Size width([](const Widget* self) -> int {
		return std::max(Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight), int((superHeight * 4.f + lineSpacing * 3.f) * float(World::window()->getScreenView().y)));
	});
	Label* cancel = new Label(1.f, "Cancel", &Program::eventClosePopupResetIcons, nullptr, string(), 1.f, Label::Alignment::center);
	vector<Widget*> con = {
		new Label(1.f, "Spawn piece", nullptr, nullptr, string(), 1.f, Label::Alignment::center),
		new Layout(Size(superHeight * 2.f, Size::abso), std::move(cont), false),
		cancel
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 4 + lineSpacing * 2), std::move(con), nullptr, &Program::eventClosePopupResetIcons, true, getSize(SizeRef::lineSpacing), 0.f, defSel ? defSel : cancel));
}

// SETTINGS

uptr<RootLayout> GuiGen::makeSettings(Interactable*& selected, ScrollArea*& content, sizet& bindingsStart) {
	// side bar
	initlist<const char*> tps = {
		"Back",
		"Info",
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		"Rules",
		"Docs",
		"Files",
#endif
		"Reset"
	};
	initlist<const char*>::iterator itps = tps.begin();

	initSizes();
	assignSizeFunc(SizeRef::settingsSideWidth, [tps]() -> int { return txtMaxLen(tps.begin(), tps.end(), lineHeight); });

	vector<Widget*> lft = {
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenMainMenu),
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenInfo),
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenRules),
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenDocs),
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenFiles),
#endif
		new Widget(0),
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventResetSettings)
	};
	selected = lft.front();

	// root layout
	vector<Widget*> cont = {
		new Layout(getSize(SizeRef::settingsSideWidth), std::move(lft), true, getSize(SizeRef::lineSpacing)),
		content = createSettingsList(bindingsStart)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, getSize(SizeRef::superSpacing), lineSpacing, RootLayout::uniformBgColor);
}

ScrollArea* GuiGen::createSettingsList(sizet& bindingsStart) {
	array<string, Settings::screenNames.size()> scnames;
	for (uint8 i = 0; i < Settings::screenNames.size(); ++i)
		scnames[i] = enameToFstr<false>(i, Settings::screenNames);

	vector<ivec2> wsizs = World::window()->windowSizes();
	vector<SDL_DisplayMode> modes = World::window()->displayModes();
	vector<string> winsiz(wsizs.size()), dmodes(modes.size());
	std::transform(wsizs.begin(), wsizs.end(), winsiz.begin(), [](ivec2 size) -> string { return toStr(size, rv2iSeparator); });
	std::transform(modes.begin(), modes.end(), dmodes.begin(), dispToFstr);
	vector<string> fonts = FileSys::listFonts();
	for (string& it : fonts)
		it = firstUpper(delExt(it));

	vector<string> msamplesOpts = { "0" };
	for (uint i = 2; i <= World::window()->getMaxMsamples(); i *= 2)
		msamplesOpts.push_back(toStr(i));

	// setting buttons, labels and action fields for labels
	initlist<const char*> txs = {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		"Display",
#endif
#ifndef __ANDROID__
		"Screen",
		"Size",
		"Mode",
#endif
#ifndef OPENGLES
		"Multisamples",
#endif
		"Shadow resolution",
		"Soft shadows",
		"SSAO",
		"Bloom",
		"Texture scale",
		"VSync",
		"Gamma",
		"Volume",
		"Color ally",
		"Color enemy",
		"Scale tiles",
		"Scale pieces",
		"Auto victory points",
		"Show tooltips",
		"Chat line limit",
		"Deadzone",
		"Resolve family",
		"Font",
		"Invert mouse wheel"
	};
	initlist<const char*>::iterator itxs = txs.begin();

	bindingsStart = txs.size() + 1;
	array<string, Binding::names.size()> bnames;
	for (uint8 i = 0; i < Binding::names.size(); ++i)
		bnames[i] = enameToFstr(i, Binding::names);
	assignSizeFunc(SizeRef::settingsDescWidth, [txs, bnames]() -> int { return std::max(txtMaxLen(txs.begin(), txs.end(), lineHeight), txtMaxLen(bnames.begin(), bnames.end(), lineHeight)); });
	assignSizeFunc(SizeRef::settingsAddWidth, []() -> int { return Label::txtLen("+", lineHeight); });
	assignSizeFunc(SizeRef::settingsSlleWidth, [this]() -> int { return Label::txtLen(string(numDigits(uint(UINT16_MAX)), '0'), lineHeight) + *getSize(GuiGen::SizeRef::caretWidth); });

	constexpr char shadowTip[] = "Width/height of shadow cubemap";
	constexpr char scaleTip[] = "Scale factor of texture sizes";
	constexpr char vsyncTip[] = "Immediate: off\n"
		"Synchronized: on\n"
		"Adaptive: on and smooth (works on fewer computers)";
	constexpr char gammaTip[] = "Brightness";
	constexpr char volumeTip[] = "Audio volume";
	constexpr char chatTip[] = "Line break limit of a chat box";
	constexpr char deadzTip[] = "Controller axis deadzone";

	vector<Widget*> lx[] = { {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new LabelEdit(1.f, toStr(World::sets()->display), nullptr, nullptr, &Program::eventSetDisplay, nullptr, "Keep 0 cause it doesn't really work")
	}, {
#endif
#ifndef __ANDROID__
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, enameToFstr(World::sets()->screen, Settings::screenNames), vector<string>(scnames.begin(), scnames.end()), &Program::eventSetScreen, nullptr, nullptr, "Window or fullscreen (\"desktop\" is native resolution)")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, toStr(World::sets()->size, rv2iSeparator), std::move(winsiz), &Program::eventSetWindowSize, nullptr, nullptr, "Window size")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, dispToFstr(World::sets()->mode), std::move(dmodes), &Program::eventSetWindowMode, nullptr, nullptr, "Fullscreen display properties")
	}, {
#endif
#ifndef OPENGLES
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, toStr(World::sets()->msamples), std::move(msamplesOpts), &Program::eventSetSamples, nullptr, nullptr, "Anti-Aliasing multisamples")
	}, {
#endif
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1, -1, Settings::shadowBitMax, 1, &Program::eventSetShadowResSL, &Program::eventUpdateShadowResSL, shadowTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->shadowRes), &Program::eventSetShadowResLE, nullptr, nullptr, nullptr, shadowTip)
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->softShadows, &Program::eventSetSoftShadows, &Program::eventSetSoftShadows, "Soft shadow edges")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->ssao, &Program::eventSetSsao, &Program::eventSetSsao, "Screen space ambient occlusion")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->bloom, &Program::eventSetBloom, &Program::eventSetBloom, "Brighten and blur bright areas")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, World::sets()->texScale, 1, 100, 10, &Program::eventSetTexturesScaleSL, &Program::eventPrcSliderUpdate, scaleTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->texScale) + '%', &Program::eventSetTextureScaleLE, nullptr, nullptr, nullptr, scaleTip)
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, Settings::vsyncNames[uint8(World::sets()->vsync+1)], vector<string>(Settings::vsyncNames.begin(), Settings::vsyncNames.end()), &Program::eventSetVsync, nullptr, nullptr, vsyncTip)
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, int(World::sets()->gamma * gammaStepFactor), 0, int(Settings::gammaMax * gammaStepFactor), int(gammaStepFactor), &Program::eventSaveSettings, &Program::eventSetGammaSL, gammaTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->gamma), &Program::eventSetGammaLE, nullptr, nullptr, nullptr, gammaTip)
	}, World::audio() ? vector<Widget*>{
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, World::sets()->avolume, 0, SDL_MIX_MAXVOLUME, 8, &Program::eventSetVolumeSL, &Program::eventSLUpdateLE, volumeTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->avolume), &Program::eventSetVolumeLE, nullptr, nullptr, nullptr, volumeTip)
	} : vector<Widget*>{
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Label(1.f, "missing audio device")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, Settings::colorNames[uint8(World::sets()->colorAlly)], vector<string>(Settings::colorNames.begin(), Settings::colorNames.end()), &Program::eventSetColorAlly, nullptr, nullptr, "Color of own pieces")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, Settings::colorNames[uint8(World::sets()->colorEnemy)], vector<string>(Settings::colorNames.begin(), Settings::colorNames.end()), &Program::eventSetColorEnemy, nullptr, nullptr, "Color of enemy pieces")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->scaleTiles, &Program::eventSetScaleTiles, &Program::eventSetScaleTiles, "Scale tile amounts when resizing board")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->scalePieces, &Program::eventSetScalePieces, &Program::eventSetScalePieces, "Scale piece amounts when resizing board")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->autoVictoryPoints, &Program::eventSetAutoVictoryPoints, &Program::eventSetAutoVictoryPoints, "Automatically adjust the configuration when toggling the victory points game variant")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->tooltips, &Program::eventSetTooltips, &Program::eventSetTooltips, "Display tooltips on hover")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, World::sets()->chatLines, 0, Settings::chatLinesMax, 256, &Program::eventSetChatLineLimitSL, &Program::eventSLUpdateLE, chatTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->chatLines), &Program::eventSetChatLineLimitLE, nullptr, nullptr, nullptr, chatTip)
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new Slider(1.f, World::sets()->deadzone, Settings::deadzoneLimit.x, Settings::deadzoneLimit.y, 2048, &Program::eventSetDeadzoneSL, &Program::eventSLUpdateLE, deadzTip),
		new LabelEdit(getSize(SizeRef::settingsSlleWidth), toStr(World::sets()->deadzone), &Program::eventSetDeadzoneLE, nullptr, nullptr, nullptr, deadzTip)
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, Settings::familyNames[uint8(World::sets()->resolveFamily)], vector<string>(Settings::familyNames.begin(), Settings::familyNames.end()), &Program::eventSetResolveFamily, nullptr, nullptr, "What family to use for resolving a host address")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new ComboBox(1.f, World::sets()->font, fonts, &Program::eventSetFont, nullptr, nullptr, "UI font"),
		new ComboBox(1.f, Settings::hintingNames[uint8(World::sets()->hinting)], vector<string>(Settings::hintingNames.begin(), Settings::hintingNames.end()), &Program::eventSetFontHinting, nullptr, nullptr, "Font hinting")
	}, {
		new Label(getSize(SizeRef::settingsDescWidth), *itxs++),
		new CheckBox(getSize(SizeRef::lineHeight), World::sets()->invertWheel, &Program::eventSetInvertWheel, &Program::eventSetInvertWheel, "Invert the mouse wheel motion")
	}, };
	vector<Widget*> lns(bindingsStart + bnames.size());
	for (sizet i = 0; i < txs.size(); ++i)
		lns[i] = new Layout(getSize(SizeRef::lineHeight), std::move(lx[i]), false, getSize(SizeRef::lineSpacing));
	lns[txs.size()] = new Widget(0);

	for (uint8 i = 0; i < bnames.size(); ++i) {
		Label* lbl = new Label(getSize(SizeRef::lineHeight), std::move(bnames[i]));
		vector<Widget*> line = {
			new Layout(getSize(SizeRef::settingsDescWidth), { lbl, new Widget() }),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).keys, Binding::Accept::keyboard, lbl),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).joys, Binding::Accept::joystick, lbl),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).gpds, Binding::Accept::gamepad, lbl),
			new Layout(getSize(SizeRef::settingsAddWidth), { new Label(getSize(SizeRef::lineHeight), "+", &Program::eventAddKeyBinding, nullptr, "Add a " + lbl->getText() + " binding", 1.f, Label::Alignment::center) })
		};
		lns[bindingsStart+i] = new Layout(keyGetLineSize(Binding::Type(i)), std::move(line), false, getSize(SizeRef::lineSpacing));
	}
	return new ScrollArea(1.f, std::move(lns), true, getSize(SizeRef::lineSpacing));
}

template <class T>
Layout* GuiGen::createKeyGetterList(Binding::Type bind, const vector<T>& refs, Binding::Accept type, Label* lbl) const {
	vector<Widget*> lst(refs.size());
	for (sizet i = 0; i < refs.size(); ++i)
		lst[i] = createKeyGetter(type, bind, i, lbl);
	return new Layout(1.f, std::move(lst), true, getSize(SizeRef::lineSpacing));
}

KeyGetter* GuiGen::createKeyGetter(Binding::Accept accept, Binding::Type bind, sizet kid, Label* lbl) const {
	return new KeyGetter(getSize(SizeRef::lineHeight), accept, bind, kid, &Program::eventSaveSettings, &Program::eventDelKeyBinding, lbl->getText() + ' ' + Binding::acceptNames[uint8(accept)] + " binding");
}

SDL_DisplayMode GuiGen::fstrToDisp(const string& str) {
	vector<SDL_DisplayMode> modes = World::window()->displayModes();
	umap<string, uint32> pixelformats;
	pixelformats.reserve(modes.size());
	for (const SDL_DisplayMode& it : modes)
		pixelformats.emplace(pixelformatName(it.format), it.format);

	SDL_DisplayMode mode = strToDisp(str);
	if (string::const_reverse_iterator sit = std::find_if(str.rbegin(), str.rend(), isSpace); sit != str.rend())
		if (umap<string, uint32>::const_iterator pit = pixelformats.find(string(sit.base(), str.end())); pit != pixelformats.end())
			mode.format = pit->second;
	return mode;
}

Size GuiGen::keyGetLineSize(Binding::Type bind) const {
	sizet cnt = std::max(std::max(std::max(World::input()->getBinding(bind).keys.size(), World::input()->getBinding(bind).joys.size()), World::input()->getBinding(bind).gpds.size()), sizet(1));
	return Size(lineHeight * float(cnt) + lineSpacing * float(cnt - 1), Size::abso);
}

void GuiGen::openPopupSettings(ScrollArea*& content, sizet& bindingsStart) {
	vector<Widget*> top = {
		new Label(1.f, "Settings", nullptr, nullptr, string(), 1.f, Label::Alignment::center),
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		new Label("Rules", &Program::eventOpenRules, nullptr, string(), 1.f, Label::Alignment::center),
		new Label("Docs", &Program::eventOpenDocs, nullptr, string(), 1.f, Label::Alignment::center)
#endif
	};
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, string(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Layout(getSize(SizeRef::lineHeight), std::move(top), false, getSize(SizeRef::lineSpacing)),
		content = createSettingsList(bindingsStart),
		new Layout(getSize(SizeRef::lineHeight), std::move(bot), false)
	};
	World::scene()->pushPopup(std::make_unique<Popup>(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventCloseScrollingPopup, true, getSize(SizeRef::lineSpacing), 0.f, nullptr, Popup::Type::settings));
}

void GuiGen::openPopupKeyGetter(Binding::Type bind) const {
	KeyGetter* kget = new KeyGetter(1.f, Binding::Accept::any, bind, SIZE_MAX, &Program::eventSetNewBinding, &Program::eventClosePopup);
	vector<Widget*> con = {
		new Label(1.f, "Add " + enameToFstr(bind, Binding::names) + " binding:"),
		kget
	};
	Size width([](const Widget* self) -> int { return Label::txtLen(static_cast<const Popup*>(self)->getWidget<Label>(0)->getText(), superHeight); });
	World::scene()->pushPopup(std::make_unique<Popup>(pair(width, superHeight * 2.f + lineSpacing), std::move(con), &Program::eventClosePopup, &Program::eventClosePopup, true, getSize(SizeRef::lineSpacing)), kget);
}

template <bool upper, class T, sizet S>
string GuiGen::enameToFstr(T val, const array<const char*, S>& names) {
	string name = names[sizet(val)];
	if constexpr (upper)
		name[0] = toupper(name[0]);
	std::replace(name.begin(), name.end(), '_', ' ');
	return name;
}

// INFO

uptr<RootLayout> GuiGen::makeInfo(Interactable*& selected, ScrollArea*& content) {
	initlist<const char*> tps = {
		"Menu",
		"Back"
	};
	initlist<const char*>::iterator itps = tps.begin();

	initlist<const char*> args = {
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
		"Release Behavior",
#if !defined(OPENGLES) && !defined(__APPLE__)
		"GLEW",
#endif
		"GLM",
		"SDL",
		"SDL_image",
		"SDL_ttf",
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		"Curl",
#endif
		"Logical cores",	// system
		"L1 line size",
		"CPU features",
		"RAM size",
		"Current audio",
		"Current video",
		"State",			// power
		"Time"
	};
	initlist<const char*>::iterator iargs = args.begin();
	initlist<const char*> dispArgs = {
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
	initlist<const char*> rendArgs = {
		"Index",
		"Name",
		"Flags",
		"Max texture size",
		"Texture formats"
	};
	initlist<const char*> ctrlArgs = {
		"Instance",
		"Name",
		"GUID",
		"Type",
		"Buttons",
		"Hats",
		"Axes",
		"Balls",
		"Power"
	};

	initSizes();
	assignSizeFunc(SizeRef::infoSideWidth, [tps]() -> int { return txtMaxLen(tps.begin(), tps.end(), lineHeight); });
	assignSizeFunc(SizeRef::infoArgWidth, [args, dispArgs, rendArgs, ctrlArgs]() -> int { return txtMaxLen({ args, dispArgs, rendArgs, ctrlArgs }, lineHeight); });
	assignSizeFunc(SizeRef::infoDispWidth, [dispArgs]() -> int { return txtMaxLen(dispArgs.begin(), dispArgs.end(), lineHeight); });
	assignSizeFunc(SizeRef::infoArgDispWidth, [this, args, dispArgs, rendArgs, ctrlArgs]() -> int {
		int arg = txtMaxLen({ args, dispArgs, rendArgs, ctrlArgs }, lineHeight);
		int dsp = txtMaxLen(dispArgs.begin(), dispArgs.end(), lineHeight);
		int spacing = *getSize(SizeRef::lineSpacing);
		return dsp + spacing < arg ? arg - dsp - spacing : 0;
	});

	// side bar
	vector<Widget*> lft = {
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenMainMenu),
		new Label(getSize(SizeRef::lineHeight), *itps++, &Program::eventOpenSettings)
	};
	selected = lft.back();

	// information block
	initlist<const char*> titles = {
		WindowSys::title,
		"System",
		"Display",
		"Power",
		"Playback",
		"Recording",
		"Audio Drivers",
		"Video Drivers",
		"Video Displays",
		"Render Drivers",
		"Controllers"
	};
	initlist<const char*>::iterator ititles = titles.begin();

	vector<Widget*> lines;
	appendProgram(lines, iargs, ititles);
	appendSystem(lines, iargs, ititles);
	appendCurrentDisplay(lines, dispArgs.begin(), ititles);
	appendPower(lines, iargs, ititles);
	appendAudioDevices(lines, ititles, SDL_FALSE);
	appendAudioDevices(lines, ititles, SDL_TRUE);
	appendDrivers(lines, ititles, SDL_GetNumAudioDrivers, SDL_GetAudioDriver);
	appendDrivers(lines, ititles, SDL_GetNumVideoDrivers, SDL_GetVideoDriver);
	appendDisplays(lines, dispArgs.begin(), ititles);
	appendRenderers(lines, rendArgs.begin(), ititles);
	appendControllers(lines, ctrlArgs.begin(), ititles);

	// root layout
	vector<Widget*> cont = {
		new Layout(getSize(SizeRef::infoSideWidth), std::move(lft), true, getSize(SizeRef::lineSpacing)),
		content = new ScrollArea(1.f, std::move(lines), true, getSize(SizeRef::lineSpacing))
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, getSize(SizeRef::superSpacing), lineSpacing, RootLayout::uniformBgColor);
}

void GuiGen::appendProgram(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) {
	lines.insert(lines.end(), {
		new Label(getSize(SizeRef::superHeight), *titles++),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, Com::commonVersion) }, false, getSize(SizeRef::lineSpacing))
	});
	if (char* basePath = SDL_GetBasePath()) {
		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args), new Label(1.f, basePath) }, false, getSize(SizeRef::lineSpacing)));
		SDL_free(basePath);
	}
	args++;

	int red, green, blue, alpha, buffer, dbuffer, depth, stencil, stereo, msbuffers, msamples, accvisual, vmaj, vmin, cflags, cprofile, swcc, srgb, crb;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha);
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &buffer);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &dbuffer);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
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

	SDL_version slver, scver, icver, tcver;
	SDL_GetVersion(&slver);
	SDL_VERSION(&scver)
	const SDL_version* ilver = IMG_Linked_Version();
	SDL_IMAGE_VERSION(&icver)
	const SDL_version* tlver = TTF_Linked_Version();
	SDL_TTF_VERSION(&tcver)
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	array<string, 4> compVers = {
#else
	array<string, 3> compVers = {
#endif
		versionText(scver),
		versionText(icver),
		versionText(tcver),
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		LIBCURL_VERSION
#endif
	};
	array<string, compVers.size()>::iterator icmpver = compVers.begin();
	assignSizeFunc(SizeRef::infoProgRightLen, [compVers]() -> int { return txtMaxLen(compVers.begin(), compVers.end(), lineHeight); });

	lines.insert(lines.end(), {
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, SDL_GetPlatform()) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, 'R' + toStr(red) + " G" + toStr(green) + " B" + toStr(blue) + " A" + toStr(alpha)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(buffer)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(dbuffer)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(depth)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(stencil)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(stereo)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(msbuffers)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(msamples)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(accvisual)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, std::move(cvers)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, std::move(sflags)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(swcc)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(srgb)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, ibtos(crb)) }, false, getSize(SizeRef::lineSpacing)),
#if !defined(OPENGLES) && !defined(__APPLE__)
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, reinterpret_cast<const char*>(glewGetString(GLEW_VERSION))) }, false, getSize(SizeRef::lineSpacing)),
#endif
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(GLM_VERSION_MAJOR) + '.' + toStr(GLM_VERSION_MINOR) + '.' + toStr(GLM_VERSION_PATCH) + '.' + toStr(GLM_VERSION_REVISION)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(versionText(slver)), new Label(1.f, SDL_GetRevision()), new Label(getSize(SizeRef::infoProgRightLen), std::move(*icmpver++)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, versionText(*ilver)), new Label(getSize(SizeRef::infoProgRightLen), std::move(*icmpver++)) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, versionText(*tlver)), new Label(getSize(SizeRef::infoProgRightLen), std::move(*icmpver++)) }, false, getSize(SizeRef::lineSpacing)),
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, World::program()->getCurlVersion()), new Label(getSize(SizeRef::infoProgRightLen), std::move(*icmpver++)) }, false, getSize(SizeRef::lineSpacing)),
#endif
		new Widget(getSize(SizeRef::lineHeight))
	});
}

void GuiGen::appendSystem(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const {
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
	const char* audio = SDL_GetCurrentAudioDriver();
	const char* video = SDL_GetCurrentVideoDriver();

	lines.insert(lines.end(), {
		new Label(getSize(SizeRef::superHeight), *titles++),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(SDL_GetCPUCount())) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(SDL_GetCPUCacheLineSize()) + 'B') }, false, getSize(SizeRef::lineSpacing))
	});
	if (!features.empty()) {
		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, std::move(features[0])) }, false, getSize(SizeRef::lineSpacing)));
		for (sizet i = 1; i < features.size(); ++i)
			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgWidth)), new Label(1.f, std::move(features[i])) }, false, getSize(SizeRef::lineSpacing)));
		features.clear();
	} else
		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Widget() }, false, getSize(SizeRef::lineSpacing)));
	lines.insert(lines.end(), {
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, toStr(SDL_GetSystemRAM()) + "MB") }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, audio ? audio : "") }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, video ? video : "") }, false, getSize(SizeRef::lineSpacing)),
		new Widget(getSize(SizeRef::lineHeight))
	});
}

void GuiGen::appendCurrentDisplay(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	if (int i = World::window()->displayID(); i >= 0)
		appendDisplay(lines, i, args);
	lines.push_back(new Widget(getSize(SizeRef::lineHeight)));
}

void GuiGen::appendDisplay(vector<Widget*>& lines, int i, initlist<const char*>::iterator args) const {
	lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[0]), new Label(1.f, toStr(i)) }, false, getSize(SizeRef::lineSpacing)));
	if (const char* name = SDL_GetDisplayName(i))
		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[1]), new Label(1.f, name) }, false, getSize(SizeRef::lineSpacing)));
	if (SDL_DisplayMode mode; !SDL_GetDesktopDisplayMode(i, &mode))
		lines.insert(lines.end(), {
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[2]), new Label(1.f, toStr(mode.w) + " x " + toStr(mode.h)) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[3]), new Label(1.f, toStr(mode.refresh_rate) + "Hz") }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[4]), new Label(1.f, pixelformatName(mode.format)) }, false, getSize(SizeRef::lineSpacing))
		});
	if (float ddpi, hdpi, vdpi; !SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi))
		lines.insert(lines.end(), {
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[5]), new Label(1.f, toStr(ddpi)) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[6]), new Label(1.f, toStr(hdpi)) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[7]), new Label(1.f, toStr(vdpi)) }, false, getSize(SizeRef::lineSpacing))
		});
}

void GuiGen::appendPower(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const {
	const char* powerNames[] = {
		"UNKNOWN",
		"ON_BATTERY",
		"NO_BATTERY",
		"CHARGING",
		"CHARGED"
	};
	int secs, pct;
	SDL_PowerState power = SDL_GetPowerInfo(&secs, &pct);

	lines.insert(lines.end(), {
		new Label(getSize(SizeRef::superHeight), *titles++),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(1.f, powerNames[power]) }, false, getSize(SizeRef::lineSpacing)),
		new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), *args++), new Label(pct >= 0 ? toStr(pct) + '%' : World::fonts()->hasGlyph(0x221E) ? reinterpret_cast<const char*>(u8"\u221E") : "inf"), new Label(1.f, secs >= 0 ? toStr(secs) + 's' : World::fonts()->hasGlyph(0x221E) ? reinterpret_cast<const char*>(u8"\u221E") : "inf") }, false, getSize(SizeRef::lineSpacing)),
		new Widget(getSize(SizeRef::lineHeight))
	});
}

void GuiGen::appendAudioDevices(vector<Widget*>& lines, initlist<const char*>::iterator& titles, int iscapture) const {
	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	for (int i = 0; i < SDL_GetNumAudioDevices(iscapture); ++i)
		if (const char* name = SDL_GetAudioDeviceName(i, iscapture))
			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), toStr(i)), new Label(1.f, name) }, false, getSize(SizeRef::lineSpacing)));
	lines.push_back(new Widget(getSize(SizeRef::lineHeight)));
}

void GuiGen::appendDrivers(vector<Widget*>& lines, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) const {
	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	for (int i = 0; i < limit(); ++i)
		if (const char* name = value(i))
			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), toStr(i)), new Label(1.f, name) }, false, getSize(SizeRef::lineSpacing)));
	lines.push_back(new Widget(getSize(SizeRef::lineHeight)));
}

void GuiGen::appendDisplays(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
		appendDisplay(lines, i, args);

		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(1.f, args[8]) }, false, lineSpacing));
		for (int j = 0; j < SDL_GetNumDisplayModes(i); ++j) {
			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgDispWidth)), new Label(getSize(SizeRef::infoDispWidth), args[0]), new Label(1.f, toStr(j)) }, false, getSize(SizeRef::lineSpacing)));
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, j, &mode))
				lines.insert(lines.end(), {
					new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgDispWidth)), new Label(getSize(SizeRef::infoDispWidth), args[2]), new Label(1.f, toStr(mode.w) + " x " + toStr(mode.h)) }, false, getSize(SizeRef::lineSpacing)),
					new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgDispWidth)), new Label(getSize(SizeRef::infoDispWidth), args[3]), new Label(1.f, toStr(mode.refresh_rate)) }, false, getSize(SizeRef::lineSpacing)),
					new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgDispWidth)), new Label(getSize(SizeRef::infoDispWidth), args[4]), new Label(1.f, pixelformatName(mode.format)) }, false, getSize(SizeRef::lineSpacing))
				});
			if (j < SDL_GetNumDisplayModes(i) - 1)
				lines.push_back(new Widget(getSize(SizeRef::lineSpacing)));
		}
		if (i < SDL_GetNumVideoDisplays() - 1)
			lines.push_back(new Widget(getSize(SizeRef::lineSpacing)));
	}
	lines.push_back(new Widget(getSize(SizeRef::lineHeight)));
}

void GuiGen::appendRenderers(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
		lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[0]), new Label(1.f, toStr(i)) }, false, getSize(SizeRef::lineSpacing)));
		if (SDL_RendererInfo info; !SDL_GetRenderDriverInfo(i, &info)) {
			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[1]), new Label(1.f, info.name) }, false, getSize(SizeRef::lineSpacing)));

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
				lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[2]), new Label(1.f, std::move(flags[0])) }, false, getSize(SizeRef::lineSpacing)));
				for (sizet j = 1; j < flags.size(); ++j)
					lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgWidth)), new Label(1.f, std::move(flags[j])) }, false, getSize(SizeRef::lineSpacing)));
			} else
				lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[2]), new Widget() }, false, getSize(SizeRef::lineSpacing)));

			lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[3]), new Label(1.f, toStr(info.max_texture_width) + " x " + toStr(info.max_texture_height)) }, false, getSize(SizeRef::lineSpacing)));
			if (info.num_texture_formats) {
				lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[4]), new Label(1.f, pixelformatName(info.texture_formats[0])) }, false, getSize(SizeRef::lineSpacing)));
				for (uint32 j = 1; j < info.num_texture_formats; ++j)
					lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Widget(getSize(SizeRef::infoArgWidth)), new Label(1.f, pixelformatName(info.texture_formats[j])) }, false, getSize(SizeRef::lineSpacing)));
			} else
				lines.push_back(new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[4]), new Label(1.f, toStr(i)) }, false, getSize(SizeRef::lineSpacing)));
		}
		if (i < SDL_GetNumRenderDrivers() - 1)
			lines.push_back(new Widget(getSize(SizeRef::lineSpacing)));
	}
	lines.push_back(new Widget(getSize(SizeRef::lineHeight)));
}

void GuiGen::appendControllers(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	const char* typeNames[] = {
		"UNKNOWN",
		"GAMECONTROLLER",
		"TYPE_WHEEL",
		"ARCADE_STICK",
		"FLIGHT_STICK",
		"DANCE_PAD",
		"GUITAR",
		"DRUM_KIT",
		"ARCADE_PAD",
		"TYPE_THROTTLE"
	};
	const char* powerNames[] = {
		"UNKNOWN",
		"EMPTY",
		"LOW",
		"MEDIUM",
		"FULL",
		"WIRED",
		"MAX"
	};

	lines.push_back(new Label(getSize(SizeRef::superHeight), *titles++));
	vector<SDL_Joystick*> joys = World::input()->listJoysticks();
	for (sizet i = 0; i < joys.size(); ++i) {
		const char* name = SDL_JoystickName(joys[i]);
		char guid[33];
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joys[i]), guid, sizeof(guid));
		lines.insert(lines.end(), {
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[0]), new Label(1.f, toStr(SDL_JoystickInstanceID(joys[i]))) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[1]), new Label(1.f, name ? name : "") }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[2]), new Label(1.f, guid) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[3]), new Label(1.f, typeNames[SDL_JoystickGetType(joys[i])]) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[4]), new Label(1.f, toStr(SDL_JoystickNumButtons(joys[i]))) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[5]), new Label(1.f, toStr(SDL_JoystickNumHats(joys[i]))) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[6]), new Label(1.f, toStr(SDL_JoystickNumAxes(joys[i]))) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[7]), new Label(1.f, toStr(SDL_JoystickNumBalls(joys[i]))) }, false, getSize(SizeRef::lineSpacing)),
			new Layout(getSize(SizeRef::lineHeight), { new Label(getSize(SizeRef::infoArgWidth), args[8]), new Label(1.f, powerNames[SDL_JoystickCurrentPowerLevel(joys[i])+1]) }, false, getSize(SizeRef::lineSpacing))
		});
		if (i < joys.size() - 1)
			lines.push_back(new Widget(getSize(SizeRef::lineSpacing)));
	}
}

string GuiGen::ibtos(int val) {
	if (!val)
		return "off";
	if (val == 1)
		return "on";
	return toStr(val);
}
