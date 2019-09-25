#include "server.h"
#include <ctime>
using namespace Com;

// DATE

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

// CONFIG

Config::Config() :
	homeSize(9, 4),
	survivalPass(randomLimit / 2),
	favorLimit(4),
	dragonDist(4),
	dragonDiag(true),
	multistage(false),
	survivalKill(false),
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
	numTiles = homeSize.y * homeSize.x;				// should equal the sum of tileAmounts
	extraSize = (homeSize.y + 1) * homeSize.x;
	boardSize = (homeSize.y * 2 + 1) * homeSize.x;	// aka. total amount of tiles
	
	numPieces = calcSum(pieceAmounts);
	piecesSize = numPieces * 2;
	objectSize = boardWidth / float(std::max(homeSize.x, uint16(homeSize.y * 2 + 1)));
}

Config& Config::checkValues() {
	homeSize = homeSize.clamp(minHomeSize, maxHomeSize);
	survivalPass = std::clamp(survivalPass, uint8(0), randomLimit);

	uint16 hsize = homeSize.area();
	for (uint8 i = 0; i < tileAmounts.size() - 2; i++)
		if (tileAmounts[i] && tileAmounts[i] < homeSize.y)
			tileAmounts[i] = homeSize.y;
	tileAmounts[uint8(Tile::fortress)] = hsize - floorAmounts(calcSum(tileAmounts, tileAmounts.size() - 1), tileAmounts.data(), hsize, tileAmounts.size() - 2, homeSize.y);
	for (uint8 i = 0; tileAmounts[uint8(Tile::fortress)] > (homeSize.y - 1) * (homeSize.x - 2); i = i < tileAmounts.size() - 2 ? i + 1 : 0) {
		tileAmounts[i]++;
		tileAmounts[uint8(Tile::fortress)]--;
	}
	floorAmounts(calcSum(middleAmounts), middleAmounts.data(), homeSize.x / 2, middleAmounts.size() - 1);

	uint16 plim = hsize < maxNumPieces ? hsize : maxNumPieces;
	uint16 psize = floorAmounts(calcSum(pieceAmounts), pieceAmounts.data(), plim, pieceAmounts.size() - 1);
	
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

void Config::toComData(uint8* data) const {
	uint16* sp = reinterpret_cast<uint16*>(data);
	SDLNet_Write16(homeSize.x, sp++);
	SDLNet_Write16(homeSize.y, sp++);

	uint8* bp = reinterpret_cast<uint8*>(sp);
	*bp++ = survivalPass;
	*bp++ = favorLimit;
	*bp++ = dragonDist;
	*bp++ = dragonDiag;
	*bp++ = multistage;
	*bp++ = survivalKill;

	sp = reinterpret_cast<uint16*>(bp);
	for (uint16 v : tileAmounts)
		SDLNet_Write16(v, sp++);
	for (uint16 v : middleAmounts)
		SDLNet_Write16(v, sp++);
	for (uint16 v : pieceAmounts)
		SDLNet_Write16(v, sp++);
	SDLNet_Write16(winFortress, sp++);
	SDLNet_Write16(winThrone, sp++);

	bp = std::copy(capturers.begin(), capturers.end(), reinterpret_cast<uint8*>(sp));
	*bp++ = shiftLeft;
	*bp++ = shiftNear;
}

void Config::fromComData(uint8* data) {
	uint16* sp = reinterpret_cast<uint16*>(data);
	homeSize.x = SDLNet_Read16(sp++);
	homeSize.y = SDLNet_Read16(sp++);

	uint8* bp = reinterpret_cast<uint8*>(sp);
	survivalPass = *bp++;
	favorLimit = *bp++;
	dragonDist = *bp++;
	dragonDiag = *bp++;
	multistage = *bp++;
	survivalKill = *bp++;

	sp = reinterpret_cast<uint16*>(bp);
	for (uint16& v : tileAmounts)
		v = SDLNet_Read16(sp++);
	for (uint16& v : middleAmounts)
		v = SDLNet_Read16(sp++);
	for (uint16& v : pieceAmounts)
		v = SDLNet_Read16(sp++);
	winFortress = SDLNet_Read16(sp++);
	winThrone = SDLNet_Read16(sp++);

	bp = reinterpret_cast<uint8*>(sp);
	std::copy(bp, bp + pieceMax, capturers.begin());
	bp += pieceMax;

	shiftLeft = *bp++;
	shiftNear = *bp++;
	updateValues();
}

string Config::capturersString() const {
	string value;
	for (uint8 i = 0; i < capturers.size(); i++)
		if (capturers[i])
			value += pieceNames[i] + ' ';
	value.pop_back();
	return value;
}

void Config::readCapturers(const string& line) {
	std::fill(capturers.begin(), capturers.end(), false);
	for (const char* pos = line.c_str(); *pos;)
		if (uint8 pi = strToEnum<uint8>(pieceNames, readWordM(pos)); pi < pieceMax)
			capturers[pi] = true;
}

// FUNCTIONS

int Com::sendRejection(TCPsocket server) {
	int sent = 0;
	if (TCPsocket tmps = SDLNet_TCP_Accept(server)) {
		uint8 data[dataHeadSize] = { uint8(Code::full) };
		SDLNet_Write16(codeSizes[data[0]], data + 1);
		sent = SDLNet_TCP_Send(tmps, data, dataHeadSize);
		SDLNet_TCP_Close(tmps);
	}
	return sent;
}

string Com::readName(const uint8* data) {
	string name;
	name.resize(data[0]);
	std::copy_n(data + 1, name.length(), name.data());
	return name;
}

// BUFFER

void Buffer::push(const vector<uint16>& vec) {
	for (uint16 i = 0; i < vec.size(); i++)
		SDLNet_Write16(vec[i], data + i * sizeof(uint16) + size);
	size += uint16(vec.size() * sizeof(uint16));
}

void Buffer::push(const uint8* vec, uint16 len) {
	std::copy_n(vec, len, data + size);
	size += len;
}

void Buffer::push(uint16 val) {
	SDLNet_Write16(val, data + size);
	size += sizeof(val);
}

uint16 Buffer::writeHead(Com::Code code, uint16 clen, uint16 pos) {
	data[pos++] = uint8(code);
	SDLNet_Write16(clen, data + pos);
	return pos + sizeof(uint16);
}
