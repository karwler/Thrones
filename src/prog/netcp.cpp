#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp() :
	socket(-1),
	cncproc(nullptr),
	webs(false)
{
#ifdef EMSCRIPTEN
	waitSend = false;
#endif
}

Netcp::~Netcp() {
	if (socket != -1)
		closeSocket(socket);
}

void Netcp::connect() {
	socket = connectSocket(World::sets()->address.c_str(), World::sets()->port, World::sets()->resolveFamily);
#ifdef EMSCRIPTEN
	waitSend = true;
#else
	sendVersion(socket, webs);
#endif
	cncproc = &Netcp::cprocWait;
}

void Netcp::disconnect() {
	if (socket != -1) {
		if (webs)
			sendWaitClose(socket);
		closeSocket(socket);
	}
}

void Netcp::tick() {
#ifdef EMSCRIPTEN
	if (waitSend) {
		try {
			sendVersion(socket, webs);
			waitSend = false;
		} catch (const Error&) {}
	}
#endif
	if (pollSocket(socket))
		for (recvb.recvData(socket); (this->*cncproc)(););
}

bool Netcp::cprocWait() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return false;

	switch (Code(data[0])) {
	case Code::rlist:
		World::program()->info &= ~Program::INF_UNIQ;
		World::program()->eventOpenLobby(data + dataHeadSize);
		break;
	case Code::start:
		World::program()->info |= Program::INF_UNIQ;
		World::game()->recvStart(data + dataHeadSize);
		break;
	default:
		throw Error("Invalid response: " + toStr(*data));
	}
	recvb.clearCur(webs);
	return true;
}

bool Netcp::cprocLobby() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return false;

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
		World::program()->info &= ~Program::INF_GUEST_WAITING;
		World::state<ProgRoom>()->updateStartButton();
		break;
	case Code::hgone:
		World::program()->eventHostLeft(data + dataHeadSize);
		break;
	case Code::hello:
		World::program()->info |= Program::INF_GUEST_WAITING;
		World::program()->eventPlayerHello(true);
		break;
	case Code::cnjoin:
		World::program()->eventJoinRoomReceive(data + dataHeadSize);
		break;
	case Code::config:
		World::game()->recvConfig(data + dataHeadSize);
		World::state<ProgRoom>()->updateConfigWidgets(World::game()->getConfig());
		break;
	case Code::start:
		World::game()->recvStart(data + dataHeadSize);
		break;
	case Code::message:
		World::program()->eventRecvMessage(data);
		break;
	default:
		throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
	}
	recvb.clearCur(webs);
	return true;
}

bool Netcp::cprocGame() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return false;

	switch (Code(data[0])) {
	case Code::rlist:
		World::program()->uninitGame();
		World::program()->eventOpenLobby(data + dataHeadSize);
		break;
	case Code::leave:
		World::program()->eventPlayerLeft();
		break;
	case Code::hgone:
		World::program()->eventHostLeft(data + dataHeadSize);
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
	case Code::record:
		World::game()->recvRecord(data + dataHeadSize);
		break;
	case Code::message:
		World::program()->eventRecvMessage(data);
		break;
	default:
		throw Error("Invalid net code " + toStr(data[0]) + " of size " + toStr(read16(data + 1)));
	}
	recvb.clearCur(webs);
	return true;
}

bool Netcp::cprocValidate() {
	switch (recvb.recvConn(socket, webs)) {
	case Buffer::Init::wait:
		return false;
	case Buffer::Init::connect:
		World::game()->sendStart();
		break;
	case Buffer::Init::error:
		closeSocket(socket);
		cncproc = &NetcpHost::cprocDiscard;
	}
	return true;
}

bool Netcp::cprocDiscard() {
	if (uint8* data = recvb.recv(socket, webs)) {
		std::cerr << "unexprected data with code '" << uint(data[0]) << '\'' << std::endl;
		recvb.clear();
	}
	return false;
}

void Netcp::sendData(Code code) {
	uint8 data[dataHeadSize] = { uint8(code) };
	write16(data + 1, dataHeadSize);
	Com::sendData(socket, data, dataHeadSize, webs);
}

// HOST

NetcpHost::~NetcpHost() {
	if (server != -1)
		closeSocket(server);
}

void NetcpHost::connect() {
	server = bindSocket(World::sets()->port, World::sets()->resolveFamily);
	cncproc = &NetcpHost::cprocDiscard;
}

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(server);
}

void NetcpHost::tick() {
	if (socket != -1 && pollSocket(socket))
		for (recvb.recvData(socket); (this->*cncproc)(););
	if (pollSocket(server)) {
		if (socket != -1)
			discardAccept(server);
		else try {
			socket = acceptSocket(server);
			cncproc = &NetcpHost::cprocValidate;
		} catch (const Error& err) {
			std::cerr << "failed to connect guest: " << err.message << std::endl;
		}
	}
}
