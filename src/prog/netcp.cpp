#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp() :
	tickproc(nullptr),
	socket(nsint(-1)),
	webs(false)
{}

Netcp::~Netcp() {
	if (socket != -1)
		closeSocket(socket);
}

void Netcp::connect() {
	connector = std::make_unique<Connector>(World::sets()->address.c_str(), World::sets()->port.c_str(), World::sets()->resolveFamily);
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
	bool final = recvb.recvData(socket);
	for (uint8* data; data = recvb.recv(socket, webs); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::version:
			throw Error("Server expected version " + readText(data));
		case Code::full:
			throw Error("Server full");
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
	if (final)
		throw Error(msgConnectionLost);
}

void Netcp::tickLobby() {
	if (!pollSocket(socket))
		return;
	bool final = recvb.recvData(socket);
	for (uint8* data; data = recvb.recv(socket, webs); recvb.clearCur(webs))
		switch (Code(data[0])) {
		case Code::rlist:
			World::program()->eventOpenLobby(data + dataHeadSize);
			break;
		case Code::rnew:
			World::state<ProgLobby>()->addRoom(readName(data + dataHeadSize));
			break;
		case Code::cnrnew:
			World::program()->eventHostRoomReceive(data);
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
			World::state<ProgRoom>()->updateConfigWidgets(World::game()->board.config);
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
	if (final)
		throw Error(msgConnectionLost);
}

void Netcp::tickGame() {
	if (!pollSocket(socket))
		return;
	bool final = recvb.recvData(socket);
	for (uint8* data; data = recvb.recv(socket, webs); recvb.clearCur(webs))
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
	if (!pollSocket(socket))
		return;
	for (bool final = recvb.recvData(socket);;)
		switch (recvb.recvConn(socket, webs)) {
		case Buffer::Init::wait:
			if (final)
				throw Error(msgConnectionLost);
			return;
		case Buffer::Init::connect:
			if (final)
				throw Error(msgConnectionLost);
			World::game()->sendStart();
			break;
		case Buffer::Init::version:
			sendVersion(socket, webs);
		case Buffer::Init::error:
			closeSocket(socket);
			tickproc = &Netcp::tickDiscard;
			return;
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
	server = bindSocket(World::sets()->port.c_str(), World::sets()->resolveFamily);
	tickproc = &NetcpHost::tickDiscard;
}

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(server);
}

void NetcpHost::tick() {
	if ((this->*tickproc)(); !World::netcp())	// in case this instance gets deleted
		return;
	if (pollSocket(server)) {
		if (socket != -1)
			sendRejection(server);
		else try {
			socket = acceptSocket(server);
			tickproc = &NetcpHost::tickValidate;
		} catch (const Error& err) {
			std::cerr << "failed to connect guest: " << err.message << std::endl;
		}
	}
}
