#include <iostream>
#include <cstdio>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include "Args.hpp"
#include "UI/BaseUI.hpp"
#include "Data/Animation.hpp"
#include "Data/Projectile.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Effect.hpp"
#include "Data/Weapon.hpp"
#include "Data/Warhead.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "UI/UI.hpp"
#include "Debug/Crash.hpp"
#include "Debug/Profiler.hpp"
#include "Utils.hpp"
#include "Debug/Profiler.hpp"
#include "Scenes/InGame.hpp"
#include "Scenes/LoadingScene.hpp"
#include "Scenes/TitleScreen.hpp"

#ifdef _WIN32
#include <windows.h>
#endif


#ifdef __EMSCRIPTEN__
#define USE_LOADING_THREAD false
#else
#define LOADING_SCENE
#define USE_LOADING_THREAD true
#endif

void checkCompilationEnv()
{
	char magic[] = {0x04, 0x03, 0x02, 0x01};
	auto magicPtr = reinterpret_cast<unsigned *>(magic);

	// We perform an endianness check here and display a warning if it fails.
	// The affected stuff are:
	//   - All the network stack
	//   - Generated replays
	//   - Computed state checksums
	// We officially support only little endian but people can play if they have the same endianness.
	assert_exp(*magicPtr == 0x01020304);
	/*if (*magicPtr != 0x01020304)
		Utils::dispMsg(
			game->gui,
			"Warning",
			"Your version of the game has been compiled in " + std::string(*magicPtr == 0x04030201 ? "big endian" : "middle endian") + " but only little endian is supported\n" +
			"You will not be able to play with players using a different endianness.\n" +
			"Moreover, you won't be able to load replays generated with a different endianness.\n"
			"Your replays will also not be compatible with a different version of the game.",
			MB_ICONWARNING
		);*/

	game->gameRandom.seed(0);
	assert_eq(game->gameRandom(), 2357136044UL);
	assert_eq(game->gameRandom(), 2546248239UL);
	assert_eq(game->gameRandom(), 3071714933UL);
	assert_eq(game->gameRandom(), 3626093760UL);
	assert_eq(game->gameRandom(), 2588848963UL);

	game->gameRandom.seed(0);
	assert_eq(game->gameRandom(), 2357136044UL);
	assert_eq(game->gameRandom(), 2546248239UL);
	assert_eq(game->gameRandom(), 3071714933UL);
	assert_eq(game->gameRandom(), 3626093760UL);
	assert_eq(game->gameRandom(), 2588848963UL);

	game->gameRandom.seed(0);
	assert_eq(game->gameRandom.min(), 0x00000000UL);
	assert_eq(game->gameRandom.max(), 0xFFFFFFFFUL);
	assert_eq(random_distrib(game->gameRandom, 0, 20), 11L);
	assert_eq(random_distrib(game->gameRandom, 0, 20), 12L);
	assert_eq(random_distrib(game->gameRandom, 0, 20), 15L);
	assert_eq(random_distrib(game->gameRandom, 0, 20), 17L);
	assert_eq(random_distrib(game->gameRandom, 0, 20), 12L);
}

void registerScenes()
{
	bool hasLoading = USE_LOADING_THREAD;

	game->scene.registerScene("title_screen", TitleScreen::create, false);
	game->scene.registerScene("in_game", InGame::create, hasLoading);
#ifdef LOADING_SCENE
	game->scene.registerScene("loading", LoadingScene::create, false);
#endif
}

void handleEvents()
{
	PROFILE_SCOPE("events");
	while (auto event = game->screen->pollEvent()) {
		if (event->is<sf::Event::Closed>())
			game->screen->close();
		game->scene.consumeEvent(*event);

		PROFILE_SCOPE("gui");
		game->gui.handleEvent(*event);
	}
}

void displayCrash(const std::exception &e)
{
	tgui::Panel::Ptr panel;
	auto win = tgui::ChildWindow::create();

	win->setSize(1000, 600);
	win->setPosition("max((&.w - w) / 2, 0)", "max((&.h - h) / 2, 0)");
#ifdef NO_TRACE
	panel = generateCrashPanel({}, e);
#else
	if (auto *ce = dynamic_cast<const cpptrace::exception *>(&e))
		panel = generateCrashPanel(ce->trace(), *ce);
	else
		panel = generateCrashPanel(cpptrace::from_current_exception(), e);
#endif
	panel->setSize("100%", "100%");
	win->add(panel);
	if (game->game)
		game->game->setPaused();
	game->gui.add(win);
}

void handleStart(Args &args)
{
	if (args.players.empty()) {
		game->scene.switchScene("title_screen");
		return;
	}

	auto params = new InGameArguments();

	params->reportProgressA = [](const std::string &){};
	params->startParams.map = args.map;
	params->startParams.seed = game->random();
	params->startParams.settings.speed = args.speed;
	params->startParams.settings.shortGame = true;
	params->startParams.settings.randomSpawn = false;
	genSpawns(args.players, params->startParams);
	game->scene.switchScene("in_game", params);
}

int main(int argc, char **argv)
{
	Args args;

	if (argc != 0 && strcmp(argv[0], "crash_handler") == 0)
		return crashHandlerMain();
	initCrashHandler();
	if (!args.parse(argc, argv))
		return EXIT_FAILURE;

	TRY {
		game = new Game("assets/NotoSans-Regular.ttf");
		registerScenes();
		checkCompilationEnv();
		game->modManager.loadAll("mods");
		game->settings.locale = args.locale;
		if (args.noLazyLoad)
			game->settings.loading = LOADING_ALL;
		else if (args.preload)
			game->settings.loading = LOADING_BASE_SPRITES;
		handleStart(args);

		std::filesystem::directory_iterator d2kMusics{"mods/dune2000/music_game"};
		std::filesystem::directory_iterator scbwMusics{"mods/starcraft_broodwar/music_game"};
		std::vector<std::filesystem::path> musics;
		musics.insert(musics.end(), begin(d2kMusics), end(d2kMusics));
		musics.insert(musics.end(), begin(scbwMusics), end(scbwMusics));
		sf::Music music{musics[game->random() % musics.size()]};

		music.play();
		music.setLooping(true);
		if (args.loadingOnly)
			game->screen->close();
		while (game->screen->isOpen()) {
			Profiler::get().beginFrame();
			TRY {
				handleEvents();

				PROFILE_SCOPE("scene_update");
				game->scene.update();
			} CPPTRACE_CATCH(const std::exception &e) {
				displayCrash(e);
			}

			game->screen->clear(sf::Color::Black);
			{
				PROFILE_SCOPE("scene_draw");
				game->scene.render();
			}
			{
				PROFILE_SCOPE("gui_draw");
				game->gui.draw();
			}
			game->scene.renderAbove();
			Profiler::get().endFrame();
			game->screen->display();
		}
		return EXIT_SUCCESS;
	} CPPTRACE_CATCH(const std::exception &e) {
		if (args.loadingOnly)
			game->screen->close();
	#ifdef NO_TRACE
		handleCrash({}, e);
	#else
		if (auto *ce = dynamic_cast<const cpptrace::exception *>(&e))
			handleCrash(ce->trace(), *ce);
		else
			handleCrash(cpptrace::from_current_exception(), e);
		delete game;
	#endif
	}
}
