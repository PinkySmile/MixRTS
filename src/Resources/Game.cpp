//
// Created by PinkySmile on 18/09/2021.
//

#include "Game.hpp"
#include "Assert.hpp"
#include "Data/Logger.hpp"

MYDLL_API Game *game = nullptr;

Game::Game(const std::filesystem::path &fontPath, const std::filesystem::path &loggerPath) :
	logger(loggerPath),
	font(fontPath),
	screen(new Screen("MixRTS")),
	gui(*this->screen)
{
	this->screen->setFramerateLimit(50);
	this->screen->setFont(this->font);
	assert_exp(!game);
	game = this;
}
