#include "server.h"
#include "log.h"
#include <csignal>
#include <random>
#ifdef _WIN32
#include <conio.h>
#elif !defined(SERVICE)
#include <termios.h>
#endif
using namespace Com;

struct Player;

static bool cprocValidate(nsint pfd, Player& player);
static bool cprocPlayer(nsint pfd, Player& player);

// PLAYER

struct Player {
	Buffer recvb;
	bool (*cproc)(nsint, Player&) = cprocValidate;
	string name;
	nsint partner = INVALID_SOCKET;
	bool webs = false;
};

// PLAYER ERROR

struct PlayerError {
	const uset<nsint> pfds;

	PlayerError(uset<nsint>&& fds);
	PlayerError(initlist<nsint> fds);
};

PlayerError::PlayerError(uset<nsint>&& fds) :
	pfds(std::move(fds))
{}

PlayerError::PlayerError(initlist<nsint> fds) :
	pfds(fds)
{}

// TERMINAL

#if !defined(_WIN32) && !defined(SERVICE)
struct Terminal {
	termios termst;
	tcflag_t oldLflag;

	Terminal();
	~Terminal();
};

Terminal::Terminal() {
	tcgetattr(STDIN_FILENO, &termst);
	oldLflag = termst.c_lflag;
	termst.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
}

Terminal::~Terminal() {
	termst.c_lflag = oldLflag;
	tcsetattr(STDIN_FILENO, TCSANOW, &termst);
}
#endif

// SERVER

constexpr uint32 checkTimeout = 500;
constexpr uint defaultMaxPlayers = 1024;
constexpr uint maxPlayersLimit = 2040;
constexpr char argPort = 'p';
constexpr char arg4 = '4';
constexpr char arg6 = '6';
constexpr char argMaxPlayers = 'c';
constexpr char argLog = 'l';
constexpr char argMaxLogs = 'm';
constexpr char	argVerbose = 'v';

static bool running = true;
static uint maxPlayers;
static Buffer sendb;
static umap<nsint, Player> players;	// socket, player data
static umap<nsint, string> rooms;	// host socket, room name
static Log slog;
static std::default_random_engine randGen;
static std::uniform_int_distribution<uint16> randNameDist(1, UINT16_MAX);	// 0 is reserved to indicate a not taken player name

static uint maxRooms() {
	return maxPlayers / 2 + maxPlayers % 2;
}

template <class T>
void rekeyRoom(T room, nsint key) {
	umap<nsint, string>::node_type rnode = rooms.extract(room);
	rnode.key() = key;
	rooms.insert(std::move(rnode));
}

static void sendRoomList(nsint pfd, Player& player, Code code, initlist<uint8> extra = {}) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(extra);
	sendb.push(uint16(rooms.size()));
	for (auto& [host, name] : rooms) {
		sendb.push({ uint8(((players.at(host).partner == INVALID_SOCKET) << 7) | name.length()) });
		sendb.push(name);
	}
	sendb.write(uint16(sendb.getDlim()), ofs);
	sendb.send(pfd, player.webs);
}

static void sendConnRoomList(nsint pfd, Player& player, bool nameClash) {
	try {
		uint16 nresp = 0;
		if (nameClash) {
			string name;
			do {
				nresp = randNameDist(randGen);
				name = toStr<16>(nresp);
			} while (std::any_of(players.begin(), players.end(), [&name](const pair<const nsint, Player>& it) -> bool { return it.second.name == name; }));
			player.name = std::move(name);
		}
		uint8 nrbuf[2];
		write16(nrbuf, nresp);
		sendRoomList(pfd, player, Code::rlistcon, { nrbuf[0], nrbuf[1] });
	} catch (const Error& err) {
		sendb.clear();
		slog.err("failed to send room list to player ", pfd, ": ", err.what());
		throw PlayerError{ pfd };
	}
	player.cproc = cprocPlayer;
}

