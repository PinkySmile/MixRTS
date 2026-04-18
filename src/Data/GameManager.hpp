//
// Created by PinkySmile on 07/04/2026.
//

#ifndef MIXRTS_GAMEMANAGER_HPP
#define MIXRTS_GAMEMANAGER_HPP


#include <vector>
#include <deque>
#include <list>
#include <map>
#include <SFML/Graphics/RenderTexture.hpp>
#include "forward.hpp"
#include "Color.hpp"
#include "Vector.hpp"
#include "Utils.hpp"

struct MapState {
	std::vector<uint32_t> grid;
	Vector2u tileSize;
	Vector2u gridSize;
	Vector2i translate;
	std::vector<Vector2u> startAreas;

	struct Quad {
		Vector2f pt1;
		Vector2f pt2;
		Vector2f pt3;
		Vector2f pt4;
	};

	Vector2i absToTileCoord(const Vector2i &);
	Quad tileCoordToAbs(const Vector2i &);
};

struct LoadedMap {
	MapState grid;
	std::vector<uint32_t> originalIndices;
	std::vector<uint32_t> displayedIndices;
	//std::vector<std::unique_ptr<Object>> mapObjects;
	//std::vector<ResourcePatch> resources;
	// ???? tileMeta

	sf::Texture tileSet;
	sf::RenderTexture renderCache;

	void load(const std::filesystem::path &path);
	void buildCache();
};

enum GridMask {
	GRIDMASK_HAS_CONCRETE      = 0x80000000,
	GRIDMASK_IS_FREE           = 0x40000000,
	GRIDMASK_INFANTRY_CAN_PASS = 0x20000000,
	GRIDMASK_VEHICULES_CAN_PASS= 0x10000000,
	GRIDMASK_WORM_CAN_PASS     = 0x08000000,
	GRIDMASK_BUILDABLE         = 0x04000000,
	GRIDMASK_CRATER_TYPE       = 0x03000000,
	GRIDMASK_TERRAIN_TYPE      = 0x0000000F,
};


#define GRID_CAN_BUILD(i) ((i & (GRIDMASK_IS_FREE | GRIDMASK_BUILDABLE)) == (GRIDMASK_IS_FREE | GRIDMASK_BUILDABLE))
#define GRID_CRATER_TYPE(i) ((i & GRIDMASK_CRATER_TYPE) >> 24)
#define GRID_TERRAIN_TYPE(i) ((i & GRIDMASK_TERRAIN_TYPE) >> 0)

class GameManager {
private:
	LoadedMap _map;
	std::vector<MapState> _mapStates;
	std::vector<Player> _players;
	std::vector<Faction> _factions;
	std::list<Building> _liveBuildings;
	std::map<unsigned, std::list<Building *>> _renderBuildings;

public:
	GameManager(const std::vector<Faction> &factions);
	void loadMap(Mod &mod, const std::filesystem::path &path);
	void addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea);
	void update();
	void updateRender();
	void render();
	void renderAbove();
	std::vector<Player> &getPlayers();
	const std::vector<Faction> &getFactions() const;
	void spawnBuilding(size_t owner, Vector2u pos, const std::string &buildQueue, const std::string &id, bool forceConcrete);
	const LoadedMap &getLoadedMap() const;
};


#endif //MIXRTS_GAMEMANAGER_HPP
