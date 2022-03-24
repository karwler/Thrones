#pragma once

#include "types.h"
#include "utils/settings.h"
#include "utils/utils.h"

class GuiGen {
public:
	enum class SizeRef : uint {
		scrollBarWidth,	// common initialized values
		caretWidth,
		popupMargin,
		smallHeight,
		lineHeight,
		superHeight,
		iconSize,
		lineSpacing,
		superSpacing,
		tooltipHeight,
		tooltipLimit,
		chatEmbed,
		menuSideWidth,	// late assignments start here
		menuMenuWidth,
		menuTitleWidth,
		lobbySideWidth,
		roomPortWidth,
		configDescWidth,
		configAmtWidth,
		setupSideWidth,
		matchSideWidth,
		matchPointsLen,
		settingsSideWidth,
		settingsDescWidth,
		settingsAddWidth,
		settingsSlleWidth,
		infoSideWidth,
		infoArgWidth,
		infoDispWidth,
		infoArgDispWidth,
		infoProgRightLen
	};

	struct ConfigIO {
		CheckBox* victoryPoints;
		LabelEdit* victoryPointsNum;
		CheckBox* vpEquidistant;
		CheckBox* ports;
		CheckBox* rowBalancing;
		CheckBox* homefront;
		CheckBox* setPieceBattle;
		LabelEdit* setPieceBattleNum;
		LabelEdit* width;
		LabelEdit* height;
		Slider* battleSL;
		LabelEdit* battleLE;
		CheckBox* favorTotal;
		LabelEdit* favorLimit;
		CheckBox* firstTurnEngage;
		CheckBox* terrainRules;
		CheckBox* dragonLate;
		CheckBox* dragonStraight;
		array<LabelEdit*, tileLim> tiles;
		array<LabelEdit*, tileLim> middles;
		array<LabelEdit*, pieceLim> pieces;
		Label* tileFortress;
		Label* middleFortress;
		Label* pieceTotal;
		LabelEdit* winThrone;
		LabelEdit* winFortress;
		array<Icon*, pieceLim> capturers;
	};

	struct RoomIO {
		Label* start;
		Label* host;
		Label* kick;
		Label* del;
	};

	struct SetupIO {
		Label* load;
		Label* back;
		Label* next;
		Layout* icons;
	};

	struct MatchIO {
		array<Icon*, favorMax> favors;
		Label* turn;
		Label* vpOwn;
		Label* vpEne;
		Icon* destroy;
		Icon* establish;
		Icon* rebuild;
		Icon* spawn;
		Icon* dragon;	// has to be nullptr if dragon can't be placed anymore
	};

	static constexpr float defaultDim = 0.5f;
	static constexpr float gammaStepFactor = 10.f;
	static constexpr char rv2iSeparator[] = " x ";
	static constexpr char msgFavorPick[] = "Pick a fate's favor";
	static constexpr char msgPickPiece[] = "Pick piece (";
	static constexpr char chatPrefix[] = ": ";
private:
	static constexpr uint16 chatCharLimit = 16384 - Settings::playerNameLimit - 3;	// subtract prefix length

	static constexpr float scrollBarWidth = 1.f / 72.f;	// 10p (values are in relation to 720p height)
	static constexpr float caretWidth = 1.f / 180.f;	// 4p
	static constexpr float popupMargin = 1.f / 144.f;	// 5p
	static constexpr float smallHeight = 1.f / 36.f;	// 20p
	static constexpr float lineHeight = 1.f / 24.f;		// 30p
	static constexpr float superHeight = 1.f / 18.f;	// 40p
	static constexpr float iconSize = 1.f / 11.f;		// 64p
	static constexpr float lineSpacing = 1.f / 144.f;	// 5p
	static constexpr float superSpacing = 1.f / 72.f;	// 10p
	static constexpr float tooltipHeight = 1.f / 45.f;	// 16p
	static constexpr float tooltipLimit = 2.f / 3.f;
	static constexpr float chatEmbed = 0.5f;

	array<pair<int, std::function<int ()>>, sizet(SizeRef::infoProgRightLen)+1> sizes;

public:
	void initSizes();
	void calsSizes();
	const int* getSize(SizeRef id) const;

	void openPopupMessage(string msg, BCall ccal, string&& ctxt = "Okay") const;
	void openPopupChoice(string&& msg, BCall kcal, BCall ccal) const;
	void openPopupInput(string&& msg, string text, BCall kcal, uint16 limit = UINT16_MAX) const;
	void openPopupFavorPick(uint16 availableFF) const;
	void openPopupConfig(const string& configName, const Config& cfg, ScrollArea*& configList, bool match);
	void openPopupSaveLoad(const umap<string, Setup>& setups, bool save) const;
	void openPopupPiecePicker(uint16 piecePicksLeft) const;
	void openPopupSpawner() const;
	void openPopupSettings(ScrollArea*& content, sizet& bindingsStart);
	void openPopupKeyGetter(Binding::Type bind) const;

