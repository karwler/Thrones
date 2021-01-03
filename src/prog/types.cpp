#include "types.h"
#include "server/server.h"
#include <queue>

// CONFIG

Config::Config() :
	homeSize(9, 4),
	battlePass(randomLimit / 2),
	opts(favorTotal | terrainRules | dragonStraight),
	victoryPointsNum(21),
	setPieceBattleNum(10),
	favorLimit(1),
	tileAmounts{ 14, 9, 5, 7 },
	middleAmounts{ 1, 1, 1, 1 },
	pieceAmounts{ 2, 2, 1, 1, 1, 2, 1, 1, 1, 1 },
	winThrone(1),
	winFortress(1),
	capturers(1 << Piece::throne)
{}

Config& Config::checkValues() {
	homeSize = glm::clamp(homeSize, minHomeSize, maxHomeSize);
	battlePass = std::min(battlePass, randomLimit);
	favorLimit = std::min(favorLimit, maxFavorMax);

	uint16 hsize = homeSize.x * homeSize.y, fort = hsize;
	if (opts & rowBalancing) {
		for (uint16& it : tileAmounts)
			if (it < homeSize.y)
				it = homeSize.y;
		uint16 tamt = floorAmounts(countTiles(), tileAmounts.data(), hsize, uint8(tileAmounts.size() - 1), homeSize.y);
		fort -= ceilAmounts(tamt, homeSize.y * 4 + homeSize.x - 4, tileAmounts.data(), uint8(tileAmounts.size() - 1));
	} else
		fort -= floorAmounts(countTiles(), tileAmounts.data(), hsize, uint8(tileAmounts.size() - 1));
	for (uint8 i = 0; fort > (homeSize.y - 1) * (homeSize.x - 2); i = i < tileAmounts.size() - 1 ? i + 1 : 0) {
		++tileAmounts[i];
		--fort;
	}

	uint16 mids = floorAmounts(countMiddles(), middleAmounts.data(), homeSize.x / 2, uint8(middleAmounts.size() - 1));
	uint16 mort = homeSize.x - mids * 2;
	if (opts & victoryPoints) {
		if ((opts & victoryPointsEquidistant) && !(homeSize.x % 2) && !mort)
			mort = homeSize.x - floorAmounts(mids, middleAmounts.data(), mids - 2, uint8(middleAmounts.size() - 1)) * 2;
		else if (!mort || ((opts & victoryPointsEquidistant) && (homeSize.x % 2 ? !(mort % 2) : mort % 2))) {
			--*std::find_if(middleAmounts.rbegin(), middleAmounts.rend(), [](uint16 amt) -> bool { return amt; });
			++mort;
		}
	}
	victoryPointsNum = std::clamp(victoryPointsNum, uint16(1), uint16(UINT16_MAX - mort));

	uint16 psize = floorAmounts(countPieces(), pieceAmounts.data(), hsize, uint8(pieceAmounts.size() - 1));
	if (!psize)
		psize = pieceAmounts[Piece::throne] = 1;
	if (!capturers)
		capturers = 0x3FF;	// all pieces set

	if (!(opts & victoryPoints)) {
		uint16 capCnt = 0;
		for (uint8 i = 0; i < Piece::lim; ++i)
			if (capturers & (1 << i))
				capCnt += pieceAmounts[i];
		winFortress = std::clamp(winFortress, uint16(0), std::min(fort, capCnt));

		winThrone = std::clamp(winThrone, uint16(0), pieceAmounts[Piece::throne]);
		if (!(winFortress || winThrone))
			if (winThrone = 1; !pieceAmounts[Piece::throne]) {
				if (psize == hsize)
					--*std::find_if(pieceAmounts.rbegin(), pieceAmounts.rend(), [](uint16 amt) -> bool { return amt; });
				++pieceAmounts[Piece::throne];
			}
		setPieceBattleNum = std::max(std::max(setPieceBattleNum, winFortress), winThrone);
	}
	return *this;
}

uint16 Config::floorAmounts(uint16 total, uint16* amts, uint16 limit, uint8 ei, uint16 floor) {
	for (uint8 i = ei; total > limit; i = i ? i - 1 : ei)
		if (amts[i] > floor) {
			--amts[i];
			--total;
		}
	return total;
}

