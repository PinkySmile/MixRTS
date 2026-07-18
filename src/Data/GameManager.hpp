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
#include "GameObject.hpp"
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
	std::list<GameObject> _liveObjects;
	unsigned _lastObjectId = 0;
	std::unordered_map<unsigned, GameObject *> _objectMap;
	std::map<unsigned, std::list<GameObject *>> _renderObjects;

	// DEBUG
	unsigned char _debugGrid = 255;
	bool _isDebugUnlocked = false;
	bool _limitCamera = true;

	template<typename T>
	void _checkObjectLocked(const std::string &name, T &object, Player &owner);
	void _unlockAll();
	void _recomputeLocked();
	bool _isInArea(const GameObject &obj, const Vector2i &pos, const Vector2u &size) const;
	void _checkScroll();
	void _renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale = {1, 1}, bool drawGrid = false);
	void _initObject(MapState &map, Player &owner, GameObject &obj, const std::string &id);
	void _onObjectSpawned(Vector2i pos, GameObject &obj);
	template<typename T>
	T *_spawnObj(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, std::list<T> &out, const std::unordered_map<std::string, const T *> &in);
	void _modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, pfGridType mask, char mod);
	void _pushOrder(GameObject &obj, const std::string &order);

public:
	GameManager(const std::vector<Faction> &factions, size_t nbPlayers);
	void loadMap(Mod &mod, const std::filesystem::path &path);
	void addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea, unsigned team);
	void update();
	void updateRender();
	void render();
	void renderAbove();
	std::vector<Player> &getPlayers();
	const std::vector<Faction> &getFactions() const;
	GameObject *getObjectById(unsigned index);
	GameObject *spawnObject(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id);
	bool canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id, const Vector2i &basePos);
	const LoadedMap &getLoadedMap() const;
	void consumeEvent(const sf::Event &event);
	bool isAreaFree(const Vector2i &pos, const Vector2u &size, const std::vector<unsigned> &exclusion = {}) const;
	void getObjectsInArea(const Vector2i &pos, const Vector2u &size, std::vector<GameObject *> &objects);
	void registerObject(GameObject *o);
	void deregisterObject(GameObject *o);
	void modUnitInArea(const MapState::Quad &quad, size_t team, int placementCount, pfGridType mask, char mod);
	void modUnitInArea(const MapState::Quad &quad, PathFindingGrid &special, Vector2i rel, Vector2u tileSize, size_t team, int placementCount, pfGridType mask, char mod);
	std::optional<Vector2i> getFirstHarvestableTile(MapState &state, Vector2i pos) const;
	std::optional<Vector2i> getClosestHarvestableTile(MapState &state, Vector2i pos) const;
	GameObject *getClosestResourceDump(const std::vector<std::string> &preferredResources, size_t player, Vector2i pos);
	MapResource &getResource(Vector2i pos);
	void consumeResource(Vector2i pos);
	void centerCameraOn(Vector2f pos);
	std::pair<std::string, unsigned> convertResource(const std::string &source, const std::vector<std::string> &expected, unsigned value);

	inline void centerCameraOn(const GameObject &object)
	{
		this->centerCameraOn(object.getPosition() + object.getSize() / 2.f);
	}
};

#include "MapState.inl"


#endif //MIXRTS_GAMEMANAGER_HPP
