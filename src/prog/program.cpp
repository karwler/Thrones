#include "engine/world.h"

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	setState(new ProgMenu);
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

// GAME SETUP

void Program::eventOpenSetup() {
	World::scene()->setObjects(game.initObjects());
	setState(new ProgSetup);
}

void Program::eventPlaceTile(Button* but) {
	Tile* til = dynamic_cast<Tile*>(World::scene()->rayCast(World::scene()->cursorDirection(mousePos())));
	if (!til)
		return;

	// remove any piece that may already be there
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (til->getType() != Tile::Type::empty) {
		Draglet* ico = static_cast<Draglet*>(ps->ticons->getWidget(uint8(til->getType()) + 1));
		ico->cusColor = til->color;
		ico->setLcall(&Program::eventPlaceTile);
		ps->tileCnt[uint8(til->getType())]++;
	}

	// place the tile
	Draglet* dlt = static_cast<Draglet*>(but);
	til->setType(valToEnum<Tile::Type>(Tile::colors, dlt->cusColor));
	if (--ps->tileCnt[uint8(til->getType())] == 0) {
		dlt->cusColor = dimColor(dlt->cusColor, 0.5f, 0.5f);
		dlt->setLcall(nullptr);
	}
}

void Program::eventPlacePiece(Button* but) {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->rayCast(World::scene()->cursorDirection(mousePos())));
	if (!bob)
		return;

	// remove any piece that may be occupying that tile already
	vec2b pos = bob->getPos();
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (Piece* pce = dynamic_cast<Piece*>(bob); pce || (pce = game.findPiece(pos))) {
		Draglet* ico = static_cast<Draglet*>(ps->picons->getWidget(uint8(pce->getType()) + 1));
		ico->cusColor = BoardObject::defaultColor;
		ico->setLcall(&Program::eventPlacePiece);
		ps->pieceCnt[uint8(pce->getType())]++;
	}

	// find the first not placed piece of the specified type
	Draglet* dlt = static_cast<Draglet*>(but);
	Piece::Type ptyp = strToEnum<Piece::Type>(Piece::names, dlt->bgTex->getName());
	Piece* pieces = game.getPieces(ptyp);
	sizet id = 0;
	while (id < Piece::amounts[uint8(ptyp)] && pieces[id].getPos().hasNot(-1))
		id++;

	// place it if exists
	if (id < Piece::amounts[uint8(ptyp)]) {
		pieces[id].setPos(pos);
		if (--ps->pieceCnt[uint8(pieces[id].getType())] == 0) {
			dlt->cusColor = dimColor(dlt->cusColor, 0.5f, 0.5f);
			dlt->setLcall(nullptr);
		}
	}
}

void Program::eventClearTile(BoardObject* obj) {
	Tile* til = static_cast<Tile*>(obj);

	static_cast<ProgSetup*>(state.get())->tileCnt[uint8(til->getType())]++;
	til->setType(Tile::Type::empty);
}

void Program::eventClearPiece(BoardObject* obj) {
	Piece* pce = static_cast<Piece*>(obj);

	static_cast<ProgSetup*>(state.get())->pieceCnt[uint8(pce->getType())]++;
	pce->setType(Piece::Type::empty);
}

void Program::eventSetupNext(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	switch (ps->stage) {
	case ProgSetup::Stage::tiles:
		if (string err = game.checkOwnTiles(); err.empty()) {
			World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
			return;
		}
		game.fillInFortress();
		game.setOwnTilesInteract(false);
		game.setMidTilesInteract(true);
		break;
	case ProgSetup::Stage::middles:
		if (string err = game.checkMidTiles(); err.empty()) {
			World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
			return;
		}
		game.setOwnTilesInteract(true);
		game.setMidTilesInteract(false);
		game.setOwnPiecesInteract(true);
		break;
	case ProgSetup::Stage::pieces:
		if (string err = game.checkOwnPieces(); err.empty()) {
			World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
			return;
		}
		game.setOwnTilesInteract(false);
		game.setOwnPiecesInteract(false);
	}

	if (ps->stage = ProgSetup::Stage(uint8(ps->stage) + 1); ps->stage < ProgSetup::Stage::ready)
		World::scene()->resetLayouts();
	else {
		game.sendSetup();
		ps->enemyReady ? eventOpenMatch() : eventShowWaitPopup();
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	switch (ps->stage) {
	case ProgSetup::Stage::middles:
		game.setOwnTilesInteract(true);
		game.setMidTilesInteract(false);
		game.takeOutFortress();
		break;
	case ProgSetup::Stage::pieces:
		game.setOwnTilesInteract(false);
		game.setMidTilesInteract(true);
		game.setOwnPiecesInteract(false);
		break;
	}
	ps->stage = ProgSetup::Stage(uint8(ps->stage) - 1);
	World::scene()->resetLayouts();
}

void Program::eventShowWaitPopup(Button*) {
	World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventExitGame, "Exit"));
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.prepareMatch();
	setState(new ProgMatch);
	World::scene()->addAnimation(Animation(game.getScreen(), {Keyframe(0.5f, Keyframe::CHG_POS, game.getScreen()->pos += vec3(0.f, -2.f, 0.f))}));
}

void Program::eventPlaceDragon(Button*) {
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->rayCast(World::scene()->cursorDirection(mousePos())));
	if (!bob)
		return;

	vec2b pos = bob->getPos();	// check if pos is in homeland
	if (outRange(pos, vec2b(0, 1), vec2b(Game::boardSize.x, Game::homeHeight)))
		return;

	if (Piece* pce = dynamic_cast<Piece*>(bob); pce || (pce = game.findPiece(pos)))	// kill any piece that might be occupying the tile
		game.killPiece(pce);
	game.getPieces(Piece::Type::dragon)->setPos(pos);	// place the dragon
	// TODO: send piece update

	ProgMatch* pm = static_cast<ProgMatch*>(state.get());	// get rid of button
	pm->dragonIcon->getParent()->deleteWidget(pm->dragonIcon->getID());
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

void Program::eventOpenInfo(Button*) {
	setState(new ProgInfo);
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
