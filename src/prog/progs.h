#pragma once

#include "utils/layouts.h"

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
	static Popup* createPopupMessage(const string& msg, PCall ccal, const string& ctxt = "Ok");
	static Popup* createPopupChoice(const string& msg, PCall kcal, PCall ccal);

protected:
	bool tryClosePopup();
};

class ProgMenu : public ProgState {
public:
	virtual ~ProgMenu() override = default;

	virtual void eventEscape() override;

	virtual Layout* createLayout() override;
};

class ProgGame : public ProgState {
public:
	virtual ~ProgGame() override = default;

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
