//
// Created by PinkySmile on 09/11/2026.
//

#include <TGUI/Widgets/Button.hpp>
#include "TitleScreen.hpp"
#include "Data/Vector.hpp"
#include "Data/Map.hpp"
#include "Data/Replay.hpp"
#include "Data/SaveState.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"
#include "UI/BaseUI.hpp"

void genSpawns(const std::vector<PlayerArg> &arg, GameStartParams &params)
{
	std::vector<size_t> array;
	std::vector<char> teams;
	MapInfo map;
	bool hasOne = false;

	map.load(params.map);
	array.resize(arg.size(), SIZE_MAX);
	teams.resize(arg.size(), 0);
	for (size_t i = 0; i < arg.size(); i++) {
		teams[i] = arg[i].team;
		if (arg[i].spawnPoint) {
			array[i] = 0;
			hasOne = true;
		}
	}
	if (!hasOne)
		array[0] = random_distrib(game->random, 0, map.startAreas.size() - 1);
	genSpawnsFurthest(arg.size(), array, teams, map.startAreas);
	for (size_t i = 0; i < arg.size(); i++) {
		auto &player = arg[i];

		params.players.push_back(PlayerParams{
			player.faction,
			player.spawnPoint ? *player.spawnPoint : map.startAreas[array[i]].to<float>(),
			player.color,
			player.team,
			player.control
		});
	}
}

void genSpawnsFurthest(size_t size, std::vector<size_t> &array, const std::vector<char> &teams, const std::vector<Vector2u> &areas)
{
	assert_exp(areas.size() >= size);
	assert_exp(teams.size() >= size);

	array.resize(size, SIZE_MAX);

	// Collect players whose spawn locations need to be generated.
	std::vector<size_t> toGenerate;

	for (size_t i = 0; i < size; ++i)
		if (array[i] == SIZE_MAX)
			toGenerate.push_back(i);
	if (toGenerate.empty())
		return;

	// Preserve existing fixed positions.
	std::vector<size_t> availableAreas;

	for (size_t i = 0; i < areas.size(); ++i) {
		bool used = false;

		for (size_t j = 0; j < size; ++j) {
			if (array[j] == i) {
				used = true;
				break;
			}
		}

		if (!used)
			availableAreas.push_back(i);
	}

	const size_t count = toGenerate.size();
	assert_exp(availableAreas.size() >= count);
	double bestScore = -std::numeric_limits<double>::infinity();
	std::vector<size_t> bestSub(count);
	std::vector<size_t> currentSub(count);

	auto evaluate = [&]() -> double {
		double score = 0.0;

		// Evaluate distances between every pair of players.
		for (size_t i = 0; i < size; ++i) {
			size_t areaI = array[i];

			if (areaI == SIZE_MAX) {
				auto it = std::find(toGenerate.begin(), toGenerate.end(), i);
				size_t index = std::distance(toGenerate.begin(), it);

				areaI = currentSub[index];
			}

			for (size_t j = i + 1; j < size; ++j) {
				size_t areaJ = array[j];

				if (areaJ == SIZE_MAX) {
					auto it = std::find(
						toGenerate.begin(), toGenerate.end(), j);

					size_t index = std::distance(toGenerate.begin(), it);
					areaJ = currentSub[index];
				}

				double dist = areas[areaI].distance2(areas[areaJ]);

				if (teams[i] == teams[j]) {
					// Teammates should be close.
					score -= dist;
				} else {
					// Opponents should be far apart.
					score += dist;
				}
			}
		}

		return score;
	};
	auto findBestCombination = [&](auto &self, size_t depth, size_t startIdx) -> void {
		if (depth == count) {
			double score = evaluate();

			if (score > bestScore) {
				bestScore = score;
				bestSub = currentSub;
			}
			return;
		}

		for (size_t j = startIdx; j < availableAreas.size(); ++j) {
			currentSub[depth] = availableAreas[j];
			self(self, depth + 1, j + 1);
		}
	};

	findBestCombination(findBestCombination, 0, 0);
	for (size_t i = 0; i < count; ++i)
		array[toGenerate[i]] = bestSub[i];
}

