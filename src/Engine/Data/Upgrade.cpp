//
// Created by PinkySmile on 11/04/2026.
//

#include <sol/sol.hpp>
#include "Resources/Assert.hpp"
#include "Upgrade.hpp"
#include "Faction.hpp"
#include "Mod.hpp"
#include "Utils.hpp"

Upgrade::Upgrade(const Faction &faction, const std::string &id, const nlohmann::json &json) :
	_base(faction)
{
	assert_exp(json.contains("cost"));
	assert_exp(json["cost"].is_object());
	for (auto &[key, value] : json["cost"].items())
		assert_msg(value.is_number(), "json[\"cost\"][\"" + key + "\"]");
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
	assert_exp(!json.contains("group") || json["group"].is_string());
	assert_exp(json.contains("requirements"));
	assert_exp(json["requirements"].is_array());
	assert_exp(!json.contains("build_speed") || json["build_speed"].is_number_unsigned());
	assert_exp(!json.contains("requires_queue_building") || json["requires_queue_building"].is_boolean());
	assert_exp(!json.contains("gives_queue") || json["gives_queue"].is_string());
	assert_exp(!json.contains("queue") || json["queue"].is_string());

	auto &base = faction.getBase();

	this->_id = id;
	this->_cost = json["cost"];
	this->_limit = json.contains("limit") ? json["limit"].get<unsigned>() : UINT32_MAX;
	this->_buildSpeed = json.contains("build_speed") ? json["build_speed"].get<unsigned>() : 0;
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

void Upgrade::pushUserType(sol::state &state)
{
	sol::usertype<Upgrade> type = state.new_usertype<Upgrade>("Upgrade");

	type["build_queue"] = sol::property(&Upgrade::getBuildQueue);
	type["given_queue"] = sol::property(&Upgrade::getGivenQueue);
	type["id"] = sol::property(&Upgrade::getId);
	type["group"] = sol::property(&Upgrade::getGroup);
	type["build_speed"] = sol::property(&Upgrade::getBuildSpeed);
	// TODO: getRequirements() is intentionally not exposed: ReqAndCombinator
	//       owns a vector of non-copyable std::unique_ptr<IReq> and isn't itself
	//       registered as a usertype, so sol2 can't push it by reference.
	type["cost"] = sol::property(&Upgrade::getCost);
	type["limit"] = sol::property(&Upgrade::getLimit);
}
