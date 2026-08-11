//
// Created by PinkySmile on 20/04/2026.
//

#ifndef MIXRTS_GAMEOBJECT_HPP
#define MIXRTS_GAMEOBJECT_HPP


#include <string>
#include <memory>
#include <deque>
#include <set>
#include <unordered_map>
#include "Vector.hpp"
#include "forward.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"
#include "Screen.hpp"
#include "Utils.hpp"
#include "MultiSound.hpp"
#include "FindPath.hpp"
#include "ConditionalAnim.hpp"

struct TurnOrder;
struct MoveOrder;
struct HarvestOrder;
struct DeployOrder;
struct DumpOrder;
struct MoveCheckOrder;
struct AttackUnitOrder;
struct FinishMovementOrder;
typedef std::variant<
	TurnOrder,
	MoveOrder,
	DeployOrder,
	HarvestOrder,
	DumpOrder,
	MoveCheckOrder,
	AttackUnitOrder,
	FinishMovementOrder
> Order;

struct TurnOrder {
	unsigned resultDir;
};
struct MoveOrder {
	Vector2i destination;
	bool force;
};
struct FinishMovementOrder {};
struct HarvestOrder {};
struct DeployOrder {};
struct DumpOrder {
	GameObject *target;
};
struct MoveCheckOrder {
	void (GameObject::*fail)();
	Vector2i destination;
};
struct AttackUnitOrder {
	GameObject *target;
};

// Result of resolving a dynamic collision while a grid-bound unit moves.
enum CollisionResolution {
	COLLISION_PROCEED, // The way ahead is clear (possibly on a freshly found path).
	COLLISION_WAIT,    // Blocked for now; keep the order and retry next frame.
	COLLISION_ABORT    // Nothing can be done; the unit must stop here.
};



struct ExtraSpawn {
	std::string unit;
	Vector2i pos;
};

struct DumpInfo {
	unsigned dir;
	unsigned delay;
	unsigned delayAfter;
	std::unique_ptr<ConditionalAnim> during;
	std::unique_ptr<ConditionalAnim> start;
	std::unique_ptr<ConditionalAnim> stop;
};

struct ResourceLot {
	std::unordered_map<std::string, unsigned> value;
};

union BuildingMask {
	struct {
		bool placementConcrete: 1; // 1
		bool solid: 1;             // 2
		bool placementRequired: 1; // 4
	};
	uint8_t value;
};

struct UnitDeployParams {
	struct SpawnBuilding {
		std::string building;
		std::string queue;
		Vector2i offset;
	};

	std::optional<unsigned> direction;
	std::optional<SpawnBuilding> spawnBuilding;
};


struct ObjectWeapon {
	const Weapon *weapon;
	unsigned priority;
	unsigned barrel;
	unsigned cooldown;

	ObjectWeapon(unsigned short extraPrio, unsigned barrel, const Mod &base, const std::unordered_map<std::string, Weapon> &wList, const nlohmann::json &j);
};

struct ObjectBarrel {
	std::vector<ObjectWeapon> weapons;
	unsigned shootTimer = 0;
	unsigned short priority;

	ObjectBarrel(unsigned index, const Mod &base, const std::unordered_map<std::string, Weapon> &wList, const nlohmann::json &j);
	void update();
};

class GameObjectData {
public:
	struct AnimationState {
		std::string storageKey;
		float health;
	};

