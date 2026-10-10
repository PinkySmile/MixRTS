//
// Created by PinkySmile on 03/08/2026.
//

#ifndef MIXRTS_WEAPON_HPP
#define MIXRTS_WEAPON_HPP


#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"

enum WeaponNeeds {
	WEAPONNEED_NONE = 0,
	WEAPONNEED_CAPTURE = 1 << 0
};

enum WeaponType {
	WEAPONTYPE_HITSCAN,
	WEAPONTYPE_PROJECTILE,
	WEAPONTYPE_ENTER
};

struct MYDLL_API WeaponTakeControl {
	std::vector<std::string> targets;
	unsigned baseTime = 0;
	unsigned timeLossHit = 0;

	WeaponTakeControl(const nlohmann::json &json);
};

struct MYDLL_API WeaponProjectile {
	std::vector<ConditionalAnim> anims;
	std::pair<unsigned, unsigned> bounce;
	const Effect *trail = nullptr;
	unsigned trailInterval;
	unsigned range = 0;
	unsigned damageRamp = 0;
	unsigned rampDelay = 0;
	float speed;
	float velocity = 0;
	float gravity = 0;
	float startY = 0;
	bool rampingDamage = false;
	bool hitTravel = false;
	bool homing = false;
	bool arc = false;
	bool shadow = false;
	bool falling = false;
	bool blockedByWall = false;

	WeaponProjectile(
		const Mod &base,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, Effect> &effects,
		const nlohmann::json &json
	);
};

class MYDLL_API Weapon {
private:
	WeaponType _type;
	WeaponNeeds _needs = WEAPONNEED_NONE;
	const Effect *_effect = nullptr;
	const MultiSound *_sound = nullptr;
	const Effect *_hitEffect = nullptr;
	unsigned _damage = 0;
	unsigned _range = 0;
	double _inaccuracyDist = 0;
	double _inaccuracyBase = 0;
	const Warhead *_warhead = nullptr;
	std::optional<std::string> _cursor;
	std::vector<std::string> _canHitTags;
	std::optional<WeaponProjectile> _projectile;
	std::optional<WeaponTakeControl> _takeControl;
	bool _sendForward = false;
	bool _ground = true;
	bool _air = false;

public:
	Weapon(
		const Mod &base,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Warhead> &warheads,
		const nlohmann::json &data
	);
	[[nodiscard]] const std::optional<std::string> &getCursor() const;
	[[nodiscard]] WeaponType getType() const;
	[[nodiscard]] const Effect *getEffect() const;
	[[nodiscard]] const MultiSound *getSound() const;
	[[nodiscard]] const Effect *getHitEffect() const;
	[[nodiscard]] unsigned getDamage() const;
	[[nodiscard]] unsigned getRange() const;
	[[nodiscard]] const Warhead *getWarhead() const;
	[[nodiscard]] const std::vector<std::string> &canHitTags() const;
	[[nodiscard]] double getInaccuracy(double dist) const;
	[[nodiscard]] const std::optional<WeaponTakeControl> &getTakeControl() const;
	[[nodiscard]] const std::optional<WeaponProjectile> &getProjectileData() const;
	[[nodiscard]] bool sendsForward() const;
	[[nodiscard]] WeaponNeeds getNeeds() const;
	[[nodiscard]] bool hitsGround() const;
	[[nodiscard]] bool hitsAir() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_WEAPON_HPP
