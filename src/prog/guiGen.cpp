#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#ifdef WEBUTILS
#include <curl/curl.h>
#endif

// GUI TEXT

GuiGen::Text::Text(string str, int sh) :
	text(std::move(str)),
	length(strLen(text.c_str(), sh)),
	height(sh)
{}

int GuiGen::Text::strLen(const char* str, int height) {
	return World::fonts()->length(str, height) + height / Label::textMarginFactor * 2;
}

template <class T>
int GuiGen::Text::maxLen(T pos, T end, int height) {
	int width = 0;
	for (; pos != end; ++pos)
		if (int len = strLen(*pos, height); len > width)
			width = len;
	return width;
}

int GuiGen::Text::maxLen(const initlist<initlist<const char*>>& lists, int height) {
	int width = 0;
	for (initlist<const char*> it : lists)
		if (int len = maxLen(it.begin(), it.end(), height); len > width)
			width = len;
	return width;
}

// GUI GEN

void GuiGen::resize() {
	smallHeight = World::window()->getScreenView().y / 36;		// 20p	(values are in relation to 720p height)
	lineHeight = World::window()->getScreenView().y / 24;		// 30p
	superHeight = World::window()->getScreenView().y / 18;		// 40p
	tooltipHeight = World::window()->getScreenView().y / 45;	// 16p
	lineSpacing = World::window()->getScreenView().y / 144;		// 5p
	superSpacing = World::window()->getScreenView().y / 72;		// 10p
	iconSize = World::window()->getScreenView().y / 11;			// 64p
	tooltipLimit = World::window()->getScreenView().x * 2 / 3;
}

Texture GuiGen::makeTooltip(const char* text) const {
	return World::sets()->tooltips ? World::fonts()->render(text, tooltipHeight, tooltipLimit) : nullptr;
}

Texture GuiGen::makeTooltipL(const char* text) const {
	if (!World::sets()->tooltips)
		return nullptr;

	uint width = 0;
	for (const char* pos = text; *pos;) {
		sizet len = strcspn(pos, "\n");
		if (uint siz = Text::strLen(string(pos, len).c_str(), tooltipHeight); siz > width)
			if (width = std::min(siz, tooltipLimit); width == tooltipLimit)
				break;
		pos += pos[len] ? len + 1 : len;
	}
	return World::fonts()->render(text, tooltipHeight, width);
}

void GuiGen::openPopupMessage(string msg, BCall ccal, string ctxt) const {
	Text ok(std::move(ctxt), superHeight);
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, std::move(ok.text), ccal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, 0)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), ccal, ccal, true, lineSpacing));
}

void GuiGen::openPopupChoice(string msg, BCall kcal, BCall ccal) const {
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, lineSpacing)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), kcal, ccal, true, lineSpacing));
}

void GuiGen::openPopupInput(string msg, string text, BCall kcal, uint16 limit) const {
	Text ms(std::move(msg), superHeight);
	LabelEdit* ledit = new LabelEdit(1.f, std::move(text), nullptr, nullptr, kcal, &Program::eventClosePopup, Texture(), 1.f, limit);
	vector<Widget*> bot = {
		new Label(1.f, "Okay", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text)),
		ledit,
		new Layout(1.f, std::move(bot), false, 0)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(std::max(World::window()->getScreenView().x / 4 * 3, ms.length), superHeight * 3 + lineSpacing * 2), std::move(con), kcal, &Program::eventClosePopup, true, lineSpacing), ledit);
}

vector<Widget*> GuiGen::createChat(TextBox*& chatBox, bool overlay) const {
	return {
		chatBox = new TextBox(1.f, smallHeight, string(), &Program::eventFocusChatLabel, &Program::eventClearLabel, Texture(), 1.f, World::sets()->chatLines, false, true, 0, Widget::colorDimmed),
		new LabelEdit(smallHeight, string(), nullptr, nullptr, &Program::eventSendMessage, overlay ? &Program::eventCloseChat : &Program::eventHideChat, Texture(), 1.f, chatLineLimit, true)
	};
}

Overlay* GuiGen::createNotification(Overlay*& notification) const {
	Text txt("<i>", superHeight);
	Label* msg = new Label(1.f, std::move(txt.text), &Program::eventToggleChat, nullptr, makeTooltip("New message"), 1.f, Label::Alignment::left, true, 0, Widget::colorDark);
	return notification = new Overlay(pair(1.f - float(txt.length) / float(World::window()->getScreenView().x), 0), pair(txt.length, superHeight), { msg }, nullptr, nullptr, true, false, true, 0, vec4(0.f));
}

