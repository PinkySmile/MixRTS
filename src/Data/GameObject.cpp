//
// Created by PinkySmile on 20/04/2026.
//

#include <cmath>
#include <sol/sol.hpp>
#include "Faction.hpp"
#include "Effect.hpp"
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "Animation.hpp"
#include "Player.hpp"
#include "ConditionalAnim.hpp"
#include "Weapon.hpp"
#include "Upgrade.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

#define currentSpeed _gpTimerF[0]
#define currentAngle _gpTimerF[0]

#define pickupOldAltitude _gpTimerI[7]

#define dumpTimer _gpTimerU[0]
#define harvestTimer _gpTimerU[0]
#define cancel _gpTimerU[1]
#define harvestDelaying _gpTimerU[2]
#define blockTimer _gpTimerU[3]
#define turnTimer _gpTimerU[4]
#define dropTimer _gpTimerU[12]
#define dropLast _gpTimerU[13]
#define currentDrop _gpTimerU[14]
#define pickupState _gpTimerU[15]
#define attackFirst _gpTimerU[15]

#define waitBlocked 60
#define PATH_UNIT_PENALTY 1000

MoveReason moveReasonFromString(const std::string &cmd)
{
	if (cmd == "move_command")
		return MOVEREASON_MOVE_COMMAND;
	if (cmd == "force_move_command")
		return MOVEREASON_FORCE_MOVE_COMMAND;
	if (cmd == "attack_command")
		return MOVEREASON_ATTACK_COMMAND;
	if (cmd == "force_attack_command")
		return MOVEREASON_FORCE_ATTACK_COMMAND;
	if (cmd == "harvest_command")
		return MOVEREASON_HARVEST_COMMAND;
	if (cmd == "harvest")
		return MOVEREASON_HARVEST;
	if (cmd == "dump_command")
		return MOVEREASON_DUMP_COMMAND;
	if (cmd == "dump")
		return MOVEREASON_DUMP;
	if (cmd == "auto_agro")
		return MOVEREASON_AUTO_AGRO;
	if (cmd == "repair")
		return MOVEREASON_REPAIR;
	if (cmd == "move_aside")
		return MOVEREASON_MOVE_ASIDE;
	if (cmd == "move_spawn")
		return MOVEREASON_MOVE_SPAWN;
	if (cmd == "finish_move")
		return MOVEREASON_FINISH_MOVE;
	assert_not_reached_msg("Invalid MoveReason \"" + cmd + "\"");
}

static Order parseBehavior(const nlohmann::json &json)
{
	assert_exp(json.contains("type") && json["type"].is_string());
	if (json["type"] == "spin") {
		assert_exp(json.contains("angle") && json["angle"].is_number());
		return SpinOrder{json["angle"]};
	}
	if (json["type"] == "pickup") {
		assert_exp(json.contains("for") && json["for"].is_array());
		assert_exp(!json.contains("dist") || json["dist"].is_number());
		assert_exp(!json.contains("extra") || json["extra"].is_object());

		WaitPickupOrder order;

		order.targets.reserve(json["for"].size());
		for (size_t i = 0; i < json["for"].size(); i++) {
			assert_msg(json["for"][i].is_string(), std::to_string(i));
			TRY {
				order.targets.push_back(moveReasonFromString(json["for"][i]));
			} CATCH_AND_RETHROW("In key \"for\"." + std::to_string(i));
		}
		if (json.contains("dist"))
			order.dist = json["dist"];
		else
			order.dist = 0;
		if (json.contains("extra"))
			TRY {
				order.other = std::make_unique<Order>(parseBehavior(json["extra"]));
			} CATCH_AND_RETHROW("In key \"extra\"");
		return order;
	}
	assert_not_reached_msg("Invalid \"type\": \"" + json["type"].get<std::string>() + "\"");
}

bool GameObject::_shortenOrder(Order &o)
{
	if (std::holds_alternative<FinishMovementOrder>(o))
		return false;
	if (std::holds_alternative<MoveOrder>(o) || std::holds_alternative<AttackUnitOrder>(o)) {
		this->_orderQueue.emplace_front(FinishMovementOrder{});
		return false;
	} else if (std::holds_alternative<HarvestOrder>(o) || std::holds_alternative<DumpOrder>(o)) {
		this->cancel = true;
		return false;
	}
	return true;
}

