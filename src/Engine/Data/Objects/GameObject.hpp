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
#include <unordered_set>
#include <sol/forward.hpp>
#include "../Vector.hpp"
#include "forward.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"
#include "Screen.hpp"
#include "UI/UI.hpp"
#include "Utils.hpp"
#include "../MultiSound.hpp"
#include "../FindPath.hpp"
#include "../ConditionalAnim.hpp"
#include "../Animation.hpp"
#include "../Warhead.hpp"
#include "../Extra.hpp"

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
	float angle;
};
struct MYDLL_API MoveOrder {
	Vector2i destination;
	MoveReason reason;
	GameObject *ignore;
	bool force;
};
struct MYDLL_API FinishMovementOrder {};
//! Walks to \a destination (the tile to harvest) and harvests it. If the
//! destination can't be reached (blocked by something that won't leave), the
//! order re-targets the closest harvestable tile instead.
struct MYDLL_API HarvestOrder {
	Vector2i destination = {0, 0};
	MoveReason reason = MOVEREASON_HARVEST;
	bool force = false;
};
struct MYDLL_API HarvestCloseOrder {};
struct MYDLL_API DeployOrder {};
struct MYDLL_API SelfDestructOrder {};
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
	unsigned maxShots;
	bool automatic;
	bool force;
};
struct MYDLL_API AttackPosOrder {
	Vector2f target;
	unsigned maxShots;
	bool automatic;
	bool force;
};
struct MYDLL_API KeepMovingOrder {
	unsigned duration;
};
struct MYDLL_API KeepMovingAndDieOrder {};
//! Clears PlayerQueue::busy. Replaces the delivery lambda that used to be an ExecCbOrder.
struct MYDLL_API FreeQueueOrder {
	std::string queue;
};
typedef std::variant<
	SpinOrder,
	TurnOrder,
	MoveOrder,
	DeployOrder,
	SelfDestructOrder,
	HarvestOrder,
	HarvestCloseOrder,
	DumpOrder,
	MoveCheckOrder,
	AttackUnitOrder,
	AttackPosOrder,
	FinishMovementOrder,
	PickupOrder,
	WaitPickupOrder,
	DieOrder,
	DeliveryOrder,
	ExecCbOrder,
	KeepMovingOrder,
	KeepMovingAndDieOrder,
	FreeQueueOrder
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
	COLLISION_ABORT,   // Nothing can be done; the unit must stop here.
	COLLISION_DEST_BLOCKED // The final destination tile is taken and won't be freed; stop next to it.
};

// Result of getting rid of everything that stands in the way of a unit.
enum BlockerClearResult {
	BLOCKERS_NONE,      // Nothing is in the way.
	BLOCKERS_IMMOVABLE, // Only things that can't move (buildings...) are in the way.
	BLOCKERS_CRUSHED,   // Everything in the way has been crushed.
	BLOCKERS_ASKED,     // The units in the way have been asked to move aside: wait for them.
	BLOCKERS_STUCK      // Something in the way can neither be crushed nor pushed.
};

// Result of one step of a movement order.
enum MoveResult {
	MOVE_IN_PROGRESS,  // Still moving (or waiting).
	MOVE_DONE,         // The movement is over (arrived, no path, or aborted).
	MOVE_DEST_BLOCKED  // The movement is over because the destination tile is occupied.
};

struct MYDLL_API DumpInfo {
	//! Facing required to dump, in degrees clockwise from up (0 = north).
	float dir;
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
		bool placementSpread: 1;   // 8
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

	//! Facing required to deploy, in degrees clockwise from up (0 = north).
	std::optional<float> direction;
	std::optional<SpawnBuilding> spawnBuilding;

	static void pushUserType(sol::state &state);
};

//! Clicking the unit (the deploy cursor) starts a countdown, then fires \a weapon at its own center.
struct MYDLL_API SelfDestructParams {
	unsigned delay;
	const Weapon *weapon;
	UI::Cursor cursor;
};

//! Parameters of the "stealth" block of an object (see documentation/OBJECTS.md).
struct MYDLL_API StealthParams {
	enum Type {
		//! Revealed by nearby enemy objects having "reveal_stealth".
		TYPE_STEALTH,
		//! Revealed by enemy objects having a "detector" in range.
		TYPE_CAMOUFLAGE
	};
	enum DisplayType {
		//! The sprite is drawn tinted with a color (usually a translucent gray/black shadow).
		DISPLAY_SHADOW,
		//! The sprite is drawn with a reduced opacity.
		DISPLAY_TRANSPARENT,
		//! Nothing is drawn.
		DISPLAY_INVISIBLE,
		//! Lens like distortion around the unit. TODO: Not implemented yet, drawn like DISPLAY_INVISIBLE.
		DISPLAY_LENS
	};

