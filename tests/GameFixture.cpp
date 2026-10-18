//
// Created for the MixRTS test target.
//

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

	this->manager = std::make_unique<GameManager>(allFactions, 1);
	this->manager->loadMap(*_mod, "mods/dune2000/maps/8PLAY1");
	this->manager->addPlayer(0, HSLColor{214, 375, 919}, 0, 0);
	this->player = &this->manager->getPlayers()[0];
	this->player->setEnabled(true);
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