void GameObject::_displayAnim(Animation &anim, Vector2f offset) const
{
	Sprite &sprite = anim.getSprite();
	auto pos = sprite.getPosition();

	sprite.setPosition({
		pos.x + this->_position.x + offset.x,
		pos.y + this->_position.y + offset.y
	});
	if (this->_altitude != 0) {
		sprite.setColor(sf::Color{0, 0, 0, 0x80});
		using Factor = sf::BlendMode::Factor;
		using Equation = sf::BlendMode::Equation;
		switch (anim.getBlending()) {
		case Animation::BLEND_ADD:
			game->screen->draw(sprite, sf::BlendAdd);
			break;
		case Animation::BLEND_SUB:
			game->screen->draw(sprite, sf::BlendMode{
				Factor::SrcAlpha, Factor::One, Equation::ReverseSubtract,
				Factor::One, Factor::One, Equation::ReverseSubtract
			});
			break;
		case Animation::BLEND_ALPHA:
			game->screen->draw(sprite, sf::BlendAlpha);
			break;
		}
		sprite.setPosition({
			pos.x + this->_position.x + offset.x,
			pos.y + this->_position.y + offset.y - this->_altitude
		});
	}

	sprite.setColor(sf::Color::White);
	using Factor = sf::BlendMode::Factor;
	using Equation = sf::BlendMode::Equation;
	switch (anim.getBlending()) {
	case Animation::BLEND_ADD:
		game->screen->draw(sprite, sf::BlendAdd);
		break;
	case Animation::BLEND_SUB:
		game->screen->draw(sprite, sf::BlendMode{
			Factor::SrcAlpha, Factor::One, Equation::ReverseSubtract,
			Factor::One, Factor::One, Equation::ReverseSubtract
		});
		break;
	case Animation::BLEND_ALPHA:
		game->screen->draw(sprite, sf::BlendAlpha);
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
	for (auto &turret : g._turrets)
		this->_turrets.emplace_back(this, turret);
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
	assert_exp(!json.contains("behavior") || json["behavior"].is_object());
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
	assert_exp(json.contains("hp") && json["hp"].is_number());
	assert_exp(json.contains("cost") && json["cost"].is_object());
	for (auto &[key, value] : json["cost"].items())
		assert_msg(value.is_number(), "json[\"cost\"][\"" + key + "\"]");
	assert_exp(!json.contains("turrets") || json["turrets"].is_array());
	if (json.contains("turrets")) {
		for (size_t i = 0; i < json["turrets"].size(); i++)
			assert_msg(json["turrets"][i].is_object(), "json[\"turrets\"][\"" + std::to_string(i) + "\"]");
	}
	assert_exp(!json.contains("spawn_params") || json["spawn_params"].is_object());
	assert_exp(!json.contains("select") || json["select"].is_object());
	assert_exp(!json.contains("decel") || json["decel"].is_object());
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
	assert_exp(!json.contains("turn_delay") || json["turn_delay"].is_number());
	assert_exp(!json.contains("do_sound") || json["do_sound"].is_string());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());
	assert_exp(!json.contains("pass_flags") || json["pass_flags"].is_string() || json["pass_flags"].is_number_unsigned());
	assert_exp(!json.contains("fake") || json["fake"].is_boolean());
	assert_exp(!json.contains("wall") || json["wall"].is_boolean());
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
	assert_exp(!json.contains("altitude") || json["altitude"].is_number());
	assert_exp(!json.contains("can_crush") || json["can_crush"].is_boolean());
	assert_exp(!json.contains("radar") || json["radar"].is_boolean());
	assert_exp(!json.contains("move_while_turn") || json["move_while_turn"].is_boolean());
	assert_exp(!json.contains("repair_delay") || json["repair_delay"].is_number());
	assert_exp(!json.contains("repair_heal") || json["repair_heal"].is_number());
	assert_exp(!json.contains("repair_cost") || json["repair_cost"].is_object());
	if (json.contains("connected")) {
		assert_exp(json["connected"].is_object());
		assert_exp(json["connected"].contains("type") && json["connected"]["type"].is_string());
		assert_exp(json["connected"].contains("valid") && json["connected"]["valid"].is_array());
		for (size_t i = 0; i < json["connected"]["valid"].size(); i++)
			assert_msg(json["connected"]["valid"][i].is_number_unsigned(), "json[\"connected\"][\"valid\"][" + std::to_string(i) + "]");
	}
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
	assert_exp(!json.contains("muzzle") || json["muzzle"].is_string());
	assert_exp(!json.contains("idle_time") || json["idle_time"].is_array());
	assert_exp(!json.contains("spawn") || json["spawn"].is_object());
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
		assert_exp(!json["sell"].contains("anim") || json["sell"]["anim"].is_object());
		assert_exp(!json["sell"].contains("units") || json["sell"]["units"].is_object());
		assert_exp(!json["sell"].contains("refund") || json["sell"]["refund"].is_object());
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

	assert_exp(!json.contains("connected") || !json.contains("directions"));
	if (json.contains("directions")) {
		directions = json["directions"].get<unsigned>();
		assert_msg(*directions % 8 == 0, "Number of directions must be divisible by 8");
	}
	if (json.contains("spawn_params"))
		this->_spawnParams = { json["spawn_params"] };
	if (json.contains("altitude"))
		this->_altitude = json["altitude"];
	if (json.contains("lower_power_when_damaged"))
		this->_lowerPowerWhenDamaged = json["lower_power_when_damaged"];
	if (json.contains("build_max_distance"))
		this->_buildMaxDistance = json["build_max_distance"];
	if (json.contains("selectable"))
		this->_selectable = json["selectable"];
	if (json.contains("ignore_terrain"))
		this->_ignoreTerrain = json["ignore_terrain"];
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
	if (json.contains("wall"))
		this->_wall = json["wall"];
	if (json.contains("repair_delay"))
		this->_repairDelay = json["repair_delay"];
	if (json.contains("repair_heal"))
		this->_repairHeal = json["repair_heal"];
	if (json.contains("repair_cost"))
		this->_repairCost = json["repair_cost"];
	if (json.contains("radar"))
		this->_isRadar = json["radar"];
	if (json.contains("move_while_turn"))
		// TODO: Not actually implemented
		this->_moveWhileTurn = json["move_while_turn"];
	if (json.contains("spawn"))
		this->_extraSpawn = ExtraSpawn{base, json["spawn"]};
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
			this->_crushAnim.emplace(base, this, animList, effects, weapons, json["crush_anim"]);
		} CATCH_AND_RETHROW("In key \"crush_anim\"\n")
	if (json.contains("producing_anim") && !json["producing_anim"].is_null())
		this->_producingAnim.emplace(base, this, animList, json["producing_anim"]);
	if (json.contains("produced_anim") && !json["produced_anim"].is_null())
		this->_producedAnim.emplace(base, this, animList, json["produced_anim"]);
	if (json.contains("idle_time"))
		this->_idleTimerRange = {json["idle_time"][0], json["idle_time"][1]};
	if (json.contains("turn_delay"))
		this->_turnDelay = json["turn_delay"];
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
	if (json.contains("behavior"))
		TRY {
			this->_idleOrder.emplace(parseBehavior(json["behavior"]));
		} CATCH_AND_RETHROW("In key \"behavior\"");
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
	if (json.contains("decel")) {
		assert_exp(json["decel"].contains("dist") && json["decel"]["dist"].is_number());
		assert_exp(json["decel"].contains("step") && json["decel"]["step"].is_number());
		assert_exp(json["decel"].contains("min") && json["decel"]["min"].is_number());
		this->_decel.dist2 = json["decel"]["dist"];
		this->_decel.dist2 *= this->_decel.dist2;
		this->_decel.step = json["decel"]["step"];
		this->_decel.min = json["decel"]["min"];
	}
	if (json.contains("group"))
		this->_group = RESOURCE(base.getName(), "group." + json["group"].get<std::string>());
	else
		this->_group = id;
	if (json.contains("show_after_build"))
		this->_showAfterBuild = json["show_after_build"];
	else if (this->_buildAnim)
		this->_showAfterBuild = this->_buildAnim->size();

	if (json.contains("connected")) {
		this->_connectTo = json["connected"]["type"];
		this->_connectedSides = json["connected"]["valid"].get<std::vector<unsigned char>>();
		assert_exp(!this->_connectTo.empty());
		assert_exp(this->_connectedSides.size() <= 8 && !this->_connectedSides.empty());
		for (size_t i = 0; i < this->_connectedSides.size(); i++) {
			assert_msg(this->_connectedSides[i] != 0, "json[\"connected\"][\"valid\"][" + std::to_string(i) + "]");
			assert_msg(this->_connectedSides[i] != 5, "json[\"connected\"][\"valid\"][" + std::to_string(i) + "]");
			assert_msg(this->_connectedSides[i] <= 9, "json[\"connected\"][\"valid\"][" + std::to_string(i) + "]");
		}
	}
	if (json.contains("sell")) {
		if (json["sell"].contains("anim")) {
			this->_sellAnim.emplace(base, this, animList, json["sell"]["anim"]);
			this->_showBeforeSell = this->_buildAnim->size() - this->_showAfterBuild;
		}
		if (json["sell"].contains("refund"))
			this->_sellRefund = json["sell"]["refund"];
		if (json["sell"].contains("units")) {
			for (auto &[key, nb] : json["sell"]["units"].items()) {
				assert_msg(nb.is_number_unsigned(), "json[\"sell\"][\"units\"][\"" + key + "\"]");
				this->_sellUnits[RESOURCE(base.getName(), key)] = nb;
			}
		}
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
		if (json.contains("muzzle")) {
			std::string muzzle = RESOURCE(base.getName(), json["muzzle"]);

			for (size_t i = 0; i < *directions; i++) {
				std::string muzzleEffect = muzzle + "_" + std::to_string(i);

				assert_msg(effects.contains(muzzleEffect), muzzleEffect);
				this->_muzzleAnim.push_back(&effects.at(muzzleEffect));
			}
		}
	} else if (!this->_connectedSides.empty()) {
		assert_exp(!json.contains("anim_move"));
		assert_exp(!json.contains("anim_fire"));
		assert_exp(!json.contains("anim_crouch"));
		assert_exp(!json.contains("anim_crawl"));
		assert_exp(!json.contains("anim_crawl_fire"));
		this->_anims.resize(1 << this->_connectedSides.size());
		for (auto &anim: json["anims"])
			for (size_t i = 0; i < (1ULL << this->_connectedSides.size()); i++) {
				unsigned index = 0;

				for (size_t j = 0; j < this->_connectedSides.size(); j++)
					if (i & (1 << j)) {
						index *= 10;
						index += j + 1;
					}
				this->_anims[i].emplace_back(base, this, animList, anim, index);
			}
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
		if (json.contains("muzzle")) {
			std::string muzzleEffect = RESOURCE(base.getName(), json["muzzle"]);

			assert_msg(effects.contains(muzzleEffect), muzzleEffect);
			this->_muzzleAnim.push_back(&effects.at(muzzleEffect));
		}
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
				this->_deathAnims.emplace_back(base, this, animList, effects, weapons, json["death"].contains(s) ? json["death"][s] : json["death"]["generic"]);
			} CATCH_AND_RETHROW("In key \"death\".\"" + std::string(s) + "\"\n")
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
	if (json.contains("turrets")) {
		for (auto &turret : json["turrets"])
			this->_turrets.emplace_back(this, base, animList, turret);
	}

	if (!this->_anims.empty() && !this->_anims.front().empty()) {
		auto &anims = this->_anims.front().front().getAnims();

		if (!anims.empty()) {
			auto &anim = anims.front();

			this->_selectBox.position.x = 0;
			this->_selectBox.position.y = anim.getOffset().y;
			this->_selectBox.size = this->_size;
		}
	}
	if (json.contains("select")) {
		if (json["select"].contains("x")) {
			assert_exp(json["select"]["x"].is_number());
			this->_selectBox.position.x = json["select"]["x"];
		}
		if (json["select"].contains("y")) {
			assert_exp(json["select"]["y"].is_number());
			this->_selectBox.position.y = json["select"]["y"];
		}
		if (json["select"].contains("h")) {
			assert_exp(json["select"]["h"].is_number());
			this->_selectBox.size.x = json["select"]["h"];
		}
		if (json["select"].contains("w")) {
			assert_exp(json["select"]["w"].is_number());
			this->_selectBox.size.y = json["select"]["w"];
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
	assert_msg(!this->_moveWhileTurn || (!this->_boundToGrid && this->_ignoreTerrain), "move_while_turn only works if the unit isn't bound to grid, and ignores terrain");
}

GameObject::~GameObject() = default;


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

static bool PathNotObstructed(Vector2i, void *)
{
	return false;
}

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
		MapState::Quad absP = {
			Vector2f(p.x, p.y),
			Vector2f(p.x + 1, p.y),
			Vector2f(p.x + 1, p.y + 1),
			Vector2f(p.x, p.y + 1)
		};

		if (octx->ggrid)
			absP = octx->ggrid->tileCoordToAbs(p);

		Vector2f tl{absP.pt1.x, absP.pt1.y};
		Vector2f br{absP.pt3.x, absP.pt3.y};

		// TODO: Handle non-square grids
		if (octx->ignoreObject->isInObject(tl, br - tl, false))
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

static float GetTileCostFree(Vector2i, void *)
{
	return 0;
}

static float GetTileCostMovableUnits(Vector2i p, void *ctx)
{
	auto octx = static_cast<ObstructionContext *>(ctx);
	float cost = 0;

	for (size_t y = 0; y < octx->hitBoxSize.y; y++)
		for (size_t x = 0; x < octx->hitBoxSize.x; x++) {
			if (octx->ignoreObject) {
				MapState::Quad absP = {
					Vector2f(p.x + x, p.y + y),
					Vector2f(p.x + x + 1, p.y + y),
					Vector2f(p.x + x + 1, p.y + y + 1),
					Vector2f(p.x + x, p.y + y + 1)
				};

				if (octx->ggrid)
					absP = octx->ggrid->tileCoordToAbs(p);

				Vector2f tl{absP.pt1.x, absP.pt1.y};
				Vector2f br{absP.pt3.x, absP.pt3.y};

				// TODO: Handle non-square grids
				if (octx->ignoreObject->isInObject(tl, br - tl, false))
					return 0;
			}

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
	if (this->_tiles)
		return {0, 0};
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

void GameObject::_computeMovePath(Vector2i destination, bool ignoreSolid, const GameObject *ignoreObject)
{
	ObstructionContext ctx;
	Vector2i target;
	Vector2i pos;
	Vector2u step = {1, 1};
	IsObstructedCb isObstructed;
	GetTileCostCb getTileCost;

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
		target = destination - this->_size / 2;
		pos = this->_position;
	}
	if (!this->_boundToGrid && this->_ignoreTerrain) {
		this->_pathCache.clear();
		this->_pathCache.push_back(target);
	} else {
		if (this->_ignoreTerrain) {
			getTileCost = &GetTileCostFree;
			isObstructed = &PathNotObstructed;
		} else {
			getTileCost = &GetTileCostMovableUnits;
			if (ignoreSolid)
				isObstructed = &IsPathObstructedTerrain;
			else
				isObstructed = &IsPathObstructedStatic;
		}
		AStar::computePath(
			*ctx.grid,
			pos, target, step,
			isObstructed,
			getTileCost,
			&ctx,
			this->_pathCache
		);
		this->_cachedDebug = AStar::getDebugImage();
		this->_cachedDebugTexLoaded = false;
	}
	if (this->_boundToGrid)
		for (auto &p : this->_pathCache)
			p = this->map->topLeftCoordToAbs(p) + this->map->tileSize / 2;
	while (!this->_pathCache.empty() && this->_pathCache.back() == this->_position)
		this->_pathCache.pop_back();
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

GameObject *GameObject::_findBlockingUnit(Vector2i absPos, Vector2u size, const GameObject *ignoreObject) const
{
	std::vector<GameObject *> objects;
	GameObject *fallback = nullptr;

	this->manager->getObjectsInArea(absPos, size, objects);
	for (auto *o : objects) {
		if (o == this || o == ignoreObject || !o->isSolid())
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

	if (this->_altitude)
		return COLLISION_PROCEED;
	if (nextTile == this->_gridPos) {
		this->blockTimer = 0;
		return COLLISION_PROCEED;
	}
	if (this->_tileAvailableFor(nextTile)) {
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
		GameObject *blocker = this->_findBlockingUnit(this->map->topLeftCoordToAbs(nextTile), this->map->tileSize, ignoreObject);

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

		GameObject *blocker = this->_findBlockingUnit(this->map->topLeftCoordToAbs(nextTile), this->map->tileSize, ignoreObject);

		if (blocker == nullptr)
			return COLLISION_PROCEED;
		if (!blocker->canMove())
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
	if (this->_altitude)
		return COLLISION_PROCEED;

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

	GameObject *blocker = this->_findBlockingUnit(nextPos, this->_size, ignoreObject);

	if (blocker == nullptr)
		return COLLISION_PROCEED;
	if (!blocker->canMove())
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
	if (tile) return this->harvest(this->map->topLeftCoordToAbs(pos), true);
	tile = this->manager->getClosestHarvestableTile(*this->map, this->_gridPos);
	if (tile) return this->harvest(this->manager->getLoadedMap().grid.topLeftCoordToAbs(*tile), true);
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
	this->_orderQueue.emplace_front(MoveCheckOrder{
		.fail = &GameObject::_goToClosestResource,
		.destination = pos
	});
	this->_orderQueue.emplace_front(MoveOrder{
		.destination = pos,
		.reason = MOVEREASON_HARVEST,
		.ignore = nullptr,
		.force = false
	});
}

void GameObject::_dumpInClosestResourceDump()
{
	if (!this->_lastResourceDump)
		this->_lastResourceDump = this->manager->getClosestResourceDump(this->_acceptResources, this->owner->state.team, this->_position + this->_size / 2);
	this->dumpResourcesTo(nullptr, true);
}

void GameObject::_goToClosestResourceDump()
{
	if (!this->_lastResourceDump)
		this->_lastResourceDump = this->manager->getClosestResourceDump(this->_acceptResources, this->owner->state.team, this->_position + this->_size / 2);
	if (!this->_lastResourceDump)
		return;

	auto pos = this->_lastResourceDump->getPosition() + this->_lastResourceDump->getPoint();

	this->_orderQueue.emplace_front(MoveCheckOrder{
		.fail = &GameObject::_goToClosestResourceDump,
		.destination = pos
	});
	this->_orderQueue.emplace_front(MoveOrder{
		.destination = pos,
		.reason = MOVEREASON_DUMP,
		.ignore = nullptr,
		.force = false
	});
}

Vector2i GameObject::_findLandingSpot(GameObject *target, Vector2i preferredTile) const
{
	auto &map = *target->map;
	Vector2u targetSize = target->getSize();
	unsigned maxRing = std::max(map.gridSize.x, map.gridSize.y);

	for (unsigned r = 0; r <= maxRing; r++) {
		for (int dy = -static_cast<int>(r); dy <= static_cast<int>(r); dy++)
			for (int dx = -static_cast<int>(r); dx <= static_cast<int>(r); dx++) {
				if (std::max(std::abs(dx), std::abs(dy)) != static_cast<int>(r))
					continue;

				Vector2i tile = preferredTile + Vector2i{dx, dy};

				if (
					tile.x < 0 || tile.y < 0 ||
					static_cast<unsigned>(tile.x) >= map.gridSize.x ||
					static_cast<unsigned>(tile.y) >= map.gridSize.y
				)
					continue;

				Vector2i tileAbs = map.topLeftCoordToAbs(tile);
				MapState::Quad footprint = {
					tileAbs.to<float>(),
					{static_cast<float>(tileAbs.x + targetSize.x), static_cast<float>(tileAbs.y)},
					{static_cast<float>(tileAbs.x + targetSize.x), static_cast<float>(tileAbs.y + targetSize.y)},
					{static_cast<float>(tileAbs.x), static_cast<float>(tileAbs.y + targetSize.y)}
				};

				// target must be able to stand there at all (passable terrain for its own pass flags).
				if (!map.allHasAnyGridBits(footprint, target->_passFlags))
					continue;
				// Nothing else already sitting on target's own tile(s).
				if (!this->manager->isAreaFree(tileAbs, targetSize, {target->index}, target->_stackable))
					continue;
				return tile;
			}
	}
	return preferredTile;
}

void GameObject::_reduceTimeControlTimer(unsigned dec)
{
	if (this->_takeControlTime == 0)
		return;
	if (this->_takeControlTime > dec) {
		this->_takeControlTime -= dec;
		return;
	}
	this->_takeControlTime = 0;
	this->deinitGlobalGrid();
	this->owner = this->_originalOwner;
	this->_originalOwner = nullptr;
	this->initGlobalGrid();
	this->setColor(this->owner->getColor());
}

void GameObject::_spawnUnits(const std::unordered_map<std::string, unsigned> &units)
{
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

	for (auto [name, nb] : units) {
		while (nb--) {
			assert_exp(i < std::size(offsets));
			auto pos = this->_gridPos + offsets[i].first;
			auto obj = this->manager->spawnObject(this->owner->state.index, this->map->topLeftCoordToAbs(pos), "", name);
			auto out = pos + offsets[i].second[game->gameRandom() % offsets[i].second.size()];

			obj->moveTo(this->map->topLeftCoordToAbs(out), MOVEREASON_MOVE_SPAWN, true);
			i++;
		}
	}
}

bool GameObject::_updateOrder(const TurnOrder &order)
{
	if (this->_currentDir == order.resultDir)
		return true;
	this->turnTimer++;
	if (this->turnTimer > this->_turnDelay) {
		size_t diff = (this->_currentDir - order.resultDir + this->_anims.size()) % this->_anims.size();

		this->turnTimer = 0;
		if (diff >= this->_anims.size() / 2)
			this->_currentDir = (this->_currentDir + 1) % this->_anims.size();
		else
			this->_currentDir = (this->_currentDir - 1 + this->_anims.size()) % this->_anims.size();
	}
	return false;
}

bool GameObject::_updateOrder(const MoveOrder &order)
{
	if (this->_firstEvent)
		this->currentSpeed = this->_speed;
	if (this->_pickup) {
		this->_pathCached = false;
		return false;
	}
	if (!this->_pathCached) {
		this->_cachedDest = order.destination;
		this->_computeMovePath(order.destination, order.force, order.ignore);
		this->_pathCached = true;
		if (this->_pathCache.empty())
			return true;

		this->_cachedDist = this->_position.distance(this->_pathCache.back() + this->_getTileOffsetFromCenter());

		for (size_t i = 0; i < this->_pathCache.size() - 1; i++)
			this->_cachedDist += this->_pathCache[i].distance(this->_pathCache[i + 1]);
	}

	for (auto &turret : this->_turrets) {
		turret.fast = true;
		turret.shotTimer = 0;
	}
	if (this->_pathCache.empty())
		return true;

	this->_pickup = this->manager->findPickup(this->owner->state.index, order.reason, this->_cachedDist, this);
	if (this->_pickup) {
		this->_pickup->pickup(this, order.destination);
		return false;
	}

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

	// TODO: All units should still have a direction even when non directional sprites are used
	//       so the turn speed can be changed.
	if (a) {
		unsigned angle = round((*a + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

		angle %= this->_anims.size();
		if (angle != this->_currentDir) {
			if (!this->_updateOrder(TurnOrder{angle}) && !this->_moveWhileTurn)
				return false;
			*a = this->getFacingAngle();
			dest = {
				std::cos(*a) * this->currentSpeed * 2,
				std::sin(*a) * this->currentSpeed * 2
			};
		}
	}

	switch (this->_boundToGrid ? this->_resolveGridCollision(order.force, order.ignore) : this->_resolveFreeCollision(order.force, order.ignore)) {
	case COLLISION_ABORT:
		return true;
	case COLLISION_WAIT:
		return false;
	case COLLISION_PROCEED:
		break;
	}

	auto pos = this->_position;
	auto diff = dest - pos;

	if (diff.magnitude2() < this->_decel.dist2) {
		this->currentSpeed -= this->_decel.step;
		if (this->currentSpeed < this->_decel.min)
			this->currentSpeed = this->_decel.min;
	}
	//if (this->_stackable) {
	//	//if (std::abs(diff.x))
	//	//qddqzzqddzq
	//}
	this->_moving = true;
	//if (this->_boundToGrid) {
	//	diff.x = std::copysign(this->currentSpeed, diff.x);
	//	diff.y = std::copysign(this->currentSpeed, diff.y);
	//} else {
		diff.normalize();
		diff *= this->currentSpeed;
	//}
	if (this->_crouchTimer != 0)
		diff /= 2;
	this->_cachedDist -= diff.magnitude();
	pos += diff;

	auto diff2 = dest - pos;

	if (pos == dest || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0)) {
		pos = dest;
		this->_pathCache.pop_back();
	}
	this->setPosition(pos);
	return false;
}

bool GameObject::_updateOrder(const MoveCheckOrder &order)
{
	if (this->_position != order.destination) {
		auto front = this->_orderQueue.front();

		this->_orderQueue.pop_front();
		(this->*order.fail)();
		this->_orderQueue.push_front(front);
	}
	return true;
}

bool GameObject::_updateOrder(const HarvestOrder &)
{
	if (this->harvestDelaying) {
		this->harvestTimer++;
		if (this->harvestTimer >= this->_harvestDelay) {
			this->harvestDelaying = false;
			this->harvestTimer = 0;
			if (this->cancel)
				return true;
			if (this->_lots.size() >= this->_resourceStorage) {
				this->_dumpInClosestResourceDump();
				return true;
			}

			auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);

			if (tile)
				return false;
			this->_harvestClosestResource();
			return true;
		}
	} else {
		if (this->_lots.size() >= this->_resourceStorage) {
			this->_dumpInClosestResourceDump();
			return true;
		}

		auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);

		if (!tile) {
			this->_harvestClosestResource();
			return true;
		}
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

bool GameObject::_updateOrder(const HarvestCloseOrder &)
{
	if (this->_pickup)
		return false;
	this->_harvestClosestResource();
	return true;
}

bool GameObject::_updateOrder(const DumpOrder &order)
{
	if (this->_firstEvent) {
		if (order.target == nullptr || order.target->isDead()) {
			this->_dumpInClosestResourceDump();
			return true;
		}
		if (this->_lots.empty()) {
			this->_harvestClosestResource();
			return true;
		}
	} else if (this->_dumpStart) {
		if ((*this->_dump->start)() && this->_dump->start->update()) {
			this->_dumping = true;
			this->_dumpStart = false;
		}
	} else if (this->_dumpStop) {
		if ((*this->_dump->stop)() && this->_dump->stop->update()) {
			if (!this->_lots.empty())
				this->_dumpInClosestResourceDump();
			else
				this->_harvestClosestResource();
			this->_dumpStop = false;
			return true;
		}
	} else if (this->_dumping) {
		if (order.target == nullptr || order.target->isDead()) {
			this->_dumpStop = true;
			this->_dumping = false;
			return false;
		}
		if (this->dumpTimer == this->_dump->delay) {
			std::unordered_map<std::string, unsigned> value;
			// TODO: Handle other strategies
			Player::CreditStrategy strategy = Player::CREDIT_SPILL;
			auto &accept = order.target->getAcceptResources();

			for (auto &[type, content] : this->_lots.front().value) {
				if (std::ranges::find(accept, type) == accept.end()) {
					auto p = this->manager->convertResource(type, accept, content);
					value[p.first] += p.second;
				} else
					value[type] += content;
			}
			for (auto &[type, content] : value)
				order.target->owner->credit(type, content, strategy);
			this->_lots.pop_front();
			order.target->produced();
		}
		if (this->dumpTimer >= this->_dump->delay + this->_dump->delayAfter) {
			if (this->_lots.empty() || this->cancel) {
				this->_dumpStop = true;
				this->_dumping = false;
			} else
				this->dumpTimer = 0;
		} else
			this->dumpTimer++;
	} else if (this->_position != (order.target->getPosition() + order.target->getPoint()).to<float>()) {
		return false;
	} else if (!this->_updateOrder(TurnOrder{this->_dump->dir})) {
		return false;
	} else
		this->_dumpStart = true;
	return false;
}

bool GameObject::_updateOrder(const DeployOrder &)
{
	assert_exp(this->_onDeploy);
	if (this->_onDeploy->spawnBuilding) {
		auto &s = *this->_onDeploy->spawnBuilding;

		if (this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building, {this->index})) {
			this->_solid = false;
			this->_disposable = true;

			GameObject *result = this->manager->spawnObject(this->owner->state.index, this->_position + s.offset, s.queue, s.building);

			result->setHealth(this->_hp * result->getHealth() / this->_maxHp);
		} else
			this->owner->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300);
	}
	return true;
}

bool GameObject::_updateOrder(const AttackUnitOrder &order)
{
	GameObject *obj = order.target;

	if (obj == nullptr || obj->isDead() || obj->owner->state.team == this->owner->state.team)
		return this->_updateOrder(FinishMovementOrder{});
	if (this->_firstEvent)
		this->attackFirst = 1;

	Vector2f myCenter;

	if (this->_boundToGrid) {
		MapState::Quad abs = this->map->tileCoordToAbs(this->_gridPos);

		myCenter = (abs.pt1 + abs.pt2 + abs.pt3 + abs.pt4) / 4;
	} else
		myCenter = this->_position + this->_size / 2;

	Vector2f opCenter = obj->getHitboxCenter(myCenter);
	Vector2f diff = opCenter - myCenter;
	Vector2f diffAbs = diff.abs();
	const ObjectWeapon *weapon = this->getBestWeaponFor(obj, UINT32_MAX, true);
	if (weapon == nullptr)
		weapon = this->getBestWeaponFor(obj, UINT32_MAX, false);
	assert_exp(weapon);

	unsigned range = weapon->weapon->getRange();

	range *= range;
	if (range < diffAbs.magnitude2()) {
		this->_firstEvent = this->attackFirst;
		this->attackFirst = 0;
		if (this->_updateOrder(MoveOrder{
			.destination = opCenter,
			.reason = order.force ?
				MOVEREASON_FORCE_ATTACK_COMMAND :
			order.automatic ?
				MOVEREASON_AUTO_AGRO :
				MOVEREASON_ATTACK_COMMAND,
			.ignore = obj,
			.force = false
		})) {
			// TODO: Handle when unit is blocked
			if (opCenter != this->_cachedDest)
				this->_pathCached = false;
		}
		return false;
	}
	if (!this->_updateOrder(FinishMovementOrder{}))
		return false;

	ObjectBarrel &barrel = this->_barrels[weapon->barrel];

	if (barrel.shootTimer != 0)
		return false;

	double angle = diff.angle();
	unsigned direction = round((angle + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));
	ObjectTurret *turret = barrel.turret;

	direction %= this->_anims.size();
	if (turret && !turret->fixed) {
		size_t d = direction * turret->anims.size() / this->_anims.size();

		turret->directionTarget = d;
		turret->fast = true;
		if (turret->direction != d) {
			turret->shotTimer = 1;
			return false;
		}
	} else if (direction != this->_currentDir && !this->_updateOrder(TurnOrder{direction}))
		return false;

	double inaccuracy = weapon->weapon->getInaccuracy(diff.magnitude());
	std::uniform_real_distribution<> dist(0, inaccuracy);
	const MultiSound *sound = weapon->weapon->getSound();
	const Effect *effect = weapon->weapon->getEffect();
	const Effect *hitEffect = weapon->weapon->getHitEffect();
	Vector2f targetPos = opCenter;
	Vector2f spawnPos = this->_position + this->_size / 2;

	spawnPos.y -= this->_altitude;
	barrel.shootTimer = weapon->cooldown;
	if (weapon->weapon->sendsForward()) {
		double dist = (targetPos - spawnPos).magnitude();
		double myAngle = this->getFacingAngle();

		targetPos = spawnPos;
		targetPos.x += std::cos(myAngle) * dist;
		targetPos.y += std::sin(myAngle) * dist;
	} else {
		double offset = dist(game->gameRandom);
		uint64_t dangle = random_distrib(game->gameRandom, 0, 360);

		targetPos.x += std::cos(dangle * M_PI / 180) * offset;
		targetPos.y += std::sin(dangle * M_PI / 180) * offset;
	}
	if (sound)
		sound->play(spawnPos);
	if (effect)
		this->manager->spawnEffect(*effect, spawnPos, this->owner);
	this->_shooting = true;
	if (turret)
		turret->attack();
	if (!this->_muzzleAnim.empty())
		this->manager->spawnEffect(*this->_muzzleAnim[this->_currentDir], this->_position - Vector2i{0, this->_altitude} + this->_size / 2, this->owner);
	switch (weapon->weapon->getType()) {
	case WEAPONTYPE_HITSCAN:
		if (hitEffect)
			this->manager->spawnEffect(*hitEffect, targetPos, this->owner);
		this->manager->weaponUsedAt(weapon->weapon->getDamage(), weapon->weapon, targetPos, this->owner->state.index);
		break;
	case WEAPONTYPE_PROJECTILE:
		this->manager->spawnProjectile(this, weapon->weapon, spawnPos, obj, targetPos);
		break;
	case WEAPONTYPE_ENTER:
		this->manager->weaponUsedOn(
			weapon->weapon->getDamage(),
			weapon->weapon,
			targetPos.distance(obj->getHitboxCenter(targetPos)),
			this->owner->state.index,
			*obj
		);
		this->_disposable = true;
		break;
	default:
		assert_not_reached();
	}
	return false;
}

bool GameObject::_updateOrder(const SpinOrder &order)
{
	if (this->_firstEvent)
		this->currentAngle = this->getFacingAngle() * 180 / M_PI;
	this->currentAngle = fmod(this->currentAngle + 360 + order.angle, 360);

	double angle = this->currentAngle * M_PI / 180;
	Vector2f speed = {this->_speed, 0};

	this->_currentDir = round((angle + M_PI_2) * this->_anims.size() / (M_PI * 2));
	this->_currentDir %= this->_anims.size();
	this->setPosition(speed.rotation(angle, {0, 0}) + this->_position);
	return false;
}

bool GameObject::_updateOrder(const PickupOrder &order)
{
	GameObject *target = order.target;

	switch (this->pickupState) {
	case 0: {
		Vector2f targetCenter = target->getMiddle();

		if (this->_updateOrder(MoveOrder{
			.destination = targetCenter - this->_getTileOffsetFromCenter(),
			.reason = MOVEREASON_DELIVERY,
			.ignore = target,
			.force = false
		}))
			this->pickupState = 1;
		return false;
	}
	case 1: {
		unsigned converted = round((target->getFacingAngle() + M_PI_2) * this->_anims.size() / (M_PI * 2));

		if (this->_currentDir != converted) {
			this->pickupState = 2;
			this->pickupOldAltitude = this->_altitude;
		} else
			this->_updateOrder(TurnOrder{converted});
		return false;
	}
	case 2:
		if (this->_altitude > 4)
			this->setAltitude(this->_altitude - 1);
		else
			this->pickupState = 3;
		return false;
	case 3:
		if (this->_altitude < this->pickupOldAltitude) {
			this->setAltitude(this->_altitude + 1);
			target->setAltitude(this->_altitude - 4);
		} else
			this->pickupState = 4;
		return false;
	case 4:
		this->_pathCached = false;
		this->_firstEvent = true;
		this->pickupState = 5;
		[[fallthrough]];
	case 5: {
		auto d = order.destination - this->_getTileOffsetFromCenter() - target->_getTileOffsetFromCenter();
		bool arrived = this->_updateOrder(MoveOrder{
			.destination = d,
			.reason = MOVEREASON_DELIVERY,
			.ignore = target,
			.force = false
		});

		target->_currentDir = round((this->getFacingAngle() + M_PI_2) * target->_anims.size() / (M_PI * 2));
		target->setPosition(this->getMiddle() + target->_getTileOffsetFromCenter());
		if (!arrived)
			return false;

		Vector2i landingTile = this->_findLandingSpot(target, target->_gridPos);

		if (landingTile == target->_gridPos) {
			this->pickupState = 6;
			return false;
		}
		this->pickupState = 4;
		if (auto *order = std::get_if<PickupOrder>(&this->_orderQueue.front()))
			order->destination = target->map->topLeftCoordToAbs(landingTile);
		this->_pathCached = false;
		return false;
	}
	case 6:
		if (this->_altitude > 4) {
			this->setAltitude(this->_altitude - 1);
			target->setAltitude(this->_altitude - 4);
		} else {
			this->pickupState = 7;
			target->_pickup = nullptr;
		}
		return false;
	case 7:
		if (this->_altitude < this->pickupOldAltitude)
			this->setAltitude(this->_altitude + 1);
		else
			this->pickupState = 100;
		return false;
	default:
		return true;
	}
}

bool GameObject::_updateOrder(const WaitPickupOrder &order)
{
	if (order.other)
		this->_updateOrder(*order.other);
	if (this->_firstEvent) {
		this->_pickupTargets = &order.targets;
		this->_pickupDistMin = order.dist;
	}
	return false;
}

bool GameObject::_updateOrder(const FinishMovementOrder &)
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
	return this->_updateOrder(MoveOrder{
		.destination = this->_cachedDest,
		.reason = MOVEREASON_FINISH_MOVE,
		.ignore = nullptr,
		.force = false
	});
}

bool GameObject::_updateOrder(const DeliveryOrder &order)
{
	const std::vector<std::string> &objects = order.objects;
	const std::string &queue = order.queue;
	const Position &destination = order.destination;
	const MapState &state = *order.state;
	GameObject *obj = this->owner->getSpawnBuilding(queue, false);
	Vector2f dest = destination.getPosition(obj ? obj->getPosition() : Vector2i{0, 0}, state, {0, 0}) + state.tileSize / 2;

	for (auto &b : this->owner->state.queueBuildings[queue])
		b->producing(false);
	switch (this->pickupState) {
	case 1:
		if (obj)
			obj->producing(true);
		if (this->_altitude < this->pickupOldAltitude)
			this->setAltitude(this->_altitude + 1);
		else
			this->pickupState = 4;
		return false;
	case 0:
		this->pickupOldAltitude = this->_altitude;
		[[fallthrough]];
	case 4: {
		this->_pathCached = false;
		this->_firstEvent = true;
		this->pickupState = 5;
		[[fallthrough]];
	}
	case 5: {
		if (obj)
			obj->producing(true);

		bool arrived = this->_updateOrder(MoveOrder{
			.destination = dest - this->_getTileOffsetFromCenter(),
			.reason = MOVEREASON_DELIVERY,
			.ignore = obj,
			.force = false
		});

		if (!arrived)
			return false;
		if (!obj)
			return true;
		if (this->_position - this->_getTileOffsetFromCenter() != dest) {
			this->pickupState = 4;
			return false;
		}
		this->pickupState = 6;
		return false;
	}
	case 6:
		if (obj)
			obj->producing(true);
		if (this->_altitude > 8)
			this->setAltitude(this->_altitude - 1);
		else
			this->pickupState = 7;
		return false;
	case 7:
		if (this->_position - this->_getTileOffsetFromCenter() != dest) {
			this->pickupState = 1;
			return false;
		}
		if (this->dropTimer != 0)
			this->dropTimer--;
		else if (this->currentDrop < objects.size()) {
			std::vector<GameObject *> buffer;
			Vector2f d;

			// TODO: Hardcoded starport layout
			if (this->dropLast == 0)
				d = dest + state.tileSize;
			else
				d = dest + Vector2f{-static_cast<float>(state.tileSize.x), static_cast<float>(state.tileSize.y)};
			this->manager->getObjectsInArea(dest - state.tileSize / 2, state.tileSize, buffer);

			auto range = std::ranges::remove_if(buffer, [obj](GameObject *o){
				return o == obj || !o->isSolid();
			});

			buffer.erase(range.begin(), range.end());
			if (buffer.empty()) {
				GameObject *object = this->manager->spawnObject(this->owner->state.index, dest, queue, objects[this->currentDrop]);

				object->moveTo(d, MOVEREASON_DELIVERY);
				this->dropTimer = 25;
				this->currentDrop++;
				this->dropLast = !this->dropLast;
			}
		} else if (this->currentDrop == objects.size())
			this->pickupState = 8;
		return false;
	case 8:
		if (this->_altitude >= this->pickupOldAltitude)
			return true;
		this->setAltitude(this->_altitude + 1);
		return false;
	default:
		return true;
	}
}

bool GameObject::_updateOrder(const ExecCbOrder &order)
{
	return order.cb();
}

bool GameObject::_updateOrder(const DieOrder &)
{
	this->_disposable = true;
	return true;
}

bool GameObject::_updateOrder(const Order &order)
{
	return std::visit([this](auto &&arg) {
		return this->_updateOrder(arg);
	}, order);
}

void GameObject::playBuiltSound(Vector2f pos) const
{
	if (this->_buildSound)
		this->_buildSound->play(pos + this->_size / 2);
}

void GameObject::update()
{
	if (this->_pickup && this->_pickup->isDisposable())
		this->_pickup = nullptr;
	if (this->_deathAnim) {
		if (this->_animFinished || !this->_deathAnim->anim || this->_deathAnim->anim->update())
			this->_animFinished = true;
		if (this->_deathAnim->stayTime && this->_gpTimerU[3] < *this->_deathAnim->stayTime)
			this->_gpTimerU[3]++;
		else if (this->_deathAnim->fadeTime && this->_gpTimerU[2] < *this->_deathAnim->fadeTime) {
			this->_gpTimerU[2]++;
			this->_alpha = static_cast<float>(*this->_deathAnim->fadeTime - this->_gpTimerU[2]) / *this->_deathAnim->fadeTime;
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
	for (auto &turret : this->_turrets)
		turret.update();
	this->_reduceTimeControlTimer(1);
	this->_moving = false;
	for (auto &barrel : this->_barrels)
		if (barrel.shootTimer)
			barrel.shootTimer--;
	if (!this->_orderQueue.empty()) {
		if (this->_firstREvent) {
			this->_firstEvent = true;
			this->_pathCached = false;
			this->_shooting = false;
			this->_gpTimerU.fill(0);
			this->_gpTimerF.fill(0);
		} else
			this->_firstEvent = false;
		if (this->_updateOrder(this->_orderQueue.front())) {
			this->_orderQueue.pop_front();
			this->_firstREvent = true;
		} else
			this->_firstREvent = false;
	} else if (this->_idleOrder) {
		if (this->_firstIEvent) {
			this->_firstEvent = true;
			this->_pathCached = false;
			this->_shooting = false;
			this->_gpTimerU.fill(0);
			this->_gpTimerF.fill(0);
		} else
			this->_firstEvent = false;
		if (this->_updateOrder(*this->_idleOrder))
			this->_firstIEvent = true;
		else
			this->_firstIEvent = false;
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
				this->_currentIdle = random_distrib(game->gameRandom, 1, this->_idleAnims.size());
			}
		}
	} else
		this->_currentIdle = 0;
	if (this->_shooting) {
		if (this->_crouchTimer == 0) {
			if (!this->_fireAnim.empty() && this->_fireAnim[this->_currentDir].update())
				this->_shooting = false;
		} else {
			if (!this->_crawlFireAnim.empty() && this->_crawlFireAnim[this->_currentDir].update())
				this->_shooting = false;
		}
	}
	if (this->_built) {
		if (!this->_buildAnim || !(*this->_buildAnim)() || this->_buildAnim->update())
			this->_built = false;
	} else if (this->_sold) {
		if (this->_sellAnim) {
			if (this->_sellAnim->current() == this->_showBeforeSell && this->_sellTimer == 0) {
				this->_sellTimer++;
				this->_spawnUnits(this->_sellUnits);
			}
			if ((*this->_sellAnim)() && this->_sellAnim->update()) {
				for (auto &[fundType, value] : this->_sellRefund)
					this->owner->credit(fundType, value, Player::CREDIT_SYSTEM);
				this->_disposable = true;
				this->_solid = false;
				return;
			}
		} else {
			this->_spawnUnits(this->_sellUnits);
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
	if (!this->_shooting && !this->_dumpStop && !this->_dumpStart && !this->_dumping && !this->_currentIdle) {
		if (this->_crouchTimer == 0) {
			if (!this->_connectTo.empty()) {
				for (auto &anim : this->_anims[this->_connectedMask])
					if (anim())
						anim.update();
			} else if (this->_moving && !this->_moveAnim.empty()) {
				if (this->_moveAnim[this->_currentDir]())
					this->_moveAnim[this->_currentDir].update();
			} else for (auto &anim : this->_anims[this->_currentDir])
				if (anim())
					anim.update();
		} else {
			if (this->_moving && !this->_crawlAnim.empty()) {
				if (this->_crawlAnim[this->_currentDir]())
					this->_crawlAnim[this->_currentDir].update();
			} else {
				if (this->_crouchAnim[this->_currentDir]())
					this->_crouchAnim[this->_currentDir].update();
			}
		}
	}
	for (auto &o : this->_orderQueue) {
		if (auto dump = std::get_if<DumpOrder>(&o)) {
			if (dump->target && dump->target->isDisposable())
				dump->target = nullptr;
		} else if (auto attack = std::get_if<AttackUnitOrder>(&o))
			if (attack->target && attack->target->isDisposable())
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
	if (this->_crouchTimer)
		this->_crouchTimer--;
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
		if (this->_deathAnim->stayTime && this->_gpTimerU[3] >= *this->_deathAnim->stayTime)
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
			if (this->_crouchTimer == 0) {
				unsigned a = this->_currentDir;

				if (!this->_connectTo.empty()) {
					for (auto &anim : this->_anims[this->_connectedMask])
						if (anim() && anim.getLayer() == layer)
							this->_displayAnim(anim.getAnim());
				} else if (this->_shooting && !this->_fireAnim.empty()) {
					auto &anim = this->_fireAnim[this->_currentDir];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				} else if (this->_moving && !this->_moveAnim.empty()) {
					auto &anim = this->_moveAnim[this->_currentDir];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				} else if (this->_currentIdle) {
					auto &anim = this->_idleAnims[this->_currentIdle - 1];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				} else for (auto &anim : this->_anims[a])
					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
			} else {
				if (this->_shooting && !this->_crawlFireAnim.empty()) {
					auto &anim = this->_crawlFireAnim[this->_currentDir];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				} else if (this->_moving && !this->_crawlAnim.empty()) {
					auto &anim = this->_crawlAnim[this->_currentDir];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				} else {
					auto &anim = this->_crouchAnim[this->_currentDir];

					if (anim() && anim.getLayer() == layer)
						this->_displayAnim(anim.getAnim());
				}
			}
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
		for (auto &turret : this->_turrets)
			turret.render(layer);
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
	if (this->_registered)
		this->manager->deregisterObject(this);
	this->_registered = false;
	this->deinitGlobalGrid();
	this->_position = pos;
	if (this->_tiles)
		this->_gridPos = this->map->absToTileCoord(pos);
	else if (this->_stackable) {
		this->_gridPos = this->map->absToTileCoord(pos - this->_getTileOffsetFromCenter());
		if (this->_stackable && !PFGRIDMASK_IS_SLOT_FREE(this->map->pathFindingGrid, this->_gridPos.x, this->_gridPos.y, this->_currentTilePosition)) {
			auto old = this->_getTileOffsetFromCenter();

			this->_currentTilePosition = PFGRIDMASK_FIND_FREE_SLOT(this->map->pathFindingGrid, this->_gridPos.x, this->_gridPos.y);
			if (!this->_moving)
				this->_position += this->_getTileOffsetFromCenter() - old;
			assert_exp(this->_currentTilePosition);
		}
	} else
		this->_gridPos = this->map->absToTileCoord(pos + this->_size / 2);
	this->initGlobalGrid();
	this->manager->registerObject(this);
	this->_registered = true;
}

void GameObject::setAltitude(int altitude)
{
	// OPTIMIZE: Diff instead of undoing it and redoing it
	if (this->_registered)
		this->manager->deregisterObject(this);
	this->_registered = false;
	this->deinitGlobalGrid();
	this->_altitude = altitude;
	this->initGlobalGrid();
	this->manager->registerObject(this);
	this->_registered = true;
}

void GameObject::setColor(const HSLColor &color)
{
	for (auto &block : this->_anims)
		for (auto &anim : block)
			anim.setColor(color);
	for (auto &turret : this->_turrets)
		for (auto &anim : turret.anims)
			anim.setColor(color);

	for (auto &d : this->_deathAnims)
		if (d.anim)
			d.anim->setColor(color);
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
	if (this->_crouchTimer)
		amount /= 2;
	this->setHealth(std::max<int>(this->_hp - amount, 0));
	if (this->_resourceStorage != 0)
		this->owner->playEVA(UI::EVA_HARV_UNDER_ATTACK, 300, 3, 600, this->_id);
	else if (this->_tiles)
		this->owner->playEVA(UI::EVA_BUILDING_ATTACKED, 300, 3, 600, this->_id);
	else
		this->owner->playEVA(UI::EVA_UNIT_ATTACKED, 300, 1, 600, this->_id);
	if (!this->_crouchAnim.empty() && random_distrib(game->gameRandom, 0, 3) == 0)
		this->_crouchTimer = 600;
	this->_reduceTimeControlTimer(this->_timeDecOnAtk);
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
	this->_gpTimerU[2] = 0;
	this->_gpTimerU[3] = 0;
	if (this->_deathAnim->effect) {
		if (!this->_deathAnim->perTile || !this->_tiles) {
			for (unsigned i = 0; i < this->_deathAnim->count; i++) {
				Vector2f pos = this->_position - Vector2i{0, this->_altitude} + this->_size / 2;

				if (this->_deathAnim->variance.x != 0)
					pos.x += random_distrib(game->gameRandom, -this->_deathAnim->variance.x, this->_deathAnim->variance.x);
				if (this->_deathAnim->variance.y != 0)
					pos.y += random_distrib(game->gameRandom, -this->_deathAnim->variance.y, this->_deathAnim->variance.y);
				this->manager->spawnEffect(*this->_deathAnim->effect, pos, this->owner);
			}
		} else for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!line[gx].solid)
					continue;
				for (unsigned i = 0; i < this->_deathAnim->count; i++) {
					MapState::Quad abs = this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					});
					Vector2f tlPos = Vector2f{
						std::min(std::min(abs.pt1.x, abs.pt2.x), std::min(abs.pt3.x, abs.pt4.x)),
						std::min(std::min(abs.pt1.y, abs.pt2.y), std::min(abs.pt3.y, abs.pt4.y))
					};
					Vector2f brPos = Vector2f{
						std::max(std::max(abs.pt1.x, abs.pt2.x), std::max(abs.pt3.x, abs.pt4.x)),
						std::max(std::max(abs.pt1.y, abs.pt2.y), std::max(abs.pt3.y, abs.pt4.y))
					};
					Vector2f pos = (tlPos + brPos) / 2;

					if (this->_deathAnim->variance.x != 0)
						pos.x += random_distrib(game->gameRandom, -this->_deathAnim->variance.x, this->_deathAnim->variance.x);
					if (this->_deathAnim->variance.y != 0)
						pos.y += random_distrib(game->gameRandom, -this->_deathAnim->variance.y, this->_deathAnim->variance.y);
					this->manager->spawnEffect(*this->_deathAnim->effect, pos, this->owner);
				}
			}
		}
		if (this->_deathAnim->shake)
			this->manager->shakeScreen(*this->_deathAnim->shake);
	}
	if (!this->_deathAnim->projectilesPool.empty()) {
		unsigned count = this->_deathAnim->projectilesCount.first;

		if (count != this->_deathAnim->projectilesCount.second)
			count = random_distrib(game->gameRandom, this->_deathAnim->projectilesCount.first, this->_deathAnim->projectilesCount.second);

		while (count--) {
			unsigned pool = 0;
			Vector2i target = this->_deathAnim->projectilesTarget.first;

			if (this->_deathAnim->projectilesPool.size() != 1)
				pool = random_distrib(game->gameRandom, 0, this->_deathAnim->projectilesPool.size() - 1);
			if (target != this->_deathAnim->projectilesTarget.second) {
				target.x = random_distrib(game->gameRandom, this->_deathAnim->projectilesTarget.first.x, this->_deathAnim->projectilesTarget.second.x);
				target.y = random_distrib(game->gameRandom, this->_deathAnim->projectilesTarget.first.y, this->_deathAnim->projectilesTarget.second.y);
			}
			target += this->_position - Vector2i{0, this->_altitude} + this->_size / 2;
			this->manager->spawnProjectile(this, this->_deathAnim->projectilesPool[pool], this->_position - Vector2i{0, this->_altitude} + this->_size / 2, nullptr, target);
		}
	}
	this->_spawnUnits(this->_deathAnim->units);
}

void GameObject::setHealth(unsigned health)
{
	if (this->_lowerPowerWhenDamaged)
		this->owner->state.producedPower += this->getPower();
	if (health == 0) {
		if (this->_deathSound)
			this->_deathSound->play(this->_position + this->_size / 2);
		// TODO: Also fire for all allies
		if (this->_tiles)
			this->owner->playEVA(UI::EVA_BUILDING_LOST, 300, 3, 600, this->_id);
		else
			this->owner->playEVA(UI::EVA_UNIT_LOST, 300, 1, 600, this->_id);
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
	return Vector2i(round(this->_position.x), round(this->_position.y - this->_altitude));
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

bool GameObject::isObstructed(bool stackable) const
{
	auto spawn = this->getSpawnOffset();

	if (spawn)
		return !this->manager->isAreaFree(
			*spawn + this->getPosition(),
			this->map->tileSize,
			{this->index},
			stackable
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
		this->_selectSound->play(this->_position + this->_size / 2);
}

void GameObject::deploy()
{
	if (this->_onDeploy) {
		if (this->_onDeploy->direction)
			this->_orderQueue.emplace_back(TurnOrder{*this->_onDeploy->direction});
		this->_orderQueue.emplace_back(DeployOrder{});
	}
	if (this->_doSound)
		this->_doSound->play(this->_position + this->_size / 2);
}

bool GameObject::canDeploy() const
{
	if (this->_onDeploy) {
		if (this->_onDeploy->spawnBuilding) {
			auto &s = *this->_onDeploy->spawnBuilding;

			return this->manager->canSpawnBuilding(this->owner->state.index, this->_position + s.offset, s.building, {this->index});
		}
	}
	return false;
}

bool GameObject::canMove() const
{
	return this->_speed != 0;
}

void GameObject::harvest(const Vector2i &pos, bool automatic, bool straight)
{
	this->moveTo(pos, automatic ? MOVEREASON_HARVEST : MOVEREASON_HARVEST_COMMAND, straight);
	this->_orderQueue.emplace_back(MoveCheckOrder{
		.fail = &GameObject::_goToClosestResource,
		.destination = pos
	});
	this->_orderQueue.emplace_back(HarvestOrder{});
}

void GameObject::harvest()
{
	this->_orderQueue.emplace_back(HarvestCloseOrder{});
}

void GameObject::attack(GameObject *obj, bool automatic, bool force)
{
	this->_orderQueue.emplace_back(AttackUnitOrder{.target = obj, .automatic = automatic, .force = force});
}

void GameObject::moveOut(const Vector2i &pos)
{
	this->_orderQueue.emplace_back(MoveOrder{
		.destination = this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos)),
		.reason = MOVEREASON_DELIVERY,
		.ignore = nullptr,
		.force = true
	});
	this->_orderQueue.emplace_back(DieOrder{});
}

void GameObject::moveTo(const Vector2i &pos, MoveReason reason, bool straight)
{
	assert_exp(this->canMove());
	if (this->_boundToGrid)
		this->_orderQueue.emplace_back(MoveOrder{
			.destination = this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos)),
			.reason = reason,
			.ignore = nullptr,
			.force = straight
		});
	else
		this->_orderQueue.emplace_back(MoveOrder{
			.destination = pos + this->_size / 2,
			.reason = reason,
			.ignore = nullptr,
			.force = straight
		});
}

