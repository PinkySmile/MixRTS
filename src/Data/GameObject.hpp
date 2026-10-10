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
#include <sol/forward.hpp>
#include "Vector.hpp"
#include "forward.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"
#include "Screen.hpp"
#include "Utils.hpp"
#include "MultiSound.hpp"
#include "FindPath.hpp"
#include "ConditionalAnim.hpp"
#include "Warhead.hpp"
#include "Extra.hpp"

enum MoveReason {
	MOVEREASON_MOVE_COMMAND,
	MOVEREASON_FORCE_MOVE_COMMAND,
	MOVEREASON_ATTACK_COMMAND,
	MOVEREASON_FORCE_ATTACK_COMMAND,
	MOVEREASON_HARVEST_COMMAND,
	MOVEREASON_HARVEST,
	MOVEREASON_DUMP_COMMAND,
	MOVEREASON_DUMP,
	MOVEREASON_AUTO_AGRO,
	MOVEREASON_REPAIR,
	MOVEREASON_MOVE_ASIDE,
	MOVEREASON_MOVE_SPAWN,
	MOVEREASON_FINISH_MOVE,
	MOVEREASON_DELIVERY,
};
MYDLL_API MoveReason moveReasonFromString(const std::string &);

struct MYDLL_API ExecCbOrder {
	std::function<bool()> cb;
};
struct MYDLL_API DeliveryOrder {
	std::vector<std::string> objects;
	std::string queue;
	Position destination;
	const MapState *state;
};
struct MYDLL_API PickupOrder {
	GameObject *target;
	Vector2f destination;
};
struct MYDLL_API WaitPickupOrder;
struct MYDLL_API SpinOrder {
	float angle;
};
struct MYDLL_API TurnOrder {
	unsigned resultDir;
};
struct MYDLL_API MoveOrder {
	Vector2i destination;
	MoveReason reason;
	bool force;
};
struct MYDLL_API FinishMovementOrder {};
struct MYDLL_API HarvestOrder {};
struct MYDLL_API HarvestCloseOrder {};
struct MYDLL_API DeployOrder {};
struct MYDLL_API DieOrder {};
struct MYDLL_API DumpOrder {
	GameObject *target;
};
struct MYDLL_API MoveCheckOrder {
	void (GameObject::*fail)();
	Vector2i destination;
};
struct MYDLL_API AttackUnitOrder {
	GameObject *target;
	bool automatic;
	bool force;
};
typedef std::variant<
	SpinOrder,
	TurnOrder,
	MoveOrder,
	DeployOrder,
	HarvestOrder,
	HarvestCloseOrder,
	DumpOrder,
	MoveCheckOrder,
	AttackUnitOrder,
	FinishMovementOrder,
	PickupOrder,
	WaitPickupOrder,
	DieOrder,
	DeliveryOrder,
	ExecCbOrder
> Order;

struct MYDLL_API WaitPickupOrder {
	std::vector<MoveReason> targets;
	double dist;
	std::shared_ptr<Order> other;
};


// Result of resolving a dynamic collision while a grid-bound unit moves.
enum CollisionResolution {
	COLLISION_PROCEED, // The way ahead is clear (possibly on a freshly found path).
	COLLISION_WAIT,    // Blocked for now; keep the order and retry next frame.
	COLLISION_ABORT    // Nothing can be done; the unit must stop here.
};

struct MYDLL_API DumpInfo {
	unsigned dir;
	unsigned delay;
	unsigned delayAfter;
	std::unique_ptr<ConditionalAnim> during;
	std::unique_ptr<ConditionalAnim> start;
	std::unique_ptr<ConditionalAnim> stop;
};

struct MYDLL_API ResourceLot {
	std::unordered_map<std::string, unsigned> value;
};

union MYDLL_API BuildingMask {
	struct {
		bool placementConcrete: 1; // 1
		bool solid: 1;             // 2
		bool placementRequired: 1; // 4
	};
	uint8_t value;
};

struct MYDLL_API UnitDeployParams {
	struct SpawnBuilding {
		std::string building;
		std::string queue;
		Vector2i offset;

		static void pushUserType(sol::state &state);
	};

	std::optional<unsigned> direction;
	std::optional<SpawnBuilding> spawnBuilding;

