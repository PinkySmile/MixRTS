//
// Created by PinkySmile on 20/04/2026.
//

#include <cpptrace/from_current.hpp>
#include "Faction.hpp"
#include "Effect.hpp"
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "Animation.hpp"
#include "Player.hpp"
#include "ConditionalAnim.hpp"
#include "Weapon.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

#define turnTimer _gpTimer[0]
#define harvestTimer _gpTimer[0]
#define cancel _gpTimer[1]
#define harvestDelaying _gpTimer[2]
#define dumpTimer _gpTimer[0]
#define blockTimer _gpTimer[3]
#define waitRetracePath 300
#define waitBlocked 60
#define PATH_UNIT_PENALTY 1000

bool GameObject::_shortenOrder(Order &o)
{
	if (std::holds_alternative<FinishMovementOrder>(o))
		return false;
	if (std::holds_alternative<MoveOrder>(o) || std::holds_alternative<AttackUnitOrder>(o)) {
		this->_orderQueue.emplace_front(FinishMovementOrder{});
		return false;
	} else if (std::holds_alternative<HarvestOrder>(o)) {
		this->cancel = true;
		return false;
	}
	return true;
}

void GameObject::_displayAnim(Animation &anim, Vector2f offset)
{
	Sprite &sprite = anim.getSprite();

	sprite.setPosition({
		sprite.getPosition().x + this->_position.x + offset.x,
		sprite.getPosition().y + this->_position.y + offset.y
	});
	switch (anim.getBlending()) {
	case Animation::BLEND_ADD:
		game->screen->draw(sprite, sf::BlendAdd);
		break;
	case Animation::BLEND_ALPHA:
		game->screen->draw(sprite);
		break;
	}
}

GameObject::GameObject(const GameObject &g) :
	GameObjectData(g)
{
	if (g._dump)
		this->_dump.emplace(
			g._dump->dir,
			g._dump->delay,
			g._dump->delayAfter,
			std::make_unique<ConditionalAnim>(this, *g._dump->during),
			std::make_unique<ConditionalAnim>(this, *g._dump->start),
			std::make_unique<ConditionalAnim>(this, *g._dump->stop)
		);

	if (g._sellAnim)
		this->_sellAnim.emplace(this, *g._sellAnim);
	if (g._repairAnim)
		this->_repairAnim.emplace(this, *g._repairAnim);
	if (g._buildAnim)
		this->_buildAnim.emplace(this, *g._buildAnim);
	if (g._crushAnim)
		this->_crushAnim.emplace(this, *g._crushAnim);
	if (g._producingAnim)
		this->_producingAnim.emplace(this, *g._producingAnim);
	if (g._producedAnim)
		this->_producedAnim.emplace(this, *g._producedAnim);

	this->_idleAnims.reserve(g._idleAnims.size());
	for (auto &anim : g._idleAnims)
		this->_idleAnims.emplace_back(this, anim);
	this->_moveAnim.reserve(g._moveAnim.size());
	for (auto &anim : g._moveAnim)
		this->_moveAnim.emplace_back(this, anim);
	this->_fireAnim.reserve(g._fireAnim.size());
	for (auto &anim : g._fireAnim)
		this->_fireAnim.emplace_back(this, anim);
	this->_crouchAnim.reserve(g._crouchAnim.size());
	for (auto &anim : g._crouchAnim)
		this->_crouchAnim.emplace_back(this, anim);
	this->_crawlAnim.reserve(g._crawlAnim.size());
	for (auto &anim : g._crawlAnim)
		this->_crawlAnim.emplace_back(this, anim);
	this->_crawlFireAnim.reserve(g._crawlFireAnim.size());
	for (auto &anim : g._crawlFireAnim)
		this->_crawlFireAnim.emplace_back(this, anim);

	this->_harvestAnims.reserve(g._harvestAnims.size());
	for (auto &anim : g._harvestAnims)
		this->_harvestAnims.emplace_back(this, anim);
	this->_deathAnims.reserve(g._deathAnims.size());
	for (auto &anim : g._deathAnims)
		this->_deathAnims.emplace_back(this, anim);

	this->_anims.reserve(g._anims.size());
	for (auto &animVec : g._anims) {
		this->_anims.emplace_back();
		this->_anims.back().reserve(animVec.size());
		for (auto &anim : animVec)
			this->_anims.back().emplace_back(this, anim);
	}

	if (!this->_idleAnims.empty())
		this->_idleTimer = random_distrib(game->gameRandom, this->_idleTimerRange.first, this->_idleTimerRange.second);

	std::unordered_map<std::string, std::set<ObjectBarrel *>> barrelByTag;

	for (auto &barrel : this->_barrels)
		for (auto &weapon : barrel.weapons)
			for (auto &tag : weapon.weapon->canHitTags()) {
				this->_weaponByTag[tag].push_back(&weapon);
				barrelByTag[tag].emplace(&barrel);
			}
	for (auto &p : barrelByTag) {
		std::vector<ObjectBarrel *> &b = this->_barrelByTag[p.first];

		b.insert(b.begin(), p.second.begin(), p.second.end());
		std::ranges::sort(b, [](ObjectBarrel *a, ObjectBarrel *b){
			return a->priority < b->priority;
		});
	}
	for (auto &w : this->_weaponByTag | std::ranges::views::values)
		std::ranges::sort(w, [](ObjectWeapon *a, ObjectWeapon *b){
			return a->priority < b->priority;
		});
}

