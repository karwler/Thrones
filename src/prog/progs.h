#pragma once

#include "engine/fileSys.h"
#include "utils/layouts.h"

// for handling program state specific things that occur in all states
class ProgState {
protected:
	static constexpr int tooltipLimit = 600;
	static constexpr int tooltipHeight = 16;
	static constexpr int lineHeight = 30;
	static constexpr int superHeight = 40;
	static constexpr int superSpacing = 10;
	static constexpr int iconSize = 64;
	static constexpr char arrowLeft[] = "<";
	static constexpr char arrowRight[] = ">";

	struct Text {
		string text;
		int length, height;

		Text(string str, int height = lineHeight);

		static int strLen(const string& str, int height = lineHeight);
	};

	struct ConfigIO {
		LabelEdit* width;
		LabelEdit* height;
		Slider* survivalSL;
		LabelEdit* survivalLE;
		LabelEdit* favors;
		LabelEdit* dragonDist;
		CheckBox* dragonDiag;
		CheckBox* multistage;
		CheckBox* survivalKill;
		array<LabelEdit*, Com::tileMax> tiles;
		array<LabelEdit*, Com::tileMax-1> middles;
		array<LabelEdit*, Com::pieceMax> pieces;
		LabelEdit* winFortress;
		LabelEdit* winThrone;
		LabelEdit* capturers;
		CheckBox* shiftLeft;
		CheckBox* shiftNear;
		Label* tileFortress;
		Label* middleFortress;
		Label* pieceTotal;
	};

public:
	virtual ~ProgState() = default;	// to keep the compiler happy

	virtual void eventEscape() {}
	virtual void eventNumpress(uint8) {}
	virtual void eventWheel(int) {}
	virtual void eventDrag(uint32) {}
	virtual void eventUndrag() {}
	virtual void eventCameraReset();

	virtual Layout* createLayout();
	static Popup* createPopupMessage(string msg, BCall ccal, string ctxt = "Ok");
	static Popup* createPopupChoice(string msg, BCall kcal, BCall ccal);
	static pair<Popup*, Widget*> createPopupInput(string msg, BCall kcal, uint limit = UINT_MAX);

protected:
	template <class T> static int findMaxLength(T pos, T end, int height = lineHeight);
	static int findMaxLength(const vector<vector<string>*>& lists, int height = lineHeight);
	static Texture makeTooltip(const string& text, int limit = tooltipLimit, int height = tooltipHeight);
	static Texture makeTooltip(const vector<string>& lines, int limit = tooltipLimit, int height = tooltipHeight);
};

class ProgMenu : public ProgState {
public:
	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
};

class ProgLobby : public ProgState {
private:
	ScrollArea* rlist;
	vector<pair<string, bool>> rooms;

public:
	ProgLobby(vector<pair<string, bool>>&& rooms);
	virtual ~ProgLobby() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;

	void addRoom(string&& name);
	void delRoom(const string& name);
	void openRoom(const string& name, bool open);
	bool roomsMaxed() const;
	bool roomNameOk(const string& name);
private:
	static Label* createRoom(string name, bool open);
};

inline ProgLobby::ProgLobby(vector<pair<string, bool>>&& rooms) :
	rooms(rooms)
{}

inline void ProgLobby::delRoom(const string& name) {
	rlist->deleteWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin()));
}

inline void ProgLobby::openRoom(const string& name, bool open) {
	static_cast<Label*>(rlist->getWidget(sizet(std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) - rooms.begin())))->color = Widget::colorNormal * (open ? 1.f : 0.5f);
}

inline bool ProgLobby::roomsMaxed() const {
	return rooms.size() >= Com::maxRooms;
}

inline bool ProgLobby::roomNameOk(const string& name) {
	return std::find_if(rooms.begin(), rooms.end(), [name](const pair<string, bool>& rm) -> bool { return rm.first == name; }) == rooms.end();
}

class ProgRoom : public ProgState {
public:
	umap<string, Com::Config> confs;
	ConfigIO wio;
private:
	Label* startButton;

public:
	ProgRoom();
	virtual ~ProgRoom() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;

	void updateStartButton();	// canStart only applies to State::host
	void updateConfigWidgets();
	static Popup* createPopupConfig();
private:
	static vector<Widget*> createConfigList(ConfigIO& wio, bool active);
	static void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id);
	static void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id);
	static string tileFortressString(const Com::Config& cfg);
	static string middleFortressString(const Com::Config& cfg);
	static string pieceTotalString(const Com::Config& cfg);
};

inline string ProgRoom::tileFortressString(const Com::Config& cfg) {
	return toStr(cfg.tileAmounts[uint8(Com::Tile::fortress)]);
}

inline string ProgRoom::middleFortressString(const Com::Config& cfg) {
	return toStr(cfg.homeSize.x - Com::Config::calcSum(cfg.middleAmounts, Com::tileMax - 1) * 2);
}

inline string ProgRoom::pieceTotalString(const Com::Config& cfg) {
	return toStr(cfg.numPieces) + '/' + toStr(cfg.numTiles < Com::Config::maxNumPieces ? cfg.numTiles : Com::Config::maxNumPieces);
}

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

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;
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

	virtual Layout* createLayout() override;
	Popup* createPopupSaveLoad(bool save);

	void setSelected(uint8 sel);
private:
	static Layout* getTicons();
	static Layout* getPicons();
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

class ProgMatch : public ProgState {
public:
	Label* message;
private:
	Label* favorIcon;
	Label* turnIcon;
	Layout* dragonIcon;	// has to be nullptr if dragon can't be placed anymore
	uint16 unplacedDragons;

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	virtual void eventWheel(int ymov) override;
	virtual void eventCameraReset() override;
	void updateFavorIcon(bool on, uint8 cnt, uint8 tot);
	void updateTurnIcon(bool on);
	void setDragonIcon(bool on);
	void decreaseDragonIcon();

	virtual Layout* createLayout() override;
};

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
	
	virtual Layout* createLayout() override;

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

	virtual Layout* createLayout() override;

private:
	static void appendProgram(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	static void appendCPU(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	static void appendRAM(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	static void appendCurrent(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles, const char* (*value)());
	static void appendCurrentDisplay(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles);
	static void appendDisplay(vector<Widget*>& lines, int i, int width, const vector<string>& args);
	static void appendPower(vector<Widget*>& lines, int width, vector<string>& args, vector<string>& titles);
	static void appendAudioDevices(vector<Widget*>& lines, int width, vector<string>& titles, int iscapture);
	static void appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int));
	static void appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles);
	static void appendRenderers(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles);

	static string versionText(const SDL_version& ver);
};

inline string ProgInfo::versionText(const SDL_version& ver) {
	return toStr(ver.major) + '.' + toStr(ver.minor) + '.' + toStr(ver.patch);
}