	static void pushUserType(sol::state &state);
};

struct MYDLL_API ObjectWeapon {
	const Weapon *weapon;
	unsigned priority;
	unsigned barrel;
	unsigned cooldown;

	ObjectWeapon(unsigned short extraPrio, unsigned barrel, const Mod &base, const std::unordered_map<std::string, Weapon> &wList, const nlohmann::json &j);
};

struct MYDLL_API ObjectBarrel {
	std::vector<ObjectWeapon> weapons;
	struct ObjectTurret *turret = nullptr;
	unsigned shootTimer = 0;
	unsigned short priority;

	ObjectBarrel(unsigned index, const Mod &base, const std::unordered_map<std::string, Weapon> &wList, const nlohmann::json &j);
	void update();
};

struct MYDLL_API ObjectTurret {
	std::vector<ConditionalAnim> anims;
	GameObject *parent;
	bool fixed = false;
	bool independent = true;
	Vector2f offset;
	unsigned shotTimer = 0;
	unsigned turnTimer = 0;
	unsigned turnDelay = 0;
	unsigned barrel;
	unsigned direction = 0;
	unsigned directionTarget = 0;
	unsigned recoilTimer = 0;

	ObjectTurret(GameObject *parent, const Mod &base, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j);
	ObjectTurret(GameObject *parent, const ObjectTurret &other);
	void update();
	void render(int layer);
	void attack();
};

struct MYDLL_API ObjectDeathParams {
	std::optional<ConditionalAnim> anim;
	std::optional<unsigned> fadeTime;
	std::optional<unsigned> stayTime;
	const Effect *effect = nullptr;
	unsigned count = 1;
	bool perTile = false;
	Vector2f variance = {0, 0};
	std::unique_ptr<ShakeParams> shake;
	std::unordered_map<std::string, unsigned> units;
	std::vector<const Weapon *> projectilesPool;
	std::pair<Vector2i, Vector2i> projectilesTarget;
	std::pair<unsigned, unsigned> projectilesCount;

	ObjectDeathParams(
		const Mod &base,
		const GameObject *obj,
		const std::unordered_map<std::string, std::vector<Animation>> &animMap,
		const std::unordered_map<std::string, Effect> &effectMap,
		const std::unordered_map<std::string, Weapon> &weapons,
		const nlohmann::json &json
	);
	ObjectDeathParams(const GameObject *obj, const ObjectDeathParams &other);
};

struct MYDLL_API ObjectSpawnParams {
	enum Type {
		TYPE_APPEAR
	};

	Type type = TYPE_APPEAR;
	Position pos;

	ObjectSpawnParams() = default;
	ObjectSpawnParams(const nlohmann::json &json);
};

struct MYDLL_API ObjectDecel {
	float dist2 = 0;
	float min;
	float step;
};

class MYDLL_API GameObjectData {
public:
	struct AnimationState {
		std::string storageKey;
		float health;

