#pragma once

#include "types.h"
#include "engine/fileSys.h"
#include "utils/layouts.h"

enum class Switch : uint8 {
	off,
	on,
	toggle
};

// for handling program state specific things that occur in all states
class ProgState {
public:
	struct Text {
		string text;
		int length, height;

		Text(string str, int sh);

		static int strLen(const char* str, int height);
		template <class T> static int maxLen(T pos, T end, int height);
		static int maxLen(const initlist<initlist<const char*>>& lists, int height);
	};

	static constexpr float defaultDim = 0.5f;

	int smallHeight, lineHeight, superHeight, tooltipHeight;
	int lineSpacing, superSpacing, iconSize;
	uint tooltipLimit;
	vec2 objectDragPos;

protected:
	struct ConfigIO {
		//ComboBox* gameType;
		CheckBox* ports;
		CheckBox* rowBalancing;
		CheckBox* setPieceOn;
		LabelEdit* setPieceNum;
		LabelEdit* width;
		LabelEdit* height;
		Slider* battleSL;
		LabelEdit* battleLE;
		CheckBox* favorTotal;
		LabelEdit* favorLimit;
		CheckBox* dragonLate;
		CheckBox* dragonStraight;
		array<LabelEdit*, Com::tileLim> tiles;
		array<LabelEdit*, Com::tileLim> middles;
		array<LabelEdit*, Com::pieceMax> pieces;
		Label* tileFortress;
		Label* middleFortress;
		Label* pieceTotal;
		LabelEdit* winFortress;
		LabelEdit* winThrone;
		LabelEdit* capturers;
	};

	static constexpr float chatEmbedSize = 0.5f;
	static constexpr float secondaryScrollThrottle = 0.1f;

	TextBox* chatBox;
	Overlay* notification;
	Label* fpsText;

public:
	ProgState();
	virtual ~ProgState() = default;	// to keep the compiler happy

	virtual void eventEscape() {}
	virtual void eventEnter() {}
	virtual void eventNumpress(uint8) {}
	virtual void eventWheel(int) {}
	virtual void eventDrag(uint32) {}
	virtual void eventUndrag() {}
	virtual void eventEndTurn() {}
	virtual void eventSurrender() {}
	virtual void eventEngage() {}
	virtual void eventDestroy(Switch) {}
	virtual void eventFavor(Favor) {}
	virtual void eventCameraReset();
	virtual void eventSecondaryAxis(const ivec2&, float) {}
	void eventResize();

	virtual uint8 switchButtons(uint8 but);
	virtual RootLayout* createLayout(Interactable*& selected) = 0;
	virtual vector<Overlay*> createOverlays();
	Popup* createPopupMessage(string msg, BCall ccal, string ctxt = "Okay") const;
	Popup* createPopupChoice(string msg, BCall kcal, BCall ccal) const;
	pair<Popup*, Widget*> createPopupInput(string msg, BCall kcal, uint limit = UINT_MAX) const;
	Popup* createPopupConfig(const Com::Config& cfg, ScrollArea*& cfgList) const;
	TextBox* getChat();
	Overlay* getNotification();
	Label* getFpsText();
	void toggleChatEmbedShow();
	void hideChatEmbed();

	Text makeFpsText(float dSec) const;
	static string tileFortressString(const Com::Config& cfg);
	static string middleFortressString(const Com::Config& cfg);
	static string pieceTotalString(const Com::Config& cfg);
protected:
	static const char* pixelformatName(uint32 format);
	Texture makeTooltip(const char* text) const;
	Texture makeTooltipL(const char* text) const;

	vector<Widget*> createChat(bool overlay = true);
	vector<Widget*> createConfigList(ConfigIO& wio, const Com::Config& cfg, bool active) const;
	Overlay* createFpsCounter();
	Overlay* createNotification();
private:
	void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id) const;
	void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const;
};

inline TextBox* ProgState::getChat() {
	return chatBox;
}

inline Overlay* ProgState::getNotification() {
	return notification;
}

inline Label* ProgState::getFpsText() {
	return fpsText;
}

inline string ProgState::tileFortressString(const Com::Config& cfg) {
	return toStr(cfg.countFreeTiles());
}

