#include "progs.h"
#include "board.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"

// PROGRAM STATE

void ProgState::chatEmbedAxisScroll(float val) {
	if (chatBox && chatBox->getParent()->getSize().pix)
		chatBox->onScroll(vec2(0.f, val * axisScrollThrottle * World::window()->getDeltaSec()));
}

void ProgState::eventCameraReset() {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.setPos(Camera::posSetup, Camera::latSetup);
}

void ProgState::eventCameraLeft(float val) {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.rotate(vec2(0.f, val * World::window()->getDeltaSec()), 0.f);
}

void ProgState::eventCameraRight(float val) {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.rotate(vec2(0.f, -val * World::window()->getDeltaSec()), 0.f);
}

void ProgState::eventCameraUp(float val) {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.rotate(vec2(val * World::window()->getDeltaSec(), 0.f), 0.f);
}

void ProgState::eventCameraDown(float val) {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.rotate(vec2(-val * World::window()->getDeltaSec(), 0.f), 0.f);
}

void ProgState::eventConfirm() {
	World::input()->mouseLast = false;
	World::scene()->onConfirm();
}

void ProgState::eventCancel() {
	World::input()->mouseLast = false;
	World::scene()->onCancel();
}

void ProgState::eventUp() {
	World::scene()->navSelect(Direction::up);
}

void ProgState::eventDown() {
	World::scene()->navSelect(Direction::down);
}

void ProgState::eventLeft() {
	World::scene()->navSelect(Direction::left);
}

void ProgState::eventRight() {
	World::scene()->navSelect(Direction::right);
}

void ProgState::eventStartCamera() {
	if (World::scene()->camera.state != Camera::State::animating) {
		World::scene()->camera.state = Camera::State::dragging;
#ifndef __EMSCRIPTEN__
		World::scene()->deselect();
		SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
	}
}

void ProgState::eventStopCamera() {
	if (World::scene()->camera.state != Camera::State::animating) {
		World::scene()->camera.state = Camera::State::stationary;
#ifndef __EMSCRIPTEN__
		SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
		World::input()->simulateMouseMove();
	}
}

void ProgState::eventChat() {
	World::program()->eventToggleChat();
}

void ProgState::eventFrameCounter() {
	World::program()->eventCycleFrameCounter();
}

void ProgState::eventSelect0() {
	eventSetSelected(0);
}

void ProgState::eventSelect1() {
	eventSetSelected(1);
}

void ProgState::eventSelect2() {
	eventSetSelected(2);
}

void ProgState::eventSelect3() {
	eventSetSelected(3);
}

void ProgState::eventSelect4() {
	eventSetSelected(4);
}

void ProgState::eventSelect5() {
	eventSetSelected(5);
}

void ProgState::eventSelect6() {
	eventSetSelected(6);
}

void ProgState::eventSelect7() {
	eventSetSelected(7);
}

void ProgState::eventSelect8() {
	eventSetSelected(8);
}

void ProgState::eventSelect9() {
	eventSetSelected(9);
}

void ProgState::eventOpenSettings() {
	if (Popup* pop = World::scene()->getPopup(); !pop)
		World::program()->eventShowSettings();
	else if (pop->getType() == Popup::Type::settings)
		World::program()->eventCloseScrollingPopup();
}

uint8 ProgState::switchButtons(uint8 but) {
	return but;
}

vector<Overlay*> ProgState::createOverlays() {
	return World::netcp() ? vector<Overlay*>{
		World::pgui()->createNotification(notification),
		World::pgui()->createFpsCounter(fpsText)
	} : vector<Overlay*>{
		World::pgui()->createFpsCounter(fpsText)
};
}

void ProgState::initObjectDrag(BoardObject* bob, Mesh* mesh, vec2 pos) {
	objectDragMesh = mesh;
	objectDragPos = pos;
	World::scene()->setCapture(bob);
}

