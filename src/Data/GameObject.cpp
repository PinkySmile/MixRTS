//
// Created by PinkySmile on 20/04/2026.
//

#include "Faction.hpp"
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "Animation.hpp"
#include "Player.hpp"
#include "ConditionalAnim.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

#define turnTimer _gpTimer[0]
#define harvestTimer _gpTimer[0]
#define cancel _gpTimer[1]
#define harvestDelaying _gpTimer[2]
#define dumpTimer _gpTimer[0]

bool GameObject::_shortenOrder(Order &o)
{
	if (std::holds_alternative<MoveOrder>(o)) {
		if (!this->_boundToGrid)
			return true;

		float x = std::fmod(this->_position.x, this->map->tileSize.x);
		float y = std::fmod(this->_position.y, this->map->tileSize.y);

		if (x == 0 && y == 0)
			return true;
		this->_pathCache.erase(this->_pathCache.begin() + 1, this->_pathCache.end());

		Vector2f pos = this->_position;

		// TODO: Handle non-square grids
		if (this->_currentDir == 1 * this->_anims.size() / 8) {
			pos.x += this->map->tileSize.x - x;
			pos.y -= y;
		} else if (this->_currentDir == 3 * this->_anims.size() / 8) {
			pos.x += this->map->tileSize.x - x;
			pos.y += this->map->tileSize.y - y;
		} else if (this->_currentDir == 5 * this->_anims.size() / 8) {
			pos.x -= x;
			pos.y += this->map->tileSize.y - y;
		} else if (this->_currentDir == 7 * this->_anims.size() / 8) {
			pos.x -= x;
			pos.y -= y;
		} else if (this->_currentDir == 0 * this->_anims.size() / 8)
			pos.y -= y;
		else if (this->_currentDir == 2 * this->_anims.size() / 8)
			pos.x += this->map->tileSize.x - x;
		else if (this->_currentDir == 4 * this->_anims.size() / 8)
			pos.y += this->map->tileSize.y - y;
		else if (this->_currentDir == 6 * this->_anims.size() / 8)
			pos.x -= x;

		this->_cachedDest = pos;
		this->_pathCache.front() = this->_cachedDest;
		return false;
	} else if (std::holds_alternative<HarvestOrder>(o)) {
		this->cancel = true;
		return false;
	}
	return true;
}

