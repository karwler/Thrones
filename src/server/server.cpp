#include "server.h"

namespace Com {

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

uint16 Config::ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei) {
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

static uint32 rol(uint32 value, uint8 bits) {
	return (value << bits) | (value >> (32 - bits));
}

static uint32& blk0(uint32& block) {
	return block = (rol(block, 24) & 0xFF00FF00) | (rol(block, 8) & 0x00FF00FF);
}

static uint32& blk(uint32* block, uint i) {
	return block[i&15] = rol(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i&15], 1);
}

static void sha1Transform(uint32* state, uint8* buffer) {
	uint32* block = reinterpret_cast<uint32*>(buffer);
	array<uint32, 5> scop = { state[0], state[1], state[2], state[3], state[4] };
	for (uint i = 0; i < 80; i++) {
		if (i < 16)
			scop[4] += ((scop[1] & (scop[2] ^ scop[3])) ^ scop[3]) + blk0(block[i]) + 0x5A827999 + rol(scop[0], 5);
		else if (i < 20)
			scop[4] += ((scop[1] & (scop[2] ^ scop[3])) ^ scop[3]) + blk(block, i) + 0x5A827999 + rol(scop[0], 5);
		else if (i < 40)
			scop[4] += (scop[1] ^ scop[2] ^ scop[3]) + blk(block, i) + 0x6ED9EBA1 + rol(scop[0], 5);
		else if (i < 60)
			scop[4] += (((scop[1] | scop[2]) & scop[3]) | (scop[1] & scop[2])) + blk(block, i) + 0x8F1BBCDC + rol(scop[0], 5);
		else
			scop[4] += (scop[1] ^ scop[2] ^ scop[3]) + blk(block, i) + 0xCA62C1D6 + rol(scop[0], 5);
		scop[1] = rol(scop[1], 30);
		std::rotate(scop.begin(), scop.end() - 1, scop.end());
	}
	for (uint i = 0; i < 5; i++)
		state[i] += scop[i];
}

static void sha1Update(uint32* state, uint32* count, uint8* buffer, uint8* data, uint32 len) {
	uint32 j = count[0];
	if (count[0] += len << 3; count[0] < j)
		count[1]++;
	count[1] += len >> 29;
	j = (j >> 3) & 63;

	uint32 i;
	if (j + len > 63) {
		i = 64 - j;
		std::copy_n(data, i, buffer + j);
		sha1Transform(state, buffer);
		for (; i + 63 < len; i += 64)
			sha1Transform(state, data + i);
		j = 0;
	} else
		i = 0;
	std::copy_n(data + i, len - i, buffer + j);
}