inline string ProgState::middleFortressString(const Com::Config& cfg) {
	return toStr(cfg.homeSize.x - cfg.countMiddles() * 2);
}

inline string ProgState::pieceTotalString(const Com::Config& cfg) {
	return toStr(cfg.countPieces()) + '/' + toStr(cfg.homeSize.x * cfg.homeSize.y);
}

inline const char* ProgState::pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

class ProgMenu : public ProgState {
public:
	Label* versionNotif;

	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;
	virtual void eventEndTurn() override;

	virtual RootLayout* createLayout(Interactable*& selected) override;
};

class ProgLobby : public ProgState {
private:
	ScrollArea* rlist;
	vector<pair<string, bool>> rooms;

public:
	ProgLobby(vector<pair<string, bool>>&& roomList);
	virtual ~ProgLobby() override = default;

	virtual void eventEscape() override;
	virtual void eventEndTurn() override;

	virtual RootLayout* createLayout(Interactable*& selected) override;

	void addRoom(string&& name);
	void delRoom(const string& name);
	void openRoom(const string& name, bool open);
private:
	 Label* createRoom(string name, bool open);
};

inline void ProgLobby::delRoom(const string& name) {
	rlist->deleteWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin()));
}

class ProgRoom : public ProgState {
public:
	umap<string, Com::Config> confs;
	ConfigIO wio;
private:
	Label* startButton;
	Label* kickButton;

public:
	ProgRoom() = default;
	ProgRoom(umap<string, Com::Config>&& configs);
	virtual ~ProgRoom() override = default;

	virtual void eventEscape() override;
	virtual void eventEndTurn() override;
	virtual void eventSecondaryAxis(const ivec2& val, float dSec) override;

	virtual RootLayout* createLayout(Interactable*& selected) override;
	void updateStartButton();	// canStart only applies to State::host
	void updateConfigWidgets(const Com::Config& cfg);
	static void updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest);
private:
	static void setAmtSliders(const Com::Config& cfg, const uint16* amts, LabelEdit** wgts, Label* total, uint8 cnt, uint16 min, uint16 (Com::Config::*counter)() const, string (*totstr)(const Com::Config&));
	static void updateWinSlider(Label* amt, uint16 val, uint16 max);
};

class ProgGame : public ProgState {
public:
	static constexpr char msgFavorPick[] = "Pick a fate's favor";

	Navigator* planeSwitch;
	Label* message;
	Icon* bswapIcon;
	ScrollArea* configList;

	ProgGame();
	virtual ~ProgGame() override = default;

	virtual void eventSecondaryAxis(const ivec2& val, float dSec) override;

	virtual uint8 switchButtons(uint8 but) override;
	virtual vector<Overlay*> createOverlays() override;
	Popup* createPopupFavorPick(uint16 availableFF) const;
};

inline ProgGame::ProgGame() :
	configList(nullptr)
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

	static constexpr char msgPickPiece[] = "Pick piece (";

	umap<string, Setup> setups;
	vector<Com::Tile> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	uint16 piecePicksLeft;
	bool enemyReady;
private:
	uint8 iselect;
	Stage stage;
	uint8 lastButton;	// last button that was used on lastHold (0 for none)
	svec2 lastHold;		// position of last object that the cursor was dragged over
	vector<uint16> counters;
	Layout* icons;

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;	// for previous stage
	virtual void eventEnter() override;		// for placing objects
	virtual void eventNumpress(uint8 num) override;
	virtual void eventWheel(int ymov) override;
	virtual void eventDrag(uint32 mStat) override;
	virtual void eventUndrag() override;
	virtual void eventEndTurn() override;			// for next stage
	virtual void eventEngage() override;			// for removing objects
	virtual void eventFavor(Favor favor) override;	// for switching icons

	Stage getStage() const;
	void setStage(Stage stg);	// returns true if match is ready to load
	Icon* getIcon(uint8 type) const;
	void incdecIcon(uint8 type, bool inc);
	uint16 getCount(uint8 type) const;
	uint8 getSelected() const;
	void selectNext(bool fwd);

	virtual RootLayout* createLayout(Interactable*& selected) override;
	Popup* createPopupSaveLoad(bool save);
	Popup* createPopupPiecePicker();

	void setSelected(uint8 sel);