static void sendRoomData(Code code, const string& name, initlist<uint8> extra, uset<nsint>& errPfds) {
	uint ofs = sendb.pushHead(code, 0) - sizeof(uint16);
	sendb.push(extra);
	sendb.push(uint8(name.length()));
	sendb.push(name);
	sendb.write(uint16(sendb.getDlim()), ofs);
	for (auto& [pfd, player] : players)
		if (player.partner == INVALID_SOCKET && !rooms.count(pfd)) {
			try {
				sendb.send(pfd, player.webs, false);
			} catch (const Error& err) {
				errPfds.insert(pfd);
				slog.err("failed to send room data ", uint(code), " of ", name, " to player ", pfd, ": ", err.what());
			}
		}
	sendb.clear();
}

static void sendRoomData(Code code, const string& name, initlist<uint8> extra = {}) {
	uset<nsint> errPfds;
	if (sendRoomData(code, name, extra, errPfds); !errPfds.empty())
		throw PlayerError(std::move(errPfds));
}

static void createRoom(const uint8* data, nsint pfd, Player& player) {
	string name = readName(data);
	CncrnewCode code = CncrnewCode::ok;
	if (name.length() > roomNameLimit)
		code = CncrnewCode::length;
	else if (rooms.size() >= maxRooms())
		code = CncrnewCode::full;
	else if (std::any_of(rooms.begin(), rooms.end(), [&name](const pair<const nsint, string>& it) -> bool { return it.second == name; }))
		code = CncrnewCode::taken;

	try {
		sendb.pushHead(Code::cnrnew);
		sendb.push(uint8(code));
		sendb.send(pfd, player.webs);
	} catch (const Error& err) {
		sendb.clear();
		slog.err("failed to send host ", code == CncrnewCode::ok ? "accept" : "rejection", " to player ", pfd, ": ", err.what());
		throw PlayerError{ pfd };
	}
	if (code == CncrnewCode::ok) {
		umap<nsint, string>::iterator it = rooms.emplace(pfd, std::move(name)).first;
		sendRoomData(Code::rnew, it->second);
	}
}

static void joinRoom(const uint8* data, nsint pfd, Player& player) {
	string name = readName(data);
	umap<nsint, string>::iterator room = std::find_if(rooms.begin(), rooms.end(), [&name](const pair<const nsint, string>& it) -> bool { return it.second == name; });
	if (umap<nsint, Player>::iterator host = room != rooms.end() ? players.find(room->first) : players.end(); host != players.end() && host->second.partner == INVALID_SOCKET) {
		try {
			sendb.pushHead(Code::hello);
			sendb.send(room->first, host->second.webs);
		} catch (const Error& err) {
			slog.err("failed to send join request from player ", pfd, " to player ", room->first, ": ", err.what());
			sendb.clear();
			try {
				sendb.pushHead(Code::cnjoin, Com::dataHeadSize + 1);
				sendb.push(uint8(false));
				sendb.send(pfd, player.webs);
			} catch (const Error& e) {
				sendb.clear();
				slog.err("failed to send join rejection to player ", pfd, ": ", e.what());
				throw PlayerError{ pfd, room->first };
			}
			throw PlayerError{ room->first };
		}
		player.partner = room->first;
		host->second.partner = pfd;
		sendRoomData(Code::ropen, name, { uint8(false) });
	} else {
		try {
			sendb.pushHead(Code::cnjoin, Com::dataHeadSize + 1);
			sendb.push(uint8(false));
			sendb.send(pfd, player.webs);
		} catch (const Error& err) {
			sendb.clear();
			slog.err("failed to send join rejection to player ", pfd, ": ", err.what());
			throw PlayerError{ pfd };
		}
	}
}

