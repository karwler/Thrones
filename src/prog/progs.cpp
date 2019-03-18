#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(const string& str, int height, int margin) :
	text(str),
	length(World::winSys()->textLength(str, height) + margin * 2),
	height(height)
{}

int ProgState::findMaxLength(const string* strs, sizet scnt, int height, int margin) {
	int width = 0;
	for (sizet i = 0; i < scnt; i++)
		if (int len = World::winSys()->textLength(strs[i], height) + margin * 2; len > width)
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
	Text ok(ctxt, popupLineHeight);
	Text ms(msg, popupLineHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, ok.text, ccal, nullptr, nullptr, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, false, 0)
	};
	return new Popup(vec2s(ms.length, popupLineHeight * 2 + Layout::defaultItemSpacing), con);
}

Popup* ProgState::createPopupChoice(const string& msg, BCall kcal, BCall ccal) {
	Text ms(msg, popupLineHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, nullptr, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, nullptr, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, ms.text, nullptr, nullptr, nullptr, Label::Alignment::center),
		new Layout(1.f, bot, false, false, 0)
	};
	return new Popup(vec2s(ms.length, popupLineHeight * 2 + Layout::defaultItemSpacing), con);
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
	sideLength = findMaxLength(sidt.data(), sidt.size(), lineHeight);
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
		"Back"
	};
	int optLength = findMaxLength(tps.data(), tps.size(), lineHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(tps), &Program::eventOpenMainMenu),
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
	int descLength = findMaxLength(txs.data(), txs.size(), lineHeight);
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
