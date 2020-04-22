#include "engine/world.h"
#ifdef WEBUTILS
#include <curl/curl.h>
#endif

// PROGRAM TEXT

ProgState::Text::Text(string str, int sh) :
	text(std::move(str)),
	length(strLen(text.c_str(), sh)),
	height(sh)
{}

int ProgState::Text::strLen(const char* str, int height) {
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

int ProgState::Text::maxLen(const initlist<initlist<const char*>>& lists, int height) {
	int width = 0;
	for (initlist<const char*> it : lists)
		if (int len = maxLen(it.begin(), it.end(), height); len > width)
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
	World::scene()->camera.setPos(Camera::posSetup, Camera::latSetup);
}

void ProgState::eventResize() {
	smallHeight = World::window()->getScreenView().y / 36;		// 20p	(values are in relation to 720p height)
	lineHeight = World::window()->getScreenView().y / 24;		// 30p
	superHeight = World::window()->getScreenView().y / 18;		// 40p
	tooltipHeight = World::window()->getScreenView().y / 45;	// 16p
	lineSpacing = World::window()->getScreenView().y / 144;	// 5p
	superSpacing = World::window()->getScreenView().y / 72;	// 10p
	iconSize = World::window()->getScreenView().y / 11;		// 64p
	tooltipLimit = World::window()->getScreenView().x * 2 / 3;
}

uint8 ProgState::switchButtons(uint8 but) {
	return but;
}

vector<Overlay*> ProgState::createOverlays() {
	return World::netcp() ? vector<Overlay*>{
		createNotification(),
		createFpsCounter()
	} : vector<Overlay*>{
		createFpsCounter()
	};
}

Popup* ProgState::createPopupMessage(string msg, BCall ccal, string ctxt) const {
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
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), ccal, ccal, true, lineSpacing);
}

Popup* ProgState::createPopupChoice(string msg, BCall kcal, BCall ccal) const {
	Text ms(std::move(msg), superHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, lineSpacing)
	};
	return new Popup(pair(ms.length, superHeight * 2 + lineSpacing), std::move(con), kcal, ccal, true, lineSpacing);
}

