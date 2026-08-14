//
// Created by PinkySmile on 03/08/2026.
//

#ifndef MIXRTS_WEAPON_HPP
#define MIXRTS_WEAPON_HPP


#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"

enum WeaponType {
	WEAPONTYPE_HITSCAN,
	WEAPONTYPE_PROJECTILE
};

struct WeaponProjectile {

};

class Weapon {
private:
	WeaponType _type;
	const Effect *_effect = nullptr;
	const MultiSound *_sound = nullptr;
	const Effect *_hitEffect = nullptr;
	unsigned _damage = 0;
	unsigned _range = 0;
	double _inaccuracyDist = 0;
	double _inaccuracyBase = 0;
	const Warhead *_warhead = nullptr;
	std::vector<std::string> _canHitTags;
	std::optional<WeaponProjectile> _projectile;

public:
	Weapon(
		const Mod &base,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Warhead> &warheads,
		const nlohmann::json &data
	);
	[[nodiscard]] WeaponType getType() const;
	[[nodiscard]] const Effect *getEffect() const;
	[[nodiscard]] const MultiSound *getSound() const;
	[[nodiscard]] const Effect *getHitEffect() const;
	[[nodiscard]] unsigned getDamage() const;
	[[nodiscard]] unsigned getRange() const;
	[[nodiscard]] const Warhead *getWarhead() const;
	[[nodiscard]] const std::vector<std::string> &canHitTags() const;
	[[nodiscard]] double getInaccuracy(double dist) const;
	[[nodiscard]] const std::optional<WeaponProjectile> &getProjectileData() const;
};


#endif //MIXRTS_WEAPON_HPP