	Type type = TYPE_STEALTH;
	bool permanent = true;
	//! Temporary only: whether the player can end the effect before the energy runs out.
	bool cancelable = true;
	//! Temporary only: energy needed to start the effect.
	unsigned activationThreshold = 0;
	//! Temporary only: energy removed every costDelay ticks while the effect is active.
	//! The effect ends when the energy is depleted.
	unsigned cost = 0;
	//! Temporary only: ticks between two energy payments (0 or 1: every tick).
	unsigned costDelay = 0;
	//! Ticks between being revealed and trying to cloak again. If empty, firing and getting close to
	//! something that reveals us doesn't uncloak the object.
	std::optional<unsigned> recloakDelay;
	//! Health ratio (0..1) under which cloaking can fail.
	std::optional<float> damagedThreshold;
	//! Chance (0..1) to successfully cloak when under damagedThreshold.
	float recloakChance = 1;
	const MultiSound *cloakSound = nullptr;
	const MultiSound *uncloakSound = nullptr;
	DisplayType allyDisplay = DISPLAY_SHADOW;
	sf::Color allyColor{0, 0, 0, 128};
	DisplayType enemyDisplay = DISPLAY_INVISIBLE;
	//! Frames of the dissolve mask. Frame 0 keeps the whole sprite, the last one hides it.
	mutable std::vector<Animation> mask;
	//! Ticks added to every frame of the mask.
	unsigned maskDelay = 0;
};

enum StealthState {
	//! Fully visible by everyone.
	STEALTH_VISIBLE,
	//! Playing the cloak animation. Already considered cloaked.
	STEALTH_CLOAKING,
	STEALTH_CLOAKED,
	//! Playing the uncloak animation. Already considered visible.
	STEALTH_UNCLOAKING,
	//! Failing to cloak: loops the cloak/uncloak animations. Considered visible.
	STEALTH_FLICKERING
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
	bool fast = true;
	Vector2f offset;
	unsigned shotTimer = 0;
	//! Degrees this turret can turn per frame. Divided by 8 while it is not tracking something.
	float turnSpeed = 360;
	unsigned barrel;
	float angle = -static_cast<float>(M_PI_2);
	float targetAngle = -static_cast<float>(M_PI_2);
	unsigned recoilTimer = 0;
	unsigned recoilMax = 0;

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

	enum Behavior {
		BEHAVIOR_NORMAL,
		BEHAVIOR_MISSILE,
		BEHAVIOR_NONE,
	};

