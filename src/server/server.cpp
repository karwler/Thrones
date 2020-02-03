#include "server.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
using sendlen = int;
using socklent = int;
#else
using sendlen = ssize_t;
using socklent = socklen_t;
#endif
#ifndef POLLRDHUP
#define POLLRDHUP 0	// ignore if not present
#endif

namespace Com {

// CONFIG

Config::Config() :
	homeSize(9, 4),
	survivalPass(randomLimit / 2),
	survivalMode(Survival::disable),
	favorLimit(true),
	favorMax(4),
	dragonDist(4),
	dragonSingle(true),
	dragonDiag(true),
	tileAmounts{ 13, 10, 5, 7 },
	middleAmounts{ 1, 1, 1, 1 },
	pieceAmounts{ 2, 2, 1, 1, 1, 2, 1, 1, 1, 1 },
	winFortress(1),
	winThrone(1),
	capturers{ false, false, false, false, false, false, false, false, false, true },
	shiftLeft(true),
	shiftNear(true)
{}

Config& Config::checkValues() {
	homeSize = glm::clamp(homeSize, minHomeSize, maxHomeSize);
	survivalPass = std::clamp(survivalPass, uint8(0), randomLimit);

	for (uint16& it : tileAmounts)
		if (it < homeSize.y)
			it = homeSize.y;
	uint16 hsize = homeSize.x * homeSize.y;
	uint16 tamt = floorAmounts(countTiles(), tileAmounts.data(), hsize, uint8(tileAmounts.size() - 1), homeSize.y);
	uint16 fort = hsize - ceilAmounts(tamt, homeSize.y * 4 + homeSize.x - 4, tileAmounts.data(), uint8(tileAmounts.size() - 1));
	for (uint8 i = 0; fort > (homeSize.y - 1) * (homeSize.x - 2); i = i < tileAmounts.size() - 1 ? i + 1 : 0) {
		tileAmounts[i]++;
		fort--;
	}
	floorAmounts(countMiddles(), middleAmounts.data(), homeSize.x / 2, uint8(middleAmounts.size() - 1));

	uint16 psize = floorAmounts(countPieces(), pieceAmounts.data(), hsize, uint8(pieceAmounts.size() - 1));
	if (!psize)
		psize = pieceAmounts[uint8(Piece::throne)] = 1;

	winFortress = std::clamp(winFortress, uint16(0), fort);
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
		write16(sp++, v);
	for (uint16 v : middleAmounts)
		write16(sp++, v);
	for (uint16 v : pieceAmounts)
		write16(sp++, v);
	write16(sp++, winFortress);
	write16(sp++, winThrone);

	data = std::copy(capturers.begin(), capturers.end(), reinterpret_cast<uint8*>(sp));
	*data++ = shiftLeft;
	*data++ = shiftNear;
}

void Config::fromComData(const uint8* data) {
	homeSize.x = *data++;
	homeSize.y = *data++;
	survivalPass = *data++;
	survivalMode = Survival(*data++);
	favorLimit = *data++;
	favorMax = *data++;
	dragonDist = *data++;
	dragonSingle = *data++;
	dragonDiag = *data++;

	const uint16* sp = reinterpret_cast<const uint16*>(data);
	for (uint16& v : tileAmounts)
		v = read16(sp++);
	for (uint16& v : middleAmounts)
		v = read16(sp++);
	for (uint16& v : pieceAmounts)
		v = read16(sp++);
	winFortress = read16(sp++);
	winThrone = read16(sp++);

	data = reinterpret_cast<const uint8*>(sp);
	std::copy(data, data + pieceMax, capturers.begin());
	data += pieceMax;

	shiftLeft = *data++;
	shiftNear = *data++;
}

string Config::capturersString() const {
	string value;
	for (uint8 i = 0; i < capturers.size(); i++)
		if (capturers[i])
			value += string(pieceNames[i]) + ' ';
	value.pop_back();
	return value;
}

void Config::readCapturers(const string& line) {
	std::fill(capturers.begin(), capturers.end(), false);
	for (const char* pos = line.c_str(); *pos;)
		if (uint8 pi = strToEnum<uint8>(pieceNames, readWordM(pos)); pi < pieceMax)
			capturers[pi] = true;
}

// UNIVERSAL FUNCTIONS

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

void discardAccept(nsint server) {
	try {
		nsint socket = acceptSocket(server);
		closeSocket(socket);
	} catch (const Error&) {}
}

void sendWaitClose(nsint socket) {
	try {
		uint8 frame[wsHeadMin] = { 0x88, 0 };
		sendNet(socket, frame, wsHeadMin);
		recvNet(socket, frame, wsHeadMin);	// should be a close response (I don't care to check)
	} catch (const Error&) {}
}

void sendVersion(nsint socket, bool webs) {
	uint16 len = uint16(strlen(commonVersion));
	vector<uint8> data(dataHeadSize + len);
	data[0] = uint8(Code::version);
	write16(data.data() + 1, uint16(data.size()));
	std::copy_n(commonVersion, len, data.data() + dataHeadSize);
	sendData(socket, data.data(), uint(data.size()), webs);
}

void sendData(nsint socket, const uint8* data, uint len, bool webs) {
	if (webs) {
		uint ofs = wsHeadMin;
		uint8 frame[wsHeadMax] = { 0x82 };
		if (len <= 125)
			frame[1] = uint8(len);
		else if (len <= UINT16_MAX) {
			frame[1] = 126;
			write16(frame + wsHeadMin, uint16(len));
			ofs += sizeof(uint16);
		} else {
			frame[1] = 127;
			write64(frame + wsHeadMin, len);
			ofs += sizeof(uint64);
		}
		vector<uint8> wdat(len + ofs);
		std::copy_n(frame, ofs, wdat.begin());
		std::copy_n(data, len, wdat.begin() + ofs);
		sendNet(socket, wdat.data(), uint(wdat.size()));
	} else
		sendNet(socket, data, len);
}

// SOCKET FUNCTIONS

static addrinfo* resolveAddress(const char* addr, uint16 port, Family family) {
	addrinfo hints;
	addrinfo* info;
	memset(&hints, 0, sizeof(hints));
#if defined(__ANDROID__)
	hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
#elif defined(EMSCRIPTEN)
	hints.ai_flags = AI_V4MAPPED | AI_NUMERICSERV;
#else
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
#endif
	if (!addr)
		hints.ai_flags |= AI_PASSIVE;
	hints.ai_family = family == Family::v4 ? AF_INET : family == Family::v6 ? AF_INET6 : AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	return !getaddrinfo(addr, toStr(port).c_str(), &hints, &info) ? info : nullptr;
}

static nsint createSocket(int family, int reuseaddr, int nodelay = 1) {
	if (family != AF_INET && family != AF_INET6)
		return -1;

	nsint fd = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (fd != -1) {
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuseaddr), sizeof(reuseaddr));
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&nodelay), sizeof(nodelay));
	}
	return fd;
}