void ProgState::setObjectDragPos(vec2 pos) {
	objectDragPos = pos;
	if (BoardObject* bob = dynamic_cast<BoardObject*>(World::scene()->getCapture())) {
		objectDragMesh->getInstanceTop().model = Object::getTransform(vec3(objectDragPos.x, bob->getTopY(), objectDragPos.y), bob->getRot(), bob->getScl());
		objectDragMesh->updateInstanceTop(offsetof(Mesh::Instance, model), sizeof(Mesh::Instance::model));
	}
}

bool ProgState::showNotification() const {
	return notification ? notification->getShow() : false;
}

void ProgState::showNotification(bool yes) const {
	if (notification)
		notification->setShow(yes);
}

tuple<string, bool, bool> ProgState::chatState() {
	Overlay* lay = chatBox ? dynamic_cast<Overlay*>(chatBox->getParent()) : nullptr;
	return tuple(chatBox ? chatBox->moveText() : string(), showNotification(), lay ? lay->getShow() : chatBox && !chatBox->getParent()->getSize().usePix);
}

void ProgState::chatState(string&& text, bool notif, bool show) {
	if (chatBox) {
		chatBox->setText(std::move(text));
		if (show) {
			if (Overlay* lay = dynamic_cast<Overlay*>(chatBox->getParent()))
				lay->setShow(true);
			else
				toggleChatEmbedShow(true);
		}
	}
	showNotification(notif && !show);
}

void ProgState::toggleChatEmbedShow(bool forceShow) {
	if (chatBox->getParent()->getSize().usePix || forceShow) {
		chatBox->getParent()->setSize(GuiGen::chatEmbedSize);
		if (!forceShow) {
			LabelEdit* le = chatBox->getParent()->getWidget<LabelEdit>(chatBox->getIndex() + 1);
			le->onClick(le->position(), SDL_BUTTON_LEFT);
		}
		notification->setShow(false);
	} else {
		chatBox->getParent()->getWidget<LabelEdit>(chatBox->getIndex() + 1)->cancel();
		hideChatEmbed();
	}
}

void ProgState::hideChatEmbed() {
	chatBox->getParent()->setSize(0);
	World::scene()->updateSelect();
}

// PROG MENU

void ProgMenu::eventEscape() {
	World::window()->close();
}

void ProgMenu::eventFinish() {
	World::program()->eventConnectServer();
}

uptr<RootLayout> ProgMenu::createLayout(Interactable*& selected) {
	return World::pgui()->makeMainMenu(selected, pname, versionNotif);
}

// PROG LOBBY

ProgLobby::ProgLobby(vector<pair<string, bool>>&& roomList) :
	ProgState(),
	roomBuff(std::move(roomList))
{
	std::sort(roomBuff.begin(), roomBuff.end(), [](pair<string, bool>& a, pair<string, bool>& b) -> bool { return strnatless(a.first, b.first); });
}

void ProgLobby::eventEscape() {
	World::program()->eventExitLobby();
}

void ProgLobby::eventFinish() {
	World::program()->eventHostRoomInput();
}

void ProgLobby::eventScrollUp(float val) {
	chatEmbedAxisScroll(-val);
}

void ProgLobby::eventScrollDown(float val) {
	chatEmbedAxisScroll(val);
}

uptr<RootLayout> ProgLobby::createLayout(Interactable*& selected) {
	return World::pgui()->makeLobby(selected, chatBox, rooms, roomBuff);
}

void ProgLobby::addRoom(string&& name) {
	vector<Widget*>::const_iterator it = std::find_if(rooms->getWidgets().begin(), rooms->getWidgets().end(), [&name](const Widget* rm) -> bool { return strnatless(reinterpret_cast<const Label*>(rm)->getText(), name); });
	rooms->insertWidget(it - rooms->getWidgets().begin(), World::pgui()->createRoom(std::move(name), true));
}

void ProgLobby::delRoom(const string& name) {
	rooms->deleteWidget(findRoom(name));
}

void ProgLobby::openRoom(const string& name, bool open) {
	Label* le = rooms->getWidget<Label>(findRoom(name));
	le->lcall = open ? &Program::eventJoinRoomRequest : nullptr;
	le->setDim(open ? 1.f : GuiGen::defaultDim);
}

