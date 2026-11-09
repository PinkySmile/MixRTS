//
// Created by PinkySmile on 28/03/2026.
//

#include <fstream>
#include <nlohmann/json.hpp>
#include "ModManager.hpp"

void ModManager::loadMod(const std::filesystem::path &path)
{
	std::ifstream stream{path / "mod.json"};
	nlohmann::json json;

	stream >> json;
	stream.close();
	this->_mods.emplace(json["name"], Mod{path, json});
}

Mod &ModManager::get(const std::string &name)
{
	return this->_mods.at(name);
}
