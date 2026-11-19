//
// Created by PinkySmile on 28/03/2026.
//

#include <fstream>
#include <nlohmann/json.hpp>
#include "ModManager.hpp"

#include "Resources/Assert.hpp"

void ModManager::reloadAll(const std::filesystem::path &path)
{
	this->_mods.clear();
	this->_factions.clear();
	this->loadAll(path);
}

void ModManager::loadAll(const std::filesystem::path &path)
{
	for (const auto &entry : std::filesystem::directory_iterator(path))
		if (entry.is_directory() && std::filesystem::exists(entry.path() / "mod.json"))
			this->loadMod(entry.path());
}

void ModManager::loadMod(const std::filesystem::path &path)
{
	std::ifstream stream{path / "mod.json"};
	nlohmann::json json;

	stream >> json;
	stream.close();
	this->_mods.emplace(json["name"], Mod{path, json});

	Mod &mod = this->_mods.at(json["name"]);

	for (const std::string &faction : mod.getFactions())
		mod.loadFactions(faction, this->_factions);
}

Mod &ModManager::get(const std::string &name)
{
	auto it = this->_mods.find(name);

	assert_exp(it != this->_mods.end());
	return it->second;
}

const Faction &ModManager::getFaction(const std::string &name)
{
	auto it = this->_factions.find(name);

	assert_exp(it != this->_factions.end());
	return it->second;
}

std::pair<Mod *, std::filesystem::path> ModManager::getPath(const std::string &name)
{
	size_t pos = name.find(':');
	assert_msg(pos != std::string::npos, name);
	std::string relPath = name.substr(pos + 1);
	std::string modName = name.substr(0, pos);
	Mod &mod = this->get(modName);

	return { &mod, mod.getBase() / relPath };
}

std::vector<std::string> ModManager::getFactionList() const
{
	std::vector<std::string> f;

	for (auto &key : this->_factions | std::ranges::views::keys)
		f.push_back(key);
	std::ranges::sort(f);
	return f;
}