bool ProgLobby::hasRoom(const string& name) const {
	return findRoom(name) < rooms->getWidgets().size();
}

sizet ProgLobby::findRoom(const string& name) const {
	return std::find_if(rooms->getWidgets().begin(), rooms->getWidgets().end(), [name](const Widget* rm) -> bool { return static_cast<const Label*>(rm)->getText() == name; }) - rooms->getWidgets().begin();
}

// PROG ROOM

ProgRoom::ProgRoom() :
	confs(FileSys::loadConfigs()),
	startConfig(World::sets()->lastConfig)
{
	setStartConfig();
}

ProgRoom::ProgRoom(string config) :
	confs(FileSys::loadConfigs()),
	startConfig(std::move(config))
{}

void ProgRoom::setStartConfig() {
	if (!confs.count(startConfig))
		startConfig = std::min_element(confs.begin(), confs.end(), [](const pair<const string, Config>& a, const pair<const string, Config>& b) -> bool { return strnatless(a.first, b.first); })->first;
	confs[startConfig].checkValues();
}

void ProgRoom::eventEscape() {
	World::program()->info & Program::INF_UNIQ ? World::program()->eventOpenMainMenu() : World::program()->eventExitRoom();
}

void ProgRoom::eventFinish() {
	World::prun(rio.start->lcall, rio.start);
}

void ProgRoom::eventScrollUp(float val) {
	chatEmbedAxisScroll(-val);
}

void ProgRoom::eventScrollDown(float val) {
	chatEmbedAxisScroll(val);
}

uptr<RootLayout> ProgRoom::createLayout(Interactable*& selected) {
	uptr<RootLayout> root = World::pgui()->makeRoom(selected, wio, rio, chatBox, configName, confs, startConfig);
	updateStartButton();
	updateDelButton();
	return root;
}

void ProgRoom::updateStartButton() {
	if (World::program()->info & Program::INF_UNIQ) {
#ifndef __EMSCRIPTEN__
		rio.start->setText("Open");
		rio.start->lcall = &Program::eventHostServer;
#endif
	} else if (World::program()->info & Program::INF_HOST) {
		bool on = World::program()->info & Program::INF_GUEST_WAITING;
		rio.start->setText(on ? "Start" : "Waiting for player...");
		rio.start->lcall = on ? &Program::eventStartGame : nullptr;
		rio.host->lcall = on ? &Program::eventTransferHost : nullptr;
		rio.host->setDim(on ? 1.f : GuiGen::defaultDim);
		rio.kick->lcall = on ? &Program::eventKickPlayer : nullptr;
		rio.kick->setDim(on ? 1.f : GuiGen::defaultDim);
	} else
		rio.start->setText("Waiting for start...");
}

void ProgRoom::updateDelButton() {
	if (World::program()->info & Program::INF_HOST) {
		bool on = confs.size() > 1;
		rio.del->lcall = on ? &Program::eventConfigDelete : nullptr;
		rio.del->setDim(on ? 1.f : GuiGen::defaultDim);
	}
}