Overlay* GuiGen::createFpsCounter(Label*& fpsText) const {
	Text txt = makeFpsText(World::window()->getDeltaSec());
	fpsText = new Label(1.f, std::move(txt.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::left, false);
	return new Overlay(pair(0, 0.959f), pair(txt.length, lineHeight), { fpsText }, nullptr, nullptr, true, World::program()->ftimeMode != Program::FrameTime::none, false, 0, Widget::colorDark);
}

GuiGen::Text GuiGen::makeFpsText(float dSec) const {
	switch (World::program()->ftimeMode) {
	case Program::FrameTime::frames:
		return Text(toStr(uint(std::round(1.f / dSec))), lineHeight);
	case Program::FrameTime::seconds:
		return Text(toStr(uint(dSec * 1000.f)), lineHeight);
	}
	return Text(string(), lineHeight);
}

// MAIN MENU

uptr<RootLayout> GuiGen::makeMainMenu(Interactable*& selected, Label*& versionNotif) const {
	// server input
	Text srvt("Server:", superHeight);
	vector<Widget*> srv = {
		new Label(srvt.length, srvt.text),
		new LabelEdit(1.f, World::sets()->address, &Program::eventUpdateAddress, &Program::eventResetAddress)
	};

	// port input and connect button
	vector<Widget*> prt = {
		new Label(srvt.length, "Port:"),
		new LabelEdit(1.f, World::sets()->port, &Program::eventUpdatePort, &Program::eventResetPort)
	};

	// net buttons
	vector<Widget*> con = {
		new Label(srvt.length, "Host", &Program::eventOpenHostMenu, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	selected = con.back();

	// title
	int width = World::fonts()->length((srvt.text + "000.000.000.000").c_str(), superHeight) + lineSpacing + superHeight / Label::textMarginFactor * 4;
	const Texture* title = World::scene()->getTex("title");
	vector<Widget*> tit = {
		new Widget(),
		new Button(title->getRes().y * width / title->getRes().x, nullptr, nullptr, Texture(), 1.f, *title, vec4(1.f))
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
	Text ver(Com::commonVersion, lineHeight);
	vector<Widget*> menu = {
		new Widget(),
		new Layout(width, std::move(buts), true, superSpacing),
		new Widget()
	};
	vector<Widget*> vers = {
		versionNotif = new Label(1.f, World::program()->getLatestVersion(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::left, false),
		new Label(ver.length, std::move(ver.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::right, false)
	};
	vector<Widget*> cont = {
		new Layout(1.f, std::move(menu), false),
		new Layout(lineHeight, std::move(vers), false)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont));
}

// LOBBY

uptr<RootLayout> GuiGen::makeLobby(Interactable*& selected, TextBox*& chatBox, ScrollArea*& rooms, vector<pair<string, bool>>& roomBuff) const {
	// side bar
	initlist<const char*> sidt = {
		"Back",
		"Host"
	};
	initlist<const char*>::iterator isidt = sidt.begin();
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, *isidt++, &Program::eventExitLobby),
		new Label(lineHeight, *isidt++, &Program::eventHostRoomInput)
	};
	selected = lft.back();

	// room list
	vector<Widget*> lns(roomBuff.size());
	for (sizet i = 0; i < roomBuff.size(); ++i)
		lns[i] = createRoom(std::move(roomBuff[i].first), roomBuff[i].second);
	roomBuff.clear();

	// main layout
	vector<Widget*> cont = {
		new Layout(sideLength, std::move(lft), true, lineSpacing),
		rooms = new ScrollArea(1.f, std::move(lns), true, lineSpacing)
	};

	// root layout
	vector<Widget*> root = {
		new Layout(1.f, std::move(cont), false, superSpacing),
		new Layout(chatEmbedSize, { createChat(chatBox, false) })
	};
	return std::make_unique<RootLayout>(1.f, std::move(root), true, superSpacing, RootLayout::uniformBgColor);
}

Label* GuiGen::createRoom(string&& name, bool open) const {
	return new Label(lineHeight, std::move(name), open ? &Program::eventJoinRoomRequest : nullptr, nullptr, Texture(), open ? 1.f : defaultDim);
}

// ROOM

uptr<RootLayout> GuiGen::makeRoom(Interactable*& selected, ConfigIO& wio, RoomIO& rio, TextBox*& chatBox, ComboBox*& configName, const umap<string, Config>& confs, const string& startConfig) const {
	bool unique = World::program()->info & Program::INF_UNIQ, host = World::program()->info & Program::INF_HOST;
	Text back("Back", lineHeight);
	Text thost("Host", lineHeight);
	Text kick("Kick", lineHeight);
	vector<Widget*> top0 = {
		new Label(back.length, std::move(back.text), unique ? &Program::eventOpenMainMenu : &Program::eventExitRoom),
		rio.start = new Label(1.f, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	selected = top0.front();
	if (unique) {
		Text setup("Setup", lineHeight);
		Text port("Port:", lineHeight);
		top0.insert(top0.begin() + 1, new Label(setup.length, std::move(setup.text), &Program::eventOpenSetup));
#ifndef EMSCRIPTEN
		top0.insert(top0.end(), {
			new Label(port.length, std::move(port.text)),
			new LabelEdit(Text::strLen(string(numDigits(uint(UINT16_MAX)), '0').c_str(), lineHeight) + LabelEdit::caretWidth, World::sets()->port, &Program::eventUpdatePort)
		});
#endif
	} else if (host)
		top0.insert(top0.end(), {
			rio.host = new Label(thost.length, std::move(thost.text)),
			rio.kick = new Label(kick.length, std::move(kick.text))
		});

	vector<Widget*> menu = createConfigList(wio, host ? confs.at(startConfig) : World::game()->board.config, host, false);
	Text cfgt("Configuration:", lineHeight);
	Text copy("Copy", lineHeight);
	vector<Widget*> top1 = {
		new Label(cfgt.length, std::move(cfgt.text)),
		configName = new ComboBox(1.f, startConfig, host ? sortNames(confs) : vector<string>{ startConfig }, host ? &Program::eventSwitchConfig : nullptr),
		new Label(copy.length, std::move(copy.text), &Program::eventConfigCopyInput)
	};
	if (host) {
		Text newt("New", lineHeight);
		Text dele("Del", lineHeight);
		top1.insert(top1.end(), {
			new Label(newt.length, std::move(newt.text), &Program::eventConfigNewInput),
			rio.del = new Label(dele.length, std::move(dele.text), &Program::eventConfigDelete)
		});

		Text reset("Reset", lineHeight);
		vector<Widget*> lineR = {
			new Label(reset.length, std::move(reset.text), &Program::eventUpdateReset, nullptr, makeTooltip("Reset current configuration")),
			new Widget()
		};
		menu.push_back(new Layout(lineHeight, std::move(lineR), false, lineSpacing));
	}

	vector<Widget*> cont = {
		new Layout(lineHeight, std::move(top0), false, lineSpacing),
		new Layout(lineHeight, std::move(top1), false, lineSpacing),
		new Widget(0),
		new ScrollArea(1.f, std::move(menu), true, lineSpacing)
	};
	vector<Widget*> rwgt = {
		new Layout(1.f, std::move(cont), true, lineSpacing)
	};
	if (!unique)
		rwgt.push_back(new Layout(host ? Size(0) : Size(chatEmbedSize), createChat(chatBox, false)));
	return std::make_unique<RootLayout>(1.f, std::move(rwgt), false, lineSpacing, RootLayout::uniformBgColor);
}

vector<Widget*> GuiGen::createConfigList(ConfigIO& wio, const Config& cfg, bool active, bool match) const {
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
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);

	string scTip = "Chance of breaching a " + string(Tile::names[Tile::fortress]);
	string tileTips[Tile::lim];
	for (uint8 i = 0; i < Tile::lim; ++i)
		tileTips[i] = "Number of " + string(Tile::names[i]) + " tiles per homeland";
	constexpr char fortressTip[] = "(calculated automatically to fill the remaining free tiles)";
	string middleTips[Tile::lim];
	for (uint8 i = 0; i < Tile::lim; ++i)
		middleTips[i] = "Number of " + string(Tile::names[i]) + " tiles in the middle row per player";
	string pieceTips[Piece::lim];
	for (uint8 i = 0; i < Piece::lim; ++i)
		pieceTips[i] = "Number of " + string(Piece::names[i]) + " pieces per player";
	constexpr char fortTip[] = "Number of fortresses that need to be captured in order to win";
	constexpr char throneTip[] = "Number of thrones that need to be killed in order to win";
	Text equidistant("centered:", lineHeight);
	Text width("width:", lineHeight);
	Text height("height:", lineHeight);
	Size amtWidth = update ? Size(Text::strLen(string(numDigits(uint(UINT16_MAX)), '0').c_str(), lineHeight) + LabelEdit::caretWidth) : Size(1.f);

	vector<vector<Widget*>> lines0 = { {
		new Label(descLength, *itxs++),
		wio.victoryPoints = new CheckBox(lineHeight, cfg.opts & Config::victoryPoints, vupdate, vupdate, makeTooltip("Use the victory points game variant")),
		wio.victoryPointsNum = new LabelEdit(1.f, toStr(cfg.victoryPointsNum), update, nullptr, nullptr, nullptr, makeTooltip("Number of victory points required to win")),
		new Label(equidistant.length, std::move(equidistant.text)),
		wio.vpEquidistant = new CheckBox(lineHeight, cfg.opts & Config::victoryPointsEquidistant, update, update, makeTooltip("Place middle row fortresses in the center"))
	}, {
		new Label(descLength, *itxs++),
		wio.ports = new CheckBox(lineHeight, cfg.opts & Config::ports, update, update, makeTooltip("Use the ports game variant"))
	}, {
		new Label(descLength, *itxs++),
		wio.rowBalancing = new CheckBox(lineHeight, cfg.opts & Config::rowBalancing, update, update, makeTooltip("Have at least one of each tile type in each row"))
	}, {
		new Label(descLength, *itxs++),
		wio.homefront = new CheckBox(lineHeight, cfg.opts & Config::homefront, update, update, makeTooltip("Use the homefront game variant"))
	}, {
		new Label(descLength, *itxs++),
		wio.setPieceBattle = new CheckBox(lineHeight, cfg.opts & Config::setPieceBattle, update, update, makeTooltip("Use the set piece battle game variant")),
		wio.setPieceBattleNum = new LabelEdit(1.f, toStr(cfg.setPieceBattleNum), update, nullptr, nullptr, nullptr, makeTooltip("Number of pieces per player to be chosen")),
	}, {
		new Label(descLength, *itxs++),
		new Label(width.length, std::move(width.text)),
		wio.width = new LabelEdit(1.f, toStr(cfg.homeSize.x), update, nullptr, nullptr, nullptr, makeTooltip("Number of the board's columns of tiles")),
		new Label(height.length, std::move(height.text)),
		wio.height = new LabelEdit(1.f, toStr(cfg.homeSize.y), update, nullptr, nullptr, nullptr, makeTooltip("Number of the board's rows of tiles minus one divided by two"))
	}, {
		new Label(descLength, *itxs++),
		wio.battleLE = new LabelEdit(amtWidth, toStr(cfg.battlePass) + '%', update, nullptr, nullptr, nullptr, makeTooltip(scTip.c_str()))
	}, {
		new Label(descLength, *itxs++),
		wio.favorLimit = new LabelEdit(1.f, toStr(cfg.favorLimit), update, nullptr, nullptr, nullptr, makeTooltip("Maximum amount of fate's favors per type"))
	}, {
		new Label(descLength, *itxs++),
		wio.favorTotal = new CheckBox(lineHeight, cfg.opts & Config::favorTotal, update, update, makeTooltip("Limit the total amount of fate's favors for the entire match"))
	}, {
		new Label(descLength, *itxs++),
		wio.firstTurnEngage = new CheckBox(lineHeight, cfg.opts & Config::firstTurnEngage, update, update, makeTooltip("Allow attacking and firing at pieces during each player's first turn"))
	}, {
		new Label(descLength, *itxs++),
		wio.terrainRules = new CheckBox(lineHeight, cfg.opts & Config::terrainRules, update, update, makeTooltip("Use terrain related rules"))
	}, {
		new Label(descLength, *itxs++),
		wio.dragonLate = new CheckBox(lineHeight, cfg.opts & Config::dragonLate, update, update, makeTooltip((firstUpper(Piece::names[Piece::dragon]) + " can be placed later during the match on a homeland " + Tile::names[Tile::fortress]).c_str()))
	}, {
		new Label(descLength, *itxs++),
		wio.dragonStraight = new CheckBox(lineHeight, cfg.opts & Config::dragonStraight, update, update, makeTooltip((firstUpper(Piece::names[Piece::dragon]) + " moves in a straight line").c_str()))
	}, {
		new Label(descLength, *itxs++)
	} };
	lines0.back().resize(Piece::lim + 1);
	for (uint8 i = 0; i < Piece::lim; ++i)
		lines0.back()[i+1] = wio.capturers[i] = new Icon(lineHeight, string(), iupdate, iupdate, nullptr, makeTooltip((firstUpper(Piece::names[i]) + " can capture fortresses").c_str()), 1.f, Label::Alignment::left, true, World::scene()->texture(Piece::names[i]), vec4(1.f), cfg.capturers & (1 << i));
	if (active)
		lines0[6].insert(lines0[6].begin() + 1, wio.battleSL = new Slider(1.f, cfg.battlePass, 0, Config::randomLimit, 10, update, &Program::eventPrcSliderUpdate, makeTooltip(scTip.c_str())));

	vector<vector<Widget*>> lines1(Tile::lim + 2);
	for (uint8 i = 0; i < Tile::lim; ++i)
		lines1[i] = {
			new Label(descLength, firstUpper(Tile::names[i])),
			wio.tiles[i] = new LabelEdit(amtWidth, toStr(cfg.tileAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(tileTips[i].c_str()) : makeTooltipL((tileTips[i] + '\n' + fortressTip).c_str()))
		};
	lines1[Tile::lim] = {
		new Label(descLength, firstUpper(Tile::names[Tile::fortress])),
		wio.tileFortress = new Label(1.f, tileFortressString(cfg), nullptr, nullptr, makeTooltipL(("Number of " + string(Tile::names[Tile::fortress]) + " tiles per homeland\n" + fortressTip).c_str()))
	};
	lines1[Tile::lim+1] = {
		new Label(descLength, *itxs++),
		wio.winFortress = new LabelEdit(amtWidth, toStr(cfg.winFortress), update, nullptr, nullptr, nullptr, makeTooltip(fortTip))
	};
	if (active) {
		uint16 rest = cfg.countFreeTiles();
		for (uint8 i = 0; i < Tile::lim; ++i)
			lines1[i].insert(lines1[i].begin() + 1, new Slider(1.f, cfg.tileAmounts[i], cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, cfg.tileAmounts[i] + rest, 1, update, &Program::eventTileSliderUpdate, makeTooltip(tileTips[i].c_str())));
		lines1[Tile::lim+1].insert(lines1[Tile::lim+1].begin() + 1, new Slider(1.f, cfg.winFortress, 0, cfg.countFreeTiles(), 1, update, &Program::eventSLUpdateLE, makeTooltip(fortTip)));
	}

	vector<vector<Widget*>> lines2(Tile::lim + 1);
	for (uint8 i = 0; i < Tile::lim; ++i)
		lines2[i] = {
			new Label(descLength, firstUpper(Tile::names[i])),
			wio.middles[i] = new LabelEdit(amtWidth, toStr(cfg.middleAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(middleTips[i].c_str()) : makeTooltipL((tileTips[i] + '\n' + fortressTip).c_str()))
	};
	lines2.back() = {
		new Label(descLength, firstUpper(Tile::names[Tile::fortress])),
		wio.middleFortress = new Label(1.f, middleFortressString(cfg), nullptr, nullptr, makeTooltipL(("Number of " + string(Tile::names[Tile::fortress]) + " tiles in the middle row\n" + fortressTip).c_str()))
	};
	if (active) {
		uint16 rest = cfg.countFreeMiddles();
		for (uint8 i = 0; i < Tile::lim; ++i)
			lines2[i].insert(lines2[i].begin() + 1, new Slider(1.f, cfg.middleAmounts[i], 0, cfg.middleAmounts[i] + rest, 1, update, &Program::eventMiddleSliderUpdate, makeTooltip(middleTips[i].c_str())));
	}

	vector<vector<Widget*>> lines3(Piece::lim + 2);
	for (uint8 i = 0; i < Piece::lim; ++i)
		lines3[i] = {
			new Label(descLength, firstUpper(Piece::names[i])),
			wio.pieces[i] = new LabelEdit(amtWidth, toStr(cfg.pieceAmounts[i]), update, nullptr, nullptr, nullptr, makeTooltip(pieceTips[i].c_str()))
		};
	lines3[Piece::lim] = {
		new Label(descLength, "Total"),
		wio.pieceTotal = new Label(1.f, pieceTotalString(cfg), nullptr, nullptr, makeTooltip("Total amount of pieces out of the maximum possible number of pieces per player"))
	};
	lines3.back() = {
		new Label(descLength, *itxs++),
		wio.winThrone = new LabelEdit(amtWidth, toStr(cfg.winThrone), update, nullptr, nullptr, nullptr, makeTooltip(throneTip))
	};
	if (active) {
		uint16 rest = cfg.countFreePieces();
		for (uint8 i = 0; i < Piece::lim; ++i)
			lines3[i].insert(lines3[i].begin() + 1, new Slider(1.f, cfg.pieceAmounts[i], 0, cfg.pieceAmounts[i] + rest, 1, update, &Program::eventPieceSliderUpdate, makeTooltip(pieceTips[i].c_str())));
		lines3.back().insert(lines3.back().begin() + 1, new Slider(1.f, cfg.winThrone, 0, cfg.pieceAmounts[Piece::throne], 1, update, &Program::eventSLUpdateLE, makeTooltip(throneTip)));
	} else if (World::netcp() && (World::game()->board.config.opts & Config::setPieceBattle) && World::game()->board.config.setPieceBattleNum < World::game()->board.config.countPieces())
		for (uint8 i = 0; i < Piece::lim; ++i)
			if (lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board.ownPieceAmts[i]), nullptr, nullptr, makeTooltip(("Number of ally " + string(Piece::names[i]) + " pieces").c_str()))); match)
				lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board.enePieceAmts[i]), nullptr, nullptr, makeTooltip(("Number of enemy " + string(Piece::names[i]) + " pieces").c_str())));

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
		menu[id++] = new Layout(lineHeight, std::move(it), false, lineSpacing);
}

