#pragma once

#include "utils/text.h"

#if defined(_WIN64)
using nsint = uint64;	// should be the same as SOCKET
#elif defined(_WIN32)
using nsint = uint32;
#else
using nsint = int;
#endif
struct addrinfo;

namespace Com {

constexpr char commonVersion[] = "0.4.3";
constexpr uint16 defaultPort = 39741;
constexpr uint16 dataHeadSize = sizeof(uint8) + sizeof(uint16);	// code + size
constexpr uint8 roomNameLimit = 64;
constexpr uint8 maxRooms = 255;
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

enum class Tile : uint8 {
	plains,
	forest,
	mountain,
	water,
	fortress,
	empty
};
constexpr uint8 tileLim = uint8(Tile::fortress);
constexpr uint8 tileMax = uint8(Tile::empty);

constexpr array<const char*, tileMax> tileNames = {
	"plains",
	"forest",
	"mountain",
	"water",
	"fortress"
};

enum class Piece : uint8 {
	ranger,
	spearman,
	crossbowman,	// it's important that crossbowman, catapult, trebuchet come right after another in that order
	catapult,
	trebuchet,
	lancer,
	warhorse,
	elephant,
	dragon,
	throne
};
constexpr uint8 pieceMax = uint8(Piece::throne) + 1;

constexpr array<const char*, pieceMax> pieceNames = {
	"ranger",
	"spearman",
	"crossbowman",
	"catapult",
	"trebuchet",
	"lancer",
	"warhorse",
	"elephant",
	"dragon",
	"throne"
};

enum class Family : uint8 {
	any,
	v4,
	v6
};

static constexpr array<const char*, uint8(Family::v6)+1> familyNames = {
	"any",
	"IPv4",
	"IPv6"
};

enum class Code : uint8 {
	version,	// version info
	rlist,		// list all rooms (amount + flags + names)
	rnew,		// create new room (room name)
	cnrnew,		// confirm new room (yes/no)
	rerase,		// delete a room (name info)
	ropen,		// info whether a room can be accessed (not full + name)
	join,		// player joins room (room name)
	leave,		// guest leaves room
	hgone,		// host left room (room list)
	kick,		// kick player
	hello,		// player has joined
	cnjoin,		// confirm join	(yes/no + config)
	config,		// player sending game config
	start,		// start setup phase (has first turn info)
	setup,		// ais the last ready signal (tile + piece info)
	move,		// piece move (piece + position info)
	kill,		// piece die (piece info)
	breach,		// fortress state change (breached or not info)
	record,		// turn record data (acting piece + protected piece + action info)
	message,	// text
	wsconn = 'G'	// first letter of websocket handshake
};

// variable game properties (shall never be changed after loading)
class Config {
public:
	enum class Survival : uint8 {
		disable,
		finish,
		kill
	};
	static constexpr array<const char*, uint8(Survival::kill)+1> survivalNames = {
		"disable",
		"finish",
		"kill"
	};

	svec2 homeSize;		// neither width nor height shall exceed UINT8_MAX
	uint8 survivalPass;
	Survival survivalMode;
	bool favorLimit;
	uint8 favorMax;
	uint8 dragonDist;
	bool dragonSingle;
	bool dragonDiag;
	array<uint16, tileLim> tileAmounts;
	array<uint16, tileLim> middleAmounts;
	array<uint16, pieceMax> pieceAmounts;
	uint16 winFortress, winThrone;
	array<bool, pieceMax> capturers;
	bool shiftLeft, shiftNear;

	static constexpr char defaultName[] = "default";
	static constexpr uint16 dataSize = sizeof(uint8) * 2 + sizeof(survivalPass) + sizeof(survivalMode) + sizeof(uint8) + sizeof(favorMax) + sizeof(dragonDist) + sizeof(uint8) + sizeof(uint8) + tileLim * sizeof(uint16) + tileLim * sizeof(uint16) + pieceMax * sizeof(uint16) + sizeof(winFortress) + sizeof(winThrone) + pieceMax * sizeof(uint8) + sizeof(uint8) + sizeof(uint8);
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;
	static constexpr svec2 minHomeSize = { 5, 2 };
	static constexpr svec2 maxHomeSize = { 101, 50 };

	Config();

	Config& checkValues();
	void toComData(uint8* data) const;
	void fromComData(const uint8* data);
	string capturersString() const;
	void readCapturers(const string& line);
	uint16 countTiles() const;
	uint16 countMiddles() const;
	uint16 countPieces() const;
	uint16 countFreeTiles() const;
	uint16 countFreeMiddles() const;
	uint16 countFreePieces() const;

private:
	static uint16 floorAmounts(uint16 total, uint16* amts, uint16 limit, uint8 ei, uint16 floor = 0);
	static uint16 ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei);
};

inline uint16 Config::countTiles() const {
	return std::accumulate(tileAmounts.begin(), tileAmounts.end(), uint16(0));
}

inline uint16 Config::countMiddles() const {
	return std::accumulate(middleAmounts.begin(), middleAmounts.end(), uint16(0));
}

inline uint16 Config::countPieces() const {
	return std::accumulate(pieceAmounts.begin(), pieceAmounts.end(), uint16(0));
}

inline uint16 Config::countFreeTiles() const {
	return homeSize.x * homeSize.y - countTiles();
}

