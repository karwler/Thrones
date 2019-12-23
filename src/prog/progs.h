#pragma once

#include "utils/layouts.h"

// for handling program state specific things that occur in all states
class ProgState {
protected:
	struct Text {
		string text;
		int length, height;

		Text(string str, int height);

		static int strLen(const string& str, int height);
		template <class T> static int maxLen(T pos, T end, int height);
		static int maxLen(const vector<vector<string>*>& lists, int height);
	};

	struct ConfigIO {
		LabelEdit* width;
		LabelEdit* height;
		Slider* survivalSL;
		LabelEdit* survivalLE;
		SwitchBox* survivalMode;
		CheckBox* favorLimit;
		LabelEdit* favorMax;
		LabelEdit* dragonDist;
		CheckBox* dragonSingle;
		CheckBox* dragonDiag;
		array<LabelEdit*, Com::tileMax-1> tiles;
		array<LabelEdit*, Com::tileMax-1> middles;
		array<LabelEdit*, Com::pieceMax> pieces;
		Label* tileFortress;
		Label* middleFortress;
		Label* pieceTotal;
		LabelEdit* winFortress;
		LabelEdit* winThrone;
		LabelEdit* capturers;
		CheckBox* shiftLeft;
		CheckBox* shiftNear;
	};

	static constexpr char arrowLeft[] = "<";
	static constexpr char arrowRight[] = ">";
	static constexpr float defaultDim = 0.5f;

	int lineHeight, superHeight, tooltipHeight;
	int lineSpacing, superSpacing, iconSize;
	int tooltipLimit;

public:
	ProgState();
	virtual ~ProgState() = default;	// to keep the compiler happy

	virtual void eventEscape() {}
	virtual void eventEnter() {}
	virtual void eventNumpress(uint8) {}
	virtual void eventWheel(int) {}
	virtual void eventDrag(uint32) {}
	virtual void eventUndrag() {}
	virtual void eventFavorize(FavorAct act);
	virtual void eventEndTurn() {}
	virtual void eventCameraReset();
	void eventResize();

	virtual RootLayout* createLayout();
	Popup* createPopupMessage(string msg, BCall ccal, string ctxt = "Ok");
	Popup* createPopupChoice(string msg, BCall kcal, BCall ccal);
	pair<Popup*, Widget*> createPopupInput(string msg, BCall kcal, uint limit = UINT_MAX);
	Popup* createPopupConfig(const Com::Config& cfg);

	static string tileFortressString(const Com::Config& cfg);
	static string middleFortressString(const Com::Config& cfg);
	static string pieceTotalString(const Com::Config& cfg);
protected:
	Texture makeTooltip(const string& text);
	Texture makeTooltipL(const vector<string>& lines);

	vector<Widget*> createConfigList(ConfigIO& wio, const Com::Config& cfg, bool active);
private:
	void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id);
	void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id);
};

inline ProgState::ProgState() {
	eventResize();
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

class ProgMenu : public ProgState {
public:
	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;

	virtual RootLayout* createLayout() override;
};

class ProgLobby : public ProgState {
private:
	ScrollArea* rlist;
	vector<pair<string, bool>> rooms;

public:
	ProgLobby(vector<pair<string, bool>>&& rooms);
	virtual ~ProgLobby() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;

	virtual RootLayout* createLayout() override;

	void addRoom(string&& name);
	void delRoom(const string& name);
	void openRoom(const string& name, bool open);
	bool roomsMaxed() const;
	bool roomTaken(const string& name);
private:
	 Label* createRoom(string name, bool open);
};

inline void ProgLobby::delRoom(const string& name) {
	rlist->deleteWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin()));
}

inline bool ProgLobby::roomsMaxed() const {
	return rooms.size() >= Com::maxRooms;
}

inline bool ProgLobby::roomTaken(const string& name) {
	return std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) != rooms.end();
}

class ProgRoom : public ProgState {
public:
	umap<string, Com::Config> confs;
	ConfigIO wio;
private:
	Label* startButton;
	Label* kickButton;

public:
	ProgRoom();
	virtual ~ProgRoom() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;

	virtual RootLayout* createLayout() override;

	void updateStartButton();	// canStart only applies to State::host
	void updateConfigWidgets(const Com::Config& cfg);
	static void updateAmtSliders(const uint16* amts, LabelEdit** wgts, uint8 cnt, uint16 min, uint16 rest);
private:
	static void updateWinSlider(Label* amt, uint16 val, uint16 max);
};

class ProgSetup : public ProgState {
public:
	enum class Stage : uint8 {
		tiles,
		middles,
		pieces,
		ready
	};