	unsigned index;
	AnimationState animationState;
	PathFindingGrid *pfGrid = nullptr;
	GameManager *manager = nullptr;
	MapState *map = nullptr;
	Player *owner = nullptr;

protected:
	const Faction *_base = nullptr;
	const MultiSound *_crushSound = nullptr;
	//! Hitscan weapon applied to whatever crushed this object. Not fired on a normal death.
	const Weapon *_crushWeapon = nullptr;
	//! Hitscan weapon detonated at this object's center when it dies. Not fired when crushed.
	const Weapon *_deathWeapon = nullptr;
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
	std::optional<SelfDestructParams> _selfDestruct;
	std::optional<StealthParams> _stealth;
	//! Radius (in tiles) in which enemy objects with a stealth of type STEALTH are revealed.
	float _revealStealthRange = 0;
	//! Radius (in tiles) in which enemy objects with a stealth of type CAMOUFLAGE are revealed.
	float _detector = 0;
	//! Teams that currently see this object despite its stealth (temporary stealth, or without recloak delay).
	std::set<size_t> _revealedToTeams;
	//! Ids of the revealers that were around last tick (sorted).
	std::vector<unsigned> _nearRevealers;
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
	//! Tile this unit currently holds in the pathfinding grid. For grid-bound
	//! units this is the tile they are stepping into (reserved as soon as the
	//! step starts), which may differ from _gridPos until they get there.
	Vector2i _occupiedTile = {0, 0};
	Vector2i _oldPos = {-1, -1};
	std::optional<ExtraSpawn> _extraSpawn;
	std::vector<std::string> _spawnOrders;
	std::vector<std::string> _acceptResources;
	GameObject *_lastResourceDump = nullptr;
	std::deque<ResourceLot> _lots;
	std::optional<Vector2i> _spawnOffset;
	std::optional<Vector2i> _exitOffset;
	Player *_originalOwner = nullptr;
	unsigned _takeControlTime = 0;
	unsigned _timeDecOnAtk;
	unsigned _resourceStorage = 0;
	//! Facing, in radians. 0 points right and the angle grows counterclockwise, so -PI/2 points up.
	float _facingAngle = -static_cast<float>(M_PI_2);
	//! Degrees this object can turn per frame. 360 means it turns instantly.
	float _turnSpeed = 360;
	unsigned _buildSpeed = -1;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _decayTarget;
	unsigned _decayTimer = 0;
	unsigned _sight = 0;
	//! Ticks left before this object looks around for something to shoot again (while idle).
	unsigned _autoAcquireTimer = 0;
	unsigned _limit = UINT32_MAX;
	std::array<unsigned, 16> _gpTimerU = {};
	std::array<int, 8> _gpTimerI = {};
	std::array<float, 4> _gpTimerF = {};
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
	StealthState _stealthState = STEALTH_VISIBLE;
	//! Current energy, between 0 and _maxEnergy.
	unsigned _energy = 0;
	unsigned _maxEnergy = 0;
	unsigned _spawnEnergy = 0;
	//! Energy gained every _energyGainDelay ticks (it doesn't while a temporary stealth is active).
	unsigned _energyGain = 0;
	unsigned _energyGainDelay = 0;
	unsigned _energyGainTimer = 0;
	//! Temporary stealth: ticks left before the next energy payment.
	unsigned _stealthCostTimer = 0;
	//! Ticks left before trying to cloak again after being revealed.
	unsigned _recloakTimer = 0;
	//! Current frame of the dissolve mask.
	unsigned _stealthFrame = 0;
	unsigned _stealthFrameTimer = 0;
	std::pair<unsigned, unsigned> _idleTimerRange;
	int _altitude = 0;
	int _power = 0;
	float _speed = 0;
	//! Missile only: each shot is aimed at the target offset by up to this much on both axes.
	float _inaccuracy = 0;
	unsigned _spawnDelay = 0;
	//! Missile only: how many volleys to fire once the missile reaches its target.
	unsigned _shootRounds = 1;
	Behavior _behavior = BEHAVIOR_NORMAL;
	float _alpha = 1;
	PFFreeMask _currentTilePosition = PFFREEMASK_NONE;
	unsigned char _buildMaxDistance = 0;
	mutable bool _cachedDebugTexLoaded = false;
	bool _countsForGame;
	bool _countsForShortGame;
	bool _shootsFacing = true;
	bool _moveWhileFire = false;
	bool _requiresPower = false;
	bool _boundToGrid = false;
	bool _disposable = false;
	bool _pathCached = false;
	bool _firstEvent = true;
	bool _firstREvent = true;
	bool _firstIEvent = true;
	bool _registered = false;
	bool _stackable = false;
	bool _solid = true;
	bool _harvesting = false;
	//! True while _occupiedTile is a tile we are still stepping into.
	bool _tileReserved = false;
	bool _fake = false;
	bool _central = false;
	bool _canCrush = false;
	bool _isRadar = false;
	bool _repairing = false;
	bool _animFinished = false;
	bool _ignoreTerrain = false;
	bool _moveWhileTurn = false;
	bool _wall = false;
	// TODO: Find a better way to do this
	bool _shooting = false;
	bool _dumpStop = false;
	bool _dumpStart = false;
	bool _dumping = false;
	bool _moving = false;
	bool _selectable = true;
	//! Whether idle enemy objects in range automatically start shooting at this object.
	bool _triggersAutoAttack = false;
	//! Whether this object automatically shoots at enemies in range when idle.
	bool _autoAttack = true;
	bool _sellable = false;
	bool _repairable = false;
	bool _built = false;
	bool _sold = false;
	bool _producing = false;
	bool _produced = false;
	bool _isPrimary = false;
	bool _lowerPowerWhenDamaged = false;
	//! Temporary stealth: the effect is currently on (from the activation until the object is fully visible again).
	bool _stealthActive = false;
	//! A recloak attempt is scheduled once _recloakTimer ran out.
	bool _recloakPending = false;
	//! Second half (uncloak) of a flickering cycle.
	bool _stealthFlickerUncloak = false;
	//! The dissolve mask plays backward.
	bool _stealthReverse = false;

