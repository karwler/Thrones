#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp(uint8 maxSockets) :
	socket(nullptr),
	cncproc(nullptr)
{
	if (!(socks = SDLNet_AllocSocketSet(maxSockets)))
		throw SDLNet_GetError();
}

Netcp::~Netcp() {
	closeSocket(socket);
	SDLNet_FreeSocketSet(socks);
}

void Netcp::connect() {
	openSockets(World::sets()->address.c_str(), socket);
	sendVersion();
	cncproc = &Netcp::cprocWait;
}

void Netcp::openSockets(const char* host, TCPsocket& sock) {
	IPaddress address;
	if (SDLNet_ResolveHost(&address, host, World::sets()->port))
		throw SDLNet_GetError();
	if (!(sock = SDLNet_TCP_Open(&address)))
		throw SDLNet_GetError();
	if (SDLNet_TCP_AddSocket(socks, sock) < 0) {
		closeSocket(sock);
		throw SDLNet_GetError();
	}
}

void Netcp::closeSocket(TCPsocket& sock) {
	SDLNet_TCP_DelSocket(socks, sock);
	SDLNet_TCP_Close(sock);
	sock = nullptr;
}

void Netcp::tick() {
	if (SDLNet_CheckSockets(socks, 0) > 0)
		checkSocket();
}

void Netcp::cprocWait(uint8* data) {
	switch (Code(data[0])) {
	case Code::full:
		throw NetcpException("Server full");
	case Code::version:
		throw NetcpException("Server expected version " + readVersion(data));
	case Code::rlist:
		World::program()->info &= ~Program::INF_UNIQ;
		World::program()->eventOpenLobby(data + dataHeadSize);
		break;
	case Code::start:
		World::program()->info |= Program::INF_UNIQ;
		World::game()->recvStart(data + dataHeadSize);
		break;
	default:
		throw NetcpException("Invalid response: " + toStr(*data));
	}
}

void Netcp::cprocLobby(uint8* data) {
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
	default:
		std::cerr << "invalid net code '" << uint(data[0]) << "' of size '" << SDLNet_Read16(data + 1) << "' while in lobby" << std::endl;
	}
}

void Netcp::cprocGame(uint8* data) {
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
	default:
		std::cerr << "invalid net code '" << uint(data[0]) << "' of size '" << SDLNet_Read16(data + 1) << "' while in game" << std::endl;
	}
}

void Netcp::checkSocket() {
	if (recvb.recv(socket, cncproc))
		throw NetcpException("Connection lost");
}

void Netcp::sendVersion() {
	uint16 len = uint16(strlen(commonVersion));
	vector<uint8> data(Com::dataHeadSize + len);
	data[0] = uint8(Code::version);
	SDLNet_Write16(uint16(data.size()), data.data() + 1);
	std::copy_n(commonVersion, len, data.data() + dataHeadSize);
	sendData(data);
}

void Netcp::sendData(Buffer& sendb) {
	if (const char* err = sendb.send(socket))
		throw NetcpException(err);
}

void Netcp::sendData(Com::Code code) {
	uint8 data[Com::dataHeadSize] = { uint8(code) };
	SDLNet_Write16(Com::dataHeadSize, data + 1);
	if (SDLNet_TCP_Send(socket, data, Com::dataHeadSize) != Com::dataHeadSize)
		throw NetcpException(SDLNet_GetError());
}

// HOST

NetcpHost::NetcpHost() :
	Netcp(2),
	server(nullptr)
{}

NetcpHost::~NetcpHost() {
	closeSocket(server);
}

void NetcpHost::connect() {
	openSockets(nullptr, server);
	cncproc = &NetcpHost::cprocDiscard;
}

void NetcpHost::tick() {
	if (SDLNet_CheckSockets(socks, 0) <= 0)
		return;

	if (SDLNet_SocketReady(server)) {
		if (socket)
			sendRejection(server);
		else try {
			if (!(socket = SDLNet_TCP_Accept(server)))
				throw NetcpException(SDLNet_GetError());
			if (SDLNet_TCP_AddSocket(socks, socket) < 0)
				throw NetcpException(SDLNet_GetError());
			cncproc = &NetcpHost::cprocValidate;
		} catch (const NetcpException& err) {
			closeSocket(socket);
			std::cerr << "failed to connect guest: " << err.message << std::endl;
		}
	}
	checkSocket();
}

void NetcpHost::cprocValidate(uint8* data) {
	static_cast<NetcpHost*>(World::netcp())->validate(data);
}

void NetcpHost::validate(uint8* data) {
	if (Code(*data) == Code::version) {
		if (string ver = readVersion(data); std::find(compatibleVersions.begin(), compatibleVersions.end(), ver) != compatibleVersions.end())
			World::game()->sendStart();
		else {
			sendVersion();
			closeSocket(socket);
			cncproc = &NetcpHost::cprocDiscard;
			std::cout << "failed to validate guest of version " << ver << std::endl;
		}
	} else {
		cprocDiscard(data);
		closeSocket(socket);
		cncproc = &NetcpHost::cprocDiscard;
	}
}