	void makeTitleBar() const;
	vector<Widget*> createChat(TextBox*& chatBox, bool overlay) const;
	Overlay* createNotification(Overlay*& notification) const;
	Overlay* createFpsCounter(Label*& fpsText) const;
	string makeFpsText(float dSec) const;
	Label* createRoom(string&& name, bool open) const;
	Overlay* createGameMessage(Label*& message, bool setup) const;
	Overlay* createGameChat(TextBox*& chatBox) const;
	vector<Widget*> createBottomIcons(bool tiles) const;
	Size keyGetLineSize(Binding::Type bind) const;
	KeyGetter* createKeyGetter(Binding::Accept accept, Binding::Type bind, sizet kid, Label* lbl) const;

	uptr<RootLayout> makeMainMenu(Interactable*& selected, LabelEdit*& pname, Label*& versionNotif);
	uptr<RootLayout> makeLobby(Interactable*& selected, TextBox*& chatBox, ScrollArea*& rooms, vector<pair<string, bool>>& roomBuff);
	uptr<RootLayout> makeRoom(Interactable*& selected, ConfigIO& wio, RoomIO& rio, TextBox*& chatBox, ComboBox*& configName, const umap<string, Config>& confs, const string& startConfig);
	uptr<RootLayout> makeSetup(Interactable*& selected, SetupIO& sio, Icon*& bswapIcon, Navigator*& planeSwitch);
	uptr<RootLayout> makeMatch(Interactable*& selected, MatchIO& mio, Icon*& bswapIcon, Navigator*& planeSwitch, uint16& unplacedDragons);
	uptr<RootLayout> makeSettings(Interactable*& selected, ScrollArea*& content, sizet& bindingsStart);
	uptr<RootLayout> makeInfo(Interactable*& selected, ScrollArea*& content);

	static string tileFortressString(const Config& cfg);
	static string middleFortressString(const Config& cfg);
	static string pieceTotalString(const Config& cfg);
	static SDL_DisplayMode fstrToDisp(const string& str);
private:
	void assignSizeFunc(SizeRef id, std::function<int ()>&& func);
	template <class T> static int txtMaxLen(T pos, T end, float hfac);
	static int txtMaxLen(const initlist<initlist<const char*>>& lists, float hfac);
	static string dispToFstr(const SDL_DisplayMode& mode);
	template <class T> Layout* createKeyGetterList(Binding::Type bind, const vector<T>& refs, Binding::Accept type, Label* lbl) const;
	template <bool upper = true, class T, sizet S> static string enameToFstr(T val, const array<const char*, S>& names);
	static string versionText(const SDL_version& ver);
	static string ibtos(int val);
	vector<Widget*> createConfigList(ConfigIO& wio, const Config& cfg, bool active, bool match);
	void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id) const;
	void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const;
	ScrollArea* createSettingsList(sizet& bindingsStart);

	void appendProgram(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles);
	void appendSystem(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendCurrentDisplay(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendDisplay(vector<Widget*>& lines, int i, initlist<const char*>::iterator args) const;
	void appendPower(vector<Widget*>& lines, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendAudioDevices(vector<Widget*>& lines, initlist<const char*>::iterator& titles, int iscapture) const;
	void appendDrivers(vector<Widget*>& lines, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) const;
	void appendDisplays(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendRenderers(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendControllers(vector<Widget*>& lines, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
};

inline const int* GuiGen::getSize(SizeRef id) const {
	return &sizes[uint(id)].first;
}

inline void GuiGen::assignSizeFunc(SizeRef id, std::function<int ()>&& func) {
	sizes[uint(id)] = pair(func(), std::move(func));
}

inline string GuiGen::tileFortressString(const Config& cfg) {
	return toStr(cfg.countFreeTiles());
}

inline string GuiGen::middleFortressString(const Config& cfg) {
	return toStr(cfg.homeSize.x - cfg.countMiddles() * 2);
}

inline string GuiGen::pieceTotalString(const Config& cfg) {
	return toStr(cfg.countPieces()) + '/' + toStr(cfg.homeSize.x * cfg.homeSize.y);
}

inline string GuiGen::dispToFstr(const SDL_DisplayMode& mode) {
	return toStr(mode.w) + rv2iSeparator + toStr(mode.h) + " | " + toStr(mode.refresh_rate) + "Hz " + pixelformatName(mode.format);
}

inline string GuiGen::versionText(const SDL_version& ver) {
	return toStr(ver.major) + '.' + toStr(ver.minor) + '.' + toStr(ver.patch);
}