uint16 Config::ceilAmounts(uint16 total, uint16 floor, uint16* amts, uint8 ei) {
	for (uint8 i = 0; total < floor; i = i < ei ? i + 1 : 0) {
		++amts[i];
		++total;
	}
	return total;
}

void Config::toComData(uint8* data, const string& name) const {
	*data++ = uint8(name.length());
	data = std::copy(name.begin(), name.end(), data);

	*data++ = uint8(homeSize.x);
	*data++ = uint8(homeSize.y);
	*data++ = battlePass;
	Com::write16(data, opts);
	Com::write16(data += sizeof(uint16), victoryPointsNum);
	Com::write16(data += sizeof(uint16), setPieceBattleNum);
	Com::write16(data += sizeof(uint16), favorLimit);

	for (uint16 ta : tileAmounts)
		Com::write16(data += sizeof(uint16), ta);
	for (uint16 ma : middleAmounts)
		Com::write16(data += sizeof(uint16), ma);
	for (uint16 pa : pieceAmounts)
		Com::write16(data += sizeof(uint16), pa);
	Com::write16(data += sizeof(uint16), winThrone);
	Com::write16(data += sizeof(uint16), winFortress);
	Com::write16(data += sizeof(uint16), capturers);
}

string Config::fromComData(const uint8* data) {
	string name = Com::readName(data);
	data += 1 + name.length();

	homeSize.x = *data++;
	homeSize.y = *data++;
	battlePass = *data++;
	opts = Option(Com::read16(data));
	victoryPointsNum = Com::read16(data += sizeof(uint16));
	setPieceBattleNum = Com::read16(data += sizeof(uint16));
	favorLimit = Com::read16(data += sizeof(uint16));

	for (uint16& ta : tileAmounts)
		ta = Com::read16(data += sizeof(uint16));
	for (uint16& ma : middleAmounts)
		ma = Com::read16(data += sizeof(uint16));
	for (uint16& pa : pieceAmounts)
		pa = Com::read16(data += sizeof(uint16));
	winThrone = Com::read16(data += sizeof(uint16));
	winFortress = Com::read16(data += sizeof(uint16));
	capturers = Com::read16(data += sizeof(uint16));
	return name;
}

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
		moves += bool(act & ACT_MOVE);
		swaps += !swaps && bool(act & ACT_SWAP) && pce->getType() != Piece::warhorse;
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

Dijkstra::Node::Node(uint16 pos, uint16 distance) :
	id(pos),
	dst(distance)
{}

vector<uint16> Dijkstra::travelDist(uint16 src, uint16 dlim, svec2 size, bool (*stepable)(uint16, void*), void* data) {
	// initialize graph
	uint16 area = size.x * size.y;
	vector<Adjacent> grid(area);
	for (uint16 i = 0; i < area; ++i)
		if (grid[i].cnt = 0; stepable(i, data) || i == src)	// ignore rules for starting point cause it can be a blocking piece
			for (uint16 (*const mov)(uint16, svec2) : adjacentIndex)
				if (uint16 ni = mov(i, size); ni < area && stepable(ni, data))
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
			for (uint8 i = 0; i < grid[u].cnt; ++i)
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
	home(0),
	extra(0),
	size(0)
{}

void TileCol::update(const Config& conf) {
	if (uint16 cnt = conf.homeSize.x * conf.homeSize.y; cnt != home) {
		home = cnt;
		extra = home + conf.homeSize.x;
		size = extra + home;
		tl = std::make_unique<Tile[]>(size);
	}
}

// PIECE COL

PieceCol::PieceCol() :
	num(0),
	size(0)
{}

void PieceCol::update(const Config& conf, bool regular) {
	if (uint16 cnt = (conf.opts & Config::setPieceBattle) && regular ? std::min(conf.countPieces(), conf.setPieceBattleNum) : conf.countPieces(); cnt != num) {
		num = cnt;
		size = cnt * 2;
		pc = std::make_unique<Piece[]>(size);
	}
}

// SETUP

void Setup::clear() {
	tiles.clear();
	mids.clear();
	pieces.clear();
}