	GameObjectData() = default;
	GameObjectData(const GameObjectData &);
};

typedef std::vector<std::pair<std::string, std::string>> DebugInfoList;

class MYDLL_API GameObject : public GameObjectData {
public:
	GameObject() = default;
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

	virtual GameObject *copy() const;
	virtual void update();
	virtual void render(int layer);
	void setColor(const HSLColor &color);
	bool canDeploy() const;
	bool isDeployable() const;
	//! True only once the self destruct countdown itself is the order being executed.
	[[nodiscard]] bool isSelfDestructing() const;
	//! Cursor shown when hovering this unit to deploy it. Empty when it has no preference.
	[[nodiscard]] std::optional<UI::Cursor> getDeployCursor() const;
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
	virtual void damage(unsigned amount, DamageType type = DMGTYPE_GENERIC);
	void setHealth(unsigned health);
	void harvest(const Vector2i &pos, bool automatic, bool straight = false);
	void harvest();
	void attack(GameObject *obj, bool automatic, bool force);
	void attackPos(Vector2f target, unsigned maxShots = UINT32_MAX);
	void moveOut(const Vector2i &pos);
	//! \param ignore Object that is never considered an obstacle on the way (e.g. the building a unit just came out of).
	void moveTo(const Vector2i &pos, MoveReason reason, bool straight = false, GameObject *ignore = nullptr);
	//! \param claimed Spots (tiles for grid bound units) already picked by other units asked to move aside
	//!                at the same time. The one picked by this unit is added to it.
	bool moveAside(const GameObject *asker, std::vector<Vector2i> *claimed = nullptr);
	void dumpResourcesTo(GameObject *dump, bool automatic, bool straight = false);
	void pickup(GameObject *target, Vector2f destination);
	void deliverTo(const std::vector<GameObject *> &objects, Vector2f destination);
	void deliverTo(const std::vector<std::string> &objects, const std::string &queue, const Position &destination, const MapState &state);
	void select() const;
	void playBuiltSound(Vector2f pos) const;
	void clearOrders();
	void renderPathFindingPath() const;
	//! Fills \a out with the internal values of this object (debug tool).
	//! \param alive Set of the objects currently alive, used to avoid following dangling pointers.
	void getDebugInfo(DebugInfoList &out, const std::unordered_set<const GameObject *> &alive) const;
	//! Short description of what the object is doing (debug tool).
	[[nodiscard]] std::string getDebugOrderSummary(const std::unordered_set<const GameObject *> &alive) const;
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
	void keepMoving(unsigned duration);
	void keepMovingAndDie();
	void pushOrder(const Order &order);
	//! 0: object copied from a faction template. Subclasses use other values.
	[[nodiscard]] virtual uint8_t getStateKind() const { return 0; }
	virtual void saveState(StateSaveContext &ctx) const;
	virtual void loadState(StateLoadContext &ctx);

