//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_FACTION_HPP
#define MIXRTS_FACTION_HPP


#include <filesystem>
#include <nlohmann/json.hpp>
#include "forward.hpp"
#include "Sprite.hpp"

class Faction {
private:
	const Mod &_base;
	bool _canSell;
	bool _canRepair;
	mutable std::array<std::vector<Sprite>, 3> _placementCursors;
	std::vector<std::string> _buildings;
	std::vector<std::string> _animations;
	std::vector<std::string> _upgrades;
	std::map<std::string, Upgrade> _loadedUpgrades;
	std::map<std::string, Building> _loadedBuildings;
	std::map<std::string, std::vector<Animation>> _loadedAnimations;
	nlohmann::json _uiData;
	bool _regressTech;

public:
	enum CursorIndex {
		CURSORINDEX_BEST,
		CURSORINDEX_OK,
		CURSORINDEX_BAD,
	};

	Faction(const Mod &base, const nlohmann::json &data);
	void load();
	const std::map<std::string, Upgrade> &getUpgrades() const;
	const std::map<std::string, Building> &getBuildings() const;
	UI *createUI() const;
	const Mod &getBase() const;
	std::array<std::vector<Sprite>, 3> &getPlacementCursors() const;
	bool canSell() const;
	bool canRepair() const;
	bool regressTech() const;
};


#endif //MIXRTS_FACTION_HPP
