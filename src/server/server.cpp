#include "server.h"
#include <iostream>
#include <string>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
using namespace Server;
using std::string;

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
	sendSingle(tmps, NetCode::full);
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

	if (uint8 buff[bufSiz]; SDLNet_TCP_Recv(players[i], buff, bufSiz) <= 0) {
		disconnectSocket(sockets, players[i]);
		std::cout << "player " << pc-- << " disconnected" << std::endl;
	} else
		std::cerr << "invalid net code " << uint(*buff) << " from player " << i << std::endl;
}

static bool waitForPlayers(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server) {
	std::cout << "waiting for players" << std::endl;
	for (sizet pc = 0; pc < maxPlayers;) {
		if (quitting())
			return false;
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			addPlayer(sockets, players, server, pc);
		for (sizet i = 0; i < maxPlayers; i++)
			checkWaitingPlayer(sockets, players, i, pc);
	}
	for (sizet i = 0; i < maxPlayers; i++)
		sendSingle(players[i], NetCode::start);
	std::cout << "all players ready" << std::endl;
	return true;
}

static bool checkPlayer(TCPsocket* players, sizet i) {
	if (uint8 buff[bufSiz]; SDLNet_SocketReady(players[i]) && SDLNet_TCP_Recv(players[i], buff, bufSiz) <= 0) {
		std::cout << "player " << i << " disconnected" << std::endl;
		return false;
	}
	// TODO: transferring data
	return true;
}

static bool runGame(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server) {
	while (!quitting()) {
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			sendRejection(server);
		for (sizet i = 0; i < maxPlayers; i++)
			if (!checkPlayer(players, i))
				return true;
	}
	return false;
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
	uint16 port = argc < 2 ? defaultPort : uint16(wcstoul(argv[1], nullptr, 0));
#else
int main(int argc, char** argv) {
	uint16 port = argc < 2 ? defaultPort : uint16(strtoul(argv[1], nullptr, 0));

	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~uint(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
	// init server
	if (SDL_Init(0)) {
		std::cerr << "Failed to initialize SDL:\n" << SDL_GetError() << std::endl;
		return -1;
	}
	if (SDLNet_Init()) {
		std::cerr << "Failed to initialize networking:\n" << SDLNet_GetError() << std::endl;
		SDL_Quit();
		return -1;
	}
	IPaddress address;
	if (SDLNet_ResolveHost(&address, nullptr, port))
		return connectionFail("Failed to resolve host:");
	TCPsocket server = SDLNet_TCP_Open(&address);
	if (!server)
		return connectionFail("Failed to resolve host:");

	TCPsocket players[2] = {nullptr, nullptr};
	SDLNet_SocketSet sockets = SDLNet_AllocSocketSet(maxSockets);
	SDLNet_TCP_AddSocket(sockets, server);

	// run game server
	for (bool cont = true; cont;) {
		cont = waitForPlayers(sockets, players, server) && runGame(sockets, players, server);
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
