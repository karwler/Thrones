#pragma once

#include "utils/text.h"
#ifdef __APPLE__
#include <SDL2_net/SDL_net.h>
#elif defined(__ANDROID__) || defined(_WIN32)
#include <SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#endif

struct Date {
	uint8 sec, min, hour;
	uint8 day, month;
	uint8 wday;
	int16 year;

	Date(uint8 second, uint8 minute, uint8 hour, uint8 day, uint8 month, int16 year, uint8 weekDay);

	static Date now();
	string toString(char ts = ':', char sep = ' ', char ds = '.') const;
};

inline string Date::toString(char ts, char sep, char ds) const {
	return toStr(year) + ds + ntosPadded(month, 2) + ds + ntosPadded(day, 2) + sep + ntosPadded(hour, 2) + ts + ntosPadded(min, 2) + ts + ntosPadded(sec, 2);
}

namespace Com {

constexpr uint16 defaultPort = 39741;
constexpr uint16 dataHeadSize = sizeof(uint8) + sizeof(uint16);	// code + size
constexpr uint8 roomNameLimit = 64;
constexpr uint8 maxRooms = 64;

enum class Tile : uint8 {
	plains,
	forest,
	mountain,
	water,
	fortress,
	empty
};
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
	record		// turn record data (piece + has attacked or switched info)
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
	array<uint16, tileMax> tileAmounts;
	array<uint16, tileMax-1> middleAmounts;
	array<uint16, pieceMax> pieceAmounts;
	uint16 winFortress, winThrone;
	array<bool, pieceMax> capturers;
	bool shiftLeft, shiftNear;
	
	static constexpr char defaultName[] = "default";
	static constexpr uint16 dataSize = sizeof(uint8) * 2 + sizeof(survivalPass) + sizeof(survivalMode) + sizeof(uint8) + sizeof(favorMax) + sizeof(dragonDist) + sizeof(uint8) + sizeof(uint8) + tileMax * sizeof(uint16) + (tileMax - 1) * sizeof(uint16) + pieceMax * sizeof(uint16) + sizeof(winFortress) + sizeof(winThrone) + pieceMax * sizeof(uint8) + sizeof(uint8) + sizeof(uint8);
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;
	static constexpr svec2 minHomeSize = { 5, 2 };
	static constexpr svec2 maxHomeSize = { 101, 50 };

public:
	Config();

	Config& checkValues();
	void toComData(uint8* data) const;
	void fromComData(uint8* data);
	string capturersString() const;
	void readCapturers(const string& line);
	uint16 countTilesNonFort() const;
	uint16 countMiddles() const;
	uint16 countPieces() const;
	uint16 countFreeTiles() const;
	uint16 countFreeMiddles() const;
	uint16 countFreePieces() const;

private:
	static uint16 floorAmounts(uint16 total, uint16* amts, uint16 limit, uint8 ei, uint16 floor = 0);
	static uint16 ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei);
};

inline uint16 Com::Config::countTilesNonFort() const {
	return std::accumulate(tileAmounts.begin(), tileAmounts.end() - 1, uint16(0));
}

inline uint16 Config::countMiddles() const {
	return std::accumulate(middleAmounts.begin(), middleAmounts.end(), uint16(0));
}

inline uint16 Config::countPieces() const {
	return std::accumulate(pieceAmounts.begin(), pieceAmounts.end(), uint16(0));
}

inline uint16 Config::countFreeTiles() const {
	return homeSize.x * homeSize.y - countTilesNonFort();
}

inline uint16 Config::countFreeMiddles() const {
	return homeSize.x / 2 - countMiddles();
}

inline uint16 Config::countFreePieces() const {
	return homeSize.x * homeSize.y - countPieces();
}

int sendRejection(TCPsocket server);
string readName(const uint8* data);
string readVersion(uint8* data);

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
	pair(Code::record, dataHeadSize + uint16(sizeof(uint8) + sizeof(uint16)))
};

}

// for sending/receiving network data (mustn't be used for both simultaneously)
class Buffer {
private:
	vector<uint8> data;
	uint dpos;

public:
	Buffer();

	uint8& operator[](uint i);
	uint size() const;
	void clear();

	void push(const vector<uint8>& vec);
	void push(const vector<uint16>& vec);
	void push(const string& str);
	void push(const uint8* vec, uint len);
	void push(uint8 val);
	void push(uint16 val);
	uint preallocate(Com::Code code);			// set head and preallocate space (returns end pos of head)
	uint preallocate(Com::Code code, uint dlen);
	uint pushHead(Com::Code code);				// should only be used for codes with fixed length (returns end pos of head)
	uint pushHead(Com::Code code, uint dlen);	// should only be used for codes with variable length (returns end pos of head)
	uint write(uint8 val, uint pos);
	uint write(uint16 val, uint pos);

	const char* send(TCPsocket socket);				// sends all data and clears
	const char* send(TCPsocket socket, uint cnt);	// returns error code or nullptr on success (doesn't clear data)
	template <class F> const char* recv(TCPsocket socket, F proc);

private:
	uint checkEnd(uint end);
};

inline uint8& Buffer::operator[](uint i) {
	return data[i];
}

inline uint Buffer::size() const {
	return dpos;
}

inline void Buffer::push(const vector<uint8>& vec) {
	push(vec.data(), uint(vec.size()));
}

inline void Buffer::push(const string& str) {
	push(reinterpret_cast<const uint8*>(str.c_str()), uint(str.length()));
}

inline uint Buffer::preallocate(Com::Code code) {
	return preallocate(code, Com::codeSizes.at(code));
}

inline uint Buffer::pushHead(Com::Code code) {
	return pushHead(code, Com::codeSizes.at(code));
}

inline const char* Buffer::send(TCPsocket socket, uint cnt) {
	return SDLNet_TCP_Send(socket, data.data(), int(cnt)) != int(cnt) ? SDLNet_GetError() : nullptr;
}

template <class F>
inline const char* Buffer::recv(TCPsocket socket, F proc) {
	if (!SDLNet_SocketReady(socket))
		return nullptr;
	if (dpos < Com::dataHeadSize) {
		checkEnd(Com::dataHeadSize);
		int len = SDLNet_TCP_Recv(socket, data.data() + dpos, int(Com::dataHeadSize - dpos));
		if (len <= 0)
			return SDLNet_GetError();
		if (dpos += uint(len); dpos < Com::dataHeadSize)
			return nullptr;
	}

	uint16 end = SDLNet_Read16(data.data() + 1);
	if (end > dpos) {
		if (!SDLNet_SocketReady(socket))
			return nullptr;
		checkEnd(end);
		int len = SDLNet_TCP_Recv(socket, data.data() + dpos, int(end - dpos));
		if (len <= 0)
			return SDLNet_GetError();
		dpos += uint(len);
	}
	if (dpos >= end) {
		proc(data.data());
		data.clear();
		dpos = 0;
	}
	return nullptr;
}
