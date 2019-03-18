#pragma once

#include "utils/layouts.h"
#include "utils/objects.h"

// for handling program state specific things that occur in all states
class ProgState {
protected:
	struct Text {
		string text;
		int length, height;

		Text(const string& str, int height, int margin = Label::defaultTextMargin);
	};
	static int findMaxLength(const string* strs, sizet scnt, int height, int margin = Label::defaultTextMargin);

	static constexpr int popupLineHeight = 40;
	static constexpr int lineHeight = 30;
	static constexpr int superHeight = 40;
	static constexpr int superSpacing = 10;
	static constexpr int iconSize = 30;

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
