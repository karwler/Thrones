#pragma once

#include "guiGen.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	vec2 objectDragPos;
protected:
	TextBox* chatBox;
	Overlay* notification;
	Label* fpsText;

	static constexpr float axisScrollThrottle = 4000.f;

public:
	ProgState();
	virtual ~ProgState() = default;	// to keep the compiler happy

	virtual void eventEscape() {}
	virtual void eventEnter() {}
	virtual void eventWheel(int) {}
	virtual void eventDrag(uint32) {}
	virtual void eventUndrag() {}
	virtual void eventFinish() {}
	virtual void eventSurrender() {}
	virtual void eventEngage() {}
	virtual void eventDestroyOn() {}
	virtual void eventDestroyOff() {}
	virtual void eventDestroyToggle() {}
	virtual void eventHasten() {}
	virtual void eventAssault() {}
	virtual void eventConspire() {}
	virtual void eventDeceive() {}
	virtual void eventDelete() {}
	virtual void eventSelectNext() {}
	virtual void eventSelectPrev() {}
	virtual void eventSetSelected(uint8) {}
	virtual void eventCameraReset();
	virtual void eventScrollUp(float) {}
	virtual void eventScrollDown(float) {}
	virtual void eventCameraLeft(float val);
	virtual void eventCameraRight(float val);
	void eventCameraUp(float val);
	void eventCameraDown(float val);
	void eventConfirm();
	void eventCancel();
	void eventUp();
	void eventDown();
	void eventLeft();
	void eventRight();
	void eventStartCamera();
	void eventStopCamera();
	void eventChat();
	void eventFrameCounter();
	void eventSelect0();
	void eventSelect1();
	void eventSelect2();
	void eventSelect3();
	void eventSelect4();
	void eventSelect5();
	void eventSelect6();
	void eventSelect7();
	void eventSelect8();
	void eventSelect9();

	virtual uint8 switchButtons(uint8 but);
	virtual uptr<RootLayout> createLayout(Interactable*& selected) = 0;
	virtual vector<Overlay*> createOverlays();

	TextBox* getChat();
	bool showNotification() const;
	void showNotification(bool yes) const;
	tuple<string, bool, bool> chatState();
	void chatState(string&& text, bool notif, bool show);
	Label* getFpsText();
	void toggleChatEmbedShow(bool forceShow = false);
	void hideChatEmbed();
protected:
	void chatEmbedAxisScroll(float val);
};

inline TextBox* ProgState::getChat() {
	return chatBox;
}

inline bool ProgState::showNotification() const {
	return notification ? notification->getShow() : false;
}

inline Label* ProgState::getFpsText() {
	return fpsText;
}

class ProgMenu : public ProgState {
public:
	Label* versionNotif;

	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;
	virtual void eventFinish() override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;
};

class ProgLobby : public ProgState {
private:
	ScrollArea* rooms;
	vector<pair<string, bool>> roomBuff;

public:
	ProgLobby(vector<pair<string, bool>>&& roomList);
	virtual ~ProgLobby() override = default;

	virtual void eventEscape() override;
	virtual void eventFinish() override;
	virtual void eventScrollUp(float val) override;
	virtual void eventScrollDown(float val) override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;

	void addRoom(string&& name);
	void delRoom(const string& name);
	void openRoom(const string& name, bool open);
	bool hasRoom(const string& name);
private:
	 sizet findRoom(const string& name) const;
};

inline void ProgLobby::delRoom(const string& name) {
	rooms->deleteWidget(findRoom(name));
}

inline bool ProgLobby::hasRoom(const string& name) {
	return findRoom(name) < rooms->getWidgets().size();
}

inline sizet ProgLobby::findRoom(const string& name) const {
	return sizet(std::find_if(rooms->getWidgets().begin(), rooms->getWidgets().end(), [name](const Widget* rm) -> bool { return static_cast<const Label*>(rm)->getText() == name; }) - rooms->getWidgets().begin());
}

class ProgRoom : public ProgState {
public:
	umap<string, Config> confs;
	ComboBox* configName;
	GuiGen::ConfigIO wio;
private:
	GuiGen::RoomIO rio;
	string startConfig;

public:
	ProgRoom();					// for host
	ProgRoom(string config);	// for guest
	virtual ~ProgRoom() override = default;

	virtual void eventEscape() override;
	virtual void eventFinish() override;
	virtual void eventScrollUp(float val) override;
	virtual void eventScrollDown(float val) override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;

	void setStartConfig();
	void updateStartButton();
	void updateDelButton();
	void updateConfigWidgets(const Config& cfg);
	static void setAmtSliders(const Config& cfg, const uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Config::*counter)() const, string (*totstr)(const Config&));
	static void updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest);
private:
	static void updateWinSlider(Label* amt, uint16 val, uint16 max);
};

class ProgGame : public ProgState {
public:
	Navigator* planeSwitch;
	Label* message;
	Icon* bswapIcon;
	ScrollArea* configList;
	string configName;

