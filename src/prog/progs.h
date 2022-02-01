#pragma once

#include "guiGen.h"

// for handling program state specific things that occur in all states
class ProgState {
public:
	ScrollArea* mainScrollContent = nullptr;
	sizet keyBindingsStart;	// only for when viewing settings

protected:
	static constexpr float axisScrollThrottle = 4000.f;

	TextBox* chatBox = nullptr;
	Overlay* notification = nullptr;
	Label* fpsText;
private:
	Mesh* objectDragMesh;
	vec2 objectDragPos;

public:
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
	void eventScreenshot();
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
	virtual void eventOpenConfig() {}
	virtual void eventOpenSettings();

	virtual uint8 switchButtons(uint8 but);
	virtual uptr<RootLayout> createLayout(Interactable*& selected) = 0;
	virtual vector<Overlay*> createOverlays();
	virtual void updateTitleBar();

	void initObjectDrag(BoardObject* bob, Mesh* mesh, vec2 pos);

	Mesh* getObjectDragMesh();
	void setObjectDragPos(vec2 pos);
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
	static void setTitleBarInteractivity(bool settings, bool config);
};

inline Mesh* ProgState::getObjectDragMesh() {
	return objectDragMesh;
}

inline TextBox* ProgState::getChat() {
	return chatBox;
}

inline Label* ProgState::getFpsText() {
	return fpsText;
}

class ProgMenu : public ProgState {
public:
	LabelEdit* pname;
	Label* versionNotif;

	~ProgMenu() override = default;

	void eventEscape() override;
	void eventFinish() override;

	uptr<RootLayout> createLayout(Interactable*& selected) override;
};

class ProgLobby : public ProgState {
private:
	ScrollArea* rooms;
	vector<pair<string, bool>> roomBuff;

public:
	ProgLobby(vector<pair<string, bool>>&& roomList);
	~ProgLobby() override = default;

	void eventEscape() override;
	void eventFinish() override;
	void eventScrollUp(float val) override;
	void eventScrollDown(float val) override;

	uptr<RootLayout> createLayout(Interactable*& selected) override;

	void addRoom(string&& name);
	void delRoom(const string& name);
	void openRoom(const string& name, bool open);
	bool hasRoom(const string& name) const;
private:
	 sizet findRoom(const string& name) const;
};

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
	~ProgRoom() override = default;

	void eventEscape() override;
	void eventFinish() override;
	void eventScrollUp(float val) override;
	void eventScrollDown(float val) override;

	uptr<RootLayout> createLayout(Interactable*& selected) override;

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
	string configName;

	ProgGame(string config);
	~ProgGame() override = default;

	void eventScrollUp(float val) override;
	void eventScrollDown(float val) override;
	virtual void eventOpenConfig() override;

	uint8 switchButtons(uint8 but) override;
	vector<Overlay*> createOverlays() override;
	void updateTitleBar() override;
private:
	void axisScroll(float val);
};

inline ProgGame::ProgGame(string config) :
	configName(std::move(config))
{}

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
	vector<TileType> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	uint16 piecePicksLeft;
	bool enemyReady = false;
private:
	uint8 iselect = 0;
	Stage stage;
	uint8 lastButton = 0;			// last button that was used on lastHold (0 for none)
	svec2 lastHold = { UINT16_MAX, UINT16_MAX };	// position of last object that the cursor was dragged over
	GuiGen::SetupIO sio;
	vector<uint16> counters;

public:
	using ProgGame::ProgGame;
	~ProgSetup() override = default;

	void eventEscape() override;	// for previous stage
	void eventEnter() override;		// for placing objects
	void eventWheel(int ymov) override;
	void eventDrag(uint32 mStat) override;
	void eventUndrag() override;
	void eventFinish() override;	// for next stage
	void eventDelete() override;
	void eventSelectNext() override;
	void eventSelectPrev() override;
	void eventSetSelected(uint8 sel) override;

	uptr<RootLayout> createLayout(Interactable*& selected) override;

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

inline uint16 ProgSetup::getCount(uint8 type) const {
	return counters[type];
}

inline uint8 ProgSetup::getSelected() const {
	return iselect;
}

class ProgMatch : public ProgGame {
public:
	PieceType spawning;	// type of piece to spawn
private:
	uint16 unplacedDragons;
	GuiGen::MatchIO mio;

public:
	using ProgGame::ProgGame;
	~ProgMatch() override = default;

	void eventEscape() override;
	void eventEnter() override;
	void eventWheel(int ymov) override;
	void eventFinish() override;
	void eventSurrender() override;
	void eventEngage() override;
	void eventDestroyOn() override;
	void eventDestroyOff() override;
	void eventDestroyToggle() override;
	void eventHasten() override;
	void eventAssault() override;
	void eventConspire() override;
	void eventDeceive() override;
	void eventCameraReset() override;
	void eventCameraLeft(float val) override;
	void eventCameraRight(float val) override;

	uptr<RootLayout> createLayout(Interactable*& selected) override;

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

inline uint16 ProgMatch::getUnplacedDragons() const {
	return unplacedDragons;
}

inline const Icon* ProgMatch::getDestroyIcon() const {
	return mio.destroy;
}

class ProgSettings : public ProgState {
public:
	~ProgSettings() override = default;

	void eventEscape() override;
	void eventFinish() override;
	void eventOpenSettings() override {}

	uptr<RootLayout> createLayout(Interactable*& selected) override;
	void updateTitleBar() override;
};

class ProgInfo : public ProgState {
public:
	~ProgInfo() override = default;

	void eventEscape() override;
	void eventFinish() override;
	void eventScrollUp(float val) override;
	void eventScrollDown(float val) override;
	void eventOpenSettings() override {}

	uptr<RootLayout> createLayout(Interactable*& selected) override;
	void updateTitleBar() override;
};
