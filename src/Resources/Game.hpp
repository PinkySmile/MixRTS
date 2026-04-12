//
// Created by PinkySmile on 18/09/2021.
//

#ifndef SOFGV_GAME_HPP
#define SOFGV_GAME_HPP


#include <mutex>
#include <memory>
#include <random>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "SoundManager.hpp"
#include "SceneManager.hpp"
#include "Data/Logger.hpp"
#include "Resources/RandomWrapper.hpp"
#include "Screen.hpp"
#include "TextureManager.hpp"
#include "Data/ModManager.hpp"

#define random_distrib(r, mi, ma) ((((uint64_t)(r)() - (r).min()) * ((ma) - (mi))) / ((r).max() - (r).min()) + (mi))

struct Game {
	bool hosting = false;
	std::random_device random;
	RandomWrapper battleRandom;
	Logger logger;
	sf::Font font;
	std::string locale = "en";
	std::string lastIp = "127.0.0.1";
	unsigned short lastPort = 0;
	std::unique_ptr<Screen> screen;
	tgui::Gui gui;
	TextureManager textureMgr;
	SoundManager soundMgr;
	SceneManager scene;
	ModManager modManager;
	Vector2u mousePos;
	sf::SoundBuffer dummySoundBuffer;

	Game(const std::filesystem::path &fontPath, const std::filesystem::path &loggerPath = "./latest.log");
};


#endif //SOFGV_GAME_HPP
