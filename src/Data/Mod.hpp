//
// Created by PinkySmile on 28/03/2026.
//

#ifndef MIXRTS_MOD_HPP
#define MIXRTS_MOD_HPP


#include <map>
#include <string>
#include "Building.hpp"
#include "CursorAnimation.hpp"
#include "Faction.hpp"

class Mod {
private:
	std::string _name;
	std::filesystem::path _base;

public:
	Mod(std::filesystem::path base, const nlohmann::json &metadata);
	void loadFactions(
		const std::filesystem::path &path,
		std::unordered_map<std::string, Faction> &factions
	) const;
	void loadCursors(
		const std::filesystem::path &path,
		std::unordered_map<std::string, CursorAnimation> &cursors
	) const;
	void loadSounds(
		const std::filesystem::path &path,
		std::unordered_map<std::string, MultiSound> &sounds
	) const;
	void loadAnimations(
		const std::filesystem::path &path,
		std::unordered_map<std::string, std::vector<Animation>> &anims
	) const;
	void loadUpgrades(
		const std::filesystem::path &path,
		const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades
	) const;
	void loadUnits(
		const std::filesystem::path &path,
		const Faction &base,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		std::unordered_map<std::string, Unit> &units
	) const;
	void loadBuildings(
		const std::filesystem::path &path,
		const Faction &base,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		std::unordered_map<std::string, Building> &buildings
	) const;
	const std::filesystem::path &getBase() const;
	const std::string &getName() const;
};


#endif //MIXRTS_MOD_HPP
