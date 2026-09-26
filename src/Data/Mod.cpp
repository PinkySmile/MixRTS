//
// Created by PinkySmile on 28/03/2026.
//

#include <map>
#include <set>
#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <cpptrace/from_current.hpp>
#include "Resources/Assert.hpp"
#include "Animation.hpp"
#include "CursorAnimation.hpp"
#include "Upgrade.hpp"
#include "Effect.hpp"
#include "Warhead.hpp"
#include "Weapon.hpp"
#include "Mod.hpp"
#include "Utils.hpp"
#include "GameObject.hpp"

Mod::Mod(std::filesystem::path base, const nlohmann::json &metadata) :
	_name(metadata["name"]),
	_base(std::move(base))
{
}

void Mod::loadFactions(const std::filesystem::path &path, std::unordered_map<std::string, Faction> &factions) const
{
	TRY {
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

			TRY {
				std::string rid = RESOURCE(this->_name, id);

				assert(!factions.contains(rid));
				factions.emplace(rid, Faction{*this, rid, mergeObjects(jfaction, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading factions file \"" + path.string() + "\"\n")
}

void Mod::loadCursors(const std::filesystem::path &path, std::unordered_map<std::string, CursorAnimation> &cursors) const
{
	TRY {
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

			TRY {
				assert(!cursors.contains(id));
				cursors.emplace(id, CursorAnimation{this->_base, jcursorArr, gen});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading cursors file \"" + path.string() + "\"\n")
}

void Mod::loadSounds(const std::filesystem::path &path, std::unordered_map<std::string, MultiSound> &sounds) const
{
	TRY {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jsound]: json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(this->_name, id);
				assert_exp(!sounds.contains(fullId));
				assert_exp(jsound.is_object() || jsound.is_array());

				if (jsound.is_object())
					mergeObjects(jsound, gen);
				else for (auto &s : jsound)
					mergeObjects(s, gen);
				sounds[fullId].init(this->_base, jsound);
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}

	} CATCH_AND_RETHROW("While loading sounds file \"" + path.string() + "\"\n")
}

void Mod::loadAnimations(const std::filesystem::path &path, std::unordered_map<std::string, std::vector<Animation>> &anims) const
{
	TRY {
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

			TRY {
				std::string fullId = RESOURCE(this->_name, id);
				assert_exp(!anims.contains(fullId));
				auto &v = anims[fullId];

				assert_exp(janimArr.is_array() || janimArr.is_object());
				if (janimArr.is_array()) {
					v.reserve(janimArr.size());
					for (auto &janim: janimArr) {
						auto animJson = mergeObjects(janim, gen);
						size_t count = 1;

						if (animJson.contains("repeat")) {
							assert_exp(animJson["repeat"].is_number_unsigned());
							count = animJson["repeat"];
							assert_exp(count != 0);
						}
						while (count--)
							v.emplace_back(this->_base, animJson);
					}
				} else {
					assert_exp(janimArr.contains("copy") && janimArr["copy"].is_string());
					assert_exp(janimArr.contains("inverted") && janimArr["inverted"].is_boolean());
					assert_exp(!janimArr.contains("count") || janimArr["count"].is_number_unsigned());

					std::string copy = RESOURCE(this->_name, janimArr["copy"]);
					assert_exp(anims.contains(copy));
					auto &c = anims[copy];
					size_t count = janimArr.contains("count") ? janimArr["count"].get<size_t>() : c.size();

					v.reserve(count);
					if (janimArr["inverted"])
						for (size_t i = 0; i < count; i++)
							v.emplace_back(c[c.size() - i - 1]);
					else
						for (size_t i = 0; i < count; i++)
							v.emplace_back(c[i]);
				}
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading animations file \"" + path.string() + "\"\n")
}

void Mod::loadObjects(
	const std::filesystem::path &path,
	const Faction &base,
	const std::unordered_map<std::string, std::vector<Animation>> &anims,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	std::unordered_map<std::string, GameObject> &objects
) const
{
	TRY {
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

			TRY {
				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!objects.contains(fullId));
				objects.emplace(fullId, GameObject{base, fullId, anims, sounds, effects, weapons, mergeObjects(jbuilding, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading objects file \"" + path.string() + "\"\n")
}

void Mod::loadUpgrades(const std::filesystem::path &path, const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades) const
{
	TRY {
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

			TRY {
				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!upgrades.contains(fullId));
				upgrades.emplace(fullId, Upgrade{base, fullId, mergeObjects(jupgrade, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading upgrades file \"" + path.string() + "\"\n")
}

void Mod::loadEffects(const std::filesystem::path &path, const std::unordered_map<std::string, std::vector<Animation>> &anims, const std::unordered_map<std::string, MultiSound> &sounds, std::unordered_map<std::string, Effect> &effects) const
{
	TRY {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jeffect] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!effects.contains(fullId));
				effects.emplace(fullId, Effect{*this, anims, sounds, mergeObjects(jeffect, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading effects file \"" + path.string() + "\"\n")
}

void Mod::loadWeapons(const std::filesystem::path &path, const std::unordered_map<std::string, std::vector<Animation>> &anims, const std::unordered_map<std::string, Effect> &effects, const std::unordered_map<std::string, MultiSound> &sounds, const std::unordered_map<std::string, Warhead> &warheads, std::unordered_map<std::string, Weapon> &weapons) const
{
	TRY {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jweapon] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!weapons.contains(fullId));
				weapons.emplace(fullId, Weapon{*this, anims, effects, sounds, warheads, mergeObjects(jweapon, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading weapons file \"" + path.string() + "\"\n")
}

void Mod::loadWarheads(const std::filesystem::path &path, std::unordered_map<std::string, Warhead> &warheads) const
{
	TRY {
		std::ifstream stream{this->_base / path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jwarhead] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(this->_name, id);

				assert_exp(!warheads.contains(fullId));
				warheads.emplace(fullId, Warhead{mergeObjects(jwarhead, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading warheads file \"" + path.string() + "\"\n")
}

void Mod::loadArmors(const std::vector<std::string> &allFactions, std::unordered_map<std::string, Warhead> &warheads) const
{
	std::set<std::string> origins;

	for (auto &faction : allFactions) {
		auto pos = faction.find(':');

		origins.insert(pos == std::string::npos ? faction : faction.substr(0, pos));
	}
	for (auto &origin : origins) {
		std::filesystem::path relPath = std::filesystem::path("armors") / (origin + ".json");
		std::filesystem::path fullPath = this->_base / relPath;

		TRY {
			std::ifstream stream{fullPath};
			nlohmann::json json;

			assert_msg(std::filesystem::exists(fullPath), "Missing armor file " + relPath.string());
			assert_msg(stream, "Failed to open " + relPath.string());
			stream >> json;
			for (auto &[id, jarmor] : json.items()) {
				if (id.starts_with('_'))
					continue;

				TRY {
					std::string fullId = RESOURCE(this->_name, id);

					assert_msg(warheads.contains(fullId), fullId);
					warheads.at(fullId).loadArmor(origin, jarmor);
				} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
			}
		} CATCH_AND_RETHROW("While loading armor file \"" + relPath.string() + "\"\n")
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
