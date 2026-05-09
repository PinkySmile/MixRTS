//
// Created by PinkySmile on 20/04/2026.
//

#include "Faction.hpp"
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "Animation.hpp"
#include "ConditionalAnim.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

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
	_position(g._position),
	_gridPos(g._gridPos),
	_spawnOffset(g._spawnOffset),
	_exitOffset(g._exitOffset),
	_currentDir(g._currentDir),
	_turnSpeed(g._turnSpeed),
	_buildSpeed(g._buildSpeed),
	_maxHp(g._maxHp),
	_hp(g._hp),
	_sight(g._sight),
	_limit(g._limit),
	_gpTimer(g._gpTimer),
	_speed(g._speed),
	_passFlags(g._passFlags),
	_index(g._index),
	_tileCount(g._tileCount),
	_requiresQueueBuilding(g._requiresQueueBuilding),
	_boundToGrid(g._boundToGrid),
	_disposable(g._disposable),
	_pathCached(g._pathCached),
	_initGrid(g._initGrid),
	_solid(g._solid),
	_fake(g._fake),
	animationState(g.animationState),
	pfGrid(g.pfGrid),
	manager(g.manager),
	map(g.map),
	owner(g.owner)
{
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
	assert_exp(!json.contains("queue") || json["queue"].is_string());
	assert_exp(!json.contains("build") || json["build"].is_string());
	assert_exp(!json.contains("speed") || json["speed"].is_number());
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
	assert_exp(!json.contains("tiles") || json["tiles"].is_array());
	if (json.contains("tiles"))
		for (size_t i = 0; i < json["tiles"].size(); i++) {
			assert_msg(json["tiles"][i].is_array(), "json[\"tiles\"][" + std::to_string(i) + "]");
			for (size_t j = 0; j < json["tiles"][i].size(); j++)
				assert_msg(json["tiles"][i][j].is_number(), "json[\"tiles\"][" + std::to_string(i) + "][" + std::to_string(j) + "]");
		}
	assert_exp(json.contains("hp") && json["hp"].is_number());
	assert_exp(json.contains("cost") && json["cost"].is_object());
	for (auto &[key, value] : json["cost"].items())
		assert_msg(value.is_number(), "json[\"cost\"][\"" + key + "\"]");
	assert_exp(!json.contains("select") || json["select"].is_object());
	assert_exp(json.contains("size") && json["size"].is_object());
	assert_exp(json["size"].contains("x") && json["size"]["x"].is_number());
	assert_exp(json["size"].contains("y") && json["size"]["y"].is_number());
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
	assert_exp(!json.contains("directions") || json["directions"].is_number_unsigned());
	assert_exp(!json.contains("build_speed") || json["build_speed"].is_number_unsigned());
	assert_exp(!json.contains("group") || json["group"].is_string());
	assert_exp(json.contains("requirements") && json["requirements"].is_array());
	assert_exp(json.contains("anims") && json["anims"].is_array());
	assert_exp(!json.contains("sight") || json["sight"].is_number());
	assert_exp(!json.contains("turn_speed") || json["turn_speed"].is_number());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());
	assert_exp(!json.contains("pass_flags") || json["pass_flags"].is_string() || json["pass_flags"].is_number_unsigned());
	assert_exp(!json.contains("fake") || json["fake"].is_boolean());
	assert_exp(!json.contains("proxy") || json["proxy"].is_string());

	auto &base = faction.getBase();

	this->_id = id;
	this->_hp = json["hp"];
	this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);

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