void GuiGen::setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const {
	int tlen = Text::strLen(title.c_str(), lineHeight);
	vector<Widget*> line = {
		new Label(tlen, std::move(title)),
		new Widget()
	};
	menu[id++] = new Layout(lineHeight, std::move(line), false, lineSpacing);
}

// SETUP

uptr<RootLayout> GuiGen::makeSetup(Interactable*& selected, SetupIO& sio, Icon*& bswapIcon, Navigator*& planeSwitch) const {
	// sidebar
	initlist<const char*> sidt = {
		"Exit",
		"Config",
		"Save",
		"Load",
		"Delete",
		"Back",
		"Chat",
		"Finish"	// width placeholder just in case for when setting nextIcon's text later
	};
	initlist<const char*>::iterator isidt = sidt.begin();
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);

	vector<Widget*> wgts = {
		new Label(lineHeight, *isidt++, &Program::eventAbortGame, nullptr, makeTooltip("Exit the game")),
		new Label(lineHeight, *isidt++, &Program::eventShowConfig, nullptr, makeTooltip("Show current game configuration")),
		new Label(lineHeight, *isidt++, &Program::eventOpenSetupSave, nullptr, makeTooltip("Save current setup")),
		sio.load = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Load a setup")),
		bswapIcon = new Icon(lineHeight, *isidt++, nullptr, nullptr, nullptr, makeTooltip("Switch between placing and deleting a tile/piece")),
		sio.back = new Label(lineHeight, *isidt++),
		sio.next = new Label(lineHeight)
	};
	selected = wgts.back();
	if (World::netcp())
		wgts.insert(wgts.begin() + 4, new Label(lineHeight, *isidt++, &Program::eventToggleChat, nullptr, makeTooltip("Toggle chat")));

	vector<Widget*> side = {
		new Layout(sideLength, std::move(wgts), true, lineSpacing),
		planeSwitch = new Navigator()
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(1.f, std::move(side), false),
		sio.icons = new Layout(iconSize, {}, false, lineSpacing),
		new Widget(superSpacing)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont));
}

