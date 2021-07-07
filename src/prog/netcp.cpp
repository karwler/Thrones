#include "netcp.h"
#include "program.h"
#include "progs.h"
#include <iostream>
using namespace Com;

// CONNECTOR

Connector::Connector(const char* addr, const char* port, int family) :
	inf(resolveAddress(addr, port, family))
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

Netcp::~Netcp() {
	if (sock.fd != INVALID_SOCKET)
		closeSocket(sock.fd);
}

void Netcp::connect(const Settings* sets) {
	connector = std::make_unique<Connector>(sets->address.c_str(), sets->port.c_str(), sets->getFamily());
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

bool Netcp::tickConnect() {
	if (nsint fd = connector->pollReady(); fd != INVALID_SOCKET) {
		connector.reset();
		sock.fd = fd;
		sendVersion(sock.fd, webs);
		tickproc = &Netcp::tickWait;
	}
	return false;
}

bool Netcp::tickWait() {
	if (!pollSocket(sock))
		return false;

	bool fin = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::version:
			throw Error("Server expected version " + readText(data));
		case Code::full:
			throw Error("Server full");
		case Code::rlist:
			prog->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::start:
			prog->eventStartUnique(data + dataHeadSize);
			break;
		default:
			throw Error("Invalid response: " + toStr(*data));
		}
	if (fin)
		throw Error(msgConnectionLost);
	return false;
}

bool Netcp::tickLobby() {
	if (!pollSocket(sock))
		return false;

	bool fin = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::rlist:
			prog->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::rnew:
			prog->getState<ProgLobby>()->addRoom(readName(data + dataHeadSize));
			break;
		case Code::cnrnew:
			prog->eventHostRoomReceive(data + dataHeadSize);
			break;
		case Code::rerase:
			prog->getState<ProgLobby>()->delRoom(readName(data + dataHeadSize));
			break;
		case Code::ropen:
			prog->getState<ProgLobby>()->openRoom(readName(data + dataHeadSize + 1), data[dataHeadSize]);
			break;
		case Code::leave:
			prog->eventRoomPlayerLeft();
			break;
		case Code::thost:
			prog->eventRecvHost(true);
			break;
		case Code::kick:
			prog->eventOpenLobby(data + dataHeadSize, "You got kicked");
			break;
		case Code::hello:
			prog->info |= Program::INF_GUEST_WAITING;
			prog->eventPlayerHello(true);
			break;
		case Code::cnjoin:
			prog->eventJoinRoomReceive(data + dataHeadSize);
			break;
		case Code::config:
			prog->eventRecvConfig(data + dataHeadSize);
			break;
		case Code::start:
			prog->getGame()->recvStart(data + dataHeadSize);
			break;
		case Code::message: case Code::glmessage:
			prog->eventRecvMessage(data);
			break;
		default:
			throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
		}
	if (fin)
		throw Error(msgConnectionLost);
	return false;
}

bool Netcp::tickGame() {
	if (!pollSocket(sock))
		return false;

	bool fin = recvb.recvData(sock.fd);
	for (uint8* data; (data = recvb.recv(sock.fd, webs)); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::rlist:
			prog->uninitGame();
			prog->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::leave:
			prog->eventGamePlayerLeft();
			break;
		case Code::hello:
			prog->info |= Program::INF_GUEST_WAITING;
			break;
		case Code::setup:
			prog->getGame()->recvSetup(data + dataHeadSize);
			break;
		case Code::move:
			prog->getGame()->recvMove(data + dataHeadSize);
			break;
		case Code::kill:
			prog->getGame()->recvKill(data + dataHeadSize);
			break;
		case Code::breach:
			prog->getGame()->recvBreach(data + dataHeadSize);
			break;
		case Code::tile:
			prog->getGame()->recvTile(data + dataHeadSize);
			break;
		case Code::record:
			if (prog->getGame()->recvRecord(data + dataHeadSize))	// it's possible that this instance gets deleted
				return true;
			break;
		case Code::message:
			prog->eventRecvMessage(data);
			break;
		default:
			throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
		}
	if (fin)
		throw Error(msgConnectionLost);
	return false;
}

bool Netcp::tickValidate() {
	if (!pollSocket(sock))
		return false;

	for (bool fin = recvb.recvData(sock.fd);;)
		switch (recvb.recvConn(sock.fd, webs)) {
		case Buffer::Init::wait:
			if (fin)
				throw Error(msgConnectionLost);
			return false;
		case Buffer::Init::connect:
			if (fin)
				throw Error(msgConnectionLost);
			prog->eventStartUnique();
			return false;
		case Buffer::Init::version:
			sendVersion(sock.fd, webs);
		case Buffer::Init::error:
			closeSocket(sock.fd);
			tickproc = &Netcp::tickDiscard;
			return false;
		}
}

bool Netcp::tickDiscard() {
	return false;
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

void NetcpHost::connect(const Settings* sets) {
	serv.fd = bindSocket(sets->port.c_str(), sets->getFamily());
	tickproc = &NetcpHost::tickDiscard;
}

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(serv.fd);
}

void NetcpHost::tick() {
	if (!(this->*tickproc)() && pollSocket(serv)) {	// check if the instance still exists in case it gets deleted during tick
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
