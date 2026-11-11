//
// Created by PinkySmile on 28/03/2026.
//

#ifndef MIXRTS_MODMANAGER_HPP
#define MIXRTS_MODMANAGER_HPP


#include "Mod.hpp"

class MYDLL_API ModManager {
private:
	std::map<std::string, Mod> _mods;
	std::unordered_map<std::string, Faction> _factions;

public:
	void loadAll(const std::filesystem::path &path);
	void loadMod(const std::filesystem::path &path);
	Mod &get(const std::string &name);
	const Faction &getFaction(const std::string &name);
	std::pair<Mod *, std::filesystem::path> getPath(const std::string &name);
};


#endif //MIXRTS_MODMANAGER_HPP