	unsigned index;
	AnimationState animationState;
	PathFindingGrid *pfGrid = nullptr;
	GameManager *manager = nullptr;
	MapState *map = nullptr;
	Player *owner = nullptr;

protected:
	std::reference_wrapper<const Faction> _base;
	const MultiSound *_crushSound = nullptr;
	const MultiSound *_buildSound = nullptr;
	const MultiSound *_doSound = nullptr;
	const MultiSound *_selectSound = nullptr;
	const MultiSound *_deathSound = nullptr;
	std::vector<ObjectBarrel> _barrels;
	std::vector<std::string> _tags;
	std::optional<std::string> _giveQueue;
	std::optional<UnitDeployParams> _onDeploy;
	std::unordered_map<std::string, unsigned> _storage;
	std::set<Vector2i> _checkTiles;
	std::optional<std::vector<std::vector<BuildingMask>>> _tiles;
	std::unordered_map<std::string, unsigned> _sellUnits;
	std::unordered_map<std::string, unsigned> _sellRefund;
	std::unordered_map<std::string, unsigned> _repairCost;
	std::unordered_map<std::string, unsigned> _cost;
	std::shared_ptr<ReqAndCombinator> _requirements;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _proxy;
	std::vector<Vector2i> _pathCache;
	Vector2i _cachedDest;
	sf::Image _cachedDebug;
	mutable sf::Texture _cachedDebugTex;
	std::deque<Order> _orderQueue;
	std::string _group;
	std::string _id;
	IntRect _selectBox;
	IntRect _skirt{0, 0, 0, 0};
	Vector2u _size;
	Vector2i _point;
	Vector2f _position;
	Vector2i _gridPos;
	Vector2i _oldPos = {-1, -1};
	std::optional<ExtraSpawn> _extraSpawn;
	std::vector<std::string> _spawnOrders;
	std::vector<std::string> _acceptResources;
	GameObject *_lastResourceDump = nullptr;
	std::deque<ResourceLot> _lots;
	std::optional<Vector2i> _spawnOffset;
	std::optional<Vector2i> _exitOffset;
	unsigned _resourceStorage = 0;
	unsigned _currentDir = 0;
	unsigned _turnSpeed = 0;
	unsigned _buildSpeed = -1;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _decayTarget;
	unsigned _sight = 0;
	unsigned _limit = UINT32_MAX;
	std::array<unsigned, 4> _gpTimer = {};
	unsigned _asideCooldown = 0;
	unsigned _passFlags = 0;
	unsigned _harvestTime = 0;
	unsigned _harvestDelay = 0;
	unsigned _showAfterBuild = 0;
	unsigned _showBeforeSell = 0;
	unsigned _idleTimer = 0;
	unsigned _currentIdle = 0;
	unsigned _repairDelay = 0;
	unsigned _repairHeal = 0;
	unsigned _repairTimer = 0;
	unsigned _sellTimer = 0;
	std::pair<unsigned, unsigned> _idleTimerRange;
	int _power = 0;
	float _speed = 0;
	PFFreeMask _oldTilePosition;
	PFFreeMask _currentTilePosition;
	unsigned char _buildMaxDistance = 0;
	mutable bool _cachedDebugTexLoaded = false;
	bool _requiresQueueBuilding = true;
	bool _requiresPlacement = false;
	bool _boundToGrid = false;
	bool _disposable = false;
	bool _pathCached = false;
	bool _firstEvent = true;
	bool _initGrid = false;
	bool _stackable = false;
	bool _solid = true;
	bool _harvesting = false;
	bool _fake = false;
	bool _central = false;
	bool _canCrush = false;
	bool _isRadar = false;
	bool _repairing = false;
	// TODO: Find a better way to do this
	bool _dumpStop = false;
	bool _dumpStart = false;
	bool _dumping = false;
	bool _moving = false;
	bool _selectable = true;
	bool _sellable = false;
	bool _repairable = false;
	bool _built = false;
	bool _sold = false;
	bool _producing = false;
	bool _produced = false;
	bool _crushed = false;
	bool _isPrimary = false;
	bool _lowerPowerWhenDamaged = false;

	GameObjectData(const Faction &base);
	GameObjectData(const GameObjectData &);
};

class GameObject : public GameObjectData {
public:
	GameObject(const GameObject &b);
	GameObject(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Weapon> &weapons,
		const nlohmann::json &json
	);
	virtual ~GameObject();

	void setColor(const HSLColor &color);
	void update();
	void render(int layer);
	bool canDeploy() const;
	bool isDeployable() const;
	bool canMove() const;
	void deploy();
	void built();
	std::optional<float> getStorageDisplay() const;
	void initGlobalGrid();
	void deinitGlobalGrid();
	void crushedBy(GameObject *crusher);
	unsigned getBottom() const;
	void drawSkirt() const;
	void setDirection(float angle);
	void setPosition(const Vector2f &pos);
	void heal(unsigned amount);
	void damage(unsigned amount);
	void setHealth(unsigned health);
	void harvest(const Vector2i &pos, bool automatic = false);
	void harvest();
	void attack(GameObject *obj);
	void moveTo(const Vector2i &pos, bool automatic = false);
	bool moveAside(GameObject *asker);
	void dumpResourcesTo(GameObject *dump, bool automatic = false);
	void select() const;
	void playBuiltSound() const;
	void clearOrders();
	void renderPathFindingPath() const;
	void playDoSound();
	void clearPrimaryDump();
	void produced();
	void producing(bool producing);
	void setPrimary(bool primary = true);
	void toggleRepair();
	void sell();
	void nextDirection();

