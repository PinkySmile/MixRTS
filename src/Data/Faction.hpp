//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_FACTION_HPP
#define MIXRTS_FACTION_HPP


#include <filesystem>
#include <nlohmann/json.hpp>
#include "forward.hpp"
#include "Sprite.hpp"
#include "MultiSound.hpp"

struct StartObject {
	Vector2i offset = {0, 0};
	std::string name;
	std::string queue;
};

class Faction {
private:
	const Mod &_base;
	mutable std::array<std::vector<Sprite>, 3> _placementCursors;
	std::vector<std::string> _sounds;
	std::vector<std::string> _objects;
	std::vector<std::string> _upgrades;
	std::vector<std::string> _animations;
	std::vector<std::string> _effects;
	std::vector<std::string> _weapons;
	std::unordered_map<std::string, MultiSound> _loadedSounds;
	std::unordered_map<std::string, Upgrade> _loadedUpgrades;
	std::unordered_map<std::string, GameObject> _loadedObjects;
	std::unordered_map<std::string, std::vector<Animation>> _loadedAnimations;
	std::unordered_map<std::string, Effect> _loadedEffects;
	std::unordered_map<std::string, Weapon> _loadedWeapons;
	std::unordered_map<std::string, unsigned> _startingMoney;
	std::vector<StartObject> _startingObjects;
	std::optional<std::string> _primaryQueue;
	std::optional<nlohmann::json> _repairAnim;
	nlohmann::json _uiData;
	std::string _name;
	bool _regressTech;
	bool _canSell;
	bool _canRepair;
	bool _hasRadar;
	bool _radarPowered;

public:
	enum CursorIndex {
		CURSORINDEX_BEST,
		CURSORINDEX_OK,
		CURSORINDEX_BAD,
	};

	Faction(const Mod &base, const std::string &name, const nlohmann::json &data);
	void load();
	[[nodiscard]] const std::optional<std::string> &getPrimaryQueue() const;
	[[nodiscard]] const std::unordered_map<std::string, MultiSound> &getSounds() const;
	[[nodiscard]] const std::unordered_map<std::string, Upgrade> &getUpgrades() const;
	[[nodiscard]] const std::unordered_map<std::string, GameObject> &getObjects() const;
	[[nodiscard]] const std::unordered_map<std::string, Effect> &getEffects() const;
	[[nodiscard]] const std::unordered_map<std::string, Weapon> &getWeapons() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getStartingMoney() const;
	[[nodiscard]] const std::vector<StartObject> &getStartingObjects() const;
	[[nodiscard]] UI *createUI() const;
	[[nodiscard]] const Mod &getBase() const;
	[[nodiscard]] std::array<std::vector<Sprite>, 3> &getPlacementCursors() const;
	[[nodiscard]] bool canSell() const;
	[[nodiscard]] bool canRepair() const;
	[[nodiscard]] bool regressesTech() const;
	[[nodiscard]] bool hasRadar() const;
	[[nodiscard]] bool isRadarPowered() const;
	[[nodiscard]] const std::string &getName() const;
	[[nodiscard]] const std::optional<nlohmann::json> &getRepairAnim() const;
};


#endif //MIXRTS_FACTION_HPP
