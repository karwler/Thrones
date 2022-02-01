#pragma once

#include "types.h"
#include "utils/settings.h"
#include "utils/utils.h"

class GuiGen {
public:
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
	static constexpr Size chatEmbedSize = { 0.5f, Size::rela };
	static constexpr char chatPrefix[] = ": ";
private:
	static constexpr uint16 chatCharLimit = 16384 - Settings::playerNameLimit - 3;	// subtract prefix length

	static constexpr Size smallHeight = { 1.f / 36.f, Size::abso };	// 20p	(values are in relation to 720p height)
	static constexpr Size lineHeight = { 1.f / 24.f, Size::abso };	// 30p
	static constexpr Size superHeight = { 1.f / 18.f, Size::abso };	// 40p
	static constexpr Size iconSize = { 1.f / 11.f, Size::abso };	// 64p
	static constexpr float lineSpacing = 1.f / 144.f;	// 5p
	static constexpr float superSpacing = 1.f / 72.f;	// 10p

public:
	int getLineHeight() const;

	void openPopupMessage(string msg, BCall ccal, string&& ctxt = "Okay") const;
	void openPopupChoice(string&& msg, BCall kcal, BCall ccal) const;
	void openPopupInput(string&& msg, string text, BCall kcal, uint16 limit = UINT16_MAX) const;
	void openPopupFavorPick(uint16 availableFF) const;
	void openPopupConfig(const string& configName, const Config& cfg, ScrollArea*& configList, bool match) const;
	void openPopupSaveLoad(const umap<string, Setup>& setups, bool save) const;
	void openPopupPiecePicker(uint16 piecePicksLeft) const;
	void openPopupSpawner() const;
	void openPopupSettings(ScrollArea*& content, sizet& bindingsStart) const;
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

	uptr<RootLayout> makeMainMenu(Interactable*& selected, LabelEdit*& pname, Label*& versionNotif) const;
	uptr<RootLayout> makeLobby(Interactable*& selected, TextBox*& chatBox, ScrollArea*& rooms, vector<pair<string, bool>>& roomBuff) const;
	uptr<RootLayout> makeRoom(Interactable*& selected, ConfigIO& wio, RoomIO& rio, TextBox*& chatBox, ComboBox*& configName, const umap<string, Config>& confs, const string& startConfig) const;
	uptr<RootLayout> makeSetup(Interactable*& selected, SetupIO& sio, Icon*& bswapIcon, Navigator*& planeSwitch) const;
	uptr<RootLayout> makeMatch(Interactable*& selected, MatchIO& mio, Icon*& bswapIcon, Navigator*& planeSwitch, uint16& unplacedDragons) const;
	uptr<RootLayout> makeSettings(Interactable*& selected, ScrollArea*& content, sizet& bindingsStart) const;
	uptr<RootLayout> makeInfo(Interactable*& selected, ScrollArea*& content) const;

	static string tileFortressString(const Config& cfg);
	static string middleFortressString(const Config& cfg);
	static string pieceTotalString(const Config& cfg);
	static SDL_DisplayMode fstrToDisp(const string& str);
private:
	template <class T> static int txtMaxLen(T pos, T end, float hfac);
	static int txtMaxLen(const initlist<initlist<const char*>>& lists, float hfac);
	static string dispToFstr(const SDL_DisplayMode& mode);
	template <class T> Layout* createKeyGetterList(Binding::Type bind, const vector<T>& refs, Binding::Accept type, Label* lbl) const;
	template <bool upper = true, class T, sizet S> static string enameToFstr(T val, const array<const char*, S>& names);
	static string versionText(const SDL_version& ver);
	static string ibtos(int val);
	vector<Widget*> createConfigList(ConfigIO& wio, const Config& cfg, bool active, bool match) const;
	void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id) const;
	void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const;
	ScrollArea* createSettingsList(sizet& bindingsStart) const;

	void appendProgram(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendSystem(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendCurrentDisplay(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendDisplay(vector<Widget*>& lines, int i, const Size& width, initlist<const char*>::iterator args) const;
	void appendPower(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendAudioDevices(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator& titles, int iscapture) const;
	void appendDrivers(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) const;
	void appendDisplays(vector<Widget*>& lines, const Size& argWidth, const Size& dispWidth, const Size& argDispWidth, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendRenderers(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendControllers(vector<Widget*>& lines, const Size& width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
};

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
