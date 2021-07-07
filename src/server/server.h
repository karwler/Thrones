#pragma once

#include "utils/alias.h"
#include <stdexcept>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define poll WSAPoll

using nsint = SOCKET;
using sendlen = int;
using socklent = int;
#else
using nsint = int;
using sendlen = ssize_t;
using socklent = socklen_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef POLLRDHUP	// ignore if not present
#define POLLRDHUP 0
#endif

constexpr short polleventsDisconnect = POLLERR | POLLHUP | POLLNVAL | POLLRDHUP;

namespace Com {

constexpr char commonVersion[] = "0.5.3";
constexpr char defaultPort[] = "39741";
constexpr uint16 dataHeadSize = sizeof(uint8) + sizeof(uint16);	// code + size
constexpr uint8 roomNameLimit = 63;
constexpr uint wsHeadMin = 2;
constexpr uint wsHeadMax = 2 + sizeof(uint64) + sizeof(uint32);

constexpr char msgAcceptFail[] = "Failed to accept";
constexpr char msgBindFail[] = "Failed to bind socket";
constexpr char msgConnectionFail[] = "Failed to connect";
constexpr char msgConnectionLost[] = "Connection lost";
constexpr char msgIoctlFail[] = "Failed to ioctl";
constexpr char msgPollFail[] = "Failed to poll";
constexpr char msgProtocolError[] = "Protocol error";
constexpr char msgResolveFail[] = "Failed to resolve host";
constexpr char msgWinsockFail[] = "failed to initialize Winsock 2.2";

constexpr array<const char*, 1> compatibleVersions = {
	commonVersion
};

enum class Code : uint8 {
	version,	// version info
	full,		// server full
	rlist,		// list all rooms (pid + amount + flags + names)
	rnew,		// create new room (room name)
	cnrnew,		// confirm new room (CncrnewCode)
	rerase,		// delete a room (name info)
	ropen,		// info whether a room can be accessed (not full + name)
	glmessage,	// global message
	join,		// player joins room (room name)
	leave,		// player leaves room
	thost,		// transfer host
	kick,		// kick player (if received, data is the same as with rlist)
	hello,		// player has joined
	cnjoin,		// confirm join	(yes/no + config if yes)
	config,		// player sending game config
	start,		// start setup phase (first turn info + config)
	setup,		// is the last ready signal (tile + piece amounts + piece info)
	move,		// piece move (piece + position info)
	kill,		// piece die (piece info)
	breach,		// fortress state change (tile + breached or not info)
	tile,		// tile type change (tile + type)
	record,		// turn record data (info + last actor + protected pieces)
	message,	// local message
	wsconn = 'G'	// first letter of websocket handshake
};

enum class CncrnewCode : uint8 {
	ok,
	full,
	taken,
	length
};

const umap<Code, uint16> codeSizes = {
	pair(Code::full, dataHeadSize),
	pair(Code::cnrnew, dataHeadSize + uint16(sizeof(uint8))),
	pair(Code::leave, dataHeadSize),
	pair(Code::thost, dataHeadSize),
	pair(Code::kick, dataHeadSize),
	pair(Code::hello, dataHeadSize),
	pair(Code::move, dataHeadSize + uint16(sizeof(uint16) * 2)),
	pair(Code::kill, dataHeadSize + uint16(sizeof(uint16))),
	pair(Code::breach, dataHeadSize + uint16(sizeof(uint16) + sizeof(uint8))),
	pair(Code::tile, dataHeadSize + uint16(sizeof(uint16) + sizeof(uint8)))
};

// socket functions
addrinfo* resolveAddress(const char* addr, const char* port, int family);
nsint createSocket(int family, int reuseaddr, int nodelay = 1);
nsint bindSocket(const char* port, int family);
nsint acceptSocket(nsint fd);
int noblockSocket(nsint fd, bool noblock);
void closeSocket(nsint& fd);

inline void closeSocketV(nsint fd) {
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
}

// universal functions
void sendWaitClose(nsint socket);
void sendVersion(nsint socket, bool webs);
void sendRejection(nsint server);
void sendData(nsint socket, const uint8* data, uint len, bool webs);
string digestSha1(string str);
string encodeBase64(const string& str);

inline uint16 read16(const void* data) {
	return SDL_SwapBE16(readMem<uint16>(data));
}

inline void* write16(void* data, uint16 val) {
	return writeMem(data, SDL_SwapBE16(val));
}

inline uint32 read32(const void* data) {
	return SDL_SwapBE32(readMem<uint32>(data));
}

inline void* write32(void* data, uint32 val) {
	return writeMem(data, SDL_SwapBE32(val));
}

inline uint64 read64(const void* data) {
	return SDL_SwapBE64(readMem<uint64>(data));
}

inline void* write64(void* data, uint64 val) {
	return writeMem(data, SDL_SwapBE64(val));
}

inline string readText(const uint8* data) {
	return string(reinterpret_cast<const char*>(data + dataHeadSize), read16(data + 1) - dataHeadSize);
}

inline string readName(const uint8* data, uint8 nmask = 0xFF) {
	return string(reinterpret_cast<const char*>(data + 1), data[0] & nmask);
}

// network error
struct Error : std::runtime_error {
	using std::runtime_error::runtime_error;
};

// for sending/receiving network data (mustn't be used for both simultaneously)
class Buffer {
public:
	enum class Init : uint8 {
		wait,
		connect,
		cont,
		version,
		error
	};

private:
	static constexpr uint sizeStep = 512;