GameObject::GameObject(
	const Faction &faction,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
) :
	GameObjectData(faction)
{
	assert_exp(!json.contains("requires_queue_building") || json["requires_queue_building"].is_boolean());
	assert_exp(!json.contains("requires_placement") || json["requires_placement"].is_boolean());
	assert_exp(!json.contains("grid_bound") || json["grid_bound"].is_boolean());
	assert_exp(!json.contains("central") || json["central"].is_boolean());
	assert_exp(!json.contains("queue") || json["queue"].is_string());
	assert_exp(!json.contains("build") || json["build"].is_string());
	assert_exp(!json.contains("speed") || json["speed"].is_number());
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
	assert_exp(json.contains("hp") && json["hp"].is_number());
	assert_exp(json.contains("cost") && json["cost"].is_object());
	for (auto &[key, value] : json["cost"].items())
		assert_msg(value.is_number(), "json[\"cost\"][\"" + key + "\"]");
	assert_exp(!json.contains("select") || json["select"].is_object());
	assert_exp(json.contains("size") && json["size"].is_object());
	assert_exp(json["size"].contains("x") && json["size"]["x"].is_number());
	assert_exp(json["size"].contains("y") && json["size"]["y"].is_number());
	assert_exp(!json.contains("directions") || json["directions"].is_number_unsigned());
	assert_exp(!json.contains("build_speed") || json["build_speed"].is_number_unsigned());
	assert_exp(!json.contains("group") || json["group"].is_string());
	assert_exp(json.contains("requirements") && json["requirements"].is_array());
	assert_exp(json.contains("tags") && json["tags"].is_array());
	for (size_t i = 0; i < json["tags"].size(); i++)
		assert_msg(json["tags"][i].is_string(), "json[\"tags\"][\"" + std::to_string(i) + "\"]");
	assert_exp(json.contains("armor") && json["armor"].is_string());
	assert_exp(json.contains("anims") && json["anims"].is_array());
	assert_exp(!json.contains("barrels") || json["barrels"].is_array());
	assert_exp(!json.contains("sight") || json["sight"].is_number());
	assert_exp(!json.contains("turn_speed") || json["turn_speed"].is_number());
	assert_exp(!json.contains("do_sound") || json["do_sound"].is_string());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());
	assert_exp(!json.contains("pass_flags") || json["pass_flags"].is_string() || json["pass_flags"].is_number_unsigned());
	assert_exp(!json.contains("fake") || json["fake"].is_boolean());
	assert_exp(!json.contains("proxy") || json["proxy"].is_string());
	assert_exp(!json.contains("harvest_anim") || json["harvest_anim"].is_object());
	assert_exp(!json.contains("death") || json["death"].is_object());
	assert_exp(!json.contains("resource_storage") || json["resource_storage"].is_number_unsigned());
	assert_exp(!json.contains("harvest_time") || json["harvest_time"].is_number_unsigned());
	assert_exp(!json.contains("harvest_delay") || json["harvest_delay"].is_number_unsigned());
	assert_exp(!json.contains("build_max_distance") || json["build_max_distance"].is_number());
	assert_exp(!json.contains("selectable") || json["selectable"].is_boolean());
	assert_exp(!json.contains("sellable") || json["sellable"].is_boolean());
	assert_exp(!json.contains("repairable") || json["repairable"].is_boolean());
	assert_exp(!json.contains("gives_queue") || json["gives_queue"].is_string());
	assert_exp(!json.contains("storage") || json["storage"].is_object());
	assert_exp(!json.contains("build_anim") || json["build_anim"].is_null() || json["build_anim"].is_object());
	assert_exp(!json.contains("producing_anim") || json["producing_anim"].is_null() || json["producing_anim"].is_object());
	assert_exp(!json.contains("produced_anim") || json["produced_anim"].is_null() || json["produced_anim"].is_object());
	assert_exp(!json.contains("stackable") || json["stackable"].is_boolean());
	assert_exp(!json.contains("crush_sound") || json["crush_sound"].is_string());
	assert_exp(!json.contains("death_sound") || json["death_sound"].is_string());
	assert_exp(!json.contains("can_crush") || json["can_crush"].is_boolean());
	assert_exp(!json.contains("radar") || json["radar"].is_boolean());
	assert_exp(!json.contains("repair_delay") || json["repair_delay"].is_number());
	assert_exp(!json.contains("repair_heal") || json["repair_heal"].is_number());
	assert_exp(!json.contains("repair_cost") || json["repair_cost"].is_object());
	if (json.contains("anim_idles")) {
		assert_exp(json["anim_idles"].is_array());
		for (size_t i = 0; i < json["anim_idles"].size(); i++)
			assert_msg(json["anim_idles"][i].is_object(), "json[\"anim_idles\"][" + std::to_string(i) + "]");
	}
	assert_exp(!json.contains("anim_move") || json["anim_move"].is_object());
	assert_exp(!json.contains("anim_fire") || json["anim_fire"].is_object());
	assert_exp(!json.contains("anim_crouch") || json["anim_crouch"].is_object());
	assert_exp(!json.contains("anim_crawl") || json["anim_crawl"].is_object());
	assert_exp(!json.contains("anim_crawl_fire") || json["anim_crawl_fire"].is_object());
	assert_exp(!json.contains("idle_time") || json["idle_time"].is_array());
	if (json.contains("idle_time")) {
		assert_exp(json["idle_time"].size() == 2);
		for (size_t i = 0; i < json["idle_time"].size(); i++)
			assert_msg(json["idle_time"][i].is_number_unsigned(), "json[\"idle_time\"][" + std::to_string(i) + "]");
	}
	assert_exp(!json.contains("show_after_build") || json["show_after_build"].is_number());
	assert_exp(!json.contains("power") || json["power"].is_number());
	assert_exp(!json.contains("lower_power_when_damaged") || json["lower_power_when_damaged"].is_boolean());
	assert_exp(!json.contains("deploy") || json["deploy"].is_object());
	if (json.contains("sell")) {
		assert_exp(json["sell"].is_object());
		assert_exp(json["sell"].contains("anim") && json["sell"]["anim"].is_object());
		assert_exp(json["sell"].contains("units") && json["sell"]["units"].is_object());
		assert_exp(json["sell"].contains("refund") && json["sell"]["refund"].is_object());
	}
	if (json.contains("skirt")) {
		assert_exp(json["skirt"].is_object());
		assert_exp(json["skirt"].contains("x") && json["skirt"]["x"].is_number());
		assert_exp(json["skirt"].contains("y") && json["skirt"]["y"].is_number());
		assert_exp(json["skirt"].contains("w") && json["skirt"]["w"].is_number());
		assert_exp(json["skirt"].contains("h") && json["skirt"]["h"].is_number());
	}
	if (json.contains("exit_offset")) {
		assert_exp(json["exit_offset"].is_object());
		assert_exp(json["exit_offset"].contains("x") && json["exit_offset"]["x"].is_number());
		assert_exp(json["exit_offset"].contains("y") && json["exit_offset"]["y"].is_number());
	}
	if (json.contains("spawn_offset")) {
		assert_exp(json["spawn_offset"].is_object());
		assert_exp(json["spawn_offset"].contains("x") && json["spawn_offset"]["x"].is_number());
		assert_exp(json["spawn_offset"].contains("y") && json["spawn_offset"]["y"].is_number());
	}
	if (json.contains("point")) {
		assert_exp(json["point"].is_object());
		assert_exp(json["point"].contains("x") && json["point"]["x"].is_number());
		assert_exp(json["point"].contains("y") && json["point"]["y"].is_number());
	}
	if (json.contains("dump")) {
		assert_exp(json["dump"].is_object());
		assert_exp(json["dump"].contains("dir") && json["dump"]["dir"].is_number_unsigned());
		assert_exp(json["dump"].contains("dir") && json["dump"]["delay"].is_number_unsigned());
		assert_exp(json["dump"].contains("dir") && json["dump"]["delay_after"].is_number_unsigned());
		assert_exp(json["dump"].contains("during") && json["dump"]["during"].is_object());
		assert_exp(json["dump"].contains("start_anim") && json["dump"]["start_anim"].is_object());
		assert_exp(json["dump"].contains("stop_anim") && json["dump"]["stop_anim"].is_object());
	}
	if (json.contains("tiles")) {
		assert_exp(json["tiles"].is_array());
		for (size_t i = 0; i < json["tiles"].size(); i++) {
			assert_msg(json["tiles"][i].is_array(), "json[\"tiles\"][" + std::to_string(i) + "]");
			for (size_t j = 0; j < json["tiles"][i].size(); j++)
				assert_msg(json["tiles"][i][j].is_number(), "json[\"tiles\"][" + std::to_string(i) + "][" + std::to_string(j) + "]");
		}
	}
	if (json.contains("accept_resources")) {
		assert_exp(json["accept_resources"].is_array());
		for (size_t i = 0; i < json["accept_resources"].size(); i++)
			assert_msg(json["accept_resources"][i].is_string(), "json[\"accept_resources\"][" + std::to_string(i) + "]");
	}
	if (json.contains("spawn_orders")) {
		assert_exp(json["spawn_orders"].is_array());
		for (size_t i = 0; i < json["spawn_orders"].size(); i++)
			assert_msg(json["spawn_orders"][i].is_string(), "json[\"spawn_orders\"][" + std::to_string(i) + "]");
	}
	if (json.contains("spawn")) {
		assert_exp(json["spawn"].is_object());
		assert_exp(json["spawn"].contains("unit"));
		assert_exp(json["spawn"]["unit"].is_string());
		assert_exp(json["spawn"].contains("pos"));
		assert_exp(json["spawn"]["pos"].is_object());
		assert_exp(json["spawn"]["pos"].contains("x"));
		assert_exp(json["spawn"]["pos"]["x"].is_number_integer());
		assert_exp(json["spawn"]["pos"].contains("y"));
		assert_exp(json["spawn"]["pos"]["y"].is_number_integer());
	}
	if (json.contains("storage"))
		for (auto &[key, value] : json["storage"].items())
			assert_msg(value.is_number(), "json[\"storage\"][\"" + key + "\"]");
	if (json.contains("deploy")) {
		assert_exp(!json["deploy"].contains("direction") || json["deploy"]["direction"].is_number());
		assert_exp(!json["deploy"].contains("spawn_building") || json["deploy"]["spawn_building"].is_object());

		this->_onDeploy.emplace();
		if (json["deploy"].contains("direction"))
			this->_onDeploy->direction = json["deploy"]["direction"];
		if (json["deploy"].contains("spawn_building")) {
			this->_onDeploy->spawnBuilding.emplace();
			assert_exp(json["deploy"]["spawn_building"].contains("name"));
			assert_exp(json["deploy"]["spawn_building"]["name"].is_string());
			assert_exp(json["deploy"]["spawn_building"].contains("queue"));
			assert_exp(json["deploy"]["spawn_building"]["queue"].is_string());
			assert_exp(json["deploy"]["spawn_building"].contains("offset"));
			assert_exp(json["deploy"]["spawn_building"]["offset"].is_object());
			assert_exp(json["deploy"]["spawn_building"]["offset"].contains("x"));
			assert_exp(json["deploy"]["spawn_building"]["offset"]["x"].is_number());
			assert_exp(json["deploy"]["spawn_building"]["offset"].contains("y"));
			assert_exp(json["deploy"]["spawn_building"]["offset"]["y"].is_number());
			this->_onDeploy->spawnBuilding->building = RESOURCE(faction.getBase().getName(), json["deploy"]["spawn_building"]["name"]);
			this->_onDeploy->spawnBuilding->queue    = RESOURCE(faction.getBase().getName(), json["deploy"]["spawn_building"]["queue"]);
			this->_onDeploy->spawnBuilding->offset.x = json["deploy"]["spawn_building"]["offset"]["x"];
			this->_onDeploy->spawnBuilding->offset.y = json["deploy"]["spawn_building"]["offset"]["y"];
		}
	}

	auto &base = faction.getBase();
	std::optional<unsigned> directions;

	this->_id = id;
	this->_hp = json["hp"];
	this->_decayTarget = this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_tags = json["tags"];
	this->_armor = RESOURCE(base.getName(), json["armor"].get<std::string>());
	// TODO: Apply right any_ flag
	this->_tags.emplace_back("any_ground");
	this->_tags.emplace_back("any");
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);

	if (json.contains("directions")) {
		directions = json["directions"].get<unsigned>();
		assert_msg(*directions % 8 == 0, "Number of directions must be divisible by 8");
	}
	if (json.contains("lower_power_when_damaged"))
		this->_lowerPowerWhenDamaged = json["lower_power_when_damaged"];
	if (json.contains("build_max_distance"))
		this->_buildMaxDistance = json["build_max_distance"];
	if (json.contains("selectable"))
		this->_selectable = json["selectable"];
	if (json.contains("sellable"))
		this->_sellable = json["sellable"];
	if (json.contains("repairable"))
		this->_repairable = json["repairable"];
	if (json.contains("power"))
		this->_power = json["power"];
	if (json.contains("sight"))
		this->_sight = json["sight"];
	if (json.contains("fake"))
		this->_fake = json["fake"];
	if (json.contains("repair_delay"))
		this->_repairDelay = json["repair_delay"];
	if (json.contains("repair_heal"))
		this->_repairHeal = json["repair_heal"];
	if (json.contains("repair_cost"))
		this->_repairCost = json["repair_cost"];
	if (json.contains("radar"))
		this->_isRadar = json["radar"];
	if (json.contains("spawn"))
		this->_extraSpawn = {
			RESOURCE(base.getName(), json["spawn"]["unit"]),
			{json["spawn"]["pos"]["x"], json["spawn"]["pos"]["y"]}
		};
	if (json.contains("central"))
		this->_central = json["central"];
	if (json.contains("spawn_orders"))
		this->_spawnOrders = json["spawn_orders"];
	if (json.contains("accept_resources"))
		this->_acceptResources = json["accept_resources"];
	if (json.contains("harvest_time"))
		this->_harvestTime = json["harvest_time"];
	if (json.contains("harvest_delay"))
		this->_harvestDelay = json["harvest_delay"];
	if (json.contains("resource_storage"))
		this->_resourceStorage = json["resource_storage"];
	if (json.contains("limit"))
		this->_limit = json["limit"];
	if (json.contains("build_speed"))
		this->_buildSpeed = json["build_speed"];
	if (json.contains("grid_bound"))
		this->_boundToGrid = json["grid_bound"];
	if (json.contains("exit_offset"))
		this->_exitOffset = {json["exit_offset"]["x"], json["exit_offset"]["y"]};
	if (json.contains("skirt"))
		this->_skirt = {json["skirt"]["x"], json["skirt"]["y"], json["skirt"]["w"], json["skirt"]["h"]};
	if (json.contains("spawn_offset"))
		this->_spawnOffset = {json["spawn_offset"]["x"], json["spawn_offset"]["y"]};
	if (json.contains("point"))
		this->_point = {json["point"]["x"], json["point"]["y"]};
	if (json.contains("requires_queue_building"))
		this->_requiresQueueBuilding = json["requires_queue_building"];
	if (json.contains("requires_placement"))
		this->_requiresPlacement = json["requires_placement"];
	if (json.contains("queue"))
		this->_buildQueue = RESOURCE(base.getName(), json["queue"]);
	if (json.contains("fake"))
		this->_fake = json["fake"];
	if (json.contains("proxy"))
		this->_proxy = json["proxy"];
	if (json.contains("stackable"))
		this->_stackable = json["stackable"];
	if (json.contains("can_crush"))
		this->_canCrush = json["can_crush"];
	if (json.contains("speed"))
		this->_speed = json["speed"];
	if (json.contains("gives_queue"))
		this->_giveQueue = RESOURCE(base.getName(), json["gives_queue"]);
	if (json.contains("storage"))
		this->_storage = json["storage"];
	if (json.contains("build_anim") && !json["build_anim"].is_null())
		this->_buildAnim.emplace(base, this, animList, json["build_anim"]);
	if (json.contains("crush_anim") && !json["crush_anim"].is_null())
		TRY {
			this->_crushAnim.emplace(base, this, animList, effects, json["crush_anim"]);
		} CATCH_AND_RETHROW("In key [\"crush_anim\"]\n")
	if (json.contains("producing_anim") && !json["producing_anim"].is_null())
		this->_producingAnim.emplace(base, this, animList, json["producing_anim"]);
	if (json.contains("produced_anim") && !json["produced_anim"].is_null())
		this->_producedAnim.emplace(base, this, animList, json["produced_anim"]);
	if (json.contains("idle_time"))
		this->_idleTimerRange = {json["idle_time"][0], json["idle_time"][1]};
	if (json.contains("turn_speed"))
		this->_turnSpeed = json["turn_speed"];
	if (json.contains("tiles")) {
		std::vector<std::vector<uint8_t>> b = json["tiles"];
		std::vector<std::vector<BuildingMask>> r;

		r.reserve(b.size());
		for (auto &v : b) {
			r.emplace_back();

			auto &h = r.back();

			h.reserve(v.size());
			for (auto i : v)
				h.push_back(BuildingMask{.value = i});
		}
		this->_tiles = r;
	}
	if (json.contains("build")) {
		auto buildSound = RESOURCE(base.getName(), json["build"]);

		assert_msg(sounds.contains(buildSound), buildSound);
		this->_buildSound = &sounds.at(buildSound);
	}
	if (json.contains("crush_sound")) {
		auto crushSound = RESOURCE(base.getName(), json["crush_sound"]);

		assert_msg(sounds.contains(crushSound), crushSound);
		this->_crushSound = &sounds.at(crushSound);
	}
	if (json.contains("death_sound")) {
		auto deathSound = RESOURCE(base.getName(), json["death_sound"]);

		assert_msg(sounds.contains(deathSound), deathSound);
		this->_deathSound = &sounds.at(deathSound);
	}
	if (json.contains("pass_flags")) {
		if (json["pass_flags"].is_string()) {
			std::string str = json["pass_flags"];

			try {
				this->_passFlags = std::stoul(str, nullptr, 16);
			} catch (std::exception &) {
				assert_not_reached_msg("Invalid flag hex \"pass_flags\": '" + str + "'");
			}
		} else
			this->_passFlags = json["pass_flags"];
	}
	if (json.contains("group"))
		this->_group = RESOURCE(base.getName(), "group." + json["group"].get<std::string>());
	else
		this->_group = id;
	if (json.contains("show_after_build"))
		this->_showAfterBuild = json["show_after_build"];
	else if (this->_buildAnim)
		this->_showAfterBuild = this->_buildAnim->size();

	if (json.contains("sell")) {
		this->_sellAnim.emplace(base, this, animList, json["sell"]["anim"]);
		this->_sellRefund = json["sell"]["refund"];
		for (auto &[key, nb] : json["sell"]["units"].items())
			this->_sellUnits[RESOURCE(base.getName(), key)] = nb;
		this->_showBeforeSell = this->_buildAnim->size() - this->_showAfterBuild;
	}
	if (json.contains("select_sound")) {
		std::string selectSound = RESOURCE(base.getName(), json["select_sound"]);

		assert_msg(sounds.contains(selectSound), selectSound);
		this->_selectSound = &sounds.at(selectSound);
	}
	if (json.contains("do_sound")) {
		std::string doSound = RESOURCE(base.getName(), json["do_sound"]);

		assert_msg(sounds.contains(doSound), doSound);
		this->_doSound = &sounds.at(doSound);
	}
	if (json.contains("anim_idles"))
		for (auto &j : json["anim_idles"])
			this->_idleAnims.emplace_back(base, this, animList, j);
	if (directions) {
		this->_anims.resize(*directions);
		for (auto &anim: json["anims"])
			for (size_t i = 0; i < *directions; i++)
				this->_anims[i].emplace_back(base, this, animList, anim, i);
		if (json.contains("anim_move"))
			for (size_t i = 0; i < *directions; i++)
				this->_moveAnim.emplace_back(base, this, animList, json["anim_move"], i);
		if (json.contains("anim_fire"))
			for (size_t i = 0; i < *directions; i++)
				this->_fireAnim.emplace_back(base, this, animList, json["anim_fire"], i);
		if (json.contains("anim_crouch"))
			for (size_t i = 0; i < *directions; i++)
				this->_crouchAnim.emplace_back(base, this, animList, json["anim_crouch"], i);
		if (json.contains("anim_crawl"))
			for (size_t i = 0; i < *directions; i++)
				this->_crawlAnim.emplace_back(base, this, animList, json["anim_crawl"], i);
		if (json.contains("anim_crawl_fire"))
			for (size_t i = 0; i < *directions; i++)
				this->_crawlFireAnim.emplace_back(base, this, animList, json["anim_crawl_fire"], i);
	} else {
		this->_anims.emplace_back();
		for (auto &anim: json["anims"])
			this->_anims.front().emplace_back(base, this, animList, anim);
		if (json.contains("anim_move"))
			this->_moveAnim.emplace_back(base, this, animList, json["anim_move"]);
		if (json.contains("anim_fire"))
			this->_fireAnim.emplace_back(base, this, animList, json["anim_fire"]);
		if (json.contains("anim_crouch"))
			this->_crouchAnim.emplace_back(base, this, animList, json["anim_crouch"]);
		if (json.contains("anim_crawl"))
			this->_crawlAnim.emplace_back(base, this, animList, json["anim_crawl"]);
		if (json.contains("anim_crawl_fire"))
			this->_crawlFireAnim.emplace_back(base, this, animList, json["anim_crawl_fire"]);
	}
	if (json.contains("harvest_anim")) {
		assert_exp(!json["harvest_anim"].contains("dirs") || json["harvest_anim"]["dirs"].is_number_unsigned());

		unsigned dirs = json["harvest_anim"]["dirs"];

		for (size_t i = 0; i < dirs; i++)
			this->_harvestAnims.emplace_back(base, this, animList, json["harvest_anim"], i);
	}
	if (json.contains("death")) {
		static_assert(DMGTYPE_COUNT - DMGTYPE_START == 4);
		static_assert(DMGTYPE_BULLET  == DMGTYPE_START);
		static_assert(DMGTYPE_MISSILE == DMGTYPE_START + 1);
		static_assert(DMGTYPE_BLAST   == DMGTYPE_START + 2);
		static_assert(DMGTYPE_SONIC   == DMGTYPE_START + 3);
		assert_exp(!json["death"].contains("generic") || json["death"]["generic"].is_object());

		const char *anims[] = { "bullet", "missile", "blast", "sonic" };

		for (const char *s : anims)
			assert_msg(json["death"].contains("generic") || (json["death"].contains(s) && json["death"][s].is_object()), s);

		for (const char *s : anims)
			TRY {
				this->_deathAnims.emplace_back(base, this, animList, effects, json["death"].contains(s) ? json["death"][s] : json["death"]["generic"]);
			} CATCH_AND_RETHROW("In key [\"death\"][\"" + std::string(s) + "\"]\n")
	}
	if (json.contains("dump"))
		this->_dump.emplace(
			json["dump"]["dir"].get<unsigned>(),
			json["dump"]["delay"].get<unsigned>(),
			json["dump"]["delay_after"].get<unsigned>(),
			std::make_unique<ConditionalAnim>(base, this, animList, json["dump"]["during"]),
			std::make_unique<ConditionalAnim>(base, this, animList, json["dump"]["start_anim"]),
			std::make_unique<ConditionalAnim>(base, this, animList, json["dump"]["stop_anim"])
		);
	if (json.contains("barrels")) {
		unsigned index = 0;

		for (auto &barrel : json["barrels"])
			this->_barrels.emplace_back(index++, base, weapons, barrel);
	}

	if (json.contains("select")) {
		assert_exp(json["select"].contains("x"));
		assert_exp(json["select"]["x"].is_number());
		assert_exp(json["select"].contains("y"));
		assert_exp(json["select"]["y"].is_number());
		assert_exp(json["select"].contains("h"));
		assert_exp(json["select"]["h"].is_number());
		assert_exp(json["select"].contains("w"));
		assert_exp(json["select"]["w"].is_number());
		this->_selectBox.position.x = json["select"]["x"];
		this->_selectBox.position.y = json["select"]["y"];
		this->_selectBox.size.x = json["select"]["h"];
		this->_selectBox.size.y = json["select"]["w"];
	} else if (!this->_anims.empty() && !this->_anims.front().empty()) {
		auto &anims = this->_anims.front().front().getAnims();

		if (!anims.empty()) {
			auto &anim = anims.front();

			this->_selectBox.position.x = 0;
			this->_selectBox.position.y = anim.getOffset().y;
			this->_selectBox.size = this->_size;
		}
	}
	this->animationState.health = 1;

	if (auto repair = faction.getRepairAnim())
		this->_repairAnim.emplace(base, this, animList, *repair);
	if (this->_storage.size() == 1)
		this->animationState.storageKey = this->_storage.begin()->first;

	if (this->_tiles && this->_buildMaxDistance)
		for (size_t y = 0; y < this->_tiles->size(); y++) {
			auto &line = (*this->_tiles)[y];

			for (size_t x = 0; x < line.size(); x++) {
				BuildingMask buildingMask = line[x];

				if (!buildingMask.placementRequired && !buildingMask.placementConcrete)
					continue;

				for (int i1 = -this->_buildMaxDistance - 1; i1 <= this->_buildMaxDistance + 1; i1++)
					for (int i2 = -this->_buildMaxDistance - 1; i2 <= this->_buildMaxDistance + 1; i2++)
						this->_checkTiles.emplace(i1 + x, i2 + y);
			}
		}

	if (this->_stackable)
		this->_currentTilePosition = PFFREEMASK_TOP_LEFT;
	else
		this->_currentTilePosition = PFFREEMASK_NONE;
	this->_oldTilePosition = this->_currentTilePosition;
	assert_exp(!this->_stackable || this->_boundToGrid);

	std::unordered_map<std::string, std::set<ObjectBarrel *>> barrelByTag;

	for (auto &barrel : this->_barrels)
		for (auto &weapon : barrel.weapons)
			for (auto &tag : weapon.weapon->canHitTags()) {
				this->_weaponByTag[tag].push_back(&weapon);
				barrelByTag[tag].emplace(&barrel);
			}
	for (auto &p : barrelByTag) {
		std::vector<ObjectBarrel *> &b = this->_barrelByTag[p.first];

		b.insert(b.begin(), p.second.begin(), p.second.end());
		std::ranges::sort(b, [](ObjectBarrel *a, ObjectBarrel *b){
			return a->priority < b->priority;
		});
	}
	for (auto &w : this->_weaponByTag | std::ranges::views::values)
		std::ranges::sort(w, [](ObjectWeapon *a, ObjectWeapon *b){
			return a->priority < b->priority;
		});
}

