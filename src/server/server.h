#pragma once

#include "utils/text.h"
#ifdef __APPLE__
#include <SDL2_net/SDL_net.h>
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
constexpr uint16 dataMaxSize = 2048;
constexpr uint8 roomNameLimit = 32;
constexpr uint8 maxRooms = (dataMaxSize - dataHeadSize - sizeof(uint8)) / (sizeof(uint8) * 2 + roomNameLimit);
constexpr uint8 maxPlayers = maxRooms * 2;

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
	full,	// server full
	rlist,	// list all rooms (amount + flags + names)
	rnew,	// create new room (room name)
	cnrnew,	// confirm new room (yes/no)
	rerase,	// delete a room (name info)
	ropen,	// info whether a room can be accessed (not full + name)
	join,	// player joins room (room name)
	leave,	// player leaves room
	hello,	// player has joined
	cnjoin,	// confirm join	(yes/no + config)
	config,	// player sending game config
	start,	// start setup phase (has first turn info)
	tiles,	// player ready to start match (tile info)
	pieces,	// arrives after ^ and is the last ready signal (piece info)
	move,	// piece move (piece + position info)
	kill,	// piece die (piece info)
	breach,	// fortress state change (breached or not info)
	record	// turn record data (piece + has attacked or switched info)
};

// variable game properties (shall never be changed after loading)
class Config {
public:
	nvec2 homeSize;
	uint8 survivalPass;
	uint8 favorLimit;
	uint8 dragonDist;
	bool dragonDiag;
	bool multistage;
	bool survivalKill;
	array<uint16, tileMax> tileAmounts;
	array<uint16, tileMax-1> middleAmounts;
	array<uint16, pieceMax> pieceAmounts;
	uint16 winFortress, winThrone;
	array<bool, pieceMax> capturers;
	bool shiftLeft, shiftNear;

	uint16 extraSize;	// home size + mid size
	uint16 boardSize;	// total number of tiles
	uint16 numTiles;	// number of own homeland tiles (must be calculated using tileAmounts)
	uint16 numPieces;	// number of own pieces (must be calculated using pieceAmounts)
	uint16 piecesSize;	// total number of pieces
	float objectSize;	// width and height of a tile/piece
	
	static constexpr char defaultName[] = "default";
	static constexpr uint16 maxNumPieces = (dataMaxSize - dataHeadSize) / 2;
	static constexpr uint16 dataSize = sizeof(homeSize) + sizeof(survivalPass) + sizeof(favorLimit) + sizeof(dragonDist) + sizeof(uint8) + sizeof(uint8) + sizeof(uint8) + tileMax * sizeof(uint16) + (tileMax - 1) * sizeof(uint16) + pieceMax * sizeof(uint16) + sizeof(winFortress) + sizeof(winThrone) + pieceMax * sizeof(uint8) + sizeof(uint8) + sizeof(uint8);
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;
private:
	static constexpr nvec2 minHomeSize = { 4, 2 };
	static constexpr nvec2 maxHomeSize = { 89, 44 };

public:
	Config();

	void updateValues();
	Config& checkValues();
	void toComData(uint8* data) const;
	void fromComData(uint8* data);
	uint16 tileCompressionSize() const;
	uint16 pieceDataSize() const;
	string capturersString() const;
	void readCapturers(const string& line);

	template <class T, sizet S> static T calcSum(const array<T, S>& nums, sizet size = S);
private:
	static uint16 floorAmounts(uint16 total, uint16* amts, uint16 limit, sizet ei, uint16 floor = 0);
};

inline uint16 Config::tileCompressionSize() const {
	return dataHeadSize + extraSize / 2 + extraSize % 2;
}

inline uint16 Config::pieceDataSize() const {
	return dataHeadSize + numPieces * sizeof(uint16);
}

template <class T, sizet S>
T Config::calcSum(const array<T, S>& nums, sizet size) {
	T res = T(0);
	for (sizet i = 0; i < size; i++)
		res += nums[i];
	return res;
}

int sendRejection(TCPsocket server);
string readName(const uint8* data);

constexpr array<uint16, uint8(Code::record)+1> codeSizes = {	// 0 means variable length
	dataHeadSize,					// full
	0,								// room list (room count * (name.len + name))
	0,								// room create (name.len + name)
	dataHeadSize + sizeof(uint8),	// confirm room create
	0,								// room delete (name.len + name)
	0,								// room is open (state + name.len + name)
	0,								// join room (name.len + name)
	dataHeadSize,					// leave room
	dataHeadSize,					// player in room
	dataHeadSize + Config::dataSize,					// confirm join room
	dataHeadSize + Config::dataSize,					// game config
	dataHeadSize + sizeof(uint8) + Config::dataSize,	// game start
	0,													// tile setup (tiles.num / 2)
	0,													// piece setup (pieces.num * 2)
	dataHeadSize + sizeof(uint16) * 2,					// piece move
	dataHeadSize + sizeof(uint16),						// piece kill
	dataHeadSize + sizeof(uint8) + sizeof(uint16),		// fortress breach
	dataHeadSize + sizeof(uint8) + sizeof(uint16) * 2	// turn record
};

}

// for making sending stuff easier
struct Buffer {
	uint16 size;	// shall not exceed bufSize, is never actually checked though
	uint8 data[Com::dataMaxSize];

	Buffer();

	void push(const vector<uint8>& vec);
	void push(const vector<uint16>& vec);
	void push(const string& str);
	void push(const uint8* vec, uint16 len);
	void push(uint8 val);
	void push(uint16 val);
	void pushHead(Com::Code code);			// shouldn't be used for codes with variable length
	uint16 writeHead(Com::Code code, uint16 clen, uint16 pos = 0);	// returns new pos
	const char* send(TCPsocket socket);		// returns error code or nullptr on success
	int recv(TCPsocket socket);
	template <class F, class... A> void processRecv(int len, F finish, A... args);
};

inline Buffer::Buffer() :
	size(0)
{}

inline void Buffer::push(const vector<uint8>& vec) {
	push(vec.data(), uint16(vec.size()));
}

inline void Buffer::push(const string& str) {
	push(reinterpret_cast<const uint8*>(str.c_str()), uint16(str.length()));
}

inline void Buffer::push(uint8 val) {
	data[size++] = val;
}

inline void Buffer::pushHead(Com::Code code) {
	size = writeHead(code, Com::codeSizes[uint8(code)], size);
}

inline const char* Buffer::send(TCPsocket socket) {
	return SDLNet_TCP_Send(socket, data, size) == size ? reinterpret_cast<const char*>(size = 0) : SDLNet_GetError();
}

inline int Buffer::recv(TCPsocket socket) {
	return SDLNet_TCP_Recv(socket, data + size, Com::dataMaxSize - size);
}

template <class F, class... A>
void Buffer::processRecv(int len, F finish, A... args) {
	uint8* dtp = data;
	do {
		if (int await = SDLNet_Read16(dtp + 1), left = await - size; size + len >= Com::dataHeadSize && len >= left) {
			finish(dtp, args...);
			len -= left;
			dtp += left;
			size = 0;
		} else {
			if (size += len; dtp != data)
				std::copy_n(dtp, size, data);
			len = 0;
		}
	} while (len);
}
