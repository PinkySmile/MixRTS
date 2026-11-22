//
// Created by PinkySmile on 29/03/2026.
//

#include <cctype>
#include <ranges>
#include <sol/sol.hpp>
#include "Resources/Assert.hpp"
#include "Faction.hpp"
#include "JsonLua.hpp"
#include "Mod.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "Effect.hpp"
#include "Weapon.hpp"
#include "Warhead.hpp"
#include "Objects/GameObject.hpp"
#include "UI/LuaUI.hpp"
#include "Resources/Game.hpp"
#include "Utils.hpp"

FactionQueue::FactionQueue(const Mod &base, const nlohmann::json &data)
{
	assert_exp(data.contains("requires_producer") && data["requires_producer"].is_boolean());
	assert_exp(data.contains("behavior") && data["behavior"].is_string());
	assert_exp(data.contains("unit_spawn") && data["unit_spawn"].is_string());
	assert_exp(!data.contains("cursor") || data["cursor"].is_string());
	assert_exp(!data.contains("power_mul") || data["power_mul"].is_array());
	assert_exp(!data.contains("producer_mul") || data["producer_mul"].is_array());

	std::string behavior = data["behavior"];
	std::string spawnType = data["unit_spawn"];

	this->requiresProducer = data["requires_producer"];
	if (behavior == "single_queue")
		this->behavior = TYPE_SINGLE_QUEUE;
	else if (behavior == "multi_queue")
		this->behavior = TYPE_MULTI_QUEUE;
	else if (behavior == "starport")
		this->behavior = TYPE_STARPORT;
	else if (behavior == "auto_tick")
		this->behavior = TYPE_AUTO_TICK;
	else
		assert_not_reached_msg("Invalid queue behavior");

	if (spawnType == "none")
		this->unitSpawn = SPAWN_NONE;
	else if (spawnType == "inside")
		this->unitSpawn = SPAWN_INSIDE;
	else if (spawnType == "placement")
		this->unitSpawn = SPAWN_PLACEMENT;
	else if (spawnType == "delivery")
		this->unitSpawn = SPAWN_DELIVERY;
	else
		assert_not_reached_msg("Invalid queue unit_spawn");

	if (data.contains("cursor"))
		this->cursor = UI::stringToCursor(data["cursor"]);
	if (data.contains("loader")) {
		assert_exp(data.contains("loader") && data["loader"].is_string());
		this->loader.emplace(base.getBase() / data["loader"].get<std::string>());
	}
	if (data.contains("power_mul"))
		for (size_t i = 0; i < data["power_mul"].size(); i++) {
			const nlohmann::json &mul = data["power_mul"][i];

			assert_msg(mul.is_array() && mul.size() == 2 && !this->powerMul.contains(mul[0]), "json[\"power_mul\"][" + std::to_string(i) + "]");
			this->powerMul[mul[0]] = mul[1];
		}
	if (data.contains("producer_mul"))
		for (size_t i = 0; i < data["producer_mul"].size(); i++) {
			const nlohmann::json &mul = data["producer_mul"][i];

			assert_msg(mul.is_array() && mul.size() == 2 && !this->producerMul.contains(mul[0]), "json[\"producer_mul\"][" + std::to_string(i) + "]");
			this->producerMul[mul[0]] = mul[1];
		}
	if (this->behavior == TYPE_STARPORT)
		this->starportParams.emplace(base, data);
	if (this->unitSpawn == SPAWN_DELIVERY)
		this->delivery.emplace(base, data);
}