bool GameObject::moveAside(const GameObject *asker)
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
	Vector2f selfCenter = this->getMiddle();
	Vector2f away = selfCenter - asker->getMiddle();
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

	this->_orderQueue.emplace_front(MoveOrder{
		.destination = destination,
		.reason = MOVEREASON_MOVE_ASIDE,
		.ignore = nullptr,
		.force = false
	});
	this->_firstREvent = true;
	this->_pathCached = false;
	this->_asideCooldown = waitBlocked;
	return true;
}

void GameObject::dumpResourcesTo(GameObject *dump, bool automatic, bool straight)
{
	if (!dump)
		dump = this->_lastResourceDump;
	else
		this->_lastResourceDump = dump;
	if (!dump)
		return;

	auto pos = dump->getPosition() + dump->getPoint();

	this->moveTo(pos, automatic ? MOVEREASON_DUMP : MOVEREASON_DUMP_COMMAND, straight);
	this->_orderQueue.emplace_back(MoveCheckOrder{
		.fail = &GameObject::_goToClosestResourceDump,
		.destination = pos
	});
	this->_orderQueue.emplace_back(DumpOrder{.target = dump});
}

void GameObject::pickup(GameObject *target, Vector2f destination)
{
	this->_orderQueue.emplace_front(PickupOrder{
		.target = target,
		.destination = destination
	});
}