void genSpawnsRandom(size_t size, std::vector<size_t> &array, const std::vector<char> &teams, const std::vector<Vector2u> &areas)
{
	assert_exp(areas.size() >= size);
	assert_exp(teams.size() >= size);

	array.resize(size, SIZE_MAX);

	// Collect players whose spawn locations need to be generated.
	std::vector<size_t> toGenerate;

	for (size_t i = 0; i < size; ++i)
		if (array[i] == SIZE_MAX)
			toGenerate.push_back(i);
	if (toGenerate.empty())
		return;

	// Preserve existing fixed positions.
	std::vector<size_t> availableAreas;

	for (size_t i = 0; i < areas.size(); ++i) {
		bool used = false;

		for (size_t j = 0; j < size; ++j) {
			if (array[j] == i) {
				used = true;
				break;
			}
		}

		if (!used)
			availableAreas.push_back(i);
	}

	assert_exp(availableAreas.size() >= toGenerate.size());
	for (size_t toGen : toGenerate) {
		size_t index = random_distrib(game->random, 0, availableAreas.size() - 1);

		array[toGen] = index;
		availableAreas.erase(std::next(availableAreas.begin(), index));
	}
}

void TitleScreen::_initTitle()
{
	this->_panel->loadWidgetsFromFile("assets/gui/title.gui");

	auto play = this->_panel->get<tgui::Button>("Play");
	auto replay = this->_panel->get<tgui::Button>("Replay");
	auto load = this->_panel->get<tgui::Button>("Load");
	auto settings = this->_panel->get<tgui::Button>("Settings");
	auto debug = this->_panel->get<tgui::Button>("Debug");
	auto quit = this->_panel->get<tgui::Button>("Quit");

#ifndef _DEBUG
	debug->setVisible(false);
#endif
	play->onClick([] {

	});
	replay->onClick([this] {
		this->_openReplayDialog();
	});
	load->onClick([this] {
		this->_openLoadDialog();
	});
	settings->onClick([] {

	});
	debug->onClick([this] {
		game->lobbySettings.players.clear();
		game->lobbySettingsValid = true;
		this->_initLobby();
	});
	quit->onClick([] {
		game->screen->close();
	});
}

void TitleScreen::_updateMapLobby(
	tgui::Picture::Ptr map,
	tgui::Label::Ptr size,
	tgui::Label::Ptr name,
	tgui::Label::Ptr origin,
	tgui::Label::Ptr author,
	tgui::Label::Ptr objects
)
{
	tgui::Texture tt;

	tt.loadFromPixelData(this->_map.preview.getSize(), this->_map.preview.getPixelsPtr());
	map->getRenderer()->setTexture(tt);
	size->setText(
		std::to_string(this->_map.gridSize.x) + "x" + std::to_string(this->_map.gridSize.y) +
		" (" + std::to_string(this->_map.tileSize.x * this->_map.gridSize.x) +
		"x" + std::to_string(this->_map.tileSize.y * this->_map.gridSize.y) + ")"
	);
	name->setText(this->_map.name);
	origin->setText("From " + this->_map.origin);
	author->setText("Made by " + this->_map.author);
	objects->setText(""); // TODO:
}

