#include "common.h"
#include "log.h"
#include <csignal>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#endif
using namespace Com;

static bool cprocValidate(uint pid);
static bool cprocPlayer(uint pid);

// PLAYER

struct Player {
	enum class State : uint8 {
		lobby,
		host,
		guest
	};
	static constexpr array<const char*, uint8(State::guest)+1> stateNames = {
		"lobby",
		"host",
		"guest"
	};

	Buffer recvb;
	bool (*cproc)(uint);
	string room;
	uint partner;
	State state;
	bool webs;

	Player();
};

Player::Player() :
	cproc(cprocValidate),
	partner(UINT_MAX),
	state(State::lobby),
	webs(false)
{}

// PLAYER ERROR

struct PlayerError {
	const uset<uint> pids;

	PlayerError(uset<uint>&& ids);
};

PlayerError::PlayerError(uset<uint>&& ids) :
	pids(std::move(ids))
{}

// SERVER

constexpr uint32 checkTimeout = 500;
constexpr uint defaultMaxPlayers = 1024;
constexpr char argPort = 'p';
constexpr char arg4 = '4';
constexpr char arg6 = '6';
constexpr char argMaxPlayers = 'c';
constexpr char argLog = 'l';
constexpr char argVerbose = 'v';

static bool running = true;
static uint maxPlayers;
static Buffer sendb;
static vector<Player> players;
static vector<pollfd> pfds;			// player socket data (last element is server)
static umap<string, bool> rooms;	// name, isFull
static Log slog;
#if !defined(_WIN32) && !defined(SERVICE)
static termios termst;
static tcflag_t oldLflag;
#endif

static uint maxRooms() {
	return maxPlayers / 2 + maxPlayers % 2;
}

static void sendRoomList(uint pid, Code code = Code::rlist) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(uint8(rooms.size()));
	for (auto& [name, full] : rooms) {
		sendb.push({ uint8(full), uint8(name.length()) });
		sendb.push(name);
	}
	sendb.write(uint16(sendb.getDlim()), ofs);
	sendb.send(pfds[pid].fd, players[pid].webs);
}

static void sendRoomData(Code code, const string& name, initlist<uint8> extra, uset<uint>& errPids) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(extra);
	sendb.push(uint8(name.length()));
	sendb.push(name);
	sendb.write(uint16(sendb.getDlim()), ofs);
	for (uint i = 0; i < uint(players.size()); i++)
		if (players[i].state == Player::State::lobby) {
			try {
				sendb.send(pfds[i].fd, players[i].webs, false);
			} catch (const Error& err) {
				errPids.insert(i);
				slog.write(std::cerr, "failed to send room data '", uint(code), "' of '", name, "' to player '", i, "': ", err.message);
			}
		}
	sendb.clear();
}

static void sendRoomData(Code code, const string& name, initlist<uint8> extra = {}) {
	uset<uint> errPids;
	if (sendRoomData(code, name, extra, errPids); !errPids.empty())
		throw PlayerError(std::move(errPids));
}

static void createRoom(const uint8* data, uint pid) {
	string room = readName(data);
	bool maxed = rooms.size() >= maxRooms();
	bool ok = !(maxed || rooms.count(room));
	try {
		string msg = ok ? "" : maxed ? "Server full" : "Name taken";
		sendb.pushHead(Code::cnrnew, dataHeadSize + uint16(msg.length()));
		sendb.push(msg);
		sendb.send(pfds[pid].fd, players[pid].webs);
	} catch (const Error& err) {
		sendb.clear();
		slog.write(std::cerr, "failed to send host ", ok ? "accept" : "rejection", " to player '", pid, "': ", err.message);
		throw PlayerError({ pid });
	}

	if (ok) {
		players[pid].room = std::move(room);
		players[pid].state = Player::State::host;
		rooms.emplace(players[pid].room, true);
		sendRoomData(Code::rnew, players[pid].room);
	}
}

