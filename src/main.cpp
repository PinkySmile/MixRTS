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

		std::vector<Faction> allFactions{
			factions.at("d2k:atreides"),
			factions.at("d2k:harkonen"),
			factions.at("d2k:ordos")
		};
		bool b = false;
		GameManager manager{allFactions, 3};
		auto &players = manager.getPlayers();
		auto &map = manager.getLoadedMap();
		std::array<size_t, 3> spawn{};
		unsigned currentPlayer = 0;

		manager.loadMap(mod, "mods/dune2000/maps/8PLAY1");
		const auto &areas = map.grid.startAreas;
		size_t numAreas = areas.size();
		assert_exp(numAreas >= 3);
		spawn[0] = game->random() % numAreas;

		double maxDist = -1.0;
		size_t bestJ = 0;
		size_t bestK = 0;

		for (size_t j = 0; j < numAreas; j++) {
			if (j == spawn[0]) continue;

			for (size_t k = j + 1; k < numAreas; k++) {
				if (k == spawn[0]) continue;

				// Calculate total distance between spawn[0], j, and k
				auto d01 = areas[spawn[0]].distance2(areas[j]); // P1 to P2
				auto d02 = areas[spawn[0]].distance2(areas[k]); // P1 to P3
				auto d12 = areas[j].distance2(areas[k]);        // P2 to P3

				// Find the absolute weakest link (shortest distance) in this trio
				auto currentMinDist = std::min({d01, d02, d12});

				// Track the pair that maximizes the total distance
				if (currentMinDist > maxDist) {
					maxDist = currentMinDist;
					bestJ = j;
					bestK = k;
				}
			}
		}

		spawn[1] = bestJ;
		spawn[2] = bestK;
		manager.addPlayer(0, HSLColor{214, 375, 919}, spawn[0], 0); // Atreides blue
		manager.addPlayer(1, HSLColor{  0, 800, 750}, spawn[1], 1); // Harkonen's red
		manager.addPlayer(2, HSLColor{145, 250, 900}, spawn[2], 2); // Ordos's green
		players[0].setEnabled(true);
		players[1].setEnabled(false);
		players[2].setEnabled(false);

		//music.play();
		reloadViews(game->screen->getSize());
		while (game->screen->isOpen()) {
			while (auto event = game->screen->pollEvent()) {
				if (event->is<sf::Event::Closed>())
					game->screen->close();
				else if (auto resized = event->getIf<sf::Event::Resized>())
					reloadViews(resized->size);
				else if (auto key = event->getIf<sf::Event::KeyPressed>()) {
				#ifdef _DEBUG
					if (key->code == sf::Keyboard::Key::F4) {
						players[currentPlayer].setEnabled(false);
						currentPlayer = (currentPlayer + 1) % players.size();
						players[currentPlayer].setEnabled(true);
					}
				#endif
				}
				manager.consumeEvent(*event);
				game->gui.handleEvent(*event);
			}

			manager.update();
			manager.update();
			if (b)
				manager.update();
			b = !b;
			manager.updateRender();
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
