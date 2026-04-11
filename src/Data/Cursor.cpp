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
