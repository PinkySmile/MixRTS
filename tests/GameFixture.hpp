//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_TESTS_GAMEFIXTURE_HPP
#define MIXRTS_TESTS_GAMEFIXTURE_HPP


#include <filesystem>
#include <memory>
#include <vector>
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

//! A part of the world that a test is about (see GameFixture::focusCamera()).
//!
//! It is a rectangle in world coordinates, that can be built from a point, a
//! rectangle, an object (its whole hitbox) or a block of tiles. A region built
//! from a null object is empty: it is ignored, so that tests can build their
//! regions straight from spawnObject()'s result.
struct FocusRegion {
	Vector2f topLeft;
	Vector2f size;
	bool empty = false;

	FocusRegion(Vector2f topLeft, Vector2f size) :
		topLeft(topLeft),
		size(size)
	{
	}

	//! A single point (the margin given to GameFixture::focusCamera() is what gives it some room).
	FocusRegion(Vector2f point) :
		topLeft(point),
		size(0, 0)
	{
	}

	FocusRegion(const GameObject *object) :
		topLeft(object ? object->getPosition().to<float>() : Vector2f{}),
		size(object ? object->getSize().to<float>() : Vector2f{}),
		empty(!object)
	{
	}

	FocusRegion(const GameObject &object) :
		FocusRegion(&object)
	{
	}

	//! A block of \a count tiles, the first one being at \a tile.
	static FocusRegion tiles(const MapState &map, Vector2i tile, Vector2u count = {1, 1})
	{
		return {
			map.topLeftCoordToAbs(tile).to<float>(),
			Vector2f(static_cast<float>(count.x * map.tileSize.x), static_cast<float>(count.y * map.tileSize.y))
		};
	}
};

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

	//! Centers the camera on the middle of everything in \a regions, and zooms
	//! as much as possible while still showing all of it (and \a margin more
	//! pixels around it) in the part of the screen that isn't covered by the UI.
	//! The camera is no longer kept inside the map afterward, so that what is
	//! shown stays exactly centered, even close to the map's border.
	//!
	//! It is meant to show what a test is about on the video recorded by
	//! --mode=record (or on the window of --mode=debug): call it again whenever
	//! the interesting part of the test moves somewhere else. Every test starts
	//! with the camera on the player's MCV (see SetUp()).
	void focusCamera(const std::vector<FocusRegion> &regions, float margin = 64, float maxZoom = 3);

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
