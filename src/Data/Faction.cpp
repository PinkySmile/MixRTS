//
// Created by PinkySmile on 29/03/2026.
//

#include "Resources/Assert.hpp"
#include "Faction.hpp"
#include "Mod.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "D2K/D2KUI.hpp"

Faction::Faction(const Mod &base, const std::string &name, const nlohmann::json &data) :
	_base(base),
	_name(name)
{
	assert_exp(data.contains("can_sell"));
	assert_exp(data.contains("can_repair"));
	assert_exp(data.contains("buildings"));
	assert_exp(data.contains("animations"));
	assert_exp(data.contains("upgrades"));
	assert_exp(data.contains("units"));
	assert_exp(data.contains("sounds"));
	assert_exp(data.contains("starting_money"));
	assert_exp(data.contains("ui"));
	assert_exp(data.contains("regress_tech"));
	assert_exp(data.contains("starting_units"));
	assert_exp(data.contains("starting_building"));

	assert_exp(data["can_sell"].is_boolean());
	assert_exp(data["can_repair"].is_boolean());
	assert_exp(data["regress_tech"].is_boolean());
	assert_exp(data["sounds"].is_array());
	for (size_t i = 0; i < data["sounds"].size(); i++)
		assert_msg(data["sounds"][i].is_string(), "data[\"sounds\"][" + std::to_string(i) + "]");
	assert_exp(data["units"].is_array());
	for (size_t i = 0; i < data["units"].size(); i++)
		assert_msg(data["units"][i].is_string(), "data[\"units\"][" + std::to_string(i) + "]");
	assert_exp(data["buildings"].is_array());
	for (size_t i = 0; i < data["buildings"].size(); i++)
		assert_msg(data["buildings"][i].is_string(), "data[\"buildings\"][" + std::to_string(i) + "]");
	assert_exp(data["animations"].is_array());
	for (size_t i = 0; i < data["animations"].size(); i++)
		assert_msg(data["animations"][i].is_string(), "data[\"animations\"][" + std::to_string(i) + "]");
	assert_exp(data["upgrades"].is_array());
	for (size_t i = 0; i < data["upgrades"].size(); i++)
		assert_msg(data["upgrades"][i].is_string(), "data[\"upgrades\"][" + std::to_string(i) + "]");
	assert_exp(data["starting_money"].is_object());
	for (auto &[key, value] : data["starting_money"].items())
		assert_msg(value.is_number(), "data[\"starting_money\"][\"" + key + "\"]");
	assert_exp(data["place"].is_object());
	assert_exp(data["place"]["best"].is_array());
	for (size_t i = 0; i < data["place"]["best"].size(); i++)
		assert_msg(data["place"]["best"][i].is_string(), "data[\"place\"][\"best\"][" + std::to_string(i) + "]");
	assert_exp(data["place"]["ok"].is_array());
	for (size_t i = 0; i < data["place"]["ok"].size(); i++)
		assert_msg(data["place"]["ok"][i].is_string(), "data[\"place\"][\"ok\"][" + std::to_string(i) + "]");
	assert_exp(data["place"]["bad"].is_array());
	for (size_t i = 0; i < data["place"]["bad"].size(); i++)
		assert_msg(data["place"]["bad"][i].is_string(), "data[\"place\"][\"bad\"][" + std::to_string(i) + "]");
	assert_exp(data["starting_building"].is_array());
	for (size_t i = 0; i < data["starting_building"].size(); i++) {
		assert_msg(data["starting_building"][i].is_object(), "data[\"starting_building\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_building"][i].contains("name"), "data[\"starting_building\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_building"][i].contains("queue"), "data[\"starting_building\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_building"][i]["name"].is_string(), "data[\"starting_building\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_building"][i]["queue"].is_string(), "data[\"starting_building\"][" + std::to_string(i) + "]");
		assert_msg(!data["starting_building"][i].contains("offset") || data["starting_building"][i]["offset"].is_object(), "data[\"starting_building\"][" + std::to_string(i) + "]");
		if (data["starting_building"][i].contains("offset")) {
			assert_msg(data["starting_building"][i]["offset"].contains("x"), "data[\"starting_building\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_building"][i]["offset"].contains("y"), "data[\"starting_building\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_building"][i]["offset"]["x"].is_number(), "data[\"starting_building\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_building"][i]["offset"]["y"].is_number(), "data[\"starting_building\"][" + std::to_string(i) + "]");
		}
	}
	assert_exp(data["starting_units"].is_array());
	for (size_t i = 0; i < data["starting_units"].size(); i++) {
		assert_msg(data["starting_units"][i].is_object(), "data[\"starting_units\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_units"][i].contains("name"), "data[\"starting_units\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_units"][i].contains("queue"), "data[\"starting_units\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_units"][i]["name"].is_string(), "data[\"starting_units\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_units"][i]["queue"].is_string(), "data[\"starting_units\"][" + std::to_string(i) + "]");
		assert_msg(!data["starting_units"][i].contains("offset") || data["starting_units"][i]["offset"].is_object(), "data[\"starting_units\"][" + std::to_string(i) + "]");
		if (data["starting_units"][i].contains("offset")) {
			assert_msg(data["starting_units"][i]["offset"].contains("x"), "data[\"starting_units\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_units"][i]["offset"].contains("y"), "data[\"starting_units\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_units"][i]["offset"]["x"].is_number(), "data[\"starting_units\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_units"][i]["offset"]["y"].is_number(), "data[\"starting_units\"][" + std::to_string(i) + "]");
		}
	}

	this->_canSell = data["can_sell"];
	this->_canRepair = data["can_repair"];
	this->_buildings = data["buildings"];
	this->_animations = data["animations"];
	this->_upgrades = data["upgrades"];
	this->_units = data["units"];
	this->_sounds = data["sounds"];
	this->_startingMoney = data["starting_money"];
	this->_uiData = data["ui"];
	this->_regressTech = data["regress_tech"];

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
	for (const auto &p : data["starting_building"]) {
		StartObject b;

		b.name  = RESOURCE(base.getName(), p["name"]);
		b.queue = RESOURCE(base.getName(), p["queue"]);
		if (p.contains("offset")) {
			b.offset.x = p["offset"]["x"];
			b.offset.y = p["offset"]["y"];
		}
		this->_startingBuildings.push_back(b);
	}
	for (const auto &p : data["starting_units"]) {
		StartObject b;

		b.name  = RESOURCE(base.getName(), p["name"]);
		b.queue = RESOURCE(base.getName(), p["queue"]);
		if (p.contains("offset")) {
			b.offset.x = p["offset"]["x"];
			b.offset.y = p["offset"]["y"];
		}
		this->_startingUnits.push_back(b);
	}
}

