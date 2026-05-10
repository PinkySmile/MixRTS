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
#include <SFML/Window/Event.hpp>
#include "forward.hpp"
#include "Color.hpp"
#include "Vector.hpp"
#include "Utils.hpp"
#include "Unit.hpp"
#include "Map.hpp"

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
	void _unlockAll();
	void _recomputeLocked();
	bool _isInArea(const GameObject &obj, const Vector2i &pos, const Vector2u &size) const;
	void _checkScroll();
	void _renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale = {1, 1});
	void _onObjectSpawned(Vector2i pos, MapState &map, Player &owner, GameObject &obj, const std::string &id);
	template<typename T>
	T *_spawnObj(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, std::list<T> &out, const std::unordered_map<std::string, const T *> &in);
	void _modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, int count);

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
	void modUnitInArea(const MapState::Quad &quad, int count);
	void modUnitInArea(const MapState::Quad &quad, PathFindingGrid &special, Vector2i rel, Vector2u tileSize, int count);
};

#include "MapState.inl"


#endif //MIXRTS_GAMEMANAGER_HPP
