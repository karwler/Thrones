#include "server.h"
#include <signal.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
using namespace Server;

enum class WaitResult : uint8 {
	ready,
	retry,
	exit
};

static bool running = true;

static int connectionFail(const char* msg, int ret = -1) {
	std::cerr << msg << '\n' << SDLNet_GetError() << std::endl;
	SDLNet_Quit();
	SDL_Quit();
	return ret;
}

static bool quitting() {
#ifdef _WIN32
	return _kbhit() && _getch() == 'q';
#else
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	timeval tv = {0, 0};
	return select(1, &fds, nullptr, nullptr, &tv) && getchar() == 'q';
#endif
}

static sizet findFirstFree(const TCPsocket* sockets) {
	for (sizet i = 0; i < maxPlayers; i++)
		if (!sockets[i])
			return i;
	return SIZE_MAX;
}

static void disconnectSocket(SDLNet_SocketSet sockets, TCPsocket& client) {
	if (client) {
		SDLNet_TCP_DelSocket(sockets, client);
		SDLNet_TCP_Close(client);
		client = nullptr;
	}
}

static void disconnectPlayers(SDLNet_SocketSet sockets, TCPsocket* players) {
	std::cout << "disconnecting" << std::endl;
	for (sizet i = 0; i < maxPlayers; i++)
		disconnectSocket(sockets, players[i]);
}

static void sendRejection(TCPsocket server) {
	TCPsocket tmps = SDLNet_TCP_Accept(server);
	NetCode code = NetCode::full;
	SDLNet_TCP_Send(tmps, &code, sizeof(code));
	SDLNet_TCP_Close(tmps);
	std::cout << "rejected incoming connection" << std::endl;
}

static void addPlayer(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server, sizet& pc) {
	if (sizet i = findFirstFree(players); i < maxPlayers) {
		players[i] = SDLNet_TCP_Accept(server);
		SDLNet_TCP_AddSocket(sockets, players[i]);
		std::cout << "player " << ++pc << " connected" << std::endl;
	} else
		sendRejection(server);
}

static void checkWaitingPlayer(SDLNet_SocketSet sockets, TCPsocket* players, sizet i, sizet& pc) {
	if (!SDLNet_SocketReady(players[i]))
		return;

	uint8 rcvBuf[bufSiz];
	if (int len; (len = SDLNet_TCP_Recv(players[i], rcvBuf, bufSiz)) <= 0) {
		disconnectSocket(sockets, players[i]);
		std::cout << "player " << pc-- << " disconnected" << std::endl;
	} else
		std::cerr << "invalid net code " << uint(*rcvBuf) << " from player " << i + 1 << std::endl;
}

static WaitResult waitForPlayers(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server) {
	std::cout << "waiting for players" << std::endl;
	for (sizet pc = 0; pc < maxPlayers;) {
		if (!running || quitting())
			return WaitResult::exit;
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			addPlayer(sockets, players, server, pc);
		for (sizet i = 0; i < maxPlayers; i++)
			checkWaitingPlayer(sockets, players, i, pc);
	}
	// decide which player goes first and send information about
	std::default_random_engine randGen = createRandomEngine();
	sizet first = sizet(std::uniform_int_distribution<uint>(0, 1)(randGen));
	uint8 buff[2] = {uint8(NetCode::setup)};
	for (sizet i = 0; i < maxPlayers; i++) {
		buff[1] = i == first;
		if (SDLNet_TCP_Send(players[i], buff, sizeof(buff)) != sizeof(buff)) {
			std::cerr << SDLNet_GetError() << std::endl;
			return WaitResult::retry;
		}
	}
	std::cout << "all players ready" << std::endl;
	return WaitResult::ready;
}

static bool checkPlayer(TCPsocket* players, sizet i) {
	int len;
	uint8 rcvBuf[bufSiz];
	if (SDLNet_SocketReady(players[i]) && (len = SDLNet_TCP_Recv(players[i], rcvBuf, bufSiz)) <= 0) {
		std::cout << "player " << i + 1 << " disconnected" << std::endl;
		return false;
	}

	if (NetCode(*rcvBuf) < NetCode::ready)
		std::cerr << "invalid net code " << uint(*rcvBuf) << " from player " << i + 1 << std::endl;
	else if (SDLNet_TCP_Send(players[(i + 1) % maxPlayers], rcvBuf, len) != len) {	// forward data to other player
		std::cerr << SDLNet_GetError() << std::endl;
		return false;
	}
	return true;
}

static WaitResult runGame(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server) {
	while (running && !quitting()) {
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			sendRejection(server);
		for (sizet i = 0; i < maxPlayers; i++)
			if (!checkPlayer(players, i))
				return WaitResult::retry;
	}
	return WaitResult::exit;
}

static void eventExit(int) {
	running = false;
}
#ifdef _WIN32
int wmain(int argc, wchar** argv) {
	uint16 port = argc < 2 ? defaultPort : uint16(wcstoul(argv[1], nullptr, 0));
#else
int main(int argc, char** argv) {
	uint16 port = argc < 2 ? defaultPort : uint16(strtoul(argv[1], nullptr, 0));
	std::cout << "using port " << port << std::endl;

	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~uint(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
	// init server
	if (SDL_Init(0)) {
		std::cerr << "failed to initialize SDL:\n" << SDL_GetError() << std::endl;
		return -1;
	}
	if (SDLNet_Init()) {
		std::cerr << "failed to initialize networking:\n" << SDLNet_GetError() << std::endl;
		SDL_Quit();
		return -1;
	}
	IPaddress address;
	if (SDLNet_ResolveHost(&address, nullptr, port))
		return connectionFail("failed to resolve host:");
	TCPsocket server = SDLNet_TCP_Open(&address);
	if (!server)
		return connectionFail("failed to resolve host:");

	TCPsocket players[2] = {nullptr, nullptr};
	SDLNet_SocketSet sockets = SDLNet_AllocSocketSet(maxSockets);
	SDLNet_TCP_AddSocket(sockets, server);
	std::cout << "press 'q' to exit" << std::endl;

	signal(SIGINT, eventExit);
	signal(SIGABRT, eventExit);
	signal(SIGTERM, eventExit);
#ifndef _WIN32
	signal(SIGQUIT, eventExit);
#endif
	// run game server
	for (WaitResult cont = WaitResult::retry; cont != WaitResult::exit;) {
		if (cont = waitForPlayers(sockets, players, server); cont == WaitResult::ready)
			cont = runGame(sockets, players, server);
		disconnectPlayers(sockets, players);
	};
	SDLNet_TCP_DelSocket(sockets, server);
	SDLNet_TCP_Close(server);
	SDLNet_FreeSocketSet(sockets);

	SDLNet_Quit();
	SDL_Quit();
#ifndef _WIN32
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
	return 0;
}