inline uint16 Config::countFreeMiddles() const {
	return homeSize.x / 2 - countMiddles();
}

inline uint16 Config::countFreePieces() const {
	return homeSize.x * homeSize.y - countPieces();
}

// universal functions
string digestSHA1(string str);
string encodeBase64(const string& str);
void discardAccept(nsint server);
void sendWaitClose(nsint socket);
void sendVersion(nsint socket, bool webs);
void sendData(nsint socket, const uint8* data, uint len, bool webs);

inline uint16 read16(const void* data) {
	return SDL_SwapBE16(*static_cast<const uint16*>(data));
}

inline void write16(void* data, uint16 val) {
	*static_cast<uint16*>(data) = SDL_SwapBE16(val);
}

inline uint32 read32(const void* data) {
	return SDL_SwapBE32(*static_cast<const uint32*>(data));
}

inline void write32(void* data, uint32 val) {
	*static_cast<uint32*>(data) = SDL_SwapBE32(val);
}

inline uint64 read64(const void* data) {
	return SDL_SwapBE64(*static_cast<const uint64*>(data));
}

inline void write64(void* data, uint64 val) {
	*static_cast<uint64*>(data) = SDL_SwapBE64(val);
}

inline string readText(const uint8* data) {
	return string(reinterpret_cast<const char*>(data + dataHeadSize), read16(data + 1) - dataHeadSize);
}

inline string readName(const uint8* data) {
	return string(reinterpret_cast<const char*>(data + 1), data[0]);
}

// socket functions
nsint connectSocket(const char* addr, uint16 port, Family family);
nsint bindSocket(uint16 port, Family family);
nsint acceptSocket(nsint fd);
bool pollSocket(nsint fd, int timeout = 0);
void noblockSocket(nsint fd, bool noblock);
void sendNet(nsint fd, const void* data, uint size);
uint recvNet(nsint fd, void* data, uint size);
long recvNow(nsint fd, void* data, uint size);
void closeSocket(nsint& fd);

const umap<Code, uint16> codeSizes = {
	pair(Code::cnrnew, dataHeadSize + uint16(sizeof(uint8))),
	pair(Code::leave, dataHeadSize),
	pair(Code::kick, dataHeadSize),
	pair(Code::hello, dataHeadSize),
	pair(Code::config, dataHeadSize + Config::dataSize),
	pair(Code::start, dataHeadSize + uint16(sizeof(uint8)) + Config::dataSize),
	pair(Code::move, dataHeadSize + uint16(sizeof(uint16) * 2)),
	pair(Code::kill, dataHeadSize + uint16(sizeof(uint16))),
	pair(Code::breach, dataHeadSize + uint16(sizeof(uint16))),
	pair(Code::record, dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16) * 2))
};

// network error
struct Error {
	const string message;

	Error(string&& msg);
};

inline Error::Error(string&& msg) :
	message(std::move(msg))
{}

// for sending/receiving network data (mustn't be used for both simultaneously)
class Buffer {
public:
	enum class Init : uint8 {
		wait,
		connect,
		cont,
		error
	};

private:
	static constexpr uint sizeStep = 512;

	uint8* data;
	uint size, dlim;

public:
	Buffer();
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&& b);
	~Buffer();

	Buffer& operator=(const Buffer&) = delete;
	Buffer& operator=(Buffer&& b);
	uint8& operator[](uint i);
	uint getDlim() const;
	void clear();				// delete all
	void clearCur(bool webs);	// delete first chunk

	void push(initlist<uint8> lst);
	void push(initlist<uint16> lst);
	void push(const string& str);
	void push(uint8 val);
	void push(uint16 val);
	uint allocate(Code code);			// set head and preallocate space (returns end pos of head)
	uint allocate(Code code, uint16 dlen);
	uint pushHead(Code code);				// should only be used for codes with fixed length (returns end pos of head)
	uint pushHead(Code code, uint16 dlen);	// should only be used for codes with variable length (returns end pos of head)
	uint write(uint8 val, uint pos);
	uint write(uint16 val, uint pos);

	void redirect(nsint socket, uint8* pos, bool sendWebs);	// doesn't clear data
	void send(nsint socket, bool webs, bool clr = true);	// sends and clears all data
	uint8* recv(nsint socket, bool webs);	// returns begin of data or nullptr if nothing to process yet
	void recvData(nsint socket);	// load recv data into buffer (call once before iterating over recv()
	Init recvConn(nsint socket, bool& webs);
private:
	bool recvHead(nsint socket, uint& ofs, uint8*& mask, bool webs);
	uint8* recvLoad(uint ofs, uint8* mask);
	void resendWs(nsint socket, uint hsize, uint plen, const uint8* mask);
	uint readLoadSize(bool webs) const;
	uint checkOver(uint end);
	void eraseFront(uint len);
	void resize(uint lim, uint ofs = 0);
	void unmask(const uint8* mask, uint ofs, uint end);
};

inline Buffer::~Buffer() {
	delete[] data;
}

inline uint8& Buffer::operator[](uint i) {
	return data[i];
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

inline uint Buffer::allocate(Code code) {
	return allocate(code, codeSizes.at(code));
}

inline uint Buffer::pushHead(Code code) {
	return pushHead(code, codeSizes.at(code));
}

}
