#include "recorder.h"
#include "board.h"
#include "types.h"
#include "engine/fileSys.h"
#include "utils/objects.h"

RecAction::RecAction(Type act, uint16 id, svec2 data, bool canBack, bool canNext) :
	aid(id),
	loc(data),
	type(act),
	prev(canBack),
	next(canNext)
{}

// RECORD WRITER

RecordWriter::RecordWriter(const string& cfgName, Board* board) {
	string path = FileSys::recordPath();
	if (!createDirectories(path))
		throw std::runtime_error("failed to create directory " + path);
	if (file = SDL_RWFromFile((path + cfgName + '_' + DateTime::now().toString() + ".txt").c_str(), defaultWriteMode); !file)
		throw std::runtime_error(string("failed to open record file: ") + SDL_GetError());

	string text;
	IniLine::write(text, iniTitleConfig);
	IniLine::write(text, iniKeywordName, cfgName);
	FileSys::writeConfig(text, board->config);
	FileSys::writeAmounts(text, iniKeywordPieceOwn, pieceNames, board->ownPieceAmts);
	FileSys::writeAmounts(text, iniKeywordPieceEne, pieceNames, board->enePieceAmts);
	for (uint16 i = 0; i < board->getTiles().getSize(); ++i)
		IniLine::write(text, iniKeywordTile, toStr(i), tileNames[uint8(board->getTiles()[i].getType())]);
	for (sizet i = 0; i < board->getPieces().getSize(); ++i)
		IniLine::write(text, iniKeywordPiece, toStr(i), toStr(board->ptog(board->getPieces()[i].getPos())));
	IniLine::write(text, iniTitleAction);
}

RecordWriter::~RecordWriter() {
	if (file)
		SDL_RWclose(file);
}

void RecordWriter::piece(uint16 pid, svec2 src, svec2 dst) {
	string line;
	IniLine::write(line, iniKeywordPiece, toStr(pid), toStr(src) + ' ' + toStr(dst));
	SDL_RWwrite(file, line.c_str(), sizeof(*line.c_str()), line.length());
}

void RecordWriter::tile(uint16 tid, TileType src, TileType dst) {
	string line;
	IniLine::write(line, iniKeywordTile, toStr(tid), tileNames[uint8(src)] + string(1, ' ') + tileNames[uint8(dst)]);
	SDL_RWwrite(file, line.c_str(), sizeof(*line.c_str()), line.length());
}

void RecordWriter::breach(uint16 tid, bool src, bool dst) {
	string line;
	IniLine::write(line, iniKeywordBreach, toStr(tid), btos(src) + string(1, ' ') + btos(dst));
	SDL_RWwrite(file, line.c_str(), sizeof(*line.c_str()), line.length());
}

void RecordWriter::top(TileTop top, svec2 src, svec2 dst) {
	string line;
	IniLine::write(line, iniKeywordTop, toStr(uint8(top)), toStr(src) + ' ' + toStr(dst));
	SDL_RWwrite(file, line.c_str(), sizeof(*line.c_str()), line.length());
}

void RecordWriter::finish(Record::Info info) {
	string line;
	IniLine::write(line, iniKeywordFinish, toStr(uint8(info)));
	SDL_RWwrite(file, line.c_str(), sizeof(*line.c_str()), line.length());
	SDL_RWclose(file);
	file = nullptr;
}

// RECORD READER

RecordReader::Action::Action(RecAction::Type act, uint16 id, svec2 from, svec2 to) :
	aid(id),
	src(from),
	dst(to),
	type(act)
{}

RecordReader::RecordReader(const string& name, RecConfig& cfg) {
	void (*dummyRead)(const IniLine&, void*) = [](const IniLine&, void*) {};
	void (*readPrpVal)(const IniLine&, void*) = dummyRead;
	void (*readPrpKeyVal)(const IniLine&, void*) = dummyRead;
	void* data = nullptr;
	for (IniLine il : readTextLines(loadFile(FileSys::recordPath() + name)))
		switch (il.type) {
		case IniLine::title:
			if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniTitleConfig)) {
				readPrpVal = readConfigPrpVal;
				readPrpKeyVal = readConfigPrpKeyVal;
				data = &cfg;
			} else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniTitleAction)) {
				readPrpVal = readActionPrpVal;
				readPrpKeyVal = readActionPrpKeyVal;
				data = &actions;
				if (sizet tcnt = sizet(cfg.homeSize.x) * (sizet(cfg.homeSize.y) * 2 + 1); tcnt > cfg.tiles.size())	// not necessary but might improve performance a little
					cfg.tiles.resize(tcnt);
				if (sizet pcnt = sizet(cfg.countPieces()) * 2; pcnt > cfg.pieces.size())
					cfg.pieces.resize(pcnt);
			} else {
				readPrpVal = dummyRead;
				readPrpKeyVal = dummyRead;
				data = nullptr;
			}
			break;
		case IniLine::prpVal:
			readPrpVal(il, data);
			break;
		case IniLine::prpKeyVal:
			readPrpKeyVal(il, data);
			break;
		}
	if (actions.empty())
		throw std::runtime_error("Record empty");

	cfg.tiles.resize(sizet(cfg.homeSize.x) * (sizet(cfg.homeSize.y) * 2 + 1));
	for (TileType& it : cfg.tiles)
		if (it >= TileType::empty)
			it = TileType::fortress;
	cfg.pieces.resize(sizet(cfg.countPieces()) * 2);
}

