//
// Created by PinkySmile on 18/11/2026.
//

#ifndef MIXRTS_MISSILE_HPP
#define MIXRTS_MISSILE_HPP


#include "GameObject.hpp"

class Missile : public GameObject {
public:
	Missile(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, Weapon> &weapons,
		const nlohmann::json &json
	);

	GameObject *copy() const override;
};


#endif //MIXRTS_MISSILE_HPP