nsint connectSocket(const char* addr, uint16 port, Family family) {
	addrinfo* inf = resolveAddress(addr, port, family);
	if (!inf)
		throw Error(msgConnectionFail);

	nsint fd = -1;
	for (addrinfo* it = inf; it; it = it->ai_next) {
		if (fd = createSocket(it->ai_family, 0); fd == -1)
			continue;

#ifdef EMSCRIPTEN
		if (::connect(fd, it->ai_addr, it->ai_addrlen) && errno != EINPROGRESS)
#else
		if (::connect(fd, it->ai_addr, socklent(it->ai_addrlen)))
#endif
			closeSocket(fd);
		else
			break;
	}
	freeaddrinfo(inf);
	if (fd == -1)
		throw Error(msgConnectionFail);
	return fd;
}

nsint bindSocket(uint16 port, Family family) {
	addrinfo* inf = resolveAddress(nullptr, port, family);
	if (!inf)
		throw Error(msgBindFail);

	nsint fd = -1;
	for (addrinfo* it = inf; it; it = it->ai_next) {
		if (fd = createSocket(it->ai_family, 1); fd == -1)
			continue;
		if (bind(fd, it->ai_addr, socklent(it->ai_addrlen)))
			closeSocket(fd);
		else if (listen(fd, 8))
			closeSocket(fd);
		else
			break;
	}
	freeaddrinfo(inf);
	if (fd == -1)
		throw Error(msgBindFail);
	return fd;
}

