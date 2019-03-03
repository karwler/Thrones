#include "engine/world.h"

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	setState(new ProgMenu);
}

void Program::eventConnectServer(Button*) {
	try {
		ProgMenu* sm = static_cast<ProgMenu*>(state.get());
		game.reset(new Game(sm->server->getText(), uint16(sstoul(sm->port->getText()))));
		World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventConnectCancelled, "Cancel"));
	} catch (const std::runtime_error& e) {
		game.reset();
		World::scene()->setPopup(ProgState::createPopupMessage(e.what(), &Program::eventClosePopup));
	}
}

void Program::eventConnectConnected() {
	setState(new ProgGame);
}

void Program::eventConnectFailed(const string& msg) {
	game.reset();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}

void Program::eventConnectCancelled(Button*) {
	game.reset();
	World::scene()->setPopup(nullptr);
}

// GAME

void Program::eventExitGame() {
	game.reset();
	eventOpenMainMenu();
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState(new ProgSettings);
}

void Program::eventSetFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
}

void Program::eventSetResolution(Button* but) {
	World::winSys()->setResolution(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetVsync(Button* but) {
	World::winSys()->setVsync(Settings::VSync(static_cast<SwitchBox*>(but)->getCurOpt()));
}

void Program::eventResetSettings(Button*) {
	World::winSys()->resetSettings();
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button*) {
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state.reset(newState);
	World::scene()->resetLayouts();
}