	ProgGame(string config);
	virtual ~ProgGame() override = default;

	virtual void eventScrollUp(float val) override;
	virtual void eventScrollDown(float val) override;

	virtual uint8 switchButtons(uint8 but) override;
	virtual vector<Overlay*> createOverlays() override;
private:
	void axisScroll(float val);
};

class ProgSetup : public ProgGame {
public:
	enum class Stage : uint8 {
		tiles,
		middles,
		pieces,
		preparation,
		ready
	};

	umap<string, Setup> setups;
	vector<Tile::Type> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	uint16 piecePicksLeft;
	bool enemyReady;
private:
	uint8 iselect;
	Stage stage;
	uint8 lastButton;	// last button that was used on lastHold (0 for none)
	svec2 lastHold;		// position of last object that the cursor was dragged over
	GuiGen::SetupIO sio;
	vector<uint16> counters;

public:
	ProgSetup(string config);
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;	// for previous stage
	virtual void eventEnter() override;		// for placing objects
	virtual void eventWheel(int ymov) override;
	virtual void eventDrag(uint32 mStat) override;
	virtual void eventUndrag() override;
	virtual void eventFinish() override;	// for next stage
	virtual void eventDelete() override;
	virtual void eventSelectNext() override;
	virtual void eventSelectPrev() override;
	virtual void eventSetSelected(uint8 sel) override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;

	Stage getStage() const;
	void setStage(Stage stg);	// returns true if match is ready to load
	Icon* getIcon(uint8 type) const;
	void incdecIcon(uint8 type, bool inc);
	uint16 getCount(uint8 type) const;
	uint8 getSelected() const;
private:
	uint8 findNextSelect(bool fwd);
	void switchIcon(uint8 type, bool on);
	void handlePlacing();
	void handleClearing();
};

inline ProgSetup::Stage ProgSetup::getStage() const {
	return stage;
}

inline Icon* ProgSetup::getIcon(uint8 type) const {
	return sio.icons->getWidget<Icon>(type + 1);
}

inline uint16 ProgSetup::getCount(uint8 type) const {
	return counters[type];
}

inline uint8 ProgSetup::getSelected() const {
	return iselect;
}

class ProgMatch : public ProgGame {
public:
	Piece::Type spawning;	// type of piece to spawn
private:
	uint16 unplacedDragons;
	GuiGen::MatchIO mio;

public:
	using ProgGame::ProgGame;
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;
	virtual void eventWheel(int ymov) override;
	virtual void eventFinish() override;
	virtual void eventSurrender() override;
	virtual void eventEngage() override;
	virtual void eventDestroyOn() override;
	virtual void eventDestroyOff() override;
	virtual void eventDestroyToggle() override;
	virtual void eventHasten() override;
	virtual void eventAssault() override;
	virtual void eventConspire() override;
	virtual void eventDeceive() override;
	virtual void eventCameraReset() override;
	virtual void eventCameraLeft(float val) override;
	virtual void eventCameraRight(float val) override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;

	void setIcons(Favor favor, Icon* homefront = nullptr);	// set favor or a homefront icon
	const array<Icon*, favorMax>& getFavorIcons() const;
	Favor favorIconSelect() const;
	Favor selectFavorIcon(Favor& type);	// returns the previous favor and sets type to none if not selectable
	uint16 getUnplacedDragons() const;
	void updateFavorIcon(Favor type, bool on);
	void updateVictoryPoints(uint16 own, uint16 ene);
	void destroyEstablishIcon();
	void decreaseDragonIcon();
	void updateIcons(bool fcont = false);
	bool selectHomefrontIcon(Icon* ico);	// does the above plus resetting the favors and returns whether the icons is selected
	const Icon* getDestroyIcon() const;
};

inline const array<Icon*, favorMax>& ProgMatch::getFavorIcons() const {
	return mio.favors;
}

inline Favor ProgMatch::favorIconSelect() const {
	return Favor(std::find_if(mio.favors.begin(), mio.favors.end(), [](Icon* it) -> bool { return it && it->selected; }) - mio.favors.begin());
}

inline uint16 ProgMatch::getUnplacedDragons() const {
	return unplacedDragons;
}

inline const Icon* ProgMatch::getDestroyIcon() const {
	return mio.destroy;
}

class ProgSettings : public ProgState {
public:
	ScrollArea* content;
	sizet bindingsStart;
private:
	umap<string, uint32> pixelformats;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	virtual void eventFinish() override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;

	const umap<string, uint32>& getPixelformats();
};

inline const umap<string, uint32>& ProgSettings::getPixelformats() {
	return pixelformats;
}

class ProgInfo : public ProgState {
private:
	ScrollArea* content;

public:
	virtual ~ProgInfo() override = default;

	virtual void eventEscape() override;
	virtual void eventFinish() override;
	virtual void eventScrollUp(float val) override;
	virtual void eventScrollDown(float val) override;

	virtual uptr<RootLayout> createLayout(Interactable*& selected) override;
};
