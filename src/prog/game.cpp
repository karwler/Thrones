#include "engine/world.h"
#include <ctime>

Game::Game() :
	socks(nullptr),
	socket(nullptr),
	conn(nullptr),
	randDist(0, 1)
{
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(std::time(nullptr)));
	}
}

Game::~Game() {
	disconnect();
}

bool Game::connect() {
	IPaddress address;
	if (SDLNet_ResolveHost(&address, World::sets()->address.c_str(), World::sets()->port))
		return connectFail();
	if (!(socket = SDLNet_TCP_Open(&address)))
		return connectFail();

	socks = SDLNet_AllocSocketSet(1);
	SDLNet_TCP_AddSocket(socks, socket);
	conn = &Game::connectionWait;

	World::scene()->setPopup(ProgState::createPopupMessage("Waiting for player...", &Program::eventConnectCancel, "Cancel"));
	return true;
}

void Game::disconnect() {
	if (socket) {
		SDLNet_TCP_DelSocket(socks, socket);
		SDLNet_TCP_Close(socket);
		SDLNet_FreeSocketSet(socks);
		socket = nullptr;
		socks = nullptr;
	}
}

void Game::tick() {
	if (socket && SDLNet_CheckSockets(socks, 0) > 0 && SDLNet_SocketReady(socket))
		SDLNet_TCP_Recv(socket, rcvBuf, sizeof(rcvBuf)) > 0 ? (this->*conn)() : disconnectMessage("Connection lost");
}

vector<Object*> Game::initObjects() {
	// TODO: finish
	sizet oi = 0;
	vector<Object*> objs(tiles.size() + pieces.size() + enemyTiles.size() + enemyPieces.size() + 1);
	createTiles(tiles);
	for (Tile& it : tiles)
		objs[oi++] = &it;
	for (Piece& it : pieces) {
		it = Piece(-1, Piece::Type::throne);
		objs[oi++] = &it;
	}
	for (Tile& it : enemyTiles) {
		it = Tile(-1, Tile::Type::plains);
		objs[oi++] = &it;
	}
	for (Piece& it : enemyPieces) {
		it = Piece(-1, Piece::Type::throne);
		objs[oi++] = &it;
	}
	screen = createScreen();
	objs[oi] = &screen;
	return objs;
}

void Game::connectionWait() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::full:
		disconnectMessage("Server full");
		break;
	case NetCode::start:
		conn = &Game::connectionSetup;
		World::program()->eventOpenGame();
		break;
	default:
		printInvalidCode();
	}
}

void Game::connectionSetup() {

}

void Game::connectionMatch() {

}

void Game::createTiles(array<Tile, numTiles>& tiles) {
	sizet id = 0;
	for (sizet i = 0; i < numPlains; i++)
		tiles[id++] = Tile(vec2b(i, 0), Tile::Type::plains);
	for (sizet i = 0; i < numForests; i++)
		tiles[id++] = Tile(vec2b(i, 1), Tile::Type::forest);
	for (sizet i = 0; i < numMountains; i++)
		tiles[id++] = Tile(vec2b(i, 2), Tile::Type::mountain);
	for (sizet i = 0; i < numWaters; i++)
		tiles[id++] = Tile(vec2b(i, 3), Tile::Type::mountain);
}

void Game::createPieces(array<Piece, numPieces>& pieces) {
	// TODO: look up the types
}

Object Game::createScreen() {
	vector<Vertex> verts = {
		Vertex(vec3(-4.5f, 0.f, 0.f), vec2(1.f, 0.f)),
		Vertex(vec3(-4.5f, 5.f, 0.f), vec2(1.f, 1.f)),
		Vertex(vec3(4.5f, 5.f, 0.f), vec2(0.f, 1.f)),
		Vertex(vec3(4.5f, 0.f, 0.f), vec2(0.f, 0.f))
	};
	return Object(vec3(0.f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, {120, 120, 120, 255});
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}