pair<Popup*, Widget*> ProgState::createPopupInput(string msg, BCall kcal, uint limit) const {
	LabelEdit* ledit = new LabelEdit(1.f, string(), kcal, &Program::eventClosePopup, nullptr, nullptr, Texture(), 1.f, limit);
	vector<Widget*> bot = {
		new Label(1.f, "Okay", kcal, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Cancel", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(msg)),
		ledit,
		new Layout(1.f, std::move(bot), false, 0)
	};
	return pair(new Popup(pair(500, superHeight * 3 + lineSpacing * 2), std::move(con), kcal, &Program::eventClosePopup, true, lineSpacing), ledit);
}

Popup* ProgState::createPopupConfig(const Com::Config& cfg, ScrollArea*& cfgList) const {
	ConfigIO wio;
	vector<Widget*> bot = {
		new Widget(),
		new Label(1.f, "Close", &Program::eventClosePopup, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		cfgList = new ScrollArea(1.f, createConfigList(wio, cfg, false), true, lineSpacing),
		new Layout(lineHeight, std::move(bot), false, 0)
	};
	return new Popup(pair(0.6f, 0.8f), std::move(con), nullptr, &Program::eventCloseConfigList, true, lineSpacing);
}

Texture ProgState::makeTooltip(const char* text) const {
	return World::sets()->tooltips ? World::fonts()->render(text, tooltipHeight, tooltipLimit) : nullptr;
}

Texture ProgState::makeTooltipL(const char* text) const {
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

vector<Widget*> ProgState::createChat(bool overlay) {
	return {
		chatBox = new TextBox(1.f, smallHeight, string(), &Program::eventFocusChatLabel, &Program::eventClearLabel, Texture(), 1.f, World::sets()->chatLines, false, true, 0, Widget::colorDimmed),
		new LabelEdit(smallHeight, string(), nullptr, nullptr, &Program::eventSendMessage, overlay ? &Program::eventCloseChat : &Program::eventHideChat, Texture(), 1.f, UINT16_MAX - Com::dataHeadSize, true)
	};
}

void ProgState::toggleChatEmbedShow() {
	if (chatBox->getParent()->relSize.usePix) {
		chatBox->getParent()->relSize = chatEmbedSize;
		chatBox->getParent()->getParent()->setSpacing(chatBox->getParent()->relSize.usePix ? 0 : lineSpacing);
		static_cast<LabelEdit*>(chatBox->getParent()->getWidget(chatBox->getIndex() + 1))->onClick(ivec2(0), SDL_BUTTON_LEFT);
		notification->setShow(false);
	} else {
		static_cast<LabelEdit*>(chatBox->getParent()->getWidget(chatBox->getIndex() + 1))->cancel();
		hideChatEmbed();
	}
}

void ProgState::hideChatEmbed() {
	chatBox->getParent()->relSize = 0;
	chatBox->getParent()->getParent()->setSpacing(chatBox->getParent()->relSize.usePix ? 0 : lineSpacing);
}

vector<Widget*> ProgState::createConfigList(ConfigIO& wio, const Com::Config& cfg, bool active) const {
	BCall update = active ? &Program::eventUpdateConfig : nullptr;
	//CCall cupdat = active ? &Program::eventUpdateConfigC : nullptr;
	initlist<const char*> txs = {
		//"Game type",
		"Ports",
		"Row balancing",
		"Set piece battle",
		"Homeland size",
		"Battle win chance",
		"Fate's favor maximum",
		"Fate's favor total",
		"Dragon place late",
		"Dragon move straight",
		"Fortresses captured",
		"Thrones killed",
		"Capturers"
	};
	initlist<const char*>::iterator itxs = txs.begin();
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);

	constexpr char ppTip[] = "Limit the number of pieces to be chosen";
	string scTip = "Chance of breaching a " + string(Com::tileNames[uint8(Com::Tile::fortress)]);
	string tileTips[Com::tileLim];
	for (uint8 i = 0; i < Com::tileLim; i++)
		tileTips[i] = "Number of " + string(Com::tileNames[i]) + " tiles per homeland";
	constexpr char fortressTip[] = "(calculated automatically to fill the remaining free tiles)";
	string middleTips[Com::tileLim];
	for (uint8 i = 0; i < Com::tileLim; i++)
		middleTips[i] = "Number of " + string(Com::tileNames[i]) + " tiles in the middle row per player";
	string pieceTips[Com::pieceMax];
	for (uint8 i = 0; i < Com::pieceMax; i++)
		pieceTips[i] = "Number of " + string(Com::pieceNames[i]) + " pieces per player";
	constexpr char fortTip[] = "Number of fortresses that need to be captured in order to win";
	constexpr char throneTip[] = "Number of thrones that need to be killed in order to win";
	Text width("width:", lineHeight);
	Text height("height:", lineHeight);
	Size amtWidth = update ? Size(Text::strLen(toStr(UINT16_MAX).c_str(), lineHeight) + LabelEdit::caretWidth) : Size(1.f);

	vector<vector<Widget*>> lines0 = { {
		/*new Label(descLength, *itxs++),
		wio.gameType = new ComboBox(1.f, Com::Config::gameTypeNames[uint8(cfg.gameType)], active ? vector<string>(Com::Config::gameTypeNames.begin(), Com::Config::gameTypeNames.end()) : vector<string>{ Com::Config::gameTypeNames[uint8(cfg.gameType)] }, cupdat, nullptr, nullptr, makeTooltip("Type of gameplay"))
	}, {*/
		new Label(descLength, *itxs++),
		wio.ports = new CheckBox(lineHeight, cfg.ports, update, update, makeTooltip("Use the ports game variant"))
	}, {
		new Label(descLength, *itxs++),
		wio.rowBalancing = new CheckBox(lineHeight, cfg.rowBalancing, update, update, makeTooltip("Have at least one of each tile type in each row"))
	}, {
		new Label(descLength, *itxs++),
		wio.setPieceOn = new CheckBox(lineHeight, cfg.setPieceOn, update, update, makeTooltip(ppTip)),
		wio.setPieceNum = new LabelEdit(1.f, toStr(cfg.setPieceNum), update, nullptr, nullptr, nullptr, makeTooltip(ppTip)),
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
		wio.favorTotal = new CheckBox(lineHeight, cfg.favorTotal, update, update, makeTooltip("Limit the total amount of fate's favors for the entire match"))
	}, {
		new Label(descLength, *itxs++),
		wio.dragonLate = new CheckBox(lineHeight, cfg.dragonLate, update, update, makeTooltip((firstUpper(Com::pieceNames[uint8(Com::Piece::dragon)]) + " can be placed later during the match on a homeland " + Com::tileNames[uint8(Com::Tile::fortress)]).c_str()))
	}, {
		new Label(descLength, *itxs++),
		wio.dragonStraight = new CheckBox(lineHeight, cfg.dragonStraight, update, update, makeTooltip((firstUpper(Com::pieceNames[uint8(Com::Piece::dragon)]) + " moves in a straight line").c_str()))
	} };
	if (active)
		lines0[4].insert(lines0[4].begin() + 1, wio.battleSL = new Slider(1.f, cfg.battlePass, 0, Com::Config::randomLimit, 10, update, &Program::eventPrcSliderUpdate, makeTooltip(scTip.c_str())));

	vector<vector<Widget*>> lines1(Com::tileLim + 1);
	for (uint8 i = 0; i < Com::tileLim; i++)
		lines1[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.tiles[i] = new LabelEdit(amtWidth, toStr(cfg.tileAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(tileTips[i].c_str()) : makeTooltipL((tileTips[i] + '\n' + fortressTip).c_str()))
		};
	lines1.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.tileFortress = new Label(1.f, tileFortressString(cfg), nullptr, nullptr, makeTooltipL(("Number of " + string(Com::tileNames[uint8(Com::Tile::fortress)]) + " tiles per homeland\n" + fortressTip).c_str()))
	};
	if (active) {
		uint16 rest = cfg.countFreeTiles();
		for (uint8 i = 0; i < Com::tileLim; i++)
			lines1[i].insert(lines1[i].begin() + 1, new Slider(1.f, cfg.tileAmounts[i], cfg.homeSize.y, cfg.tileAmounts[i] + rest, 1, update, &Program::eventTileSliderUpdate, makeTooltip(tileTips[i].c_str())));
	}

	vector<vector<Widget*>> lines2(Com::tileLim + 1);
	for (uint8 i = 0; i < Com::tileLim; i++)
		lines2[i] = {
			new Label(descLength, firstUpper(Com::tileNames[i])),
			wio.middles[i] = new LabelEdit(amtWidth, toStr(cfg.middleAmounts[i]), update, nullptr, nullptr, nullptr, i < lines1.size() - 1 ? makeTooltip(middleTips[i].c_str()) : makeTooltipL((tileTips[i] + '\n' + fortressTip).c_str()))
	};
	lines2.back() = {
		new Label(descLength, firstUpper(Com::tileNames[uint8(Com::Tile::fortress)])),
		wio.middleFortress = new Label(1.f, middleFortressString(cfg), nullptr, nullptr, makeTooltipL(("Number of " + string(Com::tileNames[uint8(Com::Tile::fortress)]) + " tiles in the middle row\n" + fortressTip).c_str()))
	};
	if (active) {
		uint16 rest = cfg.countFreeMiddles();
		for (sizet i = 0; i < lines2.size() - 1; i++)
			lines2[i].insert(lines2[i].begin() + 1, new Slider(1.f, cfg.middleAmounts[i], 0, cfg.middleAmounts[i] + rest, 1, update, &Program::eventMiddleSliderUpdate, makeTooltip(middleTips[i].c_str())));
	}

	vector<vector<Widget*>> lines3(Com::pieceMax + 1);
	for (uint8 i = 0; i < Com::pieceMax; i++)
		lines3[i] = {
			new Label(descLength, firstUpper(Com::pieceNames[i])),
			wio.pieces[i] = new LabelEdit(amtWidth, toStr(cfg.pieceAmounts[i]), update, nullptr, nullptr, nullptr, makeTooltip(pieceTips[i].c_str()))
		};
	lines3.back() = {
		new Label(descLength, "Total"),
		wio.pieceTotal = new Label(1.f, pieceTotalString(cfg), nullptr, nullptr, makeTooltip("Total amount of pieces out of the maximum possible number of pieces per player"))
	};
	if (active) {
		uint16 rest = cfg.countFreePieces();
		for (sizet i = 0; i < lines3.size() - 1; i++)
			lines3[i].insert(lines3[i].begin() + 1, new Slider(1.f, cfg.pieceAmounts[i], 0, cfg.pieceAmounts[i] + rest, 1, update, &Program::eventPieceSliderUpdate, makeTooltip(pieceTips[i].c_str())));
	} else if (World::netcp() && World::game()->board.config.setPieceOn && World::game()->board.config.setPieceNum < World::game()->board.config.countPieces()) {
		bool full = dynamic_cast<ProgMatch*>(World::state());
		for (uint8 i = 0; i < Com::pieceMax; i++)
			if (lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board.ownPieceAmts[i]), nullptr, nullptr, makeTooltip(("Number of ally " + string(Com::pieceNames[i]) + " pieces").c_str()))); full)
				lines3[i].push_back(new Label(amtWidth, toStr(World::game()->board.enePieceAmts[i]), nullptr, nullptr, makeTooltip(("Number of enemy " + string(Com::pieceNames[i]) + " pieces").c_str())));
	}

	vector<vector<Widget*>> lines4 = { {
		new Label(descLength, *itxs++),
		wio.winFortress = new LabelEdit(amtWidth, toStr(cfg.winFortress), update, nullptr, nullptr, nullptr, makeTooltip(fortTip))
	}, {
		new Label(descLength, *itxs++),
		wio.winThrone = new LabelEdit(amtWidth, toStr(cfg.winThrone), update, nullptr, nullptr, nullptr, makeTooltip(throneTip))
	}, {
		new Label(descLength, *itxs++),
		wio.capturers = new LabelEdit(1.f, cfg.capturersString(), update, nullptr, nullptr, nullptr, makeTooltip("Pieces that are able to capture fortresses"))
	} };
	if (active) {
		lines4[0].insert(lines4[0].begin() + 1, new Slider(1.f, cfg.winFortress, 0, cfg.countFreeTiles(), 1, update, &Program::eventSLUpdateLE, makeTooltip(fortTip)));
		lines4[1].insert(lines4[1].begin() + 1, new Slider(1.f, cfg.winThrone, 0, cfg.pieceAmounts[uint8(Com::Piece::throne)], 1, update, &Program::eventSLUpdateLE, makeTooltip(throneTip)));
	}

	sizet id = 0;
	vector<Widget*> menu(lines0.size() + lines1.size() + lines2.size() + lines3.size() + lines4.size() + 4);	// 4 title bars
	setConfigLines(menu, lines0, id);
	setConfigTitle(menu, "Tile amounts", id);
	setConfigLines(menu, lines1, id);
	setConfigTitle(menu, "Middle amounts", id);
	setConfigLines(menu, lines2, id);
	setConfigTitle(menu, "Piece amounts", id);
	setConfigLines(menu, lines3, id);
	setConfigTitle(menu, "Winning conditions", id);
	setConfigLines(menu, lines4, id);
	return menu;
}