void TitleScreen::_addPlayerPanels(tgui::Panel::Ptr players, tgui::Button::Ptr start)
{
	const std::vector<tgui::Widget::Ptr> &w = players->getWidgets();

	while (w.size() > game->lobbySettings.players.size() + 1) {
		players->remove(w[w.size() - 1]);
		this->_playerPanels.pop_back();
	}
	while (w.size() < game->lobbySettings.players.size() + 1) {
		char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@&$~'^¨*";
		static_assert(sizeof(characters) - 1 >= MAX_TEAM_COUNT);
		size_t index = this->_playerPanels.size();
		this->_playerPanels.emplace_back();
		PlayerUI &p = this->_playerPanels.back();
		p.panel = tgui::Panel::create({"100%", 30});
		p.panel->loadWidgetsFromFile("assets/gui/player_entry.gui");

		p.player = p.panel->get<tgui::Button>("Player");
		p.faction = p.panel->get<tgui::ComboBox>("Faction");
		p.color = p.panel->get<tgui::Button>("Color");
		p.team = p.panel->get<tgui::ComboBox>("Team");
		p.spawn = p.panel->get<tgui::ComboBox>("Spawn");

		for (auto &key : game->modManager.getFactionList())
			p.faction->addItem(key, key);
		for (size_t i = 0; i < MAX_TEAM_COUNT; i++)
			p.team->addItem(characters[i]);
		p.spawn->addItem("-");
		for (size_t i = 0; i < this->_map.startAreas.size(); i++)
			p.spawn->addItem(characters[i]);
		p.spawn->addItem("Custom");

		p.player->onClick([this, players, start, index] {
			game->lobbySettings.players.erase(game->lobbySettings.players.begin() + index);
			game->lobbySettings.players[0].control = true,
			this->_addPlayerPanels(players, start);
			this->_updatePlayerList(start);
		});
		p.color->onClick([index] {

		});
		p.faction->onItemSelect([index](const tgui::String &, const tgui::String &id) {
			game->lobbySettings.players[index].faction = id.toStdString();
		});
		p.team->onItemSelect([index](int i) {
			game->lobbySettings.players[index].team = i;
		});
		p.spawn->onItemSelect([this, index](int i) {
			if (i == 0) {
				game->lobbySettings.players[index].spawnPoint.reset();
				game->lobbySettings.players[index].spawnPointIndex.reset();
			} else if (i == this->_map.startAreas.size() + 1) {
				game->lobbySettings.players[index].spawnPoint = this->_map.startAreas[0];
				game->lobbySettings.players[index].spawnPointIndex.reset();
			} else {
				game->lobbySettings.players[index].spawnPoint.reset();
				game->lobbySettings.players[index].spawnPointIndex = i - 1;
			}
		});

		if (index == 0)
			p.panel->setPosition(0, 10);
		else
			p.panel->setPosition(0, tgui::Layout{"Player" + std::to_string(index) + ".y + Player" + std::to_string(index) + ".h"});
		players->add(p.panel, "Player" + std::to_string(index + 1));
	}

	if (game->lobbySettings.players.empty())
		players->get<tgui::Panel>("AddPlayer")->setPosition(0, 10);
	else
		players->get<tgui::Panel>("AddPlayer")->setPosition(
			0, tgui::Layout{
				"Player" + std::to_string(game->lobbySettings.players.size()) + ".y + "
				"Player" + std::to_string(game->lobbySettings.players.size()) + ".h"
			}
		);
}

void TitleScreen::_updatePlayerList(tgui::Button::Ptr start)
{
	start->setEnabled(!game->lobbySettings.players.empty());
	for (size_t i = 0; i < this->_playerPanels.size(); i++) {
		auto &p = this->_playerPanels[i];
		auto &pl = game->lobbySettings.players[i];
		Color color = pl.color.toRGB();

		p.color->getRenderer()->setBackgroundColor(color);
		p.color->getRenderer()->setBackgroundColorDisabled(color);
		p.color->getRenderer()->setBackgroundColorDown(color);
		p.color->getRenderer()->setBackgroundColorDownDisabled(color);
		p.color->getRenderer()->setBackgroundColorDownFocused(color);
		p.color->getRenderer()->setBackgroundColorDownHover(color);
		p.color->getRenderer()->setBackgroundColorFocused(color);
		p.color->getRenderer()->setBackgroundColorHover(color);
		p.player->setText("Player " + std::to_string(i + 1));
		p.faction->setSelectedItemById(pl.faction);
		p.team->setSelectedItemByIndex(pl.team);
		if (pl.spawnPoint)
			p.spawn->setSelectedItemByIndex(this->_map.startAreas.size() + 1);
		else if (pl.spawnPointIndex)
			p.spawn->setSelectedItemByIndex(*pl.spawnPointIndex + 1);
		else
			p.spawn->setSelectedItemByIndex(0);
	}
}