private:
	Layout* makeTicons();
	Layout* makePicons();
	uint8 findNextSelect(bool fwd);
	void switchIcon(uint8 type, bool on);
	void handlePlacing();
	void handleClearing();
};

inline ProgSetup::Stage ProgSetup::getStage() const {
	return stage;
}

inline Icon* ProgSetup::getIcon(uint8 type) const {
	return static_cast<Icon*>(icons->getWidget(type + 1));
}

inline uint16 ProgSetup::getCount(uint8 type) const {
	return counters[type];
}

inline uint8 ProgSetup::getSelected() const {
	return iselect;
}

inline void ProgSetup::selectNext(bool fwd) {
	setSelected(findNextSelect(fwd));
}

class ProgMatch : public ProgGame {
public:
	Com::Piece spawning;	// type of piece to spawn
	array<Icon*, favorMax> favorIcons;
	Icon* destroyIcon;
private:
	Label* turnIcon;
	Label* rebuildIcon;
	Label* establishIcon;
	Label* spawnIcon;
	Layout* dragonIcon;	// has to be nullptr if dragon can't be placed anymore
	uint16 unplacedDragons;

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;
	virtual void eventNumpress(uint8 num) override;
	virtual void eventWheel(int ymov) override;
	virtual void eventEndTurn() override;
	virtual void eventSurrender() override;
	virtual void eventEngage() override;
	virtual void eventDestroy(Switch sw) override;
	virtual void eventFavor(Favor favor) override;
	virtual void eventCameraReset() override;

	Favor favorIconSelect() const;
	Favor selectFavorIcon(Favor& type);	// returns the previous favor and sets type to none if not selectable
	void updateFavorIcon(Favor type, bool on);
	void updateEstablishIcon(bool on);
	void destroyEstablishIcon();
	void updateRebuildIcon(bool on);
	void updateSpawnIcon(bool on);
	void updateTurnIcon(bool on);
	void setDragonIcon(bool on);
	void decreaseDragonIcon();

	virtual RootLayout* createLayout(Interactable*& selected) override;
	Popup* createPopupSpawner() const;
};

inline Favor ProgMatch::favorIconSelect() const {
	return Favor(std::find_if(favorIcons.begin(), favorIcons.end(), [](Icon* it) -> bool { return it && it->selected; }) - favorIcons.begin());
}

class ProgSettings : public ProgState {
public:
	static constexpr float gammaStepFactor = 10.f;
	static constexpr char rv2iSeparator[] = " x ";

private:
	umap<string, uint32> pixelformats;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	virtual void eventEndTurn() override;

	virtual RootLayout* createLayout(Interactable*& selected) override;

	SDL_DisplayMode fstrToDisp(const string& str) const;
	static string dispToFstr(const SDL_DisplayMode& mode);
};

inline string ProgSettings::dispToFstr(const SDL_DisplayMode& mode) {
	return toStr(mode.w) + rv2iSeparator + toStr(mode.h) + " | " + toStr(mode.refresh_rate) + "Hz " + pixelformatName(mode.format);
}

class ProgInfo : public ProgState {
private:
	static constexpr array<const char*, SDL_POWERSTATE_CHARGED+1> powerNames = {
		"UNKNOWN",
		"ON_BATTERY",
		"NO_BATTERY",
		"CHARGING",
		"CHARGED"
	};

	ScrollArea* content;

public:
	virtual ~ProgInfo() override = default;

	virtual void eventEscape() override;
	virtual void eventEndTurn() override;
	virtual void eventSecondaryAxis(const ivec2& val, float dSec) override;

	virtual RootLayout* createLayout(Interactable*& selected) override;

private:
	void appendProgram(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles);
	void appendSystem(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles);
	void appendCurrentDisplay(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles);
	void appendDisplay(vector<Widget*>& lines, int i, int width, initlist<const char*>::iterator args);
	void appendPower(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles);
	void appendAudioDevices(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int iscapture);
	void appendDrivers(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int));
	void appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles);
	void appendRenderers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles);
	static string versionText(const SDL_version& ver);
	static string ibtos(int val);
};

inline string ProgInfo::versionText(const SDL_version& ver) {
	return toStr(ver.major) + '.' + toStr(ver.minor) + '.' + toStr(ver.patch);
}
