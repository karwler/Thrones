#include "types.h"

// RECORD

Record::Record(const pair<Piece*, Action>& last, umap<Piece*, bool>&& doProtect, Info tinf) :
	protects(std::move(doProtect)),
	lastAct(last),
	lastAss(nullptr, ACT_NONE),
	info(tinf)
{}

void Record::update(Piece* actor, Action action, bool regular) {
	(regular ? lastAct : lastAss) = pair(actor, action);
	umap<Piece*, Action>& rec = regular ? actors : assault;
	if (umap<Piece*, Action>::iterator it = rec.find(actor); it != actors.end())
		it->second |= action;
	else
		rec.emplace(actor, action);
}

void Record::addProtect(Piece* piece, bool strong) {
	protects.emplace(piece, strong);
	piece->alphaFactor = BoardObject::noEngageAlpha;
}

Action Record::actionsExhausted() const {
	uint8 moves = 0, swaps = 0;
	for (auto [pce, act] : actors) {
		moves += moves < 2 && bool(act & ACT_MOVE);
		swaps += !swaps && bool(act & ACT_SWAP) && pce->getType() != Com::Piece::warhorse;
		if (moves + swaps >= 2)
			return ACT_MS;
		if (act & ACT_AF)
			return ACT_AF;
		if (act & ACT_SPAWN)
			return ACT_SPAWN;
	}
	return ACT_NONE;
}

// DIJKSTRA

vector<uint16> Dijkstra::travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16, void*), uint16 (*const* vmov)(uint16, svec2), uint8 movSize, void* data) {
	// initialize graph
	uint16 area = size.x * size.y;
	vector<Adjacent> grid(area);
	for (uint16 i = 0; i < area; i++)
		if (grid[i].cnt = 0; stepable(i, data) || i == src)	// ignore rules for starting point cause it can be a blocking piece
			for (uint8 m = 0; m < movSize; m++)
				if (uint16 ni = vmov[m](i, size); ni < area && stepable(ni, data))
					grid[i].adj[grid[i].cnt++] = ni;

	vector<bool> visited(area, false);
	vector<uint16> dist(area, UINT16_MAX);
	dist[src] = 0;

	// calculate distances
	std::priority_queue<Node, vector<Node>, Comp> nodes;
	nodes.emplace(src, 0);
	do {
		uint16 u = nodes.top().id;
		nodes.pop();
		if (dist[u] < dlim && !visited[u]) {
			for (uint8 i = 0; i < grid[u].cnt; i++)
				if (uint16 v = grid[u].adj[i], du = dist[u] + 1; !visited[v] && du < dist[v]) {
					dist[v] = du;
					nodes.emplace(v, dist[v]);
				}
			visited[u] = true;
		}
	} while (!nodes.empty());
	return dist;
}

// TILE COL

TileCol::TileCol() :
	tl(nullptr),
	home(0),
	extra(0),
	size(0)
{}

void TileCol::update(const Com::Config& conf) {
	if (uint16 cnt = conf.homeSize.x * conf.homeSize.y; cnt != home) {
		home = cnt;
		extra = home + conf.homeSize.x;
		size = extra + home;

		delete[] tl;
		tl = new Tile[size];
	}
}

// PIECE COL

PieceCol::PieceCol() :
	pc(nullptr),
	num(0),
	size(0)
{}

void PieceCol::update(const Com::Config& conf, bool regular) {
	if (uint16 cnt = (conf.opts & Com::Config::setPieceBattle) && regular ? std::min(conf.countPieces(), conf.setPieceBattleNum) : conf.countPieces(); cnt != num) {
		num = cnt;
		size = cnt * 2;

		delete[] pc;
		pc = new Piece[size];
	}
}