GameObject::~GameObject()
{
	this->deinitGlobalGrid();
}


struct ObstructionContext {
	size_t team;
	uint32_t mask;
	Vector2u hitBoxSize;
	Vector2i dest;
	const MapState *ggrid;
	const PathFindingGrid *grid;
	const GameObject *ignoreObject;
	bool stackable;
	bool canCrush;
};

static bool IsPathObstructedTerrain(Vector2i p, void *ctx)
{
	auto octx = static_cast<ObstructionContext *>(ctx);

	if (p.x < 0)
		return true;
	if (p.y < 0)
		return true;
	if (p.x + octx->hitBoxSize.x > octx->grid->gridSize.x)
		return true;
	if (p.y + octx->hitBoxSize.y > octx->grid->gridSize.y)
		return true;
	if (octx->ignoreObject) {
		Vector2i pos = octx->ignoreObject->getPosition();
		Vector2u size = octx->ignoreObject->getSize();
		MapState::Quad absP = {
			Vector2f(p.x, p.y),
			Vector2f(p.x + 1, p.y),
			Vector2f(p.x + 1, p.y + 1),
			Vector2f(p.x, p.y + 1)
		};

		if (octx->ggrid)
			absP = octx->ggrid->tileCoordToAbs(p);
		if (
			pos.x + size.x > absP.pt1.x &&
			pos.y + size.y > absP.pt1.y &&
			pos.x <= absP.pt3.x &&
			pos.y <= absP.pt3.y
		)
			return false;
	}
	for (size_t y = 0; y < octx->hitBoxSize.y; y++)
		for (size_t x = 0; x < octx->hitBoxSize.x; x++) {
			auto attrs = octx->grid->grid[(p.x + x) + (p.y + y) * octx->grid->gridSize.x];

			if (!(attrs & octx->mask))
				return true;
		}
	return false;
}
static bool IsPathObstructedStatic(Vector2i p, void *ctx)
{
	auto octx = static_cast<ObstructionContext *>(ctx);

	if (p == octx->dest && octx->ignoreObject)
		return false;
	if (p.x < 0)
		return true;
	if (p.y < 0)
		return true;
	if (p.x + octx->hitBoxSize.x > octx->grid->gridSize.x)
		return true;
	if (p.y + octx->hitBoxSize.y > octx->grid->gridSize.y)
		return true;
	if (octx->ignoreObject) {
		Vector2i pos = octx->ignoreObject->getPosition();
		Vector2u size = octx->ignoreObject->getSize();
		MapState::Quad absP = {
			Vector2f(p.x, p.y),
			Vector2f(p.x + 1, p.y),
			Vector2f(p.x + 1, p.y + 1),
			Vector2f(p.x, p.y + 1)
		};

		if (octx->ggrid)
			absP = octx->ggrid->tileCoordToAbs(p);
		if (
			pos.x + size.x > absP.pt1.x &&
			pos.y + size.y > absP.pt1.y &&
			pos.x <= absP.pt3.x &&
			pos.y <= absP.pt3.y
		)
			return false;
	}
	for (size_t y = 0; y < octx->hitBoxSize.y; y++)
		for (size_t x = 0; x < octx->hitBoxSize.x; x++) {
			auto attrs = octx->grid->grid[(p.x + x) + (p.y + y) * octx->grid->gridSize.x];

			if (!(attrs & octx->mask) || PFGRIDMASK_IS_STATIC(attrs))
				return true;
		}
	return false;
}

