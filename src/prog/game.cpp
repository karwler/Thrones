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
	setTiles(tiles);
	setTiles(enemyTiles);
	setMidTiles();
	setPieces(pieces);
	setPieces(enemyPieces);
	setBoard();
	setScreen();

	sizet oi = 0;
	vector<Object*> objs(tiles.size() + enemyTiles.size() + midTiles.size() + pieces.size() + enemyPieces.size() + board.size() + 1);
	setObjectAddrs(tiles, objs, oi);
	setObjectAddrs(enemyTiles, objs, oi);
	setObjectAddrs(midTiles, objs, oi);
	setObjectAddrs(pieces, objs, oi);
	setObjectAddrs(enemyPieces, objs, oi);
	setObjectAddrs(board, objs, oi);
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
	switch (NetCode(*rcvBuf)) {
	case NetCode::setup:
		receiveSetup();
		conn = &Game::connectionMatch;
		for (Tile& it : board)	// make board not interactable cause it should be filled at all times anyway
			it.mode &= ~Object::INFO_RAYCAST;
		break;
	default:
		printInvalidCode();
	}
}

void Game::connectionMatch() {
	switch (NetCode(*rcvBuf)) {
	case NetCode::move:
		// TODO: the move
		break;
	default:
		printInvalidCode();
	}
}

void Game::setBoard() {
	sizet i = 0;
	for (int8 y = -homeHeight; y < homeHeight + 1; y++)
		for (int8 x = 0; x < boardSize.x; x++)
			board[i++] = Tile(vec2b(x, y), 0.f, Tile::Type::empty);
}

void Game::setScreen() {
	vector<Vertex> verts = {
		Vertex(vec3(-5.f, 0.f, 0.f), vec2(0.f, 1.f)),
		Vertex(vec3(-5.f, 2.6f, 0.f), vec2(0.f, 0.f)),
		Vertex(vec3(4.7f, 2.6f, 0.f), vec2(1.f, 0.f)),
		Vertex(vec3(4.7f, 0.f, 0.f), vec2(1.f, 1.f))
	};
	screen = Object(vec3(0.f, 0.f, -0.5f), vec3(0.f), vec3(1.f), verts, BoardObject::squareElements, nullptr, {120, 120, 120, 255}, Object::INFO_FILL);	// TODO: reset position
}

void Game::setMidTiles() {
	sizet id = 0;
	for (sizet c = 0; c < 2; c++)
		midTiles[id++] = Tile(vec2b(1, 0), 0.f, Tile::Type::plains);
	for (sizet c = 0; c < 2; c++)
		midTiles[id++] = Tile(vec2b(2, 0), 0.f, Tile::Type::forest);
	for (sizet c = 0; c < 2; c++)
		midTiles[id++] = Tile(vec2b(3, 0), 0.f, Tile::Type::mountain);
	for (sizet c = 0; c < 2; c++)
		midTiles[id++] = Tile(vec2b(4, 0), 0.f, Tile::Type::water);
	midTiles[id] = Tile(vec2b(5, 0), 0.f, Tile::Type::fortress);
}

void Game::setTiles(array<Tile, numTiles>& tiles) {
	sizet id = 0;
	for (sizet i = 0; i < numPlains; i++)
		tiles[id++] = Tile(vec2b(4, 1), 0.f, Tile::Type::plains);
	for (sizet i = 0; i < numForests; i++)
		tiles[id++] = Tile(vec2b(4, 2), 0.f, Tile::Type::forest);
	for (sizet i = 0; i < numMountains; i++)
		tiles[id++] = Tile(vec2b(4, 3), 0.f, Tile::Type::mountain);
	for (sizet i = 0; i < numWaters; i++)
		tiles[id++] = Tile(vec2b(4, 4), 0.f, Tile::Type::water);
	tiles[id] = Tile(vec2b(0, 3), 0.f, Tile::Type::fortress);
}

void Game::setPieces(array<Piece, numPieces>& pieces) {
	sizet id = 0;
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(-1), Piece::Type::ranger);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(-1), Piece::Type::spearman);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(-1), Piece::Type::crossbowman);
	pieces[id++] = Piece(vec2b(-1), Piece::Type::catapult);
	pieces[id++] = Piece(vec2b(-1), Piece::Type::trebuchet);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(-1), Piece::Type::lancer);
	pieces[id++] = Piece(vec2b(-1), Piece::Type::warhorse);
	for (sizet c = 0; c < 2; c++)
		pieces[id++] = Piece(vec2b(-1), Piece::Type::elephant);
	pieces[id++] = Piece(vec2b(-1), Piece::Type::dragon);
	pieces[id++] = Piece(vec2b(-1), Piece::Type::throne);
}

void Game::receiveSetup() {
	vec2b* pi = reinterpret_cast<vec2b*>(rcvBuf + sizeof(NetCode));
	for (sizet i = 0; i < enemyTiles.size(); i++)
		enemyTiles[i].setPos(*pi++);

	// TODO: figure out how to exchange middle tiles
	/*Tile::Type* ti = reinterpret_cast<Tile::Type*>(pi);
	for (sizet i = midTiles.size() / 2; i < midTiles.size() - 1; i++)
		midTiles[i].setType(*ti++);

	pi = reinterpret_cast<vec2b*>(ti);*/
	for (sizet i = 0; i < enemyPieces.size(); i++)
		enemyPieces[i].setPos(*pi++);
}

bool Game::connectFail() {
	World::scene()->setPopup(ProgState::createPopupMessage(SDLNet_GetError(), &Program::eventClosePopup));
	return false;
}

void Game::disconnectMessage(const string& msg) {
	disconnect();
	World::scene()->setPopup(ProgState::createPopupMessage(msg, &Program::eventClosePopup));
}