	[[nodiscard]] bool boundToGrid() const;
	[[nodiscard]] bool countsForGame() const;
	[[nodiscard]] bool countsForShortGame() const;
	[[nodiscard]] const ObjectWeapon *getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree) const;
	[[nodiscard]] ObjectWeapon *getBestWeaponFor(const GameObject *, unsigned barrel, bool isFree);
	[[nodiscard]] bool isWall() const;
	[[nodiscard]] const std::string &getConnectTo() const;
	[[nodiscard]] Vector2f getMiddle() const;
	[[nodiscard]] bool isBeingPickedUp() const;
	[[nodiscard]] bool canPickup(MoveReason r, double dist) const;
	[[nodiscard]] int getAltitude() const;
	[[nodiscard]] bool ignoresTerrain() const;
	[[nodiscard]] size_t getDirections() const;
	[[nodiscard]] float getFacingAngle() const;
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
	//! Whether idle enemy objects in range automatically start shooting at this object.
	[[nodiscard]] bool triggersAutoAttack() const;
	//! Whether this object shoots at enemies on its own when it is idle.
	[[nodiscard]] bool autoAttacks() const;
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
	[[nodiscard]] bool isObstructed(bool stackable) const;
	[[nodiscard]] unsigned getLimit() const;
	[[nodiscard]] unsigned getPassFlags() const;
	[[nodiscard]] bool isSolid() const;
	[[nodiscard]] bool isStackable() const;
	//! Tile (in this->map coordinates) currently held by this unit in the pathfinding grid.
	[[nodiscard]] Vector2i getOccupiedTile() const;
	//! Areas (absolute coordinates) this object currently registered in the global grid, the same ones
	//! given to GameManager::modUnitInArea (4 corners each). Empty if it isn't in the grid.
	[[nodiscard]] std::vector<std::array<Vector2f, 4>> getGridFootprint() const;
	//! Whether this unit could stand on \a tile (in this->map coordinates)
	//! right now: terrain, free space, stacking and crushing rules included.
	[[nodiscard]] bool canStandOn(Vector2i tile) const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getCost() const;
	[[nodiscard]] const std::set<Vector2i> &getCheckTiles() const;
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;

	// Stealth
	[[nodiscard]] const std::optional<StealthParams> &getStealthParams() const;
	[[nodiscard]] StealthState getStealthState() const;
	//! Energy level, between 0 and getMaxEnergy().
	[[nodiscard]] unsigned getEnergy() const;
	[[nodiscard]] unsigned getMaxEnergy() const;
	void setEnergy(unsigned energy);
	//! Radius in tiles in which enemy objects with a stealth of type STEALTH are revealed.
	[[nodiscard]] float getRevealStealthRange() const;
	//! Radius in tiles in which enemy objects with a stealth of type CAMOUFLAGE are revealed.
	[[nodiscard]] float getDetectorRange() const;
	//! Whether this object can currently reveal others (alive, built, powered...).
	[[nodiscard]] bool isOperational() const;
	//! Whether \a team can't see this object nor target it because of its stealth.
	[[nodiscard]] bool isHiddenFrom(size_t team) const;
	[[nodiscard]] bool isTemporaryStealth() const;
	//! Updates who is currently revealing this stealthed object.
	//! \param revealers Sorted ids of the enemy objects around that apply to our type of stealth.
	//! \param teams Teams of these objects.
	void updateStealthDetection(const std::vector<unsigned> &revealers, const std::set<size_t> &teams);
	//! Whether deploying this object does not interrupt what it is doing (stealth activation).
	[[nodiscard]] bool deployKeepsOrders() const;

	static void pushUserType(sol::state &state);

