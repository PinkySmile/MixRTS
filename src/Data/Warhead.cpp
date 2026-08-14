//
// Created by PinkySmile on 13/08/2026.
//

#include "nlohmann/json.hpp"
#include "Warhead.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

Warhead::Warhead(const nlohmann::json &data)
{
	assert_exp(data.contains("radius") && data["radius"].is_number_unsigned());
	assert_exp(data.contains("dmg_type") && data["dmg_type"].is_string());

	this->_radius = data["radius"];

	std::string dmgType = data["dmg_type"];

	if (dmgType == "blast")
		this->_dmgType = DMGTYPE_BLAST;
	else if (dmgType == "bullet")
		this->_dmgType = DMGTYPE_BULLET;
	else if (dmgType == "missile")
		this->_dmgType = DMGTYPE_MISSILE;
	else if (dmgType == "void")
		this->_dmgType = DMGTYPE_VOID;
	else if (dmgType == "sonic")
		this->_dmgType = DMGTYPE_SONIC;
	//else if (dmgType == "fire")
	//	this->_dmgType = DMGTYPE_FIRE;
	//else if (dmgType == "electric")
	//	this->_dmgType = DMGTYPE_ELECTRIC;
	//else if (dmgType == "melee")
	//	this->_dmgType = DMGTYPE_MELEE;
	else
		assert_not_reached_msg("Invalid dmg_type: " + dmgType);
}

void Warhead::loadArmor(const std::string &origin, const nlohmann::json &data)
{
	assert_exp(data.is_object());
	for (auto &[armorType, multiplier] : data.items()) {
		assert_msg(multiplier.is_number(), armorType);

		std::string fullArmorType = RESOURCE(origin, armorType);

		assert_msg(!this->_armor.contains(fullArmorType), fullArmorType);
		this->_armor[fullArmorType] = multiplier.get<float>();
	}
}

unsigned Warhead::getRadius() const
{
	return this->_radius;
}

DamageType Warhead::getDmgType() const
{
	return this->_dmgType;
}

float Warhead::getMultiplier(const std::string &armor) const
{
	assert_msg(this->_armor.contains(armor), armor);
	return this->_armor.at(armor);
}

const std::unordered_map<std::string, float> &Warhead::getArmorMultipliers() const
{
	return this->_armor;
}
