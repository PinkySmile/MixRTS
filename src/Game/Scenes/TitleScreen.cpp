//
// Created by PinkySmile on 09/11/2026.
//

#include <TGUI/Widgets/Button.hpp>
#include "TitleScreen.hpp"
#include "Data/Vector.hpp"
#include "Data/Map.hpp"
#include "Data/Replay.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

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
	load->onClick([] {

	});
	settings->onClick([] {

	});
	debug->onClick([] {

	});
	quit->onClick([] {
		game->screen->close();
	});
}

void TitleScreen::_openReplayDialog()
{
	auto window = openFileDialog("Open replay", "replays");

	window->setFileTypeFilters({ {"Replay file (*.replay)", {"*.replay"}}, {"All files", {}} }, 0);
	window->onFileSelect([this](const std::vector<tgui::Filesystem::Path> &arr){
		try {
			this->_loadReplay(arr[0]);
		} catch (std::exception &e) {
			dispMsg(game->gui, "Replay loading failed", "This replay is invalid, corrupted or was created for a different version of the game: " + std::string(e.what()));
		}
	});
}

void TitleScreen::_loadReplay(const std::filesystem::path &path)
{
	ReplayData replay = ReplayData::load(path);
	uint32_t crc = LoadedMap::computeChecksum(replay.map);

	if (crc != replay.mapCrc)
		throw std::runtime_error("This replay's map checksum does not match the installed map");

	auto args = new ReplayArguments();

	args->replay = std::move(replay);
	game->scene.switchScene("replay_in_game", args);
}

TitleScreen::TitleScreen(std::optional<std::string> message)
{
	this->_panel = tgui::Panel::create({"100%", "100%"});
	this->_initTitle();
	if (message)
		dispMsg(*this->_panel, "Error", *message);
}

TitleScreen::~TitleScreen() = default;

void TitleScreen::render() const
{
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