protected:
	GameObject(const GameObject &b);

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
	bool _initGrid = false;

	[[nodiscard]] Vector2f _getTileOffsetFromCenter() const;
	//! Point weapon ranges are measured from.
	[[nodiscard]] Vector2f _getAttackOrigin() const;
	//! Whether \a target is within reach of the best weapon this object has against it.
	[[nodiscard]] bool _isInWeaponRange(const GameObject *target) const;
	//! Called while idle: starts shooting at the closest enemy that is in range and triggers automatic attacks.
	void _autoAcquireTarget();

	inline void _recomputeConnectedMask()
	{
		this->_connectedMask = 0;
		for (const std::vector<const GameObject *> &a : this->_connections | std::ranges::views::reverse) {
			this->_connectedMask <<= 1;
			this->_connectedMask |= !a.empty();
		}
	}

	unsigned char _getConnectionSide(const GameObject *);
	void _computeMovePath(Vector2i destination, bool ignoreSolid, const GameObject *ignoreObject);
	//! \param extraClaims Number of (stackable) units that are about to go to \a tile and are not on it yet.
	bool _tileAvailableFor(Vector2i tile, unsigned extraClaims = 0) const;
	//! Finds everything that holds an area (free moving units).
	[[nodiscard]] std::vector<GameObject *> _findBlockingUnits(Vector2i absPos, Vector2u size, const GameObject *ignoreObject) const;
	//! Finds everything that holds \a tile in the pathfinding grid (units that reserved it included).
	[[nodiscard]] std::vector<GameObject *> _findTileBlockers(Vector2i tile, const GameObject *ignoreObject) const;
	//! Gets rid of all the \a blockers at once: allied units are asked to move aside and
	//! enemy ones are crushed (when \a allowCrush is set and we are able to).
	BlockerClearResult _clearBlockers(const std::vector<GameObject *> &blockers, bool allowCrush);
	//! Whether this unit is on its way somewhere other than \a tile (so it will free it up).
	[[nodiscard]] bool _isLeavingTile(Vector2i tile) const;
	void _reserveTile(Vector2i tile);
	[[nodiscard]] Vector2i _getFacingTile() const;
	MoveResult _updateMove(const MoveOrder &order);
	[[nodiscard]] Vector2i _findLandingSpot(GameObject *target, Vector2i preferredTile) const;
	//! \param pushBlockers When the final destination is held by an allied unit, ask it to move aside
	//!                    (instead of giving up). Used by units coming out of a building.
	CollisionResolution _resolveGridCollision(bool ignoreSolid, const GameObject *ignoreObject, bool pushBlockers = false);
	CollisionResolution _resolveFreeCollision(bool ignoreSolid, const GameObject *ignoreObject);

	bool _updateAttack(GameObject *obj, Vector2f target, unsigned maxShots, bool automatic, bool force);

	bool _updateOrder(const TurnOrder &order);
	bool _updateOrder(const MoveOrder &order);
	bool _updateOrder(const MoveCheckOrder &order);
	bool _updateOrder(HarvestOrder &order);
	bool _updateOrder(const HarvestCloseOrder &order);
	bool _updateOrder(const DumpOrder &order);
	bool _updateOrder(const DeployOrder &order);
	bool _updateOrder(const SelfDestructOrder &order);
	bool _updateOrder(const AttackUnitOrder &order);
	bool _updateOrder(const AttackPosOrder &order);
	bool _updateOrder(const SpinOrder &order);
	bool _updateOrder(const PickupOrder &order);
	bool _updateOrder(const WaitPickupOrder &order);
	bool _updateOrder(const FinishMovementOrder &order);
	bool _updateOrder(const DeliveryOrder &order);
	bool _updateOrder(const ExecCbOrder &order);
	bool _updateOrder(const DieOrder &order);
	bool _updateOrder(const KeepMovingOrder &order);
	bool _updateOrder(const KeepMovingAndDieOrder &order);
	bool _updateOrder(const FreeQueueOrder &order);
	bool _updateOrder(Order &order);

	//! Registers or removes this object's pathfinding footprint. dir is +1 or -1.
	//! Terrain words and wall connections are left alone: those are stored on their own.
	void _applyGridFootprint(int dir);
	void _saveOrder(StateSaveContext &ctx, const Order &order) const;
	void _loadOrder(StateLoadContext &ctx, Order &order);
	[[nodiscard]] uint8_t _moveFailId(void (GameObject::*fail)()) const;
	[[nodiscard]] void (GameObject::*_moveFail(uint8_t id))();

	void _reduceTimeControlTimer(unsigned dec);
	//! Plays \a weapon's sound and hit effect at \a pos, then applies it.
	//! When \a target is set, only that object is hit, at distance 0. Otherwise the weapon detonates at \a pos.
	void _fireWeaponAt(const Weapon *weapon, Vector2f pos, GameObject *target);
	//! Starts the death animation, effects and spawned units. Missile objects are not removed immediately.
	void _die(DamageType type);
	//! Fires one volley from every barrel that is ready. One volley is every ready barrel in this frame.
	void _updateMissileShots();
	void _spawnUnits(const std::unordered_map<std::string, unsigned> &units);
	void _harvestClosestResource();
	void _dumpInClosestResourceDump();
	void _goToClosestResourceDump();
	bool _shortenOrder(Order &o);
	void _displayAnim(Animation &anim, Vector2f offset = {}) const;

	// Stealth
	void _updateStealth();
	//! Called whenever something makes this object visible for a while (firing, getting too close to a revealer...).
	void _revealStealth();
	//! Called when this object has just fired a weapon.
	//! Starts uncloaking if needed. \return Whether the object can shoot right now.
	bool _prepareStealthFire();
	void _onStealthFire();
	//! Starts to cloak (or to flicker if it fails). Starts the effect of temporary stealths.
	void _startCloakAttempt();
	void _beginUncloak();
	//! Ends a temporary stealth. \a emptyEnergy also removes all the remaining energy.
	void _endStealthActive(bool emptyEnergy = false);
	void _updateEnergy();
	void _playStealthSound(bool cloak) const;
	//! Whether cloaking fails right now (damaged, and the roll failed).
	[[nodiscard]] bool _rollCloakFailure() const;
	void _activateStealth();

	friend ObjectTurret;
	friend class GameManager;
};


#endif //MIXRTS_GAMEOBJECT_HPP
