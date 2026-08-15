//
// Created by PinkySmile on 10/01/2026.
//

#include "nlohmann/json.hpp"
#include "GameObject.hpp"
#include "Animation.hpp"
#include "Mod.hpp"
#include "ConditionalAnim.hpp"
#include "Player.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

ConditionalAnim::ConditionalAnim(const GameObject *parent, const ConditionalAnim &other) :
	_parent(parent),
	_anim(other._anim),
	_cond(other._cond),
	_index(other._index),
	_delay(other._delay),
	_totalDelay(other._totalDelay),
	_layer(other._layer)
{
}

void ConditionalAnim::_init(const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j, const std::string &animName)
{
	std::function cb = [](const ConditionalAnim &){ return true; };

	assert_msg(animList.contains(animName), animName);
	assert_exp(!j.contains("delay") || j["delay"].is_number());
	assert_exp(j.contains("layer"));
	assert_exp(!j.contains("when_hp_lower") || j["when_hp_lower"].is_number());
	assert_exp(!j.contains("when_hp_bigger") || j["when_hp_bigger"].is_number());
	assert_exp(!j.contains("when_storage_bigger") || j["when_storage_bigger"].is_number());
	assert_exp(!j.contains("when_storage_is_in") || j["when_storage_is_in"].is_array());

	this->_anim = animList.at(animName);
	if (j.contains("delay"))
		this->_delay = j["delay"];
	this->_layer = j["layer"];
	if (j.contains("when_hp_lower"))
		cb = [cb, hp = j["when_hp_lower"].get<float>()](const ConditionalAnim &This) {
			if (!cb(This)) return false;
			assert_exp(This._parent);
			return This._parent->animationState.health * 100.f < hp;
		};
	if (j.contains("when_hp_bigger"))
		cb = [cb, hp = j["when_hp_bigger"].get<float>()](const ConditionalAnim &This) {
			if (!cb(This)) return false;
			assert_exp(This._parent);
			return This._parent->animationState.health * 100.f >= hp;
		};
	if (j.contains("when_storage_is_in")) {
		assert_exp(j["when_storage_is_in"].size() == 2);
		assert_exp(j["when_storage_is_in"][0].is_number() && j["when_storage_is_in"][1].is_number());
		auto range = j["when_storage_is_in"].get<std::vector<unsigned>>();
		cb = [cb, range](const ConditionalAnim &This) {
			if (!cb(This)) return false;
			assert_exp(This._parent);
			unsigned total = This._parent->owner->state.totalStorage[This._parent->animationState.storageKey];
			unsigned maxVal = This._parent->owner->state.money[This._parent->animationState.storageKey];
			float percent = maxVal * 100.f / total;
			return percent >= range[0] && percent < range[1];
		};
	}
	if (j.contains("when_storage_bigger"))
		cb = [cb, storage = j["when_storage_bigger"].get<float>()](const ConditionalAnim &This) {
			if (!cb(This)) return false;
			assert_exp(This._parent);
			unsigned total = This._parent->owner->state.totalStorage[This._parent->animationState.storageKey];
			unsigned maxVal = This._parent->owner->state.money[This._parent->animationState.storageKey];
			float percent = maxVal * 100.f / total;
			return percent >= storage;
		};
	this->_cond = cb;
}

ConditionalAnim::ConditionalAnim(const Mod &base, const GameObject *parent, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j) :
	_parent(parent)
{
	assert_exp(j.contains("anim"));
	assert_exp(j["anim"].is_string());
	this->_init(animList, j, RESOURCE(base.getName(), j["anim"]));
}

ConditionalAnim::ConditionalAnim(const Mod &base, const GameObject *parent, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j, unsigned int index) :
	_parent(parent)
{
	assert_exp(j.contains("anim"));
	assert_exp(j["anim"].is_string());
	this->_init(animList, j, RESOURCE(base.getName(), j["anim"].get<std::string>() + "_" + std::to_string(index)));
}

bool ConditionalAnim::operator()() const
{
	return this->_cond(*this);
}

bool ConditionalAnim::update()
{
	if (this->_totalDelay < this->_delay) {
		this->_totalDelay++;
		return false;
	}
	this->_totalDelay = 0;
	this->_index++;
	if (this->_index >= this->_anim.size()) {
		this->_index = 0;
		return true;
	}
	return false;
}

void ConditionalAnim::reset()
{
	this->_index = 0;
	this->_totalDelay = 0;
}

Animation &ConditionalAnim::getAnim()
{
	return this->_anim[this->_index];
}

const Animation &ConditionalAnim::getAnim() const
{
	return this->_anim[this->_index];
}

const std::vector<Animation> &ConditionalAnim::getAnims() const
{
	return this->_anim;
}

int ConditionalAnim::getLayer() const
{
	return this->_layer;
}

size_t ConditionalAnim::size() const
{
	return this->_anim.size();
}

unsigned ConditionalAnim::current() const
{
	return this->_index;
}

void ConditionalAnim::setColor(const HSLColor &color)
{
	for (auto &anim : this->_anim)
		anim.setColor(color);
}