void ProgRoom::updateConfigWidgets(const Config& cfg) {
	wio.victoryPoints->on = cfg.opts & Config::victoryPoints;
	wio.victoryPointsNum->setText(toStr(cfg.victoryPointsNum));
	wio.vpEquidistant->on = cfg.opts & Config::victoryPointsEquidistant;
	wio.ports->on = cfg.opts & Config::ports;
	wio.rowBalancing->on = cfg.opts & Config::rowBalancing;
	wio.homefront->on = cfg.opts & Config::homefront;
	wio.setPieceBattle->on = cfg.opts & Config::setPieceBattle;
	wio.setPieceBattleNum->setText(toStr(cfg.setPieceBattleNum));
	wio.width->setText(toStr(cfg.homeSize.x));
	wio.height->setText(toStr(cfg.homeSize.y));
	if (World::program()->info & Program::INF_HOST)
		wio.battleSL->setVal(cfg.battlePass);
	wio.battleLE->setText(toStr(cfg.battlePass) + '%');
	wio.favorTotal->on = cfg.opts & Config::favorTotal;
	wio.favorLimit->setText(toStr(cfg.favorLimit));
	wio.firstTurnEngage->on = cfg.opts & Config::firstTurnEngage;
	wio.terrainRules->on = cfg.opts & Config::terrainRules;
	wio.dragonLate->on = cfg.opts & Config::dragonLate;
	wio.dragonStraight->on = cfg.opts & Config::dragonStraight;
	setAmtSliders(cfg, cfg.tileAmounts.data(), wio.tiles.data(), wio.tileFortress, tileLim, cfg.opts & Config::rowBalancing ? cfg.homeSize.y : 0, &Config::countFreeTiles, GuiGen::tileFortressString);
	setAmtSliders(cfg, cfg.middleAmounts.data(), wio.middles.data(), wio.middleFortress, tileLim, 0, &Config::countFreeMiddles, GuiGen::middleFortressString);
	setAmtSliders(cfg, cfg.pieceAmounts.data(), wio.pieces.data(), wio.pieceTotal, pieceLim, 0, &Config::countFreePieces, GuiGen::pieceTotalString);
	wio.winFortress->setText(toStr(cfg.winFortress));
	updateWinSlider(wio.winFortress, cfg.winFortress, cfg.countFreeTiles());
	wio.winThrone->setText(toStr(cfg.winThrone));
	updateWinSlider(wio.winThrone, cfg.winThrone, cfg.pieceAmounts[uint8(PieceType::throne)]);
	for (uint8 i = 0; i < pieceLim; ++i)
		wio.capturers[i]->selected = cfg.capturers & (1 << i);
}

void ProgRoom::setAmtSliders(const Config& cfg, const uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Config::*counter)() const, string (*totstr)(const Config&)) {
	for (uint8 i = 0; i < cnt; ++i)
		wgts[i]->setText(toStr(amts[i]));
	total->setText(totstr(cfg));
	updateAmtSliders(amts, wgts, cnt, min, (cfg.*counter)());
}

void ProgRoom::updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest) {
	if (World::program()->info & Program::INF_HOST) {
		Layout* bline = wgts[0]->getParent();
		for (uint8 i = 0; i < cnt; ++i)
			bline->getParent()->getWidget<Layout>(bline->getIndex() + i)->getWidget<Slider>(1)->set(amts[i], min, amts[i] + rest);
	}
}

void ProgRoom::updateWinSlider(Label* amt, uint16 val, uint16 max) {
	if (World::program()->info & Program::INF_HOST)
		amt->getParent()->getWidget<Slider>(amt->getIndex() - 1)->set(val, 0, max);
}

// PROG GAME

void ProgGame::eventScrollUp(float val) {
	axisScroll(-val);
}

void ProgGame::eventScrollDown(float val) {
	axisScroll(val);
}

void ProgGame::eventOpenConfig() {
	if (Popup* pop = World::scene()->getPopup(); !pop)
		World::program()->eventShowConfig();
	else if (pop->getType() == Popup::Type::config)
		World::program()->eventCloseScrollingPopup();
}

void ProgGame::axisScroll(float val) {
	if (Popup* pop = World::scene()->getPopup(); pop && pop->getType() == Popup::Type::config)
		mainScrollContent->onScroll(vec2(0.f, val * axisScrollThrottle * World::window()->getDeltaSec()));
	else if (chatBox && static_cast<Overlay*>(chatBox->getParent())->getShow())
		chatBox->onScroll(vec2(0.f, val * axisScrollThrottle * World::window()->getDeltaSec()));
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
	if (World::netcp())
		return vector<Overlay*>{
			World::pgui()->createGameMessage(message, dynamic_cast<ProgSetup*>(this)),
			World::pgui()->createGameChat(chatBox),
			World::pgui()->createNotification(notification),
			World::pgui()->createFpsCounter(fpsText)
		};
	return ProgState::createOverlays();
}