static float GetTileCostMovableUnits(Vector2i p, void *ctx)
{
	auto octx = static_cast<ObstructionContext *>(ctx);
	float cost = 0;

	if (p == octx->dest && octx->ignoreObject)
		return 0;
	for (size_t y = 0; y < octx->hitBoxSize.y; y++)
		for (size_t x = 0; x < octx->hitBoxSize.x; x++) {
			auto index = (p.x + x) + (p.y + y) * octx->grid->gridSize.x;
			auto attrs = octx->grid->grid[index];
			auto stackCount = PFGRIDMASK_STACK_COUNT(attrs);
			bool occupied = false;

			if (!octx->grid->usedSpaces[octx->team][index])
				for (size_t i = 0; i < MAX_TEAM_COUNT; i++) {
					if (i == octx->team)
						continue;
					if (octx->grid->usedSpaces[i][index]) {
						occupied = true;
						break;
					}
				}
			if (occupied && octx->canCrush) {
				if (PFGRIDMASK_GET_NON_CRUSHABLE(attrs) != 0)
					cost += PATH_UNIT_PENALTY;
			} else if (!octx->stackable) {
				if (stackCount != 0 || PFGRIDMASK_FREE_NB(attrs) != 0)
					cost += PATH_UNIT_PENALTY;
			} else {
				if (stackCount >= PATHFINDING_STACK_LIMIT)
					cost += PATH_UNIT_PENALTY;
			}
		}
	return cost;
}

Vector2f GameObject::_getTileOffsetFromCenter() const
{
	switch (this->_currentTilePosition) {
	case PFFREEMASK_TOP_LEFT:
		return {
			this->map->tileSize.x / -2.f,
			this->map->tileSize.y / -2.f
		};
	case PFFREEMASK_TOP_RIGHT:
		return {
			this->map->tileSize.x / 2.f - this->_size.x,
			this->map->tileSize.y / -2.f
		};
	case PFFREEMASK_BOTTOM_LEFT:
		return {
			this->map->tileSize.x / -2.f,
			this->map->tileSize.y / 2.f - this->_size.y
		};
	case PFFREEMASK_BOTTOM_RIGHT:
		return {
			this->map->tileSize.x / 2.f - this->_size.x,
			this->map->tileSize.y / 2.f - this->_size.y
		};
	case PFFREEMASK_MIDDLE:
	case PFFREEMASK_NONE:
		return {
			this->_size.x / -2.f,
			this->_size.y / -2.f
		};
	default:
		assert_not_reached();
	}
}

bool GameObject::_updateTurn(unsigned int target)
{
	if (this->_currentDir == target)
		return true;
	this->turnTimer++;
	if (this->turnTimer > this->_turnSpeed) {
		size_t diff = (this->_currentDir - target + this->_anims.size()) % this->_anims.size();

		this->turnTimer = 0;
		if (diff >= this->_anims.size() / 2)
			this->_currentDir = (this->_currentDir + 1) % this->_anims.size();
		else
			this->_currentDir = (this->_currentDir - 1 + this->_anims.size()) % this->_anims.size();
	}
	return false;
}

bool GameObject::_updateCheckMove(Vector2i pos, void (GameObject::*fail)())
{
	if (this->_position == this->_oldPos && this->_gpTimer[0] < waitRetracePath) {
		this->_gpTimer[0]++;
		return false;
	}
	this->_oldPos = this->_position;

	if (this->_position != pos) {
		auto front = this->_orderQueue.front();

		this->_orderQueue.pop_front();
		(this->*fail)();
		this->_orderQueue.push_front(front);
	}
	return true;
}

void GameObject::_computeMovePath(Vector2i destination, bool ignoreSolid, const GameObject *ignoreObject)
{
	ObstructionContext ctx;
	Vector2i target;
	Vector2i pos;
	Vector2u step = {1, 1};

	ctx.ignoreObject = ignoreObject;
	ctx.dest = destination;
	ctx.team = this->owner->state.team;
	ctx.mask = this->_passFlags;
	ctx.stackable = this->_stackable;
	ctx.canCrush = this->_canCrush;
	if (this->_boundToGrid) {
		ctx.hitBoxSize = {1, 1};
		ctx.grid = &this->map->pathFindingGrid;
		ctx.ggrid = this->map;
		target = this->map->absToTileCoord(destination);
		pos = this->_gridPos;
	} else {
		ctx.hitBoxSize = this->_size;
		ctx.grid = this->pfGrid;
		ctx.ggrid = nullptr;
		target = destination;
		pos = this->_position;
	}
	AStar::computePath(
		*ctx.grid,
		pos, target, step,
		ignoreSolid ? &IsPathObstructedTerrain : &IsPathObstructedStatic,
		&GetTileCostMovableUnits,
		&ctx,
		this->_pathCache
	);
	if (this->_boundToGrid)
		for (auto &p : this->_pathCache)
			p = this->map->topLeftCoordToAbs(p) + this->map->tileSize / 2;
	while (!this->_pathCache.empty() && this->_pathCache.back() == this->_position)
		this->_pathCache.pop_back();
	this->_cachedDebug = AStar::getDebugImage();
	this->_cachedDebugTexLoaded = false;
}

bool GameObject::_tileAvailableFor(Vector2i tile) const
{
	auto &grid = this->map->pathFindingGrid;

	assert_exp(tile.x >= 0 && tile.y >= 0);
	assert_exp(tile.x < static_cast<int>(grid.gridSize.x) && tile.y < static_cast<int>(grid.gridSize.y));

	auto attrs = grid.grid[tile.x + tile.y * grid.gridSize.x];

	if (!(attrs & this->_passFlags))
		return false;
	if (PFGRIDMASK_FREE_NB(attrs) != 0)
		return false;
	if (this->_stackable)
		return PFGRIDMASK_STACK_COUNT(attrs) < PATHFINDING_STACK_LIMIT;
	return PFGRIDMASK_STACK_COUNT(attrs) == 0;
}

GameObject *GameObject::_findBlockingUnit(Vector2i absPos, Vector2u size) const
{
	std::vector<GameObject *> objects;
	GameObject *fallback = nullptr;

	this->manager->getObjectsInArea(absPos, size, objects);
	for (auto *o : objects) {
		if (o == this)
			continue;
		if (o->canMove())
			return o;
		fallback = o;
	}
	return fallback;
}

CollisionResolution GameObject::_resolveGridCollision(bool ignoreSolid, const GameObject *ignoreObject)
{
	Vector2i nextTile = this->map->absToTileCoord(this->_pathCache.back());

	if (nextTile == this->_gridPos) {
		this->blockTimer = 0;
		return COLLISION_PROCEED;
	}
	if (this->_tileAvailableFor(nextTile)) {
		if (this->_stackable && !PFGRIDMASK_IS_SLOT_FREE(this->map->pathFindingGrid, nextTile.x, nextTile.y, this->_currentTilePosition)) {
			this->_oldTilePosition = this->_currentTilePosition;
			this->deinitGlobalGrid();
			this->_currentTilePosition = PFGRIDMASK_FIND_FREE_SLOT(this->map->pathFindingGrid, nextTile.x, nextTile.y);
			// FIXME: We need to mark the slot we chose as used so another unit doesn't try to take it
			assert_exp(this->_currentTilePosition);
		}
		this->blockTimer = 0;
		return COLLISION_PROCEED;
	}

	if (this->blockTimer != 0) {
		this->blockTimer++;
		if (this->blockTimer >= waitBlocked)
			this->blockTimer = 0;
		return COLLISION_WAIT;
	}

	std::vector<Vector2i> saved;

	if (ignoreSolid) {
		GameObject *blocker = this->_findBlockingUnit(this->map->topLeftCoordToAbs(nextTile), this->map->tileSize);

		if (!blocker || !blocker->canMove())
			return COLLISION_PROCEED;
		if (blocker->owner->state.team == this->owner->state.team) {
			if (blocker->moveAside(this)) {
				this->blockTimer = 1;
				return COLLISION_WAIT;
			}
		} else if (blocker->isCrushable() && this->_canCrush) {
			blocker->crushedBy(this);
			return COLLISION_PROCEED;
		}

		saved = this->_pathCache;
		this->_computeMovePath(this->_cachedDest, ignoreSolid, ignoreObject);
		if (!this->_pathCache.empty()) {
			Vector2i newNext = this->map->absToTileCoord(this->_pathCache.back());

			if (newNext == this->_gridPos || this->_tileAvailableFor(newNext))
				return COLLISION_PROCEED;
		}
		this->_pathCache = saved;
		this->blockTimer = 1;
		return COLLISION_WAIT;
	} else {
		saved = this->_pathCache;
		this->_computeMovePath(this->_cachedDest, ignoreSolid, ignoreObject);
		if (!this->_pathCache.empty()) {
			Vector2i newNext = this->map->absToTileCoord(this->_pathCache.back());

			if (newNext == this->_gridPos || this->_tileAvailableFor(newNext))
				return COLLISION_PROCEED;
		}
		this->_pathCache = saved;

		GameObject *blocker = this->_findBlockingUnit(this->map->topLeftCoordToAbs(nextTile), this->map->tileSize);

		if (blocker == nullptr || !blocker->canMove())
			return COLLISION_ABORT;
		if (blocker->owner->state.team == this->owner->state.team) {
			if (!blocker->moveAside(this))
				return COLLISION_ABORT;
			this->blockTimer = 1;
			return COLLISION_WAIT;
		} else if (blocker->isCrushable() && this->_canCrush) {
			blocker->crushedBy(this);
			return COLLISION_PROCEED;
		}
		return COLLISION_ABORT;
	}
}

CollisionResolution GameObject::_resolveFreeCollision(bool ignoreSolid, const GameObject *ignoreObject)
{
	auto stepPos = [this](Vector2f dest) -> Vector2i {
		Vector2f pos = this->_position;
		Vector2f diff = dest - pos;

		diff.normalize();
		diff *= this->_speed;
		pos += diff;

		Vector2f diff2 = dest - pos;

		if (pos == dest || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0))
			pos = dest;
		return pos.to<int>();
	};
	Vector2i nextPos = stepPos(this->_pathCache.back());

	// TODO: ignoreSolid
	if (this->manager->isAreaFree(nextPos, this->_size, {this->index}, this->_stackable)) {
		this->blockTimer = 0;
		return COLLISION_PROCEED;
	}

	if (this->blockTimer != 0) {
		this->blockTimer++;
		if (this->blockTimer >= waitBlocked)
			this->blockTimer = 0;
		return COLLISION_WAIT;
	}

	std::vector<Vector2i> saved = this->_pathCache;

	this->_computeMovePath(this->_cachedDest, ignoreSolid, ignoreObject);
	if (!this->_pathCache.empty()) {
		Vector2i newNext = stepPos(this->_pathCache.back());

		// TODO: ignoreSolid
		if (this->manager->isAreaFree(newNext, this->_size, {this->index}, this->_stackable))
			return COLLISION_PROCEED;
	}
	this->_pathCache = saved;

	GameObject *blocker = this->_findBlockingUnit(nextPos, this->_size);

	if (blocker == nullptr || !blocker->canMove() || !blocker->moveAside(this))
		return COLLISION_ABORT;
	this->blockTimer = 1;
	return COLLISION_WAIT;
}

