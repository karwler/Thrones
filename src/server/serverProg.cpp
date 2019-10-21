#include "server.h"
#include <csignal>
#include <fstream>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
using namespace Com;

struct Player {
	enum class State : uint8 {
		lobby,
		host,
		guest
	};
	static const array<string, uint8(State::guest)+1> stateNames;

	TCPsocket sock;
	Buffer recvb;
	string room;
	uint8 partner;
	State state;

	Player(TCPsocket socket);
};

const array<string, Player::stateNames.size()> Player::stateNames = {
	"lobby",
	"host",
	"guest"
};

Player::Player(TCPsocket socket) :
	sock(socket),
	partner(UINT8_MAX),
	state(State::lobby)
{}

constexpr uint32 checkTimeout = 500;
constexpr char argPort = 'p';
constexpr char argLog = 'l';
constexpr char argVerbose = 'v';

static bool running = true;
static Buffer sendb;
static vector<Player> players;
static umap<string, bool> rooms;	// name, isFull
static bool verbose;
static std::ofstream oflog;

static void print(std::ostream& vofs, const string& msg) {
	if (verbose)
		vofs << msg << std::endl;
	if (oflog)
		oflog << msg << std::endl;
}

static string addressToStr(const IPaddress* addr) {
	string str;
	for (uint8 i = 0; i < 4; i++)
		str += toStr((addr->host >> (i * 8)) & 0xFF) + '.';
	str.back() = ':';
	return str + toStr(addr->port);
}

static const char* sendRoomList(TCPsocket socket) {
	sendb.pushHead(Code::rlist);
	sendb.push(uint8(rooms.size()));
	for (const pair<const string, bool>& it : rooms) {
		sendb.push(vector<uint8>{ uint8(it.second), uint8(it.first.length()) });
		sendb.push(it.first);
	}
	SDLNet_Write16(sendb.size, sendb.data + 1);
	return sendb.send(socket);
}

static void sendRoomData(Code code, const string& name, const vector<uint8>& extra = {}) {
	sendb.pushHead(code);
	sendb.push(extra);
	sendb.push(uint8(name.length()));
	sendb.push(name);
	SDLNet_Write16(sendb.size, sendb.data + 1);
	for (uint8 i = 0; i < players.size(); i++)
		if (players[i].state == Player::State::lobby)
			if (SDLNet_TCP_Send(players[i].sock, sendb.data, sendb.size) != sendb.size)
				print(std::cerr, "failed to send room data '" + toStr(uint8(code)) + "' of '" + name + "' to player '" + toStr(i) + "': " + SDLNet_GetError());
	sendb.size = 0;
}

static void createRoom(uint8* data, uint8 pid) {
	bool ok;
	if (string room = readName(data); ok = !rooms.count(room)) {
		players[pid].room = std::move(room);
		players[pid].state = Player::State::host;
		rooms.emplace(players[pid].room, true);
		sendRoomData(Code::rnew, players[pid].room);
	}
	sendb.pushHead(Code::cnrnew);
	sendb.push(uint8(ok));
	if (const char* err = sendb.send(players[pid].sock))
		print(std::cerr, "failed to send host " + string(ok ? "accept" : "rejection") + " to player '" + toStr(pid) + "': " + err);
}

