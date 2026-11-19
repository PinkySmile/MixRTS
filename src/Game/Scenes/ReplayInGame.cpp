//
// Created by PinkySmile on 12/11/2026.
//

#include <stdexcept>
#include "ReplayInGame.hpp"
#include "UI/CombinedUI.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

ReplayInGame::ReplayInGame(ReplayData replay, std::function<void (const std::string &)> &cb) :
	InGame(replay.toGameStartParams(), cb, [actions = replay.actions](const std::vector<Faction> &factions, UIState *uiState, GameManager *manager, Player *owner, std::vector<MapState> &mapStates, size_t playerIndex) -> std::shared_ptr<CombinedUI> {
		if (playerIndex == actions.size())
			return std::make_shared<CombinedUI>(factions, uiState, manager, owner, mapStates, playerIndex);
		assert_msg(playerIndex < actions.size(), std::to_string(playerIndex));
		return std::make_shared<ReplayCombinedUI>(factions, uiState, manager, owner, mapStates, playerIndex, actions[playerIndex]);
	}, false),
	_replay(std::move(replay))
{
	if (this->_mapCrc != this->_replay.mapCrc)
		throw std::runtime_error("Map checksum does not match this replay");
	for (const auto &player : this->_replay.players) {
		auto it = this->_factionCrcs.find(player.faction);

		assert_exp(it != this->_factionCrcs.end());
		if (it->second != player.factionCrc)
			throw std::runtime_error("Faction \"" + player.faction + "\" checksum does not match this replay's");
	}
	game->game->setControlledPlayer(0);
	this->_captureCrashState();
}

void ReplayInGame::update()
{
	float sec = this->_clock.restart().asSeconds();

	if (sec >= 0.25)
		sec = 0.25;
	this->_timer += 50 * sec * this->_startParams.settings.speed;
	bool captured = false;

	while (this->_timer >= 1) {
		if (game->game->getTick() >= this->_replay.frameCount || game->game->finished) {
			game->scene.switchScene("title_screen");
			return;
		}
		uint64_t tick = game->game->getTick();

		game->game->update();
		if (game->game->getTick() != tick) {
			this->_captureCrashState();
			this->_crashStateDirty = false;
			captured = true;
		}
		this->_timer -= 1;
	}
	if (!captured && this->_crashStateDirty) {
		this->_captureCrashState();
		this->_crashStateDirty = false;
	}
	game->screen->setMouseCursorVisible(false);
	game->screen->setMouseCursorGrabbed(!game->paused);
}

ReplayInGame *ReplayInGame::create(SceneArguments *args)
{
	checked_cast(params, ReplayArguments, args);
	return new ReplayInGame(std::move(params->replay), params->reportProgressA);
}