bool GameObject::_updateMove(Vector2i destination, bool force, const GameObject *ignoreObject)
{
	if (!this->_pathCached) {
		this->_cachedDest = destination;
		this->_computeMovePath(destination, force, ignoreObject);
		this->_pathCached = true;
		if (this->_pathCache.empty())
			return true;
	}

	if (this->_pathCache.empty())
		return true;

	auto dest = this->_pathCache.back() + this->_getTileOffsetFromCenter();
	std::optional<float> a;

	while (this->_position == dest) {
		this->_pathCache.pop_back();
		if (this->_pathCache.empty())
			return true;
	}

	if (this->_boundToGrid) {
		auto d = this->map->absToTileCoord(dest + this->_size / 2);

		if (d != this->_gridPos)
			a = this->_gridPos.angle(d);
	} else
		a = this->_position.angle(dest);

	if (a) {
		unsigned angle = round((*a + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

		angle %= this->_anims.size();
		if (angle != this->_currentDir && !this->_updateTurn(angle))
			return false;
	}

	switch (this->_boundToGrid ? this->_resolveGridCollision(force, ignoreObject) : this->_resolveFreeCollision(force, ignoreObject)) {
	case COLLISION_ABORT:
		return true;
	case COLLISION_WAIT:
		return false;
	case COLLISION_PROCEED:
		break;
	}

	auto pos = this->_position;
	auto diff = dest - pos;

	//if (this->_stackable) {
	//	//if (std::abs(diff.x))
	//	//qddqzzqddzq
	//}
	this->_moving = true;
	//if (this->_boundToGrid) {
	//	diff.x = std::copysign(this->_speed, diff.x);
	//	diff.y = std::copysign(this->_speed, diff.y);
	//} else {
		diff.normalize();
		diff *= this->_speed;
	//}
	pos += diff;

	auto diff2 = dest - pos;

	if (pos == dest || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0)) {
		pos = dest;
		this->_pathCache.pop_back();
	}
	this->setPosition(pos);
	return false;
}

void GameObject::_harvestClosestResource()
{
	auto pos = this->_gridPos;

	if (this->_currentDir == 1 * this->_anims.size() / 8) {
		pos.x++;
		pos.y--;
	} else if (this->_currentDir == 3 * this->_anims.size() / 8) {
		pos.x++;
		pos.y++;
	} else if (this->_currentDir == 5 * this->_anims.size() / 8) {
		pos.x--;
		pos.y++;
	} else if (this->_currentDir == 7 * this->_anims.size() / 8) {
		pos.x--;
		pos.y--;
	} else if (this->_currentDir == 0 * this->_anims.size() / 8)
		pos.y--;
	else if (this->_currentDir == 2 * this->_anims.size() / 8)
		pos.x++;
	else if (this->_currentDir == 4 * this->_anims.size() / 8)
		pos.y++;
	else if (this->_currentDir == 6 * this->_anims.size() / 8)
		pos.x--;

	auto tile = this->manager->getFirstHarvestableTile(*this->map, pos);
	if (tile) return this->harvest(this->map->topLeftCoordToAbs(pos));
	tile = this->manager->getClosestHarvestableTile(*this->map, this->_gridPos);
	if (tile) return this->harvest(this->manager->getLoadedMap().grid.topLeftCoordToAbs(*tile));
}

void GameObject::_goToClosestResource()
{
	auto pos = this->_gridPos;

	if (this->_currentDir == 1 * this->_anims.size() / 8) {
		pos.x++;
		pos.y--;
	} else if (this->_currentDir == 3 * this->_anims.size() / 8) {
		pos.x++;
		pos.y++;
	} else if (this->_currentDir == 5 * this->_anims.size() / 8) {
		pos.x--;
		pos.y++;
	} else if (this->_currentDir == 7 * this->_anims.size() / 8) {
		pos.x--;
		pos.y--;
	} else if (this->_currentDir == 0 * this->_anims.size() / 8)
		pos.y--;
	else if (this->_currentDir == 2 * this->_anims.size() / 8)
		pos.x++;
	else if (this->_currentDir == 4 * this->_anims.size() / 8)
		pos.y++;
	else if (this->_currentDir == 6 * this->_anims.size() / 8)
		pos.x--;

	auto tile = this->manager->getFirstHarvestableTile(*this->map, pos);

	if (!tile) {
		tile = this->manager->getClosestHarvestableTile(*this->map, this->_gridPos);
		if (!tile)
			return;
		pos = this->manager->getLoadedMap().grid.topLeftCoordToAbs(*tile);
	} else
		pos = this->map->topLeftCoordToAbs(pos);
	this->_orderQueue.emplace_front(MoveCheckOrder{&GameObject::_goToClosestResource, pos});
	this->_orderQueue.emplace_front(MoveOrder{pos, false});
}

void GameObject::_dumpInClosestResourceDump()
{
	if (!this->_lastResourceDump)
		this->_lastResourceDump = this->manager->getClosestResourceDump(this->_acceptResources, this->owner->state.team, this->_position + this->_size / 2);
	this->dumpResourcesTo(nullptr);
}

void GameObject::_goToClosestResourceDump()
{
	if (!this->_lastResourceDump)
		this->_lastResourceDump = this->manager->getClosestResourceDump(this->_acceptResources, this->owner->state.team, this->_position + this->_size / 2);
	if (!this->_lastResourceDump)
		return;

	auto pos = this->_lastResourceDump->getPosition() + this->_lastResourceDump->getPoint();

	this->_orderQueue.emplace_front(MoveCheckOrder{&GameObject::_goToClosestResourceDump, pos});
	this->_orderQueue.emplace_front(MoveOrder{pos, false});
}

bool GameObject::_updateHarvest()
{
	if (this->harvestDelaying) {
		this->harvestTimer++;
		if (this->harvestTimer >= this->_harvestDelay) {
			this->harvestDelaying = false;
			this->harvestTimer = 0;
			if (this->cancel)
				return true;
			if (this->_lots.size() >= this->_resourceStorage)
				return this->_dumpInClosestResourceDump(), true;
			auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);
			if (tile)
				return false;
			this->_harvestClosestResource();
			return true;
		}
	} else {
		if (this->_lots.size() >= this->_resourceStorage)
			return this->_dumpInClosestResourceDump(), true;
		auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);
		if (!tile)
			return this->_harvestClosestResource(), true;
		if (this->harvestTimer == 0)
			this->_harvesting = true;
		this->harvestTimer++;
		if (this->harvestTimer >= this->_harvestTime) {
			auto &r = this->manager->getResource(*tile);
			std::unordered_map<std::string, unsigned> value;

			this->harvestDelaying = true;
			this->harvestTimer = 0;
			for (size_t i = 0; i < r.storage && this->_lots.size() < this->_resourceStorage; i++) {
				std::unordered_map<std::string, unsigned> diff;

				for (auto &v : r.value) {
					auto va = v.second * (i + 1) / r.storage;

					diff[v.first] = va - value[v.first];
					value[v.first] = va;
				}
				this->_lots.emplace_back(diff);
			}
			this->manager->consumeResource(*tile);
		}
	}
	return false;
}

bool GameObject::_updateDump(GameObject *place)
{
	if (this->_firstEvent) {
		if (this->_currentDir != this->_dump->dir) {
			this->_orderQueue.emplace_front(TurnOrder{this->_dump->dir});
			this->_orderQueue.emplace_front(DumpOrder{place});
			return true;
		}
		if (place == nullptr || place->isDead()) {
			this->_dumpInClosestResourceDump();
			return true;
		}
		if (this->_lots.empty()) {
			this->_harvestClosestResource();
			return true;
		}
		if (this->_position != (place->getPosition() + place->getPoint()).to<float>())
			return false;
		this->_dumpStart = true;
	} else if (this->_dumpStart) {
		if (this->_dumpStart && (*this->_dump->start)() && this->_dump->start->update()) {
			this->_dumping = true;
			this->_dumpStart = false;
		}
	} else if (this->_dumpStop) {
		if (this->_dumpStop && (*this->_dump->stop)() && this->_dump->stop->update()) {
			if (this->_lots.size() != 0)
				this->_dumpInClosestResourceDump();
			else
				this->_harvestClosestResource();
			this->_dumpStop = false;
			return true;
		}
	} else if (this->_dumping) {
		if (place == nullptr || place->isDead()) {
			this->_dumpStop = true;
			this->_dumping = false;
			return false;
		}
		if (this->dumpTimer == this->_dump->delay) {
			std::unordered_map<std::string, unsigned> value;
			// TODO: Handle other strategies
			Player::CreditStrategy strategy = Player::CREDIT_SPILL;
			auto &accept = place->getAcceptResources();

			for (auto &[type, content] : this->_lots.front().value) {
				if (std::ranges::find(accept, type) == accept.end()) {
					auto p = this->manager->convertResource(type, accept, content);
					value[p.first] += p.second;
				} else
					value[type] += content;
			}
			for (auto &[type, content] : value)
				place->owner->credit(type, content, strategy);
			this->_lots.pop_front();
			place->produced();
		}
		if (this->dumpTimer >= this->_dump->delay + this->_dump->delayAfter) {
			if (this->_lots.empty()) {
				this->_dumpStop = true;
				this->_dumping = false;
			} else
				this->dumpTimer = 0;
		} else
			this->dumpTimer++;
	}
	return false;
}

bool GameObject::_updateDeploy()
{
	assert_exp(this->_onDeploy);
	if (this->_onDeploy->spawnBuilding) {
		auto &s = *this->_onDeploy->spawnBuilding;

		if (this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building, this->_position)) {
			this->_solid = false;
			this->_disposable = true;

			GameObject *result = this->manager->spawnObject(this->owner->state.index, this->_position + s.offset, s.queue, s.building);

			result->damage((this->_maxHp - this->_hp) * result->getHealth() / this->_maxHp);
		} else
			this->owner->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300);
	}
	return true;
}

