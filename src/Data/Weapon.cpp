//
// Created by PinkySmile on 03/08/2026.
//

#include "nlohmann/json.hpp"
#include "Weapon.hpp"
#include "Effect.hpp"
#include "MultiSound.hpp"
#include "Mod.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

Weapon::Weapon(
	const Mod &base,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const nlohmann::json &data
)
{
	assert_exp(data.contains("type") && data["type"].is_string());
	assert_exp(data.contains("damage") && data["damage"].is_number_unsigned());
	assert_exp(data.contains("range") && data["range"].is_number_unsigned());
	assert_exp(data.contains("warhead") && data["warhead"].is_string());
	assert_exp(data.contains("can_hit_tags") && data["can_hit_tags"].is_array());
	for (auto &j : data["can_hit_tags"])
		assert_exp(j.is_string());
	assert_exp(!data.contains("effect") || data["effect"].is_null() || data["effect"].is_string());
	assert_exp(!data.contains("hit_effect") || data["hit_effect"].is_null() || data["hit_effect"].is_string());
	assert_exp(!data.contains("sound") || data["sound"].is_null() || data["sound"].is_string());

	std::string type = data["type"];

	if (type == "hitscan")
		this->_type = WEAPONTYPE_HITSCAN;
	else if (type == "projectile")
		this->_type = WEAPONTYPE_PROJECTILE;
	else
		assert_not_reached_msg("Invalid weapon type \"" + type + "\"");

	this->_damage = data["damage"];
	this->_range = data["range"];
	this->_warhead = data["warhead"];
	this->_canHitTags = data["can_hit_tags"];

	if (data.contains("effect") && !data["effect"].is_null()) {
		std::string effect = RESOURCE(base.getName(), data["effect"].get<std::string>());

		assert_msg(effects.contains(effect), effect);
		this->_effect = &effects.at(effect);
	}
	if (data.contains("hit_effect") && !data["hit_effect"].is_null()) {
		std::string hitEffect = RESOURCE(base.getName(), data["hit_effect"].get<std::string>());

		assert_msg(effects.contains(hitEffect), hitEffect);
		this->_hitEffect = &effects.at(hitEffect);
	}
	if (data.contains("sound") && !data["sound"].is_null()) {
		std::string sound = RESOURCE(base.getName(), data["sound"].get<std::string>());

		assert_msg(sounds.contains(sound), sound);
		this->_sound = &sounds.at(sound);
	}
}

WeaponType Weapon::getType() const
{
	return this->_type;
}

const Effect *Weapon::getEffect() const
{
	return this->_effect;
}

const MultiSound *Weapon::getSound() const
{
	return this->_sound;
}

const Effect *Weapon::getHitEffect() const
{
	return this->_hitEffect;
}

unsigned Weapon::getDamage() const
{
	return this->_damage;
}

unsigned Weapon::getRange() const
{
	return this->_range;
}

const std::string &Weapon::getWarhead() const
{
	return this->_warhead;
}

const std::vector<std::string> &Weapon::canHitTags() const
{
	return this->_canHitTags;
}
