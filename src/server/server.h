#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL2/SDL_net.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <random>
#include <string>

// get rid of SDL's main
#ifdef main
#undef main
#endif

using std::array;
using std::initializer_list;
using std::string;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using wchar = wchar_t;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using sizet = size_t;
using pdift = ptrdiff_t;

namespace Com {
	const string loopback = "127.0.0.1";
	constexpr uint16 defaultPort = 39741;
	constexpr sizet maxPlayers = 2;
	constexpr sizet maxSockets = maxPlayers + 1;
	constexpr uint32 checkTimeout = 500;
	constexpr sizet recvSize = 128;

	constexpr int8 boardLength = 9;
	constexpr int8 homeHeight = 4;
	constexpr uint8 boardSize = boardLength * (homeHeight * 2 + 1);
	constexpr uint8 numTiles = boardLength * homeHeight;	// amount of tiles on homeland (should equate to the sum of num<tile_type> + 1)
	constexpr uint8 numPieces = 15;							// amount of one player's pieces
	constexpr uint8 piecesSize = numPieces * 2;

	constexpr array<uint8 (*)(uint8), 4> adjacentStraight = {
		[](uint8 id) -> uint8 { return id / boardLength ? id - boardLength : UINT8_MAX; },						// up
		[](uint8 id) -> uint8 { return id % boardLength ? id - 1 : UINT8_MAX; },								// left
		[](uint8 id) -> uint8 { return id % boardLength != boardLength - 1 ? id + 1 : UINT8_MAX; },				// right
		[](uint8 id) -> uint8 { return id / boardLength != boardLength - 1 ? id + boardLength : UINT8_MAX; }	// down
	};
	constexpr array<uint8 (*)(uint8), 8> adjacentFull = {
		[](uint8 id) -> uint8 { return id / boardLength && id % boardLength ? id - boardLength - 1 : UINT8_MAX; },						// left up
		[](uint8 id) -> uint8 { return id / boardLength ? id - boardLength : UINT8_MAX; },												// up
		[](uint8 id) -> uint8 { return id / boardLength && id % boardLength != boardLength - 1  ? id - boardLength + 1 : UINT8_MAX; },	// right up
		[](uint8 id) -> uint8 { return id % boardLength ? id - 1 : UINT8_MAX; },						// left
		[](uint8 id) -> uint8 { return id % boardLength != boardLength - 1 ? id + 1 : UINT8_MAX; },		// right
		[](uint8 id) -> uint8 { return id / boardLength != boardLength - 1 && id % boardLength ? id + boardLength - 1 : UINT8_MAX; },					// left down
		[](uint8 id) -> uint8 { return id / boardLength != boardLength - 1 ? id + boardLength : UINT8_MAX; },											// down
		[](uint8 id) -> uint8 { return id / boardLength != boardLength - 1 && id % boardLength != boardLength - 1 ? id + boardLength + 1 : UINT8_MAX; }	// right down
	};

	enum class Code : uint8 {
		full,	// server full
		setup,	// start setup phase (has first turn info)
		ready,	// player ready to start match (tile and piece position info)
		move,	// piece move (piece + position info)
		kill,	// piece die (piece info)
		ruin,	// fortress state change (ruined or not info)
		record,	// turn record data (piece + has attacked or switched info)
		win		// player win (if it's the sender who won info)
	};
}

std::default_random_engine createRandomEngine();