		static void pushUserType(sol::state &state);
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
	ObjectDeathParams *_deathAnim = nullptr;
	const std::vector<MoveReason> *_pickupTargets = nullptr;
	double _pickupDistMin = 0;
	GameObject *_pickup = nullptr;
	ObjectSpawnParams _spawnParams;
	ObjectDecel _decel;
	std::vector<ObjectBarrel> _barrels;
	std::vector<std::string> _tags;
	std::optional<std::string> _giveQueue;
	std::optional<UnitDeployParams> _onDeploy;
	std::unordered_map<std::string, unsigned> _storage;
	std::set<Vector2i> _checkTiles;
	std::vector<const Effect *> _muzzleAnim;
	std::optional<std::vector<std::vector<BuildingMask>>> _tiles;
	std::unordered_map<std::string, unsigned> _sellUnits;
	std::unordered_map<std::string, unsigned> _sellRefund;
	std::unordered_map<std::string, unsigned> _repairCost;
	std::unordered_map<std::string, unsigned> _cost;
	std::shared_ptr<ReqAndCombinator> _requirements;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _proxy;
	std::vector<Vector2i> _pathCache;
	std::array<std::vector<const GameObject *>, 8> _connections;
	std::vector<unsigned char> _connectedSides;
	unsigned _connectedMask = 0;
	std::string _connectTo;
	Vector2i _cachedDest;
	double _cachedDist;
	sf::Image _cachedDebug;
	mutable sf::Texture _cachedDebugTex;
	std::deque<Order> _orderQueue;
	std::optional<Order> _idleOrder;
	std::string _group;
	std::string _armor;
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
	Player *_originalOwner;
	unsigned _takeControlTime = 0;
	unsigned _timeDecOnAtk;
	unsigned _resourceStorage = 0;
	unsigned _currentDir = 0;
	unsigned _turnDelay = 0;
	unsigned _buildSpeed = -1;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _decayTarget;
	unsigned _sight = 0;
	unsigned _limit = UINT32_MAX;
	std::array<unsigned, 8> _gpTimerU = {};
	std::array<float, 8> _gpTimerF = {};
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
	unsigned _crouchTimer = 0;
	std::pair<unsigned, unsigned> _idleTimerRange;
	int _altitude = 0;
	int _power = 0;
	float _speed = 0;
	float _alpha = 1;
	PFFreeMask _currentTilePosition;
	unsigned char _buildMaxDistance = 0;
	mutable bool _cachedDebugTexLoaded = false;
	bool _requiresQueueBuilding = true;
	bool _requiresPlacement = false;
	bool _boundToGrid = false;
	bool _disposable = false;
	bool _pathCached = false;
	bool _firstEvent = true;
	bool _firstREvent = true;
	bool _firstIEvent = true;
	bool _initGrid = false;
	bool _registered = false;
	bool _stackable = false;
	bool _solid = true;
	bool _harvesting = false;
	bool _fake = false;
	bool _central = false;
	bool _canCrush = false;
	bool _isRadar = false;
	bool _repairing = false;
	bool _animFinished = false;
	bool _ignoreTerrain = false;
	bool _moveWhileTurn = false;
	// TODO: Find a better way to do this
	bool _shooting = false;
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
	bool _isPrimary = false;
	bool _lowerPowerWhenDamaged = false;

