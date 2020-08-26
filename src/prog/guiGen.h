#pragma once

#include "types.h"
#include "utils/layouts.h"

class GuiGen {
public:
	struct Text {
		string text;
		int length, height;

		Text(string str, int sh);

		static int strLen(const char* str, int height);
		static int strLen(const string& str, int height);
		template <class T> static int maxLen(T pos, T end, int height);
		static int maxLen(const initlist<initlist<const char*>>& lists, int height);
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
		array<LabelEdit*, Tile::lim> tiles;
		array<LabelEdit*, Tile::lim> middles;
		array<LabelEdit*, Piece::lim> pieces;
		Label* tileFortress;
		Label* middleFortress;
		Label* pieceTotal;
		LabelEdit* winThrone;
		LabelEdit* winFortress;
		array<Icon*, Piece::lim> capturers;
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
	static constexpr float chatEmbedSize = 0.5f;
private:
	static constexpr uint16 chatLineLimit = 2000;

	int smallHeight, lineHeight, superHeight, tooltipHeight;
	int lineSpacing, superSpacing, iconSize;
	uint tooltipLimit;

public:
	void resize();
	int getLineHeight() const;
	Texture makeTooltip(const char* text) const;
	Texture makeTooltipL(const char* text) const;

	void openPopupMessage(string msg, BCall ccal, string ctxt = "Okay") const;
	void openPopupChoice(string msg, BCall kcal, BCall ccal) const;
	void openPopupInput(string msg, string text, BCall kcal, uint16 limit = UINT16_MAX) const;
	void openPopupFavorPick(uint16 availableFF) const;
	void openPopupConfig(const string& configName, const Config& cfg, ScrollArea*& configList, bool match) const;
	void openPopupSaveLoad(const umap<string, Setup>& setups, bool save) const;
	void openPopupPiecePicker(uint16 piecePicksLeft) const;
	void openPopupSpawner() const;
	void openPopupKeyGetter(Binding::Type bind) const;

	vector<Widget*> createChat(TextBox*& chatBox, bool overlay) const;
	Overlay* createNotification(Overlay*& notification) const;
	Overlay* createFpsCounter(Label*& fpsText) const;
	Text makeFpsText(float dSec) const;
	Label* createRoom(string&& name, bool open) const;
	Overlay* createGameMessage(Label*& message, bool setup) const;
	Overlay* createGameChat(TextBox*& chatBox) const;
	vector<Widget*> createBottomIcons(bool tiles) const;
	int keyGetLineSize(Binding::Type bind) const;
	KeyGetter* createKeyGetter(KeyGetter::Accept accept, Binding::Type bind, sizet kid, Label* lbl) const;

	uptr<RootLayout> makeMainMenu(Interactable*& selected, Label*& versionNotif) const;
	uptr<RootLayout> makeLobby(Interactable*& selected, TextBox*& chatBox, ScrollArea*& rooms, vector<pair<string, bool>>& roomBuff) const;
	uptr<RootLayout> makeRoom(Interactable*& selected, ConfigIO& wio, RoomIO& rio, TextBox*& chatBox, ComboBox*& configName, const umap<string, Config>& confs, const string& startConfig) const;
	uptr<RootLayout> makeSetup(Interactable*& selected, SetupIO& sio, Icon*& bswapIcon, Navigator*& planeSwitch) const;
	uptr<RootLayout> makeMatch(Interactable*& selected, MatchIO& mio, Icon*& bswapIcon, Navigator*& planeSwitch, uint16& unplacedDragons) const;
	uptr<RootLayout> makeSettings(Interactable*& selected, ScrollArea*& content, sizet& bindingsStart, umap<string, uint32>& pixelformats) const;
	uptr<RootLayout> makeInfo(Interactable*& selected, ScrollArea*& content) const;

	static string tileFortressString(const Config& cfg);
	static string middleFortressString(const Config& cfg);
	static string pieceTotalString(const Config& cfg);
	SDL_DisplayMode fstrToDisp(const umap<string, uint32>& pixelformats, const string& str) const;
private:
	static string dispToFstr(const SDL_DisplayMode& mode);
	template <class T> Layout* createKeyGetterList(Binding::Type bind, const vector<T>& refs, KeyGetter::Accept type, Label* lbl) const;
	static string bindingToFstr(Binding::Type bind);
	static const char* pixelformatName(uint32 format);
	static string versionText(const SDL_version& ver);
	static string ibtos(int val);
	vector<Widget*> createConfigList(ConfigIO& wio, const Config& cfg, bool active, bool match) const;
	void setConfigLines(vector<Widget*>& menu, vector<vector<Widget*> >& lines, sizet& id) const;
	void setConfigTitle(vector<Widget*>& menu, string&& title, sizet& id) const;

	void appendProgram(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendSystem(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendCurrentDisplay(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendDisplay(vector<Widget*>& lines, int i, int width, initlist<const char*>::iterator args) const;
	void appendPower(vector<Widget*>& lines, int width, initlist<const char*>::iterator& args, initlist<const char*>::iterator& titles) const;
	void appendAudioDevices(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int iscapture) const;
	void appendDrivers(vector<Widget*>& lines, int width, initlist<const char*>::iterator& titles, int (*limit)(), const char* (*value)(int)) const;
	void appendDisplays(vector<Widget*>& lines, int argWidth, int dispWidth, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendRenderers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
	void appendControllers(vector<Widget*>& lines, int width, initlist<const char*>::iterator args, initlist<const char*>::iterator& titles) const;
};

inline int GuiGen::Text::strLen(const string& str, int height) {
	return strLen(str.c_str(), height);
}

inline int GuiGen::getLineHeight() const {
	return lineHeight;
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

inline const char* GuiGen::pixelformatName(uint32 format) {
	return SDL_GetPixelFormatName(format) + 16;	// skip "SDL_PIXELFORMAT_"
}

inline string GuiGen::versionText(const SDL_version& ver) {
	return toStr(ver.major) + '.' + toStr(ver.minor) + '.' + toStr(ver.patch);
}
