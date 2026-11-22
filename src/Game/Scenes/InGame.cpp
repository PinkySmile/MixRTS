//
// Created by PinkySmile on 10/11/2026.
//

#include <ctime>
#include <fstream>
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
#include "Data/SaveState.hpp"
#include "Debug/Profiler.hpp"

InGameArguments::~InGameArguments() = default;

InGame::InGame(GameStartParams &params, std::function<void (const std::string &)> &cb) :
	InGame(params, cb, [](const std::vector<Faction> &factions, UIState *uiState, GameManager *manager, Player *owner, std::vector<MapState> &mapStates, size_t playerIndex) {
		return std::make_shared<CombinedUI>(factions, uiState, manager, owner, mapStates, playerIndex);
	}, true)
{
}

InGame::InGame(GameStartParams params, std::function<void (const std::string &)> &cb, const GameManager::UIFactory &factory, bool saveReplays, bool spawnStartingObjects) :
	_startParams(params),
	_saveReplays(saveReplays)
{
	std::vector<std::string> loadedFactions;
	MapInfo map;
	size_t mapFaction = 0;
	uint32_t crc = 0;

	map.load(params.map);
	this->_panel = tgui::Panel::create();
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
	game->gameRandom.seed(params.seed);

	cb("Init factions");
	for (auto &player : params.players) {
		auto it = std::ranges::find(loadedFactions, player.faction);

		if (it == loadedFactions.end()) {
			loadedFactions.push_back(player.faction);
			this->_loadedFactions.push_back(game->modManager.getFaction(player.faction));
		}
	}
	if (!map.faction.empty()) {
		auto it = std::ranges::find(loadedFactions, map.faction);

		if (it == loadedFactions.end()) {
			mapFaction = loadedFactions.size();
			loadedFactions.push_back(map.faction);
			this->_loadedFactions.push_back(game->modManager.getFaction(map.faction));
		} else
			mapFaction = std::distance(loadedFactions.begin(), it);
		crc = this->_loadedFactions[mapFaction].getChecksum();
	}

	for (auto &f : this->_loadedFactions) {
		cb("Init faction " + f.getName());
		f.load(loadedFactions);
	}

	cb("Computing checksum for " + params.map);
	this->_mapCrc = LoadedMap::computeChecksum(params.map, crc);
	for (auto &f : this->_loadedFactions) {
		cb("Computing checksum for " + f.getName());
		this->_factionCrcs[f.getName()] = f.getChecksum();
	}

	cb("Init manager");
	game->game = std::make_unique<GameManager>(
		this->_loadedFactions,
		params.players.size() + 1,
		GameManager::GameSettings{ .shortGame = params.settings.shortGame },
		factory
	);

	size_t i = 0;
	std::vector<Player> &players = game->game->getPlayers();

	cb("Loading map " + params.map);
	game->game->loadMap(params.map);

	for (auto &player : params.players) {
		auto it = std::ranges::find(loadedFactions, player.faction);
		unsigned factionIndex = std::distance(loadedFactions.begin(), it);

		assert_exp(it != loadedFactions.end());
		cb("Adding player " + std::to_string(i));
		game->game->addPlayer(factionIndex, player.color, player.spawnPoint, player.team, spawnStartingObjects);
		players[i].setEnabled(player.control);
		if (spawnStartingObjects && player.control)
			game->game->centerCameraOn(player.spawnPoint);
		for (auto &ui : players[i].getCombinedUI().getUIs())
			if (ui)
				this->_panel->add(ui->getPanel());
		i++;
	}

	cb("Adding neutral player");
	game->game->addPlayer(mapFaction, Color::White, {0, 0}, NEUTRAL_TEAM, true);

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
	this->_captureCrashState();
}

InGame::InGame(GameStartParams params, std::shared_ptr<const SaveStateFile> state, std::function<void (const std::string &)> &cb) :
	InGame(std::move(params), cb, [](const std::vector<Faction> &factions, UIState *uiState, GameManager *manager, Player *owner, std::vector<MapState> &mapStates, size_t playerIndex) {
		return std::make_shared<CombinedUI>(factions, uiState, manager, owner, mapStates, playerIndex);
	}, false, false)
{
	if (this->_mapCrc != state->mapCrc)
		throw std::runtime_error("Map checksum does not match this state");
	if (state->factionCrcs.size() != state->start.players.size())
		throw std::runtime_error("This state file is missing a faction checksum");
	for (size_t i = 0; i < state->start.players.size(); i++) {
		auto it = this->_factionCrcs.find(state->start.players[i].faction);

		if (it == this->_factionCrcs.end() || it->second != state->factionCrcs[i])
			throw std::runtime_error("Faction \"" + state->start.players[i].faction + "\" checksum does not match this state's");
	}
	game->game->loadState(state->snapshot);
	game->game->setPaused(true);
	game->game->setCameraView(state->camera, state->zoom);
	this->_captureCrashState();
}

