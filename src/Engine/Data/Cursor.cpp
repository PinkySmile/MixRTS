//
// Created by PinkySmile on 10/01/2026.
//

#include <fstream>
#include "nlohmann/json.hpp"
#include "Cursor.hpp"
#include "../Utils.hpp"
#include "../Resources/Game.hpp"
#include "Resources/Assert.hpp"

Cursor::Cursor(const std::filesystem::path &base, const nlohmann::json &json)
{
	assert_exp(json.contains("center"));
	assert_exp(json["center"].is_object());
	assert_exp(json["center"].contains("x"));
	assert_exp(json["center"]["x"].is_number_unsigned());
	assert_exp(json["center"].contains("y"));
	assert_exp(json["center"]["y"].is_number_unsigned());
	assert_exp(json.contains("sprite"));
	assert_exp(json["sprite"].is_string());
	assert_exp(json.contains("duration"));
	assert_exp(json["duration"].is_number());
	this->_offset.x = json["center"]["x"].get<unsigned>();
	this->_offset.y = json["center"]["y"].get<unsigned>();
	assert_exp(this->_texture.loadFromFile(base / json["sprite"].get<std::string>()));
	this->_duration = json["duration"];
}

const sf::Texture &Cursor::getTexture() const
{
	return this->_texture;
}

unsigned Cursor::getDuration() const
{
	return this->_duration;
}

const Vector2u &Cursor::getOffset() const
{
	return this->_offset;
}