bool GameObject::_updateAttack(GameObject *obj)
{
	if (obj == nullptr || obj->isDead())
		return true;

	Vector2f myCenter = this->_position + this->_size / 2;
	Vector2f opCenter = obj->_position + obj->_size / 2;
	Vector2f diff = opCenter - myCenter;
	Vector2f diffAbs = diff.abs();
	const ObjectWeapon *weapon = this->_getBestWeaponFor(obj, UINT32_MAX, true);
	if (weapon == nullptr) {
		double realAngle = diff.angle();
		unsigned angle = round((realAngle + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

		angle %= this->_anims.size();
		if (angle != this->_currentDir)
			this->_updateTurn(angle);
		return false;
	}

	ObjectBarrel &barrel = this->_barrels[weapon->barrel];
	unsigned range = weapon->weapon->getRange();

	range *= range;
	if (range < diffAbs.magnitude2()) {
		if (this->_updateMove(opCenter, false, obj)) {
			// TODO: Handle when unit is blocked
			if (opCenter != this->_cachedDest)
				this->_pathCached = false;
		}
		return false;
	}
	if (!this->_updateFinishMovement())
		return false;

	double realAngle = diff.angle();
	unsigned angle = round((realAngle + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

	angle %= this->_anims.size();
	if (angle != this->_currentDir && !this->_updateTurn(angle))
		return false;

	double inaccuracy = weapon->weapon->getInaccuracy(diff.magnitude());
	std::uniform_real_distribution<> dist(0, inaccuracy);
	double offset = dist(game->gameRandom);
	uint64_t dangle = random_distrib(game->gameRandom, 0, 360);
	const MultiSound *sound = weapon->weapon->getSound();
	const Effect *effect = weapon->weapon->getEffect();
	const Effect *hitEffect = weapon->weapon->getHitEffect();
	Vector2f targetPos = opCenter;

	barrel.shootTimer = weapon->cooldown;
	targetPos.x += std::cos(dangle * M_PI / 180) * offset;
	targetPos.y += std::sin(dangle * M_PI / 180) * offset;
	if (sound)
		sound->play();
	switch (weapon->weapon->getType()) {
	case WEAPONTYPE_HITSCAN:
		if (effect)
			this->manager->spawnEffect(*effect, this->_position + this->_size / 2);
		if (hitEffect)
			this->manager->spawnEffect(*hitEffect, targetPos);
		this->manager->warheadAt(weapon->weapon->getDamage(), weapon->weapon->getWarhead(), targetPos, this->owner->state.index);
		break;
	case WEAPONTYPE_PROJECTILE:
		// TODO:
	default:
		assert_not_reached();
	}
	return false;
}

bool GameObject::_updateFinishMovement()
{
	if (!this->_boundToGrid || this->_pathCache.empty())
		return true;

	auto center = this->_getTileOffsetFromCenter();
	Vector2f pos = this->_position - center - this->map->tileSize / 2;
	float x = std::fmod(pos.x, this->map->tileSize.x);
	float y = std::fmod(pos.y, this->map->tileSize.y);

	if (x == 0 && y == 0)
		return true;
	this->_pathCache.erase(this->_pathCache.begin() + 1, this->_pathCache.end());

	auto p = this->_pathCache.back() - this->map->tileSize / 2;
	Vector2f diff = p - pos;

	// TODO: Handle non-square grids
	diff.x = std::fmod(diff.x, this->map->tileSize.x);
	diff.y = std::fmod(diff.y, this->map->tileSize.y);
	this->_cachedDest = pos + diff;
	this->_pathCache.front() = this->_cachedDest + this->map->tileSize / 2;
	return this->_updateMove(this->_cachedDest, false, nullptr);
}

bool GameObject::_handleOrder(const Order &o)
{
	if (auto turn = std::get_if<TurnOrder>(&o))
		return this->_updateTurn(turn->resultDir);
	if (auto move = std::get_if<MoveOrder>(&o))
		return this->_updateMove(move->destination, move->force, nullptr);
	if (auto cmove = std::get_if<MoveCheckOrder>(&o))
		return this->_updateCheckMove(cmove->destination, cmove->fail);
	if (std::holds_alternative<HarvestOrder>(o))
		return this->_updateHarvest();
	if (auto dump = std::get_if<DumpOrder>(&o))
		return this->_updateDump(dump->target);
	if (std::holds_alternative<DeployOrder>(this->_orderQueue.front()))
		return this->_updateDeploy();
	if (auto attack = std::get_if<AttackUnitOrder>(&o))
		return this->_updateAttack(attack->target);
	if (std::holds_alternative<FinishMovementOrder>(o))
		return this->_updateFinishMovement();
	return true;
}

void GameObject::playBuiltSound() const
{
	if (this->_buildSound)
		this->_buildSound->play();
}

void GameObject::update()
{
	if (this->_deathAnim) {
		if (this->_animFinished || !this->_deathAnim->anim || this->_deathAnim->anim->update())
			this->_animFinished = true;
		if (this->_deathAnim->stayTime && this->_gpTimer[3] < *this->_deathAnim->stayTime)
			this->_gpTimer[3]++;
		else if (this->_deathAnim->fadeTime && this->_gpTimer[2] < *this->_deathAnim->fadeTime) {
			this->_gpTimer[2]++;
			this->_alpha = static_cast<float>(*this->_deathAnim->fadeTime - this->_gpTimer[2]) / *this->_deathAnim->fadeTime;
		} else if (this->_animFinished)
			this->_disposable = true;
		return;
	}
	if (this->_repairing) {
		if (this->_repairAnim && (*this->_repairAnim)())
			this->_repairAnim->update();
		if (this->_repairTimer == 0) {
			this->heal(this->_repairHeal);
			for (auto &[name, count] : this->_repairCost)
				this->owner->consume(name, count);
			if (this->_hp >= this->_maxHp)
				this->_repairing = false;
			else
				this->_repairTimer++;
		} else
			this->_repairTimer++;
		if (this->_repairTimer >= this->_repairDelay)
			this->_repairTimer = 0;
	}
	this->_moving = false;
	for (auto &barrel : this->_barrels)
		if (barrel.shootTimer)
			barrel.shootTimer--;
	if (!this->_orderQueue.empty()) {
		if (this->_firstEvent) {
			this->_pathCached = false;
			this->_gpTimer[0] = 0;
			this->_gpTimer[1] = 0;
			this->_gpTimer[2] = 0;
			this->_gpTimer[3] = 0;
		}
		if (this->_handleOrder(this->_orderQueue.front())) {
			this->_orderQueue.pop_front();
			this->_firstEvent = true;
		} else
			this->_firstEvent = false;
	}
	if (!this->_moving && !this->_dumpStop && !this->_dumping && !this->_dumpStart && !this->_idleAnims.empty()) {
		if (this->_currentIdle) {
			if (this->_idleAnims[this->_currentIdle - 1].update())
				this->_currentIdle = 0;
		} else {
			if (this->_idleTimer != 0)
				this->_idleTimer--;
			if (this->_idleTimer == 0) {
				this->_idleTimer = random_distrib(game->gameRandom, this->_idleTimerRange.first, this->_idleTimerRange.second);
				this->_currentIdle = random_distrib(game->gameRandom, 1, this->_idleAnims.size() + 1);
			}
		}
	} else
		this->_currentIdle = 0;
	if (this->_built) {
		if (!this->_buildAnim || !(*this->_buildAnim)() || this->_buildAnim->update())
			this->_built = false;
	} else if (this->_sold) {
		assert_exp(this->_sellAnim);
		if (this->_sellAnim->current() == this->_showBeforeSell && this->_sellTimer == 0) {
			std::pair<Vector2f, std::vector<Vector2f>> offsets[] = {
				{{1, 1}, { {-1,  1}, { 0,  1}, { 1,  1} }},
				{{0, 0}, { {-1,  0}, {-1, -1}, { 0, -1} }},
				{{2, 0}, { { 0, -1}, { 1, -1}, { 1,  0} }},
				{{1, 0}, { {-1, -1}, { 0, -1}, { 1, -1} }},
				{{0, 1}, { {-1, -1}, {-1,  0}, {-1,  1} }},
				{{2, 1}, { { 1, -1}, { 1,  0}, { 1,  1} }},
				{{1, 2}, { {-1,  1}, { 0,  1}, { 1,  1} }},
				{{0, 2}, { {-1,  0}, {-1,  1}, { 0,  1} }},
				{{2, 2}, { { 1,  0}, { 1,  1}, { 0,  1} }},
			};
			size_t i = 0;

			this->_sellTimer++;
			for (auto &[name, nb] : this->_sellUnits) {
				while (nb--) {
					assert_exp(i < std::size(offsets));
					auto pos = this->_gridPos + offsets[i].first;
					auto obj = this->manager->spawnObject(this->owner->state.index, this->map->topLeftCoordToAbs(pos), "", name);
					auto out = pos + offsets[i].second[game->gameRandom() % offsets[i].second.size()];

					obj->moveTo(this->map->topLeftCoordToAbs(out), true);
					i++;
				}
			}
		}
		if ((*this->_sellAnim)() && this->_sellAnim->update()) {
			for (auto &[fundType, value] : this->_sellRefund)
				this->owner->credit(fundType, value, Player::CREDIT_SYSTEM);
			this->_disposable = true;
			this->_solid = false;
			return;
		}
	} else {
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)())
			this->_producingAnim->update();
		if (this->_produced  && this->_producedAnim  && (*this->_producedAnim)()  && this->_producedAnim->update())
			this->_produced = false;
	}
	if (this->_lastResourceDump && this->_lastResourceDump->isDisposable())
		this->_lastResourceDump = nullptr;
	if (this->_asideCooldown != 0)
		this->_asideCooldown--;
	if (!this->_dumpStop && !this->_dumpStart && !this->_dumping && !this->_currentIdle) {
		if (this->_moving && !this->_moveAnim.empty()) {
			if (this->_moveAnim[this->_currentDir]())
				this->_moveAnim[this->_currentDir].update();
		} else for (auto &anim: this->_anims[this->_currentDir])
			if (anim())
				anim.update();
	}
	for (auto &o : this->_orderQueue) {
		if (auto dump = std::get_if<DumpOrder>(&o)) {
			if (dump->target->isDisposable())
				dump->target = nullptr;
		} else if (auto attack = std::get_if<AttackUnitOrder>(&o))
			if (attack->target->isDisposable())
				attack->target = nullptr;
	}
	if (this->_dumping && (*this->_dump->during)())
		this->_dump->during->update();
	if (!this->_harvestAnims.empty() && this->_harvesting) {
		auto dir = this->_currentDir * this->_harvestAnims.size() / this->_anims.size();
		auto &anim = this->_harvestAnims[dir];

		if (!anim() || anim.update())
			this->_harvesting = false;
	}
}

void GameObject::drawSkirt() const
{
	if (this->_skirt.size.x == 0 || this->_skirt.size.y == 0)
		return;

	auto size = Vector2u(this->_skirt.size.x, this->_skirt.size.y);
	auto &lmap = this->manager->getLoadedMap();
	auto &skirts = lmap.metadata.skirts;
	auto &skirtsC = lmap.metadata.skirtsConcrete;
	auto it = skirts.find(size);
	auto itC = skirtsC.find(size);
	auto textureSize = lmap.tileSet.getSize();
	sf::Sprite sprite{lmap.tileSet};

	textureSize.x /= lmap.grid.tileSize.x;
	assert_exp(it != skirts.end());
	assert_exp(it != skirtsC.end());
	for (size_t x = 0; x < size.x; x++) {
		for (size_t y = 0; y < size.y; y++) {
			Vector2f pos = {
				this->_position.x + this->_skirt.position.x + x * this->map->tileSize.x,
				this->_position.y + this->_skirt.position.y + y * this->map->tileSize.y
			};
			MapState::Quad abs{
				{pos.x,                         pos.y},
				{pos.x + this->map->tileSize.x, pos.y},
				{pos.x + this->map->tileSize.x, pos.y + this->map->tileSize.y},
				{pos.x,                         pos.y + this->map->tileSize.y}
			};
			auto tIndex = lmap.grid.anyHasAnyGridBits(abs, GRIDMASK_HAS_CONCRETE) ?
				itC->second[x + y * size.x] :
				it->second[x + y * size.x];

			sprite.setPosition(pos);
			sprite.setTextureRect({
				sf::Vector2i(
					tIndex % textureSize.x * lmap.grid.tileSize.x,
					tIndex / textureSize.x * lmap.grid.tileSize.y
				),
				sf::Vector2i(lmap.grid.tileSize)
			});
			game->screen->displayElement(sprite);
		}
	}
}

void GameObject::render(int layer)
{
	if (this->_repairing && this->_repairAnim && (*this->_repairAnim)())
		this->_displayAnim(this->_repairAnim->getAnim(), this->_size / 2);
	if (this->_deathAnim) {
		if (this->_deathAnim->anim && !this->_animFinished) {
			if ((*this->_deathAnim->anim)() && this->_deathAnim->anim->getLayer() == layer)
				this->_displayAnim(this->_deathAnim->anim->getAnim());
			return;
		}
		if (this->_deathAnim->stayTime && this->_gpTimer[3] >= *this->_deathAnim->stayTime)
			return;
		if (this->_deathAnim->anim)
			return;
	}
	if (layer == -1)
		this->drawSkirt();

	bool display = true;

	if (this->_built && this->_buildAnim && this->_buildAnim->current() < this->_showAfterBuild)
		display = false;
	if (this->_sold && this->_sellAnim && this->_sellAnim->current() >= this->_showBeforeSell)
		display = false;
	if (display) {
		if (!this->_dumpStop && !this->_dumpStart && !this->_dumping) {
			if (this->_moving && !this->_moveAnim.empty()) {
				auto &anim = this->_moveAnim[this->_currentDir];

				if (anim() && anim.getLayer() == layer)
					this->_displayAnim(anim.getAnim());
			} else if (this->_currentIdle) {
				auto &anim = this->_idleAnims[this->_currentIdle - 1];

				if (anim() && anim.getLayer() == layer)
					this->_displayAnim(anim.getAnim());
			} else for (auto &anim: this->_anims[this->_currentDir])
				if (anim() && anim.getLayer() == layer)
					this->_displayAnim(anim.getAnim());
		}
		if (this->_dumping && (*this->_dump->during)() && this->_dump->during->getLayer() == layer)
			this->_displayAnim(this->_dump->during->getAnim());
		if (this->_dumpStop && (*this->_dump->stop)() && this->_dump->stop->getLayer() == layer)
			this->_displayAnim(this->_dump->stop->getAnim());
		if (this->_dumpStart && (*this->_dump->start)() && this->_dump->start->getLayer() == layer)
			this->_displayAnim(this->_dump->start->getAnim());
		if (!this->_harvestAnims.empty() && this->_harvesting) {
			auto dir = this->_currentDir * this->_harvestAnims.size() / this->_anims.size();
			auto &anim = this->_harvestAnims[dir];

			if (anim() && anim.getLayer() == layer)
				this->_displayAnim(anim.getAnim());
		}
	}

	if (this->_built) {
		if (this->_buildAnim && (*this->_buildAnim)() && layer == this->_buildAnim->getLayer())
			this->_displayAnim(this->_buildAnim->getAnim());
	} else if (this->_sold) {
		if (this->_sellAnim && (*this->_sellAnim)() && layer == this->_sellAnim->getLayer())
			this->_displayAnim(this->_sellAnim->getAnim());
	} else {
		if (this->_produced &&  this->_producedAnim && (*this->_producedAnim)() && layer == this->_producedAnim->getLayer())
			this->_displayAnim(this->_producedAnim->getAnim());
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)() && layer == this->_producingAnim->getLayer())
			this->_displayAnim(this->_producingAnim->getAnim());
	}
}

