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

Popup* ProgState::createPopupMessage(const string& msg, PCall ccal, const string& ctxt) {
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

Popup* ProgState::createPopupChoice(const string& msg, PCall kcal, PCall ccal) {
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

// PROG GAME

void ProgGame::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventExitGame();
}

Layout* ProgGame::createLayout() {
	// sidebar
	vector<string> sidt {
		"Exit"
	};
	int sideLength = findMaxLength(sidt.data(), sidt.size(), lineHeight);
	vector<Widget*> left = {
		new Label(lineHeight, popBack(sidt), &Program::eventExitGame)
	};

	// tile select
	vector<Widget*> botm = {
		new Widget(),
		new Draglet(iconSize, nullptr),
		new Draglet(iconSize, nullptr),
		new Widget(0),
		new Draglet(iconSize, nullptr),
		new Draglet(iconSize, nullptr),
		new Widget()
	};

	// center piece
	vector<Widget*> midl = {
		new Widget(),
		new Layout(iconSize, botm)
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(sideLength, left),
		new Layout(1.f, midl, true, false, 0),
		new Widget(sideLength)
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
		"Exit",
		"Back",
		"Reset"
	};
	int optLength = findMaxLength(tps.data(), tps.size(), superHeight);
	vector<Widget*> lft = {
		new Label(lineHeight, popBack(tps), &Program::eventResetSettings),
		new Widget(Layout::defaultItemSpacing),
		new Label(lineHeight, popBack(tps), &Program::eventOpenMainMenu),
		new Label(lineHeight, popBack(tps), &Program::eventExit)
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
