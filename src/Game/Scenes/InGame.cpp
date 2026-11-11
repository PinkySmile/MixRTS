//
// Created by PinkySmile on 10/11/2026.
//

#include "InGame.hpp"
#include "Data/Player.hpp"
#include "Data/Effect.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Weapon.hpp"
#include "Resources/Game.hpp"
#include "UI/BaseUI.hpp"

InGame::InGame(GameStartParams &params, std::function<void (const std::string &)> &cb) :
	_settings(params.settings)
{
	std::vector<std::string> loadedFactions;

	this->_panel = tgui::Panel::create();
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
	game->gameRandom = RandomWrapper(params.seed);
	cb("Init factions");
	for (auto &player : params.players) {
		auto it = std::ranges::find(loadedFactions, player.faction);

		if (it == loadedFactions.end()) {
			loadedFactions.push_back(player.faction);
			this->_loadedFactions.push_back(game->modManager.getFaction(player.faction));
		}
	}
	for (auto &f : this->_loadedFactions) {
		cb("Init faction " + f.getName());
		f.load(loadedFactions);
	}
	cb("Init manager");
	game->game = std::make_unique<GameManager>(this->_loadedFactions, params.players.size(), GameManager::GameSettings{
		.shortGame = params.settings.shortGame
	});

	size_t i = 0;
	std::unordered_map<std::string, Faction> factions;
	std::vector<Player> &players = game->game->getPlayers();

	cb("Load map");
	game->game->loadMap(params.map);

	cb("Preload sprites");
	switch (game->settings.loading) {
	case LOADING_ALL:
		game->game->preloadAllSprites();
		break;
	case LOADING_BASE_SPRITES:
		game->game->preloadSprites();
		break;
	default:
		break;
	}

	cb("Finalizing");
	for (auto &player : params.players) {
		auto it = std::ranges::find(loadedFactions, player.faction);
		unsigned factionIndex = std::distance(loadedFactions.begin(), it);

		assert_exp(it != loadedFactions.end());
		game->game->addPlayer(factionIndex, player.color, player.spawnPoint, player.team);
		players[i].setEnabled(player.control);
		if (player.control)
			game->game->centerCameraOn(player.spawnPoint);
		for (auto &ui : players[i].getUIs())
			if (ui)
				this->_panel->add(ui->getPanel());
		i++;
	}
}

InGame::~InGame() = default;

void InGame::render() const
{
	reloadViews(*game->screen, game->gui, game->screen->getSize(), {0, -20 * guiScale});
	game->game->updateRender();
	game->game->render();
}

void InGame::renderAbove() const
{
	game->game->renderAbove();
}

void InGame::update()
{
	float sec = this->_clock.restart().asSeconds();

	if (sec >= 0.25)
		sec = 0.25;
	this->_timer += 50 * sec * this->_settings.speed;
	while (this->_timer >= 1) {
		game->game->update();
		if (game->game->finished) {
			game->scene.switchScene("title_screen");
			return;
		}
		this->_timer -= 1;
	}
	game->screen->setMouseCursorVisible(false);
	game->screen->setMouseCursorGrabbed(!game->paused);
}

void InGame::consumeEvent(const sf::Event &event)
{
	game->game->consumeEvent(event);
	game->game->consumeGeneralEvent(event);
}

tgui::Panel::Ptr InGame::getPanel() const
{
	return this->_panel;
}

InGame *InGame::create(SceneArguments *args)
{
	checked_cast(params, InGameArguments, args);
	return new InGame(params->startParams, params->reportProgressA);
}
