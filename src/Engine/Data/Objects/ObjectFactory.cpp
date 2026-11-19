//
// Created by PinkySmile on 18/11/2026.
//

#include "GameObject.hpp"
#include "Missile.hpp"
#include "ObjectFactory.hpp"
#include "Data/Animation.hpp"
#include "Data/Effect.hpp"
#include "Data/MultiSound.hpp"
#include "Data/Weapon.hpp"
#include "Resources/Assert.hpp"

ObjectFactory::ObjectFactory(Faction &faction) :
	_faction(faction)
{
	this->push<GameObject>("base");
	this->push<Missile>("missile");
}

void ObjectFactory::push(const std::string &type, ObjectCreationCb cb)
{
	assert_msg(!this->_creators.contains(type), type);
	this->_creators.emplace(type, cb);
}

GameObject *ObjectFactory::create(
	const Faction &base,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
) const
{
	std::string type = "base";

	assert_exp(!json.contains("type") || json["type"].is_string());
	if (json.contains("type"))
		type = json["type"];
	assert_msg(this->_creators.contains(type), type);
	return this->_creators.at(type)(base, id, animList, sounds, effects, weapons, json);
}
