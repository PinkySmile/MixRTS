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
};
struct DeployOrder {};

typedef std::variant<
	TurnOrder,
	MoveOrder,
	DeployOrder
> Order;

class GameObject {
protected:
	const Faction &_base;
	const MultiSound *_buildSound = nullptr;
	const MultiSound *_selectSound = nullptr;
	std::set<Vector2i> _checkTiles;
	std::optional<std::vector<std::vector<uint8_t>>> _tiles;
	std::vector<std::vector<ConditionalAnim>> _anims;
	std::unordered_map<std::string, unsigned> _cost;
	std::shared_ptr<ReqAndCombinator> _requirements;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _proxy;
	std::vector<Vector2i> _pathCache;
	std::deque<Order> _orderQueue;
	std::string _group;
	std::string _id;
	IntRect _selectBox;
	Vector2u _size;
	Vector2f _position;
	unsigned _currentDir = 0;
	unsigned _turnSpeed = 0;
	unsigned _buildSpeed;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _sight;
	unsigned _limit;
	unsigned _gpTimer = 0;
	unsigned _speed = 0;
	unsigned _passFlags = 0;
	unsigned _index;
	unsigned char _tileCount = 5;
	bool _requiresQueueBuilding = true;
	bool _disposable = false;
	bool _pathCached = false;
	bool _initGrid = false;
	bool _solid = true;
	bool _fake;

	void _displayAnim(Animation &anim);
	virtual bool _handleOrder(const Order &o);

public:
	enum BuildingMask {
		MASK_SKIRT              = 1 << 0,
		MASK_SOLID              = 1 << 1,
		MASK_PLACEMENT_REQUIRED = 1 << 2,
		MASK_PLACEMENT_CONCRETE = 2 << 2,
	};

	struct AnimationState {
		std::string storageKey;
		float health;
	};

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

	unsigned getBottom() const;
	void setIndex(unsigned index);
	void setPosition(const sf::Vector2f &pos);
	void heal(unsigned amount);
	void damage(unsigned amount);
	void moveTo(const Vector2i &pos);
	void select() const;
	void playBuiltSound() const;
	void clearOrders();

	[[nodiscard]] bool isFake() const;
	[[nodiscard]] bool requiresQueueBuilding() const;
	[[nodiscard]] const std::optional<std::string> &getProxy() const;
	[[nodiscard]] const std::optional<std::vector<std::vector<uint8_t>>> &getTileData() const;
	[[nodiscard]] unsigned getIndex() const;
	[[nodiscard]] const std::string &getId() const;
	[[nodiscard]] unsigned getHealth() const;
	[[nodiscard]] bool isDead() const;
	[[nodiscard]] bool isDisposable() const;
	[[nodiscard]] const IntRect &getSelectBox() const;
	[[nodiscard]] const Vector2u &getSize() const;
	[[nodiscard]] Vector2i getPosition() const;
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
