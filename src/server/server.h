#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL2/SDL_net.h>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <random>

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

namespace Server {
	constexpr uint16 defaultPort = 39741;
	constexpr sizet maxPlayers = 2;
	constexpr sizet maxSockets = maxPlayers + 1;
	constexpr uint32 checkTimeout = 500;
	constexpr sizet bufSiz = 128;
}

enum class NetCode : uint8 {
	full,
	setup,
	ready,
	move
};

inline int sendSingle(TCPsocket socket, NetCode code) {
	return SDLNet_TCP_Send(socket, &code, sizeof(code));
}

inline std::default_random_engine createRandomEngine() {
	std::default_random_engine randGen;
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(std::time(nullptr)));
	}
	return randGen;
}
