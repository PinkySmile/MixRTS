#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Data/Animation.hpp"
#include "Data/Upgrade.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "UI.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

float guiScale = 1.5f;
#ifdef _DEBUG
bool DEBUG_MODE = false;
#endif

void reloadViews(Vector2u size)
{
	sf::View view{sf::FloatRect{
		{0, -20 * guiScale},
		{
			static_cast<float>(size.x),
			static_cast<float>(size.y)
		}
	}};
	tgui::FloatRect viewGui{
		{0, 0},
		{
			size.x / guiScale,
			size.y / guiScale
		}
	};

	game->screen->setView(view);
	game->gui.setAbsoluteView(viewGui);
}

void dispFPS(float frameTime)
{
	auto old = game->screen->getView();
	auto str = sf::String(std::to_string(static_cast<unsigned>(round(1 / frameTime))) + "FPS");
	Vector2u size = game->screen->getSize();
	sf::View view{sf::FloatRect{
		{0, 0},
		size.to<float>()
	}};

	game->screen->setView(view);
	game->screen->displayElement(str, {0, size.y - 30.f}, size.x, Screen::ALIGN_RIGHT);
	game->screen->setView(old);
}

int main()
{
#ifndef PROPAGATE_CRASH
	try {
#endif
	#ifdef _WIN32
		new Game(R"(C:\Windows\Fonts\Arial.ttf)");
	#else
		new Game("/usr/share/fonts/noto/NotoSans-Regular.ttf");
	#endif
		game->modManager.loadMod("mods/dune2000/");
		game->screen->setMouseCursorVisible(false);
		game->screen->setMouseCursorGrabbed(true);
		game->locale = "fr";

		sf::Clock clock;
		sf::Text text{game->font};
		Mod &mod = game->modManager.get("d2k");
		std::unordered_map<std::string, Faction> factions;
		sf::Music music{"mods/dune2000/music/AMBUSH.ogg"};

		mod.loadFactions("factions.json", factions);

		std::vector<Faction> allFactions{factions.at("d2k:atreides")};
		GameManager manager{allFactions};
		auto &players = manager.getPlayers();
		float timer = 0;
		auto &map = manager.getLoadedMap();
		size_t spawn = 0;

		manager.loadMap(mod, "mods/dune2000/maps/8PLAY1");
		for (size_t i = 1; i < map.grid.startAreas.size(); i++)
			if (map.grid.startAreas[i].distance2(Vector2u{0, 0}) < map.grid.startAreas[spawn].distance2(Vector2u{0, 0}))
				spawn = i;
		manager.addPlayer(0, HSLColor{214, 375, 919}, spawn, 0); // Atreides blue
		//manager.addPlayer(0, HSLColor{0, 800, 750}); // Harkonen's red
		//manager.addPlayer(0, HSLColor{145, 250, 900}); // Ordos's green
		players[0].setEnabled(true);

		music.play();
		reloadViews(game->screen->getSize());
		while (game->screen->isOpen()) {
			while (auto event = game->screen->pollEvent()) {
				if (event->is<sf::Event::Closed>())
					game->screen->close();
				else if (auto resized = event->getIf<sf::Event::Resized>())
					reloadViews(resized->size);
				manager.consumeEvent(*event);
				game->gui.handleEvent(*event);
			}

			manager.update();
			timer++;
			if (timer >= 2.5) {
				manager.updateRender();
				timer -= 2.5;
			}
			game->screen->clear(sf::Color{0x80, 0xFF, 0x40});
			manager.render();
			//displayGrid();
			game->gui.draw();
			manager.renderAbove();
			dispFPS(clock.restart().asSeconds());
			game->screen->display();
		}
		return 0;
#ifndef PROPAGATE_CRASH
	} catch (const std::exception &e) {
		if (game)
			game->logger.fatal(e.what());
		else
			fprintf(stderr, "[FATAL]: Failed to init: %s\n", e.what());
		delete game;
#ifdef _WIN32
		MessageBoxA(nullptr, e.what(), "Fatal error", MB_ICONERROR);
#endif
	}
#endif
}
