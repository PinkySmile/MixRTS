//
// Created by PinkySmile on 28/03/2026.
//

#ifndef MIXRTS_MOD_HPP
#define MIXRTS_MOD_HPP


#include <map>
#include <string>
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
	void loadEffects(
		const std::filesystem::path &path,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		std::unordered_map<std::string, Effect> &effects
	) const;
	void loadWarheads(
		const std::filesystem::path &path,
		std::unordered_map<std::string, Warhead> &warheads
	) const;
	//! @brief Loads the armor multiplier tables of every mod whose factions are being loaded (armors/<origin>.json) into warheads.
	//! @param allFactions The full list of every faction id being loaded in this session, used to determine which armor files are needed.
	//! @param warheads The already loaded warheads (from loadWarheads) that the armor multipliers will be merged into.
	void loadArmors(
		const std::vector<std::string> &allFactions,
		std::unordered_map<std::string, Warhead> &warheads
	) const;
	void loadWeapons(
		const std::filesystem::path &path,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Warhead> &warheads,
		std::unordered_map<std::string, Weapon> &weapons
	) const;
	void loadObjects(
		const std::filesystem::path &path,
		const Faction &base,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, Weapon> &weapons,
		std::unordered_map<std::string, GameObject> &objects
	) const;
	const std::filesystem::path &getBase() const;
	const std::string &getName() const;
};


#endif //MIXRTS_MOD_HPP