void GameObject::deliverTo(const std::vector<GameObject *> &objects, Vector2f destination)
{
	// TODO: For now only handle carryalls
	assert_exp(objects.size() == 1);
	this->_orderQueue.emplace_back(PickupOrder{
		.target = objects.front(),
		.destination = destination
	});
	objects.front()->_pickup = this;
	objects.front()->setAltitude(this->_altitude - 4);
	this->pickupState = 4;
	this->pickupOldAltitude = this->_altitude;
	this->_firstREvent = false;
}

void GameObject::deliverTo(const std::vector<std::string> &objects, const std::string &queue, const Position &destination, const MapState &state)
{
	this->_orderQueue.emplace_back(DeliveryOrder{objects, queue, destination, &state});
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
		this->_doSound->play(this->_position + this->_size / 2);
}

void GameObject::clearPrimaryDump()
{
	this->_lastResourceDump = nullptr;
}

void GameObject::initGlobalGrid()
{
	if (this->_initGrid)
		return;
	if (this->_altitude)
		return;

	if (!this->_connectTo.empty()) {
		std::vector<GameObject *> buffer;

		this->manager->getObjectsInArea(this->_position - this->map->tileSize, this->map->tileSize * 2 + this->_size, buffer, true, [this](const GameObject *o) {
			return this != o && this->_connectTo == o->_connectTo;
		});
		for (GameObject *obj : buffer) {
			this->connectTo(obj);
			obj->connectTo(this);
		}
	}
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
					this->owner->state.index, line[gx].placementSpread,
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
	if (!this->_connectTo.empty()) {
		std::vector<GameObject *> buffer;

		this->manager->getObjectsInArea(this->_position - this->map->tileSize, this->map->tileSize * 2 + this->_size, buffer, true, [this](const GameObject *o) {
			return this != o && this->_connectTo == o->_connectTo;
		});
		for (GameObject *obj : buffer)
			obj->disconnectFrom(this);
		this->_connections.fill({});
		this->_connectedMask = 0;
	}
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
					this->owner->state.index, -line[gx].placementSpread,
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
	this->_crushSound->play(this->_position + this->_size / 2);
	this->_deathAnim = &*this->_crushAnim;
	this->_gpTimerU[2] = 0;
	this->_gpTimerU[3] = 0;
	this->_solid = false;
}

