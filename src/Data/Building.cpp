//
// Created by PinkySmile on 10/01/2026.
//

#include "nlohmann/json.hpp"
#include "Building.hpp"
#include "Animation.hpp"
#include "../Resources/Game.hpp"
#include "Utils.hpp"

void Building::_displayAnim(Animation &anim)
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

Building::Building(const Building &b) :
	_base(b._base),
	_buildSound(b._buildSound),
	_buildQueue(b._buildQueue),
	_giveQueue(b._giveQueue),
	_tiles(b._tiles),
	_requirements(b._requirements),
	_cost(b._cost),
	_storage(b._storage),
	_id(b._id),
	_group(b._group),
	_size(b._size),
	_position(b._position),
	_checkTiles(b._checkTiles),
	_proxy(b._proxy),
	_showAfterBuild(b._showAfterBuild),
	_maxHp(b._maxHp),
	_hp(b._hp),
	_sight(b._sight),
	_buildSpeed(b._buildSpeed),
	_limit(b._limit),
	_power(b._power),
	_building(b._building),
	_producing(b._producing),
	_produced(b._produced),
	_lowerPowerWhenDamaged(b._lowerPowerWhenDamaged),
	_fake(b._fake),
	animationState(b.animationState),
	playerState(b.playerState)
{
	this->_anims.reserve(b._anims.size());
	for (auto &anim : b._anims)
		this->_anims.emplace_back(*this, anim);
	if (b._buildAnim)
		this->_buildAnim.emplace(*this, *b._buildAnim);
	if (b._producingAnim)
		this->_producingAnim.emplace(*this, *b._producingAnim);
	if (b._producedAnim)
		this->_producedAnim.emplace(*this, *b._producedAnim);
}