void GameObject::setPosition(const Vector2f &pos)
{
	// OPTIMIZE: Diff instead of undoing it and redoing it
	this->manager->deregisterObject(this);
	this->deinitGlobalGrid();
	this->_position = pos;
	if (this->_tiles)
		this->_gridPos = this->map->absToTileCoord(pos);
	else if (this->_stackable)
		this->_gridPos = this->map->absToTileCoord(pos - this->_getTileOffsetFromCenter());
	else
		this->_gridPos = this->map->absToTileCoord(pos + this->_size / 2);
	this->initGlobalGrid();
	this->manager->registerObject(this);
}

void GameObject::setColor(const HSLColor &color)
{
	for (auto &block : this->_anims)
		for (auto &anim : block)
			anim.setColor(color);

	for (auto &anim : this->_idleAnims)
		anim.setColor(color);
	for (auto &anim : this->_harvestAnims)
		anim.setColor(color);
	for (auto &anim : this->_moveAnim)
		anim.setColor(color);
	for (auto &anim : this->_fireAnim)
		anim.setColor(color);
	for (auto &anim : this->_crouchAnim)
		anim.setColor(color);
	for (auto &anim : this->_crawlAnim)
		anim.setColor(color);
	for (auto &anim : this->_crawlFireAnim)
		anim.setColor(color);
	if (this->_dump) {
		this->_dump->during->setColor(color);
		this->_dump->start->setColor(color);
		this->_dump->stop->setColor(color);
	}
	if (this->_buildAnim)
		this->_buildAnim->setColor(color);
	if (this->_producingAnim)
		this->_producingAnim->setColor(color);
	if (this->_producedAnim)
		this->_producedAnim->setColor(color);
}

const std::string &GameObject::getId() const
{
	return this->_id;
}

unsigned GameObject::getHealth() const
{
	return this->_hp;
}

unsigned GameObject::getMaxHealth() const
{
	return this->_maxHp;
}

void GameObject::heal(unsigned int amount)
{
	this->setHealth(this->_hp + amount);
}

void GameObject::damage(unsigned amount, DamageType type)
{
	if (this->isDead())
		return;
	this->setHealth(std::max<int>(this->_hp - amount, 0));
	if (this->_resourceStorage != 0)
		this->owner->playEVA(UI::EVA_HARV_UNDER_ATTACK, 300, 3, 600, this->_id);
	if (!this->isDead())
		return;
	if (this->_deathAnims.empty() || type == DMGTYPE_VOID) {
		this->_disposable = true;
		return;
	}
	this->deinitGlobalGrid();
	this->_solid = false;
	if (type == DMGTYPE_GENERIC)
		type = static_cast<DamageType>(random_distrib(game->gameRandom, DMGTYPE_START, DMGTYPE_COUNT - 1));
	this->_deathAnim = &this->_deathAnims[type - DMGTYPE_START];
	this->_gpTimer[2] = 0;
	this->_gpTimer[3] = 0;
	if (this->_deathAnim->effect)
		this->manager->spawnEffect(*this->_deathAnim->effect, this->_position + this->_size / 2);
}

void GameObject::setHealth(unsigned health)
{
	if (this->_lowerPowerWhenDamaged)
		this->owner->state.producedPower += this->getPower();
	if (health == 0) {
		if (this->_deathSound)
			this->_deathSound->play();
		this->owner->playEVA(UI::EVA_UNIT_LOST, 300, 3, 0, this->_id);
	}
	this->_hp = std::min(health, this->_maxHp);
	if (this->_lowerPowerWhenDamaged)
		this->owner->state.producedPower -= this->getPower();
	this->animationState.health = static_cast<float>(this->_hp) / static_cast<float>(this->_maxHp);
}

bool GameObject::isDead() const
{
	return this->_hp == 0 || this->_disposable;
}

bool GameObject::isDisposable() const
{
	return this->_disposable;
}

const IntRect &GameObject::getSelectBox() const
{
	return this->_selectBox;
}

const Vector2u &GameObject::getSize() const
{
	return this->_size;
}

Vector2i GameObject::getPosition() const
{
	return Vector2i(round(this->_position.x), round(this->_position.y));
}

const std::optional<Vector2i> &GameObject::getSpawnOffset() const
{
	return this->_spawnOffset;
}

const std::optional<Vector2i> &GameObject::getExitOffset() const
{
	return this->_exitOffset;
}

const std::vector<ConditionalAnim> &GameObject::getBaseAnims() const
{
	return this->_anims[this->_currentDir];
}

const ReqAndCombinator &GameObject::getRequirements() const
{
	return *this->_requirements;
}

const Mod &GameObject::getBase() const
{
	return this->_base.get().getBase();
}

const Faction &GameObject::getFaction() const
{
	return this->_base;
}

unsigned GameObject::getBuildSpeed() const
{
	return this->_buildSpeed;
}

const std::string &GameObject::getGroup() const
{
	return this->_group;
}

const std::string &GameObject::getArmor() const
{
	return this->_armor;
}

bool GameObject::isObstructed() const
{
	auto spawn = this->getSpawnOffset();

	if (spawn)
		return !this->manager->isAreaFree(
			*spawn + this->getPosition(),
			this->map->tileSize,
			{this->index}
		);
	return false;
}

unsigned GameObject::getLimit() const
{
	return this->_limit;
}

unsigned GameObject::getPassFlags() const
{
	return this->_passFlags;
}

const std::unordered_map<std::string, unsigned> &GameObject::getCost() const
{
	return this->_cost;
}

bool GameObject::isDeployable() const
{
	if (this->_onDeploy)
		return true;
	return false;
}

void GameObject::select() const
{
	// TODO: Avoid overlaps
	if (this->_selectSound)
		this->_selectSound->play();
}

void GameObject::deploy()
{
	if (this->_onDeploy) {
		if (this->_onDeploy->direction)
			this->_orderQueue.emplace_back(TurnOrder{*this->_onDeploy->direction});
		this->_orderQueue.emplace_back(DeployOrder{});
	}
	if (this->_doSound)
		this->_doSound->play();
}

bool GameObject::canDeploy() const
{
	if (this->_onDeploy) {
		if (this->_onDeploy->spawnBuilding) {
			auto &s = *this->_onDeploy->spawnBuilding;

			return this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building, this->_position);
		}
	}
	return false;
}

bool GameObject::canMove() const
{
	return this->_speed != 0;
}

void GameObject::harvest(const Vector2i &pos, bool automatic)
{
	this->moveTo(pos, automatic);
	this->_orderQueue.emplace_back(MoveCheckOrder{&GameObject::_goToClosestResource, pos});
	this->_orderQueue.emplace_back(HarvestOrder{});
}

void GameObject::harvest()
{
	this->_harvestClosestResource();
}

void GameObject::attack(GameObject *obj)
{
	this->_orderQueue.emplace_back(AttackUnitOrder{obj});
}

void GameObject::moveTo(const Vector2i &pos, bool automatic)
{
	assert_exp(this->canMove());
	if (this->_boundToGrid)
		this->_orderQueue.emplace_back(MoveOrder{this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos)), automatic});
	else
		this->_orderQueue.emplace_back(MoveOrder{pos + this->_size / 2, automatic});
}

bool GameObject::moveAside(GameObject *asker)
{
	assert_exp(this->canMove());
	if (this->_dumpStart || this->_dumping || this->_dumpStop || this->_harvesting)
		return false;
	if (this->_asideCooldown != 0)
		return true;

	static const Vector2i dirs[8] = {
		{ 0, -1}, { 1, -1}, { 1,  0}, { 1,  1},
		{ 0,  1}, {-1,  1}, {-1,  0}, {-1, -1}
	};
	// Prefer moving away from whoever asked us to move (compare centers, which
	// are valid regardless of whether either unit is grid-bound).
	Vector2f selfCenter = this->_position + this->_size / 2;
	Vector2f away = asker ? (selfCenter - (asker->_position + asker->_size / 2)) : Vector2f{0, 0};
	Vector2i destination;
	float bestScore = 0;
	bool found = false;

	if (this->_boundToGrid) {
		auto &grid = this->map->pathFindingGrid;

		for (auto &d : dirs) {
			Vector2i tile = this->_gridPos + d;

			if (tile.x < 0 || tile.y < 0)
				continue;
			if (tile.x >= static_cast<int>(grid.gridSize.x) || tile.y >= static_cast<int>(grid.gridSize.y))
				continue;
			if (!this->_tileAvailableFor(tile))
				continue;

			float score = d.x * away.x + d.y * away.y;

			if (!found || score > bestScore) {
				found = true;
				destination = this->map->topLeftCoordToAbs(tile);
				bestScore = score;
			}
		}
	} else {
		Vector2i mapPixels = {
			static_cast<int>(this->map->gridSize.x * this->map->tileSize.x),
			static_cast<int>(this->map->gridSize.y * this->map->tileSize.y)
		};

		for (auto &d : dirs) {
			Vector2i candidate = this->_position + Vector2f{
				static_cast<float>(d.x) * this->_size.x,
				static_cast<float>(d.y) * this->_size.y
			};

			if (candidate.x < 0 || candidate.y < 0)
				continue;
			if (candidate.x + static_cast<int>(this->_size.x) > mapPixels.x)
				continue;
			if (candidate.y + static_cast<int>(this->_size.y) > mapPixels.y)
				continue;
			if (!this->manager->isAreaFree(candidate, this->_size, {this->index}, this->_stackable))
				continue;

			MapState::Quad footprint = {
				candidate.to<float>(),
				{static_cast<float>(candidate.x + this->_size.x), static_cast<float>(candidate.y)},
				{static_cast<float>(candidate.x + this->_size.x), static_cast<float>(candidate.y + this->_size.y)},
				{static_cast<float>(candidate.x), static_cast<float>(candidate.y + this->_size.y)}
			};

			if (!this->map->allHasAnyGridBits(footprint, this->_passFlags))
				continue;

			float score = d.x * away.x + d.y * away.y;

			if (!found || score > bestScore) {
				found = true;
				destination = candidate;
				bestScore = score;
			}
		}
	}
	if (!found)
		return false;

	this->_orderQueue.emplace_front(MoveOrder{destination, false});
	this->_firstEvent = true;
	this->_pathCached = false;
	this->_asideCooldown = waitBlocked;
	return true;
}

void GameObject::dumpResourcesTo(GameObject *dump, bool automatic)
{
	if (!dump)
		dump = this->_lastResourceDump;
	else
		this->_lastResourceDump = dump;
	if (!dump)
		return;

	auto pos = dump->getPosition() + dump->getPoint();

	this->moveTo(pos, automatic);
	this->_orderQueue.emplace_back(MoveCheckOrder{&GameObject::_goToClosestResourceDump, pos});
	this->_orderQueue.emplace_back(DumpOrder{dump});
}

void GameObject::clearOrders()
{
	if (!this->_orderQueue.empty()) {
		if (this->_shortenOrder(this->_orderQueue.front())) {
			this->_orderQueue.clear();
			this->_firstEvent = true;
		} else
			this->_orderQueue.erase(this->_orderQueue.begin() + 1, this->_orderQueue.end());
	}
}

void GameObject::playDoSound()
{
	if (this->_doSound)
		this->_doSound->play();
}

void GameObject::clearPrimaryDump()
{
	this->_lastResourceDump = nullptr;
}

void GameObject::initGlobalGrid()
{
	if (this->_initGrid)
		return;

	if (this->_tiles) {
		this->_decayTarget = this->_maxHp * (this->manager->initUnitTiles(*this->_tiles, this->_gridPos, *this->map) + 1) / 2;
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!line[gx].placementRequired && !line[gx].placementConcrete)
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					this->owner->state.index, 1,
					this->_currentTilePosition,
					line[gx].solid ? 1 : 0,
					!this->canMove(),
					this->isCrushable()
				);
			}
		}
	} else {
		MapState::Quad abs = {
			this->_position,
			{this->_position.x + this->_size.x, this->_position.y},
			{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
			{this->_position.x, this->_position.y + this->_size.y},
		};

		if (this->_boundToGrid)
			this->manager->modUnitInArea(
				this->map->topLeftCoordToAbs(this->_gridPos),
				this->map->tileSize,
				this->owner->state.index, 0,
				this->_currentTilePosition,
				this->_solid,
				!this->canMove(),
				this->isCrushable()
			);
		else
			this->manager->modUnitInArea(
				abs,
				this->owner->state.index, 0,
			        this->_currentTilePosition,
				this->_solid,
				!this->canMove(),
				this->isCrushable()
			);
	}
	this->_initGrid = true;
}

