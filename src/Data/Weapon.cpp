//
// Created by PinkySmile on 03/08/2026.
//

#include <cpptrace/from_current.hpp>
#include <nlohmann/json.hpp>
#include "Animation.hpp"
#include "ConditionalAnim.hpp"
#include "Weapon.hpp"
#include "Effect.hpp"
#include "MultiSound.hpp"
#include "Warhead.hpp"
#include "Mod.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

Weapon::Weapon(
	const Mod &base,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Warhead> &warheads,
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
	assert_exp(!data.contains("projectile") || data["projectile"].is_object());
	assert_exp(!data.contains("inaccuracy_dist") || data["inaccuracy_dist"].is_number());
	assert_exp(!data.contains("inaccuracy_base") || data["inaccuracy_base"].is_number());
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
	assert_exp(this->_type != WEAPONTYPE_PROJECTILE || data.contains("projectile"));

	this->_damage = data["damage"];
	this->_range = data["range"];
	this->_canHitTags = data["can_hit_tags"];

	std::string warhead = RESOURCE(base.getName(), data["warhead"].get<std::string>());

	assert_msg(warheads.contains(warhead), warhead);
	this->_warhead = &warheads.at(warhead);

	if (data.contains("projectile"))
		TRY {
			this->_projectile.emplace(base, animList, effects, data["projectile"]);
		} CATCH_AND_RETHROW("In key \"projectile\"\n")
	if (data.contains("inaccuracy_base"))
		this->_inaccuracyBase = data["inaccuracy_base"];
	if (data.contains("inaccuracy_dist"))
		this->_inaccuracyDist = data["inaccuracy_dist"];
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

const Warhead *Weapon::getWarhead() const
{
	return this->_warhead;
}

const std::vector<std::string> &Weapon::canHitTags() const
{
	return this->_canHitTags;
}

double Weapon::getInaccuracy(double dist) const
{
	return this->_inaccuracyBase + this->_inaccuracyDist * dist;
}

const std::optional<WeaponProjectile> & Weapon::getProjectileData() const
{
	return this->_projectile;
}

WeaponProjectile::WeaponProjectile(
	const Mod &base,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, Effect> &effects,
	const nlohmann::json &data
)
{
	assert_exp(data.contains("anim") && data["anim"].is_object());
	assert_exp(data.contains("speed") && data["speed"].is_number());
	assert_exp(!data.contains("trail") || !data["trail"].is_object());
	assert_exp(!data.contains("trail_interval") || !data["trail_interval"].is_number());
	assert_exp(!data.contains("directions") || data["directions"].is_number());

	std::optional<unsigned> directions;

	this->speed = data["speed"];
	if (data.contains("trail_interval"))
		this->trailInterval = data["trail_interval"];
	if (data.contains("trail")) {
		std::string trail = RESOURCE(base.getName(), data["trail"].get<std::string>());

		assert_msg(effects.contains(trail), trail);
		this->trail = &effects.at(trail);
	}
	if (data.contains("directions")) {
		directions = data["directions"].get<unsigned>();
		assert_msg(*directions % 8 == 0, "Number of directions must be divisible by 8");
	}
	if (directions) {
		this->anims.reserve(*directions);
		for (size_t i = 0; i < *directions; i++)
			this->anims.emplace_back(base, nullptr, animList, data["anim"], i);
	} else
		this->anims.emplace_back(base, nullptr, animList, data["anim"]);
}
