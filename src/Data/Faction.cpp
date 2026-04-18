//
// Created by PinkySmile on 29/03/2026.
//

#include "Resources/Assert.hpp"
#include "Faction.hpp"
#include "Mod.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "D2K/D2KUI.hpp"

Faction::Faction(const Mod &base, const nlohmann::json &data) :
	_base(base),
	_canSell(data["can_sell"]),
	_canRepair(data["can_repair"]),
	_buildings(data["buildings"]),
	_animations(data["animations"]),
	_upgrades(data["upgrades"]),
	_startingMoney(data["starting_money"]),
	_uiData(data["ui"]),
	_regressTech(data["regress_tech"])
{
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
		StartBuilding b;

		b.name  = RESOURCE(base.getName(), p["name"]);
		b.queue = RESOURCE(base.getName(), p["queue"]);
		if (p.contains("offset")) {
			b.offset.x = p["offset"]["x"];
			b.offset.y = p["offset"]["y"];
		}
		this->_startingBuildings.push_back(b);
	}
}

void Faction::load()
{
	for (auto &a : this->_animations)
		this->_base.loadAnimations(a, this->_loadedAnimations);
	for (auto &b : this->_buildings)
		this->_base.loadBuildings(b, *this, this->_loadedAnimations, this->_loadedBuildings);
	for (auto &u : this->_upgrades)
		this->_base.loadUpgrades(u, *this, this->_loadedUpgrades);
}

const std::unordered_map<std::string, Building> &Faction::getBuildings() const
{
	return this->_loadedBuildings;
}

const std::unordered_map<std::string, Upgrade> &Faction::getUpgrades() const
{
	return this->_loadedUpgrades;
}

const std::unordered_map<std::string, unsigned> &Faction::getStartingMoney() const
{
	return this->_startingMoney;
}

const std::vector<StartBuilding> &Faction::getStartingBuildings() const
{
	return this->_startingBuildings;
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