	[[nodiscard]] bool canAttack(const GameObject *obj) const;
	[[nodiscard]] const std::vector<std::string> &getTags() const;
	[[nodiscard]] const std::optional<UnitDeployParams> &getOnDeploy() const;
	[[nodiscard]] unsigned getSpeed() const;
	[[nodiscard]] int getPower() const;
	[[nodiscard]] unsigned char getBuildMaxDistance() const;
	[[nodiscard]] bool isRadar() const;
	[[nodiscard]] bool isCrushable() const;
	[[nodiscard]] bool isSelectable() const;
	[[nodiscard]] bool isSellable() const;
	[[nodiscard]] bool isRepairable() const;
	[[nodiscard]] bool isPrimary() const;
	[[nodiscard]] bool isProducing() const;
	[[nodiscard]] const std::optional<std::string> &getGivenQueue() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getStorage() const;
	[[nodiscard]] bool isCentral() const;
	[[nodiscard]] const std::optional<ExtraSpawn> &getExtraSpawn() const;
	[[nodiscard]] const Vector2i &getPoint() const;
	[[nodiscard]] const std::vector<std::string> &getSpawnOrders() const;
	[[nodiscard]] const std::vector<std::string> &getAcceptResources() const;
	[[nodiscard]] unsigned getResourceStorage() const;
	[[nodiscard]] bool isFake() const;
	[[nodiscard]] bool requiresQueueBuilding() const;
	[[nodiscard]] bool requiresPlacement() const;
	[[nodiscard]] const std::optional<std::string> &getProxy() const;
	[[nodiscard]] const std::optional<std::vector<std::vector<BuildingMask>>> &getTileData() const;
	[[nodiscard]] const std::string &getId() const;
	[[nodiscard]] unsigned getMaxHealth() const;
	[[nodiscard]] unsigned getHealth() const;
	[[nodiscard]] bool isDead() const;
	[[nodiscard]] bool isDisposable() const;
	[[nodiscard]] const IntRect &getSelectBox() const;
	[[nodiscard]] const Vector2u &getSize() const;
	[[nodiscard]] Vector2i getPosition() const;
	[[nodiscard]] const std::optional<Vector2i> &getSpawnOffset() const;
	[[nodiscard]] const std::optional<Vector2i> &getExitOffset() const;
	[[nodiscard]] const std::vector<ConditionalAnim>& getBaseAnims() const;
	[[nodiscard]] const ReqAndCombinator &getRequirements() const;
	[[nodiscard]] const Mod &getBase() const;
	[[nodiscard]] const Faction &getFaction() const;
	[[nodiscard]] unsigned getBuildSpeed() const;
	[[nodiscard]] const std::string &getGroup() const;
	[[nodiscard]] bool isObstructed() const;
	[[nodiscard]] unsigned getLimit() const;
	[[nodiscard]] unsigned getPassFlags() const;
	[[nodiscard]] bool isSolid() const;
	[[nodiscard]] bool isStackable() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getCost() const;
	[[nodiscard]] const std::set<Vector2i> &getCheckTiles() const;
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;

protected:
	// Fields here (and not in GameObjectData) require a special copy procedure
	std::optional<DumpInfo> _dump;
	std::vector<std::vector<ConditionalAnim>> _anims;
	std::optional<ConditionalAnim> _sellAnim;
	std::optional<ConditionalAnim> _repairAnim;
	std::optional<ConditionalAnim> _buildAnim;
	std::optional<ConditionalAnim> _crushAnim;
	std::optional<ConditionalAnim> _producingAnim;
	std::optional<ConditionalAnim> _producedAnim;
	std::vector<ConditionalAnim> _moveAnim;
	std::vector<ConditionalAnim> _fireAnim;
	std::vector<ConditionalAnim> _crouchAnim;
	std::vector<ConditionalAnim> _crawlAnim;
	std::vector<ConditionalAnim> _crawlFireAnim;
	std::vector<ConditionalAnim> _idleAnims;
	std::vector<ConditionalAnim> _harvestAnims;
	std::unordered_map<std::string, std::vector<ObjectBarrel *>> _barrelByTag;
	std::unordered_map<std::string, std::vector<ObjectWeapon *>> _weaponByTag;

	[[nodiscard]] const ObjectWeapon *_getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree) const;
	[[nodiscard]] ObjectWeapon *_getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree);
	[[nodiscard]] Vector2f _getTileOffsetFromCenter() const;
	void _computeMovePath(Vector2i destination, bool ignoreSolid, const GameObject *ignoreObject);
	bool _tileAvailableFor(Vector2i tile) const;
	GameObject *_findBlockingUnit(Vector2i absPos, Vector2u size) const;
	CollisionResolution _resolveGridCollision(bool ignoreSolid, const GameObject *ignoreObject);
	CollisionResolution _resolveFreeCollision(bool ignoreSolid, const GameObject *ignoreObject);
	// Order update functions
	bool _updateTurn(unsigned target);
	bool _updateMove(Vector2i pos, bool force, const GameObject *ignoreObject);
	bool _updateCheckMove(Vector2i pos, void (GameObject::*fail)());
	bool _updateHarvest();
	bool _updateDump(GameObject *place);
	bool _updateDeploy();
	bool _updateAttack(GameObject *obj);
	bool _updateFinishMovement();

	void _harvestClosestResource();
	void _dumpInClosestResourceDump();
	void _goToClosestResource();
	void _goToClosestResourceDump();
	bool _shortenOrder(Order &o);
	void _displayAnim(Animation &anim, Vector2f offset = {});
	virtual bool _handleOrder(const Order &o);
};


#endif //MIXRTS_GAMEOBJECT_HPP
