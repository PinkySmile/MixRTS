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
	assert_exp(!json.contains("queue") || json["queue"].is_string());
	assert_exp(!json.contains("build") || json["build"].is_string());
	assert_exp(!json.contains("speed") || json["speed"].is_number_unsigned());
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
	assert_exp(!json.contains("tiles") || json["tiles"].is_array());
	if (json.contains("tiles"))
		for (size_t i = 0; i < json["tiles"].size(); i++) {
			assert_msg(json["tiles"][i].is_array(), "json[\"tiles\"][" + std::to_string(i) + "]");
			for (size_t j = 0; j < json["tiles"][i].size(); j++)
				assert_msg(json["tiles"][i][j].is_number(), "json[\"tiles\"][" + std::to_string(i) + "][" + std::to_string(j) + "]");
		}
	assert_exp(json.contains("hp"));
	assert_exp(json["hp"].is_number());
	assert_exp(json.contains("cost"));
	assert_exp(json["cost"].is_object());
	for (auto &[key, value] : json["cost"].items())
		assert_msg(value.is_number(), "json[\"cost\"][\"" + key + "\"]");
	assert_exp(!json.contains("select") || json["select"].is_object());
	assert_exp(json.contains("size"));
	assert_exp(json["size"].is_object());
	assert_exp(json["size"].contains("x"));
	assert_exp(json["size"]["x"].is_number());
	assert_exp(json["size"].contains("y"));
	assert_exp(json["size"]["y"].is_number());
	assert_exp(!json.contains("directions") || json["directions"].is_number_unsigned());
	assert_exp(!json.contains("build_speed") || json["build_speed"].is_number_unsigned());
	assert_exp(!json.contains("group") || json["group"].is_string());
	assert_exp(json.contains("requirements"));
	assert_exp(json["requirements"].is_array());
	assert_exp(json.contains("anims"));
	assert_exp(json["anims"].is_array());
	assert_exp(!json.contains("sight") || json["sight"].is_number());
	assert_exp(!json.contains("turn_speed") || json["turn_speed"].is_number());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());
	assert_exp(!json.contains("pass_flags") || json["pass_flags"].is_string() || json["pass_flags"].is_number_unsigned());
	assert_exp(!json.contains("fake") || json["fake"].is_boolean());
	assert_exp(!json.contains("proxy") || json["proxy"].is_string());

	auto &base = faction.getBase();
	unsigned directions = json.contains("directions") ? json["directions"].get<unsigned>() : 1;

	this->_id = id;
	this->_limit = json.contains("limit") ? json["limit"].get<unsigned>() : UINT32_MAX;
	this->_hp = json["hp"];
	this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);
	this->_buildSpeed = json.contains("build_speed") ? json["build_speed"].get<unsigned>() : -1;

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
				assert_not_reached_msg("Invalid flag hex \"pass_flags\"");
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
			this->_pathCache.emplace_back(move->destination);
			this->_pathCached = true;
		}

		float a = this->_position.angle(move->destination);
		unsigned angle = round((a + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));

		angle %= this->_anims.size();
		if (angle != this->_currentDir && !this->_handleOrder(TurnOrder{angle}))
			return false;

		auto pos = this->_position;
		auto diff = move->destination - pos;

		diff.normalize();
		diff *= this->_speed / 32.f;
		pos += diff;

		auto diff2 = move->destination - pos;

		if (pos == move->destination || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0)) {
			this->setPosition(move->destination);
			return true;
		} else {
			this->setPosition(pos);
			return false;
		}
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
	if (!this->_orderQueue.empty())
		if (this->_handleOrder(this->_orderQueue.front()))
			this->_orderQueue.pop_front();
}

void GameObject::render(int layer)
{
	for (auto &anim : this->_anims[this->_currentDir])
		if (anim() && anim.getLayer() == layer)
			this->_displayAnim(anim.getAnim());
}

void GameObject::setPosition(const sf::Vector2f &pos)
{
	// OPTIMIZE: Diff instead of undoing it and redoing it
	this->deinitGlobalGrid();
	this->_position.x = pos.x;
	this->_position.y = pos.y;
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

const Vector2u& GameObject::getSize() const
{
	return this->_size;
}

Vector2i GameObject::getPosition() const
{
	return Vector2i(round(this->_position.x), round(this->_position.y));
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
	this->_orderQueue.emplace_back(MoveOrder{this->map->topLeftCoordToAbs(this->map->absToTileCoord(pos))});
	//this->_orderQueue.emplace_back(MoveOrder{pos + this->_size / 2});
}

void GameObject::clearOrders()
{
	this->_orderQueue.clear();
}

void GameObject::initGlobalGrid()
{
	if (this->_initGrid)
		return;
	if (this->_tiles) {
		for (size_t y = 0; y < this->_tiles->size() * this->map->tileSize.y; y++) {
			if (this->_position.y + static_cast<int>(y) < 0)
				continue;
			if (this->_position.y + y >= this->pfGrid->gridSize.y)
				break;

			auto &line = (*this->_tiles)[y / this->map->tileSize.y];

			for (size_t x = 0; x < line.size() * this->map->tileSize.x; x++) {
				if (!(line[x / this->map->tileSize.x] & MASK_SOLID)) {
					x += this->map->tileSize.x - 1;
					continue;
				}
				if (this->_position.x + static_cast<int>(x) < 0)
					continue;
				if (this->_position.x + x >= this->pfGrid->gridSize.x)
					break;
				PFGRIDMASK_ADD_UNIT(*this->pfGrid, this->_position.x + x, this->_position.y + y, this->_tileCount);
			}
		}
	} else {
		for (size_t y = 0; y < this->_size.y; y++) {
			if (this->_position.y + static_cast<int>(y) < 0)
				continue;
			if (this->_position.y + y >= this->pfGrid->gridSize.y)
				break;
			for (size_t x = 0; x < this->_size.x; x++) {
				if (this->_position.x + static_cast<int>(x) < 0)
					continue;
				if (this->_position.x + x >= this->pfGrid->gridSize.x)
					break;
				PFGRIDMASK_ADD_UNIT(*this->pfGrid, this->_position.x + x, this->_position.y + y, this->_tileCount);
			}
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
		for (size_t y = 0; y < this->_tiles->size() * this->map->tileSize.y; y++) {
			if (this->_position.y + static_cast<int>(y) < 0)
				continue;
			if (this->_position.y + y >= this->pfGrid->gridSize.y)
				break;

			auto &line = (*this->_tiles)[y / this->map->tileSize.y];

			for (size_t x = 0; x < line.size() * this->map->tileSize.x; x++) {
				if (!(line[x / this->map->tileSize.x] & MASK_SOLID)) {
					x += this->map->tileSize.x - 1;
					continue;
				}
				if (this->_position.x + static_cast<int>(x) < 0)
					continue;
				if (this->_position.x + x >= this->pfGrid->gridSize.x)
					break;
				PFGRIDMASK_DEL_UNIT(*this->pfGrid, this->_position.x + x, this->_position.y + y, this->_tileCount);
			}
		}
	} else {
		for (size_t y = 0; y < this->_size.y; y++) {
			if (this->_position.y + static_cast<int>(y) < 0)
				continue;
			if (this->_position.y + y >= this->pfGrid->gridSize.y)
				break;
			for (size_t x = 0; x < this->_size.x; x++) {
				if (this->_position.x + static_cast<int>(x) < 0)
					continue;
				if (this->_position.x + x >= this->pfGrid->gridSize.x)
					break;
				PFGRIDMASK_DEL_UNIT(*this->pfGrid, this->_position.x + x, this->_position.y + y, this->_tileCount);
			}
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