bool GameObject::isSolid() const
{
	return this->_solid && this->_altitude == 0;
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

void GameObject::execCb(const std::function<bool()> &cb)
{
	this->_orderQueue.emplace_back(ExecCbOrder{cb});
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

void GameObject::setOwner(Player *player, unsigned time, unsigned timeDecOnAtk)
{
	Player *old = this->owner;

	if (this->owner == player)
		return;
	if (this->_originalOwner == nullptr)
		this->_originalOwner = this->owner;
	if (this->_tiles) {
		// TODO: Also fire for all allies
		this->owner->playEVA(UI::EVA_MY_BUILDING_CAPTURED, 300, 3, 300);
		player->playEVA(UI::EVA_BUILDING_CAPTURED, 300, 3, 300);
	} else {
		// TODO: Also fire for all allies
		this->owner->playEVA(UI::EVA_MY_UNIT_CAPTURED, 300, 3, 300);
		player->playEVA(UI::EVA_UNIT_CAPTURED, 300, 3, 300);
	}
	this->deinitGlobalGrid();
	this->owner = player;
	this->initGlobalGrid();
	this->setColor(player->getColor());
	if (this->_originalOwner != this->owner) {
		this->_takeControlTime = time;
		this->_timeDecOnAtk = timeDecOnAtk;
	} else
		this->_takeControlTime = 0;
	old->onObjectLost(this);
	player->onObjectGained(this);
}

void GameObject::spawn(const Vector2f &spawnPoint)
{
	Vector2f pos = this->_spawnParams.pos.getPosition(spawnPoint, *this->map, {0, 0});
	Vector2i gridPos = this->map->absToTileCoord(pos);
	Vector2f off = this->_tiles ? Vector2f{0, 0} : this->_getTileOffsetFromCenter() + this->map->tileSize / 2;

	this->setPosition(this->map->topLeftCoordToAbs(gridPos) + off);
	if (pos != spawnPoint)
		this->moveTo(spawnPoint, MOVEREASON_MOVE_SPAWN);
}

inline void GameObject::_recomputeConnectedMask()
{
	this->_connectedMask = 0;
	for (const std::vector<const GameObject *> &a : this->_connections | std::ranges::views::reverse) {
		this->_connectedMask <<= 1;
		this->_connectedMask |= !a.empty();
	}
}

unsigned char GameObject::_getConnectionSide(const GameObject *obj)
{
	double angle = this->getMiddle().angle(obj->getMiddle());
	// 1 (top left)     -> 5π/8 ≤ angle < 7π/8
	// 2 (top)          -> 3π/8 ≤ angle < 5π/8
	// 3 (top right)    -> π/8 ≤ angle < 3π/8
	// 4 (left)         -> 7π/8 ≤ angle ≤ π or -π/8 ≤ angle ≤ -7π/8
	// 6 (right)        -> -π/8 ≤ angle < π/8
	// 7 (bottom left)  -> -7π/8 ≤ angle < -5π/8
	// 8 (bottom)       -> -5π/8 ≤ angle < -3π/8
	// 9 (bottom right) -> -3π/8 ≤ angle < -π/8
	unsigned side;

	if (5 * M_PI / 8 <= angle && angle < 7 * M_PI / 8)
		side = 1;
	else if (3 * M_PI / 8 <= angle && angle < 5 * M_PI / 8)
		side = 2;
	else if (M_PI / 8 <= angle && angle < 3 * M_PI / 8)
		side = 3;
	else if (-M_PI / 8 <= angle && angle < M_PI / 8)
		side = 6;
	else if (-3 * M_PI / 8 <= angle && angle < -M_PI / 8)
		side = 9;
	else if (-5 * M_PI / 8 <= angle && angle < -3 * M_PI / 8)
		side = 8;
	else if (-7 * M_PI / 8 <= angle && angle < -5 * M_PI / 8)
		side = 7;
	else
		side = 4;

	auto it = std::ranges::find(this->_connectedSides, side);

	if (it == this->_connectedSides.end())
		return UINT8_MAX;
	return it - this->_connectedSides.begin();
}

void GameObject::connectTo(const GameObject *object)
{
	unsigned char side = this->_getConnectionSide(object);

	if (side == UINT8_MAX)
		return;
	assert_exp(std::ranges::find(this->_connections[side], object) == this->_connections[side].end());
	this->_connections[side].push_back(object);
	this->_recomputeConnectedMask();
}

void GameObject::disconnectFrom(const GameObject *object)
{
	unsigned char side = this->_getConnectionSide(object);

	if (side == UINT8_MAX)
		return;

	auto it = std::ranges::find(this->_connections[side], object);

	assert_exp(it != this->_connections[side].end());
	this->_connections[side].erase(it);
	this->_recomputeConnectedMask();
}

void GameObject::disposeOf()
{
	this->deinitGlobalGrid();
}

bool GameObject::isWall() const
{
	return this->_wall;
}

const std::string &GameObject::getConnectTo() const
{
	return this->_connectTo;
}

Vector2f GameObject::getMiddle() const
{
	return this->_position - this->_getTileOffsetFromCenter();
}

bool GameObject::isBeingPickedUp() const
{
	return this->_pickup;
}

bool GameObject::canPickup(MoveReason r, double dist) const
{
	if (!this->_orderQueue.empty())
		return false;
	if (dist < this->_pickupDistMin)
		return false;
	if (!this->_pickupTargets)
		return false;
	if (std::ranges::find(*this->_pickupTargets, r) == this->_pickupTargets->end())
		return false;
	return true;
}

int GameObject::getAltitude() const
{
	return this->_altitude;
}

bool GameObject::ignoresTerrain() const
{
	return this->_ignoreTerrain;
}

size_t GameObject::getDirections() const
{
	if (this->_connectTo.empty())
		return this->_anims.size();
	return 1;
}

double GameObject::getFacingAngle() const
{
	return M_PI * 2 * this->_currentDir / this->_anims.size() - M_PI_2;
}

unsigned GameObject::getDirection() const
{
	return this->_currentDir;
}

bool GameObject::isInObject(Vector2f pos, Vector2f size, bool fullTile) const
{
	if (this->_tiles) {
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!line[gx].solid)
					continue;

				MapState::Quad abs = this->map->tileCoordToAbs(this->_gridPos + Vector2{gx, gy});
				Vector2f tlPos = Vector2f{
					std::min(std::min(abs.pt1.x, abs.pt2.x), std::min(abs.pt3.x, abs.pt4.x)),
					std::min(std::min(abs.pt1.y, abs.pt2.y), std::min(abs.pt3.y, abs.pt4.y))
				};
				Vector2f brPos = Vector2f{
					std::max(std::max(abs.pt1.x, abs.pt2.x), std::max(abs.pt3.x, abs.pt4.x)),
					std::max(std::max(abs.pt1.y, abs.pt2.y), std::max(abs.pt3.y, abs.pt4.y))
				};

				if (pos.x + size.x < tlPos.x)
					continue;
				if (pos.y + size.y < tlPos.y)
					continue;
				if (pos.x >= brPos.x)
					continue;
				if (pos.y >= brPos.y)
					continue;
				return true;
			}
		}
	} else {
		auto mypos = this->_position;
		auto mysize = this->_size;

		if (
			pos.x + size.x > mypos.x &&
			pos.y + size.y > mypos.y &&
			pos.x <= mypos.x + mysize.x &&
			pos.y <= mypos.y + mysize.y
		)
			return true;
	}
	return false;
}