void ProgState::setConfigLines(vector<Widget*>& menu, vector<vector<Widget*>>& lines, sizet& id) const {
	for (vector<Widget*>& it : lines)
		menu[id++] = new Layout(lineHeight, std::move(it), false, lineSpacing);
}

void ProgState::setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const {
	int tlen = Text::strLen(title.c_str(), lineHeight);
	vector<Widget*> line = {
		new Label(tlen, std::move(title)),
		new Widget()
	};
	menu[id++] = new Layout(lineHeight, std::move(line), false, lineSpacing);
}

Overlay* ProgState::createFpsCounter() {
	Text txt = makeFpsText(World::window()->getDeltaSec());
	fpsText = new Label(1.f, std::move(txt.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::left, false);
	return new Overlay(pair(0, 0.959f), pair(txt.length, lineHeight), { fpsText }, nullptr, nullptr, true, World::program()->ftimeMode != Program::FrameTime::none, false, 0, Widget::colorDark);
}

Overlay* ProgState::createNotification() {
	Text txt("<i>", superHeight);
	Label* msg = new Label(1.f, txt.text, &Program::eventToggleChat, nullptr, makeTooltip("New message"), 1.f, Label::Alignment::left, true, 0, Widget::colorDark);
	return notification = new Overlay(pair(1.f - float(txt.length) / float(World::window()->getScreenView().x), 0), pair(txt.length, superHeight), { msg }, nullptr, nullptr, true, false, true, 0, vec4(0.f));
}

ProgState::Text ProgState::makeFpsText(float dSec) const {
	switch (World::program()->ftimeMode) {
	case Program::FrameTime::frames:
		return Text(toStr(uint(std::round(1.f / dSec))), lineHeight);
	case Program::FrameTime::seconds:
		return Text(toStr(uint(dSec * 1000.f)), lineHeight);
	}
	return Text(string(), lineHeight);
}

// PROG MENU

void ProgMenu::eventEscape() {
	World::window()->close();
}

void ProgMenu::eventEndTurn() {
	World::program()->eventConnectServer();
}

RootLayout* ProgMenu::createLayout(Interactable*& selected) {
	// server input
	Text srvt("Server:", superHeight);
	vector<Widget*> srv = {
		new Label(srvt.length, srvt.text),
		new LabelEdit(1.f, World::sets()->address, &Program::eventUpdateAddress, &Program::eventResetAddress, &Program::eventConnectServer)
	};

	// port input and connect button
	vector<Widget*> prt = {
		new Label(srvt.length, "Port:"),
		new LabelEdit(1.f, World::sets()->port, &Program::eventUpdatePort, &Program::eventResetPort, &Program::eventConnectServer, nullptr, Texture())
	};

	// net buttons
	vector<Widget*> con = {
		new Label(srvt.length, "Host", &Program::eventOpenHostMenu, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Label(1.f, "Connect", &Program::eventConnectServer, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	selected = con.back();

	// title
	int width = World::fonts()->length((srvt.text + "000.000.000.000").c_str(), superHeight) + lineSpacing + Label::textMargin * 4;
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
	return new RootLayout(1.f, std::move(cont));
}

// PROG LOBBY

ProgLobby::ProgLobby(vector<pair<string, bool>>&& roomList) :
	ProgState(),
	rooms(roomList)
{}

void ProgLobby::eventEscape() {
	World::program()->eventExitLobby();
}

void ProgLobby::eventEndTurn() {
	World::program()->eventHostRoomInput();
}

RootLayout* ProgLobby::createLayout(Interactable*& selected) {
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
	rooms.emplace(it, std::move(name), true);
}

void ProgLobby::openRoom(const string& name, bool open) {
	Label* le = static_cast<Label*>(rlist->getWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin())));
	le->lcall = open ? &Program::eventJoinRoomRequest : nullptr;
	le->setDim(open ? 1.f : defaultDim);
}

// PROG ROOM

ProgRoom::ProgRoom(umap<string, Com::Config>&& configs) :
	confs(std::move(configs))
{
	umap<string, Com::Config>::iterator it = confs.find(Com::Config::defaultName);
	World::program()->curConfig = it != confs.end() ? it->first : sortNames(confs).front();
	confs[World::program()->curConfig].checkValues();
}

void ProgRoom::eventEscape() {
	World::program()->info & Program::INF_UNIQ ? World::program()->eventOpenMainMenu() : World::program()->eventExitRoom();
}

void ProgRoom::eventEndTurn() {
	World::prun(startButton->lcall, startButton);
}

void ProgRoom::eventSecondaryAxis(const ivec2& val, float dSec) {
	if (chatBox && chatBox->getParent()->relSize.pix)
		chatBox->onScroll(vec2(val) * dSec * secondaryScrollThrottle);
}

RootLayout* ProgRoom::createLayout(Interactable*& selected) {
	Text back("Back", lineHeight);
	vector<Widget*> top0 = {
		new Label(back.length, back.text, World::program()->info & Program::INF_UNIQ ? &Program::eventOpenMainMenu : &Program::eventExitRoom),
		startButton = new Label(1.f, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center)
	};
	selected = top0.front();
	if (World::program()->info & Program::INF_UNIQ) {
		Text setup("Setup", lineHeight);
		Text port("Port:", lineHeight);
		top0.insert(top0.begin() + 1, new Label(setup.length, std::move(setup.text), &Program::eventOpenSetup));
#ifndef EMSCRIPTEN
		top0.insert(top0.end(), {
			new Label(port.length, std::move(port.text)),
			new LabelEdit(Text::strLen("00000", lineHeight) + LabelEdit::caretWidth, World::sets()->port, &Program::eventUpdatePort)
		});
#endif
	} else if (World::program()->info & Program::INF_HOST) {
		Text kick("Kick", lineHeight);
		top0.push_back(kickButton = new Label(kick.length, std::move(kick.text), nullptr, nullptr, Texture(), defaultDim));
	}

	vector<Widget*> topb = { new Layout(lineHeight, std::move(top0), false, lineSpacing) };
	vector<Widget*> menu = createConfigList(wio, World::program()->info & Program::INF_HOST ? confs[World::program()->curConfig] : World::game()->board.config, World::program()->info & Program::INF_HOST);
	if (World::program()->info & Program::INF_HOST) {
		Text cfgt("Configuration:", lineHeight);
		Text copy("Copy", lineHeight);
		Text newc("New", lineHeight);
		vector<Widget*> top1 = {
			new Label(cfgt.length, cfgt.text),
			new ComboBox(1.f, World::program()->curConfig, sortNames(confs), &Program::eventSwitchConfig),
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
			new Label(reset.length, reset.text, &Program::eventUpdateReset, nullptr, makeTooltip("Reset current configuration")),
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
	//wio.gameType->setCurOpt(uint8(cfg.gameType));
	wio.ports->on = cfg.ports;
	wio.rowBalancing->on = cfg.rowBalancing;
	wio.setPieceOn->on = cfg.setPieceOn;
	wio.setPieceNum->setText(toStr(cfg.setPieceNum));
	wio.width->setText(toStr(cfg.homeSize.x));
	wio.height->setText(toStr(cfg.homeSize.y));
	if (World::program()->info & Program::INF_HOST)
		wio.battleSL->setVal(cfg.battlePass);
	wio.battleLE->setText(toStr(cfg.battlePass) + '%');
	wio.favorTotal->on = cfg.favorTotal;
	wio.favorLimit->setText(toStr(cfg.favorLimit));
	wio.dragonLate->on = cfg.dragonLate;
	wio.dragonStraight->on = cfg.dragonStraight;
	setAmtSliders(cfg, cfg.tileAmounts.data(), wio.tiles.data(), wio.tileFortress, Com::tileLim, cfg.homeSize.y, &Com::Config::countFreeTiles, tileFortressString);
	setAmtSliders(cfg, cfg.middleAmounts.data(), wio.middles.data(), wio.middleFortress, Com::tileLim, 0, &Com::Config::countFreeMiddles, middleFortressString);
	setAmtSliders(cfg, cfg.pieceAmounts.data(), wio.pieces.data(), wio.pieceTotal, Com::pieceMax, 0, &Com::Config::countFreePieces, pieceTotalString);
	wio.winFortress->setText(toStr(cfg.winFortress));
	updateWinSlider(wio.winFortress, cfg.winFortress, cfg.countFreeTiles());
	wio.winThrone->setText(toStr(cfg.winThrone));
	updateWinSlider(wio.winThrone, cfg.winThrone, cfg.pieceAmounts[uint8(Com::Piece::throne)]);
	wio.capturers->setText(cfg.capturersString());
}

void ProgRoom::setAmtSliders(const Com::Config& cfg, const uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Com::Config::*counter)() const, string (*totstr)(const Com::Config&)) {
	for (uint8 i = 0; i < cnt; i++)
		wgts[i]->setText(toStr(amts[i]));
	total->setText(totstr(cfg));
	updateAmtSliders(amts, wgts, cnt, min, (cfg.*counter)());
}

void ProgRoom::updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest) {
	if (World::program()->info & Program::INF_HOST) {
		Layout* bline = wgts[0]->getParent();
		for (uint8 i = 0; i < cnt; i++)
			static_cast<Slider*>(static_cast<Layout*>(bline->getParent()->getWidget(bline->getIndex() + i))->getWidget(1))->set(amts[i], min, amts[i] + rest);
	}
}

void ProgRoom::updateWinSlider(Label* amt, uint16 val, uint16 max) {
	if (World::program()->info & Program::INF_HOST)
		static_cast<Slider*>(amt->getParent()->getWidget(amt->getIndex() - 1))->set(val, 0, max);
}

// PROG GAME

void ProgGame::eventSecondaryAxis(const ivec2& val, float dSec) {
	if (configList)
		configList->onScroll(vec2(val) * dSec * secondaryScrollThrottle);
	else if (chatBox && static_cast<Overlay*>(chatBox->getParent())->getShow())
		chatBox->onScroll(vec2(val) * dSec * secondaryScrollThrottle);
}

uint8 ProgGame::switchButtons(uint8 but) {
	if (bswapIcon->selected) {
		if (but == SDL_BUTTON_LEFT)
			return SDL_BUTTON_RIGHT;
		if (but == SDL_BUTTON_RIGHT)
			return SDL_BUTTON_LEFT;
	}
	return but;
}

vector<Overlay*> ProgGame::createOverlays() {
	if (!World::netcp())
		return ProgState::createOverlays();

	message = new Label(1.f, string(), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center, false);
	return vector<Overlay*>{
		new Overlay(pair(0.1f, dynamic_cast<ProgSetup*>(this) ? 0.08f : 0.92f), pair(0.8f, superHeight), { message }, nullptr, nullptr, true, true, false),
		new Overlay(pair(0.7f, 0.f), pair(0.3f, 1.f), createChat(), &Program::eventChatOpen, &Program::eventChatClose),
		createNotification(),
		createFpsCounter()
	};
}

Popup* ProgGame::createPopupFavorPick(uint16 availableFF) const {
	vector<Widget*> bot(favorMax + 2);
	bot.front() = new Widget();
	for (uint8 i = 0; i < favorMax; i++) {
		bool on = World::game()->favorsCount[i] < World::game()->favorsLeft[i];
		bot[1+i] = new Button(superHeight, on ? &Program::eventPickFavor : nullptr, nullptr, makeTooltip(firstUpper(favorNames[i]).c_str()), on ? 1.f : defaultDim, World::scene()->texture(favorNames[i]), vec4(1.f));
	}
	bot.back() = new Widget();

	Text ms(msgFavorPick + (availableFF > 1 ? " (" + toStr(availableFF) + ')' : string()), superHeight);
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(bot), false, lineSpacing)
	};
	return new Popup(pair(std::max(ms.length, int(favorMax) * superHeight + int(favorMax + 1) * lineSpacing), superHeight * 2 + lineSpacing), std::move(con), nullptr, nullptr, true, lineSpacing);
}