nsint acceptSocket(nsint fd) {
	nsint sock = accept(fd, nullptr, nullptr);
	if (sock == -1)
		throw Error(msgAcceptFail);
	return sock;
}

bool pollSocket(nsint fd, int timeout) {
	pollfd pfd = { fd, POLLIN | POLLRDHUP, 0 };
#ifdef _WIN32
	int rc = WSAPoll(&pfd, 1, timeout);
#else
	int rc = poll(&pfd, 1, timeout);
#endif
	if (!rc)
		return false;
	if (rc < 0)
		throw Error(msgPollFail);
	if (pfd.revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL))
		throw Error(msgConnectionLost);
	if (pfd.revents & POLLIN)
		return true;
	return false;
}

void noblockSocket(nsint fd, bool noblock) {
#if defined(_WIN32)
	if (u_long on = noblock; ioctlsocket(fd, FIONBIO, &on))
		throw Error(msgIoctlFail);
#elif !defined(EMSCRIPTEN)
	if (int on = noblock; ioctl(fd, FIONBIO, &on))
		throw Error(msgIoctlFail);
#endif
}

void sendNet(nsint fd, const void* data, uint size) {
	if (send(fd, static_cast<const char*>(data), size, 0) != sendlen(size))
		throw Error(msgConnectionLost);
}

