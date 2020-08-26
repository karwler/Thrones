#include "netcp.h"
#include "progs.h"
#include "engine/world.h"
#include <iostream>
using namespace Com;

// CONNECTOR

Connector::Connector(const char* addr, const char* port, int family) :
	inf(resolveAddress(addr, port, family)),
	sock{ INVALID_SOCKET, POLLOUT, 0 }
{
	if (!inf)
		throw Error(msgConnectionFail);
	nextAddr(inf);
}

Connector::~Connector() {
	if (sock.fd != INVALID_SOCKET) {
		noblockSocket(sock.fd, false);
		closeSocket(sock.fd);
	}
	freeaddrinfo(inf);
}

nsint Connector::pollReady() {
	int rc = poll(&sock, 1, 0);
	if (!rc)
		return INVALID_SOCKET;
	if (rc < 0)
		throw Error(msgConnectionFail);
	if (sock.revents & POLLOUT) {	// errors can be handled later in tick poll
		int err;
		if (socklent len = sizeof(err); !getsockopt(sock.fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len) && !err && !noblockSocket(sock.fd, false)) {
			nsint ret = sock.fd;
			sock.fd = INVALID_SOCKET;
			return ret;
		}
	}
	noblockSocket(sock.fd, false);
	closeSocket(sock.fd);
	nextAddr(cur->ai_next);
	return INVALID_SOCKET;
}

void Connector::nextAddr(addrinfo* nxt) {
	for (cur = nxt; cur; cur = cur->ai_next) {
		if (sock.fd = createSocket(cur->ai_family, 0); sock.fd == INVALID_SOCKET)
			continue;
		if (noblockSocket(sock.fd, true)) {
			closeSocket(sock.fd);
			continue;
		}

#ifdef _WIN32
		if (!connect(sock.fd, cur->ai_addr, socklent(cur->ai_addrlen)) || WSAGetLastError() == WSAEWOULDBLOCK)
#else
		if (!connect(sock.fd, cur->ai_addr, cur->ai_addrlen) || errno == EINPROGRESS)
#endif
			return;
		noblockSocket(sock.fd, false);
		closeSocket(sock.fd);
	}
	throw Error(msgConnectionFail);
}

// GUEST

Netcp::Netcp() :
	tickproc(nullptr),
	sock{ INVALID_SOCKET, POLLIN | POLLRDHUP, 0 },
	webs(false)
{}

Netcp::~Netcp() {
	if (sock.fd != INVALID_SOCKET)
		closeSocket(sock.fd);
}

void Netcp::connect() {
	connector = std::make_unique<Connector>(World::sets()->address.c_str(), World::sets()->port.c_str(), World::sets()->getFamily());
	tickproc = &Netcp::tickConnect;
}

void Netcp::disconnect() {
	if (sock.fd != INVALID_SOCKET) {
		if (webs)
			sendWaitClose(sock.fd);
		closeSocket(sock.fd);
	}
}

void Netcp::tick() {
	(this->*tickproc)();
}

void Netcp::tickConnect() {
	if (nsint fd = connector->pollReady(); fd != INVALID_SOCKET) {
		connector.reset();
		sock.fd = fd;
		sendVersion(sock.fd, webs);
		tickproc = &Netcp::tickWait;
	}
}

void Netcp::tickWait() {
	if (!pollSocket(sock))
		return;
	bool final = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::version:
			throw Error("Server expected version " + readText(data));
		case Code::full:
			throw Error("Server full");
		case Code::rlist:
			World::program()->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::start:
			World::program()->eventStartUnique(data + dataHeadSize);
			break;
		default:
			throw Error("Invalid response: " + toStr(*data));
		}
	if (final)
		throw Error(msgConnectionLost);
}

