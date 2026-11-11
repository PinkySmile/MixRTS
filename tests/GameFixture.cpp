//
// Created for the MixRTS test target.
//

#include <algorithm>
#include <optional>
#include "GameFixture.hpp"
#include "Resources/Assert.hpp"

Game *GameFixture::_game = nullptr;
Mod *GameFixture::_mod = nullptr;
std::unordered_map<std::string, Faction> GameFixture::_factions;
MockMode GameFixture::_mode = MODE_NORMAL;
std::filesystem::path GameFixture::_video = "video";
Vector2u GameFixture::_size{1280, 1024};

void GameFixture::configure(MockMode mode, std::filesystem::path video, Vector2u size)
{
	// Game (and the Mode/video/size it was constructed with) can't be changed
	// once SetUpTestSuite() has run for the first time.
	assert_exp(!_game);
	_mode = mode;
	_video = std::move(video);
	_size = size;
}

void GameFixture::SetUpTestSuite()
{
	if (_game)
		return;

	// The real game scales the views (and so the UI) by guiScale, see reloadViews() in
	// src/main.cpp. Nothing does in the tests, so the UI is displayed at its native size.
	// Keep guiScale consistent with that, or the part of the screen the UI covers
	// (see GameManager::fitCameraTo()) would be wrong.
	guiScale = 1;
	// Deliberately leaked, see the comment on the declaration in GameFixture.hpp.
	_game = new Game("assets/NotoSans-Regular.ttf", _mode, _video, _size);
	game->modManager.loadMod("mods/dune2000/");
	game->modManager.loadMod("tests/");
	_mod = &game->modManager.get("d2k");
	_mod->loadFactions("factions.json", _factions);
	game->modManager.get("test").loadFactions("factions.json", _factions);
}

void GameFixture::SetUp()
{
	assert_exp(_mod);

	std::vector allFactions{
		_factions.at("d2k:atreides"),
		_factions.at("d2k:harkonnen"),
		_factions.at("d2k:ordos"),
		_factions.at("test:dummy"),
	};

	this->manager = std::make_unique<GameManager>(allFactions, 2, LobbySettings{false});
	this->manager->loadMap(*_mod, "mods/dune2000/maps/8PLAY1");
	this->manager->addPlayer(0, HSLColor{214, 375, 919}, 0, 0);
	this->player = &this->manager->getPlayers()[0];
	this->player->setEnabled(true);
	// Tests that are about something else than the player's base move the camera themselves.
	this->focusCamera({this->getMcv()});
}

void GameFixture::focusCamera(const std::vector<FocusRegion> &regions, float margin, float maxZoom)
{
	assert_exp(this->manager);

	std::optional<Vector2f> min;
	std::optional<Vector2f> max;

	for (auto &region : regions) {
		if (region.empty)
			continue;

		Vector2f bottomRight = region.topLeft + region.size;

		if (!min) {
			min = region.topLeft;
			max = bottomRight;
			continue;
		}
		min = Vector2f{std::min(min->x, region.topLeft.x), std::min(min->y, region.topLeft.y)};
		max = Vector2f{std::max(max->x, bottomRight.x), std::max(max->y, bottomRight.y)};
	}
	if (!min)
		return;
	this->manager->debugOptions.limitCamera = false;
	this->manager->fitCameraTo(*min, *max - *min, margin, 0.05f, maxZoom);
}

GameObject *GameFixture::getMcv() const
{
	for (auto obj : this->player->state.myObjects)
		if (obj->getId() == "d2k:mcv")
			return obj;
	return nullptr;
}

void GameFixture::tick(unsigned steps)
{
	assert_exp(_game);
	assert_exp(this->manager);

	while (steps--) {
		while (auto event = _game->screen->pollEvent())
			this->manager->consumeGeneralEvent(*event);

		this->manager->update();
		this->manager->updateRender();

		_game->screen->clear(Color{0x80, 0xFF, 0x40});
		this->manager->render();

		_game->gui.draw();
		this->manager->renderAbove();
		_game->screen->textSize(16);
		_game->screen->borderColor(1);
		_game->screen->displayElement(::testing::UnitTest::GetInstance()->current_test_info()->name(), {0, 0});
		_game->screen->borderColor();
		_game->screen->display();
		// Mixes/pushes one tick's worth of samples for whatever sound(s) just
		// got played this tick to the WAV file (MODE_RECORD only, a no-op
		// otherwise), keeping it in sync with the video pushed above.
		_game->soundMgr.update();
	}
}
