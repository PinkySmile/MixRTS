//
// Created by PinkySmile on 28/03/2026.
//

#include <map>
#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Resources/Assert.hpp"
#include "Animation.hpp"
#include "CursorAnimation.hpp"
#include "Upgrade.hpp"
#include "Mod.hpp"
#include "Utils.hpp"

Mod::Mod(std::filesystem::path base, const nlohmann::json &metadata) :
	_name(metadata["name"]),
	_base(std::move(base))
{
}

void Mod::loadFactions(const std::filesystem::path &path, std::unordered_map<std::string, Faction> &factions) const
{
	try {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jfaction] : json.items()) {
			if (id.starts_with('_'))
				continue;

			std::string rid = RESOURCE(this->_name, id);

			assert(!factions.contains(rid));
			factions.emplace(rid, Faction{*this, rid, mergeObjects(jfaction, gen)});
		}
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading factions file \"" + path.string() + "\"\n" + e.what());
	}
}

void Mod::loadCursors(const std::filesystem::path &path, std::unordered_map<std::string, CursorAnimation> &cursors) const
{
	try {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jcursorArr] : json.items()) {
			if (id.starts_with('_'))
				continue;

			assert(!cursors.contains(id));
			cursors.emplace(id, CursorAnimation{this->_base, jcursorArr, gen});
		}
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading cursors file \"" + path.string() + "\"\n" + e.what());
	}
}

void Mod::loadAnimations(const std::filesystem::path &path, std::unordered_map<std::string, std::vector<Animation>> &anims) const
{
	try {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, janimArr]: json.items()) {
			if (id.starts_with('_'))
				continue;

			std::string fullId = RESOURCE(this->_name, id);
			assert_exp(!anims.contains(fullId));
			auto &v = anims[fullId];

			v.reserve(janimArr.size());
			for (auto &janim: janimArr)
				v.emplace_back(this->_base, mergeObjects(janim, gen));
		}
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading animation file \"" + path.string() + "\"\n" + e.what());
	}
}

void Mod::loadUpgrades(const std::filesystem::path &path, const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades) const
{
	try {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jupgrade] : json.items()) {
			if (id.starts_with('_'))
				continue;

			std::string fullId = RESOURCE(this->_name, id);

			assert_exp(!upgrades.contains(fullId));
			upgrades.emplace(fullId, Upgrade{base, fullId, mergeObjects(jupgrade, gen)});
		}
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading upgrade file \"" + path.string() + "\"\n" + e.what());
	}
}

void Mod::loadBuildings(const std::filesystem::path &path, const Faction &base, const std::unordered_map<std::string, std::vector<Animation>> &anims, std::unordered_map<std::string, Building> &buildings) const
{
	try {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jbuilding] : json.items()) {
			if (id.starts_with('_'))
				continue;

			std::string fullId = RESOURCE(this->_name, id);

			assert_exp(!buildings.contains(fullId));
			buildings.emplace(fullId, Building{base, fullId, anims, mergeObjects(jbuilding, gen)});
		}
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading building file \"" + path.string() + "\"\n" + e.what());
	}
}

const std::filesystem::path &Mod::getBase() const
{
	return this->_base;
}

const std::string &Mod::getName() const
{
	return this->_name;
}
