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
	Tile* til = World::scene()->pickObject<Tile>();
	if (!til)
		return;

	// remove any piece that may already be there
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (til->getType() != Tile::Type::empty) {
		Draglet* ico = static_cast<Draglet*>(ps->ticons->getWidget(uint8(til->getType()) + 1));
		ico->setColor(til->color);
		ico->setLcall(&Program::eventPlaceTile);
		ps->tileCnt[uint8(til->getType())]++;
	}

	// place the tile
	Draglet* dlt = static_cast<Draglet*>(but);
	til->setType(valToEnum<Tile::Type>(Tile::colors, dlt->color));
	if (--ps->tileCnt[uint8(til->getType())] == 0) {
		dlt->setColor(dlt->color * 0.5f);
		dlt->setLcall(nullptr);
	}
}

void Program::eventPlacePiece(Button* but) {
	vec2b pos;
	Piece* pce;
	BoardObject* bob = pickBob(pos, pce);
	if (!bob)
		return;

	// remove any piece that may be occupying that tile already
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (pce) {
		Draglet* ico = static_cast<Draglet*>(ps->picons->getWidget(uint8(pce->getType()) + 1));
		ico->setColor(BoardObject::defaultColor);
		ico->setLcall(&Program::eventPlacePiece);
		ps->pieceCnt[uint8(pce->getType())]++;
	}

	// find the first not placed piece of the specified type
	Draglet* dlt = static_cast<Draglet*>(but);
	Piece::Type ptyp = strToEnum<Piece::Type>(Piece::names, dlt->bgTex->getName());
	Piece* pieces = game.getOwnPieces(ptyp);
	sizet id = 0;
	while (id < Piece::amounts[uint8(ptyp)] && pieces[id].getPos().hasNot(-1))
		id++;

	// place it if exists
	if (id < Piece::amounts[uint8(ptyp)]) {
		pieces[id].setPos(pos);
		if (--ps->pieceCnt[uint8(pieces[id].getType())] == 0) {
			dlt->setColor(dlt->color * 0.5f);
			dlt->setLcall(nullptr);
		}
	}
}

void Program::eventMoveTile(BoardObject* obj) {
	// switch types with destination tile
	if (Tile* dst = World::scene()->pickObject<Tile>()) {
		Tile* src = static_cast<Tile*>(obj);
		Tile::Type desType = dst->getType();
		dst->setType(src->getType());
		src->setType(desType);
	}
}

void Program::eventMovePiece(BoardObject* obj) {
	// get new position and set or swap positions with possibly already occupying piece
	vec2b pos;
	if (Piece* dst; BoardObject* bob = pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->setPos(pos);
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
	vec2b pos;
	Piece* pce;
	BoardObject* bob = pickBob(pos, pce);
	if (!bob || game.getTile(pos)->getType() != Tile::Type::fortress)	// tile needs to be a fortress
		return;

	// kill any piece that might be occupying the tile
	if (pce)
		game.killPiece(pce);

	// place the dragon
	Piece* maid = game.getOwnPieces(Piece::Type::dragon);
	maid->mode |= Object::INFO_SHOW;
	game.placePiece(maid, pos);

	// get rid of button
	ProgMatch* pm = static_cast<ProgMatch*>(state.get());
	pm->dragonIcon->getParent()->deleteWidget(pm->dragonIcon->getID());
}

void Program::eventMove(BoardObject* obj) {
	vec2b pos;
	if (Piece* pce; BoardObject* bob = pickBob(pos, pce))
		game.movePiece(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventAttack(BoardObject* obj) {
	if (Piece* pce = World::scene()->pickObject<Piece>())
		game.attackPiece(static_cast<Piece*>(obj), pce);
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

BoardObject* Program::pickBob(vec2b& pos, Piece*& pce) {
	BoardObject* bob = World::scene()->pickObject<BoardObject>();
	if (bob) {
		pos = bob->getPos();
		if (!(pce = dynamic_cast<Piece*>(bob)))
			pce = game.findPiece(pos);
	}
	return bob;

}