FactionQueue::StarportParams::StarportParams(const Mod &base, const nlohmann::json &data)
{
	assert_exp(data.contains("stock_increase_delay") && data["stock_increase_delay"].is_number_unsigned());
	assert_exp(data.contains("stock_increase_proba") && data["stock_increase_proba"].is_number_unsigned());
	assert_exp(data.contains("stock_max") && data["stock_max"].is_number_unsigned());
	assert_exp(data.contains("discount_min") && data["discount_min"].is_number());
	assert_exp(data.contains("discount_max") && data["discount_max"].is_number());
	assert_exp(data.contains("delivery_delay") && data["delivery_delay"].is_number_unsigned());
	assert_exp(data.contains("delivery_max") && data["delivery_max"].is_number_unsigned());
	assert_exp(data.contains("delivery_eva_times") && data["delivery_eva_times"].is_number_unsigned());
	assert_exp(data.contains("single_max") && data["single_max"].is_number_unsigned());
	assert_exp(data.contains("units") && data["units"].is_array());

	this->stockIncreaseDelay = data["stock_increase_delay"];
	this->stockIncreaseProba = data["stock_increase_proba"];
	this->stockMax = data["stock_max"];
	this->discountMin = data["discount_min"];
	this->discountMax = data["discount_max"];
	this->deliveryDelay = data["delivery_delay"];
	this->deliveryMax = data["delivery_max"];
	this->deliveryEvaTimes = data["delivery_eva_times"];
	this->singleMax = data["single_max"];
	this->units.reserve(data["units"].size());
	for (size_t i = 0; i < data["units"].size(); i++)
		TRY {
			assert_exp(data["units"][i].is_string());
			this->units.emplace_back(RESOURCE(base.getName(), data["units"][i]));
		} CATCH_AND_RETHROW("In [\"units\"][" + std::to_string(i) + "]");
}

SLFactionObjectFactory::SLFactionObjectFactory(const std::filesystem::path &path)
{
	this->_library = dlopen(path.string().c_str(), RTLD_LAZY);
	assert_msg(this->_library, path.string() + ": " + dlerror());

	auto version = reinterpret_cast<unsigned *>(dlsym(this->_library, "_ABI_VERSION"));
	assert_exp(version);
	assert_eq(*version, ABI_VERSION);

	this->_registerFactory = reinterpret_cast<void (*)(ObjectFactory &factory)>(dlsym(this->_library, "registerFactory"));
	assert_exp(this->_registerFactory);
}

SLFactionObjectFactory::~SLFactionObjectFactory()
{
	dlclose(this->_library);
}

void SLFactionObjectFactory::registerFactory(ObjectFactory &factory)
{
	this->_registerFactory(factory);
}

