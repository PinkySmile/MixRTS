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
#include <sol/sol.hpp>
#include "Resources/Assert.hpp"
#include "Animation.hpp"
#include "CursorAnimation.hpp"
#include "Upgrade.hpp"
#include "Effect.hpp"
#include "Warhead.hpp"
#include "Weapon.hpp"
#include "Mod.hpp"
#include "Utils.hpp"
#include "Objects/GameObject.hpp"
#include "Resources/Game.hpp"

Mod::Mod(std::filesystem::path base, const nlohmann::json &metadata) :
	_name(metadata["name"]),
	_base(std::move(base))
{
	std::string folder = metadata["maps_folder"];

	for (auto &entry : std::filesystem::directory_iterator(this->_base / folder))
		if (std::filesystem::exists(entry.path() / "map.json")) {
			this->_maps.emplace_back();
			this->_maps.back().load(*this, entry.path());
			this->_maps.back().path = this->_name + ":" + std::filesystem::relative(entry.path(), this->_base).string();
		}
	this->_factions = metadata["factions"];
}

void Mod::loadAnimation(const std::filesystem::path &path, const std::string &key, std::vector<Animation> &anims) const
{
	TRY {
		std::ifstream stream{path};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		assert_msg(json.contains(key), key);
		TRY {
			auto &janimArr = json[key];
			assert_exp(janimArr.is_array());
			anims.reserve(janimArr.size());
			for (auto &janim: janimArr) {
				auto animJson = mergeObjects(janim, gen);
				size_t count = 1;

				if (animJson.contains("repeat")) {
					assert_exp(animJson["repeat"].is_number_unsigned());
					count = animJson["repeat"];
					assert_exp(count != 0);
				}
				while (count--)
					anims.emplace_back(*this, animJson);
			}
		} CATCH_AND_RETHROW("In key \"" + key + "\"\n")
	} CATCH_AND_RETHROW("While loading animations file \"" + path.string() + "\"\n")
}

static void rememberFile(std::set<std::string> *files, const Mod &mod, const std::filesystem::path &fullPath)
{
	if (!files)
		return;

	std::error_code err;
	std::filesystem::path rel = std::filesystem::relative(fullPath, mod.getBase(), err);

	if (err)
		rel = fullPath.filename();
	files->insert(mod.getName() + ":" + rel.generic_string());
}

void Mod::loadFactions(const std::string &relPath, std::unordered_map<std::string, Faction> &factions) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jfaction] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string rid = RESOURCE(path.first->_name, id);

				assert(!factions.contains(rid));
				factions.emplace(rid, Faction{*path.first, rid, mergeObjects(jfaction, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading factions file \"" + path.second.string() + "\"\n")
}

void Mod::loadCursors(const std::string &relPath, std::unordered_map<std::string, CursorAnimation> &cursors) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
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
				cursors.emplace(id, CursorAnimation{*path.first, jcursorArr, gen});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading cursors file \"" + path.second.string() + "\"\n")
}

void Mod::loadSounds(const std::string &relPath, std::unordered_map<std::string, MultiSound> &sounds, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jsound]: json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);
				assert_exp(!sounds.contains(fullId));
				assert_exp(jsound.is_object() || jsound.is_array());

				if (jsound.is_object())
					mergeObjects(jsound, gen);
				else for (auto &s : jsound)
					mergeObjects(s, gen);
				sounds[fullId].init(*path.first, jsound);
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}

	} CATCH_AND_RETHROW("While loading sounds file \"" + path.second.string() + "\"\n")
}

void Mod::loadAnimations(const std::string &relPath, std::unordered_map<std::string, std::vector<Animation>> &anims, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, janimArr]: json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);
				assert_exp(!anims.contains(fullId));
				std::vector<Animation> &v = anims[fullId];

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
							v.emplace_back(*path.first, animJson);
					}
				} else {
					assert_exp(janimArr.contains("copy") && janimArr["copy"].is_string());
					assert_exp(janimArr.contains("inverted") && janimArr["inverted"].is_boolean());
					assert_exp(!janimArr.contains("count") || janimArr["count"].is_number_unsigned());

					std::string copy = RESOURCE(path.first->_name, janimArr["copy"]);
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
	} CATCH_AND_RETHROW("While loading animations file \"" + path.second.string() + "\"\n")
}

