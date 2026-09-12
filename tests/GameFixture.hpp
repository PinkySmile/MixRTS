//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_TESTS_GAMEFIXTURE_HPP
#define MIXRTS_TESTS_GAMEFIXTURE_HPP


#include <filesystem>
#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>
#include "Data/Animation.hpp"
#include "Data/Effect.hpp"
#include "Data/Faction.hpp"
#include "Data/GameManager.hpp"
#include "Data/Player.hpp"
#include "Data/Projectile.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Warhead.hpp"
#include "Data/Weapon.hpp"
#include "Resources/Game.hpp"

//! Base fixture for tests that need to run an actual game simulation.
//!
//! The "d2k" mod is loaded once for the whole test binary (SetUpTestSuite(),
//! shared by every test using this fixture, since constructing Game and
//! loading the mod's assets is fairly expensive and Game itself can only ever
//! be instantiated once per process). Every individual test then gets its own
//! fresh GameManager with a single "d2k:atreides" player spawned on
//! mods/dune2000/maps/8PLAY1 (SetUp()), so that tests don't affect each other.
//!
//! Tests using this fixture must be run with the repository root as the
//! current working directory, since the mod/font paths below are relative to
//! it. This is set up for ctest by the add_test(... WORKING_DIRECTORY ...)
//! call in CMakeLists.txt.
class GameFixture : public ::testing::Test {
public:
	//! Lets tests/main.cpp (which parses them from the command line) set the
	//! Mode/video path/window size the shared Game instance below will be
	//! constructed with. Must be called (if at all) before the very first
	//! SetUpTestSuite() call, i.e. before RUN_ALL_TESTS() - it has no effect
	//! afterward, since Game can only ever be constructed once per process.
	static void configure(MockMode mode, std::filesystem::path video, Vector2u size);

protected:
	static void SetUpTestSuite();

	void SetUp() override;

	//! The atreides player's currently owned MCV, or nullptr if there isn't one
	//! (e.g. because it just got deployed into a construction yard).
	[[nodiscard]] GameObject *getMcv() const;

	void tick(unsigned steps = 1);
	std::unique_ptr<GameManager> manager;
	Player *player = nullptr;

private:
	// Intentionally never destroyed/reset: just like main.cpp, which never
	// deletes "game" on its happy path either (some engine singletons aren't
	// safe to tear down, only to leak until process exit).
	static Game *_game;
	static Mod *_mod;
	static std::unordered_map<std::string, Faction> _factions;

	// Defaults match the ones GameFixture used to hardcode before configure()
	// existed. Overridable from the command line, see tests/main.cpp.
	static MockMode _mode;
	static std::filesystem::path _video;
	static Vector2u _size;
};


#endif //MIXRTS_TESTS_GAMEFIXTURE_HPP
