#pragma once

#include "utils/text.h"
#ifdef __APPLE__
#include <SDL2_net/SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#endif
#include <iostream>
#include <random>

// get rid of SDL's main
#ifdef main
#undef main
#endif

namespace Com {

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

constexpr uint16 defaultPort = 39741;
constexpr uint16 recvSize = 1380;

constexpr array<uint16 (*)(uint16, uint16), 4> adjacentStraight = {
	[](uint16 id, uint16 lim) -> uint16 { return id / lim ? id - lim : UINT16_MAX; },			// up
	[](uint16 id, uint16 lim) -> uint16 { return id % lim ? id - 1 : UINT16_MAX; },				// left
	[](uint16 id, uint16 lim) -> uint16 { return id % lim != lim - 1 ? id + 1 : UINT16_MAX; },	// right
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 ? id + lim : UINT16_MAX; }	// down
};

constexpr array<uint16 (*)(uint16, uint16), 8> adjacentFull = {
	[](uint16 id, uint16 lim) -> uint16 { return id / lim && id % lim ? id - lim - 1 : UINT16_MAX; },						// left up
	[](uint16 id, uint16 lim) -> uint16 { return id / lim ? id - lim : UINT16_MAX; },										// up
	[](uint16 id, uint16 lim) -> uint16 { return id / lim && id % lim != lim - 1  ? id - lim + 1 : UINT16_MAX; },			// right up
	[](uint16 id, uint16 lim) -> uint16 { return id % lim ? id - 1 : UINT16_MAX; },											// left
	[](uint16 id, uint16 lim) -> uint16 { return id % lim != lim - 1 ? id + 1 : UINT16_MAX; },								// right
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 && id % lim ? id + lim - 1 : UINT16_MAX; },			// left down
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 ? id + lim : UINT16_MAX; },							// down
	[](uint16 id, uint16 lim) -> uint16 { return id / lim != lim - 1 && id % lim != lim - 1 ? id + lim + 1 : UINT16_MAX; }	// right down
};

enum class Code : uint8 {
	none,	// denotes a state of not reading data
	full,	// server full
	setup,	// start setup phase (has first turn info)
	tiles,	// player ready to start match (tile info)
	pieces,	// arrives after ^ and is the last ready signal (piece info)
	move,	// piece move (piece + position info)
	kill,	// piece die (piece info)
	breach,	// fortress state change (breached or not info)
	favor,	// set favor (tile info)
	record,	// turn record data (piece + has attacked or switched info)
	win		// player win (if it's the sender who won info)
};

// variable game properties (shall never be changed after loading)
class Config {
public:
	static constexpr char defaultName[] = "default";

	string name;
	uint16 homeWidth, homeHeight;
	array<uint16, tileMax> tileAmounts;
	array<uint16, pieceMax> pieceAmounts;
	uint16 winFortress, winThrone;
	array<bool, pieceMax> capturers;
	bool shiftLeft, shiftNear;		// TODO: use

	uint16 extraSize;	// home size + mid size
	uint16 boardSize;	// total number of tiles
	uint16 numTiles;	// number of own homeland tiles (must be calculated using tileAmounts)
	uint16 numPieces;	// number of own pieces (must be calculated using pieceAmounts)
	uint16 piecesSize;	// total number of pieces

private:
	static constexpr uint16 minWidth = 9;
	static constexpr uint16 maxWidth = 65;
	static constexpr uint16 minHeight = 3;
	static constexpr uint16 maxHeight = 32;
	static constexpr char keywordSize[] = "size";
	static constexpr char keywordTile[] = "tile_";
	static constexpr char keywordPiece[] = "piece_";
	static constexpr char keywordWinFortress[] = "win_fortresses";
	static constexpr char keywordWinThrone[] = "win_thrones";
	static constexpr char keywordCapturers[] = "capturers";
	static constexpr char keywordShift[] = "middle_shift";
	static constexpr char keywordLeft[] = "left";
	static constexpr char keywordRight[] = "right";
	static constexpr char keywordNear[] = "near";
	static constexpr char keywordFar[] = "far";

public:
	Config(const string& name = defaultName);

	void updateValues();
	Config& checkValues();
	vector<uint8> toComData() const;
	void fromComData(const uint8* data);
	uint16 dataSize(Code code) const;
	uint16 tileCompressionEnd() const;
	string toIniText() const;
	void fromIniLine(const string& line);
private:
	void readSize(const string& line);
	static uint16 makeOdd(uint16 val);
	template <sizet S, class F> static void matchAmounts(uint16& total, array<uint16, S>& amts, uint16 limit, uint8 si, uint8 ei, int8 mov, F comp);
	template <sizet S> static void readAmount(const pairStr& it, const string& word, const array<string, S>& names, array<uint16, S>& amts);
	template <sizet S> static void writeAmount(string& text, const string& word, const array<string, S>& names, const array<uint16, S>& amts);
	void readCapturers(const string& line);
	string capturersString() const;
	void readShift(const string& line);
};

inline uint16 Config::tileCompressionEnd() const {
	return 1 + extraSize / 2 + extraSize % 2;
}

inline uint16 Config::makeOdd(uint16 val) {
	return val % 2 ? val : val - 1;
}

template <sizet S, class F>
void Config::matchAmounts(uint16& total, array<uint16, S>& amts, uint16 limit, uint8 si, uint8 ei, int8 mov, F comp) {
	for (uint8 i = si; comp(total, limit); total += uint16(mov)) {
		amts[i] += uint16(mov);
		if (i += uint8(mov); i >= ei)
			i = si;
	}
}

template <sizet S>
void Config::readAmount(const pairStr& it, const string& word, const array<string, S>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, it.first.substr(0, word.length())); id < amts.size())
		amts[id] = uint16(sstol(it.second));
}

template <sizet S>
void Config::writeAmount(string& text, const string& word, const array<string, S>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); i++)
		text += makeIniLine(word + names[i], to_string(amts[i]));
}

std::default_random_engine createRandomEngine();

}
