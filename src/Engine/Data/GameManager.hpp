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
#include "Debug/DebugOptions.hpp"

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

//! One object touched by a warhead (debug tool).
struct MYDLL_API WarheadTarget {
	unsigned objectId;
	Vector2f hitPoint;
	double distance;
	float multiplier;
	unsigned damage;
};

//! One warhead application (debug tool).
struct MYDLL_API WarheadHit {
	//! Tick of the first contact
	uint64_t tick;
	//! Tick of the last contact (differs from \a tick for hitTravel projectiles)
	uint64_t lastTick;
	Vector2f position;
	unsigned radius;
	unsigned baseDamage;
	unsigned hits;
	DamageType damageType;
	size_t owner;
	const Weapon *weapon;
	const Projectile *source;
	std::vector<WarheadTarget> targets;
};

class MYDLL_API GameManager {
public:
	struct GameSettings {
		bool shortGame;
	};

	bool finished = false;
	//! Debug tools settings, driven by the debug panel (F3).
	DebugOptions debugOptions;
	//! Set by projectiles around their calls to weaponUsedAt so that the debug tools know where a hit comes from.
	const Projectile *hitSource = nullptr;

	GameManager(const std::vector<Faction> &factions, size_t nbPlayers, const GameSettings &settings);
	~GameManager();
	void loadMap(const std::string &path);
	void addPlayer(size_t factionIndex, const HSLColor &color, Vector2i startingArea, unsigned team);
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
	//! Sets the camera zoom: 1 is the native resolution, 2 shows everything twice as big, 0.5 twice as small.
	void setCameraZoom(float zoom);
	[[nodiscard]] float getCameraZoom() const;
	//! Position, in the world, of what is displayed in the top left corner of the screen.
	[[nodiscard]] Vector2f getCameraTranslate() const;
	//! Size, in world units, of the part of the screen that isn't covered by the UI at the current zoom.
	[[nodiscard]] Vector2f getVisibleSize() const;
	//! Centers the camera in the middle of the (not covered by the UI) viewport on the rectangle
	//! starting at \a topLeft and of size \a size, and picks the biggest zoom (between \a minZoom
	//! and \a maxZoom) that fits all of it, plus \a margin world units on each side, in the viewport.
	//! Contrary to centerCameraOn(), the camera is not kept inside the map.
	void fitCameraTo(Vector2f topLeft, Vector2f size, float margin = 0, float minZoom = 0.05f, float maxZoom = 4.f);
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
	const GameSettings &getSettings() const;

	// Debug tools
	//! Opens/closes the debug panel
	void toggleDebugPanel();
	[[nodiscard]] bool isDebugPanelOpen() const;
	//! Toggles the "everything unlocked" debug mode
	void toggleDebugUnlock();
	[[nodiscard]] bool isDebugUnlocked() const;
	//! Makes the given player the one being controlled (the only enabled one)
	void setControlledPlayer(size_t index);
	//! Controls the next player (wraps around)
	void debugNextPlayer();
	//! Index of the first enabled player (0 if none)
	[[nodiscard]] size_t getControlledPlayer() const;
	//! Rotates all the objects selected by enabled players
	void debugRotateSelected();
	[[nodiscard]] bool isPaused() const;
	//! Runs a single tick while paused
	void requestStep();
	[[nodiscard]] uint64_t getTick() const;
	//! Number of selectable grid views (see DebugOptions::gridView)
	[[nodiscard]] size_t getGridViewCount() const;
	[[nodiscard]] std::string getGridViewLabel(unsigned char view) const;
	[[nodiscard]] const std::deque<WarheadHit> &getWarheadHits() const;

	inline void centerCameraOn(const GameObject &object)
	{
		this->centerCameraOn(object.getPosition() + object.getSize() / 2.f);
	}

	static void pushUserType(sol::state &state);

private:
	sf::Text _text;
	GameSettings _settings;
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
	std::unique_ptr<DebugOverlay> _debugOverlay;
	std::unique_ptr<DebugPanel> _debugPanel;
	std::deque<WarheadHit> _warheadHits;
	WarheadHit *_currentHit = nullptr;
	uint64_t _tick = 0;
	bool _hitThisTick = false;
	bool _isDebugUnlocked = false;
	bool _paused = false;
	bool _forceStep = false;

	template<typename T>
	void _checkObjectLocked(const std::string &name, T &object, Player &owner);
	void _validateArmors() const;
	void _unlockAll();
	void _recomputeLocked();

	GameObject *_spawnExtra(GameObject &parent, const ExtraSpawn &extra);

	void _checkScroll();
	//! Computes which stealthed objects are currently revealed by the enemy objects around them.
	void _updateStealthDetection();
	void _renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale = {1, 1}, bool drawGrid = false);
	void _initObject(MapState &map, Player &owner, GameObject &obj);
	void _onObjectSpawned(Vector2i pos, GameObject &obj);
	void _modUnitInArea(const MapState::Quad &quad, LoadedMap &state, Vector2u tileSize, size_t player, char mod);
	void _modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, pfGridType mask, size_t team, char mod, bool isStatic, bool crushable);
	void _pushOrder(GameObject &obj, const std::string &order);

	void _recordWarheadHit(const Weapon *weapon, Vector2f position, unsigned damage, size_t owner);
	void _recordWarheadTarget(const Weapon *weapon, const GameObject &obj, Vector2f impact, double dist, float multiplier, unsigned damage, size_t owner);
	//! Mouse position in world coordinates
	[[nodiscard]] Vector2f _getWorldMouse() const;
	//! Returns the render order key the cursor stands at
	[[nodiscard]] int64_t _getCursorRenderKey() const;
	//! Calls \a fct on each object that passes the render order filter
	template<typename F>
	void _forEachRenderObject(F fct);

	static bool _isInArea(const GameObject &obj, const Vector2i &pos, const Vector2u &size, bool onlySolid, bool partial);

	friend class DebugOverlay;
	friend class DebugPanel;
};

#include "MapState.inl"


#endif //MIXRTS_GAMEMANAGER_HPP
