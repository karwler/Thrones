#include "utils/utils.h"
#include <random>

class Game {
private:
	SDLNet_SocketSet socks;
	TCPsocket socket;
	uint8 rcvBuf[Server::bufSiz];
	void (Game::*conn)();

public:
	Game(string addr, uint16 port);
	~Game();

	void tick();

private:
	void connectionWait();
	void connectionGame();
};
