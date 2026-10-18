//
// Created by PinkySmile on 07/04/2026.
//

#ifndef MIXRTS_GAMEMANAGER_HPP
#define MIXRTS_GAMEMANAGER_HPP


#include <vector>
#include <deque>
#include <list>
#include <map>
#include <SFML/Window/Event.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Vector.hpp"
#include "Utils.hpp"
#include "GameObject.hpp"
#include "Map.hpp"

// #define QUADRANT_SIZE_X 128
// #define QUADRANT_SIZE_Y 128

struct MYDLL_API ShakeParams {
	unsigned time = 0;
	unsigned power = 0;
	unsigned interval = 1;
	bool freezeTime = false;

	ShakeParams() = default;
	ShakeParams(const nlohmann::json &j);

	static void pushUserType(sol::state &state);
};

class MYDLL_API GameManager {
private:
	sf::Text _text;
	ShakeParams _shake;
	unsigned _shakeTimer = 0;
	float _cameraZoom = 1;
	Vector2f _cameraTranslate = {0, 0};
	LoadedMap _map;
	std::vector<MapState> _mapStates;
	std::vector<Faction> _factions;
	std::vector<Player> _players;
	std::array<std::vector<Effect>, 5> _effects;
	std::list<Projectile> _liveProjectiles;
	std::list<std::unique_ptr<GameObject>> _liveObjects;
	unsigned _lastObjectId = 0;
	std::unordered_map<unsigned, GameObject *> _objectMap;
	std::map<int64_t, std::list<GameObject *>> _renderObjects;
	std::vector<unsigned> _preloadedSprites;

	// DEBUG
	unsigned char _debugGrid = 255;
	bool _isDebugUnlocked = false;
	bool _limitCamera = true;
	bool _paused = false;
	bool _forceStep = false;

	template<typename T>
	void _checkObjectLocked(const std::string &name, T &object, Player &owner);
	void _validateArmors() const;
	void _unlockAll();
	void _recomputeLocked();

	GameObject *_spawnExtra(GameObject &parent, const ExtraSpawn &extra);

	void _checkScroll();
	void _renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale = {1, 1}, bool drawGrid = false);
	void _initObject(MapState &map, Player &owner, GameObject &obj);
	void _onObjectSpawned(Vector2i pos, GameObject &obj);
	void _modUnitInArea(const MapState::Quad &quad, LoadedMap &state, Vector2u tileSize, size_t player, char mod);
	void _modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, pfGridType mask, size_t team, char mod, bool isStatic, bool crushable);
	void _pushOrder(GameObject &obj, const std::string &order);

	static bool _isInArea(const GameObject &obj, const Vector2i &pos, const Vector2u &size, bool onlySolid, bool partial);

public:
	GameManager(const std::vector<Faction> &factions, size_t nbPlayers);
	~GameManager();
	void loadMap(Mod &mod, const std::filesystem::path &path);
	void addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea, unsigned team);
	void update();
	void updateRender();
	void render();
	void renderAbove();
	void setPaused(bool paused = true);
	void renderMinimap(Vector2f pos, Vector2u size);
	std::vector<Player> &getPlayers();
	std::vector<Faction> &getFactions();
	const std::vector<Faction> &getFactions() const;
	GameObject *getObjectById(unsigned index);
	GameObject *findPickup(size_t owner, MoveReason reason, double dist, GameObject *ignore);
	GameObject *spawnObject(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id);
	void spawnCustomObject(size_t owner, Vector2i absPos, GameObject *object);
	bool canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id, const std::vector<unsigned> &exclusionList) const;
	const LoadedMap &getLoadedMap() const;
	void consumeEvent(const sf::Event &event);
	void consumeGeneralEvent(const sf::Event &event);
	bool isAreaFree(const Vector2i &pos, const Vector2u &size, const std::vector<unsigned> &exclusion = {}, bool stackable = false, bool notSolid = false) const;
	GameObject *getObjectInArea(const Vector2i &pos, const Vector2u &size, bool partial=true, const std::function<bool (GameObject *)> &fct={});
	void getObjectsInArea(const Vector2i &pos, const Vector2u &size, std::vector<GameObject *> &objects, bool partial=true, const std::function<bool (GameObject *)> &fct={});
	void getObjectsInArea(const Vector2i &pos, const Vector2u &size, std::vector<const GameObject *> &objects, bool partial=true, const std::function<bool (const GameObject *)> &fct={});
	void registerObject(GameObject *o);
	void deregisterObject(GameObject *o);
	void modUnitInArea(const MapState::Quad &quad, size_t player, int placementCount, pfGridType mask, char mod, bool isStatic, bool crushable);
	void modUnitInArea(Vector2i rel, Vector2u tileSize, size_t player, int placementCount, pfGridType mask, char mod, bool isStatic, bool crushable);
	//! Returns the first resource tile (in loaded map grid coordinates) in the tile at \a pos
	//! (in the harvester's map coordinates) that \a harvester can currently stand on.
	std::optional<Vector2i> getFirstHarvestableTile(const GameObject &harvester, Vector2i pos) const;
	//! Returns the closest resource tile (in the harvester's map coordinates) to \a pos that
	//! \a harvester can currently stand on.
	std::optional<Vector2i> getClosestHarvestableTile(const GameObject &harvester, Vector2i pos) const;
	GameObject *getClosestResourceDump(const std::vector<std::string> &preferredResources, size_t player, Vector2i pos);
	MapResource &getResource(Vector2i pos);
	void consumeResource(Vector2i pos);
	void centerCameraOn(Vector2f pos);
	float initUnitTiles(const std::vector<std::vector<BuildingMask>> &tiles, Vector2i pos, MapState &map);
	void deinitUnitTiles(const std::vector<std::vector<BuildingMask>> &tiles, Vector2i pos, MapState &map);
	std::pair<std::string, unsigned> convertResource(const std::string &source, const std::vector<std::string> &expected, unsigned value);
	Effect &spawnEffect(const Effect &effect, Vector2f pos, const Player *owner);
	Projectile &spawnProjectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, const GameObject *target, Vector2f targetPos);
	void weaponUsedAt(unsigned damage, const Weapon *weapon, Vector2f position, size_t owner);
	void weaponUsedOn(unsigned damage, const Weapon *weapon, double dist, size_t owner, GameObject &obj);
	void shakeScreen(const ShakeParams &params);
	void preloadAllSprites();
	void preloadSprites();

	inline void centerCameraOn(const GameObject &object)
	{
		this->centerCameraOn(object.getPosition() + object.getSize() / 2.f);
	}

	static void pushUserType(sol::state &state);
};

#include "MapState.inl"


#endif //MIXRTS_GAMEMANAGER_HPP