vector<Widget*> GuiGen::createBottomIcons(bool tiles) const {
	uint8 size = tiles ? Tile::lim : Piece::lim;
	const char* const* names = tiles ? Tile::names.data() : Piece::names.data();
	vector<Widget*> ibot(size + 2);
	ibot.front() = new Widget();
	for (uint8 i = 0; i < size; ++i)
		ibot[1+i] = new Icon(iconSize, string(), &Program::eventIconSelect, nullptr, nullptr, makeTooltip(firstUpper(names[i]).c_str()), 1.f, Label::Alignment::center, true, World::scene()->texture(names[i]), vec4(1.f));
	ibot.back() = new Widget();
	return ibot;
}

Overlay* GuiGen::createGameMessage(Label*& message, bool setup) const {
	message = new Label(1.f, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center, false);
	return new Overlay(setup ? pair(0.1f, 0.08f) : pair(0.f, 0.92f), pair(setup ? 0.8f : 1.f, superHeight), { message }, nullptr, nullptr, true, true, false);
}

Overlay* GuiGen::createGameChat(TextBox*& chatBox) const {
	return new Overlay(pair(0.7f, 0.f), pair(0.3f, 1.f), createChat(chatBox, true), &Program::eventChatOpen, &Program::eventChatClose);
}

void GuiGen::openPopupFavorPick(uint16 availableFF) const {
	Widget* defSel = nullptr;
	vector<Widget*> bot(favorMax + 2);
	bot.front() = new Widget();
	for (uint8 i = 0; i < favorMax; ++i) {
		bool on = World::game()->favorsCount[i] < World::game()->favorsLeft[i];
		bot[1+i] = new Button(superHeight, on ? &Program::eventPickFavor : nullptr, nullptr, makeTooltip(firstUpper(favorNames[i]).c_str()), on ? 1.f : defaultDim, World::scene()->texture(favorNames[i]), vec4(1.f));
		if (on && !defSel)
			defSel = bot[1+i];
	}
	bot.back() = new Widget();

	Text ms(msgFavorPick + (availableFF > 1 ? " (" + toStr(availableFF) + ')' : string()), superHeight);
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, lineSpacing)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(std::max(ms.length, int(favorMax) * superHeight + int(favorMax + 1) * lineSpacing), superHeight * 2 + lineSpacing), std::move(con), nullptr, nullptr, true, lineSpacing, defSel));
}

void GuiGen::openPopupConfig(const string& configName, const Config& cfg, ScrollArea*& configList, bool match) const {
	ConfigIO wio;
	Text rules("Rules", lineHeight);
	Text docs("Docs", lineHeight);
	vector<Widget*> top = {
		new Label(1.f, configName, nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		new Label(rules.length, std::move(rules.text), &Program::eventOpenRules, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(docs.length, std::move(docs.text), &Program::eventOpenDocs, nullptr, Texture(), 1.f, Label::Alignment::center)
#endif
	};
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Layout(lineHeight, std::move(top), false, lineSpacing),
		configList = new ScrollArea(1.f, createConfigList(wio, cfg, false, match), true, lineSpacing),
		new Layout(lineHeight, std::move(bot), false, 0)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventCloseConfigList, true, lineSpacing));
}

void GuiGen::openPopupSaveLoad(const umap<string, Setup>& setups, bool save) const {
	Text back("Back", lineHeight);
	Text tnew("New", lineHeight);
	Label* blbl = new Label(back.length, std::move(back.text), &Program::eventClosePopup);
	vector<Widget*> top = {
		blbl
	};
	if (save)
		top.insert(top.end(), {
			new LabelEdit(1.f, string(), nullptr, nullptr, &Program::eventSetupNew),
			new Label(tnew.length, std::move(tnew.text), &Program::eventSetupNew)
		});

	vector<string> names = sortNames(setups);
	vector<Widget*> saves(setups.size());
	for (sizet i = 0; i < setups.size(); ++i)
		saves[i] = new Label(lineHeight, std::move(names[i]), save ? &Program::eventSetupSave : &Program::eventSetupLoad, &Program::eventSetupDelete);

	vector<Widget*> con = {
		new Layout(lineHeight, std::move(top), false, lineSpacing),
		new ScrollArea(1.f, std::move(saves), true, lineSpacing)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventClosePopup, true, superSpacing, blbl));
}

void GuiGen::openPopupPiecePicker(uint16 piecePicksLeft) const {
	Widget* defSel = nullptr;
	vector<Widget*> bot[2] = { vector<Widget*>(5), vector<Widget*>(5) };
	for (uint8 r = 0, t = 0; r < 2; ++r)
		for (uint8 i = 0; i < 5; ++i, ++t) {
			bool more = World::game()->board.ownPieceAmts[t] < World::game()->board.config.pieceAmounts[t];
			bot[r][i] = new Button(superHeight, more ? &Program::eventSetupPickPiece : nullptr, nullptr, makeTooltip(firstUpper(Piece::names[t]).c_str()), more ? 1.f : defaultDim, World::scene()->texture(Piece::names[t]), vec4(1.f));
			if (more && !defSel)
				defSel = bot[r][i];
		}

	int plen = superHeight * 5 + lineSpacing * 4;
	vector<Widget*> mid = {
		new Layout(superHeight, std::move(bot[0]), false, lineSpacing),
		new Layout(superHeight, std::move(bot[1]), false, lineSpacing)
	};
	vector<Widget*> cont = {
		new Widget(),
		new Layout(plen, std::move(mid), true, lineSpacing),
		new Widget()
	};

	Text ms(msgPickPiece + toStr(piecePicksLeft) + ')', superHeight);
	vector<Widget*> con = {
		new Label(superHeight, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(cont), false, 0)
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(std::max(ms.length, plen), superHeight * 3 + lineSpacing * 2), std::move(con), nullptr, nullptr, true, lineSpacing, defSel));
}

