//
// Created by PinkySmile on 18/09/2021.
//

#ifndef MIXRTS_GAME_HPP
#define MIXRTS_GAME_HPP


#include <mutex>
#include <memory>
#include <random>
#include <sol/sol.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "MSVCMacros.hpp"
#include "SoundManager.hpp"
#include "SceneManager.hpp"
#include "Data/Logger.hpp"
#include "Resources/RandomWrapper.hpp"
#include "Screen.hpp"
#include "TextureManager.hpp"
#include "Data/ModManager.hpp"

#ifdef MOCK
#include "Mock/MockType.hpp"
#include "Mock/MockedScreen.hpp"
#include "Mock/MockedTextureManager.hpp"
#include "Mock/MockedSoundManager.hpp"
typedef MockedScreen ScreenImpl;
typedef MockedTextureManager TextureManagerImpl;
typedef MockedSoundManager SoundManagerImpl;
#else
typedef Screen ScreenImpl;
typedef TextureManager TextureManagerImpl;
typedef SoundManager SoundManagerImpl;
#endif

//! Random from r, between mi and ma (included)
#define random_distrib(r, mi, ma) ((((int64_t)(r)() - (int64_t)(r).min()) * (int64_t)((ma) + 1 - (mi))) / ((int64_t)(r).max() - (int64_t)(r).min() + 1) + (int64_t)(mi))

struct MYDLL_API Game {
	bool hosting = false;
	bool paused = false;
	std::random_device random;
	RandomWrapper gameRandom;
	Logger logger;
	sf::Font font;
	std::string locale = "en";
	std::string lastIp = "127.0.0.1";
	unsigned short lastPort = 0;
	std::unique_ptr<ScreenImpl> screen;
	tgui::Gui gui;
	TextureManagerImpl textureMgr;
	SoundManagerImpl soundMgr;
	SceneManager scene;
	ModManager modManager;
	Vector2u mousePos;
	sol::state lua;
	std::unique_ptr<GameManager> game;

	static void pushUserType(sol::state &state);

#ifdef MOCK
	Game(const std::filesystem::path &fontPath, MockMode mode, const std::filesystem::path &video, Vector2u size = {1280, 1024}, const std::filesystem::path &loggerPath = "./latest.log");
#else
	Game(const std::filesystem::path &fontPath, const std::filesystem::path &loggerPath = "./latest.log");
#endif
	void init();
};

#ifdef _DEBUG
MYDLL_API extern bool DEBUG_MODE;
#else
#define DEBUG_MODE false
#endif

//! UI/window scale factor. Owned here (rather than in main.cpp) so that it is
//! available to every target that links the engine sources, including the
//! MixRTSTests target which has no main.cpp of its own.
extern float guiScale;

#endif //MIXRTS_GAME_HPP