static void leaveRoom(nsint pfd, Player& player, Code listCode = Code::rlist) {	// use Code::version to not send a room list
	uset<nsint> errPfds;
	umap<nsint, Player>::iterator partner = players.find(player.partner);
	if (umap<nsint, string>::iterator room = rooms.find(pfd); room == rooms.end()) {	// is a guest
		room = rooms.find(partner->first);
		sendRoomData(Code::ropen, room->second, { uint8(true) }, errPfds);
	} else if (partner == players.end()) {	// is a host without guest
		sendRoomData(Code::rerase, room->second, {}, errPfds);
		rooms.erase(room);
	} else {	// is host with guest
		sendRoomData(Code::ropen, room->second, { uint8(true) }, errPfds);
		rekeyRoom(room, partner->first);
	}

	if (partner != players.end()) {
		try {
			sendb.pushHead(Code::leave);
			sendb.send(partner->first, partner->second.webs);
		} catch (const Error& err) {
			slog.err("failed to send leave info from player ", pfd, " to player ", partner->first, ": ", err.what());
			errPfds.insert(partner->first);
		}
		player.partner = partner->second.partner = INVALID_SOCKET;
	}

	if (listCode != Code::version) {
		try {
			sendRoomList(pfd, player, listCode);
		} catch (const Error& err) {
			slog.err("failed to send room list to player ", pfd, ": ", err.what());
			errPfds.insert(pfd);
		}
	}
	if (!errPfds.empty())
		throw PlayerError(std::move(errPfds));
}

static void transferHost(nsint pfd, Player& player) {
	umap<nsint, Player>::iterator partner = players.find(player.partner);
	rekeyRoom(pfd, partner->first);
	try {
		sendb.pushHead(Code::thost);
		sendb.send(partner->first, partner->second.webs);
	} catch (const Error& err) {
		slog.err("failed to send host info from player ", pfd, " to player ", partner->first, ": ", err.what());
		throw PlayerError{ pfd, partner->first };	// host will have already changed its UI, so kick both
	}
}

static void globalMessage(uint8* data, nsint pfd, Player& player) {
	uset<nsint> errPfds;
	for (auto& [fd, pl] : players)
		if (fd != pfd && pl.partner == INVALID_SOCKET && !rooms.count(fd)) {
			try {
				player.recvb.redirect(fd, data, pl.webs);
			} catch (const Error& err) {
				errPfds.insert(fd);
				slog.err("failed to send global message to player ", fd, ": ", err.what());
			}
		}
	if (!errPfds.empty())
		throw PlayerError(std::move(errPfds));
}

static void redirectData(uint8* data, nsint pfd, Player& player) {
	if (Code(data[0]) < Code::hello || Code(data[0]) > Code::message) {
		slog.err("invalid net code ", uint(data[0]), " from player ", pfd, " of size ", read16(data + 1));
		throw PlayerError{ pfd };
	}
	umap<nsint, Player>::iterator partner = players.find(player.partner);
	if (partner == players.end()) {
		slog.err("data with code ", uint(data[0]), " from player ", pfd, " of size ", read16(data + 1), " to invalid partner ", player.partner);
		throw PlayerError{ pfd };
	}

	try {
		player.recvb.redirect(partner->first, data, partner->second.webs);
	} catch (const Error& err) {
		slog.err("failed to send data with code ", uint(data[0]), " of size ", read16(data + 1), " from player ", pfd, " to player ", partner->first, ": ", err.what());
		throw PlayerError{ partner->first };
	}
}

static void connectPlayer(vector<pollfd>& pfds) {
	if (players.size() >= maxPlayers) {
		sendRejection(pfds[0].fd);
		slog.out("rejected incoming connection");
	} else try {
		pfds.push_back({ acceptSocket(pfds[0].fd), POLLIN | POLLRDHUP, 0 });
		players.emplace(pfds.back().fd, Player());
		slog.out("player ", pfds.back().fd, " connected");
	} catch (const Error& err) {
		slog.err(err.what());
	}
}

static void disconnectPlayers(uint& icur, vector<pollfd>& pfds, const uset<nsint>& dfds) {
	for (nsint fd : dfds) {
		vector<pollfd>::iterator pit = std::find_if(pfds.begin() + 1, pfds.end(), [fd](const pollfd& it) -> bool { return it.fd == fd; });
		if (pit <= pfds.begin() + icur)
			--icur;

		if (umap<nsint, Player>::iterator player = players.find(fd); player != players.end()) {
			if (player->second.partner != INVALID_SOCKET || rooms.count(player->first))
				leaveRoom(player->first, player->second, Code::version);
			players.erase(player);
		}
		closeSocketV(fd);
		if (pit != pfds.end())
			pfds.erase(pit);
		slog.out("player ", fd, " disconnected");
	}
}