void Mod::loadObjects(
	const std::string &relPath,
	const Faction &base,
	const ObjectFactory &factory,
	const std::unordered_map<std::string, std::vector<Animation>> &anims,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	std::unordered_map<std::string, std::shared_ptr<GameObject>> &objects,
	std::set<std::string> *checksumFiles
) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jbuilding] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);

				assert_exp(!objects.contains(fullId));
				objects.emplace(fullId, std::shared_ptr<GameObject>{factory.create(base, fullId, anims, sounds, effects, weapons, mergeObjects(jbuilding, gen))});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading objects file \"" + path.second.string() + "\"\n")
}

const std::vector<std::string> &Mod::getFactions() const
{
	return this->_factions;
}

const std::vector<MapInfo> &Mod::getMaps() const
{
	return this->_maps;
}

void Mod::loadUpgrades(const std::string &relPath, const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jupgrade] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);

				assert_exp(!upgrades.contains(fullId));
				upgrades.emplace(fullId, Upgrade{base, fullId, mergeObjects(jupgrade, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading upgrades file \"" + path.second.string() + "\"\n")
}

void Mod::loadEffects(const std::string &relPath, const std::unordered_map<std::string, std::vector<Animation>> &anims, const std::unordered_map<std::string, MultiSound> &sounds, std::unordered_map<std::string, Effect> &effects, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jeffect] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);

				assert_exp(!effects.contains(fullId));
				effects.emplace(fullId, Effect{*path.first, anims, sounds, mergeObjects(jeffect, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading effects file \"" + path.second.string() + "\"\n")
}

void Mod::loadWeapons(const std::string &relPath, const std::unordered_map<std::string, std::vector<Animation>> &anims, const std::unordered_map<std::string, Effect> &effects, const std::unordered_map<std::string, MultiSound> &sounds, const std::unordered_map<std::string, Warhead> &warheads, std::unordered_map<std::string, Weapon> &weapons, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jweapon] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);

				assert_exp(!weapons.contains(fullId));
				weapons.emplace(fullId, Weapon{*path.first, anims, effects, sounds, warheads, mergeObjects(jweapon, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading weapons file \"" + path.second.string() + "\"\n")
}

void Mod::loadWarheads(const std::string &relPath, std::unordered_map<std::string, Warhead> &warheads, std::set<std::string> *checksumFiles) const
{
	std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(this->_name, relPath));

	TRY {
		rememberFile(checksumFiles, *path.first, path.second);
		std::ifstream stream{path.second};
		nlohmann::json json;
		nlohmann::json gen;

		assert_msg(stream, "Failed to open " + path.second.string());
		stream >> json;
		if (json.contains("_general"))
			gen = json["_general"];
		else
			gen = nlohmann::json::object();
		for (auto &[id, jwarhead] : json.items()) {
			if (id.starts_with('_'))
				continue;

			TRY {
				std::string fullId = RESOURCE(path.first->_name, id);

				assert_exp(!warheads.contains(fullId));
				warheads.emplace(fullId, Warhead{mergeObjects(jwarhead, gen)});
			} CATCH_AND_RETHROW("In key \"" + id + "\"\n")
		}
	} CATCH_AND_RETHROW("While loading warheads file \"" + path.second.string() + "\"\n")
}

void Mod::loadArmors(const std::vector<std::string> &allFactions, std::unordered_map<std::string, Warhead> &warheads, std::set<std::string> *checksumFiles) const
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
			rememberFile(checksumFiles, *this, fullPath);
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

void Mod::pushUserType(sol::state &state)
{
	sol::usertype<Mod> type = state.new_usertype<Mod>("Mod");

	type["name"] = sol::property(&Mod::getName);
	// Exposed as a plain string rather than std::filesystem::path (which has
	// no Lua conversion registered); scripts should treat it as an opaque
	// base path to join with '/' using string concatenation.
	type["path"] = sol::property([](const Mod &mod) { return mod.getBase().string(); });
	// NOTE: Only loadCursors() is exposed here: it is the only Mod load*()
	// method ever called outside of the engine's own asset-loading pass (it
	// is called directly from UI constructors to load a UI-local cursor set,
	// e.g. D2KUI). The other load*() methods are internal mod-loading
	// machinery invoked once by the engine itself and are not meant to be
	// (re)called from scripts.
	type["load_cursors"] = [](const Mod &mod, const std::string &path, std::unordered_map<std::string, CursorAnimation> &cursors) {
		mod.loadCursors(path, cursors);
	};
}