void GameObject::deinitGlobalGrid()
{
	if (!this->_initGrid)
		return;
	if (this->_tiles) {
		this->manager->deinitUnitTiles(*this->_tiles, this->_gridPos, *this->map);
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!line[gx].placementRequired && !line[gx].placementConcrete)
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					this->owner->state.index, -1,
					0, // TODO: Handle mask
					line[gx].solid ? -1 : 0,
					!this->canMove(),
					this->isCrushable()
				);
			}
		}
	} else {
		MapState::Quad abs = {
			this->_position,
			{this->_position.x + this->_size.x, this->_position.y},
			{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
			{this->_position.x, this->_position.y + this->_size.y},
		};

		if (this->_boundToGrid)
			this->manager->modUnitInArea(
				this->map->topLeftCoordToAbs(this->_gridPos),
				this->map->tileSize,
				this->owner->state.index, 0,
				this->_stackable ? static_cast<pfGridType>(this->_currentTilePosition) : 0,
				-1,
				!this->canMove(),
				this->isCrushable()
			);
		else
			this->manager->modUnitInArea(
				abs,
				this->owner->state.index, 0,
				0, // TODO: Handle mask
				-1,
				!this->canMove(),
				this->isCrushable()
			);
	}
	this->_initGrid = false;
}

std::optional<float> GameObject::getStorageDisplay() const
{
	if (this->_resourceStorage != 0)
		return static_cast<float>(this->_lots.size()) / static_cast<float>(this->_resourceStorage);
	if (this->_storage.empty())
		return {};

	float percent = 0;

	for (auto &key: this->_storage | std::ranges::views::keys)
		percent += static_cast<float>(this->owner->state.money[key]) / this->owner->state.totalStorage[key];
	percent /= this->_storage.size();
	return percent;
}

void GameObject::crushedBy(GameObject *)
{
	this->deinitGlobalGrid();
	this->setHealth(0);
	this->_crushSound->play();
	this->_deathAnim = &*this->_crushAnim;
	this->_gpTimer[2] = 0;
	this->_gpTimer[3] = 0;
	this->_solid = false;
}

bool GameObject::isSolid() const
{
	return this->_solid;
}

bool GameObject::isStackable() const
{
	return this->_stackable;
}

const std::optional<ExtraSpawn> &GameObject::getExtraSpawn() const
{
	return this->_extraSpawn;
}

const Vector2i &GameObject::getPoint() const
{
	return this->_point;
}

const std::vector<std::string> &GameObject::getSpawnOrders() const
{
	return this->_spawnOrders;
}

const std::vector<std::string> &GameObject::getAcceptResources() const
{
	return this->_acceptResources;
}

unsigned GameObject::getResourceStorage() const
{
	return this->_resourceStorage;
}

bool GameObject::isFake() const
{
	return this->_fake;
}

bool GameObject::requiresQueueBuilding() const
{
	return this->_requiresQueueBuilding;
}

bool GameObject::requiresPlacement() const
{
	return this->_requiresPlacement;
}

const std::optional<std::string> &GameObject::getProxy() const
{
	return this->_proxy;
}

const std::optional<std::vector<std::vector<BuildingMask>>> &GameObject::getTileData() const
{
	return this->_tiles;
}

unsigned GameObject::getBottom() const
{
	if (this->_tiles)
		return this->_position.y + this->map->tileSize.y * this->_tiles->size() - 1;
	return this->_position.y + this->_size.y - 1;
}

const std::set<Vector2i> &GameObject::getCheckTiles() const
{
	return this->_checkTiles;
}

const std::optional<std::string> &GameObject::getBuildQueue() const
{
	return this->_buildQueue;
}

void GameObject::setDirection(float angle)
{
	this->_currentDir = round((angle + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));
	this->_currentDir %= this->_anims.size();
}

void GameObject::renderPathFindingPath() const
{
	if (!this->_pathCached)
		return;

	sf::VertexArray arr{sf::PrimitiveType::LineStrip, this->_pathCache.size() + 1};
	sf::VertexArray arr2{sf::PrimitiveType::LineStrip, 2};
	auto off = this->_getTileOffsetFromCenter();

	arr[0].position = this->_position + this->_size / 2;
	arr[0].color = sf::Color::Blue;
	for (size_t i = this->_pathCache.size(); i > 0; i--) {
		arr[i].position = this->_pathCache[i - 1] + off + this->_size / 2;
		arr[i].color = sf::Color::Blue;
	}
	game->screen->draw(arr);

	arr2[0].position = this->_position + this->_size / 2;
	arr2[0].color = sf::Color::Green;
	arr2[1].position = this->_cachedDest + this->map->tileSize / 2;
	arr2[1].color = sf::Color::Green;
	game->screen->draw(arr2);

	if (this->_cachedDebug.getSize().x != 0 && this->_cachedDebug.getSize().y != 0) {
		if (!this->_cachedDebugTexLoaded) {
			assert_exp(this->_cachedDebugTex.loadFromImage(this->_cachedDebug));
			this->_cachedDebugTexLoaded = true;
		}

		sf::Sprite sprite{this->_cachedDebugTex};
		auto scale = this->pfGrid->gridSize;

		scale.x /= this->_cachedDebugTex.getSize().x;
		scale.y /= this->_cachedDebugTex.getSize().y;
		sprite.setPosition({0, 0});
		sprite.setScale(scale);
		game->screen->draw(sprite);
	}
}

bool GameObject::isCentral() const
{
	return this->_central;
}

void GameObject::built()
{
	// FIXME:
	auto old = this->_lowerPowerWhenDamaged;

	this->_lowerPowerWhenDamaged = false;
	this->setHealth(this->_decayTarget);
	this->_lowerPowerWhenDamaged = old;
	this->_built = true;
	if (this->_buildAnim)
		this->_buildAnim->reset();
}

void GameObject::produced()
{
	this->_produced = true;
	if (this->_producedAnim)
		this->_producedAnim->reset();
}

void GameObject::producing(bool producing)
{
	this->_producing = producing;
}

bool GameObject::isProducing() const
{
	return this->_producing;
}

int GameObject::getPower() const
{
	if (!this->_lowerPowerWhenDamaged)
		return this->_power;
	return this->_power * static_cast<int>(this->_hp) / static_cast<int>(this->_maxHp);
}

const std::unordered_map<std::string, unsigned> &GameObject::getStorage() const
{
	return this->_storage;
}

const std::optional<std::string> &GameObject::getGivenQueue() const
{
	return this->_giveQueue;
}

unsigned char GameObject::getBuildMaxDistance() const
{
	return this->_buildMaxDistance;
}

bool GameObject::isRadar() const
{
	return this->_isRadar;
}

bool GameObject::isCrushable() const
{
	return this->_crushSound != nullptr;
}

bool GameObject::isSelectable() const
{
	return this->_selectable;
}

bool GameObject::isSellable() const
{
	return this->_sellable && !this->_sold;
}

bool GameObject::isRepairable() const
{
	return this->_repairable;
}

bool GameObject::isPrimary() const
{
	return this->_isPrimary;
}

void GameObject::setPrimary(bool primary)
{
	this->_isPrimary = primary;
}

void GameObject::toggleRepair()
{
	this->_repairing = !this->_repairing;
}

void GameObject::sell()
{
	if (!this->isSellable())
		return;
	this->_sold = true;
	if (this->_sellAnim)
		this->_sellAnim->reset();
}

void GameObject::nextDirection()
{
	this->_currentDir = (this->_currentDir + 1) % this->_anims.size();
}

const ObjectWeapon *GameObject::_getBestWeaponFor(const GameObject *obj, unsigned barrel, bool isFree) const
{
	const std::vector<std::string> &tags = obj->getTags();

	for (const std::string &tag : tags) {
		auto it = this->_weaponByTag.find(tag);

		if (it == this->_weaponByTag.end())
			continue;

		for (auto &weapon : it->second) {
			if (barrel != UINT32_MAX && weapon->barrel != barrel)
				continue;
			if (isFree && this->_barrels[weapon->barrel].shootTimer != 0)
				continue;
			return weapon;
		}
	}
	return nullptr;
}

ObjectWeapon *GameObject::_getBestWeaponFor(const GameObject *obj, unsigned barrel, bool isFree)
{
	return const_cast<ObjectWeapon *>(static_cast<const GameObject *>(this)->_getBestWeaponFor(obj, barrel, isFree));
}

bool GameObject::canAttack(const GameObject *obj) const
{
	return this->_getBestWeaponFor(obj, UINT32_MAX, false) != nullptr;
}

const std::vector<std::string> &GameObject::getTags() const
{
	return this->_tags;
}

const std::optional<UnitDeployParams> &GameObject::getOnDeploy() const
{
	return this->_onDeploy;
}

unsigned GameObject::getSpeed() const
{
	return this->_speed;
}

ObjectBarrel::ObjectBarrel(unsigned index, const Mod &base, const std::unordered_map<std::string, Weapon> &wList, const nlohmann::json &j)
{
	assert_exp(j.contains("weapons") && j["weapons"].is_array());
	assert_exp(!j.contains("priority") || j["priority"].is_number());

	this->priority = (j.contains("priority") ? j["priority"].get<unsigned short>() : 0);
	for (auto &wj : j["weapons"])
		this->weapons.emplace_back(this->priority * UINT16_MAX, index, base, wList, wj);
	std::ranges::sort(this->weapons, [](const ObjectWeapon &a, const ObjectWeapon &b) -> bool {
		return a.priority < b.priority;
	});
}

void ObjectBarrel::update()
{
	if (this->shootTimer != 0) {
		this->shootTimer--;
		return;
	}
}

ObjectDeathParams::ObjectDeathParams(const Mod &base, const GameObject *obj, const std::unordered_map<std::string, std::vector<Animation>> &animMap, const std::unordered_map<std::string, Effect> &effectMap, const nlohmann::json &json)
{
	assert_exp(!json.contains("anim") || json["anim"].is_object());
	assert_exp(!json.contains("fade") || json["fade"].is_number_unsigned());
	assert_exp(!json.contains("stay") || json["stay"].is_number_unsigned());
	assert_exp(!json.contains("effect") || json["effect"].is_string());

	if (json.contains("anim"))
		this->anim.emplace(base, obj, animMap, json["anim"]);
	if (json.contains("fade"))
		this->fadeTime.emplace(json["fade"]);
	if (json.contains("stay"))
		this->stayTime.emplace(json["stay"]);
	if (json.contains("effect")) {
		std::string effect = RESOURCE(base.getName(), json["effect"]);

		assert_msg(effectMap.contains(effect), effect);
		this->effect = &effectMap.at(effect);
	}
}

ObjectDeathParams::ObjectDeathParams(const GameObject *obj, const ObjectDeathParams &other)
{
	if (other.anim)
		this->anim.emplace(obj, *other.anim);
	this->fadeTime = other.fadeTime;
	this->stayTime = other.stayTime;
	this->effect = other.effect;
}

ObjectWeapon::ObjectWeapon(unsigned short extraPrio, unsigned barrel, const Mod &base, const std::unordered_map<std::string, Weapon> &weapons, const nlohmann::json &j)
{
	assert_exp(j.contains("name") && j["name"].is_string());
	assert_exp(j.contains("cooldown") && j["cooldown"].is_number());
	assert_exp(!j.contains("priority") || j["priority"].is_number());

	std::string weaponName = RESOURCE(base.getName(), j["name"].get<std::string>());

	assert_exp(weapons.contains(weaponName));
	this->weapon = &weapons.at(weaponName);
	this->priority = (j.contains("priority") ? j["priority"].get<unsigned short>() : 0) + extraPrio;
	this->barrel = barrel;
	this->cooldown = j["cooldown"];
}

GameObjectData::GameObjectData(const Faction &base) :
	_base(base)
{
}

GameObjectData::GameObjectData(const GameObjectData &) = default;
