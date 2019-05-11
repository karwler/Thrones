#include "engine/world.h"
#include <cassert>

const string Program::argAddress = "s";
const string Program::argPort = "p";
const string Program::argConnect = "c";
const string Program::argSetup = "d";

void Program::start() {
	if (const string& addr = World::getOpt(argAddress); !addr.empty())
		World::sets()->address = addr;
	else if (World::hasFlag(argAddress))
		World::sets()->address = Com::loopback;

	if (const string& port = World::getOpt(argPort); !port.empty())
		World::sets()->port = uint16(sstoul(port));
	else if (World::hasFlag(argPort))
		World::sets()->port = Com::defaultPort;

	World::scene()->setObjects(game.initObjects());	// doesn't need to be here but I like having the game board in the background
	eventOpenMainMenu();
	if (World::hasFlag(argConnect))
		eventConnectServer(nullptr);
}

// MAIN MENU

void Program::eventOpenMainMenu(Button*) {
	game.uninitObjects();	// either do this or reset objects
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
	state.reset(new ProgSetup);
#ifdef NDEBUG
	World::scene()->setObjects(game.initObjects());
#else
	World::scene()->setObjects(World::hasFlag(argSetup) ? game.initDummyObjects() : game.initObjects());
#endif
	static_cast<ProgSetup*>(state.get())->setStage(ProgSetup::Stage::tiles);
}

void Program::eventPlaceTileC(BoardObject* obj) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (uint8 tid = ps->getSelected(); ps->getCount(tid))
		placeTile(static_cast<Tile*>(obj), tid);
}

void Program::eventPlaceTileD(Button* but) {
	if (Tile* tile = dynamic_cast<Tile*>(World::scene()->select))
		placeTile(tile, uint8(but->getID() - 1));
}

void Program::placeTile(Tile* tile, uint8 type) {
	// remove any piece that may already be there
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (tile->getType() < Tile::Type::fortress)
		ps->incdecIcon(uint8(tile->getType()), true, true);

	// place the tile
	tile->setType(Tile::Type(type));
	tile->setCalls(Tile::Interactivity::tiling);
	ps->incdecIcon(type, false, true);
}

void Program::eventPlacePieceC(BoardObject* obj) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (uint8 tid = ps->getSelected(); ps->getCount(tid)) {
		vec2b pos = obj->getPos();
		placePiece(pos, tid, extractPiece(obj, pos));
	}
}

void Program::eventPlacePieceD(Button* but) {
	vec2b pos;
	if (Piece* pce; pickBob(pos, pce))
		placePiece(pos, uint8(but->getID() - 1), pce);
}

void Program::placePiece(vec2b pos, uint8 type, Piece* occupant) {
	// remove any piece that may be occupying that tile already
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (occupant) {
		occupant->disable();
		ps->incdecIcon(uint8(occupant->getType()), true, false);
	}

	// find the first not placed piece of the specified type and place it if it exists
	Piece* pieces = game.getOwnPieces(Piece::Type(type));
	for (uint8 i = 0; i < Piece::amounts[type]; i++)
		if (!pieces[i].active()) {
			pieces[i].enable(pos);
			ps->incdecIcon(type, false, false);
			break;
		}
}

void Program::eventMoveTile(BoardObject* obj) {
	// switch types with destination tile
	if (Tile* src = static_cast<Tile*>(obj); Tile* dst = dynamic_cast<Tile*>(World::scene()->select)) {
		Tile::Type desType = dst->getType();
		dst->setType(src->getType());
		dst->setCalls(Tile::Interactivity::tiling);
		src->setType(desType);
		src->setCalls(Tile::Interactivity::tiling);
	}
}

void Program::eventMovePiece(BoardObject* obj) {
	// get new position and set or swap positions with possibly already occupying piece
	vec2b pos;
	if (Piece* dst; pickBob(pos, dst)) {
		Piece* src = static_cast<Piece*>(obj);
		if (dst)
			dst->setPos(src->getPos());
		src->setPos(pos);
	}
}

void Program::eventClearTile(BoardObject* obj) {
	Tile* til = static_cast<Tile*>(obj);

	static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(til->getType()), true, true);
	til->setType(Tile::Type::empty);
	til->setCalls(Tile::Interactivity::tiling);
}

void Program::eventClearPiece(BoardObject* obj) {
	if (Piece* pce = extractPiece(obj, obj->getPos())) {
		static_cast<ProgSetup*>(state.get())->incdecIcon(uint8(pce->getType()), true, false);
		pce->disable();
	}
}

void Program::eventSetupNext(Button*) {
	try {
		ProgSetup* ps = static_cast<ProgSetup*>(state.get());
		switch (ps->getStage()) {
		case ProgSetup::Stage::tiles:
			game.checkOwnTiles();
			game.fillInFortress();
			break;
		case ProgSetup::Stage::middles:
			game.checkMidTiles();
			break;
		case ProgSetup::Stage::pieces:
			game.checkOwnPieces();
		}
		if (ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) + 1))) {
			game.sendSetup();
			ps->enemyReady ? eventOpenMatch() : eventShowWaitPopup();
		}
	} catch (const string& err) {
		World::scene()->setPopup(ProgState::createPopupMessage(err, &Program::eventClosePopup));
	}
}

void Program::eventSetupBack(Button*) {
	ProgSetup* ps = static_cast<ProgSetup*>(state.get());
	if (ps->getStage() == ProgSetup::Stage::middles)
		game.takeOutFortress();
	ps->setStage(ProgSetup::Stage(uint8(ps->getStage()) - 1));
}

void Program::eventShowWaitPopup(Button*) {
	World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventExitGame, "Exit"));
}

// GAME MATCH

void Program::eventOpenMatch() {
	game.prepareMatch();
	setState(new ProgMatch);
	World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, Game::screenPosDown) })));
	World::scene()->addAnimation(Animation(World::scene()->getCamera(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posMatch, Camera::latMatch) })));
}

void Program::eventPlaceDragon(Button*) {
	vec2b pos;
	if (Piece* pce; pickBob(pos, pce))
		game.placeDragon(pos, pce);
}

void Program::eventMove(BoardObject* obj) {
	vec2b pos;
	if (Piece* pce; pickBob(pos, pce))
		game.pieceMove(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventFire(BoardObject* obj) {
	vec2b pos;
	if (Piece* pce; pickBob(pos, pce))
		game.pieceFire(static_cast<Piece*>(obj), pos, pce);
}

void Program::eventExitGame(Button*) {
	if (dynamic_cast<ProgMatch*>(state.get())) {
		World::scene()->addAnimation(Animation(game.getScreen(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS, Game::screenPosUp) })));
		World::scene()->addAnimation(Animation(World::scene()->getCamera(), queue<Keyframe>({ Keyframe(0.5f, Keyframe::CHG_POS | Keyframe::CHG_LAT, Camera::posSetup, Camera::latSetup) })));
	}
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

void Program::eventSetSmooth(Button* but) {
	World::winSys()->setSmooth(Settings::Smooth(static_cast<SwitchBox*>(but)->getCurOpt()));
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
	BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->select);
	if (bob) {
		pos = bob->getPos();
		pce = extractPiece(bob, pos);
	}
	return bob;

}
