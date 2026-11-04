//
// Created by PinkySmile on 03/08/2026.
//

#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
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
	assert_exp(!data.contains("cursor") || data["cursor"].is_string());
	assert_exp(data.contains("can_hit_tags") && data["can_hit_tags"].is_array());
	for (auto &j : data["can_hit_tags"])
		assert_exp(j.is_string());
	assert_exp(!data.contains("projectile") || data["projectile"].is_object());
	assert_exp(!data.contains("take_control") || data["take_control"].is_object());
	assert_exp(!data.contains("send_forward") || data["send_forward"].is_boolean());
	assert_exp(!data.contains("inaccuracy_dist") || data["inaccuracy_dist"].is_number());
	assert_exp(!data.contains("inaccuracy_base") || data["inaccuracy_base"].is_number());
	assert_exp(!data.contains("effect") || data["effect"].is_null() || data["effect"].is_string());
	assert_exp(!data.contains("hit_effect") || data["hit_effect"].is_null() || data["hit_effect"].is_string());
	assert_exp(!data.contains("sound") || data["sound"].is_null() || data["sound"].is_string());
	assert_exp(!data.contains("ground") || data["ground"].is_boolean());
	assert_exp(!data.contains("air") || data["air"].is_boolean());

	std::string type = data["type"];

	if (type == "hitscan")
		this->_type = WEAPONTYPE_HITSCAN;
	else if (type == "projectile")
		this->_type = WEAPONTYPE_PROJECTILE;
	else if (type == "enter")
		this->_type = WEAPONTYPE_ENTER;
	else
		assert_not_reached_msg("Invalid weapon type \"" + type + "\"");
	assert_exp(this->_type != WEAPONTYPE_PROJECTILE || data.contains("projectile"));

	this->_damage = data["damage"];
	this->_range = data["range"];
	this->_canHitTags = data["can_hit_tags"];

	std::string warhead = RESOURCE(base.getName(), data["warhead"].get<std::string>());

	assert_msg(warheads.contains(warhead), warhead);
	this->_warhead = &warheads.at(warhead);

	if (data.contains("cursor"))
		this->_cursor = UI::stringToCursor(data["cursor"]);
	if (data.contains("air"))
		this->_air = data["air"];
	if (data.contains("ground"))
		this->_ground = data["ground"];
	if (data.contains("take_control"))
		this->_takeControl.emplace(data["take_control"]);
	if (data.contains("send_forward"))
		this->_sendForward = data["send_forward"];
	if (data.contains("inaccuracy_base"))
		this->_inaccuracyBase = data["inaccuracy_base"];
	if (data.contains("inaccuracy_dist"))
		this->_inaccuracyDist = data["inaccuracy_dist"];
	if (data.contains("projectile"))
		TRY {
			this->_projectile.emplace(base, animList, effects, data["projectile"]);
		} CATCH_AND_RETHROW("In key \"projectile\"\n")
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