// PROG SETUP

ProgSetup::ProgSetup() :
	enemyReady(false),
	iselect(0),
	lastButton(0),
	lastHold(UINT16_MAX)
{}

void ProgSetup::eventEscape() {
	stage > Stage::tiles && stage < Stage::preparation ? World::program()->eventSetupBack() : World::program()->eventAbortGame();
}

void ProgSetup::eventEnter() {
	bswapIcon->selected ? handleClearing() : handlePlacing();
}

void ProgSetup::eventNumpress(uint8 num) {
	if (num < counters.size())
		setSelected(num);
}

void ProgSetup::eventWheel(int ymov) {
	setSelected(cycle(iselect, uint8(counters.size()), int8(ymov)));
}

void ProgSetup::eventDrag(uint32 mStat) {
	if (bswapIcon->selected)
		mStat = swapBits(mStat, SDL_BUTTON_LEFT - 1, SDL_BUTTON_RIGHT - 1);

	uint8 curButton = mStat & SDL_BUTTON_LMASK ? SDL_BUTTON_LEFT : mStat & SDL_BUTTON_RMASK ? SDL_BUTTON_RIGHT : 0;
	BoardObject* bo = dynamic_cast<BoardObject*>(World::scene()->getSelect());
	svec2 curHold = bo ? World::game()->board.ptog(bo->getPos()) : svec2(UINT16_MAX);
	if (bo && curButton && (curHold != lastHold || curButton != lastButton)) {
		if (stage <= Stage::middles)
			curButton == SDL_BUTTON_LEFT ? World::program()->eventPlaceTile() : World::program()->eventClearTile();
		else if (stage == Stage::pieces)
			curButton == SDL_BUTTON_LEFT ? World::program()->eventPlacePiece() : World::program()->eventClearPiece();
	}
	lastHold = curHold;
	lastButton = curButton;
}