Building::Building(const Faction &faction, const std::string &id, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &json) :
	_base(faction)
{
	auto &base = faction.getBase();

	this->_id = id;
	this->_limit = json.contains("limit") ? json["limit"].get<unsigned>() : UINT32_MAX;
	this->_buildSound.load(base.getBase() / json["build"]["path"].get<std::string>());
	this->_buildSound.setVolume(json["build"]["volume"]);
	this->_hp = json["hp"];
	this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_tiles = json["tiles"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);
	this->_buildSpeed = json.contains("speed") ? json["speed"].get<unsigned>() : -1;
	this->_buildMaxDistance = json["build_max_distance"];
	this->_selectable = json["selectable"];
	this->_sellable = json["sellable"];
	this->_repairable = json["repairable"];

	if (json.contains("queue"))
		this->_buildQueue = RESOURCE(base.getName(), json["queue"]);
	if (json.contains("gives_queue"))
		this->_giveQueue = RESOURCE(base.getName(), json["gives_queue"]);
	if (json.contains("group"))
		this->_group = RESOURCE(base.getName(), "group." + json["group"].get<std::string>());
	else
		this->_group = id;

	for (auto &anim : json["anims"])
		this->_anims.emplace_back(*this, animList, anim);
	if (!this->_anims.empty()) {
		auto &anims = this->_anims.front().getAnims();

		if (!anims.empty()) {
			auto &anim = anims.front();

			this->_selectBox.position = anim.getOffset();
			this->_selectBox.size = this->_size;
		}
	}
	if (json.contains("storage"))
		this->_storage = json["storage"];
	if (json.contains("proxy"))
		this->_proxy = json["proxy"];
	if (json.contains("build_anim") && !json["build_anim"].is_null())
		this->_buildAnim.emplace(*this, animList, json["build_anim"]);
	if (json.contains("producing_anim") && !json["producing_anim"].is_null())
		this->_producingAnim.emplace(*this, animList, json["producing_anim"]);
	if (json.contains("produced_anim") && !json["produced_anim"].is_null())
		this->_producedAnim.emplace(*this, animList, json["produced_anim"]);
	if (json.contains("show_after_build"))
		this->_showAfterBuild = json["show_after_build"];
	else if (this->_buildAnim)
		this->_showAfterBuild = this->_buildAnim->size();
	this->_power = json.contains("power") ? json["power"].get<int>() : 0;
	this->_sight = json.contains("sight") ? json["sight"].get<int>() : 0;
	this->_fake = json.contains("fake") && json["fake"];
	this->_lowerPowerWhenDamaged = json.contains("lower_power_when_damaged") && json["lower_power_when_damaged"];
	this->animationState.health = 1;
	if (this->_storage.size() == 1)
		this->animationState.storageKey = this->_storage.begin()->first;

	for (size_t y = 0; y < this->_tiles.size(); y++) {
		auto &line = this->_tiles[y];

		for (size_t x = 0; x < line.size(); x++) {
			uint8_t buildingMask = line[x];

			if ((buildingMask & (Building::MASK_PLACEMENT_REQUIRED | Building::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			for (int i1 = -this->_buildMaxDistance - 1; i1 <= this->_buildMaxDistance + 1; i1++)
				for (int i2 = -this->_buildMaxDistance - 1; i2 <= this->_buildMaxDistance + 1; i2++)
					this->_checkTiles.emplace(i1 + x, i2 + y);
		}
	}
}

const Mod &Building::getBase() const
{
	return this->_base.getBase();
}

void Building::update()
{
	if (this->_building && this->_buildAnim && (*this->_buildAnim)()) {
		if (this->_buildAnim->update())
			this->_building = false;
	} else {
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)() && this->_producingAnim->update())
			this->_producing = false;
		if (this->_produced  && this->_producedAnim  && (*this->_producedAnim)()  && this->_producedAnim->update())
			this->_produced = false;
	}
	for (auto &anim : this->_anims)
		if (anim())
			anim.update();
}

void Building::render(int layer)
{
	if (!this->_building || !this->_buildAnim || this->_buildAnim->current() >= this->_showAfterBuild)
		for (auto &anim : this->_anims)
			if (anim() && anim.getLayer() == layer)
				this->_displayAnim(anim.getAnim());
	if (this->_building && this->_buildAnim && (*this->_buildAnim)()) {
		if (layer == this->_buildAnim->getLayer())
			this->_displayAnim(this->_buildAnim->getAnim());
	} else {
		if (this->_produced &&  this->_producedAnim && (*this->_producedAnim)() && layer == this->_producedAnim->getLayer())
			this->_displayAnim(this->_producedAnim->getAnim());
		if (this->_producing && this->_producingAnim && (*this->_producingAnim)() && layer == this->_producingAnim->getLayer())
			this->_displayAnim(this->_producingAnim->getAnim());
	}
}

void Building::built()
{
	this->_building = true;
	if (this->_buildAnim)
		this->_buildAnim->reset();
}

void Building::playBuiltSound() const
{
	this->_buildSound.play();
}

void Building::produced()
{
	this->_produced = true;
	if (this->_producedAnim)
		this->_producedAnim->reset();
}

void Building::producing(bool producing)
{
	this->_producing = producing;
}

void Building::setPosition(const sf::Vector2i &pos)
{
	this->_position = pos;
}

void Building::setColor(const HSLColor &color)
{
	for (auto &anim : this->_anims)
		anim.setColor(color);
	if (this->_buildAnim)
		this->_buildAnim->setColor(color);
	if (this->_producingAnim)
		this->_producingAnim->setColor(color);
	if (this->_producedAnim)
		this->_producedAnim->setColor(color);
}

const std::string &Building::getId() const
{
	return this->_id;
}

int Building::getPower() const
{
	if (!this->_lowerPowerWhenDamaged)
		return this->_power;
	return this->_power * static_cast<int>(this->_hp) / static_cast<int>(this->_maxHp);
}

const std::vector<std::vector<uint8_t>> &Building::getTileData() const
{
	return this->_tiles;
}

void Building::damage(unsigned int amount)
{
	if (this->_hp < amount)
		this->_hp = 0;
	else
		this->_hp -= amount;
	this->animationState.health = static_cast<float>(this->_hp) / static_cast<float>(this->_maxHp);
}

void Building::heal(unsigned int amount)
{
	this->_hp += amount;
	if (this->_hp > this->_maxHp)
		this->_hp = this->_maxHp;
	this->animationState.health = static_cast<float>(this->_hp) / static_cast<float>(this->_maxHp);
}

unsigned Building::getHealth() const
{
	return this->_hp;
}

const std::optional<std::string> &Building::getProxy() const
{
	return this->_proxy;
}

bool Building::isFake() const
{
	return this->_fake;
}

const std::unordered_map<std::string, unsigned> &Building::getStorage() const
{
	return this->_storage;
}

bool Building::isDead() const
{
	return this->_hp == 0;
}

bool Building::isDisposable() const
{
	return this->_hp == 0;
}

const Vector2u& Building::getSize() const
{
	return this->_size;
}

const Vector2i &Building::getPosition() const
{
	return this->_position;
}

const std::vector<ConditionalAnim>& Building::getBaseAnims() const
{
	return this->_anims;
}

const std::optional<std::string> &Building::getGivenQueue() const
{
	return this->_giveQueue;
}

const ReqAndCombinator &Building::getRequirements() const
{
	return *this->_requirements;
}

const Faction &Building::getFaction() const
{
	return this->_base;
}

const std::optional<std::string> &Building::getBuildQueue() const
{
	return this->_buildQueue;
}

unsigned Building::getBuildSpeed() const
{
	return this->_buildSpeed;
}

const std::string &Building::getGroup() const
{
	return this->_group;
}

bool Building::isObstructed() const
{
	return false;
}

unsigned Building::getLimit() const
{
	return this->_limit;
}

const std::unordered_map<std::string, unsigned> &Building::getCost() const
{
	return this->_cost;
}

unsigned char Building::getBuildMaxDistance() const
{
	return this->_buildMaxDistance;
}

bool Building::isSelectable() const
{
	return this->_selectable;
}

bool Building::isSellable() const
{
	return this->_sellable;
}

bool Building::isRepairable() const
{
	return this->_repairable;
}

const std::set<Vector2i> &Building::getCheckTiles() const
{
	return this->_checkTiles;
}
