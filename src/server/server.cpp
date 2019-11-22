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

const array<string, Config::survivalNames.size()> Config::survivalNames = {
	"disable",
	"finish",
	"kill"
};

Config::Config() :
	homeSize(9, 4),
	survivalPass(randomLimit / 2),
	survivalMode(Survival::disable),
	favorLimit(true),
	favorMax(4),
	dragonDist(4),
	dragonSingle(true),
	dragonDiag(true),
	tileAmounts({ 11, 10, 7, 7, 1 }),
	middleAmounts({ 1, 1, 1, 1 }),
	pieceAmounts({ 2, 2, 1, 1, 1, 2, 1, 1, 1, 1 }),
	winFortress(1),
	winThrone(1),
	capturers({ false, false, false, false, false, false, false, false, false, true }),
	shiftLeft(true),
	shiftNear(true)
{}

Config& Config::checkValues() {
	homeSize = glm::clamp(homeSize, minHomeSize, maxHomeSize);
	survivalPass = std::clamp(survivalPass, uint8(0), randomLimit);

	uint16 hsize = homeSize.x * homeSize.y;
	for (uint8 i = 0; i < tileAmounts.size() - 2; i++)
		if (tileAmounts[i] && tileAmounts[i] < homeSize.y)
			tileAmounts[i] = homeSize.y;
	uint16 tamt = floorAmounts(countTilesNonFort(), tileAmounts.data(), hsize, uint8(tileAmounts.size() - 2), homeSize.y);
	tileAmounts[uint8(Tile::fortress)] = hsize - ceilAmounts(tamt, homeSize.y * 4 + homeSize.x - 4, tileAmounts.data(), uint8(tileAmounts.size() - 2));
	for (uint8 i = 0; tileAmounts[uint8(Tile::fortress)] > (homeSize.y - 1) * (homeSize.x - 2); i = i < tileAmounts.size() - 2 ? i + 1 : 0) {
		tileAmounts[i]++;
		tileAmounts[uint8(Tile::fortress)]--;
	}
	floorAmounts(countMiddles(), middleAmounts.data(), homeSize.x / 2, uint8(middleAmounts.size() - 1));

	uint16 psize = floorAmounts(countPieces(), pieceAmounts.data(), hsize, uint8(pieceAmounts.size() - 1));
	if (!psize)
		psize = pieceAmounts[uint8(Piece::throne)] = 1;

	winFortress = std::clamp(winFortress, uint16(0), tileAmounts[uint8(Tile::fortress)]);
	winThrone = std::clamp(winThrone, uint16(0), pieceAmounts[uint8(Piece::throne)]);
	if (!winFortress && !winThrone)
		if (winThrone = 1; !pieceAmounts[uint8(Piece::throne)]) {
			if (psize == hsize)
				(*std::find_if(pieceAmounts.rbegin(), pieceAmounts.rend(), [](uint16 amt) -> bool { return amt; }))--;
			pieceAmounts[uint8(Piece::throne)]++;
		}

	if (std::find(capturers.begin(), capturers.end(), true) == capturers.end())
		std::fill(capturers.begin(), capturers.end(), true);
	return *this;
}

uint16 Config::floorAmounts(uint16 total, uint16* amts, uint16 limit, uint8 ei, uint16 floor) {
	for (uint8 i = ei; total > limit; i = i ? i - 1 : ei)
		if (amts[i] > floor) {
			amts[i]--;
			total--;
		}
	return total;
}

uint16 Com::Config::ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei) {
	for (uint8 i = 0; total < floor; i = i < ei ? i + 1 : 0) {
		amts[i]++;
		total++;
	}
	return total;
}