void ProgSetup::eventUndrag() {
	lastHold = svec2(UINT16_MAX);
}

void ProgSetup::eventEndTurn() {
	if (stage < (World::netcp() ? Stage::preparation : Stage::pieces))
		World::program()->eventSetupNext();
}

void ProgSetup::eventEngage() {
	bswapIcon->selected ? handlePlacing() : handleClearing();
}

void ProgSetup::eventFavor(Favor favor) {
	eventWheel(int8(favor) % 2 * 2 - 1);
}

void ProgSetup::setStage(ProgSetup::Stage stg) {
	switch (stage = stg) {
	case Stage::tiles:
		World::game()->board.setOwnTilesInteract(Tile::Interact::interact);
		World::game()->board.setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->board.setOwnPiecesVisible(false);
		counters = World::game()->board.countOwnTiles();
		break;
	case Stage::middles:
		World::game()->board.setOwnTilesInteract(Tile::Interact::ignore, true);
		World::game()->board.setMidTilesInteract(Tile::Interact::interact);
		World::game()->board.setOwnPiecesVisible(false);
		World::game()->board.fillInFortress();
		counters = World::game()->board.countMidTiles();
		break;
	case Stage::pieces:
		World::game()->board.setOwnTilesInteract(Tile::Interact::recognize);
		World::game()->board.setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->board.setOwnPiecesVisible(true);
		counters = World::game()->board.countOwnPieces();
		break;
	default:
		World::game()->board.setOwnTilesInteract(Tile::Interact::ignore);
		World::game()->board.setMidTilesInteract(Tile::Interact::ignore);
		World::game()->board.disableOwnPiecesInteract(false);
		counters.clear();
	}
	string txt = chatBox ? chatBox->moveText() : string();
	if (World::scene()->resetLayouts(); chatBox)
		chatBox->setText(std::move(txt));

	if (iselect = 0; icons) {	// like setSelected but without crashing
		static_cast<Icon*>(icons->getWidget(iselect + 1))->selected = true;
		for (sizet i = 0; i < counters.size(); i++)
			switchIcon(uint8(i), counters[i]);
	}
}

void ProgSetup::setSelected(uint8 sel) {
	static_cast<Icon*>(icons->getWidget(iselect + 1))->selected = false;
	iselect = sel;
	static_cast<Icon*>(icons->getWidget(iselect + 1))->selected = true;
}

uint8 ProgSetup::findNextSelect(bool fwd) {
	uint8 m = btom<uint8>(fwd);
	for (uint8 i = iselect + m; i < uint8(counters.size()); i += m)
		if (counters[i])
			return i;
	for (uint8 i = fwd ? 0 : uint8(counters.size() - 1); i != iselect; i += m)
		if (counters[i])
			return i;
	return iselect;
}

void ProgSetup::handlePlacing() {
	if (stage <= Stage::middles) {
		if (Tile* til = dynamic_cast<Tile*>(World::scene()->getSelect()); til && til->getType() != Com::Tile::empty)
			til->startKeyDrag(SDL_BUTTON_LEFT);
		else
			World::program()->eventPlaceTile();
	} else if (stage == Stage::pieces) {
		if (Piece* pce = dynamic_cast<Piece*>(World::scene()->getSelect()))
			pce->startKeyDrag(SDL_BUTTON_LEFT);
		else
			World::program()->eventPlacePiece();
	}
}

void ProgSetup::handleClearing() {
	if (stage <= Stage::middles)
		World::program()->eventClearTile();
	else if (stage == Stage::pieces)
		World::program()->eventClearPiece();
}

RootLayout* ProgSetup::createLayout(Interactable*& selected) {
	// sidebar
	initlist<const char*> sidt = {
		"Exit",
		"Config",
		"Save",
		"Load",
		"Delete",
		"Back",
		stage < Stage::pieces || !World::netcp() ? "Next" : "Finish",
		"Chat"
	};
	initlist<const char*>::iterator isidt = sidt.begin();
	int sideLength = Text::maxLen(sidt.begin(), sidt.end(), lineHeight);
	bool cnext = (stage < Stage::pieces || World::netcp()) && stage < Stage::preparation, cback = stage > Stage::tiles && stage < Stage::preparation;

	vector<Widget*> wgts = {
		new Label(lineHeight, *isidt++, &Program::eventAbortGame, nullptr, makeTooltip("Exit the game")),
		new Label(lineHeight, *isidt++, &Program::eventShowConfig, nullptr, makeTooltip("Show current game configuration")),
		new Label(lineHeight, *isidt++, &Program::eventOpenSetupSave, nullptr, makeTooltip("Save current setup")),
		new Label(lineHeight, *isidt++, stage < Stage::preparation ? &Program::eventOpenSetupLoad : nullptr, nullptr, makeTooltip("Load a setup"), stage < Stage::preparation ? 1.f : defaultDim),
		bswapIcon = new Icon(lineHeight, *isidt++, stage < Stage::preparation ? &Program::eventSwitchGameButtons : nullptr, nullptr, nullptr, makeTooltip("Switch between placing and deleting a tile/piece"), stage < Stage::preparation ? 1.f : defaultDim),
		new Label(lineHeight, *isidt++, cback ? &Program::eventSetupBack : nullptr, nullptr, makeTooltip(stage == Stage::middles ? "Go to placing homeland tiles" : stage == Stage::pieces ? "Go to placing middle row tiles" : ""), cback ? 1.f : defaultDim),
		new Label(lineHeight, *isidt++, cnext ? &Program::eventSetupNext : nullptr, nullptr, makeTooltip(stage == Stage::tiles ? "Go to placing middle row tiles" : stage == Stage::middles ? "Go to placing pieces" : stage == Stage::pieces && World::netcp() ? "Confirm and finish setup" : ""), cnext ? 1.f : defaultDim)
	};
	selected = wgts.back();
	if (World::netcp())
		wgts.insert(wgts.begin() + 4, new Label(lineHeight, *isidt++, &Program::eventToggleChat, nullptr, makeTooltip("Toggle chat")));

	vector<Widget*> side = {
		new Layout(sideLength, std::move(wgts), true, lineSpacing),
		planeSwitch = new Navigator()
	};

	// root layout
	if (stage <= Stage::pieces) {
		vector<Widget*> cont = {
			new Layout(1.f, std::move(side), false),
			icons = stage == Stage::pieces ? makePicons() : makeTicons(),
			new Widget(superSpacing)
		};
		return new RootLayout(1.f, std::move(cont));
	}
	icons = nullptr;
	return new RootLayout(1.f, std::move(side), false);
}

