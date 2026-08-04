//
// Created by PinkySmile on 03/08/2026.
//

#include "nlohmann/json.hpp"
#include "Effect.hpp"
#include "Animation.hpp"
#include "MultiSound.hpp"
#include "Mod.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

Effect::Effect(
	const Mod &base,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const nlohmann::json &data
)
{
	assert_exp(data.contains("anim") && data["anim"].is_object());
	assert_exp(data["anim"].contains("anim") && data["anim"]["anim"].is_string());
	assert_exp(data["anim"].contains("layer") && data["anim"]["layer"].is_number());
	assert_exp(!data["anim"].contains("delay") || data["anim"]["delay"].is_number());
	assert_exp(!data.contains("sound") || data["sound"].is_null() || data["sound"].is_string());

	std::string animName = RESOURCE(base.getName(), data["anim"]["anim"].get<std::string>());

	assert_msg(animList.contains(animName), animName);
	this->_anim = animList.at(animName);
	this->_layer = data["anim"]["layer"];
	if (data["anim"].contains("delay"))
		this->_delay = data["anim"]["delay"];
	if (data.contains("sound") && !data["sound"].is_null()) {
		std::string soundName = RESOURCE(base.getName(), data["sound"].get<std::string>());

		assert_msg(sounds.contains(soundName), soundName);
		this->_sound = &sounds.at(soundName);
	}
}

bool Effect::update()
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

void Effect::reset()
{
	this->_index = 0;
	this->_totalDelay = 0;
}

Animation &Effect::getAnim()
{
	return this->_anim[this->_index];
}

const Animation &Effect::getAnim() const
{
	return this->_anim[this->_index];
}

const std::vector<Animation> &Effect::getAnims() const
{
	return this->_anim;
}

int Effect::getLayer() const
{
	return this->_layer;
}

size_t Effect::size() const
{
	return this->_anim.size();
}

unsigned Effect::current() const
{
	return this->_index;
}

const MultiSound *Effect::getSound() const
{
	return this->_sound;
}

void Effect::playSound() const
{
	if (this->_sound)
		this->_sound->play();
}
