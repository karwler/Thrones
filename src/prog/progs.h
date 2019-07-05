#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// for handling program state specific things that occur in all states
class ProgState {
protected:
	static constexpr int lineHeight = 30;
	static constexpr int superHeight = 40;
	static constexpr int superSpacing = 10;
	static constexpr int iconSize = 64;
	static constexpr char arrowLeft[] = "<";
	static constexpr char arrowRight[] = ">";

	struct Text {
		string text;
		int length, height;

		Text(string str, int height = lineHeight, int margin = Label::defaultTextMargin);

		static int strLen(const string& str, int height = lineHeight, int margin = Label::defaultTextMargin);
	};
	template <class T> static int findMaxLength(T pos, T end, int height = lineHeight, int margin = Label::defaultTextMargin);
	static int findMaxLength(const vector<vector<string>*>& lists, int height = lineHeight, int margin = Label::defaultTextMargin);

public:
	virtual ~ProgState() = default;	// to keep the compiler happy

	void eventEnter();
	virtual void eventEscape() {}
	virtual void eventWheel(int) {}
	virtual void eventDrag(uint32) {}
	virtual void eventUndrag() {}

	virtual Layout* createLayout();
	static Popup* createPopupMessage(string msg, BCall ccal, string ctxt = "Ok");
	static Popup* createPopupChoice(string msg, BCall kcal, BCall ccal);
	static pair<Popup*, Widget*> createPopupInput(string msg, BCall kcal);

protected:
	bool tryClosePopup();
};

template<class T>
int ProgState::findMaxLength(T pos, T end, int height, int margin) {
	int width = 0;
	for (; pos != end; pos++)
		if (int len = Text::strLen(*pos, height, margin); len > width)
			width = len;
	return width;
}

class ProgMenu : public ProgState {
public:
	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
};

class ProgHost : public ProgState {
public:
	vector<Com::Config> confs;
	sizet curConf;

	LabelEdit* inWidth;
	LabelEdit* inHeight;
	Slider* inSurvivalSL;
	LabelEdit* inSurvivalLE;
	LabelEdit* inFavors;
	LabelEdit* inDragonDist;
	CheckBox* inDragonDiag;
	CheckBox* inMultistage;
	array<LabelEdit*, Com::tileMax> inTiles;
	array<LabelEdit*, Com::tileMax-1> inMiddles;
	array<LabelEdit*, Com::pieceMax> inPieces;
	LabelEdit* inWinFortress;
	LabelEdit* inWinThrone;
	LabelEdit* inCapturers;
	CheckBox* inShiftLeft;
	CheckBox* inShiftNear;
	Label* outTileFortress;
	Label* outMiddleFortress;
	Label* outPieceTotal;
	
public:
	ProgHost();
	virtual ~ProgHost() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;

	static string tileFortressString(const Com::Config& cfg);
	static string middleFortressString(const Com::Config& cfg);
	static string pieceTotalString(const Com::Config& cfg);
private:
	static void setLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id);
	static void setTitle(vector<Widget*>& menu, string&& title, sizet& id);
};

inline string ProgHost::tileFortressString(const Com::Config& cfg) {
	return to_string(cfg.tileAmounts[uint8(Com::Tile::fortress)]);
}

inline string ProgHost::middleFortressString(const Com::Config& cfg) {
	return to_string(cfg.homeWidth - Com::Config::calcSum(cfg.middleAmounts, Com::tileMax - 1) * 2);
}

inline string ProgHost::pieceTotalString(const Com::Config& cfg) {
	return to_string(cfg.numPieces) + '/' + to_string(cfg.numTiles < Com::Config::maxNumPieces ? cfg.numTiles : Com::Config::maxNumPieces);
}

class ProgSetup : public ProgState {
public:
	enum class Stage : uint8 {
		tiles,
		middles,
		pieces,
		ready
	};

	vector<Com::Tile> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	Label* message;
	bool enemyReady;
private:
	uint8 selected;
	Stage stage;
	vector<uint16> counters;
	Layout* icons;
	vec2s lastHold;		// position of last object that the cursor was dragged over
	uint8 lastButton;	// last button that was used on lastHold (0 for none)

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;
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
private:
	static Layout* getTicons();
	static Layout* getPicons();
	Layout* createSidebar(int& sideLength) const;
	void setSelected(uint8 sel);
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

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	void updateFavorIcon(bool on, uint8 cnt, uint8 tot);
	void updateTurnIcon(bool on);
	void setDragonIcon(bool on);
	void deleteDragonIcon();

	virtual Layout* createLayout() override;
};

class ProgSettings : public ProgState {
public:
	SwitchBox* screen;
	SwitchBox* winSize;
	SwitchBox* dspMode;
	SwitchBox* msample;

	static constexpr float gammaStepFactor = 10.f;
	static constexpr float brightStepFactor = 10.f;
private:
	static constexpr char rv2iSeparator[] = " x ";
	umap<string, uint32> pixelformats;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	
	virtual Layout* createLayout() override;

	SDL_DisplayMode currentMode() const;
private:
	static string sizeToFstr(vec2i size);
	static string dispToFstr(const SDL_DisplayMode& mode);
};

inline string ProgSettings::sizeToFstr(vec2i size) {
	return size.toString(rv2iSeparator);
}

inline string ProgSettings::dispToFstr(const SDL_DisplayMode& mode) {
	return to_string(mode.w) + rv2iSeparator + to_string(mode.h) + " | " + to_string(mode.refresh_rate) + "Hz " + pixelformatName(mode.format);
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
	static void appendDevices(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(int), const char* (*value)(int, int), int arg);
	static void appendDrivers(vector<Widget*>& lines, int width, vector<string>& titles, int (*limit)(), const char* (*value)(int));
	static void appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, const vector<string>& args, vector<string>& titles);
	static void appendRenderers(vector<Widget*>& lines, int width, const vector<string>& args, vector<string>& titles);
};