void Netcp::tickLobby() {
	if (!pollSocket(sock))
		return;
	bool final = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::rlist:
			World::program()->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::rnew:
			World::state<ProgLobby>()->addRoom(readName(data + dataHeadSize));
			break;
		case Code::cnrnew:
			World::program()->eventHostRoomReceive(data + dataHeadSize);
			break;
		case Code::rerase:
			World::state<ProgLobby>()->delRoom(readName(data + dataHeadSize));
			break;
		case Code::ropen:
			World::state<ProgLobby>()->openRoom(readName(data + dataHeadSize + 1), data[dataHeadSize]);
			break;
		case Code::leave:
			World::program()->eventRoomPlayerLeft();
			break;
		case Code::thost:
			World::program()->eventRecvHost(true);
			break;
		case Code::kick:
			World::program()->eventOpenLobby(data + dataHeadSize, "You got kicked");
			break;
		case Code::hello:
			World::program()->info |= Program::INF_GUEST_WAITING;
			World::program()->eventPlayerHello(true);
			break;
		case Code::cnjoin:
			World::program()->eventJoinRoomReceive(data + dataHeadSize);
			break;
		case Code::config:
			World::program()->eventRecvConfig(data + dataHeadSize);
			break;
		case Code::start:
			World::game()->recvStart(data + dataHeadSize);
			break;
		case Code::message: case Code::glmessage:
			World::program()->eventRecvMessage(data);
			break;
		default:
			throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
		}
	if (final)
		throw Error(msgConnectionLost);
}

void Netcp::tickGame() {
	if (!pollSocket(sock))
		return;
	bool final = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::rlist:
			World::program()->uninitGame();
			World::program()->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::leave:
			World::program()->eventGamePlayerLeft();
			break;
		case Code::hello:
			World::program()->info |= Program::INF_GUEST_WAITING;
			break;
		case Code::setup:
			World::game()->recvSetup(data + dataHeadSize);
			break;
		case Code::move:
			World::game()->recvMove(data + dataHeadSize);
			break;
		case Code::kill:
			World::game()->recvKill(data + dataHeadSize);
			break;
		case Code::breach:
			World::game()->recvBreach(data + dataHeadSize);
			break;
		case Code::tile:
			World::game()->recvTile(data + dataHeadSize);
			break;
		case Code::record:
			if (World::game()->recvRecord(data + dataHeadSize); !World::netcp())	// it's possible that this instance gets deleted
				return;
			break;
		case Code::message:
			World::program()->eventRecvMessage(data);
			break;
		default:
			throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
		}
	if (final)
		throw Error(msgConnectionLost);
}

void Netcp::tickValidate() {
	if (!pollSocket(sock))
		return;
	for (bool final = recvb.recvData(sock.fd);;)
		switch (recvb.recvConn(sock.fd, webs)) {
		case Buffer::Init::wait:
			if (final)
				throw Error(msgConnectionLost);
			return;
		case Buffer::Init::connect:
			if (final)
				throw Error(msgConnectionLost);
			World::program()->eventStartUnique();
			break;
		case Buffer::Init::version:
			sendVersion(sock.fd, webs);
		case Buffer::Init::error:
			closeSocket(sock.fd);
			tickproc = &Netcp::tickDiscard;
			return;
		}
}

bool Netcp::pollSocket(pollfd& sock) {
	if (int rc = poll(&sock, 1, 0)) {
		if (rc < 0)
			throw Error(msgPollFail);
		if (sock.revents & POLLIN)
			return true;	// recv can handle disconnect
		if (sock.revents & polleventsDisconnect)
			throw Error(msgConnectionLost);
	}
	return false;
}

void Netcp::sendData(Code code) {
	uint8 data[dataHeadSize] = { uint8(code) };
	write16(data + 1, dataHeadSize);
	Com::sendData(sock.fd, data, dataHeadSize, webs);
}

// HOST

NetcpHost::~NetcpHost() {
	if (serv.fd != INVALID_SOCKET)
		closeSocket(serv.fd);
}

void NetcpHost::connect() {
	serv.fd = bindSocket(World::sets()->port.c_str(), World::sets()->getFamily());
	tickproc = &NetcpHost::tickDiscard;
}

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(serv.fd);
}

void NetcpHost::tick() {
	(this->*tickproc)();
	if (World::netcp() && pollSocket(serv)) {	// check if the instance still exists in case it gets deleted during tick
		if (sock.fd != INVALID_SOCKET)
			sendRejection(serv.fd);
		else try {
			sock.fd = acceptSocket(serv.fd);
			tickproc = &NetcpHost::tickValidate;
		} catch (const Error& err) {
			std::cerr << "failed to connect guest: " << err.what() << std::endl;
		}
	}
}
