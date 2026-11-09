//
// Created by PinkySmile on 07/11/2026.
//

#include "ObjectSpawner.hpp"
#include "Player.hpp"
#include "GameManager.hpp"

#define _number _gpTimerU[0]
#define _delay _gpTimerU[1]
#define _delayTimer _gpTimerU[2]

ObjectSpawner::ObjectSpawner(const std::string &objName, unsigned count, unsigned delay, const std::function<void (GameObject *)> &initObject) :
	_name(objName),
	_init(initObject)
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
	assert_exp(this->_init);
	assert_exp(this->_number > 0);
}

void ObjectSpawner::update()
{
	this->_delayTimer++;
	if (this->_delayTimer > this->_delay) {
		GameObject *obj = this->manager->spawnObject(this->owner->state.index, this->_position, "", this->_name);

		this->_init(obj);
		this->_number--;
		this->_disposable = this->_number == 0;
		this->_delayTimer = 0;
	}
}

void ObjectSpawner::render(int)
{
}
