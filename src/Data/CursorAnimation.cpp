//
// Created by PinkySmile on 28/03/2026.
//

#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "CursorAnimation.hpp"
#include "Utils.hpp"
#include "../Resources/Game.hpp"


CursorAnimation::CursorAnimation(const std::filesystem::path &base, const nlohmann::json &json, const nlohmann::json &gen)
{
	for (auto jcursor : json)
		this->_frames.emplace_back(base, mergeObjects(jcursor, gen));
}

void CursorAnimation::update()
{
	this->_timer++;
	if (this->_timer >= this->_frames[this->_currentAnimation].getDuration()) {
		this->_currentAnimation++;
		this->_currentAnimation %= this->_frames.size();
		this->_timer = 0;
	}
}

void CursorAnimation::render() const
{
	sf::View old = game->screen->getView();
	sf::View v = sf::View{sf::FloatRect{
		{0, 0},
		{
			static_cast<float>(game->screen->getSize().x),
			static_cast<float>(game->screen->getSize().y)
		}
	}};
	auto pos = game->screen->getMousePosition();
	auto frame = this->_frames[this->_currentAnimation];
	sf::Sprite cursor{frame.getTexture()};
	auto &offset = frame.getOffset();

	pos.x -= offset.x;
	pos.y -= offset.y;
	cursor.setPosition(sf::Vector2f(pos));

	game->screen->setView(v);
	game->screen->draw(cursor);
	game->screen->setView(old);
}