bool cprocValidate(nsint pfd, Player& player) {
	try {
		bool nameClash;
		switch (player.recvb.recvConn(pfd, player.webs, nameClash, [](const string& name) -> bool { return std::any_of(players.begin(), players.end(), [name](const pair<const nsint, Player>& it) -> bool { return it.second.name == name; }); })) {
		case Buffer::Init::wait:
			return false;
		case Buffer::Init::connect:
			sendConnRoomList(pfd, player, nameClash);
			break;
		case Buffer::Init::version:
			sendVersionRejection(pfd, player.webs);
		case Buffer::Init::error:
			throw PlayerError{ pfd };
		}
	} catch (const Error&) {
		throw PlayerError{ pfd };
	}
	return true;
}

bool cprocPlayer(nsint pfd, Player& player) {
	uint8* data;
	try {
		if (data = player.recvb.recv(pfd, player.webs); !data)
			return false;
	} catch (const Error&) {
		throw PlayerError{ pfd };
	}

	try {
		switch (Code(data[0])) {
		case Code::rnew:
			createRoom(data + dataHeadSize, pfd, player);
			break;
		case Code::glmessage:
			globalMessage(data, pfd, player);
			break;
		case Code::join:
			joinRoom(data + dataHeadSize, pfd, player);
			break;
		case Code::leave:
			leaveRoom(pfd, player);
			break;
		case Code::thost:
			transferHost(pfd, player);
			break;
		case Code::kick:
			leaveRoom(player.partner, players.at(player.partner), Code::kick);
			break;
		default:
			redirectData(data, pfd, player);
		}
	} catch (const PlayerError&) {
		player.recvb.clearCur(player.webs);
		throw;
	}
	player.recvb.clearCur(player.webs);
	return true;
}

#ifndef SERVICE
template <sizet S>
void printTable(vector<array<string, S>>& table, const char* title, array<string, S>&& header) {
	array<uint, S> lens{};
	table[0] = std::move(header);
	for (const array<string, S>& it : table)
		for (sizet i = 0; i < S; ++i)
			if (it[i].length() > lens[i])
				lens[i] = it[i].length();

	std::cout << title << linend;
	for (const array<string, S>& it : table) {
		for (sizet i = 0; i < S; ++i)
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
		vector<array<string, 2>> table(players.size() + 1);
		uint i = 1;
		for (auto& [pfd, player] : players)
			table[i++] = { toStr(pfd), player.partner != INVALID_SOCKET ? toStr(player.partner) : string() };
		printTable(table, "Players:", { "SOCKET", "PARTNER" });
		break; }
	case 'R': {
		vector<array<string, 3>> table(rooms.size() + 1);
		uint i = 1;
		for (auto& [host, name] : rooms) {
			Player& player = players.at(host);
			table[i++] = { name, toStr(host), player.partner != INVALID_SOCKET ? toStr(player.partner) : string() };
		}
		printTable(table, "Rooms:", { "NAME", "HOST", "GUEST" });
		break; }
	case 'Q':
		running = false;
		break;
	default:
		std::cerr << "unknown input: " << char(ch) << " (" << ch << ')' << std::endl;
	}
}
#endif

static void eventExit(int) {
	running = false;
}