Vector2f GameObject::getHitboxCenter(Vector2f pos) const
{
	Vector2f ocenter;

	if (this->_tiles) {
		double dist = std::numeric_limits<double>::infinity();

		ocenter = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!line[gx].solid)
					continue;

				MapState::Quad abs = this->map->tileCoordToAbs(this->_gridPos + Vector2{gx, gy});
				Vector2f center = (abs.pt1 + abs.pt2 + abs.pt3 + abs.pt4) / 4;
				double d = pos.distance2(center);

				if (d >= dist)
					continue;
				dist = d;
				ocenter = center;
			}
		}
	} else
		ocenter = this->_position - Vector2i{0, this->_altitude} + this->_size / 2;
	return ocenter;
}

const ObjectWeapon *GameObject::getBestWeaponFor(const GameObject *obj, unsigned barrel, bool isFree) const
{
	const std::vector<std::string> &tags = obj->getTags();
	bool air = obj->getAltitude() > 0;
	bool gnd = obj->getAltitude() == 0;
	bool und = obj->getAltitude() < 0;

	for (const std::string &tag : tags) {
		auto it = this->_weaponByTag.find(tag);

		if (it == this->_weaponByTag.end())
			continue;

		for (auto &weapon : it->second) {
			if (barrel != UINT32_MAX && weapon->barrel != barrel)
				continue;
			if (isFree && this->_barrels[weapon->barrel].shootTimer != 0)
				continue;
			if (air) {
				if (!weapon->weapon->hitsAir())
					continue;
			} else if (gnd) {
				if (!weapon->weapon->hitsGround())
					continue;
			}
			return weapon;
		}
	}
	return nullptr;
}

