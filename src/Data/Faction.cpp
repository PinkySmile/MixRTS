//
// Created by PinkySmile on 29/03/2026.
//

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include "Resources/Assert.hpp"
#include "Faction.hpp"
#include "Mod.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "Effect.hpp"
#include "Weapon.hpp"
#include "Warhead.hpp"
#include "GameObject.hpp"
#include "D2K/D2KUI.hpp"

Faction::Faction(const Mod &base, const std::string &name, const nlohmann::json &data) :
	_base(base),
	_name(name)
{
	assert_exp(data.contains("can_sell") && data["can_sell"].is_boolean());
	assert_exp(data.contains("can_repair") && data["can_repair"].is_boolean());
	assert_exp(data.contains("regress_tech") && data["regress_tech"].is_boolean());
	assert_exp(!data.contains("primary_queue") || data["primary_queue"].is_string());
	assert_exp(data.contains("has_radar") && data["has_radar"].is_boolean());
	assert_exp(data.contains("radar_powered") && data["radar_powered"].is_boolean());
	assert_exp(!data.contains("repairing_animation") || data["repairing_animation"].is_object());

	assert_exp(data.contains("sounds") && data["sounds"].is_array());
	for (size_t i = 0; i < data["sounds"].size(); i++)
		assert_msg(data["sounds"][i].is_string(), "data[\"sounds\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("objects") && data["objects"].is_array());
	for (size_t i = 0; i < data["objects"].size(); i++)
		assert_msg(data["objects"][i].is_string(), "data[\"objects\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("animations") && data["animations"].is_array());
	for (size_t i = 0; i < data["animations"].size(); i++)
		assert_msg(data["animations"][i].is_string(), "data[\"animations\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("upgrades") && data["upgrades"].is_array());
	for (size_t i = 0; i < data["upgrades"].size(); i++)
		assert_msg(data["upgrades"][i].is_string(), "data[\"upgrades\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("effects") && data["effects"].is_array());
	for (size_t i = 0; i < data["effects"].size(); i++)
		assert_msg(data["effects"][i].is_string(), "data[\"effects\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("weapons") && data["weapons"].is_array());
	for (size_t i = 0; i < data["weapons"].size(); i++)
		assert_msg(data["weapons"][i].is_string(), "data[\"weapons\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("warheads") && data["warheads"].is_array());
	for (size_t i = 0; i < data["warheads"].size(); i++)
		assert_msg(data["warheads"][i].is_string(), "data[\"warheads\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("starting_money") && data["starting_money"].is_object());
	for (auto &[key, value] : data["starting_money"].items())
		assert_msg(value.is_number(), "data[\"starting_money\"][\"" + key + "\"]");

	assert_exp(data.contains("place") && data["place"].is_object());
	assert_exp(data["place"].contains("best") && data["place"]["best"].is_array());
	for (size_t i = 0; i < data["place"]["best"].size(); i++)
		assert_msg(data["place"]["best"][i].is_string(), "data[\"place\"][\"best\"][" + std::to_string(i) + "]");

	assert_exp(data["place"].contains("ok") && data["place"]["ok"].is_array());
	for (size_t i = 0; i < data["place"]["ok"].size(); i++)
		assert_msg(data["place"]["ok"][i].is_string(), "data[\"place\"][\"ok\"][" + std::to_string(i) + "]");

	assert_exp(data["place"].contains("bad") && data["place"]["bad"].is_array());
	for (size_t i = 0; i < data["place"]["bad"].size(); i++)
		assert_msg(data["place"]["bad"][i].is_string(), "data[\"place\"][\"bad\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("starting_objects") && data["starting_objects"].is_array());
	for (size_t i = 0; i < data["starting_objects"].size(); i++) {
		assert_msg(data["starting_objects"][i].is_object(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i].contains("name"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i].contains("queue"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i]["name"].is_string(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i]["queue"].is_string(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(!data["starting_objects"][i].contains("offset") || data["starting_objects"][i]["offset"].is_object(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		if (data["starting_objects"][i].contains("offset")) {
			assert_msg(data["starting_objects"][i]["offset"].contains("x"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"].contains("y"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"]["x"].is_number(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"]["y"].is_number(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		}
	}

	this->_canSell = data["can_sell"];
	this->_canRepair = data["can_repair"];
	this->_objects = data["objects"];
	this->_animations = data["animations"];
	this->_upgrades = data["upgrades"];
	this->_effects = data["effects"];
	this->_weapons = data["weapons"];
	this->_warheads = data["warheads"];
	this->_sounds = data["sounds"];
	this->_startingMoney = data["starting_money"];
	this->_uiData = data["ui"];
	this->_regressTech = data["regress_tech"];
	this->_hasRadar = data["has_radar"];
	this->_radarPowered = data["radar_powered"];

	if (data.contains("repairing_animation"))
		this->_repairAnim = data["repairing_animation"];
	if (data.contains("primary_queue"))
		this->_primaryQueue = data["primary_queue"].get<std::string>();
	for (const auto &p : data["place"]["best"]) {
		this->_placementCursors[0].emplace_back();
		this->_placementCursors[0].back().load(base.getBase() / p);
	}
	for (const auto &p : data["place"]["ok"]) {
		this->_placementCursors[1].emplace_back();
		this->_placementCursors[1].back().load(base.getBase() / p);
	}
	for (const auto &p : data["place"]["bad"]) {
		this->_placementCursors[2].emplace_back();
		this->_placementCursors[2].back().load(base.getBase() / p);
	}
	for (const auto &p : data["starting_objects"]) {
		StartObject b;

		b.name  = RESOURCE(base.getName(), p["name"]);
		b.queue = RESOURCE(base.getName(), p["queue"]);
		if (p.contains("offset")) {
			b.offset.x = p["offset"]["x"];
			b.offset.y = p["offset"]["y"];
		}
		this->_startingObjects.push_back(b);
	}
}

void Faction::load(const std::vector<std::string> &allFactions)
{
	TRY {
		for (auto &a: this->_animations)
			this->_base.loadAnimations(a, this->_loadedAnimations);
		for (auto &a: this->_sounds)
			this->_base.loadSounds(a, this->_loadedSounds);
		for (auto &e: this->_effects)
			this->_base.loadEffects(e, this->_loadedAnimations, this->_loadedSounds, this->_loadedEffects);
		for (auto &w: this->_warheads)
			this->_base.loadWarheads(w, this->_loadedWarheads);
		if (!this->_warheads.empty())
			this->_base.loadArmors(allFactions, this->_loadedWarheads);
		for (auto &w: this->_weapons)
			this->_base.loadWeapons(w, this->_loadedEffects, this->_loadedSounds, this->_loadedWarheads, this->_loadedWeapons);
		for (auto &u: this->_objects)
			this->_base.loadObjects(u, *this, this->_loadedAnimations, this->_loadedSounds, this->_loadedWeapons, this->_loadedObjects);
		for (auto &u: this->_upgrades)
			this->_base.loadUpgrades(u, *this, this->_loadedUpgrades);
	} CATCH_AND_RETHROW("While loading faction \"" + this->_name + "\"\n")
}

const std::unordered_map<std::string, GameObject> &Faction::getObjects() const
{
	return this->_loadedObjects;
}

const std::unordered_map<std::string, MultiSound> &Faction::getSounds() const
{
	return this->_loadedSounds;
}

const std::unordered_map<std::string, Upgrade> &Faction::getUpgrades() const
{
	return this->_loadedUpgrades;
}

const std::unordered_map<std::string, Effect> &Faction::getEffects() const
{
	return this->_loadedEffects;
}

const std::unordered_map<std::string, Weapon> &Faction::getWeapons() const
{
	return this->_loadedWeapons;
}

const std::unordered_map<std::string, Warhead> &Faction::getWarheads() const
{
	return this->_loadedWarheads;
}

const std::unordered_map<std::string, unsigned> &Faction::getStartingMoney() const
{
	return this->_startingMoney;
}

const std::vector<StartObject> &Faction::getStartingObjects() const
{
	return this->_startingObjects;
}

UI *Faction::createUI() const
{
	return new D2KUI(*this, this->_base.getBase(), this->_uiData);
}

const Mod &Faction::getBase() const
{
	return this->_base;
}

std::array<std::vector<Sprite>, 3> &Faction::getPlacementCursors() const
{
	return this->_placementCursors;
}

bool Faction::canSell() const
{
	return this->_canSell;
}

bool Faction::canRepair() const
{
	return this->_canRepair;
}

bool Faction::regressesTech() const
{
	return this->_regressTech;
}

bool Faction::hasRadar() const
{
	return this->_hasRadar;
}

bool Faction::isRadarPowered() const
{
	return this->_radarPowered;
}

const std::string &Faction::getName() const
{
	return this->_name;
}

const std::optional<nlohmann::json> &Faction::getRepairAnim() const
{
	return this->_repairAnim;
}

const std::optional<std::string> &Faction::getPrimaryQueue() const
{
	return this->_primaryQueue;
}

