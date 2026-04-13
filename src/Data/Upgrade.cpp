//
// Created by PinkySmile on 11/04/2026.
//

#include "Upgrade.hpp"
#include "Faction.hpp"
#include "Mod.hpp"

Upgrade::Upgrade(const Faction &faction, const std::string &id, const nlohmann::json &json) :
	_base(faction)
{
	auto &base = faction.getBase();

	this->_id = id;
	this->_cost = json["cost"];
	this->_limit = json.contains("limit") ? json["limit"].get<unsigned>() : UINT32_MAX;
	this->_buildSpeed = json.contains("speed") ? json["speed"].get<unsigned>() : 0;
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);

	if (json.contains("queue"))
		this->_buildQueue = RESOURCE(base.getName(), json["queue"]);
	if (json.contains("gives_queue"))
		this->_giveQueue = RESOURCE(base.getName(), json["gives_queue"]);
	if (json.contains("group"))
		this->_group = RESOURCE(base.getName(), "group." + json["group"].get<std::string>());
	else
		this->_group = id;
}

const std::optional<std::string> &Upgrade::getBuildQueue() const
{
	return this->_buildQueue;
}

const std::optional<std::string> &Upgrade::getGivenQueue() const
{
	return this->_giveQueue;
}

const std::string &Upgrade::getId() const
{
	return this->_id;
}

const std::string &Upgrade::getGroup() const
{
	return this->_group;
}

unsigned Upgrade::getBuildSpeed() const
{
	return this->_buildSpeed;
}

const ReqAndCombinator &Upgrade::getRequirements() const
{
	return *this->_requirements;
}

unsigned Upgrade::getLimit() const
{
	return this->_limit;
}

const std::unordered_map<std::string, unsigned> &Upgrade::getCost() const
{
	return this->_cost;
}