// MATCH

uptr<RootLayout> GuiGen::makeMatch(Interactable*& selected, MatchIO& mio, Icon*& bswapIcon, Navigator*& planeSwitch, uint16& unplacedDragons) const {
	// sidebar
	initlist<const char*> sidt = {
		"Exit",
		"Surrender",
		"Config",
		"Chat",
		"Engage",
		"Destroy",
		"Finish",
		"Establish",
		"Rebuild",
		"Spawn"
	};
	initlist<const char*>::iterator isidt = sidt.begin();
	int sideLength = std::max(Text::maxLen(sidt.begin(), sidt.end(), lineHeight), iconSize * 2 + lineSpacing * 2);	// second argument is length of FF line

	vector<Widget*> left = {
		new Label(lineHeight, *isidt++, &Program::eventAbortGame, nullptr, makeTooltip("Exit the game")),
		new Label(lineHeight, *isidt++, &Program::eventSurrender, nullptr, makeTooltip("Surrender and quit the match")),
		new Label(lineHeight, *isidt++, &Program::eventShowConfig, nullptr, makeTooltip("Show current game configuration")),
		new Label(lineHeight, *isidt++, &Program::eventToggleChat, nullptr, makeTooltip("Toggle chat")),
		bswapIcon = new Icon(lineHeight, *isidt++, &Program::eventSwitchGameButtons, nullptr, nullptr, makeTooltip("Switch between engaging and moving a piece")),
		mio.destroy = new Icon(lineHeight, *isidt++, &Program::eventSwitchDestroy, nullptr, nullptr, makeTooltip("Destroy a tile when moving off of it")),
		mio.turn = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Finish current turn"))
	};
	selected = mio.turn;
	if (World::game()->board.config.opts & Config::homefront) {
		left.insert(left.end() - 1, {
			mio.establish = new Icon(lineHeight, *isidt++, nullptr, nullptr, nullptr, makeTooltip("Respawn a piece")),
			mio.rebuild = new Icon(lineHeight, *isidt++, nullptr, nullptr, nullptr, makeTooltip("Rebuild a fortress or farm")),
			mio.spawn = new Icon(lineHeight, *isidt++, nullptr, nullptr, nullptr, makeTooltip("Establish a farm or city"))
		});
	} else
		mio.rebuild = mio.establish = mio.spawn = nullptr;
	if (World::game()->board.config.favorLimit && World::game()->board.ownPieceAmts[Piece::throne]) {
		for (sizet i = 0; i < mio.favors.size(); ++i)
			mio.favors[i] = new Icon(iconSize, string(), nullptr, nullptr, nullptr, makeTooltip(firstUpper(favorNames[i]).c_str()), 1.f, Label::Alignment::center, true, World::scene()->texture(favorNames[i]), vec4(1.f));
		left.push_back(new Widget(0));
		for (sizet i = 0; i < mio.favors.size(); i += 2)
			left.push_back(new Layout(iconSize, { new Widget(0), mio.favors[i], mio.favors[i+1] }, false, lineSpacing));
	} else
		std::fill(mio.favors.begin(), mio.favors.end(), nullptr);

	unplacedDragons = 0;
	for (Piece* pce = World::game()->board.getOwnPieces(Piece::dragon); pce->getType() == Piece::dragon; ++pce)
		if (!pce->show)
			++unplacedDragons;
	if (unplacedDragons) {
		mio.dragon = new Icon(iconSize, string(), nullptr, nullptr, nullptr, makeTooltip((string("Place ") + Piece::names[Piece::dragon]).c_str()), 1.f, Label::Alignment::left, true, World::scene()->texture(Piece::names[Piece::dragon]), vec4(1.f));
		left.push_back(new Layout(iconSize, { mio.dragon }, false, 0));
	} else
		mio.dragon = nullptr;

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, std::move(left), true, lineSpacing),
		nullptr
	};
	if (World::game()->board.config.opts & Config::victoryPoints) {
		int plen = Text::strLen(string(numDigits(World::game()->board.config.victoryPointsNum), '0').c_str(), lineHeight);
		Text tit("Victory Points", lineHeight);
		vector<Widget*> topb = {
			new Widget(),
			mio.vpOwn = new Label(plen, "0", nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
			new Label(tit.length, std::move(tit.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center, false),
			mio.vpEne = new Label(plen, "0", nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
			new Widget(),
			new Widget(sideLength)
		};
		vector<Widget*> midf = {
			new Widget(superSpacing),
			new Layout(lineHeight, std::move(topb), false, lineSpacing),
			planeSwitch = new Navigator()
		};
		cont.back() = new Layout(1.f, std::move(midf));
	} else {
		mio.vpOwn = mio.vpEne = nullptr;
		cont.back() = planeSwitch = new Navigator();
	}
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, 0);	// interactive icons need to be updated once the game match has been set up
}

void GuiGen::openPopupSpawner() const {
	Widget* defSel = nullptr;
	vector<Widget*> bot[2] = { vector<Widget*>(4), vector<Widget*>(4) };
	for (uint8 r = 0, t = 0; r < 2; ++r)
		for (uint8 i = 0; i < 4; ++i, ++t) {
			bool on = World::game()->board.pieceSpawnable(Piece::Type(t));
			bot[r][i] = new Button(superHeight, on ? BCall(&Program::eventSpawnPiece) : nullptr, nullptr, makeTooltip(firstUpper(Piece::names[t]).c_str()), on ? 1.f : defaultDim, World::scene()->texture(Piece::names[t]), vec4(1.f));
			if (on && !defSel)
				defSel = bot[r][i];
		}

	int plen = superHeight * 4 + lineSpacing * 3;
	vector<Widget*> mid = {
		new Layout(superHeight, std::move(bot[0]), false, lineSpacing),
		new Layout(superHeight, std::move(bot[1]), false, lineSpacing)
	};
	vector<Widget*> cont = {
		new Widget(),
		new Layout(plen, std::move(mid)),
		new Widget()
	};

	Text ms("Spawn piece", superHeight);
	Label* cancel = new Label(1.f, "Cancel", &Program::eventClosePopupResetIcons, nullptr, Texture(), 1.f, Label::Alignment::center);
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(superHeight * 2, std::move(cont), false),
		cancel
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(std::max(ms.length, plen), superHeight * 4 + lineSpacing * 2), std::move(con), nullptr, &Program::eventClosePopupResetIcons, true, lineSpacing, defSel ? defSel : cancel));
}

// SETTINGS

