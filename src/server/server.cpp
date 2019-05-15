#include "server.h"
#include <ctime>
using namespace Com;

Config::Config(const string& name) :
	name(name),
	homeWidth(9),
	homeHeight(4),
	tileAmounts({ 11, 10, 7, 7, 1 }),
	pieceAmounts({ 2, 2, 2, 1, 1, 2, 1, 2, 1, 1 }),
	winFortress(1),
	winThrone(1),
	capturers({ false, false, false, false, false, false, false, false, false, true }),
	shiftLeft(true),
	shiftNear(true)
{
	updateValues();
}

void Config::updateValues() {
	numTiles = 0;
	for (uint16 it : tileAmounts)
		numTiles += it;

	numPieces = 0;
	for (uint16 it : pieceAmounts)
		numPieces += it;

	extraSize = homeHeight + 1 * homeWidth;
	boardSize = homeHeight * 2 + 1 * homeWidth;	// should equal countTiles * 2 + homeWidth
	piecesSize = numPieces * 2;
}

Config& Config::checkValues() {
	homeWidth = std::clamp(makeOdd(homeWidth), minWidth, maxWidth);
	homeHeight = std::clamp(homeHeight, minHeight, maxHeight);
	updateValues();

	uint16 limit = homeHeight * homeWidth;
	matchAmounts(numTiles, tileAmounts, limit, 0, uint8(tileAmounts.size()), 1, [](uint16 a, uint16 b) -> bool { return a < b; });
	matchAmounts(numTiles, tileAmounts, limit, uint8(tileAmounts.size() - 1), uint8(tileAmounts.size()), -1, [](uint16 a, uint16 b) -> bool { return a > b; });

	limit = (recvSize - 1) / 2;	// max transferable amount of pieces
	matchAmounts(numPieces, pieceAmounts, numTiles < limit ? numTiles : limit, uint8(tileAmounts.size() - 1), uint8(tileAmounts.size()), -1, [](uint16 a, uint16 b) -> bool { return a > b; });

	winFortress = std::clamp(winFortress, uint16(0), tileAmounts[uint8(Tile::fortress)]);
	winThrone = std::clamp(winThrone, uint16(0), pieceAmounts[uint8(Piece::throne)]);

	if (std::find(capturers.begin(), capturers.end(), true) == capturers.end())
		std::fill(capturers.begin(), capturers.end(), true);
	return *this;
}

vector<uint8> Config::toComData() const {
	vector<uint8> data(dataSize(Code::setup));
	data[0] = uint8(Code::setup);	// leave second byte free for first turn indicator

	uint16* sp = reinterpret_cast<uint16*>(data.data() + 2);
	*sp++ = homeWidth;
	*sp++ = homeHeight;
	sp = std::copy(tileAmounts.begin(), tileAmounts.end(), sp);
	sp = std::copy(pieceAmounts.begin(), pieceAmounts.end(), sp);

	*sp++ = winFortress;
	*sp++ = winThrone;
	uint8* bp = std::copy(capturers.begin(), capturers.end(), reinterpret_cast<uint8*>(sp));

	*bp++ = shiftLeft;
	*bp++ = shiftNear;
	return data;
}

void Config::fromComData(const uint8* data) {
	const uint16* sp = reinterpret_cast<const uint16*>(data + 2);	// skip com code and turn indicator
	homeWidth = *sp++;
	homeHeight = *sp++;
	sp = std::copy_n(sp, tileAmounts.size(), tileAmounts.begin());
	sp = std::copy_n(sp, pieceAmounts.size(), pieceAmounts.begin());

	winFortress = *sp++;
	winThrone = *sp++;

	const uint8* bp = reinterpret_cast<const uint8*>(sp);
	std::copy(bp, bp + pieceMax, capturers.begin());
	bp += pieceMax;

	shiftLeft = *bp++;
	shiftNear = *bp++;
	updateValues();
}

