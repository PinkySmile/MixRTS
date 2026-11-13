//
// Created by PinkySmile on 03/08/2026.
//

#include <sol/sol.hpp>
#include "nlohmann/json.hpp"
#include "Effect.hpp"
#include "StateStream.hpp"
#include "Animation.hpp"
#include "MultiSound.hpp"
#include "Mod.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"
#include "Resources/Game.hpp"

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
	assert_exp(!data.contains("opacity") || data["opacity"].is_number());
	assert_exp(!data.contains("sound") || data["sound"].is_string());
	assert_exp(!data.contains("speed") || data["speed"].is_object());

	std::string animName = RESOURCE(base.getName(), data["anim"]["anim"].get<std::string>());

	assert_msg(animList.contains(animName), animName);
	this->_anim = animList.at(animName);
	this->_layer = data["anim"]["layer"];
	if (data["anim"].contains("delay"))
		this->_delay = data["anim"]["delay"];
	if (data.contains("opacity"))
		this->_opacity = data["opacity"];
	if (data.contains("speed")) {
		assert_exp(!data["speed"].contains("x") || data["speed"]["x"].is_number());
		assert_exp(!data["speed"].contains("y") || data["speed"]["y"].is_number());
		this->_speed = { data["speed"]["x"], data["speed"]["y"] };
	}
	if (data.contains("sound")) {
		std::string soundName = RESOURCE(base.getName(), data["sound"].get<std::string>());

		assert_msg(sounds.contains(soundName), soundName);
		this->_sound = &sounds.at(soundName);
	}
}

void Effect::repeatFor(unsigned ticks)
{
	this->_repeating = true;
	this->_repeatFor = ticks;
}

bool Effect::update()
{
	// Counted at the start so the effect is still drawn on its last tick and removed on the next one.
	if (this->_repeating && this->_played >= this->_repeatFor)
		return true;
	this->_played++;
	this->_position += this->_speed;
	if (this->_totalDelay < this->_delay) {
		this->_totalDelay++;
		return false;
	}
	this->_totalDelay = 0;
	this->_index++;
	if (this->_index >= this->_anim.size()) {
		this->_index = 0;
		// A repeating effect wraps back to the first frame for as long as its duration allows.
		return !this->_repeating;
	}
	return false;
}

void Effect::render()
{
	Animation &anim = this->_anim[this->_index];
	Sprite &sprite = anim.getSprite();

	sprite.setColor(sf::Color{0xFF, 0xFF, 0xFF, static_cast<std::uint8_t>(0xFF * this->_opacity)});
	sprite.setPosition({
		sprite.getPosition().x + this->_position.x,
		sprite.getPosition().y + this->_position.y
	});

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

void Effect::setPosition(Vector2f pos)
{
	this->_position = pos;
}

Vector2f Effect::getPosition() const
{
	return this->_position;
}

void Effect::setColor(HSLColor color)
{
	for (auto &anim : this->_anim)
		anim.setColor(color);
}

void Effect::setTemplate(uint32_t templateId, uint32_t ownerIndex)
{
	this->_templateId = templateId;
	this->_ownerIndex = ownerIndex;
}

uint32_t Effect::getTemplateId() const
{
	return this->_templateId;
}

uint32_t Effect::getOwnerIndex() const
{
	return this->_ownerIndex;
}

void Effect::savePlayback(StateWriter &out) const
{
	out.vec2(this->_position);
	out.vec2(this->_speed);
	out.u32(this->_delay);
	out.u32(this->_totalDelay);
	out.u32(this->_index);
	out.u32(this->_repeatFor);
	out.u32(this->_played);
	out.boolean(this->_repeating);
	out.f32(this->_opacity);
	out.i32(this->_layer);
}

void Effect::loadPlayback(StateReader &in)
{
	this->_position = in.vec2<float>();
	this->_speed = in.vec2<float>();
	this->_delay = in.u32();
	this->_totalDelay = in.u32();
	this->_index = in.u32();
	this->_repeatFor = in.u32();
	this->_played = in.u32();
	this->_repeating = in.boolean();
	this->_opacity = in.f32();
	this->_layer = in.i32();
	assert_msg(this->_anim.empty() || this->_index < this->_anim.size(), "Effect frame is out of range");
}

int Effect::getLayer() const
{
	return this->_layer;
}

void Effect::playSound() const
{
	if (this->_sound)
		this->_sound->play(this->_position);
}

void Effect::pushUserType(sol::state &state)
{
	sol::usertype<Effect> type = state.new_usertype<Effect>("Effect");

	type["update"] = sol::property(&Effect::update);
	type["render"] = sol::property(&Effect::render);
	type["play_sound"] = sol::property(&Effect::playSound);
	type["position"] = sol::property(&Effect::getPosition, &Effect::setPosition);
	type["color"] = sol::property(&Effect::setColor);
	type["layer"] = sol::property(&Effect::getLayer);
}