void GameObject::_displayAnim(Animation &anim)
{
	Sprite &sprite = anim.getSprite();

	sprite.setPosition({
		sprite.getPosition().x + this->_position.x,
		sprite.getPosition().y + this->_position.y
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
	_base(g._base),
	_buildSound(g._buildSound),
	_doSound(g._doSound),
	_selectSound(g._selectSound),
	_checkTiles(g._checkTiles),
	_tiles(g._tiles),
	_cost(g._cost),
	_requirements(g._requirements),
	_buildQueue(g._buildQueue),
	_proxy(g._proxy),
	_pathCache(g._pathCache),
	_orderQueue(g._orderQueue),
	_group(g._group),
	_id(g._id),
	_selectBox(g._selectBox),
	_size(g._size),
	_point(g._point),
	_position(g._position),
	_gridPos(g._gridPos),
	_extraSpawn(g._extraSpawn),
	_spawnOrders(g._spawnOrders),
	_acceptResources(g._acceptResources),
	_lastRefinery(g._lastRefinery),
	_spawnOffset(g._spawnOffset),
	_exitOffset(g._exitOffset),
	_resourceStorage(g._resourceStorage),
	_currentDir(g._currentDir),
	_turnSpeed(g._turnSpeed),
	_buildSpeed(g._buildSpeed),
	_maxHp(g._maxHp),
	_hp(g._hp),
	_sight(g._sight),
	_limit(g._limit),
	_gpTimer(g._gpTimer),
	_passFlags(g._passFlags),
	_harvestTime(g._harvestTime),
	_harvestDelay(g._harvestDelay),
	_speed(g._speed),
	_tileCount(g._tileCount),
	_requiresQueueBuilding(g._requiresQueueBuilding),
	_boundToGrid(g._boundToGrid),
	_disposable(g._disposable),
	_pathCached(g._pathCached),
	_initGrid(g._initGrid),
	_solid(g._solid),
	_fake(g._fake),
	_central(g._central),
	_dumpStop(g._dumpStop),
	_dumpStart(g._dumpStart),
	_dumping(g._dumping),
	index(g.index),
	animationState(g.animationState),
	pfGrid(g.pfGrid),
	manager(g.manager),
	map(g.map),
	owner(g.owner)
{
	this->_harvestAnims.reserve(g._harvestAnims.size());
	for (auto &anim : g._harvestAnims)
		this->_harvestAnims.emplace_back(*this, anim);

	if (g._dump)
		this->_dump.emplace(
			g._dump->dir,
			g._dump->delay,
			g._dump->delay_after,
			std::make_unique<ConditionalAnim>(*this, *g._dump->during),
			std::make_unique<ConditionalAnim>(*this, *g._dump->start),
			std::make_unique<ConditionalAnim>(*this, *g._dump->stop)
		);

	this->_anims.reserve(g._anims.size());
	for (auto &animVec : g._anims) {
		this->_anims.emplace_back();
		this->_anims.back().reserve(animVec.size());
		for (auto &anim : animVec)
			this->_anims.back().emplace_back(*this, anim);
	}
}

GameObject::GameObject(
	const Faction &faction,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const nlohmann::json &json
) :
	_base(faction)
{
	assert_exp(!json.contains("requires_queue_building") || json["requires_queue_building"].is_boolean());
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
	assert_exp(json.contains("anims") && json["anims"].is_array());
	assert_exp(!json.contains("sight") || json["sight"].is_number());
	assert_exp(!json.contains("turn_speed") || json["turn_speed"].is_number());
	assert_exp(!json.contains("do_sound") || json["do_sound"].is_string());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());
	assert_exp(!json.contains("pass_flags") || json["pass_flags"].is_string() || json["pass_flags"].is_number_unsigned());
	assert_exp(!json.contains("fake") || json["fake"].is_boolean());
	assert_exp(!json.contains("proxy") || json["proxy"].is_string());
	assert_exp(!json.contains("harvest_anim") || json["harvest_anim"].is_object());
	assert_exp(!json.contains("resource_storage") || json["resource_storage"].is_number_unsigned());
	assert_exp(!json.contains("harvest_time") || json["harvest_time"].is_number_unsigned());
	assert_exp(!json.contains("harvest_delay") || json["harvest_delay"].is_number_unsigned());
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

	auto &base = faction.getBase();

	this->_id = id;
	this->_hp = json["hp"];
	this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);

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
	if (json.contains("spawn_offset"))
		this->_spawnOffset = {json["spawn_offset"]["x"], json["spawn_offset"]["y"]};
	if (json.contains("point"))
		this->_point = {json["point"]["x"], json["point"]["y"]};
	if (json.contains("requires_queue_building"))
		this->_requiresQueueBuilding = json["requires_queue_building"];
	if (json.contains("queue"))
		this->_buildQueue = RESOURCE(base.getName(), json["queue"]);
	if (json.contains("tiles"))
		this->_tiles = json["tiles"];
	if (json.contains("fake"))
		this->_fake = json["fake"];
	if (json.contains("proxy"))
		this->_proxy = json["proxy"];
	if (json.contains("build")) {
		auto buildSound = RESOURCE(base.getName(), json["build"]);

		assert_msg(sounds.contains(buildSound), buildSound);
		this->_buildSound = &sounds.at(buildSound);
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
	if (json.contains("speed"))
		this->_speed = json["speed"];
	if (json.contains("group"))
		this->_group = RESOURCE(base.getName(), "group." + json["group"].get<std::string>());
	else
		this->_group = id;

	if (json.contains("turn_speed"))
		this->_turnSpeed = json["turn_speed"];
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
	if (json.contains("directions")) {
		unsigned directions = json["directions"];

		this->_anims.resize(directions);
		for (auto &anim: json["anims"])
			for (size_t i = 0; i < directions; i++)
				this->_anims[i].emplace_back(*this, animList, anim, i);
	} else {
		this->_anims.emplace_back();
		for (auto &anim: json["anims"])
			this->_anims.front().emplace_back(*this, animList, anim);
	}
	if (json.contains("harvest_anim")) {
		assert_exp(!json["harvest_anim"].contains("dirs") || json["harvest_anim"]["dirs"].is_number_unsigned());


		unsigned dirs = json["harvest_anim"]["dirs"];

		for (size_t i = 0; i < dirs; i++)
			this->_harvestAnims.emplace_back(*this, animList, json["harvest_anim"], i);
	}
	if (json.contains("dump"))
		this->_dump.emplace(
			json["dump"]["dir"].get<unsigned>(),
			json["dump"]["delay"].get<unsigned>(),
			json["dump"]["delay_after"].get<unsigned>(),
			std::make_unique<ConditionalAnim>(*this, animList, json["dump"]["during"]),
			std::make_unique<ConditionalAnim>(*this, animList, json["dump"]["start_anim"]),
			std::make_unique<ConditionalAnim>(*this, animList, json["dump"]["stop_anim"])
		);

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

			this->_selectBox.position = anim.getOffset();
			this->_selectBox.size = this->_size;
		}
	}
	this->_sight = json.contains("sight") ? json["sight"].get<int>() : 0;
	this->animationState.health = 1;
}

