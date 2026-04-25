//
// Created by PinkySmile on 20/04/2026.
//

#ifndef MIXRTS_GAMEOBJECT_HPP
#define MIXRTS_GAMEOBJECT_HPP


#include <string>
#include <memory>
#include <deque>
#include <unordered_map>
#include "Vector.hpp"
#include "forward.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"
#include "Screen.hpp"
#include "Utils.hpp"
#include "MultiSound.hpp"

struct TurnOrder {
	unsigned resultDir;
};

struct DeployOrder {};

typedef std::variant<
	TurnOrder,
	DeployOrder
> Order;

class GameObject {
protected:
	const Faction &_base;
	const MultiSound *_selectSound = nullptr;
	std::vector<std::vector<ConditionalAnim>> _anims;
	std::unordered_map<std::string, unsigned> _cost;
	std::shared_ptr<ReqAndCombinator> _requirements;
	std::deque<Order> _orderQueue;
	std::string _queue;
	std::string _group;
	std::string _id;
	IntRect _selectBox;
	Vector2u _size;
	Vector2i _position;
	unsigned _currentDir = 0;
	unsigned _turnSpeed = 0;
	unsigned _buildSpeed;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _sight;
	unsigned _limit;
	unsigned _gpTimer = 0;
	bool _disposable = false;

	void _displayAnim(Animation &anim);
	virtual bool _handleOrder(const Order &o);

public:
	struct AnimationState {
		std::string storageKey;
		float health;
	};

	AnimationState animationState;
	GameManager *manager = nullptr;
	Player *owner = nullptr;


	GameObject(const GameObject &b);
	GameObject(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &json
	);

	virtual void setColor(const HSLColor &color);
	virtual void update();
	virtual void render(int layer);
	virtual bool canDeploy() const;
	virtual bool isDeployable() const;
	virtual bool deploy();

	void setPosition(const sf::Vector2i &pos);
	const std::string &getId() const;
	unsigned getHealth() const;
	void heal(unsigned amount);
	void damage(unsigned amount);
	bool isDead() const;
	bool isDisposable() const;
	const IntRect &getSelectBox() const;
	const Vector2u &getSize() const;
	const Vector2i &getPosition() const;
	const std::vector<ConditionalAnim>& getBaseAnims() const;
	const ReqAndCombinator &getRequirements() const;
	const Mod &getBase() const;
	const Faction &getFaction() const;
	unsigned getBuildSpeed() const;
	const std::string &getGroup() const;
	bool isObstructed() const;
	unsigned getLimit() const;
	const std::unordered_map<std::string, unsigned> &getCost() const;
	void select() const;
};


#endif //MIXRTS_GAMEOBJECT_HPP