Layout* ProgSetup::makeTicons() {
	vector<Widget*> tbot(Com::tileLim + 2);
	tbot.front() = new Widget();
	for (uint8 i = 0; i < Com::tileLim; i++)
		tbot[1+i] = new Icon(iconSize, string(), &Program::eventIconSelect, nullptr, nullptr, makeTooltip(firstUpper(Com::tileNames[i]).c_str()), 1.f, Label::Alignment::center, true, World::scene()->texture(Com::tileNames[i]), vec4(1.f));
	tbot.back() = new Widget();
	return new Layout(iconSize, std::move(tbot), false, lineSpacing);
}

Layout* ProgSetup::makePicons() {
	vector<Widget*> pbot(Com::pieceNames.size() + 2);
	pbot.front() = new Widget();
	for (sizet i = 0; i < Com::pieceNames.size(); i++)
		pbot[1+i] = new Icon(iconSize, string(), &Program::eventIconSelect, nullptr, nullptr, makeTooltip(firstUpper(Com::pieceNames[i]).c_str()), 1.f, Label::Alignment::center, true, World::scene()->texture(Com::pieceNames[i]), vec4(1.f));
	pbot.back() = new Widget();
	return new Layout(iconSize, std::move(pbot), false, lineSpacing);
}

void ProgSetup::incdecIcon(uint8 type, bool inc) {
	if (!inc && counters[type] == 1) {
		switchIcon(type, false);
		setSelected(findNextSelect(true));
	} else if (inc && !counters[type])
		switchIcon(type, true);
	counters[type] += btom<uint16>(inc);
}

void ProgSetup::switchIcon(uint8 type, bool on) {
	Icon* ico = getIcon(type);
	ico->setDim(on ? 1.f : defaultDim);
	ico->lcall = on ? &Program::eventIconSelect : nullptr;
}