void RecordReader::readConfigPrpVal(const IniLine& il, void* data) {
	RecConfig& cfg = *static_cast<RecConfig*>(data);
	if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordName))
		cfg.name = il.val;
	else
		FileSys::readConfigPrpVal(il, cfg);
}

void RecordReader::readConfigPrpKeyVal(const IniLine& il, void* data) {
	RecConfig& cfg = *static_cast<RecConfig*>(data);
	if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordPieceOwn))
		FileSys::readAmount(il, pieceNames, cfg.ownPieceAmts);
	else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordPieceEne))
		FileSys::readAmount(il, pieceNames, cfg.enePieceAmts);
	else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordTile)) {
		ulong id = sstoul(il.key);
		if (id >= cfg.tiles.size())
			cfg.tiles.resize(id + 1);
		cfg.tiles[id] = strToEnum<TileType>(tileNames, il.val);
	} else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordPiece)) {
		ulong id = sstoul(il.key);
		if (id >= cfg.pieces.size())
			cfg.pieces.resize(id + 1);
		cfg.pieces[id] = stoiv<svec2>(il.val.c_str(), strtoul);
	} else
		FileSys::readConfigPrpKeyVal(il, cfg);
}

void RecordReader::readActionPrpVal(const IniLine& il, void* data) {
	vector<Action>& actions = *static_cast<vector<Action>*>(data);
	if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordFinish))
		actions.emplace_back(RecAction::finish, sstoul(il.val));
}

void RecordReader::readActionPrpKeyVal(const IniLine& il, void* data) {
	vector<Action>& actions = *static_cast<vector<Action>*>(data);
	if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordPiece))
		readUvec4(il, actions, RecAction::piece);
	else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordTile)) {
		const char* pos = il.val.c_str();
		uint16 id = readNumber<uint16>(pos, strtoul, 0);
		TileType st = strToEnum<TileType>(tileNames, readWord(pos));
		TileType dt = strToEnum<TileType>(tileNames, readWord(pos));
		if (st < TileType::empty && dt < TileType::empty)
			actions.emplace_back(RecAction::tile, id, svec2(st, st), svec2(dt, dt));
	} else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordBreach)) {
		const char* pos = il.val.c_str();
		uint16 id = readNumber<uint16>(pos, strtoul, 0);
		bool sb = stob(readWord(pos));
		bool db = stob(readWord(pos));
		actions.emplace_back(RecAction::breach, id, svec2(sb), svec2(db));
	} else if (!SDL_strcasecmp(il.prp.c_str(), RecordWriter::iniKeywordTop))
		readUvec4(il, actions, RecAction::top);
}

void RecordReader::readUvec4(const IniLine& il, vector<Action>& actions, RecAction::Type act) {
	const char* pos = il.val.c_str();
	uint16 id = readNumber<uint16>(pos, strtoul, 0);
	uvec4 posv = stoiv<uvec4>(pos, strtoul);
	actions.emplace_back(act, id, svec2(posv.x, posv.y), svec2(posv.z, posv.w));
}

RecAction RecordReader::nextAction() {
	sizet next = std::min(apos + 1, actions.size() - 1);
	RecAction act(actions[apos].type, actions[apos].aid, actions[apos].dst, next, next < actions.size() - 1);
	apos = next;
	return act;
}

RecAction RecordReader::prevAction() {
	sizet prev = apos ? apos - 1 : apos;
	RecAction act(actions[apos].type, actions[apos].aid, actions[apos].src, prev, prev < actions.size() - 1);
	apos = prev;
	return act;
}