uptr<RootLayout> GuiGen::makeSettings(Interactable*& selected, ScrollArea*& content, sizet& bindingsStart, umap<string, uint32>& pixelformats) const {
	// side bar
	initlist<const char*> tps = {
		"Back",
		"Info",
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		"Rules",
		"Docs",
#endif
		"Reset"
	};
	initlist<const char*>::iterator itps = tps.begin();
	int optLength = Text::maxLen(tps.begin(), tps.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, *itps++, &Program::eventOpenMainMenu),
		new Label(lineHeight, *itps++, &Program::eventOpenInfo),
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		new Label(lineHeight, *itps++, &Program::eventOpenRules),
		new Label(lineHeight, *itps++, &Program::eventOpenDocs),
#endif
		new Widget(0),
		new Label(lineHeight, *itps++, &Program::eventResetSettings)
	};
	selected = lft.front();

	// resolution list
	vector<ivec2> sizes = World::window()->windowSizes();
	vector<SDL_DisplayMode> modes = World::window()->displayModes();
	pixelformats.clear();
	for (const SDL_DisplayMode& it : modes)
		pixelformats.emplace(pixelformatName(it.format), it.format);
	vector<string> winsiz(sizes.size()), dmodes(modes.size());
	std::transform(sizes.begin(), sizes.end(), winsiz.begin(), [](ivec2 size) -> string { return toStr(size, rv2iSeparator); });
	std::transform(modes.begin(), modes.end(), dmodes.begin(), dispToFstr);

	// setting buttons, labels and action fields for labels
	initlist<const char*> txs = {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		"Display",
#endif
#ifndef __ANDROID__
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
		"Color ally",
		"Color enemy",
		"Scale tiles",
		"Scale pieces",
		"Auto victory points",
		"Show tooltips",
		"Chat line limit",
		"Deadzone",
		"Resolve family",
		"Regular font",
		"Invert mouse wheel"
	};
	initlist<const char*>::iterator itxs = txs.begin();

	bindingsStart = txs.size() + 1;
	array<string, Binding::names.size()> bnames;
	for (uint8 i = 0; i < uint8(Binding::names.size()); ++i)
		bnames[i] = bindingToFstr(Binding::Type(i));
	int descLength = std::max(Text::maxLen(txs.begin(), txs.end(), lineHeight), Text::maxLen(bnames.begin(), bnames.end(), lineHeight));

	constexpr char shadowTip[] = "Width/height of shadow cubemap";
	constexpr char scaleTip[] = "Scale factor of texture sizes";
	constexpr char vsyncTip[] = "Immediate: off\n"
		"Synchronized: on\n"
		"Adaptive: on and smooth (works on fewer computers)";
	constexpr char gammaTip[] = "Brightness";
	constexpr char volumeTip[] = "Audio volume";
	constexpr char chatTip[] = "Line break limit of a chat box";
	constexpr char deadzTip[] = "Controller axis deadzone";
	int slleLength = Text::strLen(string(numDigits(uint(UINT16_MAX)), '0').c_str(), lineHeight) + LabelEdit::caretWidth;

	vector<Widget*> lx[] = { {
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, toStr(World::sets()->display), nullptr, nullptr, &Program::eventSetDisplay, nullptr, makeTooltip("Keep 0 cause it doesn't really work"))
	}, {
#endif
#ifndef __ANDROID__
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Settings::screenNames[uint8(World::sets()->screen)], vector<string>(Settings::screenNames.begin(), Settings::screenNames.end()), &Program::eventSetScreen, nullptr, nullptr, makeTooltip("Window or fullscreen (\"desktop\" is native resolution)"))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, toStr(World::sets()->size, rv2iSeparator), std::move(winsiz), &Program::eventSetWindowSize, nullptr, nullptr, makeTooltip("Window size"))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, dispToFstr(World::sets()->mode), std::move(dmodes), &Program::eventSetWindowMode, nullptr, nullptr, makeTooltip("Fullscreen display properties"))
	}, {
#endif
#ifndef OPENGLES
		new Label(descLength, *itxs++),
		new ComboBox(1.f, toStr(World::sets()->msamples), { "0", "1", "2", "4" }, &Program::eventSetSamples, nullptr, nullptr, makeTooltip("Anti-Aliasing multisamples (requires restart)"))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->shadowRes ? int(std::log2(World::sets()->shadowRes)) : -1, -1, 15, 1, nullptr, &Program::eventSetShadowResSL, makeTooltip(shadowTip)),
		new LabelEdit(slleLength, toStr(World::sets()->shadowRes), &Program::eventSetShadowResLE, nullptr, nullptr, nullptr, makeTooltip(shadowTip))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->softShadows, &Program::eventSetSoftShadows, &Program::eventSetSoftShadows, makeTooltip("Soft shadow edges"))
	}, {
#endif
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->texScale, 1, 100, 10, &Program::eventSetTexturesScaleSL, &Program::eventPrcSliderUpdate, makeTooltip(scaleTip)),
		new LabelEdit(slleLength, toStr(World::sets()->texScale) + '%', &Program::eventSetTextureScaleLE, nullptr, nullptr, nullptr, makeTooltip(scaleTip))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Settings::vsyncNames[uint8(World::sets()->vsync+1)], vector<string>(Settings::vsyncNames.begin(), Settings::vsyncNames.end()), &Program::eventSetVsync, nullptr, nullptr, makeTooltipL(vsyncTip))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, int(World::sets()->gamma * gammaStepFactor), 0, int(Settings::gammaMax * gammaStepFactor), int(gammaStepFactor), &Program::eventSaveSettings, &Program::eventSetGammaSL, makeTooltip(gammaTip)),
		new LabelEdit(slleLength, toStr(World::sets()->gamma), &Program::eventSetGammaLE, nullptr, nullptr, nullptr, makeTooltip(gammaTip))
	}, World::audio() ? vector<Widget*>{
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->avolume, 0, SDL_MIX_MAXVOLUME, 8, &Program::eventSaveSettings, &Program::eventSetVolumeSL, makeTooltip(volumeTip)),
		new LabelEdit(slleLength, toStr(World::sets()->avolume), &Program::eventSetVolumeLE, nullptr, nullptr, nullptr, makeTooltip(volumeTip))
	} : vector<Widget*>{
		new Label(descLength, *itxs++),
		new Label(1.f, "missing audio device")
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Settings::colorNames[uint8(World::sets()->colorAlly)], vector<string>(Settings::colorNames.begin(), Settings::colorNames.end()), &Program::eventSetColorAlly, nullptr, nullptr, makeTooltip("Color of own pieces"))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Settings::colorNames[uint8(World::sets()->colorEnemy)], vector<string>(Settings::colorNames.begin(), Settings::colorNames.end()), &Program::eventSetColorEnemy, nullptr, nullptr, makeTooltip("Color of enemy pieces"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->scaleTiles, &Program::eventSetScaleTiles, &Program::eventSetScaleTiles, makeTooltip("Scale tile amounts when resizing board"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->scalePieces, &Program::eventSetScalePieces, &Program::eventSetScalePieces, makeTooltip("Scale piece amounts when resizing board"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->autoVictoryPoints, &Program::eventSetAutoVictoryPoints, &Program::eventSetAutoVictoryPoints, makeTooltip("Automatically adjust the configuration when toggling the victory points game variant"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->tooltips, &Program::eventSetTooltips, &Program::eventSetTooltips, makeTooltip("Display tooltips on hover"))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->chatLines, 0, Settings::chatLinesMax, 256, &Program::eventSetChatLineLimitSL, &Program::eventSLUpdateLE, makeTooltip(chatTip)),
		new LabelEdit(slleLength, toStr(World::sets()->chatLines), &Program::eventSetChatLineLimitLE, nullptr, nullptr, nullptr, makeTooltip(chatTip))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->deadzone, Settings::deadzoneLimit.x, Settings::deadzoneLimit.y, 2048, &Program::eventSetDeadzoneSL, &Program::eventSLUpdateLE, makeTooltip(deadzTip)),
		new LabelEdit(slleLength, toStr(World::sets()->deadzone), &Program::eventSetDeadzoneLE, nullptr, nullptr, nullptr, makeTooltip(deadzTip))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Settings::familyNames[uint8(World::sets()->resolveFamily)], vector<string>(Settings::familyNames.begin(), Settings::familyNames.end()), &Program::eventSetResolveFamily, nullptr, nullptr, makeTooltip("What family to use for resolving a host address"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->fontRegular, &Program::eventSetFontRegular, &Program::eventSetFontRegular, makeTooltip("Use the standard Romanesque or Merriweather to support more characters"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->invertWheel, &Program::eventSetInvertWheel, &Program::eventSetInvertWheel, makeTooltip("Invert the mouse wheel motion"))
	}, };
	vector<Widget*> lns(bindingsStart + bnames.size());
	for (sizet i = 0; i < txs.size(); ++i)
		lns[i] = new Layout(lineHeight, std::move(lx[i]), false, lineSpacing);
	lns[txs.size()] = new Widget(0);

	for (uint8 i = 0; i < uint8(bnames.size()); ++i) {
		Text add("+", lineHeight);
		Label* lbl = new Label(lineHeight, std::move(bnames[i]));
		vector<Widget*> line = {
			new Layout(descLength, { lbl, new Widget() }),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).keys, KeyGetter::Accept::keyboard, lbl),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).joys, KeyGetter::Accept::joystick, lbl),
			createKeyGetterList(Binding::Type(i), World::input()->getBinding(Binding::Type(i)).gpds, KeyGetter::Accept::gamepad, lbl),
			new Layout(add.length, { new Label(lineHeight, std::move(add.text), &Program::eventAddKeyBinding, nullptr, makeTooltip(("Add a " + lbl->getText() + " binding").c_str()), 1.f, Label::Alignment::center) })
		};
		lns[bindingsStart+i] = new Layout(keyGetLineSize(Binding::Type(i)), std::move(line), false, lineSpacing);
	}

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		content = new ScrollArea(1.f, std::move(lns), true, lineSpacing)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

