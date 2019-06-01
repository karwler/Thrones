#include "server.h"
#include <csignal>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
using namespace Com;

enum class WaitResult : uint8 {
	ready,
	retry,
	exit
};

constexpr uint32 checkTimeout = 500;

static bool running = true;

static Config getConfig(const Arguments& args, uint16& port) {
	umap<string, string>::const_iterator it = args.opts.find("p");
	port = it != args.opts.end() ? uint16(sstoul(it->second)) : defaultPort;
	
	it = args.opts.find("c");
	string name = it != args.opts.end() ? it->second : Config::defaultName;

	char* path = SDL_GetBasePath();
	it = args.opts.find("f");
	string file = it != args.opts.end() ? it->second : (path ? path : string("")) + defaultConfigFile;
	SDL_free(path);

	Config ret;
	vector<Config> confs = loadConfs(file);
	if (vector<Config>::iterator cit = std::find_if(confs.begin(), confs.end(), [&name](const Config& cf) -> bool { return cf.name == name; }); cit != confs.end())
		ret = cit->checkValues();
	else
		confs.push_back(ret);
	saveConfs(confs, file);
	return ret;
}

static int connectionFail(const char* msg, int ret = -1) {
	std::cerr << msg << '\n' << SDLNet_GetError() << std::endl;
	SDLNet_Quit();
	SDL_Quit();
	return ret;
}

static bool quitting() {
#ifdef _WIN32
	char cv = _kbhit() ? char(_getch()) : '\0';
#else
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	timeval tv = { 0, 0 };
	char cv = select(1, &fds, nullptr, nullptr, &tv) ? char(getchar()) : '\0';
#endif
	return cv == 'q' || cv == 'Q';
}

static void disconnectSocket(SDLNet_SocketSet sockets, TCPsocket& client) {
	if (client) {
		SDLNet_TCP_DelSocket(sockets, client);
		SDLNet_TCP_Close(client);
		client = nullptr;
	}
}

static void disconnectPlayers(SDLNet_SocketSet sockets, TCPsocket* players) {
	std::cout << "disconnecting\n" << std::endl;
	for (uint8 i = 0; i < maxPlayers; i++)
		disconnectSocket(sockets, players[i]);
}

static void addPlayer(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server, uint8& pc) {
	if (uint8 i = uint8(std::find(players, players + maxPlayers, nullptr) - players); i < maxPlayers) {
		players[i] = SDLNet_TCP_Accept(server);
		SDLNet_TCP_AddSocket(sockets, players[i]);
		std::cout << "player " << uint(++pc) << " connected" << std::endl;
	} else
		sendRejection(server);
}

static void checkWaitingPlayer(SDLNet_SocketSet sockets, TCPsocket* players, uint8 i, uint8& pc) {
	if (!SDLNet_SocketReady(players[i]))
		return;

	if (uint8 rbuf; SDLNet_TCP_Recv(players[i], &rbuf, sizeof(rbuf)) <= 0) {
		disconnectSocket(sockets, players[i]);
		std::cout << "player " << uint(pc--) << " disconnected" << std::endl;
	} else
		std::cerr << "invalid data from player " << uint(i + 1) << std::endl;
}

static WaitResult waitForPlayers(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server, const Config& conf) {
	std::cout << "waiting for players" << std::endl;
	for (uint8 pc = 0; pc < maxPlayers;) {
		if (!running || quitting())
			return WaitResult::exit;
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			addPlayer(sockets, players, server, pc);
		for (uint8 i = 0; i < maxPlayers; i++)
			checkWaitingPlayer(sockets, players, i, pc);
	}

	// decide which player goes first and send game config information
	std::default_random_engine ranGen = createRandomEngine();
	uint8 first = uint8(std::uniform_int_distribution<uint>(0, 1)(ranGen));
	vector<uint8> sendb(conf.dataSize(Code::setup));
	conf.toComData(sendb.data());
	for (uint8 i = 0; i < maxPlayers; i++) {
		sendb[1] = i == first;
		if (SDLNet_TCP_Send(players[i], sendb.data(), int(sendb.size())) != int(sendb.size())) {
			std::cerr << SDLNet_GetError() << std::endl;
			return WaitResult::retry;
		}
	}
	std::cout << "all players ready" << std::endl;
	return WaitResult::ready;
}

static bool checkPlayer(TCPsocket* players, uint8 i, uint8* ntbuf) {
	if (!SDLNet_SocketReady(players[i]))
		return true;

	int len = SDLNet_TCP_Recv(players[i], ntbuf, recvSize);
	if (len <= 0) {
		std::cout << "player " << uint(i + 1) << " disconnected" << std::endl;
		return false;
	}
	if (SDLNet_TCP_Send(players[(i + 1) % maxPlayers], ntbuf, len) != len) {	// forward data to other player
		std::cerr << SDLNet_GetError() << std::endl;
		return false;
	}
	return true;
}

static WaitResult runGame(SDLNet_SocketSet sockets, TCPsocket* players, TCPsocket server) {
	for (uint8 ntbuf[recvSize]; running && !quitting();) {
		if (SDLNet_CheckSockets(sockets, checkTimeout) <= 0)
			continue;

		if (SDLNet_SocketReady(server))
			sendRejection(server);
		for (uint8 i = 0; i < maxPlayers; i++)
			if (!checkPlayer(players, i, ntbuf))
				return WaitResult::retry;
	}
	return WaitResult::exit;
}

static void eventExit(int) {
	running = false;
}
#ifdef _WIN32
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
	termios termst;
	tcgetattr(STDIN_FILENO, &termst);
	tcflag_t oldLflag = termst.c_lflag;
	termst.c_lflag &= ~uint(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
#endif
	uint16 port;
	Config conf = getConfig(Arguments(argc, argv), port);

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
	std::cout << "using port " << port << std::endl;
	
	IPaddress address;
	if (SDLNet_ResolveHost(&address, nullptr, port))
		return connectionFail("failed to resolve host:");
	TCPsocket server = SDLNet_TCP_Open(&address);
	if (!server)
		return connectionFail("failed to resolve host:");

	TCPsocket players[maxPlayers] = { nullptr, nullptr };
	SDLNet_SocketSet sockets = SDLNet_AllocSocketSet(maxPlayers + 1);
	SDLNet_TCP_AddSocket(sockets, server);
	std::cout << "press 'q' to exit\n" << std::endl;

	signal(SIGINT, eventExit);
	signal(SIGABRT, eventExit);
	signal(SIGTERM, eventExit);
#ifndef _WIN32
	signal(SIGQUIT, eventExit);
#endif
	// run game server
	for (WaitResult cont = WaitResult::retry; cont != WaitResult::exit;) {
		if (cont = waitForPlayers(sockets, players, server, conf); cont == WaitResult::ready)
			cont = runGame(sockets, players, server);
		disconnectPlayers(sockets, players);
	};
	SDLNet_TCP_DelSocket(sockets, server);
	SDLNet_TCP_Close(server);
	SDLNet_FreeSocketSet(sockets);

	SDLNet_Quit();
	SDL_Quit();
#ifndef _WIN32
	termst.c_lflag = oldLflag;
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
#endif
	return 0;
}
