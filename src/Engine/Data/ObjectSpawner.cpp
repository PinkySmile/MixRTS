//
// Created by PinkySmile on 07/11/2026.
//

#include "ObjectSpawner.hpp"
#include "Player.hpp"
#include "GameManager.hpp"
#include "StateContext.hpp"

#define _number _gpTimerU[0]
#define _delay _gpTimerU[1]
#define _delayTimer _gpTimerU[2]

ObjectSpawner::ObjectSpawner(const std::string &objName, unsigned count, unsigned delay, std::vector<Order> initOrders) :
	_name(objName),
	_initOrders(std::move(initOrders))
{
	this->_behavior = BEHAVIOR_NONE;
	this->_number = count;
	this->_delay = delay;
	this->_id = "base:spawner";
	this->_decayTarget = this->_maxHp = this->_hp = 1;
	this->_size = {1, 1};
	this->_solid = false;
	this->_countsForGame = false;
	this->_countsForShortGame = false;
	assert_exp(!this->_initOrders.empty());
	assert_exp(this->_number > 0);
}

ObjectSpawner::ObjectSpawner(ObjectSpawnerRestoreTag)
{
	this->_behavior = BEHAVIOR_NONE;
	this->_number = 1;
	this->_delay = 1;
	this->_id = "base:spawner";
	this->_decayTarget = this->_maxHp = this->_hp = 1;
	this->_size = {1, 1};
	this->_solid = false;
	this->_countsForGame = false;
	this->_countsForShortGame = false;
}

void ObjectSpawner::update()
{
	this->_delayTimer++;
	if (this->_delayTimer > this->_delay) {
		GameObject *obj = this->manager->spawnObject(this->owner->state.index, this->_position, "", this->_name);

		for (const Order &order : this->_initOrders)
			obj->pushOrder(order);
		this->_number--;
		this->_disposable = this->_number == 0;
		this->_delayTimer = 0;
	}
}

void ObjectSpawner::render(int)
{
}

void ObjectSpawner::saveState(StateSaveContext &ctx) const
{
	GameObject::saveState(ctx);
	ctx.out.string(this->_name);
	ctx.out.size(this->_initOrders.size());
	for (const Order &order : this->_initOrders)
		this->_saveOrder(ctx, order);
}

void ObjectSpawner::loadState(StateLoadContext &ctx)
{
	GameObject::loadState(ctx);
	this->_name = ctx.in.string();

	size_t count = ctx.in.size();

	this->_initOrders.clear();
	this->_initOrders.reserve(static_cast<size_t>(count));
	for (uint64_t i = 0; i < count; i++) {
		Order order;

		this->_loadOrder(ctx, order);
		this->_initOrders.push_back(std::move(order));
	}
}
