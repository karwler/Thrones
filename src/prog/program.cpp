#include "engine/world.h"

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	setState(new ProgMenu);
	World::scene()->setObjects(game.initObjects());
}

void Program::eventConnectServer(Button*) {
	game.connect();
}

void Program::eventConnectCancel(Button*) {
	game.disconnect();
	World::scene()->setPopup(nullptr);
}

void Program::eventUpdateAddress(Button* but) {
	World::sets()->address = static_cast<LabelEdit*>(but)->getText();
}

void Program::eventUpdatePort(Button* but) {
	World::sets()->port = uint16(sstoul(static_cast<LabelEdit*>(but)->getText()));
}

// GAME

void Program::eventOpenGame() {
	setState(new ProgGame);
}

void Program::eventPlaceTile(Button*) {
	// TODO: place the tile
}

void Program::eventExitGame(Button*) {
	game.disconnect();
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
