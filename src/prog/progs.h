#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// for handling program state specific things that occur in all states
class ProgState {
protected:
	static constexpr int lineHeight = 30;
	static constexpr int superHeight = 40;
	static constexpr int superSpacing = 10;
	static constexpr int iconSize = 30;

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
	} stage;
	bool amFirst, enemyReady;
	array<Tile::Type, 9> rcvMidBuffer;	// buffer for received opponent's middle tile placement (positions in own left to right)
	array<uint8, sizet(Tile::Type::fortress)> tileCnt, midCnt;
	array<uint8, sizet(Piece::Type::empty)> pieceCnt;
	Layout* ticons;
	Layout* micons;
	Layout* picons;
	Label* message;

public:
	ProgSetup();
	virtual ~ProgSetup() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
	void setTicons();
	void setMicons();
	void setPicons();

private:
	Layout* createSidebar(int& sideLength) const;
};

class ProgMatch : public ProgState {
public:
	Layout* dragonIcon;

public:
	virtual ~ProgMatch() override = default;

	virtual void eventEscape() override;

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