	GameObjectData(const Faction &base);
	GameObjectData(const GameObjectData &);
};

class MYDLL_API GameObject : public GameObjectData {
public:
	GameObject(const GameObject &b);
	GameObject(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Effect> &effects,
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
	void execCb(const std::function<bool()> &cb);
	std::optional<float> getStorageDisplay() const;
	void initGlobalGrid();
	void deinitGlobalGrid();
	void crushedBy(GameObject *crusher);
	unsigned getBottom() const;
	void drawSkirt() const;
	void setDirection(float angle);
	void setPosition(const Vector2f &pos);
	void setAltitude(int altitude);
	void heal(unsigned amount);
	void damage(unsigned amount, DamageType type = DMGTYPE_GENERIC);
	void setHealth(unsigned health);
	void harvest(const Vector2i &pos, bool automatic, bool straight = false);
	void harvest();
	void attack(GameObject *obj, bool automatic, bool force);
	void moveOut(const Vector2i &pos);
	void moveTo(const Vector2i &pos, MoveReason reason, bool straight = false);
	bool moveAside(GameObject *asker);
	void dumpResourcesTo(GameObject *dump, bool automatic, bool straight = false);
	void pickup(GameObject *target, Vector2f destination);
	void deliverTo(const std::vector<GameObject *> &objects, Vector2f destination);
	void deliverTo(const std::vector<std::string> &objects, const std::string &queue, const Position &destination, const MapState &state);
	void select() const;
	void playBuiltSound(Vector2f pos) const;
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
	void setOwner(Player *player, unsigned time, unsigned timeDecOnAtk);
	void spawn(const Vector2f &spawnPoint);
	void connectTo(const GameObject *object);
	void disconnectFrom(const GameObject *object);
	void disposeOf();

	[[nodiscard]] const std::string &getConnectTo() const;
	[[nodiscard]] Vector2f getMiddle() const;
	[[nodiscard]] bool isBeingPickedUp() const;
	[[nodiscard]] bool canPickup(MoveReason r, double dist) const;
	[[nodiscard]] int getAltitude() const;
	[[nodiscard]] bool ignoresTerrain() const;
	[[nodiscard]] size_t getDirections() const;
	[[nodiscard]] double getFacingAngle() const;
	[[nodiscard]] unsigned getDirection() const;
	[[nodiscard]] bool isInObject(Vector2f pos, Vector2f size, bool fullTile) const;
	[[nodiscard]] Vector2f getHitboxCenter(Vector2f pos) const;
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
	[[nodiscard]] const std::string &getArmor() const;
	[[nodiscard]] bool isObstructed() const;
	[[nodiscard]] unsigned getLimit() const;
	[[nodiscard]] unsigned getPassFlags() const;
	[[nodiscard]] bool isSolid() const;
	[[nodiscard]] bool isStackable() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getCost() const;
	[[nodiscard]] const std::set<Vector2i> &getCheckTiles() const;
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;

	static void pushUserType(sol::state &state);

protected:
	// Fields here (and not in GameObjectData) require a special copy procedure
	std::optional<DumpInfo> _dump;
	std::vector<std::vector<ConditionalAnim>> _anims;
	std::optional<ObjectDeathParams> _crushAnim;
	std::optional<ConditionalAnim> _sellAnim;
	std::optional<ConditionalAnim> _repairAnim;
	std::optional<ConditionalAnim> _buildAnim;
	std::optional<ConditionalAnim> _producingAnim;
	std::optional<ConditionalAnim> _producedAnim;
	std::vector<ConditionalAnim> _moveAnim;
	std::vector<ConditionalAnim> _fireAnim;
	std::vector<ConditionalAnim> _crouchAnim;
	std::vector<ConditionalAnim> _crawlAnim;
	std::vector<ConditionalAnim> _crawlFireAnim;
	std::vector<ConditionalAnim> _idleAnims;
	std::vector<ConditionalAnim> _harvestAnims;
	std::vector<ObjectDeathParams> _deathAnims;
	std::vector<ObjectTurret> _turrets;
	std::unordered_map<std::string, std::vector<ObjectBarrel *>> _barrelByTag;
	std::unordered_map<std::string, std::vector<ObjectWeapon *>> _weaponByTag;

	[[nodiscard]] const ObjectWeapon *_getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree) const;
	[[nodiscard]] ObjectWeapon *_getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree);
	[[nodiscard]] Vector2f _getTileOffsetFromCenter() const;
	void _recomputeConnectedMask();
	unsigned char _getConnectionSide(const GameObject *);
	void _computeMovePath(Vector2i destination, bool ignoreSolid, const GameObject *ignoreObject);
	bool _tileAvailableFor(Vector2i tile) const;
	GameObject *_findBlockingUnit(Vector2i absPos, Vector2u size, const GameObject *ignoreObject) const;
	[[nodiscard]] Vector2i _findLandingSpot(GameObject *target, Vector2i preferredTile) const;
	CollisionResolution _resolveGridCollision(bool ignoreSolid, const GameObject *ignoreObject);
	CollisionResolution _resolveFreeCollision(bool ignoreSolid, const GameObject *ignoreObject);
	// Order update functions
	bool _updateTurn(unsigned target);
	bool _updateMove(Vector2i pos, MoveReason reason, bool force, const GameObject *ignoreObject);
	bool _updateCheckMove(Vector2i pos, void (GameObject::*fail)());
	bool _updateHarvest();
	bool _updateHarvestClose();
	bool _updateDump(GameObject *place);
	bool _updateDeploy();
	bool _updateAttack(GameObject *obj, bool automatic, bool force);
	bool _updateSpin(float amount);
	bool _updatePickup(GameObject *target, Vector2f destination);
	bool _updateWaitPickup(const std::vector<MoveReason> &targets, double dist, const Order *other);
	bool _updateFinishMovement();
	bool _updateDelivery(const std::vector<std::string> &objects, const std::string &queue, const Position &destination, const MapState &state);
	void _reduceTimeControlTimer(unsigned dec);
	void _spawnUnits(const std::unordered_map<std::string, unsigned> &units);

	void _harvestClosestResource();
	void _dumpInClosestResourceDump();
	void _goToClosestResource();
	void _goToClosestResourceDump();
	bool _shortenOrder(Order &o);
	void _displayAnim(Animation &anim, Vector2f offset = {}) const;
	virtual bool _handleOrder(const Order &o);

	friend ObjectTurret;
};


#endif //MIXRTS_GAMEOBJECT_HPP