GameObject::~GameObject() = default;


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

bool GameObject::_updateMove(Vector2i destination, bool force)
{
	if (!this->_pathCached) {
		this->_cachedDest = destination;
		if (force) {
			this->_pathCache.clear();
			this->_pathCache.emplace_back(destination);
		} else {
			if (this->_boundToGrid) {
				AStar::computePath(
					this->map->pathFindingGrid,
					this->_gridPos,
					this->map->absToTileCoord(destination),
					{1, 1},
					{1, 1},
					this->_passFlags,
					this->_pathCache
				);
				for (auto &p : this->_pathCache)
					p = this->map->topLeftCoordToAbs(p);
			} else
				AStar::computePath(
					*this->pfGrid,
					this->_position,
					destination,
					this->_size,
					{1, 1},
					this->_passFlags,
					this->_pathCache
				);
			while (!this->_pathCache.empty() && this->_pathCache.back() == this->_position)
				this->_pathCache.pop_back();
			this->_cachedDebug = AStar::getDebugImage();
			this->_cachedDebugTexLoaded = false;
		}
		this->_pathCached = true;
		if (this->_pathCache.empty()) {
			this->_orderQueue.clear();
			return false;
		}
	}

	if (this->_pathCache.empty())
		return true;

	auto &dest = this->_pathCache.back();
	float a = this->_position.angle(dest);
	unsigned angle = round((a + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

	angle %= this->_anims.size();
	if (angle != this->_currentDir && !this->_handleOrder(TurnOrder{angle}))
		return false;

	auto pos = this->_position;
	auto diff = dest - pos;

	diff.normalize();
	diff *= this->_speed;
	pos += diff;

	auto diff2 = dest - pos;

	if (pos == dest || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0)) {
		this->setPosition(dest);
		this->_pathCache.pop_back();
	} else
		this->setPosition(pos);
	return false;
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
	if (tile) return this->harvest(this->map->topLeftCoordToAbs(pos));
	tile = this->manager->getClosestHarvestableTile(*this->map, this->_gridPos);
	if (tile) return this->harvest(this->map->topLeftCoordToAbs(*tile));
}

void GameObject::_goToClosestRefinery()
{
	if (!this->_lastRefinery)
		this->_lastRefinery = this->manager->getClosestResourceDump(this->_acceptResources, this->_position + this->_size / 2);
	this->dumpResourcesTo(nullptr);
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
				return this->_goToClosestRefinery(), true;
			auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);
			if (tile)
				return false;
			this->_goToClosestResource();
			return true;
		}
	} else {
		if (this->_lots.size() >= this->_resourceStorage)
			return this->_goToClosestRefinery(), true;
		auto tile = this->manager->getFirstHarvestableTile(*this->map, this->_gridPos);
		if (!tile)
			return this->_goToClosestResource(), true;
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

bool GameObject::_updateDump(Building *place)
{
	if (this->_firstEvent) {
		if (this->_currentDir != this->_dump->dir) {
			this->_orderQueue.emplace_front(TurnOrder{this->_dump->dir});
			this->_orderQueue.emplace_front(DumpOrder{place});
			return true;
		}
		if (place == nullptr || place->isDead()) {
			this->_goToClosestRefinery();
			return true;
		}
		if (this->_lots.size() == 0) {
			this->_goToClosestResource();
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
				this->_goToClosestRefinery();
			else
				this->_goToClosestResource();
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
				this->owner->credit(type, content, strategy);
			this->_lots.pop_front();
			place->produced();
		}
		if (this->dumpTimer >= this->_dump->delay + this->_dump->delay_after) {
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

bool GameObject::_handleOrder(const Order &o)
{
	if (auto turn = std::get_if<TurnOrder>(&o))
		return this->_updateTurn(turn->resultDir);
	else if (auto move = std::get_if<MoveOrder>(&o))
		return this->_updateMove(move->destination, move->force);
	else if (std::holds_alternative<HarvestOrder>(o))
		return this->_updateHarvest();
	else if (auto dump = std::get_if<DumpOrder>(&o))
		return this->_updateDump(dump->target);
	return true;
}

void GameObject::playBuiltSound() const
{
	if (this->_buildSound)
		this->_buildSound->play();
}

void GameObject::update()
{
	if(this->_lastRefinery && this->_lastRefinery->isDisposable())
		this->_lastRefinery = nullptr;
	if (!this->_dumpStop && !this->_dumpStart && !this->_dumping)
		for (auto &anim : this->_anims[this->_currentDir])
			if (anim())
				anim.update();
	for (auto &o : this->_orderQueue)
		if (auto dump = std::get_if<DumpOrder>(&o))
			if (dump->target->isDisposable())
				dump->target = nullptr;
	if (this->_dumping && (*this->_dump->during)())
		this->_dump->during->update();
	if (!this->_harvestAnims.empty() && this->_harvesting) {
		auto dir = this->_currentDir * this->_harvestAnims.size() / this->_anims.size();
		auto &anim = this->_harvestAnims[dir];

		if (!anim() || anim.update())
			this->_harvesting = false;
	}
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
}

void GameObject::render(int layer)
{
	if (!this->_dumpStop && !this->_dumpStart && !this->_dumping)
		for (auto &anim : this->_anims[this->_currentDir])
			if (anim() && anim.getLayer() == layer)
				this->_displayAnim(anim.getAnim());
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

void GameObject::setPosition(const Vector2f &pos)
{
	// OPTIMIZE: Diff instead of undoing it and redoing it
	this->deinitGlobalGrid();
	this->_position = pos;
	if (this->_tiles)
		this->_gridPos = this->map->absToTileCoord(pos);
	else
		this->_gridPos = this->map->absToTileCoord(pos + this->_size / 2);
	this->initGlobalGrid();
}

void GameObject::setColor(const HSLColor &color)
{
	for (auto &block : this->_anims)
		for (auto &anim : block)
			anim.setColor(color);
	if (this->_dump) {
		this->_dump->during->setColor(color);
		this->_dump->start->setColor(color);
		this->_dump->stop->setColor(color);
	}
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
	this->_hp += amount;
	if (this->_hp > this->_maxHp)
		this->_hp = this->_maxHp;
	this->animationState.health = static_cast<float>(this->_hp) / static_cast<float>(this->_maxHp);
}

void GameObject::damage(unsigned int amount)
{
	if (this->_hp < amount)
		this->_hp = 0;
	else
		this->_hp -= amount;
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
	return this->_base.getBase();
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

bool GameObject::isObstructed() const
{
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
	if (this->_doSound)
		this->_doSound->play();
}

bool GameObject::canDeploy() const
{
	return false;
}

bool GameObject::canMove() const
{
	return this->_speed != 0;
}

void GameObject::harvest(const Vector2i &pos, bool automatic)
{
	this->moveTo(pos, automatic);
	this->_orderQueue.emplace_back(HarvestOrder{});
}

void GameObject::harvest()
{
	this->_goToClosestResource();
}

void GameObject::moveTo(const Vector2i &pos, bool automatic)
{
	if (!automatic) {
		if (!this->canMove())
			return;
		if (this->_boundToGrid)
			this->_orderQueue.emplace_back(MoveOrder{this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos)), false});
		else
			this->_orderQueue.emplace_back(MoveOrder{pos + this->_size / 2, false});
	} else {
		MoveOrder order;

		if (this->_boundToGrid)
			order = MoveOrder{this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos)), true};
		else
			order = MoveOrder{pos + this->_size / 2, true};
		this->_orderQueue.emplace_back(order);
	}
}

void GameObject::dumpResourcesTo(Building *dump, bool automatic)
{
	if (!dump)
		dump = this->_lastRefinery;
	else
		this->_lastRefinery = dump;
	if (!dump)
		return;

	auto pos = dump->getPosition() + dump->getPoint();

	this->moveTo(pos, automatic);
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
	this->_lastRefinery = nullptr;
}

void GameObject::initGlobalGrid()
{
	if (this->_initGrid)
		return;

	if (this->_tiles) {
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!(line[gx] & MASK_PLACEMENT_REQUIRED))
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					this->owner->state.team, 1,
					(line[gx] & MASK_SOLID) ? this->_tileCount : 0
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
				abs,
				this->map->pathFindingGrid,
				this->_gridPos,
				this->map->tileSize,
				0, 0,
				this->_tileCount
			);
		else
			this->manager->modUnitInArea(abs, 0, 0, this->_tileCount);
	}
	this->manager->registerObject(this);
	this->_initGrid = true;
}