static void joinRoom(const uint8* data, uint pid) {
	string name = readName(data);
	if (uint ptc = uint(std::find_if(players.begin(), players.end(), [pid, &name](Player& it) -> bool { return &it != &players[pid] && it.room == name; }) - players.begin()); rooms.count(name) && rooms[name] && ptc < players.size()) {
		try {
			sendb.pushHead(Code::hello);
			sendb.send(pfds[ptc].fd, players[ptc].webs);
		} catch (const Error& err) {
			slog.write(std::cerr, "failed to send join request from player '", pid, "' to player '", ptc, "': ", err.message);
			sendb.clear();
			try {
				sendb.pushHead(Code::cnjoin, Com::dataHeadSize + 1);
				sendb.push(uint8(false));
				sendb.send(pfds[pid].fd, players[pid].webs);
			} catch (const Error& e) {
				sendb.clear();
				slog.write(std::cerr, "failed to send join rejection to player '", pid, "': ", e.message);
				throw PlayerError({ pid, ptc });
			}
			throw PlayerError({ ptc });
		}
		players[pid].room = std::move(name);
		players[pid].state = Player::State::guest;
		players[pid].partner = ptc;
		players[ptc].partner = pid;
		sendRoomData(Code::ropen, players[pid].room, { uint8(rooms[players[pid].room] = false) });
	} else {
		try {
			sendb.pushHead(Code::cnjoin, Com::dataHeadSize + 1);
			sendb.push(uint8(false));
			sendb.send(pfds[pid].fd, players[pid].webs);
		} catch (const Error& err) {
			sendb.clear();
			slog.write(std::cerr, "failed to send join rejection to player '", pid, "': ", err.message);
			throw PlayerError({ pid });
		}
	}
}

static void leaveRoom(uint pid, bool sendList = true) {
	uset<uint> errPids;
	uint ptc = players[pid].partner;
	if (players[pid].state == Player::State::guest) {
		sendRoomData(Code::ropen, players[pid].room, { uint8(rooms[players[pid].room] = true) }, errPids);
		try {
			sendb.pushHead(Code::leave);
			sendb.send(pfds[ptc].fd, players[ptc].webs);
		} catch (const Error& err) {
			slog.write(std::cerr, "failed to send leave info from player '", pid, "' to player '", ptc, "': ", err.message);
			errPids.insert(pid);
		}
	} else {	// must be host
		rooms.erase(players[pid].room);
		sendRoomData(Code::rerase, players[pid].room, {}, errPids);
		if (ptc < players.size()) {
			players[ptc].state = Player::State::lobby;
			try {
				sendRoomList(ptc, Code::hgone);
			} catch (const Error& err) {
				slog.write(std::cerr, "failed to send room list to player '", ptc, "': ", err.message);
				errPids.insert(pid);
			}
		}
	}

	if (players[pid].state = Player::State::lobby; ptc < players.size())
		players[pid].partner = players[ptc].partner = UINT_MAX;
	if (sendList) {
		try {
			sendRoomList(pid);
		} catch (const Error& err) {
			slog.write(std::cerr, "failed to send room list to player '", pid, "': ", err.message);
			errPids.insert(pid);
		}
	}
	if (!errPids.empty())
		throw PlayerError(std::move(errPids));
}

static void connectPlayer(nsint server) {
	if (players.size() >= maxPlayers) {
		sendRejection(server);
		slog.write(std::cout, "rejected incoming connection");
	} else try {
		pfds.insert(pfds.end() - 1, pollfd{ acceptSocket(server), POLLIN | POLLRDHUP, 0 });
		players.emplace_back();
		slog.write(std::cout, "player '", players.size() - 1, "' connected");
	} catch (const Error& err) {
		slog.write(std::cerr, err.message);
	}
}

