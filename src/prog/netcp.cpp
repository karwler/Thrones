#include "engine/world.h"
using namespace Com;

// GUEST

Netcp::Netcp() :
	sendSize(0),
	recvState(Code::none),
	recvPos(0),
	socks(nullptr),
	socket(nullptr),
	randGen(createRandomEngine()),
	randDist(0, 1)
{}

Netcp::~Netcp() {
	closeSockets(socket);
}

void Netcp::connect() {
	openSockets(World::sets()->address.c_str(), socket, 1);
}

void Netcp::openSockets(const char* host, TCPsocket& sock, uint8 num) {
	IPaddress address;
	if (SDLNet_ResolveHost(&address, host, World::sets()->port))
		throw SDLNet_GetError();
	if (!(sock = SDLNet_TCP_Open(&address)))
		throw SDLNet_GetError();

	socks = SDLNet_AllocSocketSet(num);
	SDLNet_TCP_AddSocket(socks, sock);
}

void Netcp::closeSocket(TCPsocket& sock) {
	SDLNet_TCP_DelSocket(socks, sock);
	SDLNet_TCP_Close(sock);
	sock = nullptr;
}

void Netcp::closeSockets(TCPsocket& last) {
	if (last) {
		closeSocket(last);
		SDLNet_FreeSocketSet(socks);
		socks = nullptr;
	}
}

void Netcp::tick() {
	if (SDLNet_CheckSockets(socks, 0) > 0)
		checkSocket();
}

void Netcp::checkSocket() {
	if (SDLNet_SocketReady(socket)) {
		if (int len = SDLNet_TCP_Recv(socket, recvb + recvPos, Com::recvSize - recvPos); len <= 0)
			World::game()->disconnect("Connection lost");
		else do {
			if (recvState == Code::none) {
				recvState = Code(*recvb);
				len--;
				recvPos++;
			}
			if (uint16 await = World::game()->getConfig().dataSize(recvState), left = await - recvPos; len >= left) {
				World::game()->processCode(recvState, recvb + 1);
				len -= left;
				std::copy_n(recvb + await, len, recvb);
				recvState = Code::none;
				recvPos = 0;
			} else {
				recvPos += len;
				len = 0;
			}
		} while (len);
	}
}

bool Netcp::sendData() {
	if (SDLNet_TCP_Send(socket, sendb, sendSize) == sendSize) {
		sendSize = 0;
		return true;
	}
	World::game()->disconnect(SDLNet_GetError());
	return false;
}

// HOST

NetcpHost::NetcpHost() :
	Netcp(),
	server(nullptr),
	tickfunc(&NetcpHost::tickWait)
{}

NetcpHost::~NetcpHost() {
	if (socket)
		closeSocket(socket);
	closeSockets(server);
}

void NetcpHost::connect() {
	openSockets(nullptr, server, maxPlayers);
}

void NetcpHost::tick() {
	if (SDLNet_CheckSockets(socks, 0) > 0)
		(this->*tickfunc)();
}

void NetcpHost::tickWait() {
	if (SDLNet_SocketReady(server)) {
		socket = SDLNet_TCP_Accept(server);
		SDLNet_TCP_AddSocket(socks, socket);
		tickfunc = &NetcpHost::tickGame;

		sendSize = World::game()->getConfig().dataSize(Code::setup);
		World::game()->getConfig().toComData(sendb);
		if (sendb[1] = random(); sendData()) {
			sendb[1] = !sendb[1];
			World::game()->processCode(Code::setup, sendb + 1);
			tickfunc = &NetcpHost::tickGame;
		}
	}
}

void NetcpHost::tickGame() {
	if (SDLNet_SocketReady(server))
		sendRejection(server);
	checkSocket();
}
