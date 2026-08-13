//
// Created by PinkySmile on 13/08/2026.
//

#ifndef MIXRTS_WARHEAD_HPP
#define MIXRTS_WARHEAD_HPP


#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"

enum DamageType {
	DMGTYPE_GENERIC,
	DMGTYPE_FIRE,
	DMGTYPE_BULLET,
	DMGTYPE_EXPLOSION,
	DMGTYPE_SONIC,
	//DMGTYPE_ELECTRIC,
	//DMGTYPE_MELEE
};

class Warhead {
private:
	DamageType _dmgType;
	unsigned _radius = 0;
	std::unordered_map<std::string, float> _armor;

public:
	explicit Warhead(const nlohmann::json &data);
	//! @brief Merges the multipliers of an armor file into this warhead.
	//! @param origin The name of the mod whose armor vocabulary is described by data (used to qualify the armor type keys).
	//! @param data The json object mapping armor type names to their multiplier for this warhead.
	void loadArmor(const std::string &origin, const nlohmann::json &data);
	[[nodiscard]] unsigned getRadius() const;
	[[nodiscard]] DamageType getDmgType() const;
	[[nodiscard]] float getMultiplier(const std::string &armor) const;
	[[nodiscard]] const std::unordered_map<std::string, float> &getArmorMultipliers() const;
};


#endif //MIXRTS_WARHEAD_HPP