uint recvNet(nsint fd, void* data, uint size) {
	long len = recv(fd, static_cast<char*>(data), size, 0);
#ifdef EMSCRIPTEN
	if (!len || (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
#else
	if (len <= 0)
#endif
		throw Error(msgConnectionLost);
	return uint(len);
}

long recvNow(nsint fd, void* data, uint size) {
#if defined(_WIN32)
	int len = recv(fd, static_cast<char*>(data), size, 0);
	if (!len || (len < 0 && WSAGetLastError() != WSAEWOULDBLOCK)) {
		noblockSocket(fd, false);
		throw Error(msgConnectionLost);
	}
#elif !defined(MSG_DONTWAIT)	// aka. macOS
	ssize_t len = recv(fd, data, size, 0);
	if (!len || (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
		noblockSocket(fd, false);
		throw Error(msgConnectionLost);
	}
#else
	ssize_t len = recv(fd, data, size, MSG_DONTWAIT);
	if (!len || (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
		throw Error(msgConnectionLost);
#endif
	return len;
}

void closeSocket(nsint& fd) {
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
	fd = -1;
}

// BUFFER

Buffer::Buffer() :
	data(new uint8[sizeStep]),
	size(sizeStep),
	dlim(0)
{}

Buffer::Buffer(Buffer&& b) :
	data(b.data),
	size(b.size),
	dlim(b.dlim)
{
	b.data = nullptr;
}

Buffer& Buffer::operator=(Buffer&& b) {
	delete[] data;
	data = b.data;
	size = b.size;
	dlim = b.dlim;
	b.data = nullptr;
	return *this;
}

void Buffer::push(initlist<uint8> lst) {
	uint end = checkOver(dlim + uint(lst.size()));
	std::copy(lst.begin(), lst.end(), data + dlim);
	dlim = end;
}

void Buffer::push(initlist<uint16> lst) {
	checkOver(dlim + uint(lst.size() * sizeof(uint16)));
	for (uint16 n : lst) {
		write16(data + dlim, n);
		dlim += sizeof(uint16);
	}
}

void Buffer::push(const string& str) {
	uint end = checkOver(dlim + uint(str.length()));
	std::copy(str.begin(), str.end(), data + dlim);
	dlim = end;
}

void Buffer::push(uint8 val) {
	checkOver(dlim + 1);
	data[dlim++] = val;
}

void Buffer::push(uint16 val) {
	uint end = checkOver(dlim + sizeof(val));
	write16(data + dlim, val);
	dlim = end;
}

uint Buffer::allocate(Code code, uint16 dlen) {
	uint end = checkOver(dlim + dlen);
	data[dlim] = uint8(code);
	write16(data + dlim + 1, dlen);
	uint ret = dlim + dataHeadSize;
	dlim = end;
	return ret;
}

uint Buffer::pushHead(Code code, uint16 dlen) {
	uint end = checkOver(dlim + dataHeadSize);
	data[dlim] = uint8(code);
	write16(data + dlim + 1, dlen);
	return dlim = end;
}

uint Buffer::write(uint8 val, uint pos) {
	data[pos] = val;
	return pos += sizeof(val);
}

uint Buffer::write(uint16 val, uint pos) {
	write16(data + pos, val);
	return pos + sizeof(val);
}

void Buffer::redirect(nsint socket, uint8* pos, bool sendWebs) {
	if (pos == data)	// no offset means no ws frame
		sendData(socket, data, readLoadSize(false), sendWebs);	// send like normal
	else if (sendWebs) {
		if (data[1] & 0x80) {
			data[1] &= 0x7F;
			std::copy(pos, data + dlim, pos - sizeof(uint32));	// should already be unmasked
			dlim -= sizeof(uint32);	// skip resize because there should be an erase right after this anyway
		}
		sendNet(socket, data, readLoadSize(true));	// reuse ws frame without mask
	} else
		sendNet(socket, pos, read16(pos + 1));	// skip ws frame
}

void Buffer::send(nsint socket, bool webs, bool clr) {
	if (sendData(socket, data, dlim, webs); clr)
		clear();
}

uint8* Buffer::recv(nsint socket, bool webs) {
	uint ofs = 0;
	uint8* mask = nullptr;
	return recvHead(socket, ofs, mask, webs) ? recvLoad(ofs, mask) : nullptr;
}

void Buffer::recvData(nsint socket) {
#ifndef MSG_DONTWAIT
	noblockSocket(socket, true);
#endif
	for (;;) {
		checkOver(dlim);	// allocate next block if full
		long len = recvNow(socket, data + dlim, size - dlim);
		if (len < 0)
			break;
		dlim += len;
	}
#ifndef MSG_DONTWAIT
	noblockSocket(socket, false);
#endif
}

Buffer::Init Buffer::recvConn(nsint socket, bool& webs) {
	uint ofs = 0;
	uint8* mask = nullptr;
	if (!recvHead(socket, ofs, mask, webs))
		return Init::wait;

	switch (uint8 dc = mask ? data[ofs] ^ mask[0] : data[ofs]; Code(dc)) {
	case Code::version: {
		uint8* dat = recvLoad(ofs, mask);
		if (!dat)
			return Init::wait;

		string ver = readText(dat);
		array<const char*, compatibleVersions.size()>::const_iterator it = std::find(compatibleVersions.begin(), compatibleVersions.end(), ver);
		if (clearCur(webs); it == compatibleVersions.end())
			return Init::error;
		return Init::connect; }
	case Code::wsconn: {
		if (webs)
			return Init::error;

		string word = "\r\n\r\n";
		uint8* rend = std::search(data, data + dlim, word.begin(), word.end());
		if (rend == data + dlim)
			return Init::wait;

		rend += pdift(word.length());
		word = "Sec-WebSocket-Key:";
		uint8* pos = std::search(data, rend, word.begin(), word.end());
		if (pos == rend)
			return Init::error;

		pos += pdift(word.length());
		word = "\r\n";
		string key = string(pos, std::search(pos, rend, word.begin(), word.end()));
		string response = "HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: " + encodeBase64(digestSHA1(trim(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) + "\r\n\r\n";
		sendNet(socket, response.c_str(), uint(response.length()));
		eraseFront(uint(rend - data));
		webs = true;
		return Init::cont; }
	default:
		std::cerr << "invalid init code '" << uint(dc) << '\'' << std::endl;
	}
	return Init::error;
}

bool Buffer::recvHead(nsint socket, uint& ofs, uint8*& mask, bool webs) {
	if (webs) {
		if (ofs += wsHeadMin; dlim < ofs)
			return false;
		if ((data[0] & 0xF0) != 0x80)	// TODO: handle fragmentation
			throw Error(msgProtocolError);

		uint plen = data[1] & 0x7F;
		if (plen == 126) {
			if (ofs += sizeof(uint16); dlim < ofs)
				return false;
			plen = read16(data + wsHeadMin);
		} else if (plen == 127) {
			if (ofs += sizeof(uint64); dlim < ofs)
				return false;
			plen = uint(read64(data + wsHeadMin));
		}

		if (data[1] & 0x80) {
			if (ofs += sizeof(uint32); dlim < ofs)
				return false;
			mask = data + ofs - sizeof(uint32);
		}

		if (uint8 opc = data[0] & 0xF; opc != 2) {
			switch (opc) {
			case 8:
				resendWs(socket, ofs, plen, mask);
				throw Error("Connection closed");
			case 9:
				data[0] = 0x8A;
				resendWs(socket, ofs, plen, mask);
				break;
			default:
				throw Error(msgProtocolError);
			}
			return false;
		}
	}
	return dlim >= ofs + dataHeadSize;
}

uint8* Buffer::recvLoad(uint ofs, uint8* mask) {
	if (mask) {
		uint end = *reinterpret_cast<uint16*>(data + ofs + 1);
		for (uint i = 0; i < sizeof(uint16); i++)
			reinterpret_cast<uint8*>(&end)[i] ^= mask[i+1];
		if (end = read16(&end) + ofs; dlim < end)
			return nullptr;
		unmask(mask, ofs, end);
	} else if (dlim < read16(data + ofs + 1))
		return nullptr;
	return data + ofs;
}

void Buffer::resendWs(nsint socket, uint hsize, uint plen, const uint8* mask) {
	uint end = hsize + plen;
	for (checkOver(end); dlim < end; dlim += recvNet(socket, data + dlim, size - dlim));

	if (mask) {
		data[1] &= 0x7F;
		unmask(mask, hsize, end);
		std::copy_n(data + hsize, plen, data + hsize - sizeof(uint32));
	}
	sendData(socket, data, end, false);
	eraseFront(end);
}

uint Buffer::readLoadSize(bool webs) const {
	uint ofs = 0;
	if (webs) {
		ofs += wsHeadMin;
		if (uint plen = data[1] & 0x7F; plen == 126)
			ofs += sizeof(uint16);
		else if (plen == 127)
			ofs += sizeof(uint64);
		if (data[1] & 0x80)
			ofs += sizeof(uint32);
	}
	return ofs + read16(data + ofs + 1);
}

uint Buffer::checkOver(uint end) {
	if (end >= size)	// if end == size then already allocate the next block
		resize(end);
	return end;
}

void Buffer::eraseFront(uint len) {
	if (dlim -= len; size - dlim <= sizeStep)	// if nlim == size - sizeStep then still keep the next block
		std::copy_n(data + len, dlim, data);
	else
		resize(dlim, len);
}

void Buffer::resize(uint lim, uint ofs) {
	uint nsiz = (lim / sizeStep + 1) * sizeStep;
	uint8* ndat = new uint8[nsiz];
	std::copy_n(data + ofs, dlim, ndat);
	delete[] data;
	data = ndat;
	size = nsiz;
}

void Buffer::unmask(const uint8* mask, uint ofs, uint end) {
	for (uint i = ofs; i < end; i++)
		data[i] ^= mask[(i - ofs) % sizeof(uint32)];
}

}