template <class T>
Layout* GuiGen::createKeyGetterList(Binding::Type bind, const vector<T>& refs, KeyGetter::Accept type, Label* lbl) const {
	vector<Widget*> lst(refs.size());
	for (sizet i = 0; i < refs.size(); ++i)
		lst[i] = createKeyGetter(type, bind, i, lbl);
	return new Layout(1.f, std::move(lst), true, lineSpacing);
}

KeyGetter* GuiGen::createKeyGetter(KeyGetter::Accept accept, Binding::Type bind, sizet kid, Label* lbl) const {
	return new KeyGetter(lineHeight, accept, bind, kid, &Program::eventSaveSettings, &Program::eventDelKeyBinding, makeTooltip((lbl->getText() + ' ' + KeyGetter::acceptNames[uint8(accept)] + " binding").c_str()));
}

SDL_DisplayMode GuiGen::fstrToDisp(const umap<string, uint32>& pixelformats, const string& str) const {
	SDL_DisplayMode mode = strToDisp(str);
	if (string::const_reverse_iterator sit = std::find_if(str.rbegin(), str.rend(), isSpace); sit != str.rend())
		if (umap<string, uint32>::const_iterator pit = pixelformats.find(string(sit.base(), str.end())); pit != pixelformats.end())
			mode.format = pit->second;
	return mode;
}

int GuiGen::keyGetLineSize(Binding::Type bind) const {
	int cnt = std::max(int(std::max(std::max(World::input()->getBinding(bind).keys.size(), World::input()->getBinding(bind).joys.size()), World::input()->getBinding(bind).gpds.size())), 1);
	return lineHeight * cnt + lineSpacing * (cnt - 1);
}

void GuiGen::openPopupKeyGetter(Binding::Type bind) const {
	Text ms("Add " + bindingToFstr(bind) + " binding:", superHeight);
	KeyGetter* kget = new KeyGetter(1.f, KeyGetter::Accept::any, bind, SIZE_MAX, &Program::eventSetNewBinding, &Program::eventClosePopup);
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text)),
		kget
	};
	World::scene()->setPopup(std::make_unique<Popup>(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), &Program::eventClosePopup, &Program::eventClosePopup, true, lineSpacing), kget);
}

string GuiGen::bindingToFstr(Binding::Type bind) {
	string name = firstUpper(Binding::names[uint8(bind)]);
	std::replace(name.begin(), name.end(), '_', ' ');
	return name;
}

// INFO

uptr<RootLayout> GuiGen::makeInfo(Interactable*& selected, ScrollArea*& content) const {
	// side bar
	initlist<const char*> tps = {
		"Menu",
		"Back"
	};
	initlist<const char*>::iterator itps = tps.begin();
	int optLength = Text::maxLen(tps.begin(), tps.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, *itps++, &Program::eventOpenMainMenu),
		new Label(lineHeight, *itps++, &Program::eventOpenSettings)
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
#ifdef WEBUTILS
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
	int argWidth = Text::maxLen({ args, dispArgs, rendArgs, ctrlArgs }, lineHeight);

	vector<Widget*> lines;
	appendProgram(lines, argWidth, iargs, ititles);
	appendSystem(lines, argWidth, iargs, ititles);
	appendCurrentDisplay(lines, argWidth, dispArgs.begin(), ititles);
	appendPower(lines, argWidth, iargs, ititles);
	appendAudioDevices(lines, argWidth, ititles, SDL_FALSE);
	appendAudioDevices(lines, argWidth, ititles, SDL_TRUE);
	appendDrivers(lines, argWidth, ititles, SDL_GetNumAudioDrivers, SDL_GetAudioDriver);
	appendDrivers(lines, argWidth, ititles, SDL_GetNumVideoDrivers, SDL_GetVideoDriver);
	appendDisplays(lines, argWidth, Text::maxLen(dispArgs.begin(), dispArgs.end(), lineHeight), dispArgs.begin(), ititles);
	appendRenderers(lines, argWidth, rendArgs.begin(), ititles);
	appendControllers(lines, argWidth, ctrlArgs.begin(), ititles);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		content = new ScrollArea(1.f, std::move(lines), true, lineSpacing)
	};
	return std::make_unique<RootLayout>(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

void GuiGen::appendProgram(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const {
	lines.insert(lines.end(), {
		new Label(superHeight, *titles++),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, Com::commonVersion) }, false, lineSpacing)
	});
	if (char* basePath = SDL_GetBasePath()) {
		lines.push_back(new Layout(lineHeight, { new Label(width, *args), new Label(1.f, basePath) }, false, lineSpacing));
		SDL_free(basePath);
	}
	args++;

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

	SDL_version bver, icver, tcver;
	SDL_GetVersion(&bver);
	Text slv(versionText(bver), lineHeight);
	SDL_VERSION(&bver)
	const SDL_version* ilver = IMG_Linked_Version();
	SDL_IMAGE_VERSION(&icver)
	const SDL_version* tlver = TTF_Linked_Version();
	SDL_TTF_VERSION(&tcver)
#ifdef WEBUTILS
	const curl_version_info_data* cinf = curl_version_info(CURLVERSION_NOW);
	string clver;
	if (cinf->features & CURL_VERSION_IPV6)
		clver += " IPv6,";
#if CURL_AT_LEAST_VERSION(7, 10, 0)
	if (cinf->features & CURL_VERSION_SSL)
		clver += " SSL,";
	if (cinf->features & CURL_VERSION_LIBZ)
		clver += " libz,";
#endif
#if CURL_AT_LEAST_VERSION(7, 33, 0)
	if (cinf->features & CURL_VERSION_HTTP2)
		clver += " HTTP2,";
#endif
#if CURL_AT_LEAST_VERSION(7, 66, 0)
	if (cinf->features & CURL_VERSION_HTTP3)
		clver += " HTTP3,";
#endif
#if CURL_AT_LEAST_VERSION(7, 72, 0)
	if (cinf->features & CURL_VERSION_UNICODE)
		clver += " Unicode,";
#endif
	clver = clver.empty() ? cinf->version : cinf->version + string(" with") + clver.substr(0, clver.length() - 1);
