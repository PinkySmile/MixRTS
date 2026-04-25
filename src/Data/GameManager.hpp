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
#include <SFML/Window/Event.hpp>
#include "forward.hpp"
#include "Color.hpp"
#include "Vector.hpp"
#include "Utils.hpp"
#include "Unit.hpp"

struct MapState {
	// OPTIMIZE: Align line size to PAGE_SIZE, or a power of 2
	std::vector<uint32_t> grid;
	std::vector<std::vector<uint8_t>> placementGrid;
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

	void clampedPosition(const Quad &abs, Vector2i &topLeft, Vector2i &bottomRight);
	void updatePlacementGrid(const Vector2i &rel, unsigned player, int delta);
	void updatePlacementGrid(const Quad &abs, unsigned player, int delta);
	void resetGridBits(const Vector2i &rel, uint32_t bits);
	void resetGridBits(const Quad &abs, uint32_t bits);
	void setGridBits(const Vector2i &rel, uint32_t bits);
	void setGridBits(const Quad &abs, uint32_t bits);
	bool hasAnyGridBits(const Vector2i &rel, uint32_t bits);
	bool hasAllGridBits(const Vector2i &rel, uint32_t bits);
	bool hasSomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits);
	bool anyHasAnyGridBits(const Quad &abs, uint32_t bits);
	bool anyHasAllGridBits(const Quad &abs, uint32_t bits);
	bool anyHasSomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected);
	bool allHasAnyGridBits(const Quad &abs, uint32_t bits);
	bool allHasAllGridBits(const Quad &abs, uint32_t bits);
	bool allHasSomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected);
	Vector2i absToTileCoord(const Vector2i &);
	Vector2i topLeftCoordToAbs(const Vector2i &);
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


#define GRIDMASK_CAN_BUILD (GRIDMASK_IS_FREE | GRIDMASK_BUILDABLE)
#define GRID_CRATER_TYPE(i) ((i & GRIDMASK_CRATER_TYPE) >> 24)
#define GRID_TERRAIN_TYPE(i) ((i & GRIDMASK_TERRAIN_TYPE) >> 0)

class GameManager {
private:
	float _cameraZoom = 1;
	Vector2f _cameraTranslate = {0, 0};
	LoadedMap _map;
	std::vector<MapState> _mapStates;
	std::vector<Player> _players;
	std::vector<Faction> _factions;
	std::list<Unit> _liveUnits;
	std::list<Building> _liveBuildings;
	std::map<unsigned, std::list<GameObject *>> _renderObjects;

	void _checkScroll();

public:
	GameManager(const std::vector<Faction> &factions);
	void loadMap(Mod &mod, const std::filesystem::path &path);
	void addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea, unsigned team);
	void update();
	void updateRender();
	void render();
	void renderAbove();
	std::vector<Player> &getPlayers();
	const std::vector<Faction> &getFactions() const;
	Unit *spawnUnit(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, bool forceConcrete);
	Building *spawnBuilding(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, bool forceConcrete);
	bool canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id);
	const LoadedMap &getLoadedMap() const;
	void consumeEvent(const sf::Event &event);
};


#endif //MIXRTS_GAMEMANAGER_HPP
