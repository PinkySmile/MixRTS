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

bool Effect::update()
{
	this->_position += this->_speed;
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

void Effect::setColor(HSLColor color)
{
	for (auto &anim : this->_anim)
		anim.setColor(color);
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
