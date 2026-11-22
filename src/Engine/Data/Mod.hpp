//
// Created by PinkySmile on 28/03/2026.
//

#ifndef MIXRTS_MOD_HPP
#define MIXRTS_MOD_HPP


#include <map>
#include <set>
#include <string>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "CursorAnimation.hpp"
#include "Faction.hpp"

class MYDLL_API Mod {
private:
	std::string _name;
	std::filesystem::path _base;
	std::vector<MapInfo> _maps;
	std::vector<std::string> _factions;

public:
	Mod(std::filesystem::path base, const nlohmann::json &metadata);
	void loadFactions(const std::string &path, std::unordered_map<std::string, Faction> &factions) const;
	void loadCursors(const std::string &path, std::unordered_map<std::string, CursorAnimation> &cursors) const;
	void loadSounds(const std::string &path, std::unordered_map<std::string, MultiSound> &sounds, std::set<std::string> *checksumFiles = nullptr) const;
	void loadAnimations(const std::string &path, std::unordered_map<std::string, std::vector<Animation>> &anims, std::set<std::string> *checksumFiles = nullptr) const;
	void loadUpgrades(const std::string &path, const Faction &base, std::unordered_map<std::string, Upgrade> &upgrades, std::set<std::string> *checksumFiles = nullptr) const;
	void loadEffects(
		const std::string &path,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		std::unordered_map<std::string, Effect> &effects,
		std::set<std::string> *checksumFiles = nullptr
	) const;
	void loadWarheads(const std::string &path, std::unordered_map<std::string, Warhead> &warheads, std::set<std::string> *checksumFiles = nullptr) const;
	//! @brief Loads the armor multiplier tables of every mod whose factions are being loaded (armors/<origin>.json) into warheads.
	//! @param allFactions The full list of every faction id being loaded in this session, used to determine which armor files are needed.
	//! @param warheads The already loaded warheads (from loadWarheads) that the armor multipliers will be merged into.
	void loadArmors(const std::vector<std::string> &allFactions, std::unordered_map<std::string, Warhead> &warheads, std::set<std::string> *checksumFiles = nullptr) const;
	void loadWeapons(
		const std::string &path,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Warhead> &warheads,
		std::unordered_map<std::string, Weapon> &weapons,
		std::set<std::string> *checksumFiles = nullptr
	) const;
	void loadObjects(
		const std::string &path,
		const Faction &base,
		const ObjectFactory &factory,
		const std::unordered_map<std::string, std::vector<Animation>> &anims,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, Weapon> &weapons,
		std::unordered_map<std::string, std::shared_ptr<GameObject>> &objects,
		std::set<std::string> *checksumFiles = nullptr
	) const;
	[[nodiscard]] const std::vector<std::string> &getFactions() const;
	[[nodiscard]] const std::vector<MapInfo> &getMaps() const;
	[[nodiscard]] const std::filesystem::path &getBase() const;
	[[nodiscard]] const std::string &getName() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_MOD_HPP