// PROG SETUP

void ProgSetup::eventEscape() {
	stage > Stage::tiles && stage < Stage::preparation ? World::program()->eventSetupBack() : World::program()->eventAbortGame();
}

void ProgSetup::eventEnter() {
	bswapIcon->selected ? handleClearing() : handlePlacing();
}

void ProgSetup::eventWheel(int ymov) {
	eventSetSelected(cycle(iselect, uint8(counters.size()), int8(ymov)));
}

void ProgSetup::eventDrag(uint32 mStat) {
	if (bswapIcon->selected)
		mStat = swapBits(mStat, SDL_BUTTON_LEFT - 1, SDL_BUTTON_RIGHT - 1);

	uint8 curButton = mStat & SDL_BUTTON_LMASK ? SDL_BUTTON_LEFT : mStat & SDL_BUTTON_RMASK ? SDL_BUTTON_RIGHT : 0;
	BoardObject* bo = dynamic_cast<BoardObject*>(World::scene()->getSelect());
	svec2 curHold = bo ? World::game()->board->ptog(bo->getPos()) : svec2(UINT16_MAX);
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

void ProgSetup::eventFinish() {
	if (stage < (World::netcp() ? Stage::preparation : Stage::pieces))
		World::program()->eventSetupNext();
}

void ProgSetup::eventDelete() {
	bswapIcon->selected ? handlePlacing() : handleClearing();
}

void ProgSetup::eventSelectNext() {
	eventWheel(-1);
}

void ProgSetup::eventSelectPrev() {
	eventWheel(1);
}

void ProgSetup::eventSetSelected(uint8 sel) {
	if (sel < counters.size()) {
		sio.icons->getWidget<Icon>(iselect + 1)->selected = false;
		iselect = sel;
		sio.icons->getWidget<Icon>(iselect + 1)->selected = true;
	}
}

void ProgSetup::setStage(ProgSetup::Stage stg) {
	switch (stage = stg) {
	case Stage::tiles:
		World::game()->board->setOwnTilesInteract(Tile::Interact::interact);
		World::game()->board->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->board->setOwnPiecesVisible(false);
		counters = World::game()->board->countOwnTiles();
		break;
	case Stage::middles:
		World::game()->board->setOwnTilesInteract(Tile::Interact::ignore, true);
		World::game()->board->setMidTilesInteract(Tile::Interact::interact);
		World::game()->board->setOwnPiecesVisible(false);
		World::game()->board->fillInFortress();
		counters = World::game()->board->countMidTiles();
		break;
	case Stage::pieces:
		World::game()->board->setOwnTilesInteract(Tile::Interact::recognize);
		World::game()->board->setMidTilesInteract(Tile::Interact::ignore, true);
		World::game()->board->setOwnPiecesVisible(true);
		counters = World::game()->board->countOwnPieces();
		break;
	default:
		World::game()->board->setOwnTilesInteract(Tile::Interact::ignore);
		World::game()->board->setMidTilesInteract(Tile::Interact::ignore);
		World::game()->board->disableOwnPiecesInteract(false);
		counters.clear();
	}

	bool tiles = stage < Stage::pieces;
	bool setup = stage < Stage::preparation;
	bool cnext = (tiles || World::netcp()) && stage < Stage::preparation;
	bool cback = stage > Stage::tiles && stage < Stage::preparation;
	sio.load->lcall = setup ? &Program::eventOpenSetupLoad : nullptr;
	sio.load->setDim(setup ? 1.f : GuiGen::defaultDim);
	bswapIcon->lcall = setup ? &Program::eventSwitchGameButtons : nullptr;
	bswapIcon->setDim(setup ? 1.f : GuiGen::defaultDim);
	sio.back->lcall = cback ? &Program::eventSetupBack : nullptr;
	sio.back->setDim(cback ? 1.f : GuiGen::defaultDim);
	sio.back->setTooltip(World::pgui()->makeTooltip(stage == Stage::middles ? "Go to placing homeland tiles" : stage == Stage::pieces ? "Go to placing middle row tiles" : ""));
	sio.next->lcall = cnext ? &Program::eventSetupNext : nullptr;
	sio.next->setDim(cnext ? 1.f : GuiGen::defaultDim);
	sio.next->setText(tiles || !World::netcp() ? "Next" : "Finish");
	sio.next->setTooltip(World::pgui()->makeTooltip(stage == Stage::tiles ? "Go to placing middle row tiles" : stage == Stage::middles ? "Go to placing pieces" : stage == Stage::pieces && World::netcp() ? "Confirm and finish setup" : ""));

	iselect = 0;
	if (stage <= Stage::pieces) {
		sio.icons->setWidgets(World::pgui()->createBottomIcons(tiles));
		sio.icons->getWidget<Icon>(iselect + 1)->selected = true;	// like eventSetSelected but without crashing
		for (sizet i = 0; i < counters.size(); ++i)
			switchIcon(i, counters[i]);
	} else
		sio.icons->setWidgets({});
}

uint8 ProgSetup::findNextSelect(bool fwd) {
	uint8 m = btom<uint8>(fwd);
	for (uint8 i = iselect + m; i < counters.size(); i += m)
		if (counters[i])
			return i;
	for (uint8 i = fwd ? 0 : counters.size() - 1; i != iselect; i += m)
		if (counters[i])
			return i;
	return iselect;
}

void ProgSetup::handlePlacing() {
	if (stage <= Stage::middles) {
		if (Tile* til = dynamic_cast<Tile*>(World::scene()->getSelect()); til && til->getType() != TileType::empty)
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

uptr<RootLayout> ProgSetup::createLayout(Interactable*& selected) {
	return World::pgui()->makeSetup(selected, sio, bswapIcon, planeSwitch);
}

Icon* ProgSetup::getIcon(uint8 type) const {
	return sio.icons->getWidget<Icon>(type + 1);
}

void ProgSetup::incdecIcon(uint8 type, bool inc) {
	if (!inc && counters[type] == 1) {
		switchIcon(type, false);
		eventSetSelected(findNextSelect(true));
	} else if (inc && !counters[type])
		switchIcon(type, true);
	counters[type] += btom<uint16>(inc);
}

void ProgSetup::switchIcon(uint8 type, bool on) {
	Icon* ico = getIcon(type);
	ico->setDim(on ? 1.f : GuiGen::defaultDim);
	ico->lcall = on ? &Program::eventIconSelect : nullptr;
}

// PROG MATCH

void ProgMatch::eventEscape() {
	World::program()->eventAbortGame();
}

void ProgMatch::eventEnter() {
	if (Piece* pce = dynamic_cast<Piece*>(World::scene()->getSelect()))
		pce->startKeyDrag(SDL_BUTTON_LEFT);
}

void ProgMatch::eventWheel(int ymov) {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.zoom(ymov);
}

void ProgMatch::eventFinish() {
	if (mio.turn->lcall)
		World::program()->eventEndTurn();
}

void ProgMatch::eventSurrender() {
	World::program()->eventSurrender();
}

void ProgMatch::eventEngage() {
	if (Piece* pce = dynamic_cast<Piece*>(World::scene()->getSelect()))
		pce->startKeyDrag(SDL_BUTTON_RIGHT);
}

void ProgMatch::eventDestroyOn() {
	mio.destroy->selected = true;
	World::game()->board->setPxpadPos(dynamic_cast<Piece*>(World::scene()->getCapture()));
}

void ProgMatch::eventDestroyOff() {
	mio.destroy->selected = false;
	World::game()->board->setPxpadPos(nullptr);
}

void ProgMatch::eventDestroyToggle() {
	mio.destroy->selected = !mio.destroy->selected;
	World::game()->board->setPxpadPos(mio.destroy->selected ? dynamic_cast<Piece*>(World::scene()->getCapture()) : nullptr);
}

void ProgMatch::eventHasten() {
	setIcons(Favor::hasten);
}

void ProgMatch::eventAssault() {
	setIcons(Favor::assault);
}

void ProgMatch::eventConspire() {
	setIcons(Favor::conspire);
}

void ProgMatch::eventDeceive() {
	setIcons(Favor::deceive);
}

void ProgMatch::eventCameraReset() {
	if (World::scene()->camera.state != Camera::State::animating)
		World::scene()->camera.setPos(Camera::posMatch, Camera::latMatch);
}

void ProgMatch::eventCameraLeft(float val) {
	if (World::scene()->camera.state != Camera::State::animating) {
		val *= -World::window()->getDeltaSec();
		World::scene()->camera.rotate(vec2(0.f, val), val);
	}
}

void ProgMatch::eventCameraRight(float val) {
	if (World::scene()->camera.state != Camera::State::animating) {
		val *= World::window()->getDeltaSec();
		World::scene()->camera.rotate(vec2(0.f, val), val);
	}
}

void ProgMatch::setIcons(Favor favor, Icon* homefront) {
	if (mio.establish && mio.establish->selected && mio.establish != homefront)
		mio.establish->selected = false;
	if (mio.rebuild && mio.rebuild->selected && mio.rebuild != homefront)
		mio.rebuild->selected = false;
	if (mio.spawn && mio.spawn->selected && mio.spawn != homefront) {
		if (mio.spawn != homefront)
			mio.spawn->selected = false;
		World::game()->board->resetTilesAfterSpawn();
	}
	if (mio.dragon && mio.dragon->selected) {
		if (mio.dragon != homefront)
			mio.dragon->selected = false;
		World::scene()->setCapture(nullptr);	// stop dragging dragon
	}

	Favor old = selectFavorIcon(favor);
	World::game()->finishFavor(favor, old);
	World::game()->board->setFavorInteracts(favor, World::game()->getOwnRec());
}

Favor ProgMatch::selectFavorIcon(Favor& type) {
	Favor old = favorIconSelect();
	if (old != Favor::none && old != type)
		mio.favors[uint8(old)]->selected = false;
	if (type != Favor::none) {
		if (mio.favors[uint8(type)])
			mio.favors[uint8(type)]->selected = mio.favors[uint8(type)]->lcall && !mio.favors[uint8(type)]->selected;
		if (!(mio.favors[uint8(type)] && mio.favors[uint8(type)]->selected))
			type = Favor::none;
	}
	return old;
}

void ProgMatch::updateFavorIcon(Favor type, bool on) {
	if (Icon* ico = mio.favors[uint8(type)]) {
		if (World::game()->favorsCount[uint8(type)] || World::game()->favorsLeft[uint8(type)]) {
			on = on && World::game()->favorsCount[uint8(type)];
			ico->lcall = on ? &Program::eventSelectFavor : nullptr;
			ico->setDim(on ? 1.f : GuiGen::defaultDim);
		} else {
			if (Layout* box = ico->getParent(); box->getWidgets().size() > 1)
				box->deleteWidget(ico->getIndex());
			else
				box->getParent()->deleteWidget(box->getIndex());
			mio.favors[uint8(type)] = nullptr;
		}
	}
}

Favor ProgMatch::favorIconSelect() const {
	return Favor(std::find_if(mio.favors.begin(), mio.favors.end(), [](Icon* it) -> bool { return it && it->selected; }) - mio.favors.begin());
}

void ProgMatch::updateVictoryPoints(uint16 own, uint16 ene) {
	if (mio.vpOwn && mio.vpEne) {
		mio.vpOwn->setText(toStr(own));
		mio.vpEne->setText(toStr(ene));
	}
}

void ProgMatch::destroyEstablishIcon() {
	mio.establish->getParent()->deleteWidget(mio.establish->getIndex());
	mio.establish = nullptr;
}

void ProgMatch::decreaseDragonIcon() {
	if (!--unplacedDragons) {
		mio.dragon->getParent()->getParent()->deleteWidget(mio.dragon->getParent()->getIndex());
		mio.dragon = nullptr;
	}
}

void ProgMatch::updateIcons(bool fcont) {
	bool xmov = World::game()->getEneRec().info == Record::battleFail;
	bool regular = World::game()->getMyTurn() && !xmov;
	bool canSpawn = regular && std::none_of(World::game()->getOwnRec().actors.begin(), World::game()->getOwnRec().actors.end(), [](const pair<Piece*, Action>& pa) -> bool { return pa.second; });
	if (mio.establish) {
		mio.establish->selected = false;
		mio.establish->lcall = regular ? BCall(&Program::eventEstablish) : nullptr;
		mio.establish->setDim(regular ? 1.f : GuiGen::defaultDim);
	}
	if (mio.rebuild) {
		mio.rebuild->selected = false;
		mio.rebuild->lcall = regular ? BCall(&Program::eventRebuildTile) : nullptr;
		mio.rebuild->setDim(regular ? 1.f : GuiGen::defaultDim);
	}
	if (mio.spawn) {
		mio.spawn->selected = false;
		mio.spawn->lcall = canSpawn ? BCall(&Program::eventOpenSpawner) : nullptr;
		mio.spawn->setDim(canSpawn ? 1.f : GuiGen::defaultDim);
	}
	if (mio.dragon) {
		mio.dragon->selected = false;
		mio.dragon->lcall = canSpawn ? &Program::eventClickPlaceDragon : nullptr;
		mio.dragon->hcall = canSpawn ? &Program::eventHoldPlaceDragon : nullptr;
		mio.dragon->setDim(canSpawn ? 1.f : GuiGen::defaultDim);
	}

	bool canTurn = xmov || fcont || World::game()->hasDoneAnything();
	mio.turn->lcall = canTurn ? &Program::eventEndTurn : nullptr;
	mio.turn->setDim(canTurn ? 1.f : GuiGen::defaultDim);

	bool hasFavors = regular && std::any_of(World::game()->favorsCount.begin(), World::game()->favorsCount.end(), [](uint16 cnt) -> bool { return cnt; });
	bool canFavor = hasFavors && std::all_of(World::game()->getOwnRec().actors.begin(), World::game()->getOwnRec().actors.end(), [](const pair<Piece*, Action>& pa) -> bool { return !(pa.second & ~ACT_MS); });
	for (uint8 i = 0; i < favorMax; ++i)
		updateFavorIcon(Favor(i), canFavor);
}

bool ProgMatch::selectHomefrontIcon(Icon* ico) {
	setIcons(Favor::none, ico);	// takes care of resetting the pieces, thus no need to reset for the establishIcon or rebuildIcon
	return ico ? (ico->selected = !ico->selected) : false;
}

uptr<RootLayout> ProgMatch::createLayout(Interactable*& selected) {
	return World::pgui()->makeMatch(selected, mio, bswapIcon, planeSwitch, unplacedDragons);
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	World::program()->eventOpenMainMenu();
}

void ProgSettings::eventFinish() {
	World::program()->eventOpenInfo();
}

uptr<RootLayout> ProgSettings::createLayout(Interactable*& selected) {
	return World::pgui()->makeSettings(selected, mainScrollContent, keyBindingsStart);
}

// PROG INFO

void ProgInfo::eventEscape() {
	World::program()->eventOpenSettings();
}

void ProgInfo::eventFinish() {
	World::program()->eventOpenSettings();
}

void ProgInfo::eventScrollUp(float val) {
	mainScrollContent->onScroll(vec2(0.f, -val * axisScrollThrottle * World::window()->getDeltaSec()));
}

void ProgInfo::eventScrollDown(float val) {
	mainScrollContent->onScroll(vec2(0.f, val * axisScrollThrottle * World::window()->getDeltaSec()));
}

uptr<RootLayout> ProgInfo::createLayout(Interactable*& selected) {
	return World::pgui()->makeInfo(selected, mainScrollContent);
}
