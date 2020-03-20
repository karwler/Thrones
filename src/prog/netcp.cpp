#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp() :
	tickproc(nullptr),
	socket(-1),
	webs(false)
{}

Netcp::~Netcp() {
	if (socket != -1)
		closeSocket(socket);
}

void Netcp::connect() {
	connector.reset(new Connector(World::sets()->address.c_str(), World::sets()->port, World::sets()->resolveFamily));
	tickproc = &Netcp::tickConnect;
}

void Netcp::disconnect() {
	if (socket != -1) {
		if (webs)
			sendWaitClose(socket);
		closeSocket(socket);
	}
}

void Netcp::tick() {
	(this->*tickproc)();
}

void Netcp::tickConnect() {
	if (nsint fd = connector->pollReady(); fd != -1) {
		connector.reset();
		socket = fd;
		sendVersion(socket, webs);
		tickproc = &Netcp::tickWait;
	}
}

void Netcp::tickWait() {
	if (!pollSocket(socket))
		return;
	for (recvb.recvData(socket);; recvb.clearCur(webs)) {
		uint8* data = recvb.recv(socket, webs);
		if (!data)
			break;

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
	}
}

void Netcp::tickLobby() {
	if (!pollSocket(socket))
		return;
	for (recvb.recvData(socket);; recvb.clearCur(webs)) {
		uint8* data = recvb.recv(socket, webs);
		if (!data)
			break;

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
	}
}

void Netcp::tickGame() {
	if (!pollSocket(socket))
		return;
	for (recvb.recvData(socket);; recvb.clearCur(webs)) {
		uint8* data = recvb.recv(socket, webs);
		if (!data)
			break;

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
	}
}

void Netcp::tickValidate() {
	if (!pollSocket(socket))
		return;
	for (recvb.recvData(socket);;)
		switch (recvb.recvConn(socket, webs)) {
		case Buffer::Init::wait:
			return;
		case Buffer::Init::connect:
			World::game()->sendStart();
			break;
		case Buffer::Init::error:
			closeSocket(socket);
			tickproc = &NetcpHost::tickDiscard;
			return;
		}
}

void Netcp::tickDiscard() {
	if (pollSocket(socket))
		if (recvb.recvData(socket); uint8* data = recvb.recv(socket, webs)) {
			std::cerr << "unexprected data with code '" << uint(data[0]) << '\'' << std::endl;
			recvb.clear();
		}
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
	tickproc = &NetcpHost::tickDiscard;
}

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(server);
}

void NetcpHost::tick() {
	if (socket != -1)
		(this->*tickproc)();
	if (pollSocket(server)) {
		if (socket != -1)
			discardAccept(server);
		else try {
			socket = acceptSocket(server);
			tickproc = &NetcpHost::tickValidate;
		} catch (const Error& err) {
			std::cerr << "failed to connect guest: " << err.message << std::endl;
		}
	}
}