static bool exec(vector<pollfd>& pfds) {
	if (int rcp = poll(pfds.data(), ulong(pfds.size()), checkTimeout)) {
		if (rcp < 0 || (pfds[0].revents & polleventsDisconnect)) {
			slog.err(msgPollFail);
			return running = false;
		}

		if (pfds[0].revents & POLLIN)
			connectPlayer(pfds);
		for (uint i = 1; i < pfds.size(); ++i) {
			try {
				if (pfds[i].revents & POLLIN) {
					Player& player = players.at(pfds[i].fd);
					bool fin = player.recvb.recvData(pfds[i].fd);
					while (player.cproc(pfds[i].fd, player));
					if (fin)
						throw PlayerError{ pfds[i].fd };
				} else if (pfds[i].revents & polleventsDisconnect)
					throw PlayerError{ pfds[i].fd };
			} catch (const PlayerError& err) {
				disconnectPlayers(i, pfds, err.pfds);
			} catch (...) {
				slog.err("unexpected error during player ", pfds[i].fd, " iteration");
				disconnectPlayers(i, pfds, { pfds[i].fd });
			}
		}
	}
#ifndef SERVICE
	checkInput();
#endif
	return running;
}

static int cleanup(const vector<pollfd>& pfds, int rc) {
	slog.out("exiting with code ", rc);
	for (vector<pollfd>::const_reverse_iterator it = pfds.rbegin(); it != pfds.rend(); ++it) {
		closeSocketV(it->fd);
		slog.out("socket ", it->fd, " closed");
	}
	slog.end();
#ifdef _WIN32
	WSACleanup();
#endif
	return rc;
}

#if defined(_WIN32) && !defined(__MINGW32__)
int wmain(int argc, wchar** argv) {
#else
int main(int argc, char** argv) {
#endif
#ifndef _WIN32
	signal(SIGQUIT, eventExit);
#endif
	signal(SIGINT, eventExit);
	signal(SIGABRT, eventExit);
	signal(SIGTERM, eventExit);

	vector<pollfd> pfds = { { INVALID_SOCKET, POLLIN | POLLRDHUP, 0 } };	// first element is server
	try {
		Arguments args(argc - 1, argv + 1, { arg4, arg6, argVerbose }, { argPort, argMaxPlayers, argLog, argMaxLogs });
		const char* maxLogs = args.getOpt(argMaxLogs);
		slog.start(args.hasFlag(argVerbose), args.getOpt(argLog), maxLogs ? toNum<uint>(maxLogs) : Log::defaultMaxLogfiles);

		const char* port = args.getOpt(argPort);
		if (!port)
			port = defaultPort;
		const char* playerLim = args.getOpt(argMaxPlayers);
		maxPlayers = playerLim ? std::min(toNum<uint>(playerLim), maxPlayersLimit) : defaultMaxPlayers;
		int family = AF_UNSPEC;
		if (args.hasFlag(arg4) && !args.hasFlag(arg6))
			family = AF_INET;
		else if (args.hasFlag(arg6) && !args.hasFlag(arg4))
			family = AF_INET6;

#ifdef _WIN32
		DWORD pid = GetCurrentProcessId();
		if (WSADATA wsad; WSAStartup(MAKEWORD(2, 2), &wsad))
			throw Error(msgWinsockFail);
#else
		pid_t pid = getpid();
#endif
		pfds[0].fd = bindSocket(port, family);
		slog.out(linend, "Thrones Server v", commonVersion, linend, "PID: ", pid, linend, "port: ", port, linend, "family: ", family == AF_INET ? "AF_INET" : family == AF_INET6 ? "AF_INET6" : "AF_UNSPEC", linend, "player limit: ", maxPlayers, linend, "room limit: ", maxRooms(), linend);
		randGen.seed(generateRandomSeed());
	} catch (const Error& err) {
		slog.err(err.what());
		return cleanup(pfds, EXIT_FAILURE);
	}

#if !defined(_WIN32) && !defined(SERVICE)
	Terminal term;	// here to set and reset the terminal
#endif
	try {
		while (exec(pfds));
	} catch (const std::runtime_error& err) {
		slog.err("runtime error: ", err.what());
		return cleanup(pfds, EXIT_FAILURE);
	} catch (...) {
		slog.err("unknown error");
		return cleanup(pfds, EXIT_FAILURE);
	}
	return cleanup(pfds, EXIT_SUCCESS);
}