void GameObject::deinitGlobalGrid()
{
	if (!this->_initGrid)
		return;
	if (this->_tiles) {
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!(line[gx] & MASK_PLACEMENT_REQUIRED))
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					this->owner->state.team, -1,
					(line[gx] & MASK_SOLID) ? -this->_tileCount : 0
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
				abs,
				this->map->pathFindingGrid,
				this->_gridPos,
				this->map->tileSize,
				0, 0,
				-this->_tileCount
			);
		else
			this->manager->modUnitInArea(abs, 0, 0, -this->_tileCount);
	}
	this->manager->deregisterObject(this);
	this->_initGrid = false;
}

std::optional<float> GameObject::getStorageDisplay() const
{
	if (this->_resourceStorage != 0)
		return static_cast<float>(this->_lots.size()) / static_cast<float>(this->_resourceStorage);
	return {};
}

bool GameObject::isSolid() const
{
	return this->_solid;
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

const std::optional<std::string> &GameObject::getProxy() const
{
	return this->_proxy;
}

const std::optional<std::vector<std::vector<uint8_t>>> &GameObject::getTileData() const
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

void GameObject::built()
{
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

void GameObject::renderPathFindingPath()
{
	if (!this->_pathCached)
		return;

	sf::VertexArray arr{sf::PrimitiveType::LineStrip, this->_pathCache.size() + 1};
	sf::VertexArray arr2{sf::PrimitiveType::LineStrip, 2};

	arr[0].position = this->_position + this->_size / 2;
	arr[0].color = sf::Color::Blue;
	for (size_t i = this->_pathCache.size(); i > 0; i--) {
		arr[i].position = this->_pathCache[i - 1] + this->_size / 2;
		arr[i].color = sf::Color::Blue;
	}
	game->screen->draw(arr);

	arr2[0].position = this->_position + this->_size / 2;
	arr2[0].color = sf::Color::Green;
	arr2[1].position = this->_cachedDest + this->_size / 2;
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
