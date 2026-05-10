//
// Created by PinkySmile on 10/01/2026.
//

#include "nlohmann/json.hpp"
#include "GameManager.hpp"
#include "Animation.hpp"
#include "Building.hpp"
#include "Utils.hpp"
#include "../Resources/Assert.hpp"
#include "../Resources/Game.hpp"

Building::Building(const Building &b) :
	GameObject(b),
	_giveQueue(b._giveQueue),
	_storage(b._storage),
	_showAfterBuild(b._showAfterBuild),
	_power(b._power),
	_building(b._building),
	_producing(b._producing),
	_produced(b._produced),
	_lowerPowerWhenDamaged(b._lowerPowerWhenDamaged)
{
	if (b._buildAnim)
		this->_buildAnim.emplace(*this, *b._buildAnim);
	if (b._producingAnim)
		this->_producingAnim.emplace(*this, *b._producingAnim);
	if (b._producedAnim)
		this->_producedAnim.emplace(*this, *b._producedAnim);
}

Building::Building(
	const Faction &faction,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const nlohmann::json &json
) :
	GameObject(faction, id, animList, sounds, json)
{
	assert_exp(json.contains("build_max_distance"));
	assert_exp(json["build_max_distance"].is_number());
	assert_exp(json.contains("selectable"));
	assert_exp(json["selectable"].is_boolean());
	assert_exp(json.contains("sellable"));
	assert_exp(json["sellable"].is_boolean());
	assert_exp(json.contains("repairable"));
	assert_exp(json["repairable"].is_boolean());
	assert_exp(!json.contains("gives_queue") || json["gives_queue"].is_string());
	assert_exp(!json.contains("storage") || json["storage"].is_object());
	if (json.contains("storage"))
		for (auto &[key, value] : json["storage"].items())
			assert_msg(value.is_number(), "json[\"storage\"][\"" + key + "\"]");
	assert_exp(!json.contains("build_anim") || json["build_anim"].is_null() || json["build_anim"].is_object());
	assert_exp(!json.contains("producing_anim") || json["producing_anim"].is_null() || json["producing_anim"].is_object());
	assert_exp(!json.contains("produced_anim") || json["produced_anim"].is_null() || json["produced_anim"].is_object());
	assert_exp(!json.contains("show_after_build") || json["show_after_build"].is_number());
	assert_exp(!json.contains("power") || json["power"].is_number());
	assert_exp(!json.contains("lower_power_when_damaged") || json["lower_power_when_damaged"].is_boolean());

	auto &base = faction.getBase();

	this->_buildMaxDistance = json["build_max_distance"];
	this->_selectable = json["selectable"];
	this->_sellable = json["sellable"];
	this->_repairable = json["repairable"];

	if (json.contains("gives_queue"))
		this->_giveQueue = RESOURCE(base.getName(), json["gives_queue"]);

	if (json.contains("storage"))
		this->_storage = json["storage"];
	if (json.contains("build_anim") && !json["build_anim"].is_null())
		this->_buildAnim.emplace(*this, animList, json["build_anim"]);
	if (json.contains("producing_anim") && !json["producing_anim"].is_null())
		this->_producingAnim.emplace(*this, animList, json["producing_anim"]);
	if (json.contains("produced_anim") && !json["produced_anim"].is_null())
		this->_producedAnim.emplace(*this, animList, json["produced_anim"]);
	if (json.contains("show_after_build"))
		this->_showAfterBuild = json["show_after_build"];
	else if (this->_buildAnim)
		this->_showAfterBuild = this->_buildAnim->size();
	this->_power = json.contains("power") ? json["power"].get<int>() : 0;
	this->_fake = json.contains("fake") && json["fake"];
	this->_lowerPowerWhenDamaged = json.contains("lower_power_when_damaged") && json["lower_power_when_damaged"];
	if (this->_storage.size() == 1)
		this->animationState.storageKey = this->_storage.begin()->first;

	if (this->_tiles && this->_buildMaxDistance)
		for (size_t y = 0; y < this->_tiles->size(); y++) {
			auto &line = (*this->_tiles)[y];

			for (size_t x = 0; x < line.size(); x++) {
				uint8_t buildingMask = line[x];

				if ((buildingMask & (GameObject::MASK_PLACEMENT_REQUIRED | GameObject::MASK_PLACEMENT_CONCRETE)) == 0)
					continue;

				for (int i1 = -this->_buildMaxDistance - 1; i1 <= this->_buildMaxDistance + 1; i1++)
					for (int i2 = -this->_buildMaxDistance - 1; i2 <= this->_buildMaxDistance + 1; i2++)
						this->_checkTiles.emplace(i1 + x, i2 + y);
			}
		}
}

void Building::update()
{
	if (this->_building && this->_buildAnim && (*this->_buildAnim)()) {
		if (this->_buildAnim->update())
			this->_building = false;
	} else {
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)())
			this->_producingAnim->update();
		if (this->_produced  && this->_producedAnim  && (*this->_producedAnim)()  && this->_producedAnim->update())
			this->_produced = false;
	}
	GameObject::update();
}

void Building::render(int layer)
{
	if (!this->_building || !this->_buildAnim || this->_buildAnim->current() >= this->_showAfterBuild)
		GameObject::render(layer);
	if (this->_building && this->_buildAnim && (*this->_buildAnim)()) {
		if (layer == this->_buildAnim->getLayer())
			this->_displayAnim(this->_buildAnim->getAnim());
	} else {
		if (this->_produced &&  this->_producedAnim && (*this->_producedAnim)() && layer == this->_producedAnim->getLayer())
			this->_displayAnim(this->_producedAnim->getAnim());
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)() && layer == this->_producingAnim->getLayer())
			this->_displayAnim(this->_producingAnim->getAnim());
	}
}

void Building::built()
{
	this->_building = true;
	if (this->_buildAnim)
		this->_buildAnim->reset();
}

void Building::produced()
{
	this->_produced = true;
	if (this->_producedAnim)
		this->_producedAnim->reset();
}

void Building::producing(bool producing)
{
	this->_producing = producing;
}

void Building::setColor(const HSLColor &color)
{
	GameObject::setColor(color);
	if (this->_buildAnim)
		this->_buildAnim->setColor(color);
	if (this->_producingAnim)
		this->_producingAnim->setColor(color);
	if (this->_producedAnim)
		this->_producedAnim->setColor(color);
}

int Building::getPower() const
{
	if (!this->_lowerPowerWhenDamaged)
		return this->_power;
	return this->_power * static_cast<int>(this->_hp) / static_cast<int>(this->_maxHp);
}

const std::unordered_map<std::string, unsigned> &Building::getStorage() const
{
	return this->_storage;
}

const std::optional<std::string> &Building::getGivenQueue() const
{
	return this->_giveQueue;
}

unsigned char Building::getBuildMaxDistance() const
{
	return this->_buildMaxDistance;
}

bool Building::isSelectable() const
{
	return this->_selectable;
}

bool Building::isSellable() const
{
	return this->_sellable;
}

bool Building::isRepairable() const
{
	return this->_repairable;
}

bool Building::isPrimary() const
{
	return this->_isPrimary;
}

void Building::setPrimary(bool primary)
{
	this->_isPrimary = primary;
}
