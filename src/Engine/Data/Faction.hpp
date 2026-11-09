//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_FACTION_HPP
#define MIXRTS_FACTION_HPP


#include <nlohmann/json.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Extra.hpp"
#include "Sprite.hpp"
#include "MultiSound.hpp"
#include "SharedLibrary.hpp"
#include "UI/UI.hpp"


struct MYDLL_API StartObject {
	Vector2i offset = {0, 0};
	std::string name;
	std::string queue;

	static void pushUserType(sol::state &state);
};

struct MYDLL_API FactionQueue {
	enum Type {
		TYPE_SINGLE_QUEUE,
		TYPE_MULTI_QUEUE,
		TYPE_STARPORT,
		TYPE_AUTO_TICK
	};
	enum Spawn {
		SPAWN_NONE,
		SPAWN_INSIDE,
		SPAWN_PLACEMENT,
		SPAWN_DELIVERY
	};

	struct StarportParams {
		unsigned stockIncreaseDelay;
		unsigned stockIncreaseProba;
		unsigned stockMax;
		int discountMin;
		int discountMax;
		unsigned deliveryDelay;
		unsigned deliveryMax;
		unsigned deliveryEvaTimes;
		unsigned singleMax;
		std::vector<std::string> units;

		StarportParams(const Mod &base, const nlohmann::json &data);
	};

	bool requiresProducer;
	Type behavior;
	Spawn unitSpawn;
	std::optional<UI::Cursor> cursor;
	std::map<float, float, std::greater<>> powerMul;
	std::map<unsigned, float, std::greater<>> producerMul;
	std::optional<StarportParams> starportParams;
	std::optional<ExtraSpawn> delivery;
	std::optional<std::filesystem::path> loader;

	FactionQueue(const Mod &base, const nlohmann::json &data);

	static void pushUserType(sol::state &state);
};

class MYDLL_API Faction {
private:
	const Mod &_base;
	mutable std::array<std::vector<Sprite>, 3> _placementCursors;
	std::vector<std::string> _sounds;
	std::vector<std::string> _objects;
	std::vector<std::string> _upgrades;
	std::vector<std::string> _animations;
	std::vector<std::string> _effects;
	std::vector<std::string> _weapons;
	std::vector<std::string> _warheads;
	std::unordered_map<std::string, FactionQueue> _queues;
	std::unordered_map<std::string, MultiSound> _loadedSounds;
	std::unordered_map<std::string, Upgrade> _loadedUpgrades;
	std::unordered_map<std::string, GameObject> _loadedObjects;
	std::unordered_map<std::string, std::vector<Animation>> _loadedAnimations;
	std::unordered_map<std::string, Effect> _loadedEffects;
	std::unordered_map<std::string, Weapon> _loadedWeapons;
	std::unordered_map<std::string, Warhead> _loadedWarheads;
	std::unordered_map<std::string, unsigned> _startingMoney;
	std::vector<StartObject> _startingObjects;
	std::optional<std::string> _primaryQueue;
	std::optional<nlohmann::json> _repairAnim;
	nlohmann::json _uiData;
	std::string _uiPath;
	std::string _name;
	mutable SharedLibrary _sl = nullptr;
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

	typedef UI *(*UICreationCB)(const Faction &faction, UIState *state, GameManager *manager, PlayerState *ownerState, MapState *map, const std::filesystem::path &base, const nlohmann::json &params);

	Faction(const Mod &base, const std::string &name, const nlohmann::json &data);
	~Faction();
	void load(const std::vector<std::string> &allFactions);
	[[nodiscard]] const std::optional<std::string> &getPrimaryQueue() const;
	[[nodiscard]] const std::unordered_map<std::string, MultiSound> &getSounds() const;
	[[nodiscard]] const std::unordered_map<std::string, Upgrade> &getUpgrades() const;
	[[nodiscard]] const std::unordered_map<std::string, GameObject> &getObjects() const;
	[[nodiscard]] const std::unordered_map<std::string, Effect> &getEffects() const;
	[[nodiscard]] const std::unordered_map<std::string, Weapon> &getWeapons() const;
	[[nodiscard]] const std::unordered_map<std::string, Warhead> &getWarheads() const;
	[[nodiscard]] const std::unordered_map<std::string, std::vector<Animation>> &getAnimations() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getStartingMoney() const;
	[[nodiscard]] const std::vector<StartObject> &getStartingObjects() const;
	[[nodiscard]] UI *createUI(UIState *state, GameManager *manager, PlayerState *ownerState, MapState *map) const;
	[[nodiscard]] const Mod &getBase() const;
	[[nodiscard]] std::array<std::vector<Sprite>, 3> &getPlacementCursors() const;
	[[nodiscard]] bool canSell() const;
	[[nodiscard]] bool canRepair() const;
	[[nodiscard]] bool regressesTech() const;
	[[nodiscard]] bool hasRadar() const;
	[[nodiscard]] bool isRadarPowered() const;
	[[nodiscard]] const std::string &getName() const;
	[[nodiscard]] const std::optional<nlohmann::json> &getRepairAnim() const;
	[[nodiscard]] const nlohmann::json &getUIData() const;
	[[nodiscard]] const std::unordered_map<std::string, FactionQueue> &getQueues() const;

	[[nodiscard]] std::unordered_map<std::string, MultiSound> &getSounds();
	[[nodiscard]] std::unordered_map<std::string, Upgrade> &getUpgrades();
	[[nodiscard]] std::unordered_map<std::string, GameObject> &getObjects();
	[[nodiscard]] std::unordered_map<std::string, Effect> &getEffects();
	[[nodiscard]] std::unordered_map<std::string, Weapon> &getWeapons();
	[[nodiscard]] std::unordered_map<std::string, Warhead> &getWarheads();
	[[nodiscard]] std::unordered_map<std::string, std::vector<Animation>> &getAnimations();

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_FACTION_HPP