InGame::~InGame()
{
	// Drop the anchors before any buffer they point at is freed.
	this->_retractCrashState();
	this->_retractCrashAnchor();
	// Read the flag directly: a virtual call here would not reach a subclass that is already destroyed.
	if (this->_saveReplays)
		this->saveReplay(game->data / "replays");
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

		assert_exp(players.size() - 1 == this->_startParams.players.size());
		for (size_t i = 0; i < players.size() - 1; i++) {
			crcs.push_back(this->_factionCrcs.at(this->_startParams.players[i].faction));
			actions.push_back(players[i].getCombinedUI().getRecordedActions());
		}

		// getTick() is the step in progress when an exception left update(). Playback enters
		// every tick up to this one, so the faulting step runs again. A captured signal
		// stores that same tick on the anchor.
		auto tick = static_cast<uint32_t>(game->game->getTick());

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
	reloadViews(*game->screen, game->gui, game->screen->getSize(), {0, -20 * guiScale}, guiScale);
	game->game->updateRender();
	game->game->render();
	if (this->_tickTimes.empty())
		return;

	char details[96];
	auto old = game->screen->getView();
	Vector2u size = game->screen->getSize();
	sf::View view{sf::FloatRect{
		{0, 0},
		size.to<float>()
	}};
	float avg = 0;
	float w = 0;

	for (auto tick : this->_tickTimes) {
		avg += tick;
		w = std::max(w, tick);
	}
	avg /= this->_tickTimes.size();
	snprintf(details, sizeof(details), "%.zuTPS (l%.2fms a%.2fms w%.2fms)", this->_tickTimestamp.size(), 1000 * this->_tickTimes.back(), 1000 * avg, 1000 * w);
	game->screen->setView(view);
	game->screen->displayElement(sf::String(details), {0, size.y - 35.f}, size.x, Screen::ALIGN_LEFT);
	game->screen->setView(old);
}

void InGame::renderAbove() const
{
	game->game->renderAbove();
}