Popup* ProgSetup::createPopupSaveLoad(bool save) {
#ifdef EMSCRIPTEN
	if (!FileSys::canRead())
		return createPopupMessage("Waiting for files to sync", &Program::eventClosePopup);
#endif
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

Popup* ProgSetup::createPopupPiecePicker() {
	vector<Widget*> bot[2] = { vector<Widget*>(5), vector<Widget*>(5) };
	for (uint8 r = 0, t = 0; r < 2; r++)
		for (uint8 i = 0; i < 5; i++, t++) {
			bool more = World::game()->board.ownPieceAmts[t] < World::game()->board.config.pieceAmounts[t];
			bot[r][i] = new Button(superHeight, more ? &Program::eventSetupPickPiece : nullptr, nullptr, makeTooltip(firstUpper(Com::pieceNames[t]).c_str()), more ? 1.f : defaultDim, World::scene()->texture(Com::pieceNames[t]), vec4(1.f));
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
	return new Popup(pair(std::max(ms.length, plen), superHeight * 3 + lineSpacing * 2), std::move(con), nullptr, nullptr, true, lineSpacing);
}

// PROG MATCH

void ProgMatch::eventEscape() {
	World::program()->eventAbortGame();
}

void ProgMatch::eventEnter() {
	if (Piece* pce = dynamic_cast<Piece*>(World::scene()->getSelect()))
		pce->startKeyDrag(SDL_BUTTON_LEFT);
}

void ProgMatch::eventNumpress(uint8 num) {
	if (num < favorIcons.size())
		eventFavor(Favor(num));
}

void ProgMatch::eventWheel(int ymov) {
	World::scene()->camera.zoom(ymov);
}

void ProgMatch::eventEndTurn() {
	if (turnIcon->lcall)
		World::program()->eventEndTurn();
}

void ProgMatch::eventSurrender() {
	World::program()->eventSurrender();
}

void ProgMatch::eventEngage() {
	if (Piece* pce = dynamic_cast<Piece*>(World::scene()->getSelect()))
		pce->startKeyDrag(SDL_BUTTON_RIGHT);
}

void ProgMatch::eventDestroy(Switch sw) {
	switch (sw) {
	case Switch::off:
		destroyIcon->selected = false;
		World::game()->board.setPxpadPos(nullptr);
		break;
	case Switch::on:
		destroyIcon->selected = true;
		World::game()->board.setPxpadPos(dynamic_cast<Piece*>(World::scene()->capture));
		break;
	case Switch::toggle:
		destroyIcon->selected = !destroyIcon->selected;
		World::game()->board.setPxpadPos(destroyIcon->selected ? dynamic_cast<Piece*>(World::scene()->capture) : nullptr);
	}
}

void ProgMatch::eventFavor(Favor favor) {
	Favor old = selectFavorIcon(favor);
	World::game()->finishFavor(favor, old);
	World::game()->board.setFavorInteracts(favor, World::game()->getOwnRec());
}

void ProgMatch::eventCameraReset() {
	World::scene()->camera.setPos(Camera::posMatch, Camera::latMatch);
}

Favor ProgMatch::selectFavorIcon(Favor& type) {
	Favor old = Favor(std::find_if(favorIcons.begin(), favorIcons.end(), [](Icon* it) -> bool { return it && it->selected; }) - favorIcons.begin());
	if (old != Favor::none && old != type)
		favorIcons[uint8(old)]->selected = false;
	if (type != Favor::none) {
		if (favorIcons[uint8(type)])
			favorIcons[uint8(type)]->selected = favorIcons[uint8(type)]->lcall && !favorIcons[uint8(type)]->selected;
		if (!(favorIcons[uint8(type)] && favorIcons[uint8(type)]->selected))
			type = Favor::none;
	}
	return old;
}

void ProgMatch::updateFavorIcon(Favor type, bool on) {
	if (Icon* ico = favorIcons[uint8(type)]) {
		if (World::game()->favorsCount[uint8(type)] || World::game()->favorsLeft[uint8(type)]) {
			on = on && World::game()->favorsCount[uint8(type)];
			ico->lcall = on ? &Program::eventSelectFavor : nullptr;
			ico->setDim(on ? 1.f : defaultDim);
		} else {
			if (Layout* box = ico->getParent(); box->getWidgets().size() > 1)
				box->deleteWidget(ico->getIndex());
			else
				box->getParent()->deleteWidget(box->getIndex());
			favorIcons[uint8(type)] = nullptr;
		}
	}
}

void ProgMatch::updateEstablishIcon(bool on) {
	if (establishIcon) {
		establishIcon->lcall = on ? &Program::eventEstablishB : nullptr;
		establishIcon->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::destroyEstablishIcon() {
	establishIcon->getParent()->deleteWidget(establishIcon->getIndex());
	establishIcon = nullptr;
}

void ProgMatch::updateRebuildIcon(bool on) {
	if (rebuildIcon) {
		rebuildIcon->lcall = on ? &Program::eventRebuildTileB : nullptr;
		rebuildIcon->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::updateSpawnIcon(bool on) {
	if (spawnIcon) {
		spawnIcon->lcall = on ? &Program::eventSpawnPieceB : nullptr;
		spawnIcon->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::updateTurnIcon(bool on) {
	turnIcon->lcall = on ? &Program::eventEndTurn : nullptr;
	turnIcon->setDim(on ? 1.f : defaultDim);
}

void ProgMatch::setDragonIcon(bool on) {
	if (dragonIcon) {
		Icon* ico = static_cast<Icon*>(dragonIcon->getWidget(0));
		ico->selected = false;
		ico->lcall = on ? &Program::eventClickPlaceDragon : nullptr;
		ico->hcall = on ? &Program::eventHoldPlaceDragon : nullptr;
		ico->setDim(on ? 1.f : defaultDim);
	}
}

void ProgMatch::decreaseDragonIcon() {
	if (!--unplacedDragons) {
		dragonIcon->getParent()->deleteWidget(dragonIcon->getIndex());
		dragonIcon = nullptr;
	}
}

RootLayout* ProgMatch::createLayout(Interactable*& selected) {
	// sidebar
	initlist<const char*> sidt = {
		"Exit",
		"Surrender",
		"Config",
		"Chat",
		"Engage",
		"Destroy",
		"Finish",
		"Spawn",
		"Establish",
		"Rebuild"
	};
	initlist<const char*>::iterator isidt = sidt.begin();
	int sideLength = std::max(Text::maxLen(sidt.begin(), sidt.end(), lineHeight), iconSize * 2 + lineSpacing * 2);	// second argument is length of FF line

	vector<Widget*> left = {
		new Label(lineHeight, *isidt++, &Program::eventAbortGame, nullptr, makeTooltip("Exit the game")),
		new Label(lineHeight, *isidt++, &Program::eventSurrender, nullptr, makeTooltip("Surrender and quit the match")),
		new Label(lineHeight, *isidt++, &Program::eventShowConfig, nullptr, makeTooltip("Show current game configuration")),
		new Label(lineHeight, *isidt++, &Program::eventToggleChat, nullptr, makeTooltip("Toggle chat")),
		bswapIcon = new Icon(lineHeight, *isidt++, &Program::eventSwitchGameButtons, nullptr, nullptr, makeTooltip("Switch between engaging and moving a piece")),
		destroyIcon = new Icon(lineHeight, *isidt++, &Program::eventSwitchDestroy, nullptr, nullptr, makeTooltip("Destroy a tile when moving off of it")),
		turnIcon = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Finish current turn"))
	};
	selected = turnIcon;
	if (World::game()->board.config.gameType == Com::Config::GameType::homefront) {
		left.insert(left.end() - 1, {
			establishIcon = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Respawn a piece")),
			rebuildIcon = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Rebuild a fortress or farm")),
			spawnIcon = new Label(lineHeight, *isidt++, nullptr, nullptr, makeTooltip("Establish a farm or city"))
		});
	} else
		rebuildIcon = establishIcon = spawnIcon = nullptr;
	if (World::game()->board.config.favorLimit && World::game()->board.ownPieceAmts[uint8(Com::Piece::throne)]) {
		for (sizet i = 0; i < favorIcons.size(); i++)
			favorIcons[i] = new Icon(iconSize, string(), nullptr, nullptr, nullptr, makeTooltip(firstUpper(favorNames[i]).c_str()), 1.f, Label::Alignment::center, true, World::scene()->texture(favorNames[i]), vec4(1.f));
		left.push_back(new Widget(0));
		for (sizet i = 0; i < favorIcons.size(); i += 2)
			left.push_back(new Layout(iconSize, { new Widget(0), favorIcons[i], favorIcons[i+1] }, false, lineSpacing));
	} else
		std::fill(favorIcons.begin(), favorIcons.end(), nullptr);

	unplacedDragons = 0;
	for (Piece* pce = World::game()->board.getOwnPieces(Com::Piece::dragon); pce->getType() == Com::Piece::dragon; pce++)
		if (!pce->show)
			unplacedDragons++;
	if (unplacedDragons) {
		left.push_back(dragonIcon = new Layout(iconSize, { new Icon(iconSize, string(), nullptr, nullptr, nullptr, makeTooltip((string("Place ") + Com::pieceNames[uint8(Com::Piece::dragon)]).c_str()), 1.f, Label::Alignment::left, true, World::scene()->texture(Com::pieceNames[uint8(Com::Piece::dragon)]), vec4(1.f)) }, false, 0));
		setDragonIcon(World::game()->getMyTurn());
	} else
		dragonIcon = nullptr;

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, std::move(left), true, lineSpacing),
		planeSwitch = new Navigator()
	};
	RootLayout* root = new RootLayout(1.f, std::move(cont), false, 0);
	updateEstablishIcon(World::game()->getMyTurn());
	updateRebuildIcon(World::game()->getMyTurn());
	updateSpawnIcon(World::game()->getMyTurn());
	updateTurnIcon(World::game()->getMyTurn());
	for (uint8 i = 0; i < favorMax; i++)
		updateFavorIcon(Favor(i), World::game()->getMyTurn());
	return root;
}

Popup* ProgMatch::createPopupSpawner() const {
	vector<Widget*> bot[2] = { vector<Widget*>(4), vector<Widget*>(4) };
	for (uint8 r = 0, t = 0; r < 2; r++)
		for (uint8 i = 0; i < 4; i++, t++) {
			bool on = World::game()->board.pieceSpawnable(Com::Piece(t));
			bot[r][i] = new Button(superHeight, on ? &Program::eventSpawnPieceB : nullptr, nullptr, makeTooltip(firstUpper(Com::pieceNames[i]).c_str()), on ? 1.f : defaultDim, World::scene()->texture(Com::pieceNames[t]), vec4(1.f));
		}

	vector<Widget*> mid = {
		new Layout(superHeight, std::move(bot[0]), false, lineSpacing),
		new Layout(superHeight, std::move(bot[1]), false, lineSpacing)
	};
	vector<Widget*> cont = {
		new Widget(),
		new Layout(1.f, std::move(mid)),
		new Widget()
	};

	Text ms("Spawn piece", superHeight);
	vector<Widget*> con = {
		new Label(1.f, std::move(ms.text), nullptr, nullptr, Texture(), 1.f, Label::Alignment::center),
		new Layout(1.f, std::move(cont), false, 0),
		new Label(1.f, "Cancel", &Program::eventClosePopup)
	};
	return new Popup(pair(std::max(ms.length, 4 * superHeight + 5 * lineSpacing), superHeight * 3 + lineSpacing * 2), std::move(con), nullptr, nullptr, true, lineSpacing);
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	World::program()->eventOpenMainMenu();
}

void ProgSettings::eventEndTurn() {
	World::program()->eventOpenInfo();
}

RootLayout* ProgSettings::createLayout(Interactable*& selected) {
	// side bar
	initlist<const char*> tps = {
		"Back",
		"Info",
		"Reset"
	};
	initlist<const char*>::iterator itps = tps.begin();
	int optLength = Text::maxLen(tps.begin(), tps.end(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, *itps++, &Program::eventOpenMainMenu),
		new Label(lineHeight, *itps++, &Program::eventOpenInfo),
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
		"Show tooltips",
		"Chat line limit",
		"Deadzone",
		"Resolve family",
		"Regular font"
	};
	initlist<const char*>::iterator itxs = txs.begin();
	int descLength = Text::maxLen(txs.begin(), txs.end(), lineHeight);
	constexpr char shadowTip[] = "Width/height of shadow cubemap";
	constexpr char scaleTip[] = "Scale factor of texture sizes";
	constexpr char vsyncTip[] = "Immediate: off\n"
		"Synchronized: on\n"
		"Adaptive: on and smooth (works on fewer computers)";
	constexpr char gammaTip[] = "Brightness";
	constexpr char volumeTip[] = "Audio volume";
	constexpr char chatTip[] = "Line break limit of a chat box";
	constexpr char deadzTip[] = "Controller axis deadzone";
	sizet lnc = txs.size();
	int slleLength = Text::strLen("00000", lineHeight) + LabelEdit::caretWidth;

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
		new CheckBox(lineHeight, World::sets()->tooltips, &Program::eventSetTooltips, &Program::eventSetTooltips, makeTooltip("Display tooltips on hover"))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->chatLines, 0, Settings::chatLinesMax, 256, &Program::eventSetChatLineLimitSL, &Program::eventSLUpdateLE, makeTooltip(chatTip)),
		new LabelEdit(slleLength, toStr(World::sets()->chatLines), &Program::eventSetChatLineLimitLE, nullptr, nullptr, nullptr, makeTooltip(chatTip))
	}, {
		new Label(descLength, *itxs++),
		new Slider(1.f, World::sets()->deadzone, 0, SHRT_MAX + 1, 2048, &Program::eventSetDeadzoneSL, &Program::eventSLUpdateLE, makeTooltip(deadzTip)),
		new LabelEdit(slleLength, toStr(World::sets()->deadzone), &Program::eventSetDeadzoneLE, nullptr, nullptr, nullptr, makeTooltip(deadzTip))
	}, {
		new Label(descLength, *itxs++),
		new ComboBox(1.f, Com::familyNames[uint8(World::sets()->resolveFamily)], vector<string>(Com::familyNames.begin(), Com::familyNames.end()), &Program::eventSetResolveFamily, nullptr, nullptr, makeTooltip("What family to use for resolving a host address"))
	}, {
		new Label(descLength, *itxs++),
		new CheckBox(lineHeight, World::sets()->fontRegular, &Program::eventSetFontRegular, &Program::eventSetFontRegular, makeTooltip("Use the standard Romanesque or Merriweather to support more characters"))
	}, };
	vector<Widget*> lns(lnc);
	for (sizet i = 0; i < lnc; i++)
		lns[i] = new Layout(lineHeight, std::move(lx[i]), false, lineSpacing);

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		new ScrollArea(1.f, std::move(lns), true, lineSpacing)
	};
	return new RootLayout(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

SDL_DisplayMode ProgSettings::fstrToDisp(const string& str) const {
	SDL_DisplayMode mode = strToDisp(str);
	if (string::const_reverse_iterator sit = std::find_if(str.rbegin(), str.rend(), isSpace); sit != str.rend())
		if (umap<string, uint32>::const_iterator pit = pixelformats.find(string(sit.base(), str.end())); pit != pixelformats.end())
			mode.format = pit->second;
	return mode;
}

// PROG INFO

void ProgInfo::eventEscape() {
	World::program()->eventOpenSettings();
}

void ProgInfo::eventEndTurn() {
	World::program()->eventOpenSettings();
}

void ProgInfo::eventSecondaryAxis(const ivec2& val, float dSec) {
	content->onScroll(vec2(val) * dSec * secondaryScrollThrottle);
}

RootLayout* ProgInfo::createLayout(Interactable*& selected) {
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
		"Render Drivers"
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
	int argWidth = Text::maxLen({ args, dispArgs, rendArgs }, lineHeight);

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

	// root layout
	vector<Widget*> cont = {
		new Layout(optLength, std::move(lft), true, lineSpacing),
		content = new ScrollArea(1.f, std::move(lines), true, lineSpacing)
	};
	return new RootLayout(1.f, std::move(cont), false, superSpacing, RootLayout::uniformBgColor);
}

void ProgInfo::appendProgram(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) {
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
	Text slr(toStr(SDL_GetRevisionNumber()), lineHeight);
	SDL_VERSION(&bver)
	Text scv(versionText(bver), lineHeight);
	const SDL_version* ilver = IMG_Linked_Version();
	SDL_IMAGE_VERSION(&icver)
	const SDL_version* tlver = TTF_Linked_Version();
	SDL_TTF_VERSION(&tcver)
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
		new Layout(lineHeight, { new Label(width, *args++), new Label(slv.length, std::move(slv.text)), new Label(slr.length, std::move(slr.text)), new Label(1.f, SDL_GetRevision()), new Label(scv.length, std::move(scv.text)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, versionText(*ilver)), new Label(1.f, versionText(icver)) }, false, lineSpacing),
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, versionText(*tlver)), new Label(1.f, versionText(tcver)) }, false, lineSpacing),
#ifdef WEBUTILS
		new Layout(lineHeight, { new Label(width, *args++), new Label(1.f, curl_version_info(CURLVERSION_NOW)->version), new Label(1.f, LIBCURL_VERSION) }, false, lineSpacing),
#endif
		new Widget(lineHeight)
	});
}