Faction::Faction(const Mod &base, const std::string &name, const nlohmann::json &data) :
	_base(base),
	_factory(*this),
	_name(name)
{
	assert_exp(!data.contains("selectable") || data["selectable"].is_boolean());
	assert_exp(data.contains("can_sell") && data["can_sell"].is_boolean());
	assert_exp(data.contains("can_repair") && data["can_repair"].is_boolean());
	assert_exp(data.contains("regress_tech") && data["regress_tech"].is_boolean());
	assert_exp(!data.contains("primary_queue") || data["primary_queue"].is_string());
	assert_exp(data.contains("has_radar") && data["has_radar"].is_boolean());
	assert_exp(data.contains("radar_powered") && data["radar_powered"].is_boolean());
	assert_exp(!data.contains("repairing_animation") || data["repairing_animation"].is_object());
	assert_exp(data.contains("queues") && data["queues"].is_object());
	assert_exp(!data.contains("ui_path") || data["ui_path"].is_string());
	assert_exp(!data.contains("object_factory") || data["object_factory"].is_string());
	assert_exp(data.contains("preview") && data["preview"].is_object());
	assert_exp(data["preview"].contains("file") && data["preview"]["file"].is_string());
	assert_exp(data["preview"].contains("key") && data["preview"]["key"].is_string());
	assert_exp(data["preview"].contains("size") && data["preview"]["size"].is_object());
	assert_exp(data["preview"]["size"].contains("x") && data["preview"]["size"]["x"].is_number_unsigned());
	assert_exp(data["preview"]["size"].contains("y") && data["preview"]["size"]["y"].is_number_unsigned());

	assert_exp(data.contains("sounds") && data["sounds"].is_array());
	for (size_t i = 0; i < data["sounds"].size(); i++)
		assert_msg(data["sounds"][i].is_string(), "data[\"sounds\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("objects") && data["objects"].is_array());
	for (size_t i = 0; i < data["objects"].size(); i++)
		assert_msg(data["objects"][i].is_string(), "data[\"objects\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("animations") && data["animations"].is_array());
	for (size_t i = 0; i < data["animations"].size(); i++)
		assert_msg(data["animations"][i].is_string(), "data[\"animations\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("upgrades") && data["upgrades"].is_array());
	for (size_t i = 0; i < data["upgrades"].size(); i++)
		assert_msg(data["upgrades"][i].is_string(), "data[\"upgrades\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("effects") && data["effects"].is_array());
	for (size_t i = 0; i < data["effects"].size(); i++)
		assert_msg(data["effects"][i].is_string(), "data[\"effects\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("weapons") && data["weapons"].is_array());
	for (size_t i = 0; i < data["weapons"].size(); i++)
		assert_msg(data["weapons"][i].is_string(), "data[\"weapons\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("warheads") && data["warheads"].is_array());
	for (size_t i = 0; i < data["warheads"].size(); i++)
		assert_msg(data["warheads"][i].is_string(), "data[\"warheads\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("starting_money") && data["starting_money"].is_object());
	for (auto &[key, value] : data["starting_money"].items())
		assert_msg(value.is_number(), "data[\"starting_money\"][\"" + key + "\"]");

	assert_exp(data.contains("place") && data["place"].is_object());
	assert_exp(data["place"].contains("best") && data["place"]["best"].is_array());
	for (size_t i = 0; i < data["place"]["best"].size(); i++)
		assert_msg(data["place"]["best"][i].is_string(), "data[\"place\"][\"best\"][" + std::to_string(i) + "]");

	assert_exp(data["place"].contains("ok") && data["place"]["ok"].is_array());
	for (size_t i = 0; i < data["place"]["ok"].size(); i++)
		assert_msg(data["place"]["ok"][i].is_string(), "data[\"place\"][\"ok\"][" + std::to_string(i) + "]");

	assert_exp(data["place"].contains("bad") && data["place"]["bad"].is_array());
	for (size_t i = 0; i < data["place"]["bad"].size(); i++)
		assert_msg(data["place"]["bad"][i].is_string(), "data[\"place\"][\"bad\"][" + std::to_string(i) + "]");

	assert_exp(data.contains("starting_objects") && data["starting_objects"].is_array());
	for (size_t i = 0; i < data["starting_objects"].size(); i++) {
		assert_msg(data["starting_objects"][i].is_object(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i].contains("name"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i].contains("queue"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i]["name"].is_string(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(data["starting_objects"][i]["queue"].is_string(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		assert_msg(!data["starting_objects"][i].contains("offset") || data["starting_objects"][i]["offset"].is_object(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		if (data["starting_objects"][i].contains("offset")) {
			assert_msg(data["starting_objects"][i]["offset"].contains("x"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"].contains("y"), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"]["x"].is_number(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
			assert_msg(data["starting_objects"][i]["offset"]["y"].is_number(), "data[\"starting_objects\"][" + std::to_string(i) + "]");
		}
	}

	this->_canSell = data["can_sell"];
	this->_canRepair = data["can_repair"];
	this->_objects = data["objects"];
	this->_animations = data["animations"];
	this->_upgrades = data["upgrades"];
	this->_effects = data["effects"];
	this->_weapons = data["weapons"];
	this->_warheads = data["warheads"];
	this->_sounds = data["sounds"];
	this->_startingMoney = data["starting_money"];
	this->_regressTech = data["regress_tech"];
	this->_hasRadar = data["has_radar"];
	this->_radarPowered = data["radar_powered"];

	if (data.contains("selectable"))
		this->_selectable = data["selectable"];
	if (data.contains("object_factory")) {
		std::pair<Mod *, std::filesystem::path> path = game->modManager.getPath(RESOURCE(base.getName(), data["object_factory"]));

		if (path.second.extension() == "lua")
			assert_not_reached();
		else
			this->_factionFactory = std::make_shared<SLFactionObjectFactory>(path.second);
		this->_factionFactory->registerFactory(this->_factory);
	}
	if (data.contains("ui_path")) {
		this->_uiPath = data["ui_path"];
		assert_exp(this->_uiPath.ends_with(".lua") || this->_uiPath.ends_with(".ui"));
	}
	if (data.contains("ui_params"))
		this->_uiData = data["ui_params"];
	if (data.contains("repairing_animation"))
		this->_repairAnim = data["repairing_animation"];
	if (data.contains("primary_queue"))
		this->_primaryQueue = data["primary_queue"].get<std::string>();
	for (const auto &p : data["place"]["best"]) {
		this->_placementCursors[0].emplace_back();
		this->_placementCursors[0].back().load(base.getBase() / p);
	}
	for (const auto &p : data["place"]["ok"]) {
		this->_placementCursors[1].emplace_back();
		this->_placementCursors[1].back().load(base.getBase() / p);
	}
	for (const auto &p : data["place"]["bad"]) {
		this->_placementCursors[2].emplace_back();
		this->_placementCursors[2].back().load(base.getBase() / p);
	}
	for (const auto &p : data["starting_objects"]) {
		StartObject b;

		b.name  = RESOURCE(base.getName(), p["name"]);
		b.queue = RESOURCE(base.getName(), p["queue"]);
		if (p.contains("offset")) {
			b.offset.x = p["offset"]["x"];
			b.offset.y = p["offset"]["y"];
		}
		this->_startingObjects.push_back(b);
	}
	for (const auto &[n, q] : data["queues"].items())
		TRY {
			this->_queues.emplace(RESOURCE(base.getName(), n), FactionQueue{base, q});
		} CATCH_AND_RETHROW("In faction queue \"" + n + "\"\n")
	this->_definition = data.dump();
	this->_previewSize.x = data["preview"]["size"]["x"];
	this->_previewSize.y = data["preview"]["size"]["y"];
	this->_base.loadAnimation(this->_base.getBase() / data["preview"]["file"], data["preview"]["key"], this->_preview);
}

Faction::~Faction()
{
	if (this->_sl)
		dlclose(this->_sl);
}

void Faction::load(const std::vector<std::string> &allFactions)
{
	TRY {
		for (auto &a : this->_animations)
			this->_base.loadAnimations(a, this->_loadedAnimations, &this->_checksumFiles);
		for (auto &a : this->_sounds)
			this->_base.loadSounds(a, this->_loadedSounds, &this->_checksumFiles);
		for (auto &e : this->_effects)
			this->_base.loadEffects(e, this->_loadedAnimations, this->_loadedSounds, this->_loadedEffects, &this->_checksumFiles);
		for (auto &w : this->_warheads)
			this->_base.loadWarheads(w, this->_loadedWarheads, &this->_checksumFiles);
		if (!this->_warheads.empty())
			this->_base.loadArmors(allFactions, this->_loadedWarheads, &this->_checksumFiles);
		for (auto &w : this->_weapons)
			this->_base.loadWeapons(w, this->_loadedAnimations, this->_loadedEffects, this->_loadedSounds, this->_loadedWarheads, this->_loadedWeapons, &this->_checksumFiles);
		for (auto &u : this->_objects)
			this->_base.loadObjects(u, *this, this->_factory, this->_loadedAnimations, this->_loadedSounds, this->_loadedEffects, this->_loadedWeapons, this->_loadedObjects, &this->_checksumFiles);
		for (auto &u : this->_upgrades)
			this->_base.loadUpgrades(u, *this, this->_loadedUpgrades, &this->_checksumFiles);
		for (auto &[n, q] : this->_queues)
			if (q.starportParams)
				for (auto &u : q.starportParams->units)
					TRY {
						assert_exp(this->_loadedObjects.contains(u));
					} CATCH_AND_RETHROW("In queue \"" + n + "\"")
	} CATCH_AND_RETHROW("While loading faction \"" + this->_name + "\"\n")
}

const std::unordered_map<std::string, std::shared_ptr<GameObject>> &Faction::getObjects() const
{
	return this->_loadedObjects;
}

const std::unordered_map<std::string, MultiSound> &Faction::getSounds() const
{
	return this->_loadedSounds;
}

const std::unordered_map<std::string, Upgrade> &Faction::getUpgrades() const
{
	return this->_loadedUpgrades;
}

const std::unordered_map<std::string, Effect> &Faction::getEffects() const
{
	return this->_loadedEffects;
}

const std::unordered_map<std::string, Weapon> &Faction::getWeapons() const
{
	return this->_loadedWeapons;
}

const std::unordered_map<std::string, Warhead> &Faction::getWarheads() const
{
	return this->_loadedWarheads;
}

const std::unordered_map<std::string, std::vector<Animation>> & Faction::getAnimations() const
{
	return this->_loadedAnimations;
}

std::unordered_map<std::string, std::shared_ptr<GameObject>> &Faction::getObjects()
{
	return this->_loadedObjects;
}

std::unordered_map<std::string, MultiSound> &Faction::getSounds()
{
	return this->_loadedSounds;
}

std::unordered_map<std::string, Upgrade> &Faction::getUpgrades()
{
	return this->_loadedUpgrades;
}

std::unordered_map<std::string, Effect> &Faction::getEffects()
{
	return this->_loadedEffects;
}

std::unordered_map<std::string, Weapon> &Faction::getWeapons()
{
	return this->_loadedWeapons;
}

std::unordered_map<std::string, Warhead> &Faction::getWarheads()
{
	return this->_loadedWarheads;
}

std::unordered_map<std::string, std::vector<Animation>> & Faction::getAnimations()
{
	return this->_loadedAnimations;
}

const std::unordered_map<std::string, unsigned> &Faction::getStartingMoney() const
{
	return this->_startingMoney;
}

const std::vector<StartObject> &Faction::getStartingObjects() const
{
	return this->_startingObjects;
}

UI *Faction::createUI(UIState *state, GameManager *manager, Player *owner, MapState *map) const
{
	if (this->_uiPath.empty())
		return nullptr;
	if (this->_uiPath.ends_with(".lua"))
		return new LuaUI(*this, state, manager, owner, map, game->lua, (this->_base.getBase() / this->_uiPath).string());
	if (this->_sl == nullptr) {
		std::filesystem::path p = this->_base.getBase() / this->_uiPath;

		this->_sl = dlopen(p.string().c_str(), RTLD_LAZY);
		assert_msg(this->_sl, p.string() + ": " + dlerror());

		auto version = reinterpret_cast<unsigned *>(dlsym(this->_sl, "_ABI_VERSION"));
		assert_exp(version);
		assert_eq(*version, ABI_VERSION);
	}

	auto fct = reinterpret_cast<UICreationCB>(dlsym(this->_sl, "createUiFor"));

	assert_msg(fct, dlerror());
	return fct(*this, state, manager, owner, map, this->_base.getBase(), this->_uiData);
}

const Mod &Faction::getBase() const
{
	return this->_base;
}

std::array<std::vector<Sprite>, 3> &Faction::getPlacementCursors() const
{
	return this->_placementCursors;
}

bool Faction::canSell() const
{
	return this->_canSell;
}

bool Faction::canRepair() const
{
	return this->_canRepair;
}

bool Faction::regressesTech() const
{
	return this->_regressTech;
}

bool Faction::hasRadar() const
{
	return this->_hasRadar;
}

bool Faction::isRadarPowered() const
{
	return this->_radarPowered;
}

bool Faction::isSelectable() const
{
	return this->_selectable;
}

static bool isTextChecksumFile(const std::filesystem::path &path)
{
	std::string ext = path.extension().string();

	for (char &c : ext)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return ext == ".json" || ext == ".lua" || ext == ".txt";
}

uint32_t Faction::getChecksum() const
{
	uint32_t crc = crc32(this->_definition.data(), this->_definition.size());

	for (const auto &id : this->_checksumFiles) {
		std::filesystem::path file = game->modManager.getPath(id).second;

		crc = crc32File(file, crc, isTextChecksumFile(file));
	}
	return crc;
}

const std::string &Faction::getName() const
{
	return this->_name;
}

const std::optional<nlohmann::json> &Faction::getRepairAnim() const
{
	return this->_repairAnim;
}

const nlohmann::json &Faction::getUIData() const
{
	return this->_uiData;
}

const std::unordered_map<std::string, FactionQueue> & Faction::getQueues() const
{
	return this->_queues;
}

std::vector<Animation> &Faction::getPreview()
{
	return this->_preview;
}

Vector2u Faction::getPreviewSize() const
{
	return this->_previewSize;
}

const std::optional<std::string> &Faction::getPrimaryQueue() const
{
	return this->_primaryQueue;
}

void StartObject::pushUserType(sol::state &state)
{
	sol::usertype<StartObject> type = state.new_usertype<StartObject>("StartObject");

	type["offset"] = &StartObject::offset;
	type["name"] = &StartObject::name;
	type["queue"] = &StartObject::queue;
}

void FactionQueue::pushUserType(sol::state &state)
{
	state.new_enum<Type>("FactionQueue_Type", {
		{"SINGLE_QUEUE", TYPE_SINGLE_QUEUE},
		{"MULTI_QUEUE",  TYPE_MULTI_QUEUE},
		{"STARPORT",     TYPE_STARPORT},
	});
	state.new_enum<Spawn>("FactionQueue_Spawn", {
		{"NONE",      SPAWN_NONE},
		{"INSIDE",    SPAWN_INSIDE},
		{"PLACEMENT", SPAWN_PLACEMENT},
		{"DELIVERY",  SPAWN_DELIVERY},
	});

	sol::usertype<StarportParams> starportType = state.new_usertype<StarportParams>("FactionQueue_StarportParams");

	starportType["stock_increase_delay"] = &StarportParams::stockIncreaseDelay;
	starportType["stock_increase_proba"] = &StarportParams::stockIncreaseProba;
	starportType["stock_max"] = &StarportParams::stockMax;
	starportType["discount_min"] = &StarportParams::discountMin;
	starportType["discount_max"] = &StarportParams::discountMax;
	starportType["delivery_delay"] = &StarportParams::deliveryDelay;
	starportType["delivery_max"] = &StarportParams::deliveryMax;
	starportType["delivery_eva_times"] = &StarportParams::deliveryEvaTimes;
	starportType["single_max"] = &StarportParams::singleMax;
	starportType["units"] = &StarportParams::units;

	sol::usertype<FactionQueue> type = state.new_usertype<FactionQueue>("FactionQueue");

	type["behavior"] = &FactionQueue::behavior;
	type["requires_producer"] = &FactionQueue::requiresProducer;
	type["unit_spawn"] = &FactionQueue::unitSpawn;
	type["cursor"] = &FactionQueue::cursor;
	type["starport_params"] = &FactionQueue::starportParams;
	type["delivery"] = &FactionQueue::delivery;
}

void Faction::pushUserType(sol::state &state)
{
	StartObject::pushUserType(state);
	FactionQueue::pushUserType(state);

	state.new_enum<Faction::CursorIndex>("FactionCursorIndex", {
		{"BEST", Faction::CURSORINDEX_BEST},
		{"OK",   Faction::CURSORINDEX_OK},
		{"BAD",  Faction::CURSORINDEX_BAD},
	});

	sol::usertype<Faction> type = state.new_usertype<Faction>("Faction");

	type["primary_queue"] = sol::property(&Faction::getPrimaryQueue);
	type["sounds"] = sol::property(static_cast<const std::unordered_map<std::string, MultiSound> &(Faction::*)() const>(&Faction::getSounds));
	type["upgrades"] = sol::property(static_cast<const std::unordered_map<std::string, Upgrade> &(Faction::*)() const>(&Faction::getUpgrades));
	type["objects"] = sol::property(static_cast<const std::unordered_map<std::string, std::shared_ptr<GameObject>> &(Faction::*)() const>(&Faction::getObjects));
	type["effects"] = sol::property(static_cast<const std::unordered_map<std::string, Effect> &(Faction::*)() const>(&Faction::getEffects));
	type["weapons"] = sol::property(static_cast<const std::unordered_map<std::string, Weapon> &(Faction::*)() const>(&Faction::getWeapons));
	type["warheads"] = sol::property(static_cast<const std::unordered_map<std::string, Warhead> &(Faction::*)() const>(&Faction::getWarheads));
	type["animations"] = sol::property(static_cast<const std::unordered_map<std::string, std::vector<Animation>> &(Faction::*)() const>(&Faction::getAnimations));
	type["starting_money"] = sol::property(&Faction::getStartingMoney);
	type["starting_objects"] = sol::property(&Faction::getStartingObjects);
	type["base"] = sol::property(&Faction::getBase);
	type["can_sell"] = sol::property(&Faction::canSell);
	type["can_repair"] = sol::property(&Faction::canRepair);
	type["regresses_tech"] = sol::property(&Faction::regressesTech);
	type["has_radar"] = sol::property(&Faction::hasRadar);
	type["is_radar_powered"] = sol::property(&Faction::isRadarPowered);
	type["name"] = sol::property(&Faction::getName);
	type["queues"] = sol::property(&Faction::getQueues);
	type["repair_anim"] = sol::property(&Faction::getRepairAnim);
	type["ui_data"] = sol::property(&Faction::getUIData);
	// NOTE: createUI() (owning raw pointer, lifecycle managed by the engine)
	// and getPlacementCursors() (internal rendering state) are intentionally
	// not exposed.
}