void InGame::_publishCrashAnchor()
{
	PROFILE_SCOPE("Publish crash anchor");
	if (!this->_saveReplays || this->_crashAnchorPublished || !game || !game->game)
		return;

	auto &players = game->game->getPlayers();
	std::vector<uint32_t> crcs;
	std::vector<std::vector<RecordedAction>> actions(players.size() - 1);

	assert_exp(players.size() - 1 == this->_startParams.players.size());
	for (size_t i = 0; i < players.size() - 1; i++)
		crcs.push_back(this->_factionCrcs.at(this->_startParams.players[i].faction));

	auto data = ReplayData::fromGame(this->_startParams, this->_mapCrc, crcs, 0, std::move(actions));
	std::ostringstream header{std::ios::out | std::ios::binary};

	data.save(header);

	std::string bytes = header.str();

	this->_crashHeader.assign(bytes.begin(), bytes.end());
	this->_crashStreams.assign(players.size() - 1, {});
	for (size_t i = 0; i < players.size() - 1; i++)
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

SaveStateFile InGame::_makeStateFile() const
{
	PROFILE_SCOPE("save_state");
	SaveStateFile file;
	auto &players = game->game->getPlayers();

	assert_exp(players.size() - 1 == this->_startParams.players.size());
	file.start = this->_startParams;
	file.mapCrc = this->_mapCrc;
	file.camera = game->game->getCameraTranslate();
	file.zoom = game->game->getCameraZoom();
	for (const auto &player : this->_startParams.players)
		file.factionCrcs.push_back(this->_factionCrcs.at(player.faction));
	game->game->saveState(file.snapshot);
	return file;
}

void InGame::_captureCrashState()
{
	if (!game || !game->game)
		return;
	try {
		std::vector<uint8_t> bytes = this->_makeStateFile().encode();

		if (bytes.empty())
			return;

		unsigned next = this->_crashStateSlot ^ 1u;

		this->_crashState[next] = std::move(bytes);
		crashStateAnchor.magic = 0;
		std::atomic_signal_fence(std::memory_order_release);
		crashStateAnchor.data = this->_crashState[next].data();
		crashStateAnchor.size = this->_crashState[next].size();
		std::atomic_signal_fence(std::memory_order_release);
		crashStateAnchor.magic = CrashStateAnchor::MAGIC;
		this->_crashStateSlot = next;
	} catch (const std::exception &) {
	}
}

void InGame::_retractCrashState()
{
	crashStateAnchor.magic = 0;
	std::atomic_signal_fence(std::memory_order_release);
	crashStateAnchor.data = nullptr;
	crashStateAnchor.size = 0;
}

void InGame::saveCrashState(const std::filesystem::path &dir)
{
	try {
		std::filesystem::create_directories(dir);

		std::filesystem::path file = dir / "state.state";

		if (crashStateAnchor.magic == CrashStateAnchor::MAGIC && crashStateAnchor.data && crashStateAnchor.size) {
			std::ofstream stream(file, std::ios::binary);

			if (!stream)
				return;
			stream.write(reinterpret_cast<const char *>(crashStateAnchor.data), static_cast<std::streamsize>(crashStateAnchor.size));
			return;
		}
		this->_makeStateFile().save(file);
	} catch (const std::exception &) {
	}
}

void InGame::_writeState()
{
	try {
		std::filesystem::path dir = "saves";

		std::filesystem::create_directories(dir);

		SaveStateFile file = this->_makeStateFile();
		std::string stamp = replayTimestamp();
		std::filesystem::path path = dir / (stamp.substr(0, 10) + "_" + stamp.substr(11) + ".state");
		int suffix = 1;

		while (std::filesystem::exists(path))
			path = dir / (stamp.substr(0, 10) + "_" + stamp.substr(11) + "-" + std::to_string(suffix++) + ".state");
		file.save(path);
		game->logger.info("Saved state to " + path.string());
	} catch (const std::exception &e) {
		game->logger.error(std::string("Failed to save state: ") + e.what());
	}
}

void InGame::update()
{
	this->_publishCrashAnchor();
	if (game->game->takeStateSaveRequest())
		this->_writeState();
	if (this->_crashAnchorPublished)
		crashReplayAnchor.tick = static_cast<uint32_t>(game->game->getTick());

	float sec = this->_clock.restart().asSeconds();

	if (sec >= 0.25)
		sec = 0.25;
	this->_timer += 50 * sec * this->_startParams.settings.speed;
	bool captured = false;

	while (this->_timer >= 1) {
		this->_tickTimestamp.push_back(this->_clock2.getElapsedTime().asSeconds());
		uint64_t tick = game->game->getTick();
		sf::Clock clock;

		game->game->update();
		this->_tickTimes.push_back(clock.getElapsedTime().asSeconds());
		while (this->_tickTimes.size() > 50)
			this->_tickTimes.pop_front();
		while (this->_tickTimestamp.back() - this->_tickTimestamp.front() > 1)
			this->_tickTimestamp.pop_front();
		if (game->game->getTick() != tick) {
			this->_captureCrashState();
			this->_crashStateDirty = false;
			captured = true;
		}
		if (this->_crashAnchorPublished)
			crashReplayAnchor.tick = static_cast<uint32_t>(game->game->getTick());
		if (game->game->finished) {
			game->scene.switchScene("title_screen");
			return;
		}
		this->_timer -= 1;
	}
	// Orders queued while no tick ran (paused, or this frame did not step) still belong in the dump.
	if (!captured && this->_crashStateDirty) {
		this->_captureCrashState();
		this->_crashStateDirty = false;
	}
	game->screen->setMouseCursorGrabbed(!game->paused);
}

void InGame::consumeEvent(const sf::Event &event)
{
	game->game->consumeEvent(event);
	game->game->consumeGeneralEvent(event);
	this->_crashStateDirty = true;
}

tgui::Panel::Ptr InGame::getPanel() const
{
	return this->_panel;
}

InGame *InGame::create(SceneArguments *args)
{
	checked_cast(params, InGameArguments, args);
	if (params->state)
		return new InGame(params->state->start, params->state, params->reportProgressA);
	return new InGame(params->startParams, params->reportProgressA);
}
