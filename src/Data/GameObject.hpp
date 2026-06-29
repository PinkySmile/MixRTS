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

struct TurnOrder {
	unsigned resultDir;
};
struct MoveOrder {
	Vector2i destination;
	bool force;
};
struct HarvestOrder {};
struct DeployOrder {};
struct DumpOrder {
	Building *target;
};

typedef std::variant<
	TurnOrder,
	MoveOrder,
	DeployOrder,
	HarvestOrder,
	DumpOrder
> Order;


struct ExtraSpawn {
	std::string unit;
	Vector2i pos;
};

struct DumpInfo {
	unsigned dir;
	unsigned delay;
	unsigned delay_after;
	std::unique_ptr<ConditionalAnim> during;
	std::unique_ptr<ConditionalAnim> start;
	std::unique_ptr<ConditionalAnim> stop;
};

struct ResourceLot {
	std::unordered_map<std::string, unsigned> value;
};

class GameObject {
protected:
	const Faction &_base;
	const MultiSound *_buildSound = nullptr;
	const MultiSound *_doSound = nullptr;
	const MultiSound *_selectSound = nullptr;
	std::set<Vector2i> _checkTiles;
	std::optional<std::vector<std::vector<uint8_t>>> _tiles;
	std::vector<std::vector<ConditionalAnim>> _anims;
	std::vector<ConditionalAnim> _harvestAnims;
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
	std::optional<ExtraSpawn> _extraSpawn;
	std::vector<std::string> _spawnOrders;
	std::vector<std::string> _acceptResources;
	Building *_lastRefinery = nullptr;
	std::deque<ResourceLot> _lots;
	std::optional<Vector2i> _spawnOffset;
	std::optional<Vector2i> _exitOffset;
	std::optional<DumpInfo> _dump;
	unsigned _resourceStorage = 0;
	unsigned _currentDir = 0;
	unsigned _turnSpeed = 0;
	unsigned _buildSpeed = -1;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _sight;
	unsigned _limit = UINT32_MAX;
	std::array<unsigned, 4> _gpTimer = {};
	unsigned _passFlags = 0;
	unsigned _harvestTime = 0;
	unsigned _harvestDelay = 0;
	float _speed = 0;
	unsigned char _tileCount = 5;
	bool _requiresQueueBuilding = true;
	mutable bool _cachedDebugTexLoaded = false;
	bool _boundToGrid = false;
	bool _disposable = false;
	bool _pathCached = false;
	bool _firstEvent = true;
	bool _initGrid = false;
	bool _solid = true;
	bool _harvesting = false;
	bool _fake = false;
	bool _central = false;
	// TODO: Find a better way to do this
	bool _dumpStop = false;
	bool _dumpStart = false;
	bool _dumping = false;

	bool _updateTurn(unsigned target);
	bool _updateMove(Vector2i pos, bool force);
	bool _updateHarvest();
	bool _updateDump(Building *place);

	void _goToClosestResource();
	void _goToClosestRefinery();
	bool _shortenOrder(Order &o);
	void _displayAnim(Animation &anim);
	virtual bool _handleOrder(const Order &o);

public:
	enum BuildingMask {
		MASK_PLACEMENT_CONCRETE = 1 << 0,
		MASK_SOLID              = 1 << 1,
		MASK_PLACEMENT_REQUIRED = 1 << 2,
	};

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


	GameObject(const GameObject &b);
	GameObject(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &json
	);
	virtual ~GameObject();

	virtual void setColor(const HSLColor &color);
	virtual void update();
	virtual void render(int layer);
	virtual bool canDeploy() const;
	virtual bool isDeployable() const;
	virtual bool canMove() const;
	virtual void deploy();
	virtual void built();
	virtual void initGlobalGrid();
	virtual void deinitGlobalGrid();
	virtual std::optional<float> getStorageDisplay() const;

	unsigned getBottom() const;
	void drawSkirt() const;
	void setDirection(float angle);
	void setPosition(const Vector2f &pos);
	void heal(unsigned amount);
	void damage(unsigned amount);
	void harvest(const Vector2i &pos, bool automatic = false);
	void harvest();
	void moveTo(const Vector2i &pos, bool automatic = false);
	void dumpResourcesTo(Building *dump, bool automatic = false);
	void select() const;
	void playBuiltSound() const;
	void clearOrders();
	void renderPathFindingPath() const;
	void playDoSound();
	void clearPrimaryDump();

	[[nodiscard]] bool isCentral() const;
	[[nodiscard]] const std::optional<ExtraSpawn> &getExtraSpawn() const;
	[[nodiscard]] const Vector2i &getPoint() const;
	[[nodiscard]] const std::vector<std::string> &getSpawnOrders() const;
	[[nodiscard]] const std::vector<std::string> &getAcceptResources() const;
	[[nodiscard]] unsigned getResourceStorage() const;
	[[nodiscard]] bool isFake() const;
	[[nodiscard]] bool requiresQueueBuilding() const;
	[[nodiscard]] const std::optional<std::string> &getProxy() const;
	[[nodiscard]] const std::optional<std::vector<std::vector<uint8_t>>> &getTileData() const;
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
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getCost() const;
	[[nodiscard]] const std::set<Vector2i> &getCheckTiles() const;
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;
};


#endif //MIXRTS_GAMEOBJECT_HPP
