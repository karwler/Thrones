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
constexpr uint8 maxPlayers = maxRooms * 2;
constexpr char argPort = 'p';
constexpr char argLog = 'l';
constexpr char argVerbose = 'v';

static bool running = true;
static SDLNet_SocketSet sockets = nullptr;
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

static const char* sendRoomList(TCPsocket socket, Code code = Code::rlist) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(uint8(rooms.size()));
	for (const pair<const string, bool>& it : rooms) {
		sendb.push(vector<uint8>{ uint8(it.second), uint8(it.first.length()) });
		sendb.push(it.first);
	}
	sendb.write(uint16(sendb.size()), ofs);
	return sendb.send(socket);
}

static void sendRoomData(Code code, const string& name, const vector<uint8>& extra = {}) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(extra);
	sendb.push(uint8(name.length()));
	sendb.push(name);
	sendb.write(uint16(sendb.size()), ofs);
	for (uint8 i = 0; i < players.size(); i++)
		if (players[i].state == Player::State::lobby)
			if (const char* err = sendb.send(players[i].sock, sendb.size()))
				print(std::cerr, "failed to send room data '" + toStr(uint8(code)) + "' of '" + name + "' to player '" + toStr(i) + "': " + err);
	sendb.clear();
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
	string name = readName(data);
	if (uint8 ptc = uint8(std::find_if(players.begin(), players.end(), [pid, &name](Player& it) -> bool { return &it != &players[pid] && it.room == name; }) - players.begin()); rooms.count(name) && rooms[name] && ptc < players.size()) {
		players[pid].room = std::move(name);
		players[pid].state = Player::State::guest;
		players[pid].partner = ptc;
		players[ptc].partner = pid;
		sendRoomData(Code::ropen, players[pid].room, { uint8(rooms[players[pid].room] = false) });
		if (sendb.pushHead(Code::hello); const char* err = sendb.send(players[ptc].sock))
			print(std::cerr, "failed to send join request from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
	} else {
		sendb.pushHead(Code::cnjoin, Com::dataHeadSize + 1);
		if (sendb.push(uint8(false)); const char* err = sendb.send(players[pid].sock))
			print(std::cerr, "failed to send join rejection to player '" + toStr(pid) + "': " + err);
	}
}

static void leaveRoom(uint8 pid, bool sendList = true) {
	uint8 ptc = players[pid].partner;
	if (players[pid].state == Player::State::guest) {
		sendRoomData(Code::ropen, players[pid].room, { uint8(rooms[players[pid].room] = true) });
		if (sendb.pushHead(Code::leave); const char* err = sendb.send(players[ptc].sock))
			print(std::cerr, "failed to send leave info from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
	} else {		// must be host
		rooms.erase(players[pid].room);
		sendRoomData(Code::rerase, players[pid].room);
		if (ptc < players.size())
			if (players[ptc].state = Player::State::lobby; const char* err = sendRoomList(players[ptc].sock, Code::hgone))
				print(std::cerr, "failed to send room list to player '" + toStr(ptc) + "': " + err);
	}

	if (players[pid].state = Player::State::lobby; ptc < players.size())
		players[pid].partner = players[ptc].partner = UINT8_MAX;
	if (const char* err; sendList && (err = sendRoomList(players[pid].sock)))
		print(std::cerr, "failed to send room list to player '" + toStr(pid) + "': " + err);
}

static void connectPlayer(TCPsocket server) {
	if (players.size() >= maxPlayers) {
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
		players.emplace_back(psock);
		print(std::cout, "player '" + toStr(players.size() - 1) + "' connected");
	} catch (const char* err) {
		SDLNet_TCP_DelSocket(sockets, psock);
		SDLNet_TCP_Close(psock);
		print(std::cerr, "failed to connect player: " + string(err));
	}
}

static void disconnectPlayer(uint8 pid) {
	if (players[pid].state != Player::State::lobby)
		leaveRoom(pid, false);
	SDLNet_TCP_DelSocket(sockets, players[pid].sock);
	SDLNet_TCP_Close(players[pid].sock);

	for (Player& it : players)
		if (it.partner > pid && it.partner < players.size())
			it.partner--;
	players.erase(players.begin() + pid);
	print(std::cout, "player '" + toStr(pid) + "' disconnected");
}

static void validatePlayer(uint8* data, uint8 pid) {
	if (string ver = readVersion(data); std::find(compatibleVersions.begin(), compatibleVersions.end(), ver) != compatibleVersions.end()) {
		if (const char* err = sendRoomList(players[pid].sock)) {
			print(std::cerr, "failed to send room list to player '" + toStr(pid) + "': " + err);
			disconnectPlayer(pid);
		}
	} else {
		uint16 len = uint16(strlen(commonVersion));
		sendb.pushHead(Com::Code::version, dataHeadSize + len);
		sendb.push(reinterpret_cast<const uint8*>(commonVersion), len);
		sendb.send(players[pid].sock);

		print(std::cout, "failed to validate player '" + toStr(pid) + "' of version '" + ver + "'");
		disconnectPlayer(pid);
	}
}

static bool checkPlayer(uint8 pid) {
	return !players[pid].recvb.recv(players[pid].sock, [pid](uint8* data) {
		switch (Code(data[0])) {
		case Code::version:
			validatePlayer(data, pid);
			break;
		case Code::rnew:
			createRoom(data + dataHeadSize, pid);
			break;
		case Code::join:
			joinRoom(data + dataHeadSize, pid);
			break;
		case Code::leave:
			leaveRoom(pid);
			break;
		case Code::kick:
			leaveRoom(players[pid].partner);
			break;
		default:
			if (uint16 size = SDLNet_Read16(data + 1); Code(data[0]) < Code::hello)
				print(std::cerr, "invalid net code from player " + toStr(pid));
			else if (uint8 ptc = players[pid].partner; ptc >= players.size())
				print(std::cerr, "data with code '" + toStr(data[0]) + "' of size '" + toStr(size) + "' to invalid partner '" + toStr(ptc) + "' of player '" + toStr(pid) + "'");
			else if (const char* err = players[pid].recvb.send(players[ptc].sock, size))
				print(std::cerr, "failed to send data with codde '" + toStr(data[0]) + "' of size '" + toStr(size) + "' from player '" + toStr(pid) + "' to player '" + toStr(ptc) + "': " + err);
		}
	});
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
		vector<array<string, 5>> table(players.size() + 1);
		for (sizet i = 0; i < players.size(); i++) {
			table[i+1] = { toStr(i), addressToStr(SDLNet_TCP_GetPeerAddress(players[i].sock)), Player::stateNames[uint8(players[i].state)], "", "" };
			if (players[i].state != Player::State::lobby)
				table[i+1][4] = players[i].room;
			if (players[i].partner < players.size())
				table[i+1][5] = toStr(players[i].partner);
		}
		printTable(table, "Players:", { "ID", "ADDRESS", "STATE", "ROOM", "PARTNER" });
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
		print(std::cout, "Thrones Server " + string(commonVersion) + linend + "using port " + toStr(port));

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
				connectPlayer(server);
			for (uint8 i = 0; i < players.size(); i++) {
				try {
					if (!checkPlayer(i))
						disconnectPlayer(i--);
				} catch (const char* err) {
					print(std::cerr, "player '" + toStr(i) + "' error: " + err);
					disconnectPlayer(i--);
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
