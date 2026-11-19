//
// Created by PinkySmile on 18/11/2026.
//

#include "Missile.hpp"

Missile::Missile(
	const Faction &base,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
) :
	GameObject(base, id, animList, sounds, effects, weapons, json)
{
	this->_behavior = BEHAVIOR_MISSILE;
	this->_solid = false;
	this->_selectable = false;
	this->_autoAttack = false;
	this->_triggersAutoAttack = false;
	this->_ignoreTerrain = true;
	assert_msg(json.contains("death"), "A missile object needs a \"death\" block");
	assert_msg(!this->_barrels.empty(), "A missile object needs at least one barrel");
}

GameObject *Missile::copy() const
{
	return new Missile(*this);
}