void Faction::load()
{
#ifndef PROPAGATE_CRASH
	try {
#endif
		for (auto &a: this->_animations)
			this->_base.loadAnimations(a, this->_loadedAnimations);
		for (auto &a: this->_sounds)
			this->_base.loadSounds(a, this->_loadedSounds);
		for (auto &u: this->_units)
			this->_base.loadUnits(u, *this, this->_loadedAnimations, this->_loadedSounds, this->_loadedUnits);
		for (auto &b: this->_buildings)
			this->_base.loadBuildings(b, *this, this->_loadedAnimations, this->_loadedSounds, this->_loadedBuildings);
		for (auto &u: this->_upgrades)
			this->_base.loadUpgrades(u, *this, this->_loadedUpgrades);
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading faction \"" + this->_name + "\"\n" + e.what());
	}
#endif
}

const std::unordered_map<std::string, Building> &Faction::getBuildings() const
{
	return this->_loadedBuildings;
}

const std::unordered_map<std::string, Unit> &Faction::getUnits() const
{
	return this->_loadedUnits;
}

const std::unordered_map<std::string, MultiSound> &Faction::getSounds() const
{
	return this->_loadedSounds;
}

const std::unordered_map<std::string, Upgrade> &Faction::getUpgrades() const
{
	return this->_loadedUpgrades;
}

const std::unordered_map<std::string, unsigned> &Faction::getStartingMoney() const
{
	return this->_startingMoney;
}

const std::vector<StartObject> &Faction::getStartingBuildings() const
{
	return this->_startingBuildings;
}

const std::vector<StartObject> &Faction::getStartingUnits() const
{
	return this->_startingUnits;
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

bool Faction::regressTech() const
{
	return this->_regressTech;
}