string digestSHA1(string str) {
	uint32 state[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
	uint32 count[2] = { 0, 0 };
	uint8 buffer[64];
	for (sizet i = 0; i < str.length(); i++)
		sha1Update(state, count, buffer, reinterpret_cast<uint8*>(str.data()) + i, 1);

	uint8 finalcount[8];
	for (uint i = 0; i < 8; i++)
		finalcount[i] = (count[i<4] >> ((3 - (i & 3)) * 8)) & 255;

	uint8 c = 0x80;
	sha1Update(state, count, buffer, &c, 1);
	while ((count[0] & 504) != 448) {
		c = 0;
		sha1Update(state, count, buffer, &c, 1);
	}
	sha1Update(state, count, buffer, finalcount, 8);

	str.resize(20);
	for (sizet i = 0; i < str.length(); i++)
		str[i] = char((state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
	return str;
}

string encodeBase64(const string& str) {
	constexpr char b64charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	string ret;
	uint8 arr3[3], arr4[4];
	uint i = 0;
	for (sizet l = 0; l < str.length(); l++) {
		arr3[i++] = uint8(str[l]);
		if (i == 3) {
			arr4[0] = (arr3[0] & 0xFC) >> 2;
			arr4[1] = uint8(((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xF0) >> 4));
			arr4[2] = uint8(((arr3[1] & 0x0F) << 2) + ((arr3[2] & 0xC0) >> 6));
			arr4[3] = arr3[2] & 0x3F;

			for (i = 0; i < 4 ; i++)
				ret += b64charset[arr4[i]];
			i = 0;
		}
	}
	if (i) {
		for (uint j = i; j < 3; j++)
			arr3[j] = '\0';
		arr4[0] = (arr3[0] & 0xFC) >> 2;
		arr4[1] = uint8(((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xF0) >> 4));
		arr4[2] = uint8(((arr3[1] & 0x0F) << 2) + ((arr3[2] & 0xC0) >> 6));

		for (uint j = 0; j < i + 1; j++)
			ret += b64charset[arr4[j]];
		while (i++ < 3)
			ret += '=';
	}
	return ret;
}

void sendWaitClose(TCPsocket socket) {
	if (uint8 frame[wsHeadMin] = { 0x88, 0 }; SDLNet_TCP_Send(socket, frame, wsHeadMin) == wsHeadMin)
		SDLNet_TCP_Recv(socket, frame, wsHeadMin);	// should be a close response (I don't care about further checks)
}

void sendRejection(TCPsocket server) {
	if (TCPsocket tmps = SDLNet_TCP_Accept(server)) {
		uint8 data[dataHeadSize] = { uint8(Code::full) };
		SDLNet_Write16(codeSizes.at(Code(data[0])), data + 1);
		SDLNet_TCP_Send(tmps, data, dataHeadSize);
		SDLNet_TCP_Close(tmps);
	}
}

void sendVersion(TCPsocket socket, bool webs) {
	uint16 len = uint16(strlen(commonVersion));
	vector<uint8> data(dataHeadSize + len);
	data[0] = uint8(Code::version);
	SDLNet_Write16(uint16(data.size()), data.data() + 1);
	std::copy_n(commonVersion, len, data.data() + dataHeadSize);
	sendData(socket, data.data(), uint(data.size()), webs);
}

void sendData(TCPsocket socket, const uint8* data, uint len, bool webs) {
	if (webs) {
		uint ofs = wsHeadMin;
		uint8 frame[wsHeadMax] = {
			0x82,
			uint8(len <= 125 ? len : len <= UINT16_MAX ? 126 : 127)
		};
		if (len > UINT16_MAX) {
			ofs += sizeof(uint64);
			SDLNet_Write32(len, frame + wsHeadMin);
		} else if (len > 125) {
			ofs += sizeof(uint16);
			write64(len, frame + wsHeadMin);
		}
		vector<uint8> wdat(len + ofs);
		std::copy_n(frame, ofs, wdat.begin());
		std::copy_n(data, len, wdat.begin() + ofs);
		if (SDLNet_TCP_Send(socket, wdat.data(), int(wdat.size())) != int(wdat.size()))
			throw NetError(SDLNet_GetError());
	} else if (SDLNet_TCP_Send(socket, data, int(len)) != int(len))
		throw NetError(SDLNet_GetError());
}

string readVersion(uint8* data) {
	string ver;
	ver.resize(SDLNet_Read16(data + 1) - dataHeadSize);
	std::copy_n(data + dataHeadSize, ver.length(), ver.data());
	return ver;
}

string readName(const uint8* data) {
	string name;
	name.resize(data[0]);
	std::copy_n(data + 1, name.length(), name.data());
	return name;
}

// BUFFER

void Buffer::clear() {
	data.clear();
	dlim = 0;
}

void Buffer::push(const vector<uint16>& vec) {
	checkEnd(dlim + uint(vec.size()) * sizeof(*vec.data()));
	for (uint16 i = 0; i < vec.size(); i++, dlim += sizeof(*vec.data()))
		SDLNet_Write16(vec[i], data.data() + dlim);
}

void Buffer::push(const uint8* vec, uint len) {
	uint end = checkEnd(dlim + len);
	std::copy_n(vec, len, data.data() + dlim);
	dlim = end;
}

void Buffer::push(uint8 val) {
	uint end = checkEnd(dlim + 1);
	data[dlim] = val;
	dlim = end;
}

void Buffer::push(uint16 val) {
	uint end = checkEnd(dlim + sizeof(val));
	SDLNet_Write16(val, data.data() + dlim);
	dlim = end;
}

uint Buffer::preallocate(Code code, uint dlen) {
	uint end = checkEnd(dlim + dlen);
	data[dlim] = uint8(code);
	SDLNet_Write16(dlen, data.data() + dlim + 1);
	uint ret = dlim + dataHeadSize;
	dlim = end;
	return ret;
}

uint Buffer::pushHead(Code code, uint dlen) {
	uint end = checkEnd(dlim + dataHeadSize);
	data[dlim] = uint8(code);
	SDLNet_Write16(dlen, data.data() + dlim + 1);
	return dlim = end;
}

uint Buffer::write(uint8 val, uint pos) {
	data[pos] = val;
	return pos += sizeof(val);
}

uint Buffer::write(uint16 val, uint pos) {
	SDLNet_Write16(val, data.data() + pos);
	return pos + sizeof(val);
}

void Buffer::redirect(TCPsocket socket, uint8* pos, bool sendWebs) {
	if (pos == data.data())
		send(socket, sendWebs, false);	// send like normal
	else if (sendWebs) {
		if (data[1] & 0x80) {
			data[1] &= 0x7F;
			std::copy(pos, data.data() + dlim, pos - sizeof(uint32));	// should already be unmasked
			dlim -= sizeof(uint32);
		}
		send(socket, false, false);	// reuse ws frame without mask
	} else
		sendData(socket, pos, dlim - uint(pos - data.data()), false);	// skip ws frame
}

void Buffer::send(TCPsocket socket, bool webs, bool clr) {
	if (sendData(socket, data.data(), dlim, webs); clr)
		clear();
}

uint8* Buffer::recv(TCPsocket socket, bool webs) {
	uint ofs, hsize = 0;
	uint8* mask = nullptr;
	return recvHead(socket, hsize, ofs, mask, webs) ? recvLoad(socket, hsize, ofs, mask) : nullptr;
}

Buffer::Init Buffer::recvInit(TCPsocket socket, bool& webs) {
	uint ofs, hsize = 0;
	uint8* mask = nullptr;
	if (!recvHead(socket, hsize, ofs, mask, webs))
		return Init::wait;

	switch (Code(data[ofs])) {
	case Code::version: {
		uint8* dat = recvLoad(socket, hsize, ofs, mask);
		if (!dat)
			return Init::wait;

		string ver = readVersion(dat);
		decltype(compatibleVersions)::const_iterator it = std::find(compatibleVersions.begin(), compatibleVersions.end(), ver);
		if (clear(); it == compatibleVersions.end()) {
			std::cout << "failed to validate player of version " << ver << std::endl;
			return Init::version;
		}
		return Init::connect; }
	case Code::wsconn: {
		if (webs)
			return Init::error;

		string word = "\r\n\r\n";
		uint8 *dend, *rend;
		uint memSize = expectedRequestSize;
		do {
			if (dlim == memSize)
				memSize += expectedRequestSize;
			recvDat(socket, memSize);
			dend = data.data() + dlim;
			rend = std::search(data.data(), dend, word.begin(), word.end());
		} while (rend == dend);

		rend += word.length();
		word = "Sec-WebSocket-Key:";
		uint8* pos = std::search(data.data(), rend, word.begin(), word.end());
		if (pos == rend)
			return Init::error;

		pos += word.length();
		word = "\r\n";
		string key = string(pos, std::search(pos, rend, word.begin(), word.end()));
		string response = "HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: " + encodeBase64(digestSHA1(trim(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) + "\r\n\r\n";
		if (SDLNet_TCP_Send(socket, response.c_str(), int(response.length())) != int(response.length()))
			throw NetError(SDLNet_GetError());

		pdift diff = rend - data.data();
		data.erase(data.begin(), data.begin() + diff);
		dlim -= uint(diff);
		webs = true;
		break; }
	default:
		std::cerr << "invalid init code '" << uint(data[0]) << '\'' << std::endl;
		return Init::error;
	}
	return Init::wait;
}

bool Buffer::recvHead(TCPsocket socket, uint& hsize, uint& ofs, uint8*& mask, bool webs) {
	if (!SDLNet_SocketReady(socket))
		return false;

	if (webs) {
		if (hsize += wsHeadMin; dlim < hsize)
			if (recvDat(socket, hsize); dlim < hsize)
				return false;
		if ((data[0] & 0xF0) != 0x80)	// TODO: handle fragmentation
			throw NetError("Protocol error");

		uint plen = data[1] & 0x7F;
		if (plen == 126) {
			if (hsize += sizeof(uint16); dlim < hsize)
				if (recvDat(socket, hsize); dlim < hsize)
					return false;
			plen = SDLNet_Read16(data.data() + wsHeadMin);
		} else if (plen == 127) {
			if (hsize += sizeof(uint64); dlim < hsize)
				if (recvDat(socket, hsize); dlim < hsize)
					return false;
			plen = uint(read64(data.data() + wsHeadMin));
		}

		if (data[1] & 0x80) {
			if (hsize += sizeof(uint32); dlim < hsize)
				if (recvDat(socket, hsize); dlim < hsize)
					return false;
			mask = data.data() + hsize - sizeof(uint32);
		}

		if (uint8 opc = data[0] & 0xF; opc != 2) {
			switch (opc) {
			case 8:
				resendWs(socket, hsize, plen, mask);
				throw NetError("Connection closed");
			case 9:
				data[0] = 0x8A;
				resendWs(socket, hsize, plen, mask);
				break;
			default:
				throw NetError("Protocol error");
			}
			return false;
		}
	}

	ofs = hsize;
	if (hsize += dataHeadSize; dlim < hsize) {
		if (recvDat(socket, hsize); dlim < hsize)
			return false;
		if (mask)
			unmask(mask, ofs, ofs);
	}
	return true;
}

uint8* Buffer::recvLoad(TCPsocket socket, uint hsize, uint ofs, uint8* mask) {
	uint end = SDLNet_Read16(data.data() + ofs + 1) + ofs;
	if (dlim < end)
		recvDat(socket, end);
	if (dlim < end)
		return nullptr;
	if (mask)
		unmask(mask, hsize, ofs);
	return data.data() + ofs;
}

void Buffer::resendWs(TCPsocket socket, uint hsize, uint plen, const uint8* mask) {
	uint8 maskdat[4];
	if (mask) {
		data[1] &= 0x7F;
		hsize -= sizeof(uint32);
		dlim = hsize;
		std::copy_n(mask, sizeof(uint32), maskdat);
	}
	uint end = hsize + plen;
	while (dlim < end)
		recvDat(socket, end);
	if (mask)
		unmask(maskdat, hsize, hsize);
	send(socket, false);
}

void Buffer::recvDat(TCPsocket socket, uint size) {
	checkEnd(size);
	int len = SDLNet_TCP_Recv(socket, data.data() + dlim, int(size - dlim));
	if (len <= 0)
		throw NetError("Connection lost");
	dlim += uint(len);
}

uint Buffer::checkEnd(uint end) {
	if (end > data.size())
		data.resize(end);
	return end;
}

void Buffer::unmask(const uint8* mask, uint i, uint ofs) {
	for (; i < dlim; i++)
		data[i] ^= mask[(i - ofs) % sizeof(uint32)];
}

}