uint16 Config::dataSize(Code code) const {
	switch (code) {
	case Code::none:
		return sizeof(Code);
	case Code::full:
		return sizeof(Code);
	case Code::setup:
		return uint16(sizeof(homeWidth) + sizeof(homeHeight) + tileAmounts.size() * sizeof(tileAmounts[0]) + pieceAmounts.size() * sizeof(pieceAmounts[0]) + sizeof(winFortress) + sizeof(winThrone) + capturers.size() * sizeof(capturers[0]) + sizeof(shiftLeft) + sizeof(shiftNear));
	case Code::tiles:
		return tileCompressionEnd();
	case Code::pieces:
		return sizeof(Code) + numPieces * sizeof(uint16);
	case Code::move:
		return sizeof(Code) + sizeof(uint16) * 2;
	case Code::kill:
		return sizeof(Code) + sizeof(bool) + sizeof(uint16);
	case Code::breach:
		return sizeof(Code) + sizeof(bool) + sizeof(uint16);
	case Code::favor:
		return sizeof(Code) + sizeof(uint8) + sizeof(uint16);
	case Code::record:
		return sizeof(Code) + sizeof(uint16) + sizeof(bool) * 2;
	case Code::win:
		return sizeof(Code) + sizeof(bool);
	}
	return sizeof(Code);
}

string Config::toIniText() const {
	string text = '[' + name + ']' + linend;
	text += makeIniLine(keywordSize, to_string(homeWidth) + ' ' + to_string(homeHeight));
	writeAmount(text, keywordTile, tileNames, tileAmounts);
	writeAmount(text, keywordPiece, pieceNames, pieceAmounts);
	text += makeIniLine(keywordWinFortress, to_string(winFortress));
	text += makeIniLine(keywordWinThrone, to_string(winThrone));
	text += makeIniLine(keywordCapturers, capturersString());
	text += makeIniLine(keywordShift, string(shiftLeft ? keywordLeft : keywordRight) + ' ' + (shiftNear ? keywordNear : keywordFar));
	return text;
}

void Config::fromIniLine(const string& line) {
	if (pairStr it = readIniLine(line); strcicmp(it.first, keywordSize))
		readSize(it.second);
	else if (strncicmp(it.first, keywordTile, strlen(keywordTile)))
		readAmount(it, keywordTile, tileNames, tileAmounts);
	else if (strncicmp(it.first, keywordPiece, strlen(keywordPiece)))
		readAmount(it, keywordPiece, pieceNames, pieceAmounts);
	else if (strcicmp(it.first, keywordWinFortress))
		winFortress = uint16(sstol(it.second));
	else if (strcicmp(it.first, keywordWinThrone))
		winThrone = uint16(sstol(it.second));
	else if (strcicmp(it.first, keywordCapturers))
		readCapturers(it.second);
	else if (strcicmp(it.first, keywordShift))
		readShift(it.second);
}

void Config::readSize(const string& line) {
	const char* pos = line.c_str();
	homeWidth = readNumber<uint16>(pos, strtol, 0);
	homeHeight = readNumber<uint16>(pos, strtol, 0);
}

void Config::readCapturers(const string& line) {
	std::fill(capturers.begin(), capturers.end(), false);
	for (const char* pos = line.c_str(); *pos;)
		if (uint8 pi = strToEnum<uint8>(pieceNames, readWord(pos)); pi < pieceMax)
			capturers[pi] = true;
}

string Config::capturersString() const {
	string value;
	for (uint8 i = 0; i < capturers.size(); i++)
		if (capturers[i])
			value += pieceNames[i] + ' ';
	value.pop_back();
	return value;
}

void Config::readShift(const string& line) {
	for (const char* pos = line.c_str(); *pos;) {
		if (string word = readWord(pos); strcicmp(word, keywordLeft))
			shiftLeft = true;
		else if (strcicmp(word, keywordRight))
			shiftLeft = false;
		else if (strcicmp(word, keywordNear))
			shiftNear = true;
		else if (strcicmp(word, keywordFar))
			shiftNear = false;
	}
}

std::default_random_engine Com::createRandomEngine() {
	std::default_random_engine randGen;
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(time(nullptr)));
	}
	return randGen;
}
