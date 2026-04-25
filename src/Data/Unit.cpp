//
// Created by PinkySmile on 20/04/2026.
//

#include "Unit.hpp"
#include "Animation.hpp"
#include "ConditionalAnim.hpp"
#include "GameManager.hpp"
#include "Faction.hpp"
#include "Player.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"


Unit::Unit(const Unit &u) :
	GameObject(u),
	_onDeploy(u._onDeploy),
	_speed(u._speed)
{
}

Unit::Unit(
	const Faction &base,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const nlohmann::json &json
) :
	GameObject(base, id, animList, sounds, json)
{
	assert_exp(json.contains("speed"));
	assert_exp(json["speed"].is_number());
	assert_exp(!json.contains("deploy") || json["deploy"].is_object());

	this->_speed = json["speed"];
	if (json.contains("deploy")) {
		assert_exp(!json["deploy"].contains("direction") || json["deploy"]["direction"].is_number());
		assert_exp(!json["deploy"].contains("spawn_building") || json["deploy"]["spawn_building"].is_object());

		this->_onDeploy.emplace();
		if (json["deploy"].contains("direction"))
			this->_onDeploy->direction = json["deploy"]["direction"];
		if (json["deploy"].contains("spawn_building")) {
			this->_onDeploy->spawnBuilding.emplace();
			assert_exp(json["deploy"]["spawn_building"].contains("name"));
			assert_exp(json["deploy"]["spawn_building"]["name"].is_string());
			assert_exp(json["deploy"]["spawn_building"].contains("queue"));
			assert_exp(json["deploy"]["spawn_building"]["queue"].is_string());
			assert_exp(json["deploy"]["spawn_building"].contains("offset"));
			assert_exp(json["deploy"]["spawn_building"]["offset"].is_object());
			assert_exp(json["deploy"]["spawn_building"]["offset"].contains("x"));
			assert_exp(json["deploy"]["spawn_building"]["offset"]["x"].is_number());
			assert_exp(json["deploy"]["spawn_building"]["offset"].contains("y"));
			assert_exp(json["deploy"]["spawn_building"]["offset"]["y"].is_number());
			this->_onDeploy->spawnBuilding->building = RESOURCE(base.getBase().getName(), json["deploy"]["spawn_building"]["name"]);
			this->_onDeploy->spawnBuilding->queue    = RESOURCE(base.getBase().getName(), json["deploy"]["spawn_building"]["queue"]);
			this->_onDeploy->spawnBuilding->offset.x = json["deploy"]["spawn_building"]["offset"]["x"];
			this->_onDeploy->spawnBuilding->offset.y = json["deploy"]["spawn_building"]["offset"]["y"];
		}
	}
}

const std::optional<UnitDeployParams> &Unit::getOnDeploy() const
{
	return this->_onDeploy;
}

unsigned Unit::getSpeed() const
{
	return this->_speed;
}

bool Unit::isDeployable() const
{
	if (this->_onDeploy)
		return true;
	return GameObject::isDeployable();
}

bool Unit::deploy()
{
	if (this->_onDeploy) {
		if (this->_onDeploy->direction)
			this->_orderQueue.emplace_back(TurnOrder{*this->_onDeploy->direction});
		this->_orderQueue.emplace_back(DeployOrder{});
	}
	return GameObject::deploy();
}

bool Unit::_handleOrder(const Order &o)
{
	if (std::holds_alternative<DeployOrder>(this->_orderQueue.front())) {
		assert_exp(this->_onDeploy);
		if (this->_onDeploy->spawnBuilding) {
			auto &s = *this->_onDeploy->spawnBuilding;

			if (this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building)) {
				this->manager->spawnBuilding(this->owner->state.index, this->_position + s.offset, s.queue, s.building, false);
				this->_disposable = true;
			} else
				this->owner->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300);
		}
		return true;
	}
	return GameObject::_handleOrder(o);
}

bool Unit::canDeploy() const
{
	if (this->_onDeploy) {
		if (this->_onDeploy->spawnBuilding) {
			auto &s = *this->_onDeploy->spawnBuilding;

			return this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building);
		}
	}
	return GameObject::canDeploy();
}