static void disconnectPlayer(uint pid) {
	if (players[pid].state != Player::State::lobby)
		leaveRoom(pid, false);
	closeSocket(pfds[pid].fd);
	pfds.erase(pfds.begin() + pid);

	for (Player& it : players)
		if (it.partner > pid && it.partner < players.size())
			it.partner--;
	players.erase(players.begin() + pid);
	slog.write(std::cout, "player '", pid, "' disconnected");
}

static void disconnectPlayers(uint& icur, vector<uint> pids) {
	for (sizet i = 0; i < pids.size(); i++) {
		if (pids[i] <= icur)
			icur--;
		for (sizet j = i + 1; j < pids.size(); j++)
			if (pids[j] > pids[i])
				pids[j]--;
		disconnectPlayer(pids[i]);
	}
}

bool cprocValidate(uint pid) {
	try {
		switch (players[pid].recvb.recvConn(pfds[pid].fd, players[pid].webs)) {
		case Buffer::Init::wait:
			return false;
		case Buffer::Init::connect:
			try {
				sendRoomList(pid);
			} catch (const Error& err) {
				sendb.clear();
				slog.write(std::cerr, "failed to send room list to player '", pid, "': ", err.message);
				throw PlayerError({ pid });
			}
			players[pid].cproc = cprocPlayer;
			break;
		case Buffer::Init::version:
			sendVersion(pfds[pid].fd, players[pid].webs);
		case Buffer::Init::error:
			throw PlayerError({ pid });
		}
	} catch (const Error&) {
		throw PlayerError({ pid });
	}
	return true;
}

bool cprocPlayer(uint pid) {
	uint8* data;
	try {
		if (!(data = players[pid].recvb.recv(pfds[pid].fd, players[pid].webs)))
			return false;
	} catch (const Error&) {
		throw PlayerError({ pid });
	}

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
	case Code::kick:
		leaveRoom(players[pid].partner);
		break;
	default: {
		if (Code(data[0]) < Code::hello || Code(data[0]) > Code::message) {
			slog.write(std::cerr, "invalid net code '", uint(data[0]), "' from player '", pid, "' of size '", read16(data + 1), '\'');
			throw PlayerError({ pid });
		}
		uint ptc = players[pid].partner;
		if (ptc >= players.size()) {
			slog.write(std::cerr, "data with code '", uint(data[0]), "' from player '", pid, "' of size '", read16(data + 1), "' to invalid partner '", ptc + '\'');
			throw PlayerError({ pid });
		}

		try {
			players[pid].recvb.redirect(pfds[ptc].fd, data, players[ptc].webs);
		} catch (const Error& err) {
			slog.write(std::cerr, "failed to send data with code '", uint(data[0]), "' of size '", read16(data + 1), "' from player '", pid, "' to player '", ptc, "': ", err.message);
			throw PlayerError({ ptc });
		}
	} }
	players[pid].recvb.clearCur(players[pid].webs);
	return true;
}

#ifndef SERVICE
template <sizet S>
void printTable(vector<array<string, S>>& table, const char* title, array<string, S>&& header) {
	array<uint, S> lens{};
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
	case 'P': {
		vector<array<string, 4>> table(players.size() + 1);
		for (sizet i = 0; i < players.size(); i++)
			table[i+1] = { toStr(i), Player::stateNames[uint8(players[i].state)], players[i].state != Player::State::lobby ? table[i+1][2] = players[i].room : string(), players[i].partner < players.size() ? table[i+1][3] = toStr(players[i].partner) : string() };
		printTable(table, "Players:", { "ID", "STATE", "ROOM", "PARTNER" });
		break; }
	case 'R': {
		vector<string> names = sortNames(rooms);
		vector<array<string, 2>> table(names.size() + 1);
		for (sizet i = 0; i < names.size(); i++) {
			table[i+1][1] = btos(rooms[names[i]]);
			table[i+1][0] = std::move(names[i]);
		}
		printTable(table, "Rooms:", { "NAME", "OPEN" });
		break; }
	case 'Q':
		running = false;
		break;
	default:
		std::cerr << "unknown input: '" << char(ch) << "' (" << ch << ')' << std::endl;
	}
}
#endif