ObjectWeapon *GameObject::getBestWeaponFor(const GameObject *obj, unsigned barrel, bool isFree)
{
	return const_cast<ObjectWeapon *>(static_cast<const GameObject *>(this)->getBestWeaponFor(obj, barrel, isFree));
}

bool GameObject::canAttack(const GameObject *obj) const
{
	return this->getBestWeaponFor(obj, UINT32_MAX, false) != nullptr;
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

#undef turnTimer
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

ObjectTurret::ObjectTurret(GameObject *parent, const Mod &base, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &json) :
	parent(parent)
{
	assert_exp(json.contains("anim") && json["anim"].is_object());
	assert_exp(json.contains("barrel") && json["barrel"].is_number());
	assert_exp(!json.contains("fixed") || json["fixed"].is_boolean());
	assert_exp(!json.contains("offset") || json["offset"].is_object());
	assert_exp(!json.contains("recoil") || json["recoil"].is_number());
	assert_exp(!json.contains("turn_delay") || json["turn_delay"].is_number());
	assert_exp(!json.contains("directions") || json["directions"].is_number());

	size_t directions = parent->getDirections();

	if (json.contains("directions")) {
		directions = json["directions"].get<unsigned>();
		assert_msg(directions % 8 == 0, "Number of directions must be divisible by 8");
	}
	if (directions == 1)
		this->anims.emplace_back(base, parent, animList, json["anim"]);
	else for (size_t i = 0; i < directions; i++)
		this->anims.emplace_back(base, parent, animList, json["anim"], i);
	this->barrel = json["barrel"];
	assert_exp(parent->_barrels.size() > this->barrel);
	assert_exp(parent->_barrels[this->barrel].turret == nullptr);
	parent->_barrels[this->barrel].turret = this;
	if (json.contains("turn_delay"))
		this->turnDelay = json["turn_delay"].get<unsigned>() + 1;
	if (json.contains("recoil"))
		this->recoilMax = json["recoil"];
	if (json.contains("fixed"))
		this->fixed = json["fixed"];
	if (json.contains("offset")) {
		assert_exp(!json["offset"].contains("x") || json["offset"]["x"].is_number());
		assert_exp(!json["offset"].contains("y") || json["offset"]["y"].is_number());
		this->offset = { json["offset"]["x"], json["offset"]["y"] };
	}
}

ObjectTurret::ObjectTurret(GameObject *parent, const ObjectTurret &other) :
	parent(parent)
{
	for (auto &anim : other.anims)
		this->anims.emplace_back(parent, anim);
	this->fixed = other.fixed;
	this->offset = other.offset;
	this->barrel = other.barrel;
	parent->_barrels[this->barrel].turret = this;
	this->direction = other.direction;
	this->recoilTimer = other.recoilTimer;
	this->recoilMax = other.recoilMax;
}

void ObjectTurret::attack()
{
	this->recoilTimer = this->recoilMax;
	this->shotTimer = 300;
	this->fast = true;
}

void ObjectTurret::update()
{
	if (this->fixed)
		this->direction = this->parent->getDirection() * this->anims.size() / this->parent->getDirections();
	if (this->recoilTimer)
		this->recoilTimer--;
	if (this->independent) {
		if (this->shotTimer == 0 && this->parent->getDirections() != 1)
			this->directionTarget = this->parent->getDirection() * this->anims.size() / this->parent->getDirections();
		else
			this->shotTimer--;
	}
	if (!this->fixed) {
		if (this->direction != this->directionTarget) {
			unsigned delay = this->fast ? this->turnDelay : this->turnDelay * 8;

			this->turnTimer++;
			if (this->turnTimer >= delay) {
				size_t diff = (this->direction - this->directionTarget + this->anims.size()) % this->anims.size();

				this->turnTimer = 0;
				if (diff >= this->anims.size() / 2)
					this->direction = (this->direction + 1) % this->anims.size();
				else
					this->direction = (this->direction - 1 + this->anims.size()) % this->anims.size();
			}
		} else
			this->fast = false;
	}
}

void ObjectTurret::render(int layer)
{
	double angle = M_PI * 2 * this->direction / this->anims.size() - M_PI_2 - M_PI;
	Vector2d recoilOffset = {(std::log(this->recoilMax + 1) - std::log(this->recoilMax + 1 - this->recoilTimer)) * 2, 0};

	if (this->anims[this->direction].getLayer() == layer)
		this->parent->_displayAnim(this->anims[this->direction].getAnim(), this->offset + recoilOffset.rotation(angle, {0, 0}));
}

ObjectDeathParams::ObjectDeathParams(
	const Mod &base,
	const GameObject *obj,
	const std::unordered_map<std::string, std::vector<Animation>> &animMap,
	const std::unordered_map<std::string, Effect> &effectMap,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
)
{
	assert_exp(!json.contains("anim") || json["anim"].is_object());
	assert_exp(!json.contains("fade") || json["fade"].is_number_unsigned());
	assert_exp(!json.contains("stay") || json["stay"].is_number_unsigned());
	assert_exp(!json.contains("effect") || json["effect"].is_string());
	assert_exp(!json.contains("count") || json["count"].is_number());
	assert_exp(!json.contains("variance") || json["variance"].is_object());
	assert_exp(!json.contains("shake") || json["shake"].is_object());
	assert_exp(!json.contains("per_tile") || json["per_tile"].is_boolean());
	assert_exp(!json.contains("projectiles") || json["projectiles"].is_object());
	assert_exp(!json.contains("units") || json["units"].is_object());

	if (json.contains("anim"))
		this->anim.emplace(base, obj, animMap, json["anim"]);
	if (json.contains("fade"))
		this->fadeTime.emplace(json["fade"]);
	if (json.contains("stay"))
		this->stayTime.emplace(json["stay"]);
	if (json.contains("count"))
		this->count = json["count"];
	if (json.contains("per_tile"))
		this->perTile = json["per_tile"];
	if (json.contains("effect")) {
		std::string effect = RESOURCE(base.getName(), json["effect"]);

		assert_msg(effectMap.contains(effect), effect);
		this->effect = &effectMap.at(effect);
	}
	if (json.contains("variance")) {
		assert_exp(json["variance"].contains("x") && json["variance"]["x"].is_number());
		assert_exp(json["variance"].contains("y") && json["variance"]["y"].is_number());
		this->variance.x = json["variance"]["x"];
		this->variance.y = json["variance"]["y"];
	}
	if (json.contains("units")) {
		for (auto &[key, nb] : json["units"].items()) {
			assert_msg(nb.is_number_unsigned(), "json[\"units\"][\"" + key + "\"]");
			this->units[RESOURCE(base.getName(), key)] = nb;
		}
	}
	if (json.contains("projectiles")) {
		assert_exp(json["projectiles"].contains("pool") && json["projectiles"]["pool"].is_array());
		for (size_t i = 0; i < json["projectiles"]["pool"].size(); i++)
			assert_msg(json["projectiles"]["pool"][i].is_string(), std::to_string(i));
		assert_exp(json["projectiles"].contains("count") && (
			(json["projectiles"]["count"].is_array() && json["projectiles"]["count"].size() == 2) ||
			json["projectiles"]["count"].is_number()
		));
		assert_exp(json["projectiles"].contains("target") && json["projectiles"]["target"].is_object());
		assert_exp(json["projectiles"]["target"].contains("x") && (
			(json["projectiles"]["target"]["x"].is_array() && json["projectiles"]["target"]["x"].size() == 2) ||
			json["projectiles"]["target"]["x"].is_number()
		));
		assert_exp(json["projectiles"]["target"].contains("y") && (
			(json["projectiles"]["target"]["y"].is_array() && json["projectiles"]["target"]["y"].size() == 2) ||
			json["projectiles"]["target"]["y"].is_number()
		));

		std::vector<std::string> pool = json["projectiles"]["pool"];

		for (size_t i = 0; i < pool.size(); i++) {
			std::string weapon = RESOURCE(base.getName(), pool[i]);

			assert_msg(weapons.contains(weapon), "json[\"projectiles\"][\"pool\"][" + std::to_string(i) + "]: " + weapon);
			this->projectilesPool.push_back(&weapons.at(weapon));
		}
		if (json["projectiles"]["count"].is_number())
			this->projectilesCount = {
				json["projectiles"]["count"],
				json["projectiles"]["count"]
			};
		else
			this->projectilesCount = {
				json["projectiles"]["count"][0],
				json["projectiles"]["count"][1]
			};

		if (json["projectiles"]["target"]["x"].is_number()) {
			this->projectilesTarget.first.x  = json["projectiles"]["target"]["x"];
			this->projectilesTarget.second.x = json["projectiles"]["target"]["x"];
		} else {
			this->projectilesTarget.first.x  = json["projectiles"]["target"]["x"][0];
			this->projectilesTarget.second.x = json["projectiles"]["target"]["x"][1];
		}
		if (json["projectiles"]["target"]["y"].is_number()) {
			this->projectilesTarget.first.y  = json["projectiles"]["target"]["y"];
			this->projectilesTarget.second.y = json["projectiles"]["target"]["y"];
		} else {
			this->projectilesTarget.first.y  = json["projectiles"]["target"]["y"][0];
			this->projectilesTarget.second.y = json["projectiles"]["target"]["y"][1];
		}
	}
	if (json.contains("shake"))
		TRY {
			this->shake = std::make_unique<ShakeParams>(json["shake"]);
		} CATCH_AND_RETHROW("In key \"shake\"");
}

ObjectDeathParams::ObjectDeathParams(const GameObject *obj, const ObjectDeathParams &other)
{
	if (other.anim) this->anim.emplace(obj, *other.anim);
	this->fadeTime = other.fadeTime;
	this->stayTime = other.stayTime;
	this->effect = other.effect;
	this->count = other.count;
	this->variance = other.variance;
	this->perTile = other.perTile;
	this->units = other.units;
	this->projectilesPool = other.projectilesPool;
	this->projectilesTarget = other.projectilesTarget;
	this->projectilesCount = other.projectilesCount;
	if (other.shake) this->shake = std::make_unique<ShakeParams>(*other.shake);
}

ObjectSpawnParams::ObjectSpawnParams(const nlohmann::json &json)
{
	assert_exp(json.contains("type") && json["type"].is_string());

	std::string type = json["type"];

	this->pos = Position(json);
	if (type == "appear")
		this->type = TYPE_APPEAR;
	else
		assert_not_reached_msg(type);
}

ObjectWeapon::ObjectWeapon(unsigned short extraPrio, unsigned barrel, const Mod &base, const std::unordered_map<std::string, Weapon> &weapons, const nlohmann::json &j)
{
	assert_exp(j.contains("name") && j["name"].is_string());
	assert_exp(j.contains("cooldown") && j["cooldown"].is_number());
	assert_exp(!j.contains("priority") || j["priority"].is_number());

	std::string weaponName = RESOURCE(base.getName(), j["name"].get<std::string>());

	assert_msg(weapons.contains(weaponName), weaponName);
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

void GameObjectData::AnimationState::pushUserType(sol::state &state)
{
	sol::usertype<AnimationState> type = state.new_usertype<AnimationState>("GameObject_AnimationState");

	type["health"] = &AnimationState::health;
	type["storage_key"] = &AnimationState::storageKey;
}

void UnitDeployParams::SpawnBuilding::pushUserType(sol::state &state)
{
	sol::usertype<SpawnBuilding> type = state.new_usertype<SpawnBuilding>("UnitDeployParams_SpawnBuilding");

	type["building"] = &SpawnBuilding::building;
	type["queue"] = &SpawnBuilding::queue;
	type["offset"] = &SpawnBuilding::offset;
}

void UnitDeployParams::pushUserType(sol::state &state)
{
	SpawnBuilding::pushUserType(state);

	sol::usertype<UnitDeployParams> type = state.new_usertype<UnitDeployParams>("UnitDeployParams");

	type["direction"] = &UnitDeployParams::direction;
	type["spawnBuilding"] = &UnitDeployParams::spawnBuilding;
}

void GameObject::pushUserType(sol::state &state)
{
	state.new_enum<MoveReason>("GameObject_MoveReason", {
		{"MOVE_COMMAND",         MOVEREASON_MOVE_COMMAND},
		{"FORCE_MOVE_COMMAND",   MOVEREASON_FORCE_MOVE_COMMAND},
		{"ATTACK_COMMAND",       MOVEREASON_ATTACK_COMMAND},
		{"FORCE_ATTACK_COMMAND", MOVEREASON_FORCE_ATTACK_COMMAND},
		{"HARVEST_COMMAND",      MOVEREASON_HARVEST_COMMAND},
		{"HARVEST",              MOVEREASON_HARVEST},
		{"DUMP_COMMAND",         MOVEREASON_DUMP_COMMAND},
		{"DUMP",                 MOVEREASON_DUMP},
		{"AUTO_AGRO",            MOVEREASON_AUTO_AGRO},
		{"REPAIR",               MOVEREASON_REPAIR},
		{"MOVE_ASIDE",           MOVEREASON_MOVE_ASIDE},
		{"MOVE_SPAWN",           MOVEREASON_MOVE_SPAWN},
		{"FINISH_MOVE",          MOVEREASON_FINISH_MOVE},
		{"DELIVERY",             MOVEREASON_DELIVERY},
	});

	AnimationState::pushUserType(state);
	UnitDeployParams::pushUserType(state);

	sol::usertype<GameObject> type = state.new_usertype<GameObject>("GameObject");

	type["update"] = &GameObject::update;
	type["render"] = &GameObject::render;
	type["deploy"] = &GameObject::deploy;
	type["built"] = &GameObject::built;
	type["exec_cb"] = &GameObject::execCb;
	type["crushedBy"] = &GameObject::crushedBy;
	type["bottom"] = sol::property(&GameObject::getBottom);
	type["draw_skirt"] = &GameObject::drawSkirt;
	type["heal"] = &GameObject::heal;
	type["damage"] = &GameObject::damage;
	type["harvest"] = sol::overload(
		sol::resolve<void(const Vector2i &, bool, bool)>(&GameObject::harvest),
		sol::resolve<void()>(&GameObject::harvest)
	);
	type["attack"] = &GameObject::attack;
	type["move_out"] = &GameObject::moveOut;
	type["move_to"] = &GameObject::moveTo;
	type["move_aside"] = &GameObject::moveAside;
	type["dump_resources_to"] = &GameObject::dumpResourcesTo;
	type["pickup"] = &GameObject::pickup;
	type["deliver_to"] = sol::overload(
		sol::resolve<void(const std::vector<GameObject *> &, Vector2f)>(&GameObject::deliverTo),
		sol::resolve<void(const std::vector<std::string> &, const std::string &, const Position &, const MapState &)>(&GameObject::deliverTo)
	);
	type["select"] = &GameObject::select;
	type["play_built_sound"] = &GameObject::playBuiltSound;
	type["clear_orders"] = &GameObject::clearOrders;
	type["render_path_finding_path"] = &GameObject::renderPathFindingPath;
	type["play_do_sound"] = &GameObject::playDoSound;
	type["clear_primary_dump"] = &GameObject::clearPrimaryDump;
	type["produced"] = &GameObject::produced;
	type["set_primary"] = &GameObject::setPrimary;
	type["toggle_repair"] = &GameObject::toggleRepair;
	type["sell"] = &GameObject::sell;
	type["next_direction"] = &GameObject::nextDirection;
	type["set_owner"] = &GameObject::setOwner;
	type["spawn"] = &GameObject::spawn;

	// setColor() has no matching getColor(), so this is write-only (not a
	// get/set pair like the other properties below).
	type["color"] = sol::writeonly_property(&GameObject::setColor);
	type["can_move"] = sol::property(&GameObject::canMove);
	type["storage_display"] = sol::property(&GameObject::getStorageDisplay);
	type["health"] = sol::property(&GameObject::getHealth, &GameObject::setHealth);
	type["can_deploy"] = sol::property(&GameObject::canDeploy);
	type["deployable"] = sol::property(&GameObject::isDeployable);
	type["middle"] = sol::property(&GameObject::getMiddle);
	type["being_picked_up"] = sol::property(&GameObject::isBeingPickedUp);
	type["can_pickup"] = sol::property(&GameObject::canPickup);
	type["altitude"] = sol::property(&GameObject::getAltitude, &GameObject::setAltitude);
	type["ignores_terrain"] = sol::property(&GameObject::ignoresTerrain);
	type["directions"] = sol::property(&GameObject::getDirections);
	type["facing_angle"] = sol::property(&GameObject::getFacingAngle);
	type["direction"] = sol::property(&GameObject::getDirection, &GameObject::setDirection);
	type["in_object"] = sol::property(&GameObject::isInObject);
	type["hitbox_center"] = sol::property(&GameObject::getHitboxCenter);
	type["can_attack"] = sol::property(&GameObject::canAttack);
	type["tags"] = sol::property(&GameObject::getTags);
	type["on_deploy"] = sol::property(&GameObject::getOnDeploy);
	type["speed"] = sol::property(&GameObject::getSpeed);
	type["power"] = sol::property(&GameObject::getPower);
	type["build_max_distance"] = sol::property(&GameObject::getBuildMaxDistance);
	type["radar"] = sol::property(&GameObject::isRadar);
	type["crushable"] = sol::property(&GameObject::isCrushable);
	type["selectable"] = sol::property(&GameObject::isSelectable);
	type["sellable"] = sol::property(&GameObject::isSellable);
	type["repairable"] = sol::property(&GameObject::isRepairable);
	type["primary"] = sol::property(&GameObject::isPrimary);
	type["producing"] = sol::property(&GameObject::isProducing, &GameObject::producing);
	type["given_queue"] = sol::property(&GameObject::getGivenQueue);
	type["storage"] = sol::property(&GameObject::getStorage);
	type["central"] = sol::property(&GameObject::isCentral);
	type["extra_spawn"] = sol::property(&GameObject::getExtraSpawn);
	type["point"] = sol::property(&GameObject::getPoint);
	type["spawn_orders"] = sol::property(&GameObject::getSpawnOrders);
	type["accept_resources"] = sol::property(&GameObject::getAcceptResources);
	type["resource_storage"] = sol::property(&GameObject::getResourceStorage);
	type["fake"] = sol::property(&GameObject::isFake);
	type["requires_placement"] = sol::property(&GameObject::requiresPlacement);
	type["proxy"] = sol::property(&GameObject::getProxy);
	type["id"] = sol::property(&GameObject::getId);
	type["max_health"] = sol::property(&GameObject::getMaxHealth);
	type["dead"] = sol::property(&GameObject::isDead);
	type["disposable"] = sol::property(&GameObject::isDisposable);
	type["select_box"] = sol::property(&GameObject::getSelectBox);
	type["size"] = sol::property(&GameObject::getSize);
	type["position"] = sol::property(&GameObject::getPosition, &GameObject::setPosition);
	type["spawn_offset"] = sol::property(&GameObject::getSpawnOffset);
	type["exit_offset"] = sol::property(&GameObject::getExitOffset);
	type["base_anims"] = sol::property(&GameObject::getBaseAnims);
	type["base"] = sol::property(&GameObject::getBase);
	type["faction"] = sol::property(&GameObject::getFaction);
	type["build_speed"] = sol::property(&GameObject::getBuildSpeed);
	type["group"] = sol::property(&GameObject::getGroup);
	type["armor"] = sol::property(&GameObject::getArmor);
	type["obstructed"] = sol::property(&GameObject::isObstructed);
	type["limit"] = sol::property(&GameObject::getLimit);
	type["pass_flags"] = sol::property(&GameObject::getPassFlags);
	type["solid"] = sol::property(&GameObject::isSolid);
	type["stackable"] = sol::property(&GameObject::isStackable);
	type["cost"] = sol::property(&GameObject::getCost);
	type["check_tiles"] = sol::property(&GameObject::getCheckTiles);
	type["build_queue"] = sol::property(&GameObject::getBuildQueue);
	type["animation_state"] = &GameObject::animationState;
	// TODO: getRequirements() (ReqAndCombinator holds non-copyable
	//       std::unique_ptr<IReq> and isn't itself registered, same reason as
	//       Upgrade::getRequirements), getTileData() (raw BuildingMask bit-grid,
	//       internal collision data) and initGlobalGrid()/deinitGlobalGrid()
	//       (internal engine lifecycle, not meant to be called from scripts) are
	//       intentionally not exposed.
}
