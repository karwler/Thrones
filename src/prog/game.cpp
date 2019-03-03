#include "engine/world.h"

Game::Game(string addr, uint16 port) :
	socks(nullptr),
	socket(nullptr),
	conn(&Game::connectionWait)
{
	IPaddress address;
	if (SDLNet_ResolveHost(&address, addr.c_str(), port))
		throw std::runtime_error(SDLNet_GetError());
	if (!(socket = SDLNet_TCP_Open(&address)))
		throw std::runtime_error(SDLNet_GetError());
	socks = SDLNet_AllocSocketSet(1);
	SDLNet_TCP_AddSocket(socks, socket);
}

Game::~Game() {
	SDLNet_TCP_DelSocket(socks, socket);
	SDLNet_TCP_Close(socket);
	SDLNet_FreeSocketSet(socks);
}

void Game::tick() {
	if (SDLNet_CheckSockets(socks, 0) > 0 && SDLNet_SocketReady(socket)) {
		if (SDLNet_TCP_Recv(socket, rcvBuf, sizeof(rcvBuf)) > 0)
			(this->*conn)();
		else
			World::program()->eventConnectFailed("Connection lost");
	}
}

void Game::connectionWait() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::full:
		World::program()->eventConnectFailed("Server full");
		break;
	case NetCode::start:
		conn = &Game::connectionGame;
		World::program()->eventConnectConnected();
		break;
	default:
		std::cerr << "Invalid net code " << uint(*rcvBuf) << std::endl;
	}
}

void Game::connectionGame() {
	// TODO: stuff
}
