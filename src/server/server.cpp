#include "server.h"
#include <ctime>
using namespace Com;

Date::Date(uint8 second, uint8 minute, uint8 hour, uint8 day, uint8 month, int16 year, uint8 weekDay) :
	sec(second),
	min(minute),
	hour(hour),
	day(day),
	month(month),
	wday(weekDay),
	year(year)
{}

Date Date::now() {
	time_t rawt = time(nullptr);
	struct tm* tim = localtime(&rawt);
	return Date(uint8(tim->tm_sec), uint8(tim->tm_min), uint8(tim->tm_hour), uint8(tim->tm_mday), uint8(tim->tm_mon + 1), int16(tim->tm_year + 1900), uint8(tim->tm_wday ? tim->tm_wday : 7));
}

Config::Config(string name) :
	name(std::move(name)),
	homeWidth(9),
	homeHeight(4),
	survivalPass(randomLimit / 2),
	favorLimit(4),
	dragonDist(4),
	dragonDiag(false),
	tileAmounts({ 11, 10, 7, 7, 1 }),
	middleAmounts({ 1, 1, 1, 1 }),
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
	numTiles = homeHeight * homeWidth;				// should equal the sum of tileAmounts
	extraSize = (homeHeight + 1) * homeWidth;
	boardSize = (homeHeight * 2 + 1) * homeWidth;	// aka. total amount of tiles
	
	numPieces = calcSum(pieceAmounts);
	piecesSize = numPieces * 2;
	objectSize = boardWidth / float(std::max(homeWidth, uint16(homeHeight * 2 + 1)));
}

Config& Config::checkValues() {
	homeWidth = std::clamp(homeWidth, minWidth, maxWidth);
	homeHeight = std::clamp(homeHeight, minHeight, maxHeight);
	survivalPass = std::clamp(survivalPass, uint8(0), randomLimit);

	uint16 hsize = homeHeight * homeWidth;
	for (uint8 i = 0; i < tileAmounts.size() - 2; i++)
		if (tileAmounts[i] && tileAmounts[i] < homeHeight)
			tileAmounts[i] = homeHeight;
	tileAmounts[uint8(Tile::fortress)] = hsize - floorAmounts(calcSum(tileAmounts, tileAmounts.size() - 1), tileAmounts.data(), hsize, tileAmounts.size() - 2, homeHeight);
	for (uint8 i = 0; tileAmounts[uint8(Tile::fortress)] > (homeHeight - 1) * (homeWidth - 2); i = i < tileAmounts.size() - 2 ? i + 1 : 0) {
		tileAmounts[i]++;
		tileAmounts[uint8(Tile::fortress)]--;
	}
	floorAmounts(calcSum(middleAmounts, middleAmounts.size()), middleAmounts.data(), homeWidth / 2, tileAmounts.size() - 1);

	uint16 plim = hsize < maxNumPieces ? hsize : maxNumPieces;
	uint16 psize = floorAmounts(calcSum(pieceAmounts), pieceAmounts.data(), plim, tileAmounts.size() - 1);
	
	if (!winFortress && !winThrone)
		winFortress = winThrone = 1;
	winFortress = std::clamp(winFortress, uint16(0), tileAmounts[uint8(Tile::fortress)]);
	if (winThrone && !pieceAmounts[uint8(Piece::throne)]) {
		if (psize == plim)
			(*std::find_if(pieceAmounts.rbegin(), pieceAmounts.rend(), [](uint16 amt) -> bool { return amt; }))--;
		pieceAmounts[uint8(Piece::throne)]++;
	} else
		winThrone = std::clamp(winThrone, uint16(0), pieceAmounts[uint8(Piece::throne)]);

	if (std::find(capturers.begin(), capturers.end(), true) == capturers.end())
		std::fill(capturers.begin(), capturers.end(), true);

	updateValues();
	return *this;
}

uint16 Config::floorAmounts(uint16 total, uint16* amts, uint16 limit, sizet ei, uint16 floor) {
	for (sizet i = ei; total > limit; i = i ? i - 1 : ei)
		if (amts[i] > floor) {
			amts[i]--;
			total--;
		}
	return total;
}

uint8* Config::toComData(uint8* data) const {
	data[0] = uint8(Code::setup);	// leave second byte free for first turn indicator

	uint16* sp = reinterpret_cast<uint16*>(data + 2);
	*sp++ = homeWidth;
	*sp++ = homeHeight;

	uint8* bp = reinterpret_cast<uint8*>(sp);
	*bp++ = survivalPass;
	*bp++ = favorLimit;
	*bp++ = dragonDist;
	*bp++ = dragonDiag;

	sp = std::copy(tileAmounts.begin(), tileAmounts.end(), reinterpret_cast<uint16*>(bp));
	sp = std::copy(middleAmounts.begin(), middleAmounts.end(), sp);
	sp = std::copy(pieceAmounts.begin(), pieceAmounts.end(), sp);

	*sp++ = winFortress;
	*sp++ = winThrone;

	bp = std::copy(capturers.begin(), capturers.end(), reinterpret_cast<uint8*>(sp));
	*bp++ = shiftLeft;
	*bp++ = shiftNear;
	return data;
}