void ProgInfo::appendSystem(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) {
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
		for (sizet i = 1; i < features.size(); i++)
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

void ProgInfo::appendCurrentDisplay(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) {
	lines.push_back(new Label(superHeight, *titles++));
	if (int i = World::window()->displayID(); i >= 0)
		appendDisplay(lines, i, width, args);
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplay(vector<Widget*>& lines, int i, int width, initlist<const char*>::iterator args) {
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

void ProgInfo::appendPower(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) {
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

void ProgInfo::appendAudioDevices(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int iscapture) {
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < SDL_GetNumAudioDevices(iscapture); i++)
		if (const char* name = SDL_GetAudioDeviceName(i, iscapture))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDrivers(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) {
	lines.push_back(new Label(superHeight, *titles++));
	for (int i = 0; i < limit(); i++)
		if (const char* name = value(i))
			lines.push_back(new Layout(lineHeight, { new Label(width, toStr(i)), new Label(1.f, name) }, false, lineSpacing));
	lines.push_back(new Widget(lineHeight));
}

void ProgInfo::appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) {
	lines.push_back(new Label(superHeight, *titles++));
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

void ProgInfo::appendRenderers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) {
	lines.push_back(new Label(superHeight, *titles++));
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
				for (uint32 j = 1; j < info.num_texture_formats; j++)
					lines.push_back(new Layout(lineHeight, { new Widget(width), new Label(1.f, pixelformatName(info.texture_formats[j])) }, false, lineSpacing));
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