	uptr<uint8[]> data;
	uint size = sizeStep;
	uint dlim = 0;

public:
	Buffer();

	uint8& operator[](uint i);
	uint8 operator[](uint i) const;
	const uint8* getData() const;
	uint getDlim() const;
	void clear();				// delete all
	void clearCur(bool webs);	// delete first chunk

	uint pushHead(Code code);				// should only be used for codes with fixed length (returns end position of head)
	uint pushHead(Code code, uint16 dlen);	// should only be used for codes with variable length (returns end position of head)
	uint allocate(Code code);				// set head and allocate space in advance (returns end position of head)
	uint allocate(Code code, uint16 dlen);
	void push(uint8 val);
	void push(uint16 val);
	void push(uint32 val);
	void push(uint64 val);
	void push(initlist<uint8> lst);
	void push(initlist<uint16> lst);
	void push(initlist<uint32> lst);
	void push(initlist<uint64> lst);
	void push(const string& str);
	uint write(uint8 val, uint pos);
	uint write(uint16 val, uint pos);
	uint write(uint32 val, uint pos);
	uint write(uint64 val, uint pos);

	void redirect(nsint socket, uint8* pos, bool sendWebs);	// doesn't clear data
	void send(nsint socket, bool webs, bool clr = true);	// sends and clears all data
	uint8* recv(nsint socket, bool webs);	// returns begin of data or nullptr if nothing to process yet
	bool recvData(nsint socket);	// load recv data into buffer; returns true if the connection closed (call once before iterating over recv()
	Init recvConn(nsint socket, bool& webs);
private:
	bool recvHead(nsint socket, uint& ofs, uint8*& mask, bool webs);
	uint8* recvLoad(uint ofs, const uint8* mask);
	void resendWs(nsint socket, uint hsize, uint plen, const uint8* mask);
	uint readLoadSize(bool webs) const;
	uint checkOver(uint end);
	void eraseFront(uint len);
	void resize(uint lim, uint ofs = 0);
	void unmask(const uint8* mask, uint ofs, uint end);
	template <class T, class F> void pushNumber(T val, F writer);
	template <class T, class F> void pushNumberList(initlist<T> lst, F writer);
	template <class T> void pushRaw(const T& vec);
	template <class T, class F> uint writeNumber(T val, uint pos, F writer);
};

inline Buffer::Buffer() :
	data(std::make_unique<uint8[]>(sizeStep))
{}

inline uint8& Buffer::operator[](uint i) {
	return data[i];
}

inline uint8 Buffer::operator[](uint i) const {
	return data[i];
}

inline const uint8* Buffer::getData() const {
	return data.get();
}

inline uint Buffer::getDlim() const {
	return dlim;
}

inline void Buffer::clear() {
	eraseFront(dlim);
}

inline void Buffer::clearCur(bool webs) {
	eraseFront(readLoadSize(webs));
}

inline uint Buffer::pushHead(Code code) {
	return pushHead(code, codeSizes.at(code));
}

inline uint Buffer::allocate(Code code) {
	return allocate(code, codeSizes.at(code));
}

}
