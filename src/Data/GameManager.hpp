//
// Created by PinkySmile on 07/04/2026.
//

#ifndef MIXRTS_GAMEMANAGER_HPP
#define MIXRTS_GAMEMANAGER_HPP


#include <vector>
#include <deque>
#include <list>
#include <map>
#include "forward.hpp"
#include "Color.hpp"
#include "Vector.hpp"
#include "Utils.hpp"

struct MapState {
	uint32_t *grid;
	Vector2u tileSize;
	Vector2u gridSize;
	Vector2i translate;

	struct Quad {
		Vector2f pt1;
		Vector2f pt2;
		Vector2f pt3;
		Vector2f pt4;
	};

	Vector2i absToTileCoord(const Vector2i &);
	Quad tileCoordToAbs(const Vector2i &);
};

enum GridMask {
	GRIDMASK_HAS_CONCRETE = 0x80000000,
	GRIDMASK_IS_FREE      = 0x40000000,
	// GRIDMASK_IS_FREE = 0x0F000000,
};

class GameManager {
private:
	std::vector<MapState> _mapStates;
	std::vector<Player> _players;
	std::vector<Faction> _factions;
	std::list<Building> _liveBuildings;
	std::map<unsigned, std::list<Building *>> _renderBuildings;

public:
	GameManager(const std::vector<Faction> &factions);
	void loadMap();
	void addPlayer(size_t factionIndex, const HSLColor &color);
	void update();
	void render();
	void renderAbove();
	std::vector<Player> &getPlayers();
	void spawnBuilding(size_t owner, Vector2u pos, const std::string &buildQueue, const std::string &id, bool forceConcrete);
};


#endif //MIXRTS_GAMEMANAGER_HPP
