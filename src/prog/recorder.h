#pragma once

#include "types.h"

struct RecAction {
	enum Type : uint8 {
		piece,
		tile,
		breach,
		top,
		finish
	};

	uint16 aid;	// piece/tile/top id or Record::Info
	svec2 loc;	// position/TileType/breach before and after
	Type type;
	bool prev, next;

	RecAction(Type act, uint16 id, svec2 data, bool canBack, bool canNext);
};

struct RecConfig : Config {
	string name;
	vector<TileType> tiles;
	vector<svec2> pieces;
	array<uint16, pieceLim> ownPieceAmts, enePieceAmts;
};

class RecordWriter {
public:
	static constexpr char iniTitleConfig[] = "Config";
	static constexpr char iniTitleAction[] = "Action";
	static constexpr char iniKeywordName[] = "name";
	static constexpr char iniKeywordPieceOwn[] = "piece_own";
	static constexpr char iniKeywordPieceEne[] = "piece_ene";
	static constexpr char iniKeywordPiece[] = "piece";
	static constexpr char iniKeywordTile[] = "tile";
	static constexpr char iniKeywordBreach[] = "breach";
	static constexpr char iniKeywordTop[] = "top";
	static constexpr char iniKeywordFinish[] = "finish";

private:
	SDL_RWops* file = nullptr;

public:
	RecordWriter(string_view cfgName, Board* board);
	~RecordWriter();

	void piece(uint16 pid, svec2 src, svec2 dst);
	void tile(uint16 tid, TileType src, TileType dst);
	void breach(uint16 tid, bool src, bool dst);
	void top(TileTop top, svec2 src, svec2 dst);
	void finish(Record::Info info);
};

class RecordReader {
private:
	struct Action {
		uint16 aid;
		svec2 src, dst;
		RecAction::Type type;

		Action(RecAction::Type act, uint16 id, svec2 from = svec2(0), svec2 to = svec2(0));
	};

	vector<Action> actions;
	sizet apos = 0;

public:
	RecordReader(string_view name, RecConfig& cfg);

	RecAction nextAction();
	RecAction prevAction();

private:
	static void readConfigPrpVal(const IniLine& il, void* data);
	static void readConfigPrpKeyVal(const IniLine& il, void* data);
	static void readActionPrpVal(const IniLine& il, void* data);
	static void readActionPrpKeyVal(const IniLine& il, void* data);
	static void readUvec4(const IniLine& il, vector<Action>& actions, RecAction::Type act);
};
