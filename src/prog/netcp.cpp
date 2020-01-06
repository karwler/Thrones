#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp(uint8 maxSockets) :
	socket(nullptr),
	cncproc(nullptr),
	webs(false)
{
#ifdef EMSCRIPTEN
	waitSend = false;
#endif
	if (!(socks = SDLNet_AllocSocketSet(maxSockets)))
		throw NetError(SDLNet_GetError());
}

Netcp::~Netcp() {
	closeSocket(socket);
	SDLNet_FreeSocketSet(socks);
}

void Netcp::connect() {
	openSockets(World::sets()->address.c_str(), socket);
#ifdef EMSCRIPTEN
	waitSend = true;
#else
	sendVersion(socket, webs);
#endif
	cncproc = &Netcp::cprocWait;
}

void Netcp::disconnect() {
	if (webs && socket)
		sendWaitClose(socket);
	closeSocket(socket);
}

void Netcp::openSockets(const char* host, TCPsocket& sock) {
	IPaddress address;
	if (SDLNet_ResolveHost(&address, host, World::sets()->port))
		throw NetError(SDLNet_GetError());
	if (!(sock = SDLNet_TCP_Open(&address)))
		throw NetError(SDLNet_GetError());
	if (SDLNet_TCP_AddSocket(socks, sock) < 0) {
		closeSocket(sock);
		throw NetError(SDLNet_GetError());
	}
}

void Netcp::closeSocket(TCPsocket& sock) {
	SDLNet_TCP_DelSocket(socks, sock);
	SDLNet_TCP_Close(sock);
	sock = nullptr;
}

void Netcp::tick() {
#ifdef EMSCRIPTEN
	if (waitSend) {
		try {
			sendVersion(socket, webs);
			waitSend = false;
		} catch (const NetError&) {}
	}
#endif
	if (SDLNet_CheckSockets(socks, 0) > 0)
		(this->*cncproc)();
}

void Netcp::cprocWait() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return;

	switch (Code(data[0])) {
	case Code::full:
		throw NetError("Server full");
	case Code::version:
		throw NetError("Server expected version " + readVersion(data));
	case Code::rlist:
		World::program()->info &= ~Program::INF_UNIQ;
		World::program()->eventOpenLobby(data + dataHeadSize);
		break;
	case Code::start:
		World::program()->info |= Program::INF_UNIQ;
		World::game()->recvStart(data + dataHeadSize);
		break;
	default:
		throw NetError("Invalid response: " + toStr(*data));
	}
	recvb.clear();
}

void Netcp::cprocLobby() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return;

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
	recvb.clear();
}

void Netcp::cprocGame() {
	uint8* data = recvb.recv(socket, webs);
	if (!data)
		return;

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
	recvb.clear();
}

void Netcp::cprocValidate() {
	switch (recvb.recvInit(socket, webs)) {
	case Buffer::Init::connect:
		World::game()->sendStart();
		break;
	case Buffer::Init::version:
		sendVersion(socket, webs);
	case Buffer::Init::error:
		closeSocket(socket);
		cncproc = &NetcpHost::cprocDiscard;
	}
}

void Netcp::cprocDiscard() {
	if (uint8* data = recvb.recv(socket, webs)) {
		std::cerr << "unexprected data with code '" << uint(data[0]) << '\'' << std::endl;
		recvb.clear();
	}
}

void Netcp::sendData(Code code) {
	uint8 data[dataHeadSize] = { uint8(code) };
	SDLNet_Write16(dataHeadSize, data + 1);
	if (SDLNet_TCP_Send(socket, data, dataHeadSize) != dataHeadSize)
		throw NetError(SDLNet_GetError());
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

void NetcpHost::disconnect() {
	Netcp::disconnect();
	closeSocket(server);
}

void NetcpHost::tick() {
	if (SDLNet_CheckSockets(socks, 0) <= 0)
		return;

	if (SDLNet_SocketReady(server)) {
		if (socket)
			sendRejection(server);
		else try {
			if (!(socket = SDLNet_TCP_Accept(server)))
				throw SDLNet_GetError();
			if (SDLNet_TCP_AddSocket(socks, socket) < 0)
				throw SDLNet_GetError();
			cncproc = &NetcpHost::cprocValidate;
		} catch (const char* err) {
			closeSocket(socket);
			std::cerr << "failed to connect guest: " << err << std::endl;
		}
	}
	(this->*cncproc)();
}
