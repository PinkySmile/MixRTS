//
// Created by PinkySmile on 20/04/2026.
//

#include "Faction.hpp"
#include "GameObject.hpp"
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
	_selectSound(g._selectSound),
	_cost(g._cost),
	_requirements(g._requirements),
	_queue(g._queue),
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
	animationState(g.animationState),
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
	assert_exp(!json.contains("limit") || json["limit"].is_number_unsigned());
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
	assert_exp(!json.contains("speed") || json["speed"].is_number_unsigned());
	assert_exp(!json.contains("group") || json["group"].is_string());
	assert_exp(json.contains("anims"));
	assert_exp(json["anims"].is_array());
	assert_exp(!json.contains("sight") || json["sight"].is_number());
	assert_exp(!json.contains("turn_speed") || json["turn_speed"].is_number());
	assert_exp(!json.contains("select_sound") || json["select_sound"].is_string());

	auto &base = faction.getBase();
	unsigned directions = json.contains("directions") ? json["directions"].get<unsigned>() : 1;

	this->_id = id;
	this->_limit = json.contains("limit") ? json["limit"].get<unsigned>() : UINT32_MAX;
	this->_hp = json["hp"];
	this->_maxHp = this->_hp;
	this->_cost = json["cost"];
	this->_size = {json["size"]["x"], json["size"]["y"]};
	this->_requirements = std::make_shared<ReqAndCombinator>(base.getName(), json["requirements"]);
	this->_buildSpeed = json.contains("speed") ? json["speed"].get<unsigned>() : -1;

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

bool GameObject::_handleOrder(const Order &o)
{
	if (auto turn = std::get_if<TurnOrder>(&this->_orderQueue.front())) {
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
	}
	return true;
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

void GameObject::setPosition(const sf::Vector2i &pos)
{
	this->_position = pos;
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

const Vector2i &GameObject::getPosition() const
{
	return this->_position;
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

bool GameObject::deploy()
{
	return false;
}

bool GameObject::canDeploy() const
{
	return false;
}