bool GameObject::_handleOrder(const Order &o)
{
	if (auto turn = std::get_if<TurnOrder>(&o)) {
		if (this->_currentDir == turn->resultDir)
			return true;
		this->_gpTimer++;
		if (this->_gpTimer > this->_turnSpeed) {
			size_t diff = (this->_currentDir - turn->resultDir + this->_anims.size()) % this->_anims.size();

			this->_gpTimer = 0;
			if (diff >= this->_anims.size() / 2)
				this->_currentDir = (this->_currentDir + 1) % this->_anims.size();
			else
				this->_currentDir = (this->_currentDir - 1 + this->_anims.size()) % this->_anims.size();
		}
		return false;
	} else if (auto move = std::get_if<MoveOrder>(&o)) {
		if (!this->_pathCached) {
			this->_cachedDest = move->destination;
			if (this->_boundToGrid) {
				AStar::computePath(
					this->map->pathFindingGrid,
					this->_gridPos,
					this->map->absToTileCoord(move->destination),
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
					move->destination,
					this->_size,
					{1, 1},
					this->_passFlags,
					this->_pathCache
				);
			while (!this->_pathCache.empty() && this->_pathCache.back() == this->_position)
				this->_pathCache.pop_back();
			this->_cachedDebug = AStar::getDebugImage();
			this->_cachedDebugTexLoaded = false;
			this->_pathCached = true;
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
	return true;
}

void GameObject::playBuiltSound() const
{
	if (this->_buildSound)
		this->_buildSound->play();
}

void GameObject::update()
{
	for (auto &anim : this->_anims[this->_currentDir])
		if (anim())
			anim.update();
	if (!this->_orderQueue.empty()) {
		if (this->_firstEvent)
			this->_pathCached = false;
		if (this->_handleOrder(this->_orderQueue.front())) {
			this->_orderQueue.pop_front();
			this->_firstEvent = true;
		} else
			this->_firstEvent = false;
	}
}

void GameObject::render(int layer)
{
	for (auto &anim : this->_anims[this->_currentDir])
		if (anim() && anim.getLayer() == layer)
			this->_displayAnim(anim.getAnim());
}

void GameObject::setPosition(const Vector2f &pos)
{
	// OPTIMIZE: Diff instead of undoing it and redoing it
	this->deinitGlobalGrid();
	this->_position = pos;
	this->_gridPos = this->map->absToTileCoord(pos + this->_size / 2);
	this->initGlobalGrid();
}

void GameObject::setColor(const HSLColor &color)
{
	for (auto &block : this->_anims)
		for (auto &anim : block)
			anim.setColor(color);
}

const std::string &GameObject::getId() const
{
	return this->_id;
}

unsigned GameObject::getHealth() const
{
	return this->_hp;
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
	return this->_hp == 0;
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
}

bool GameObject::canDeploy() const
{
	return false;
}

bool GameObject::canMove() const
{
	return this->_speed;
}

void GameObject::moveTo(const Vector2i &pos)
{
	if (!this->canMove())
		return;
	if (this->_boundToGrid)
		this->_orderQueue.emplace_back(MoveOrder{this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos))});
	else
		this->_orderQueue.emplace_back(MoveOrder{pos + this->_size / 2});
}

void GameObject::clearOrders()
{
	this->_orderQueue.clear();
	this->_firstEvent = true;
}

void GameObject::initGlobalGrid()
{
	if (this->_initGrid)
		return;
	if (this->_tiles) {
		for (size_t gy = 0; gy < this->_tiles->size(); gy++) {
			auto &line = (*this->_tiles)[gy];

			for (size_t gx = 0; gx < line.size(); gx++) {
				if (!(line[gx] & MASK_SOLID))
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					this->_tileCount
				);
			}
		}
	} else {
		if (this->_boundToGrid) {
			this->manager->modUnitInArea(
				{
					this->_position,
					{this->_position.x + this->_size.x, this->_position.y},
					{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
					{this->_position.x, this->_position.y + this->_size.y},
				},
				this->map->pathFindingGrid,
				this->_gridPos,
				this->map->tileSize,
				this->_tileCount
			);
		} else {
			this->manager->modUnitInArea(
				{
					this->_position,
					{this->_position.x + this->_size.x, this->_position.y},
					{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
					{this->_position.x, this->_position.y + this->_size.y},
				},
				this->_tileCount
			);
		}
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
				if (!(line[gx] & MASK_SOLID))
					continue;
				this->manager->modUnitInArea(
					this->map->tileCoordToAbs({
						static_cast<int>(this->_gridPos.x + gx),
						static_cast<int>(this->_gridPos.y + gy)
					}),
					-this->_tileCount
				);
			}
		}
	} else {
		if (this->_boundToGrid) {
			this->manager->modUnitInArea(
				{
					this->_position,
					{this->_position.x + this->_size.x, this->_position.y},
					{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
					{this->_position.x, this->_position.y + this->_size.y},
				},
				this->map->pathFindingGrid,
				this->_gridPos,
				this->map->tileSize,
				-this->_tileCount
			);
		} else {
			this->manager->modUnitInArea(
				{
					this->_position,
					{this->_position.x + this->_size.x, this->_position.y},
					{this->_position.x + this->_size.x, this->_position.y + this->_size.y},
					{this->_position.x, this->_position.y + this->_size.y},
				},
				-this->_tileCount
			);
		}
	}
	this->manager->deregisterObject(this);
	this->_initGrid = false;
}

bool GameObject::isSolid() const
{
	return this->_solid;
}

void GameObject::setIndex(unsigned int index)
{
	this->_index = index;
}

unsigned GameObject::getIndex() const
{
	return this->_index;
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

	if (!this->_cachedDebugTexLoaded) {
		assert_exp(this->_cachedDebugTex.loadFromImage(this->_cachedDebug));
		this->_cachedDebugTexLoaded = true;
	}

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

	sf::Sprite sprite{this->_cachedDebugTex};
	auto scale = this->pfGrid->gridSize;

	scale.x /= this->_cachedDebugTex.getSize().x;
	scale.y /= this->_cachedDebugTex.getSize().y;
	sprite.setPosition({0, 0});
	sprite.setScale(scale);
	game->screen->draw(sprite);
}
