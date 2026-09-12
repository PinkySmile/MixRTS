//
// Created by PinkySmile on 18/09/2021.
//

#include "Game.hpp"
#include "Assert.hpp"
#include "Data/Logger.hpp"

MYDLL_API Game *game = nullptr;
float guiScale = 1.5f;
#ifdef _DEBUG
bool DEBUG_MODE = false;
#endif

#ifdef MOCK
Game::Game(const std::filesystem::path &fontPath, MockMode mode, const std::filesystem::path &video, Vector2u size, const std::filesystem::path &loggerPath) :
	logger(loggerPath),
	font(fontPath),
	screen(new ScreenImpl("MixRTS", mode, video, size)),
	gui(this->screen->getRenderTarget()),
	textureMgr(mode),
	soundMgr(mode, video)
{
	this->screen->setFramerateLimit(50);
	this->screen->setFont(this->font);
	assert_exp(!game);
	game = this;
}
#else
Game::Game(const std::filesystem::path &fontPath, const std::filesystem::path &loggerPath) :
	logger(loggerPath),
	font(fontPath),
	screen(new ScreenImpl("MixRTS")),
	gui(this->screen->getRenderTarget())
{
	this->screen->setFramerateLimit(50);
	this->screen->setFont(this->font);
	assert_exp(!game);
	game = this;
}
#endif