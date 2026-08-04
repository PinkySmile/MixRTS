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

class Weapon {
private:
	WeaponType _type;
	const Effect *_effect = nullptr;
	const MultiSound *_sound = nullptr;
	const Effect *_hitEffect = nullptr;
	unsigned _damage = 0;
	unsigned _range = 0;
	std::string _warhead;
	std::vector<std::string> _canHitTags;

public:
	Weapon(
		const Mod &base,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &data
	);
	[[nodiscard]] WeaponType getType() const;
	[[nodiscard]] const Effect *getEffect() const;
	[[nodiscard]] const MultiSound *getSound() const;
	[[nodiscard]] const Effect *getHitEffect() const;
	[[nodiscard]] unsigned getDamage() const;
	[[nodiscard]] unsigned getRange() const;
	[[nodiscard]] const std::string &getWarhead() const;
	[[nodiscard]] const std::vector<std::string> &canHitTags() const;
};


#endif //MIXRTS_WEAPON_HPP