void TitleScreen::_initLobby()
{
	if (game->lobbySettings.map.empty())
		game->lobbySettings.map = "d2k:maps/0PLAY0";
	this->_panel->loadWidgetsFromFile("assets/gui/lobby.gui");

	auto leave = this->_panel->get<tgui::Button>("Leave");
	auto start = this->_panel->get<tgui::Button>("Start");
	auto change = this->_panel->get<tgui::Button>("MapChange");
	auto zoom = this->_panel->get<tgui::Button>("MapZoom");
	auto chat = this->_panel->get<tgui::ChatBox>("ChatBox");
	auto players = this->_panel->get<tgui::Panel>("Players");
	auto settings = this->_panel->get<tgui::Panel>("Settings");
	auto map = this->_panel->get<tgui::Picture>("Map");
	auto size = this->_panel->get<tgui::Label>("MapSize");
	auto name = this->_panel->get<tgui::Label>("MapName");
	auto origin = this->_panel->get<tgui::Label>("MapOrigin");
	auto author = this->_panel->get<tgui::Label>("MapAuthor");
	auto objects = this->_panel->get<tgui::Label>("MapObjects");
	auto panel = tgui::Panel::create({"100%", 30});
	auto addPlayer = tgui::Button::create("+");

	addPlayer->setPosition(10, 0);
	addPlayer->setSize("&.w - 362", 20);
	panel->add(addPlayer);
	panel->setPosition(0, 10);
	players->add(panel, "AddPlayer");
	this->_addPlayerPanels(players, start);
	addPlayer->onClick([this, players, start] {
		game->lobbySettings.players.push_back(PlayerArg{
			game->modManager.getFactionList()[0],
			{},
			{},
			HSLColor(
				random_distrib(game->random, 0, 360),
				random_distrib(game->random, 0, 1000),
				random_distrib(game->random, 0, 1000)
			),
			static_cast<char>(game->lobbySettings.players.size()),
			game->lobbySettings.players.empty()
		});
		this->_addPlayerPanels(players, start);
		this->_updatePlayerList(start);
	});
	start->onClick([] {
		auto params = new InGameArguments();

		params->reportProgressA = [](const std::string &){};
		params->startParams.map = game->lobbySettings.map;
		params->startParams.seed = game->random();
		params->startParams.settings = game->lobbySettings.settings;
		genSpawns(game->lobbySettings.players, params->startParams);
		game->scene.switchScene("in_game", params);
	});
	leave->onClick([this] {
		game->lobbySettingsValid = false;
		this->_initTitle();
	});

	this->_map.load(game->lobbySettings.map);
	this->_updatePlayerList(start);
	this->_updateMapLobby(map, size, name, origin, author, objects);
}

void TitleScreen::_openReplayDialog()
{
	auto window = openFileDialog("Open replay", game->data/ "replays");

	window->setFileTypeFilters({ {"Replay file (*.replay)", {"*.replay"}}, {"All files", {}} }, 0);
	window->onFileSelect([this](const std::vector<tgui::Filesystem::Path> &arr){
		try {
			this->_loadReplay(arr[0]);
		} catch (std::exception &e) {
			dispMsg(game->gui, "Replay loading failed", "This replay is invalid, corrupted or was created for a different version of the game: " + std::string(e.what()));
		}
	});
}

void TitleScreen::_openLoadDialog()
{
	auto window = openFileDialog("Open state", game->data / "saves");

	window->setFileTypeFilters({ {"State file (*.state)", {"*.state"}}, {"All files", {}} }, 0);
	window->onFileSelect([this](const std::vector<tgui::Filesystem::Path> &arr){
		try {
			this->_loadState(arr[0]);
		} catch (std::exception &e) {
			dispMsg(game->gui, "State loading failed", "This state is invalid, corrupted or was created for a different version of the game: " + std::string(e.what()));
		}
	});
}

void TitleScreen::_loadState(const std::filesystem::path &path)
{
	auto file = std::make_shared<SaveStateFile>(SaveStateFile::load(path));
	// TODO: Check CRCs before starting loading
	auto args = new InGameArguments();

	args->startParams = file->start;
	args->state = std::move(file);
	game->scene.switchScene("in_game", args);
}

void TitleScreen::_loadReplay(const std::filesystem::path &path)
{
	ReplayData replay = ReplayData::load(path);
	// TODO: Check CRCs before starting loading
	auto args = new ReplayArguments();

	args->replay = std::move(replay);
	game->scene.switchScene("replay_in_game", args);
}

TitleScreen::TitleScreen(std::optional<std::string> message)
{
	this->_panel = tgui::Panel::create({"100%", "100%"});
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Black);
	if (game->lobbySettingsValid)
		this->_initLobby();
	else
		this->_initTitle();
	if (message)
		dispMsg(*this->_panel, "Error", *message);
}

TitleScreen::~TitleScreen() = default;

void TitleScreen::render() const
{
	reloadViews(*game->screen, game->gui, game->screen->getSize(), {0, 0});
}

void TitleScreen::renderAbove() const
{
}

void TitleScreen::update()
{
	game->screen->setMouseCursorVisible(true);
	game->screen->setMouseCursorGrabbed(false);
}

void TitleScreen::consumeEvent(const sf::Event &event)
{
}

tgui::Panel::Ptr TitleScreen::getPanel() const
{
	return this->_panel;
}

TitleScreen *TitleScreen::create(SceneArguments *args)
{
	std::optional<std::string> message;

	if (args) {
		checked_cast(params, TitleScreenArguments, args);
		message = std::move(params->message);
	}
	return new TitleScreen(std::move(message));
}
