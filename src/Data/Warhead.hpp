//
// Created by PinkySmile on 13/08/2026.
//

#ifndef MIXRTS_WARHEAD_HPP
#define MIXRTS_WARHEAD_HPP


#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"

enum DamageType {
	DMGTYPE_VOID,
	DMGTYPE_GENERIC,
	DMGTYPE_START,
	DMGTYPE_BULLET = DMGTYPE_START,
	DMGTYPE_MISSILE,
	DMGTYPE_BLAST,
	DMGTYPE_SONIC,
	//DMGTYPE_ELECTRIC,
	//DMGTYPE_FIRE,
	//DMGTYPE_MELEE,
	DMGTYPE_COUNT
};

class Warhead {
private:
	DamageType _dmgType;
	unsigned _radius = 0;
	std::unordered_map<std::string, float> _armor;

public:
	explicit Warhead(const nlohmann::json &data);
	void loadArmor(const std::string &origin, const nlohmann::json &data);
	[[nodiscard]] unsigned getRadius() const;
	[[nodiscard]] DamageType getDmgType() const;
	[[nodiscard]] float getMultiplier(const std::string &armor) const;
	[[nodiscard]] const std::unordered_map<std::string, float> &getArmorMultipliers() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_WARHEAD_HPP