#endif
	array<string, 4> compVers = {
		versionText(bver),
		versionText(icver),
		versionText(tcver),
#ifdef WEBUTILS
		LIBCURL_VERSION
#endif
	};
	array<string, 4>::iterator icmpver = compVers.begin();
	int rightLen = Text::maxLen(compVers.begin(), compVers.end(), lineHeight);

	lines.insert(lines.end(), {
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, SDL_GetPlatform()) }, false, lineSpacing),
#ifndef __APPLE__
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, std::move(glvers)) }, false, lineSpacing),
#endif
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, 'R' + toStr(red) + " G" + toStr(green) + " B" + toStr(blue) + " A" + toStr(alpha)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(buffer)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(dbuffer)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(depth)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(stencil)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, 'R' + toStr(ared) + " G" + toStr(agreen) + " B" + toStr(ablue) + " A" + toStr(aalpha)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(stereo)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(msbuffers)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(msamples)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(accvisual)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, std::move(cvers)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, std::move(sflags)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(swcc)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(srgb)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, ibtos(crb)) }, false, lineSpacing),
#if !defined(OPENGLES) && !defined(__APPLE__)
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, reinterpret_cast<const char*>(glewGetString(GLEW_VERSION))) }, false, lineSpacing),
#endif
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(GLM_VERSION_MAJOR) + '.' + toStr(GLM_VERSION_MINOR) + '.' + toStr(GLM_VERSION_PATCH) + '.' + toStr(GLM_VERSION_REVISION)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(slv.length, std::move(slv.text)), new Label(1.f, SDL_GetRevision()), new Label(rightLen, std::move(*icmpver++)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, versionText(*ilver)), new Label(rightLen, std::move(*icmpver++)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, versionText(*tlver)), new Label(rightLen, std::move(*icmpver++)) }, false, lineSpacing),
#ifdef WEBUTILS
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, std::move(clver)), new Label(rightLen, std::move(*icmpver++)) }, false, lineSpacing),
#endif
		new Widget(lineHeight)
	});
}

void GuiGen::appendSystem(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const {
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
		new Label(superHeight, *titles++),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(SDL_GetCPUCount())) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(SDL_GetCPUCacheLineSize()) + 'B') }, false, lineSpacing)
	});
	if (!features.empty()) {
		lines.push_back(new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, std::move(features[0])) }, false, lineSpacing));
		for (sizet i = 1; i < features.size(); ++i)
			lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, std::move(features[i])) }, false, lineSpacing));
		features.clear();
	} else
		lines.push_back(new Layout(lineHeight, { new Label(width, *args++), new Widget() }, false, lineSpacing));
	lines.insert(lines.end(), {
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, toStr(SDL_GetSystemRAM()) + "MB") }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, audio ? audio : "") }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, video ? video : "") }, false, lineSpacing),
		new Widget(lineHeight)
	});
}

void GuiGen::appendCurrentDisplay(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	lines.push_back(new Label(superHeight, *titles++));
	if (int i = World::window()->displayID(); i >= 0)
		appendDisplay(lines, i, width, args);
	lines.push_back(new Widget(lineHeight));
}

void GuiGen::appendDisplay(vector<Widget*>& lines, int i, int width, initlist<const char*>::iterator args) const {
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

void GuiGen::appendPower(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const {
	const char* powerNames[] = {
		"UNKNOWN",
		"ON_BATTERY",
		"NO_BATTERY",
		"CHARGING",
		"CHARGED"
	};
	int secs, pct;
	SDL_PowerState power = SDL_GetPowerInfo(&secs, &pct);
	Text tprc(pct >= 0 ? toStr(pct) + '%' : World::sets()->fontRegular ? "inf" : u8"\u221E", lineHeight);

	lines.insert(lines.end(), {
		new Label(superHeight, *titles++),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, powerNames[power]) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(tprc.length, std::move(tprc.text)), new Label(1.f, secs >= 0 ? toStr(secs) + 's' : World::sets()->fontRegular ? "inf" : u8"\u221E") }, false, lineSpacing),
		new Widget(lineHeight)
	});
}

void GuiGen::appendAudioDevices(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int iscapture) const {
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < SDL_GetNumAudioDevices(iscapture); ++i)
		if (const char* name = SDL_GetAudioDeviceName(i, iscapture))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void GuiGen::appendDrivers(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) const {
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < limit(); ++i)
		if (const char* name = value(i))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void GuiGen::appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	int sidew = dispWidth + lineSpacing < argWidth ? argWidth - dispWidth - lineSpacing : 0;
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
		appendDisplay(lines, i, argWidth, args);

		lines.push_back(new Layout(lineHeight, { new Label(1.f, args[8]) }, false, lineSpacing));
		for (int j = 0; j < SDL_GetNumDisplayModes(i); ++j) {
			lines.push_back(new Layout(lineHeight, { new Widget(sidew), new Label(dispWidth, args[0]), new Label(1.f, toStr(j)) }, false, lineSpacing));
			if (SDL_DisplayMode mode; !SDL_GetDisplayMode(i, j, &mode))
				lines.insert(lines.end(), {
					new Layout(lineHeight, { new Widget(sidew), new Label(dispWidth, args[2]), new Label(1.f, toStr(mode.w) + " x " + toStr(mode.h)) }, false, lineSpacing),
					new Layout(lineHeight, { new Widget(sidew), new Label(dispWidth, args[3]), new Label(1.f, toStr(mode.refresh_rate)) }, false, lineSpacing),
					new Layout(lineHeight, { new Widget(sidew), new Label(dispWidth, args[4]), new Label(1.f, pixelformatName(mode.format)) }, false, lineSpacing)
				});
			if (j < SDL_GetNumDisplayModes(i) - 1)
				lines.push_back(new Widget(lineSpacing));
		}
		if (i < SDL_GetNumVideoDisplays() - 1)
			lines.push_back(new Widget(lineSpacing));
	}
	lines.push_back(new Widget(lineHeight));
}

void GuiGen::appendRenderers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
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
				for (sizet j = 1; j < flags.size(); ++j)
					lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, std::move(flags[j])) }, false, lineSpacing));
			} else
				lines.push_back(new Layout(lineHeight, { new Label(width, args[2]), new Widget() }, false, lineSpacing));

			lines.push_back(new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, toStr(info.max_texture_width) + " x " + toStr(info.max_texture_height)) }, false, lineSpacing));
			if (info.num_texture_formats) {
				lines.push_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, pixelformatName(info.texture_formats[0])) }, false, lineSpacing));
				for (uint32 j = 1; j < info.num_texture_formats; ++j)
					lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, pixelformatName(info.texture_formats[j])) }, false, lineSpacing));
			} else
				lines.push_back(new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, toStr(i)) }, false, lineSpacing));
		}
		if (i < SDL_GetNumRenderDrivers() - 1)
			lines.push_back(new Widget(lineSpacing));
	}
	lines.push_back(new Widget(lineHeight));
}

void GuiGen::appendControllers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const {
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

	lines.push_back(new Label(superHeight, *titles++));
	vector<SDL_Joystick*> joys = World::input()->listJoysticks();
	for (sizet i = 0; i < joys.size(); ++i) {
		const char* name = SDL_JoystickName(joys[i]);
		char guid[33];
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joys[i]), guid, sizeof(guid));
		lines.insert(lines.end(), {
			new Layout(lineHeight, { new Label(width, args[0]), new Label(1.f, toStr(SDL_JoystickInstanceID(joys[i]))) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[1]), new Label(1.f, name ? name : "") }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[2]), new Label(1.f, guid) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[3]), new Label(1.f, typeNames[SDL_JoystickGetType(joys[i])]) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[4]), new Label(1.f, toStr(SDL_JoystickNumButtons(joys[i]))) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[5]), new Label(1.f, toStr(SDL_JoystickNumHats(joys[i]))) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[6]), new Label(1.f, toStr(SDL_JoystickNumAxes(joys[i]))) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[7]), new Label(1.f, toStr(SDL_JoystickNumBalls(joys[i]))) }, false, lineSpacing),
			new Layout(lineHeight, { new Label(width, args[8]), new Label(1.f, powerNames[SDL_JoystickCurrentPowerLevel(joys[i])+1]) }, false, lineSpacing)
		});
		if (i < joys.size() - 1)
			lines.push_back(new Widget(lineSpacing));
	}
}

string GuiGen::ibtos(int val) {
	if (!val)
		return "off";
	if (val == 1)
		return "on";
	return toStr(val);
}
