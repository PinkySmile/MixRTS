#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Data/Animation.hpp"
#include "Data/Upgrade.hpp"
#include "Resources/Game.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "UI.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GRID_SIZE 32

float guiScale = 1.5f;
#ifdef _DEBUG
bool DEBUG_MODE = true;
#endif

void displayGrid()
{
	sf::RectangleShape shape;

	shape.setOutlineThickness(1);
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineColor(sf::Color::Black);
	shape.setSize({GRID_SIZE, game->screen->getView().getSize().y});
	for (size_t i = 0; i < game->screen->getView().getSize().x; i++) {
		shape.setPosition({static_cast<float>(GRID_SIZE * 2 * i), 0});
		game->screen->draw(shape);
	}
	shape.setSize({game->screen->getView().getSize().x, GRID_SIZE});
	for (size_t i = 0; i < game->screen->getView().getSize().y; i++) {
		shape.setPosition({0, static_cast<float>(GRID_SIZE * 2 * i)});
		game->screen->draw(shape);
	}
}

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

void debugInit(Player &player, const std::vector<Faction> &factions)
{
	auto &state = player.state;

	for (auto &f : factions) {
		for (auto &[name, b] : f.getBuildings()) {
			auto &bqueue = b.getBuildQueue();

			if (!bqueue)
				continue;

			auto &queue = state.queues[*bqueue][name];

			queue.unlocked = true;
			queue.limit = b.getLimit();
			queue.buildable = true;
			queue.paused = false;
			queue.buildProgress = 0;
			queue.buildTime = 1;
			queue.faction = &f;
			queue.building = &b;
			queue.upgrade = nullptr;
			queue.unit = nullptr;
			queue.elementId = &name;
			queue.queueId = &*bqueue;
			player.playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
		}
		for (auto &[name, u] : f.getUpgrades()) {
			auto &bqueue = u.getBuildQueue();

			if (!bqueue)
				continue;

			auto &queue = state.queues[*bqueue][name];

			queue.unlocked = true;
			queue.limit = u.getLimit();
			queue.buildable = true;
			queue.paused = false;
			queue.buildProgress = 0;
			queue.buildTime = 1;
			queue.faction = &f;
			queue.building = nullptr;
			queue.upgrade = &u;
			queue.unit = nullptr;
			queue.elementId = &name;
			queue.queueId = &*bqueue;
			player.playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
		}
	}
}

int main()
{
#ifndef _DEBUG
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

		Mod &mod = game->modManager.get("d2k");
		std::unordered_map<std::string, Faction> factions;
		sf::Music music{"mods/dune2000mm/music/AMBUSH.ogg"};

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
		if (DEBUG_MODE)
			debugInit(players[0], manager.getFactions());

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
			game->screen->display();
		}
		return 0;
#ifndef _DEBUG
	} catch (const std::exception &e) {
		delete game;
#ifdef _WIN32
		MessageBoxA(nullptr, e.what(), "Fatal error", MB_ICONERROR);
#endif
	}
#endif
}