void Config::fromComData(const uint8* data) {
	const uint16* sp = reinterpret_cast<const uint16*>(data + 1);	// skip turn indicator (com code should be already skipped)
	homeWidth = *sp++;
	homeHeight = *sp++;

	const uint8* bp = reinterpret_cast<const uint8*>(sp);
	survivalPass = *bp++;
	favorLimit = *bp++;
	dragonDist = *bp++;
	dragonDiag = *bp++;

	sp = reinterpret_cast<const uint16*>(bp);
	std::copy_n(sp, tileAmounts.size(), tileAmounts.begin());
	std::copy_n(sp += tileAmounts.size(), middleAmounts.size(), middleAmounts.begin());
	std::copy_n(sp += middleAmounts.size(), pieceAmounts.size(), pieceAmounts.begin());
	sp += pieceAmounts.size();

	winFortress = *sp++;
	winThrone = *sp++;

	bp = reinterpret_cast<const uint8*>(sp);
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
		return uint16(sizeof(Code) + sizeof(bool) + sizeof(homeWidth) + sizeof(homeHeight) + sizeof(uint8) * 4 + tileAmounts.size() * sizeof(tileAmounts[0]) + middleAmounts.size() * sizeof(middleAmounts[0]) + pieceAmounts.size() * sizeof(pieceAmounts[0]) + sizeof(winFortress) + sizeof(winThrone) + capturers.size() * sizeof(capturers[0]) + sizeof(shiftLeft) + sizeof(shiftNear));
	case Code::tiles:
		return tileCompressionEnd();
	case Code::pieces:
		return sizeof(Code) + numPieces * sizeof(uint16);
	case Code::move:
		return sizeof(Code) + sizeof(uint16) * 2;
	case Code::kill:
		return sizeof(Code) + sizeof(uint16);
	case Code::breach:
		return sizeof(Code) + sizeof(bool) + sizeof(uint16);
	case Code::record:
		return sizeof(Code) + sizeof(uint16) * 4 + sizeof(bool) * 3;
	case Code::win:
		return sizeof(Code) + sizeof(bool);
	}
	return sizeof(Code);
}

string Config::toIniText() const {
	string text = '[' + name + ']' + linend;
	text += makeIniLine(keywordSize, to_string(homeWidth) + ' ' + to_string(homeHeight));
	text += makeIniLine(keywordSurvival, to_string(survivalPass));
	text += makeIniLine(keywordFavors, to_string(favorLimit));
	text += makeIniLine(keywordDragonDist, to_string(dragonDist));
	text += makeIniLine(keywordDragonDiag, btos(dragonDiag));
	writeAmounts(text, keywordTile, tileNames, tileAmounts);
	writeAmounts(text, keywordMiddle, tileNames, middleAmounts);
	writeAmounts(text, keywordPiece, pieceNames, pieceAmounts);
	text += makeIniLine(keywordWinFortress, to_string(winFortress));
	text += makeIniLine(keywordWinThrone, to_string(winThrone));
	text += makeIniLine(keywordCapturers, capturersString());
	text += makeIniLine(keywordShift, string(shiftLeft ? keywordLeft : keywordRight) + ' ' + (shiftNear ? keywordNear : keywordFar));
	return text;
}

void Config::fromIniLine(const string& line) {
	if (pairStr it = readIniLine(line); !strcicmp(it.first, keywordSize))
		readSize(it.second);
	else if (!strcicmp(it.first, keywordSurvival))
		survivalPass = uint8(sstoul(it.second));
	else if (!strcicmp(it.first, keywordFavors))
		favorLimit = uint8(sstoul(it.second));
	else if (!strcicmp(it.first, keywordDragonDist))
		dragonDist = uint8(sstoul(it.second));
	else if (!strcicmp(it.first, keywordDragonDiag))
		dragonDiag = uint8(sstoul(it.second));
	else if (!strncicmp(it.first, keywordTile, strlen(keywordTile)))
		readAmount(it, keywordTile, tileNames, tileAmounts);
	else if (!strncicmp(it.first, keywordMiddle, strlen(keywordMiddle)))
		readAmount(it, keywordMiddle, tileNames, middleAmounts);
	else if (!strncicmp(it.first, keywordPiece, strlen(keywordPiece)))
		readAmount(it, keywordPiece, pieceNames, pieceAmounts);
	else if (!strcicmp(it.first, keywordWinFortress))
		winFortress = uint16(sstol(it.second));
	else if (!strcicmp(it.first, keywordWinThrone))
		winThrone = uint16(sstol(it.second));
	else if (!strcicmp(it.first, keywordCapturers))
		readCapturers(it.second);
	else if (!strcicmp(it.first, keywordShift))
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
		if (uint8 pi = strToEnum<uint8>(pieceNames, readWordM(pos)); pi < pieceMax)
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
		if (string word = readWordM(pos); !strcicmp(word, keywordLeft))
			shiftLeft = true;
		else if (!strcicmp(word, keywordRight))
			shiftLeft = false;
		else if (!strcicmp(word, keywordNear))
			shiftNear = true;
		else if (!strcicmp(word, keywordFar))
			shiftNear = false;
	}
}

vector<Config> Com::loadConfs(const string& file) {
	vector<Config> confs;
	for (const string& line : readTextFile(file)) {
		if (string title = readIniTitle(line); !title.empty())
			confs.emplace_back(std::move(title));
		else
			confs.back().fromIniLine(line);
	}
	return !confs.empty() ? confs : vector<Config>(1);
}

void Com::saveConfs(const vector<Config>& confs, const string& file) {
	string text;
	for (const Config& cfg : confs)
		text += cfg.toIniText() + linend;
	writeTextFile(file, text);
}

void Com::sendRejection(TCPsocket server) {
	TCPsocket tmps = SDLNet_TCP_Accept(server);
	uint8 code = uint8(Code::full);
	SDLNet_TCP_Send(tmps, &code, sizeof(code));
	SDLNet_TCP_Close(tmps);
	std::cout << "rejected incoming connection" << std::endl;
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
