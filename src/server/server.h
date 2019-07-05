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
	return to_string(year) + ds + ntosPadded(month, 2) + ds + ntosPadded(day, 2) + sep + ntosPadded(hour, 2) + ts + ntosPadded(min, 2) + ts + ntosPadded(sec, 2);
}

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
constexpr uint8 maxPlayers = 2;
constexpr char defaultConfigFile[] = "game.ini";

enum class Code : uint8 {
	none,	// denotes a state of not reading data
	full,	// server full
	setup,	// start setup phase (has first turn info)
	tiles,	// player ready to start match (tile info)
	pieces,	// arrives after ^ and is the last ready signal (piece info)
	move,	// piece move (piece + position info)
	kill,	// piece die (piece info)
	breach,	// fortress state change (breached or not info)
	record,	// turn record data (piece + has attacked or switched info)
	win		// player win (if it's the sender who won info)
};

// variable game properties (shall never be changed after loading)
class Config {
public:
	static constexpr char defaultName[] = "default";
	static constexpr uint16 maxNumPieces = (recvSize - sizeof(Code)) / 2;
	static constexpr float boardWidth = 10.f;
	static constexpr uint8 randomLimit = 100;

	string name;
	uint16 homeWidth, homeHeight;
	uint8 survivalPass;
	uint8 favorLimit;
	uint8 dragonDist;
	bool dragonDiag;
	bool multistage;
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
	
private:
	static constexpr uint16 minWidth = 3;	// TODO: can be lower if no fortress
	static constexpr uint16 maxWidth = 71;
	static constexpr uint16 minHeight = 3;
	static constexpr uint16 maxHeight = 35;
	static constexpr char keywordSize[] = "size";
	static constexpr char keywordSurvival[] = "survival";
	static constexpr char keywordFavors[] = "favors";
	static constexpr char keywordDragonDist[] = "dragon_dist";
	static constexpr char keywordDragonDiag[] = "dragon_diag";
	static constexpr char keywordMultistage[] = "multistage";
	static constexpr char keywordTile[] = "tile_";
	static constexpr char keywordMiddle[] = "middle_";
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
	Config(string name = defaultName);

	void updateValues();
	Config& checkValues();
	void toComData(uint8* data) const;
	void fromComData(const uint8* data);
	uint16 dataSize(Code code) const;
	uint16 tileCompressionEnd() const;
	string toIniText() const;
	void fromIniLine(const string& line);
	string capturersString() const;
	void readCapturers(const string& line);
	template <class T, sizet S> static T calcSum(const array<T, S>& nums, sizet size = S);
private:
	void readSize(const string& line);
	static uint16 floorAmounts(uint16 total, uint16* amts, uint16 limit, sizet ei, uint16 floor = 0);
	template <sizet N, sizet S> static void readAmount(const pairStr& it, const string& word, const array<string, N>& names, array<uint16, S>& amts);
	template <sizet N, sizet S> static void writeAmounts(string& text, const string& word, const array<string, N>& names, const array<uint16, S>& amts);
	void readShift(const string& line);
};

inline uint16 Config::tileCompressionEnd() const {
	return 1 + extraSize / 2 + extraSize % 2;
}

template <class T, sizet S>
T Config::calcSum(const array<T, S>& nums, sizet size) {
	T res = T(0);
	for (sizet i = 0; i < size; i++)
		res += nums[i];
	return res;
}

template <sizet N, sizet S>
void Config::readAmount(const pairStr& it, const string& word, const array<string, N>& names, array<uint16, S>& amts) {
	if (uint8 id = strToEnum<uint8>(names, it.first.substr(0, word.length())); id < amts.size())
		amts[id] = uint16(sstol(it.second));
}

template <sizet N, sizet S>
void Config::writeAmounts(string& text, const string& word, const array<string, N>& names, const array<uint16, S>& amts) {
	for (sizet i = 0; i < amts.size(); i++)
		text += makeIniLine(word + names[i], to_string(amts[i]));
}

vector<Config> loadConfs(const string& file);
void saveConfs(const vector<Config>& confs, const string& file);
void sendRejection(TCPsocket server);
std::default_random_engine createRandomEngine();

}
