#pragma once

#include "utils/text.h"
#ifdef __APPLE__
#include <SDL2_net/SDL_net.h>
#elif defined(__ANDROID__) || defined(_WIN32)
#include <SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#endif

struct NetError {
	const string message;

	NetError(string&& msg);
};

inline NetError::NetError(string&& msg) :
	message(std::move(msg))
{}

namespace Com {

constexpr uint16 defaultPort = 39741;
constexpr uint16 dataHeadSize = sizeof(uint8) + sizeof(uint16);	// code + size
constexpr uint8 roomNameLimit = 64;
constexpr uint8 maxRooms = 64;
constexpr uint wsHeadMin = 2;
constexpr uint wsHeadMax = 2 + sizeof(uint64) + sizeof(uint32);

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

const array<string, tileMax> tileNames = {
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

const array<string, pieceMax> pieceNames = {
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

enum class Code : uint8 {
	version,	// version info
	full,		// server full
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

const array<string, 1> compatibleVersions = {
	commonVersion
};

// variable game properties (shall never be changed after loading)
class Config {
public:
	enum class Survival : uint8 {
		disable,
		finish,
		kill
	};
	static const array<string, uint8(Survival::kill)+1> survivalNames;

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
	void fromComData(uint8* data);
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

string digestSHA1(string str);
string encodeBase64(const string& str);
void sendWaitClose(TCPsocket socket);
void sendRejection(TCPsocket server);
void sendVersion(TCPsocket socket, bool webs);
void sendData(TCPsocket socket, const uint8* data, uint len, bool webs);
string readText(uint8* data);
string readName(const uint8* data);

inline uint64 read64(const void* data) {
	return SDL_SwapBE64(*reinterpret_cast<const uint64*>(data));
}

inline void write64(uint64 val, void* data) {
	*reinterpret_cast<uint64*>(data) = SDL_SwapBE64(val);
}

const umap<Code, uint16> codeSizes = {
	pair(Code::full, dataHeadSize),
	pair(Code::cnrnew, dataHeadSize + uint16(sizeof(uint8))),
	pair(Code::leave, dataHeadSize),
	pair(Code::kick, dataHeadSize),
	pair(Code::hello, dataHeadSize),
	pair(Code::config, dataHeadSize + Config::dataSize),
	pair(Code::start, dataHeadSize + uint16(sizeof(uint8)) + Config::dataSize),
	pair(Code::move, dataHeadSize + uint16(sizeof(uint16) * 2)),
	pair(Code::kill, dataHeadSize + uint16(sizeof(uint16))),
	pair(Code::breach, dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16))),
	pair(Code::record, dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16) * 2))
};

// for sending/receiving network data (mustn't be used for both simultaneously)
class Buffer {
public:
	enum class Init : uint8 {
		wait,
		connect,
		version,
		error
	};

private:
	vector<uint8> data;
	uint dlim;

	static constexpr uint expectedRequestSize = 512;

public:
	Buffer();

	uint8& operator[](uint i);
	uint size() const;
	void clear();

	void push(const initlist<uint8>& lst);
	void push(const initlist<uint16>& vec);
	void push(const string& str);
	void push(uint8 val);
	void push(uint16 val);
	uint preallocate(Code code);			// set head and preallocate space (returns end pos of head)
	uint preallocate(Code code, uint dlen);
	uint pushHead(Code code);				// should only be used for codes with fixed length (returns end pos of head)
	uint pushHead(Code code, uint dlen);	// should only be used for codes with variable length (returns end pos of head)
	uint write(uint8 val, uint pos);
	uint write(uint16 val, uint pos);

	void redirect(TCPsocket socket, uint8* pos, bool sendWebs);	// doesn't clear data
	void send(TCPsocket socket, bool webs, bool clr = true);	// sends all data
	uint8* recv(TCPsocket socket, bool webs);	// returns begin of data or nullptr if nothing to process yet
	Init recvInit(TCPsocket socket, bool& webs);
private:
	bool recvHead(TCPsocket socket, uint& hsize, uint& ofs, uint8*& mask, bool webs);
	uint8* recvLoad(TCPsocket socket, uint hsize, uint ofs, uint8* mask);
	void resendWs(TCPsocket socket, uint hsize, uint plen, const uint8* mask);
	void recvDat(TCPsocket socket, uint size);
	uint checkEnd(uint end);
	void unmask(const uint8* mask, uint i, uint ofs);
};

inline Buffer::Buffer() :
	dlim(0)
{}

inline uint8& Buffer::operator[](uint i) {
	return data[i];
}

inline uint Buffer::size() const {
	return dlim;
}

inline uint Buffer::preallocate(Code code) {
	return preallocate(code, codeSizes.at(code));
}

inline uint Buffer::pushHead(Code code) {
	return pushHead(code, codeSizes.at(code));
}

}
