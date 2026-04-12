#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Data/Animation.hpp"
#include "Data/Upgrade.hpp"
#include "Resources/Game.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "UI.hpp"

#define GRID_SIZE 32

float guiScale = 1.5f;

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

void debugInit(Player &player)
{
	auto &state = player.state;

	state.money["base:credits"] = 10000;
	state.systemMoney["base:credits"] = 10000;

#if 0
	state.queues["d2k:building"]["d2k:wall"].unlocked = true;
	state.queues["d2k:building"]["d2k:gunt"].unlocked = true;
	state.queues["d2k:building"]["d2k:roct"].unlocked = true;
	state.queues["d2k:building"]["d2k:wall"].buildable = false;
	state.queues["d2k:building"]["d2k:gunt"].buildable = false;
	state.queues["d2k:building"]["d2k:roct"].buildable = false;

	state.queues["d2k:building"]["d2k:repr"].unlocked = true;
	state.queues["d2k:building"]["d2k:2con"].unlocked = true;
	state.queues["d2k:building"]["d2k:3con"].unlocked = true;
	state.queues["d2k:building"]["d2k:wtra"].unlocked = true;
	state.queues["d2k:building"]["d2k:barr"].unlocked = true;
	state.queues["d2k:building"]["d2k:refi"].unlocked = true;
	state.queues["d2k:building"]["d2k:lfac"].unlocked = true;
	state.queues["d2k:building"]["d2k:hfac"].unlocked = true;
	state.queues["d2k:building"]["d2k:outp"].unlocked = true;
	state.queues["d2k:building"]["d2k:silo"].unlocked = true;
	state.queues["d2k:building"]["d2k:star"].unlocked = true;
	state.queues["d2k:building"]["d2k:htec"].unlocked = true;
	state.queues["d2k:building"]["d2k:ixrc"].unlocked = true;
	state.queues["d2k:building"]["d2k:ahtec"].unlocked = true;
	state.queues["d2k:building"]["d2k:apala"].unlocked = true;
	state.queues["d2k:building"]["d2k:hpala"].unlocked = true;
	state.queues["d2k:building"]["d2k:opala"].unlocked = true;
	state.queues["d2k:building"]["d2k:repr"].buildable = true;
	state.queues["d2k:building"]["d2k:2con"].buildable = true;
	state.queues["d2k:building"]["d2k:3con"].buildable = true;
	state.queues["d2k:building"]["d2k:wtra"].buildable = true;
	state.queues["d2k:building"]["d2k:barr"].buildable = true;
	state.queues["d2k:building"]["d2k:refi"].buildable = true;
	state.queues["d2k:building"]["d2k:lfac"].buildable = true;
	state.queues["d2k:building"]["d2k:hfac"].buildable = true;
	state.queues["d2k:building"]["d2k:outp"].buildable = true;
	state.queues["d2k:building"]["d2k:silo"].buildable = true;
	state.queues["d2k:building"]["d2k:star"].buildable = true;
	state.queues["d2k:building"]["d2k:htec"].buildable = true;
	state.queues["d2k:building"]["d2k:ixrc"].buildable = true;
	state.queues["d2k:building"]["d2k:ahtec"].buildable = true;
	state.queues["d2k:building"]["d2k:apala"].buildable = true;
	state.queues["d2k:building"]["d2k:hpala"].buildable = true;
	state.queues["d2k:building"]["d2k:opala"].buildable = true;

	for (auto &b : state.queues["d2k:building"]) {
		b.second.buildTime = 120;
		b.second.buildProgress = 120;
	}

	state.queues["units"]["d2k:rifl"].unlocked = true;
	state.queues["units"]["d2k:trop"].unlocked = true;
	state.queues["units"]["d2k:engi"].unlocked = true;
	state.queues["units"]["d2k:rifl"].buildable = true;
	state.queues["units"]["d2k:trop"].buildable = true;
	state.queues["units"]["d2k:engi"].buildable = true;

	for (auto &b : state.queues["units"]) {
		b.second.buildTime = 120;
		b.second.buildProgress = 120;
	}
#endif
}

bool isMouseOverUI()
{
	Vector2i pos = sf::Mouse::getPosition(*game->screen);

	pos.x /= guiScale;
	pos.y /= guiScale;
	for (auto widget : game->gui.getWidgets()) {
		if (!widget->isVisible())
			continue;
		if (auto container = widget->cast<tgui::Container>())
			for (auto w : container->getWidgets())
				if (w->isMouseOnWidget(pos))
					return true;
	}
	return false;
}

int main()
{
	new Game("/usr/share/fonts/noto/NotoSans-Regular.ttf");
	game->modManager.loadMod("mods/dune2000/");
	game->screen->setMouseCursorVisible(false);
	game->locale = "fr";
	//game->screen->setMouseCursorGrabbed(true);

	Mod &mod = game->modManager.get("d2k");
	std::map<std::string, Faction> factions;

	mod.loadFactions("factions.json", factions);

	std::vector<Faction> allFactions{factions.at("d2k:atreides")};
	GameManager manager{allFactions};
	auto &players = manager.getPlayers();
	float timer = 0;

	manager.loadMap();
	manager.addPlayer(0, HSLColor{214, 375, 919}); // Atreides blue
	//manager.addPlayer(0, HSLColor{0, 800, 750}); // Harkonen's red
	//manager.addPlayer(0, HSLColor{145, 250, 900}); // Ordos's green
	debugInit(players[0]);
	players[0].setEnabled(true);
	manager.spawnBuilding(0, {0, 1}, "d2k:building", "d2k:a_cony", false);

	reloadViews(game->screen->getSize());
	while (game->screen->isOpen()) {
		while (auto event = game->screen->pollEvent()) {
			if (event->is<sf::Event::Closed>())
				game->screen->close();
			else if (auto resized = event->getIf<sf::Event::Resized>())
				reloadViews(resized->size);
			if (!isMouseOverUI())
				for (auto &p : players)
					p.consumeEvent(*event);
			game->gui.handleEvent(*event);
		}

		if (isMouseOverUI())
			for (auto &p : players)
				p.mouseOnUi();
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
}