static void joinRoom(uint8* data, uint8 pid) {
	if (string name = readName(data); rooms[name])
		players[pid].room = std::move(name);
	else {
		sendb.pushHead(Code::cnjoin);
		sendb.push(uint8(false));
		if (const char* err = sendb.send(players[pid].sock))
			print(std::cerr, "failed to send join rejection to player '" + toStr(pid) + "': " + err);
		return;
	}
	for (uint8 i = 0; i < players.size(); i++)
		if (i != pid && players[i].room == players[pid].room) {
			players[pid].partner = i;
			players[i].partner = pid;
			break;
		}
	players[pid].state = Player::State::guest;
	sendRoomData(Code::ropen, players[pid].room, { false });

	sendb.pushHead(Code::hello);
	if (uint8 ptc = players[pid].partner; const char* err = sendb.send(players[ptc].sock))
		print(std::cerr, "failed to send join request from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
}

static void leaveRoom(uint8 pid, bool sendList = true) {
	uint8 ptc = players[pid].partner;
	if (players[pid].state == Player::State::guest) {
		if (sendb.pushHead(Code::leave); const char* err = sendb.send(players[ptc].sock))
			print(std::cerr, "failed to send leave info from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
		sendRoomData(Code::ropen, players[pid].room, { true });
	} else {		// must be host
		rooms.erase(players[pid].room);
		if (ptc < players.size()) {
			if (players[ptc].state = Player::State::lobby; const char* err = sendRoomList(players[ptc].sock))
				print(std::cerr, "failed to send room list to player '" + toStr(ptc) + "': " + err);
			if (sendb.pushHead(Code::leave); const char* err = sendb.send(players[ptc].sock))
				print(std::cerr, "failed to send leave info from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
		}
		sendRoomData(Code::rerase, players[pid].room);
	}

	if (players[pid].state = Player::State::lobby; ptc < players.size())
		players[pid].partner = players[ptc].partner = UINT8_MAX;
	if (const char* err; sendList && (err = sendRoomList(players[pid].sock)))
		print(std::cerr, "failed to send room list to player '" + toStr(pid) + "': " + err);
}

static void connectPlayer(SDLNet_SocketSet sockets, TCPsocket server) {
	if (players.size() >= maxPlayers) {	// TODO: version check
		sendRejection(server);
		print(std::cout, "rejected incoming connection");
		return;
	}

	TCPsocket psock;
	try {
		if (!(psock = SDLNet_TCP_Accept(server)))
			throw SDLNet_GetError();
		if (SDLNet_TCP_AddSocket(sockets, psock) < 0)
			throw SDLNet_GetError();
		if (const char* err = sendRoomList(psock))
			throw err;
		players.emplace_back(psock);
		print(std::cout, "player '" + toStr(players.size() - 1) + "' connected");
	} catch (const char* err) {
		SDLNet_TCP_DelSocket(sockets, psock);
		SDLNet_TCP_Close(psock);
		print(std::cerr, "failed to connect player: " + string(err));
	}
}

static void disconnectPlayer(SDLNet_SocketSet sockets, uint8 pid) {
	if (players[pid].state != Player::State::lobby)
		leaveRoom(pid, false);
	SDLNet_TCP_DelSocket(sockets, players[pid].sock);
	SDLNet_TCP_Close(players[pid].sock);

	for (Player& it : players)
		if (it.state != Player::State::lobby && it.partner > pid)
			it.partner--;
	players.erase(players.begin() + pid);
	print(std::cout, "player '" + toStr(pid) + "' disconnected");
}

static bool checkPlayer(uint8 pid) {
	if (!SDLNet_SocketReady(players[pid].sock))
		return true;
	int len = players[pid].recvb.recv(players[pid].sock);
	if (len <= 0)
		return false;

	players[pid].recvb.processRecv(len, [pid](uint8* data) {
		switch (Code(data[0])) {
		case Code::rnew:
			createRoom(data + dataHeadSize, pid);
			break;
		case Code::join:
			joinRoom(data + dataHeadSize, pid);
			break;
		case Code::leave:
			leaveRoom(pid);
			break;
		default:
			if (Code(data[0]) >= Code::hello)
				sendb.push(data, SDLNet_Read16(data + 1));
			else
				print(std::cerr, "invalid net code from player " + toStr(pid));
		}
	});
	if (sendb.size) {
		if (uint8 ptc = players[pid].partner; ptc >= players.size()) {
			sendb.size = 0;
			print(std::cerr, "data with code '" + toStr(sendb.data[0]) + "' of size '" + toStr(sendb.size) + "' to invalid partner '" + toStr(ptc) + "' of player '" + toStr(pid) + "'");
		} else if (const char* err = sendb.send(players[ptc].sock)) {
			sendb.size = 0;
			print(std::cerr, "failed to send data with codde '" + toStr(sendb.data[0]) + "' of size '" + toStr(sendb.size) + "' from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
		}
	}
	return true;
}

template <sizet S>
void printTable(vector<array<string, S>>& table, const char* title, array<string, S>&& header) {
	array<uint, S> lens;
	std::fill(lens.begin(), lens.end(), 0);
	table[0] = std::move(header);
	for (const array<string, S>& it : table)
		for (sizet i = 0; i < S; i++)
			if (it[i].length() > lens[i])
				lens[i] = uint(it[i].length());

	std::cout << title << linend;
	for (const array<string, S>& it : table) {
		for (sizet i = 0; i < S; i++)
			std::cout << it[i] << string(lens[i] - it[i].length() + 2, ' ');
		std::cout << linend;
	}
	std::cout << std::endl;
}

static void checkInput() {
#ifdef _WIN32
	if (!_kbhit())
		return;
	int ch = toupper(_getch());
#else
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	if (timeval tv = { 0, 0 }; !select(1, &fds, nullptr, nullptr, &tv))
		return;
	int ch = toupper(getchar());
#endif
	switch (ch) {
	case 'L': {
		vector<string> names = sortNames(rooms);
		vector<array<string, 2>> table(names.size() + 1);
		for (sizet i = 0; i < names.size(); i++) {
			table[i+1][1] = btos(rooms[names[i]]);
			table[i+1][0] = std::move(names[i]);
		}
		printTable(table, "Rooms:", { "NAME", "OPEN" });
		break; }
	case 'P': {
		vector<array<string, 6>> table(players.size() + 1);
		for (sizet i = 0; i < players.size(); i++) {
			table[i+1] = { toStr(i), addressToStr(SDLNet_TCP_GetPeerAddress(players[i].sock)), toStr(players[i].recvb.size), Player::stateNames[uint8(players[i].state)], "", "" };
			if (players[i].state != Player::State::lobby)
				table[i+1][4] = players[i].room;
			if (players[i].partner < players.size())
				table[i+1][5] = toStr(players[i].partner);
		}
		printTable(table, "Players:", { "ID", "ADDRESS", "RSIZE", "STATE", "ROOM", "PARTNER" });
		break; }
	case 'Q':
		running = false;
		break;
	default:
		std::cerr << "unknown input: '" << char(ch) << "' (" << ch << ')' << std::endl;
	}
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

	signal(SIGQUIT, eventExit);
#endif
	signal(SIGINT, eventExit);
	signal(SIGABRT, eventExit);
	signal(SIGTERM, eventExit);

	TCPsocket server = nullptr;
	SDLNet_SocketSet sockets = nullptr;
	try {
		Arguments args(argc, argv, { argLog, argVerbose }, { argPort });
		const char* cfstr = args.getOpt(argPort);
		uint16 port = cfstr ? uint16(sstoul(cfstr)) : defaultPort;
		verbose = args.hasFlag(argVerbose);
		if (args.hasFlag(argLog))
			oflog.open("server_log_" + Date::now().toString('-', '_', '-') + ".txt");

		if (SDL_Init(0))
			throw SDL_GetError();
		if (SDLNet_Init())
			throw SDLNet_GetError();
		print(std::cout, "using port " + toStr(port));

		IPaddress address;
		if (SDLNet_ResolveHost(&address, nullptr, port))
			throw SDLNet_GetError();
		if (!(sockets = SDLNet_AllocSocketSet(maxPlayers + 1)))
			throw SDLNet_GetError();
		if (!(server = SDLNet_TCP_Open(&address)))
			throw SDLNet_GetError();
		if (SDLNet_TCP_AddSocket(sockets, server) < 0)
			throw SDLNet_GetError();
	} catch (const char* err) {
		print(std::cerr, err);
		SDLNet_TCP_Close(server);
		SDLNet_FreeSocketSet(sockets);
		SDLNet_Quit();
		SDL_Quit();
		return -1;
	}

	while (running) {
		if (SDLNet_CheckSockets(sockets, checkTimeout) > 0) {
			if (SDLNet_SocketReady(server))
				connectPlayer(sockets, server);
			for (uint8 i = 0; i < players.size(); i++) {
				try {
					if (!checkPlayer(i))
						disconnectPlayer(sockets, i--);
				} catch (const char* err) {
					print(std::cerr, "player '" + toStr(i) + "' error: " + err);
					disconnectPlayer(sockets, i--);
				}
			}
		}
		checkInput();
	}
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
