//
// Created by PinkySmile on 07/04/2026.
//

#ifndef MIXRTS_GAMEMANAGER_HPP
#define MIXRTS_GAMEMANAGER_HPP


#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Window/Event.hpp>
#include "forward.hpp"
#include "Color.hpp"
#include "Vector.hpp"
#include "Utils.hpp"
#include "Unit.hpp"
#include "FindPath.hpp"

struct MapState {
	// OPTIMIZE: Align line size to PAGE_SIZE, or a power of 2
	std::vector<uint32_t> grid;
	PathFindingGrid pathFindingGrid;
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

	void clampedPosition(const Quad &abs, Vector2i &topLeft, Vector2i &bottomRight) const;
	void updatePlacementGrid(const Vector2i &rel, unsigned player, int delta);
	void updatePlacementGrid(const Quad &abs, unsigned player, int delta);
	void resetGridBits(const Vector2i &rel, uint32_t bits);
	void resetGridBits(const Quad &abs, uint32_t bits);
	void setGridBits(const Vector2i &rel, uint32_t bits);
	void setGridBits(const Quad &abs, uint32_t bits);
	[[nodiscard]] bool hasAnyGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] bool hasAllGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] bool hasExactlySomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits) const;
	[[nodiscard]] bool anyHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool anyHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool anyHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] bool allHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool allHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool allHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] Vector2i absToTileCoord(const Vector2i &) const;
	[[nodiscard]] Vector2i topLeftCoordToAbs(const Vector2i &) const;
	[[nodiscard]] Quad tileCoordToAbs(const Vector2i &) const;
};

struct LoadedMap {
	MapState grid;
	PathFindingGrid pathFindingGrid;
	std::vector<uint32_t> originalIndices;
	std::vector<uint32_t> displayedIndices;
	//std::vector<std::set<GameObject *>> quadrants;
	//std::vector<std::unique_ptr<GameObject>> mapObjects;
	//std::vector<ResourcePatch> resources;
	// ???? tileMeta

	sf::Texture tileSet;
	sf::RenderTexture renderCache;

	void load(const std::filesystem::path &path);
	void buildCache();
};

// #define QUADRANT_SIZE_X 128
// #define QUADRANT_SIZE_Y 128

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
	unsigned _lastObjectId = 0;
	std::unordered_map<unsigned, GameObject *> _objectMap;
	std::map<unsigned, std::list<GameObject *>> _renderObjects;
	unsigned char _debugGrid = 255;
	bool _isDebugUnlocked = false;

	template<typename T>
	void _checkObjectLocked(const std::string &name, T &object, Player &owner);
	template<typename T>
	void _unlockObject(Faction &f, const std::string &&name, T &object, Player &owner) = delete;
	void _unlockAll();
	void _recomputeLocked();
	bool _isInArea(const GameObject &obj, const Vector2i &pos, const Vector2u &size) const;
	void _checkScroll();
	void _renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale = {1, 1});
	void _onObjectSpawned(Vector2i pos, MapState &map, Player &owner, GameObject &obj, const std::string &id);
	template<typename T>
	T *_spawnObj(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, std::list<T> &out, const std::unordered_map<std::string, const T *> &in);

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
	GameObject *getObjectById(unsigned index);
	Unit *spawnUnit(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id);
	Building *spawnBuilding(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id);
	bool canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id, const Vector2i &basePos);
	const LoadedMap &getLoadedMap() const;
	void consumeEvent(const sf::Event &event);
	bool isAreaFree(const Vector2i &pos, const Vector2u &size);
	void getObjectsInArea(const Vector2i &pos, const Vector2u &size, std::vector<GameObject *> &objects);
	void registerObject(GameObject *o);
	void deregisterObject(GameObject *o);
};

#include "MapState.inl"


#endif //MIXRTS_GAMEMANAGER_HPP