	umap<string, Setup> setups;
	vector<Com::Tile> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	Label* message;
	bool enemyReady;
private:
	uint8 selected;
	Stage stage;
	uint8 lastButton;	// last button that was used on lastHold (0 for none)
	svec2 lastHold;		// position of last object that the cursor was dragged over
	vector<uint16> counters;
	Layout* icons;
	Draglet* deleteIcon;

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;
	virtual void eventNumpress(uint8 num) override;
	virtual void eventWheel(int ymov) override;
	virtual void eventDrag(uint32 mStat) override;
	virtual void eventUndrag() override;

	Stage getStage() const;
	bool setStage(Stage stg);	// returns true if match is ready to load
	Draglet* getIcon(uint8 type) const;
	void incdecIcon(uint8 type, bool inc, bool isTile);
	uint16 getCount(uint8 type) const;
	uint8 getSelected() const;
	void selectNext(bool fwd);
	bool getDeleteLock() const;
	void setDeleteLock(bool on);

	virtual RootLayout* createLayout() override;
	Popup* createPopupSaveLoad(bool save);

	void setSelected(uint8 sel);
private:
	void setInteractivity(bool dlock = false);
	Layout* makeTicons();
	Layout* makePicons();
	uint8 findNextSelect(bool fwd);
	void switchIcon(uint8 type, bool on, bool isTile);
};

inline ProgSetup::Stage ProgSetup::getStage() const {
	return stage;
}

inline Draglet* ProgSetup::getIcon(uint8 type) const {
	return static_cast<Draglet*>(icons->getWidget(type + 1));
}

inline uint16 ProgSetup::getCount(uint8 type) const {
	return counters[type];
}

inline uint8 ProgSetup::getSelected() const {
	return selected;
}

inline void ProgSetup::selectNext(bool fwd) {
	setSelected(findNextSelect(fwd));
}

inline bool ProgSetup::getDeleteLock() const {
	return deleteIcon->selected;
}

inline void ProgSetup::setDeleteLock(bool on) {
	setInteractivity(deleteIcon->selected = on);
}

class ProgMatch : public ProgState {
public:
	Label* message;
private:
	Draglet* favorIcon;
	Draglet* fnowIcon;
	Label* turnIcon;
	Layout* dragonIcon;	// has to be nullptr if dragon can't be placed anymore
	uint16 unplacedDragons;

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;
	virtual void eventWheel(int ymov) override;
	virtual void eventFavorize(FavorAct act) override;
	virtual void eventEndTurn() override;
	virtual void eventCameraReset() override;
	bool favorIconOn() const;
	bool fnowIconOn() const;
	FavorAct favorIconSelect() const;
	void selectFavorIcon(FavorAct act, bool force = true);
	void updateFavorIcon(bool on, uint8 cnt, uint8 tot);
	void updateFnowIcon(bool on = false, uint8 cnt = 0);
	void updateTurnIcon(bool on);
	void setDragonIcon(bool on);
	void decreaseDragonIcon();

	virtual RootLayout* createLayout() override;
};

inline bool ProgMatch::favorIconOn() const {
	return favorIcon && favorIcon->lcall;
}

inline bool ProgMatch::fnowIconOn() const {
	return fnowIcon && fnowIcon->lcall;
}

class ProgSettings : public ProgState {
public:
	LabelEdit* display;
	SwitchBox* screen;
	SwitchBox* winSize;
	SwitchBox* dspMode;

	static constexpr float gammaStepFactor = 10.f;
	static constexpr char rv2iSeparator[] = " x ";
private:
	umap<string, uint32> pixelformats;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;
	
	virtual RootLayout* createLayout() override;

	SDL_DisplayMode currentMode() const;
	static string dispToFstr(const SDL_DisplayMode& mode);
};

inline string ProgSettings::dispToFstr(const SDL_DisplayMode& mode) {
	return toStr(mode.w) + rv2iSeparator + toStr(mode.h) + " | " + toStr(mode.refresh_rate) + "Hz " + pixelformatName(mode.format);
}

class ProgInfo : public ProgState {
private:
	static const array<string, SDL_POWERSTATE_CHARGED+1> powerNames;

public:
	virtual ~ProgInfo() override = default;

	virtual void eventEscape() override;
	virtual void eventEnter() override;

	virtual RootLayout* createLayout() override;

private:
	void appendProgram(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	void appendCPU(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	void appendRAM(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	void appendCurrent(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles, const char* (*value)());
	void appendCurrentDisplay(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles);
	void appendDisplay(vector<Widget*>& lines, int i, int width, const vector<string>& args);
	void appendPower(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	void appendAudioDevices(vector<Widget*>& lines, int width, vector<string>& titles, int iscapture);
	void appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int));
	void appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles);
	void appendRenderers(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles);
	static string versionText(const SDL_version& ver);
};

inline string ProgInfo::versionText(const SDL_version& ver) {
	return toStr(ver.major) + '.' + toStr(ver.minor) + '.' + toStr(ver.patch);
}