static void eventExit(int) {
	running = false;
}

static bool exec() {
	if (int rcp = poll(pfds.data(), ulong(pfds.size()), checkTimeout)) {
		if (rcp < 0 || (pfds.back().revents & polleventsDisconnect)) {
			slog.write(std::cerr, msgPollFail);
			return running = false;
		}

		if (pfds.back().revents & POLLIN)
			connectPlayer(pfds.back().fd);
		for (uint i = 0; i < uint(players.size()); i++) {
			try {
				if (pfds[i].revents & POLLIN) {
					bool final = players[i].recvb.recvData(pfds[i].fd);
					while (players[i].cproc(i));
					if (final)
						throw PlayerError({ i });
				}
				if (pfds[i].revents & polleventsDisconnect)
					throw PlayerError({ i });
			} catch (const PlayerError& err) {
				disconnectPlayers(i, vector<uint>(err.pids.begin(), err.pids.end()));
			} catch (...) {
				slog.write(std::cerr, "unexpected error during player '", i, "' iteration");
			}
		}
	}
#ifndef SERVICE
	checkInput();
#endif
	return running;
}

static int cleanup(int rc) {
	for (pollfd& it : pfds)
		closeSocket(it.fd);
	slog.end();
#ifdef _WIN32
	WSACleanup();
#elif !defined(SERVICE)
	termst.c_lflag = oldLflag;
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
#endif
	return rc;
}

#ifdef _WIN32
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
#ifndef SERVICE
	tcgetattr(STDIN_FILENO, &termst);
	oldLflag = termst.c_lflag;
	termst.c_lflag &= ~uint(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
#endif
	signal(SIGQUIT, eventExit);
#endif
	signal(SIGINT, eventExit);
	signal(SIGABRT, eventExit);
	signal(SIGTERM, eventExit);

	try {
		Arguments args(argc, argv, { arg4, arg6, argVerbose }, { argPort, argMaxPlayers, argLog });
		slog.start(args.hasFlag(argVerbose), args.getOpt(argLog));

		const char* port = args.getOpt(argPort);
		if (!port)
			port = defaultPort;
		const char* playerLim = args.getOpt(argMaxPlayers);
		maxPlayers = playerLim ? std::min(sstoul(playerLim), ulong(UINT_MAX)) : defaultMaxPlayers;
		Family family = Family::any;
		if (args.hasFlag(arg4) && !args.hasFlag(arg6))
			family = Family::v4;
		else if (args.hasFlag(arg6) && !args.hasFlag(arg4))
			family = Family::v6;

#ifdef _WIN32
		DWORD pid = GetCurrentProcessId();
		if (WSADATA wsad; WSAStartup(MAKEWORD(2, 2), &wsad))
			throw Error(msgWinsockFail);
#else
		pid_t pid = getpid();
#endif
		pfds.push_back({ bindSocket(port, family), POLLIN | POLLRDHUP, 0 });
		slog.write(std::cout, "Thrones Server v", commonVersion, linend, "PID: ", pid, linend, "port: ", port, linend, "family: ", familyNames[uint8(family)], linend, "player limit: ", maxPlayers, linend, "room limit: ", maxRooms(), linend);
	} catch (const Error& err) {
		slog.write(std::cerr, err.message);
		return cleanup(EXIT_FAILURE);
	}

	try {
		while (exec());
	} catch (const std::runtime_error& err) {
		slog.write(std::cerr, "runtime error: ", err.what());
		return cleanup(EXIT_FAILURE);
	} catch (...) {
		slog.write(std::cerr, "unknown error");
		return cleanup(EXIT_FAILURE);
	}
	return cleanup(EXIT_SUCCESS);
}
