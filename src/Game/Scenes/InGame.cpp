//
// Created by PinkySmile on 10/11/2026.
//

#include <ctime>
#include <sstream>
#include "InGame.hpp"
#include "Data/Player.hpp"
#include "Data/Effect.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Weapon.hpp"
#include "Data/Map.hpp"
#include "Resources/Game.hpp"
#include "UI/BaseUI.hpp"
#include "UI/CombinedUI.hpp"

InGame::InGame(GameStartParams &params, std::function<void (const std::string &)> &cb) :
	InGame(params, cb, [](const std::vector<Faction> &factions, UIState *uiState, GameManager *manager, PlayerState *ownerState, std::vector<MapState> &mapStates, size_t playerIndex) {
		return std::make_shared<CombinedUI>(factions, uiState, manager, ownerState, mapStates, playerIndex);
	}, true)
{
}

InGame::InGame(GameStartParams params, std::function<void (const std::string &)> &cb, const GameManager::UIFactory &factory, bool saveReplays) :
	_startParams(params),
	_saveReplays(saveReplays)
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

	cb("Computing checksum for " + params.map);
	this->_mapCrc = LoadedMap::computeChecksum(params.map);
	for (auto &f : this->_loadedFactions) {
		cb("Computing checksum for " + f.getName());
		this->_factionCrcs[f.getName()] = f.getChecksum();
	}

	cb("Init manager");
	game->game = std::make_unique<GameManager>(this->_loadedFactions, params.players.size(), GameManager::GameSettings{
		.shortGame = params.settings.shortGame
	}, factory);

	size_t i = 0;
	std::unordered_map<std::string, Faction> factions;
	std::vector<Player> &players = game->game->getPlayers();

	cb("Loading map " + params.map);
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

	for (auto &player : params.players) {
		auto it = std::ranges::find(loadedFactions, player.faction);
		unsigned factionIndex = std::distance(loadedFactions.begin(), it);

		assert_exp(it != loadedFactions.end());
		cb("Adding player " + std::to_string(i));
		game->game->addPlayer(factionIndex, player.color, player.spawnPoint, player.team);
		players[i].setEnabled(player.control);
		if (player.control)
			game->game->centerCameraOn(player.spawnPoint);
		for (auto &ui : players[i].getCombinedUI().getUIs())
			if (ui)
				this->_panel->add(ui->getPanel());
		i++;
	}
}

InGame::~InGame()
{
	// Drop the anchor before any buffer it points at is freed.
	this->_retractCrashAnchor();
	// Read the flag directly: a virtual call here would not reach a subclass that is already destroyed.
	if (this->_saveReplays)
		this->saveReplay("replays");
	// Faction UI libraries are closed when GameManager is destroyed. Button callbacks are code in
	// those libraries, so every widget has to be released while the libraries are still loaded.
	this->_panel.reset();
	game->game.reset();
}

bool InGame::_recordsReplay() const
{
	return this->_saveReplays;
}

static std::string replayTimestamp()
{
	time_t timer;
	char buffer[32];
	tm *info;

	time(&timer);
	info = localtime(&timer);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", info);
	return buffer;
}

void InGame::saveReplay(const std::filesystem::path &dir)
{
	if (!this->_recordsReplay() || this->_replaySaved || !game || !game->game)
		return;
	this->_replaySaved = true;
	try {
		std::filesystem::create_directories(dir);

		std::vector<uint32_t> crcs;
		std::vector<std::vector<RecordedAction>> actions;
		auto &players = game->game->getPlayers();

		assert_exp(players.size() == this->_startParams.players.size());
		for (size_t i = 0; i < players.size(); i++) {
			crcs.push_back(this->_factionCrcs.at(this->_startParams.players[i].faction));
			actions.push_back(players[i].getCombinedUI().getRecordedActions());
		}

		// getTick() is the step in progress when an exception left update(). Playback enters
		// every tick up to this one, so the faulting step runs again. A captured signal
		// stores that same tick on the anchor.
		uint32_t tick = static_cast<uint32_t>(game->game->getTick());

		if (crashReplayAnchor.crashed)
			tick = crashReplayAnchor.crashTick;

		auto data = ReplayData::fromGame(
			this->_startParams,
			this->_mapCrc,
			crcs,
			tick,
			std::move(actions)
		);
		std::string stamp = replayTimestamp();
		std::filesystem::path file = dir / (stamp + ".replay");
		int suffix = 1;

		while (std::filesystem::exists(file))
			file = dir / (stamp + "-" + std::to_string(suffix++) + ".replay");
		data.save(file);
	} catch (const std::exception &e) {
		game->logger.error(std::string("Failed to save replay: ") + e.what());
	}
}

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

void InGame::_publishCrashAnchor()
{
	if (!this->_saveReplays || this->_crashAnchorPublished || !game || !game->game)
		return;

	auto &players = game->game->getPlayers();
	std::vector<uint32_t> crcs;
	std::vector<std::vector<RecordedAction>> actions(players.size());

	assert_exp(players.size() == this->_startParams.players.size());
	for (size_t i = 0; i < players.size(); i++)
		crcs.push_back(this->_factionCrcs.at(this->_startParams.players[i].faction));

	auto data = ReplayData::fromGame(this->_startParams, this->_mapCrc, crcs, 0, std::move(actions));
	std::ostringstream header{std::ios::out | std::ios::binary};

	data.save(header);

	std::string bytes = header.str();

	this->_crashHeader.assign(bytes.begin(), bytes.end());
	this->_crashStreams.assign(players.size(), {});
	for (size_t i = 0; i < players.size(); i++)
		players[i].getCombinedUI().getCrashJournal().bind(&this->_crashStreams[i]);

	crashReplayAnchor.header = this->_crashHeader.data();
	crashReplayAnchor.headerSize = this->_crashHeader.size();
	crashReplayAnchor.tick = static_cast<uint32_t>(game->game->getTick());
	crashReplayAnchor.playerCount = static_cast<uint32_t>(players.size());
	crashReplayAnchor.streams = this->_crashStreams.data();
	crashReplayAnchor.liveTick = game->game->getTickPointer();
	crashReplayAnchor.crashTick = 0;
	crashReplayAnchor.crashed = 0;
	std::atomic_signal_fence(std::memory_order_release);
	crashReplayAnchor.magic = CrashReplayAnchor::MAGIC;
	this->_crashAnchorPublished = true;
}

void InGame::_retractCrashAnchor()
{
	if (!this->_crashAnchorPublished)
		return;
	crashReplayAnchor.magic = 0;
	std::atomic_signal_fence(std::memory_order_release);
	crashReplayAnchor.liveTick = nullptr;
	crashReplayAnchor.crashed = 0;
	this->_crashAnchorPublished = false;
}

void InGame::update()
{
	this->_publishCrashAnchor();
	if (this->_crashAnchorPublished && game && game->game)
		crashReplayAnchor.tick = static_cast<uint32_t>(game->game->getTick());

	float sec = this->_clock.restart().asSeconds();

	if (sec >= 0.25)
		sec = 0.25;
	this->_timer += 50 * sec * this->_startParams.settings.speed;
	while (this->_timer >= 1) {
		game->game->update();
		if (this->_crashAnchorPublished)
			crashReplayAnchor.tick = static_cast<uint32_t>(game->game->getTick());
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
