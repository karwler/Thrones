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

	struct Text {
		string text;
		int length, height;

		Text(const string& str, int height = lineHeight, int margin = Label::defaultTextMargin);

		static int strLength(const string& str, int height = lineHeight, int margin = Label::defaultTextMargin);
	};
	template <class T> static int findMaxLength(T pos, T end, int height = lineHeight, int margin = Label::defaultTextMargin);
	static int findMaxLength(const vector<vector<string>*>& lists, int height = lineHeight, int margin = Label::defaultTextMargin);

public:
	virtual ~ProgState() = default;	// to keep the compiler happy

	void eventEnter();
	virtual void eventEscape() {}
	virtual void eventWheel(int) {}
	virtual void eventResized() {}

	virtual Layout* createLayout();
	static Popup* createPopupMessage(const string& msg, BCall ccal, const string& ctxt = "Ok");
	static Popup* createPopupChoice(const string& msg, BCall kcal, BCall ccal);

protected:
	bool tryClosePopup();
};

template<class T>
int ProgState::findMaxLength(T pos, T end, int height, int margin) {
	int width = 0;
	for (; pos != end; pos++)
		if (int len = Text::strLength(*pos, height, margin); len > width)
			width = len;
	return width;
}

class ProgMenu : public ProgState {
public:
	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
};

class ProgSetup : public ProgState {
public:
	enum class Stage : uint8 {
		tiles,
		middles,
		pieces,
		ready
	};

	bool enemyReady;
	array<Tile::Type, Com::boardLength> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	Label* message;
private:
	Layout* icons;
	vector<uint8> counters;
	uint8 selected;
	Stage stage;

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;
	virtual void eventWheel(int ymov) override;

	Stage getStage() const;
	bool setStage(Stage stg);	// returns true if match is ready to load
	Draglet* getIcon(uint8 type) const;
	void incdecIcon(uint8 type, bool inc, bool isTile);
	uint8 getCount(uint8 type) const;
	uint8 getSelected() const;

	virtual Layout* createLayout() override;
private:
	static Layout* getTicons();
	static Layout* getPicons();
	Layout* createSidebar(int& sideLength) const;
	void setSelected(uint8 sel);
	void switchIcon(uint8 type, bool on, bool isTile);
};

inline ProgSetup::Stage ProgSetup::getStage() const {
	return stage;
}

inline Draglet* ProgSetup::getIcon(uint8 type) const {
	return static_cast<Draglet*>(icons->getWidget(type + 1));
}

inline uint8 ProgSetup::getCount(uint8 type) const {
	return counters[type];
}

inline uint8 ProgSetup::getSelected() const {
	return selected;
}

class ProgMatch : public ProgState {
public:
	Layout* dragonIcon;	// has to be nullptr if dragon can't be placed anymore
	Label* message;

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;
	void setDragonIconOn(bool on);

	virtual Layout* createLayout() override;
};

class ProgSettings : public ProgState {
private:
	LabelEdit* resolution;

public:
	virtual ~ProgSettings() override = default;

	virtual void eventEscape() override;
	virtual void eventResized() override;
	
	virtual Layout* createLayout() override;
};

class ProgInfo : public ProgState {
private:
	static const array<string, SDL_POWERSTATE_CHARGED+1> powerNames;
	static const string infinity;

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