const std::optional<UI::Cursor> &Weapon::getCursor() const
{
	return this->_cursor;
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

const std::optional<WeaponTakeControl> & Weapon::getTakeControl() const
{
	return this->_takeControl;
}

const std::optional<WeaponProjectile> &Weapon::getProjectileData() const
{
	return this->_projectile;
}

bool Weapon::sendsForward() const
{
	return this->_sendForward;
}

WeaponNeeds Weapon::getNeeds() const
{
	return this->_needs;
}

bool Weapon::hitsGround() const
{
	return this->_ground;
}

bool Weapon::hitsAir() const
{
	return this->_air;
}

WeaponTakeControl::WeaponTakeControl(const nlohmann::json &json)
{
	assert_exp(json.contains("targets") && json["targets"].is_array());
	for (size_t i = 0; i < json["targets"].size(); i++)
		assert_msg(json["targets"][i].is_string(), "json[\"targets\"][" + std::to_string(i) + "]");
	assert_exp(!json.contains("base_time") || json["base_time"].is_number_unsigned());
	assert_exp(!json.contains("time_loss_hit") || json["time_loss_hit"].is_number_unsigned());

	this->targets = json["targets"];
	if (json.contains("base_time"))
		this->baseTime = json["base_time"];
	if (json.contains("time_loss_hit"))
		this->timeLossHit = json["time_loss_hit"];
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
	assert_exp(!data.contains("trail") || data["trail"].is_string());
	assert_exp(!data.contains("range") || data["range"].is_number());
	assert_exp(!data.contains("ramp_delay") || data["ramp_delay"].is_number());
	assert_exp(!data.contains("damage_ramp") || data["damage_ramp"].is_number());
	assert_exp(!data.contains("homing") || data["homing"].is_boolean());
	assert_exp(!data.contains("hit_travel") || data["hit_travel"].is_boolean());
	assert_exp(!data.contains("trail_interval") || data["trail_interval"].is_number());
	assert_exp(!data.contains("directions") || data["directions"].is_number());
	assert_exp(!data.contains("arc") || data["arc"].is_boolean());
	assert_exp(!data.contains("shadow") || data["shadow"].is_boolean());
	assert_exp(!data.contains("falling") || data["falling"].is_boolean());
	assert_exp(!data.contains("blocked_by_wall") || data["blocked_by_wall"].is_boolean());
	assert_exp(!data.contains("bounce") || (data["bounce"].is_array() && data["bounce"].size() == 2));
	assert_exp(!data.contains("v") || data["v"].is_number());
	assert_exp(!data.contains("y") || data["y"].is_number());
	assert_exp(!data.contains("g") || data["g"].is_number());

	std::optional<unsigned> directions;

	this->speed = data["speed"];

	if (data.contains("bounce"))
		this->bounce = { data["bounce"][0], data["bounce"][1] };
	if (data.contains("blocked_by_wall"))
		this->blockedByWall = data["blocked_by_wall"];
	if (data.contains("falling"))
		this->falling = data["falling"];
	if (data.contains("ramp_delay"))
		this->rampDelay = data["ramp_delay"];
	if (data.contains("damage_ramp"))
		this->damageRamp = data["damage_ramp"];
	if (data.contains("range"))
		this->range = data["range"];
	if (data.contains("hit_travel"))
		this->hitTravel = data["hit_travel"];
	if (data.contains("homing"))
		this->homing = data["homing"];
	if (data.contains("trail_interval"))
		this->trailInterval = data["trail_interval"];
	if (data.contains("arc"))
		this->arc = data["arc"];
	if (data.contains("shadow"))
		this->shadow = data["shadow"];
	if (data.contains("y"))
		this->startY = data["y"];
	if (data.contains("v"))
		this->velocity = data["v"];
	if (data.contains("g"))
		this->gravity = data["g"];
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

void Weapon::pushUserType(sol::state &state)
{
	state.new_enum<WeaponType>("Weapon_WeaponType", {
		{"HITSCAN",    WEAPONTYPE_HITSCAN},
		{"PROJECTILE", WEAPONTYPE_PROJECTILE},
		{"ENTER",      WEAPONTYPE_ENTER},
	});
	state.new_enum<WeaponNeeds>("Weapon_WeaponNeeds", {
		{"NONE",    WEAPONNEED_NONE},
		{"CAPTURE", WEAPONNEED_CAPTURE},
	});

	sol::usertype<WeaponTakeControl> takeControlType = state.new_usertype<WeaponTakeControl>("Weapon_WeaponTakeControl");

	takeControlType["targets"] = &WeaponTakeControl::targets;
	takeControlType["base_time"] = &WeaponTakeControl::baseTime;
	takeControlType["time_loss_hit"] = &WeaponTakeControl::timeLossHit;

	sol::usertype<WeaponProjectile> projectileType = state.new_usertype<WeaponProjectile>("WWeapon_eaponProjectile");

	projectileType["bounce"] = &WeaponProjectile::bounce;
	projectileType["trail"] = &WeaponProjectile::trail;
	projectileType["trail_interval"] = &WeaponProjectile::trailInterval;
	projectileType["range"] = &WeaponProjectile::range;
	projectileType["damage_ramp"] = &WeaponProjectile::damageRamp;
	projectileType["ramp_delay"] = &WeaponProjectile::rampDelay;
	projectileType["speed"] = &WeaponProjectile::speed;
	projectileType["velocity"] = &WeaponProjectile::velocity;
	projectileType["gravity"] = &WeaponProjectile::gravity;
	projectileType["start_y"] = &WeaponProjectile::startY;
	projectileType["ramping_damage"] = &WeaponProjectile::rampingDamage;
	projectileType["hit_travel"] = &WeaponProjectile::hitTravel;
	projectileType["homing"] = &WeaponProjectile::homing;
	projectileType["arc"] = &WeaponProjectile::arc;
	projectileType["shadow"] = &WeaponProjectile::shadow;
	projectileType["falling"] = &WeaponProjectile::falling;

	sol::usertype<Weapon> type = state.new_usertype<Weapon>("Weapon");

	type["type"] = sol::property(&Weapon::getType);
	type["effect"] = sol::property(&Weapon::getEffect);
	type["sound"] = sol::property(&Weapon::getSound);
	type["hit_effect"] = sol::property(&Weapon::getHitEffect);
	type["damage"] = sol::property(&Weapon::getDamage);
	type["range"] = sol::property(&Weapon::getRange);
	type["warhead"] = sol::property(&Weapon::getWarhead);
	type["hit_tags"] = sol::property(&Weapon::canHitTags);
	type["inaccuracy"] = sol::property(&Weapon::getInaccuracy);
	type["take_control"] = sol::property(&Weapon::getTakeControl);
	type["projectile_data"] = sol::property(&Weapon::getProjectileData);
	type["needs"] = sol::property(&Weapon::getNeeds);
	type["sends_forward"] = sol::property(&Weapon::sendsForward);
	type["hits_ground"] = sol::property(&Weapon::hitsGround);
	type["hits_air"] = sol::property(&Weapon::hitsAir);
}