void Config::toComData(uint8* data) const {
	*data++ = uint8(homeSize.x);
	*data++ = uint8(homeSize.y);
	*data++ = survivalPass;
	*data++ = uint8(survivalMode);
	*data++ = favorLimit;
	*data++ = favorMax;
	*data++ = dragonDist;
	*data++ = dragonSingle;
	*data++ = dragonDiag;

	uint16* sp = reinterpret_cast<uint16*>(data);
	for (uint16 v : tileAmounts)
		SDLNet_Write16(v, sp++);
	for (uint16 v : middleAmounts)
		SDLNet_Write16(v, sp++);
	for (uint16 v : pieceAmounts)
		SDLNet_Write16(v, sp++);
	SDLNet_Write16(winFortress, sp++);
	SDLNet_Write16(winThrone, sp++);

	data = std::copy(capturers.begin(), capturers.end(), reinterpret_cast<uint8*>(sp));
	*data++ = shiftLeft;
	*data++ = shiftNear;
}

void Config::fromComData(uint8* data) {
	homeSize.x = *data++;
	homeSize.y = *data++;
	survivalPass = *data++;
	survivalMode = Survival(*data++);
	favorLimit = *data++;
	favorMax = *data++;
	dragonDist = *data++;
	dragonSingle = *data++;
	dragonDiag = *data++;

	uint16* sp = reinterpret_cast<uint16*>(data);
	for (uint16& v : tileAmounts)
		v = SDLNet_Read16(sp++);
	for (uint16& v : middleAmounts)
		v = SDLNet_Read16(sp++);
	for (uint16& v : pieceAmounts)
		v = SDLNet_Read16(sp++);
	winFortress = SDLNet_Read16(sp++);
	winThrone = SDLNet_Read16(sp++);

	data = reinterpret_cast<uint8*>(sp);
	std::copy(data, data + pieceMax, capturers.begin());
	data += pieceMax;

	shiftLeft = *data++;
	shiftNear = *data++;
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
		SDLNet_Write16(codeSizes.at(Code(data[0])), data + 1);
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

string Com::readVersion(uint8* data) {
	string name;
	name.resize(SDLNet_Read16(data + 1) - dataHeadSize);
	std::copy_n(data + dataHeadSize, name.length(), name.data());
	return name;
}

// BUFFER

Buffer::Buffer() :
	dpos(0)
{}

void Buffer::clear() {
	data.clear();
	dpos = 0;
}

void Buffer::push(const vector<uint16>& vec) {
	checkEnd(dpos + uint(vec.size()) * sizeof(*vec.data()));
	for (uint16 i = 0; i < vec.size(); i++, dpos += sizeof(*vec.data()))
		SDLNet_Write16(vec[i], data.data() + dpos);
}

void Buffer::push(const uint8* vec, uint len) {
	uint end = checkEnd(dpos + len);
	std::copy_n(vec, len, data.data() + dpos);
	dpos = end;
}

void Buffer::push(uint8 val) {
	uint end = checkEnd(dpos + 1);
	data[dpos] = val;
	dpos = end;
}

void Buffer::push(uint16 val) {
	uint end = checkEnd(dpos + sizeof(val));
	SDLNet_Write16(val, data.data() + dpos);
	dpos = end;
}

uint Buffer::preallocate(Com::Code code, uint dlen) {
	uint end = checkEnd(dpos + dlen);
	data[dpos] = uint8(code);
	SDLNet_Write16(dlen, data.data() + dpos + 1);
	uint ret = dpos + Com::dataHeadSize;
	dpos = end;
	return ret;
}

uint Buffer::pushHead(Com::Code code, uint dlen) {
	uint end = checkEnd(dpos + Com::dataHeadSize);
	data[dpos] = uint8(code);
	SDLNet_Write16(dlen, data.data() + dpos + 1);
	return dpos = end;
}

uint Buffer::write(uint8 val, uint pos) {
	data[pos] = val;
	return pos += sizeof(val);
}

uint Buffer::write(uint16 val, uint pos) {
	SDLNet_Write16(val, data.data() + pos);
	return pos + sizeof(val);
}

const char* Buffer::send(TCPsocket socket) {
	const char* err = send(socket, dpos);
	if (!err)
		clear();
	return err;
}

uint Buffer::checkEnd(uint end) {
	if (end > data.size())
		data.resize(end);
	return end;
}
