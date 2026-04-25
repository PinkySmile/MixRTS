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
#ifndef PROPAGATE_CRASH
	try {
#endif
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
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string rid = RESOURCE(this->_name, id);

				assert(!factions.contains(rid));
				factions.emplace(rid, Faction{*this, rid, mergeObjects(jfaction, gen)});
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading factions file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadCursors(const std::filesystem::path &path, std::unordered_map<std::string, CursorAnimation> &cursors) const
{
#ifndef PROPAGATE_CRASH
	try {
#endif
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
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				assert(!cursors.contains(id));
				cursors.emplace(id, CursorAnimation{this->_base, jcursorArr, gen});
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading cursors file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadSounds(const std::filesystem::path &path, std::unordered_map<std::string, MultiSound> &sounds) const
{

#ifndef PROPAGATE_CRASH
	try {
#endif
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		for (auto &[id, jsound]: json.items()) {

		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string fullId = RESOURCE(this->_name, id);
				assert_exp(!sounds.contains(fullId));

				sounds[fullId].init(this->_base, jsound);

		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}

#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading sound file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadAnimations(const std::filesystem::path &path, std::unordered_map<std::string, std::vector<Animation>> &anims) const
{
#ifndef PROPAGATE_CRASH
	try {
#endif
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
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string fullId = RESOURCE(this->_name, id);
				assert_exp(!anims.contains(fullId));
				auto &v = anims[fullId];

				v.reserve(janimArr.size());
				for (auto &janim: janimArr)
					v.emplace_back(this->_base, mergeObjects(janim, gen));
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading animation file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadUnits(
	const std::filesystem::path &path,
	const Faction &base,
	const std::unordered_map<std::string, std::vector<Animation>> &anims,
	const std::unordered_map<std::string, MultiSound> &sounds,
	std::unordered_map<std::string, Unit> &units
) const
{
#ifndef PROPAGATE_CRASH
	try {
#endif
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, junit] : json.items()) {
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!units.contains(fullId));
				units.emplace(fullId, Unit{base, fullId, anims, sounds, mergeObjects(junit, gen)});
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading units file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadUpgrades(const std::filesystem::path &path, const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades) const
{
#ifndef PROPAGATE_CRASH
	try {
#endif
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
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!upgrades.contains(fullId));
				upgrades.emplace(fullId, Upgrade{base, fullId, mergeObjects(jupgrade, gen)});
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading upgrade file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

void Mod::loadBuildings(
	const std::filesystem::path &path,
	const Faction &base,
	const std::unordered_map<std::string, std::vector<Animation>> &anims,
	const std::unordered_map<std::string, MultiSound> &sounds,
	std::unordered_map<std::string, Building> &buildings
) const
{
#ifndef PROPAGATE_CRASH
	try {
#endif
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
		#ifndef PROPAGATE_CRASH
			try {
		#endif
				if (id.starts_with('_'))
					continue;

				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!buildings.contains(fullId));
				buildings.emplace(fullId, Building{base, fullId, anims, sounds, mergeObjects(jbuilding, gen)});
		#ifndef PROPAGATE_CRASH
			} catch (std::exception &e) {
				throw _AssertionFailedException("In key \"" + id + "\"\n" + e.what());
			}
		#endif
		}
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading building file \"" + path.string() + "\"\n" + e.what());
	}
#endif
}

const std::filesystem::path &Mod::getBase() const
{
	return this->_base;
}

const std::string &Mod::getName() const
{
	return this->_name;
}
