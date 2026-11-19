//
// Created for the MixRTS test target.
//
// Regression tests for the Dune 2000 carryall (aka "carrier", "d2k:carr")
// pickup/delivery behavior. They cover 3 known bugs, all now fixed:
//
//   1. A carryall dropping a unit off never checked whether the destination
//      tile (and, for the carryall itself, the 2x2 tiles it needs to land
//      on) was actually free before placing the carried unit there. Fixed
//      by having GameObject::_updatePickup() re-validate the landing spot
//      right before touching down and, if it's no longer free, pick the
//      closest free alternative instead (see GameObject::_findLandingSpot).
//   2. A carryall that is already busy carrying one unit could in theory
//      still be selected by another unit requesting a pickup. This was
//      already fixed prior to writing these tests: GameObject::canPickup()
//      rejects any carrier with a non-empty order queue.
//   3. Flying units (e.g. a carryall at altitude) were counted by
//      GameManager::isAreaFree()/_isInArea(), so they incorrectly blocked
//      building placement on the ground tile(s) below them. Fixed by having
//      _isInArea() ignore any object with a non-zero altitude.
//
// CarryallHarvestOrderIsCarriedToSpiceField and
// CarryallDumpOrderIsCarriedToRefinery are baseline sanity tests: they don't
// exercise any of the 3 bugs above. All tests below are expected to PASS.
//

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Mock/MockedUI.hpp"
#include "../src/Engine/Data/Objects/GameObject.hpp"
#include "Data/GameManager.hpp"
#include "Data/Map.hpp"
#include "Data/GridMask.hpp"

namespace {

//! Distance (in the "dist" field of d2k:carr's "pickup" behavior, see
//! mods/dune2000/units/units.json) above which a unit requesting to move
//! becomes eligible for a carryall pickup. Tests place units well beyond
//! this so a pickup is unambiguously expected.
constexpr double CARRYALL_PICKUP_DIST = 320;

//! d2k:carr uses "spawn_params": {"type": "appear", "x": "close_border", ...},
//! meaning a freshly spawned carryall actually first appears at the map's
//! border and only then flies in towards the position it was "spawned" at
//! (see GameObject::spawn()), spending its very first many ticks doing so
//! (and, crucially, not running its idle "wait for a pickup" behavior at all
//! until that initial flight-in is done). That's the right behavior for
//! reinforcements, but not what these tests want (an already-idle carryall
//! sitting on the field, ready to be assigned a pickup) - so every carryall
//! spawned by a test goes through this helper, which drops it directly onto
//! its intended spot and cancels the redundant flight-in order.
GameObject *spawnIdleCarryallAt(GameManager &manager, size_t owner, Vector2i absPos)
{
	GameObject *carryall = manager.spawnObject(owner, absPos, "", "d2k:carr");

	if (carryall) {
		carryall->setPosition(Vector2f(absPos));
		carryall->clearOrders();
	}
	return carryall;
}

} // namespace

//! Baseline sanity check (not related to any of the 3 known bugs above):
//! a harvester placed far enough away from a spice field that reaching it
//! by itself would exceed the carryall's pickup distance should, when given
//! a HarvestOrder, be picked up by an idle carryall sitting on the field and
//! flown over to the requested tile, then actually harvest there.
//! Should currently PASS.
TEST_F(GameFixture, CarryallHarvestOrderIsCarriedToSpiceField)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i carryallTile = findFreePassableTile(*this->manager, map, spiceTile, 4, {2, 2}, GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_ALL_CAN_PASS);
	Vector2i farTile = findFreePassableTile(*this->manager, map, spiceTile, 20, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);

	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, tileCenterAbs(map, carryallTile));
	GameObject *harvester = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, farTile), "", "d2k:harv");

	ASSERT_NE(carryall, nullptr);
	ASSERT_NE(harvester, nullptr);
	ASSERT_GE(Vector2i(farTile).distance(spiceTile) * map.tileSize.x, CARRYALL_PICKUP_DIST) << "The harvester needs to be far enough away for a pickup to be expected";
	// The carryall flies to the harvester, then both of them go to the spice field.
	this->focusCamera({carryall, harvester, FocusRegion::tiles(map, spiceTile)});

	Vector2i spiceAbs = tileCenterAbs(map, spiceTile);
	auto *mockedUi = installMockedUI(*this->player);

	mockedUi->orderQueue.emplace_back(UI::HarvestUnitEvent{{harvester->index}, Vector2u(spiceAbs.to<unsigned>())});

	bool wasCarried = false;
	std::optional<float> storage;

	for (unsigned i = 0; i < 3000 && !storage; i++) {
		this->tick(1);
		wasCarried = wasCarried || harvester->isBeingPickedUp();
		storage = harvester->getStorageDisplay();
		if (storage && *storage <= 0.f)
			storage.reset();
	}

	EXPECT_TRUE(wasCarried) << "A harvester sent so far away should have been picked up by the idle carryall";
	ASSERT_TRUE(storage.has_value()) << "The harvester never harvested anything";
	EXPECT_GT(*storage, 0.f);
}

//! Baseline sanity check (not related to any of the 3 known bugs above):
//! a harvester placed far away from a spice field, given a DumpOrder while a
//! carryall is idle nearby and a refinery sits on buildable land, should be
//! picked up and flown straight to the refinery's dump tile.
//! Should currently PASS.
TEST_F(GameFixture, CarryallDumpOrderIsCarriedToRefinery)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i carryallTile = findFreePassableTile(*this->manager, map, spiceTile, 4, {2, 2}, GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_ALL_CAN_PASS);
	Vector2i farTile = findFreePassableTile(*this->manager, map, spiceTile, 20, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i refineryTile = findFreeBuildableTile(*this->manager, map, spiceTile, 12, {3, 3});

	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, tileCenterAbs(map, carryallTile));
	GameObject *harvester = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, farTile), "", "d2k:harv");
	GameObject *refinery = this->manager->spawnObject(this->player->state.index, map.topLeftCoordToAbs(refineryTile), "", "d2k:refi");

	ASSERT_NE(carryall, nullptr);
	ASSERT_NE(harvester, nullptr);
	ASSERT_NE(refinery, nullptr);

	// d2k:refi comes with a free starting harvester (its "spawn.unit"), which
	// would otherwise immediately compete with our own harvester for the
	// only carryall on the map (via its own "spawn_orders": ["harvest"]).
	// Get rid of it so this test unambiguously exercises our harvester only.
	// The refinery also spawns that harvester already being carried by a
	// carryall of its own: that one has to go too (and first, with its
	// orders dropped), otherwise it would keep a dangling pointer to the
	// harvester once the latter is disposed of.
	for (auto *c : this->player->getObjects("d2k:carr"))
		if (c != carryall) {
			c->clearOrders();
			c->damage(999999, DMGTYPE_VOID);
		}
	for (auto *h : this->player->getObjects("d2k:harv"))
		if (h != harvester) {
			h->clearOrders();
			h->damage(999999, DMGTYPE_VOID);
		}

	Vector2i dumpTile = map.absToTileCoord(refinery->getPosition() + refinery->getPoint());
	auto *mockedUi = installMockedUI(*this->player);

	mockedUi->orderQueue.emplace_back(UI::DumpUnitEvent{{harvester->index}, refinery->index});
	// The carryall flies to the harvester, then both of them go to the refinery.
	this->focusCamera({carryall, harvester, refinery});

	bool wasCarried = false;
	bool reachedDumpTile = false;

	for (unsigned i = 0; i < 3000 && !reachedDumpTile; i++) {
		this->tick(1);
		wasCarried = wasCarried || harvester->isBeingPickedUp();
		if (map.absToTileCoord(harvester->getPosition()) == dumpTile)
			reachedDumpTile = true;
	}

	EXPECT_TRUE(wasCarried) << "A harvester sent so far away should have been picked up by the idle carryall";
	EXPECT_TRUE(reachedDumpTile) << "The harvester should have been flown to the refinery's dump tile";
}

//! Bug #1: a carryall dropping off a unit doesn't check that the destination
//! tile is actually free. Here, a trike drives onto the exact tile a
//! harvester was sent to harvest while the carryall carrying that harvester
//! is still in flight; the fixed behavior is for the carryall to notice the
//! tile is now blocked and land the harvester on a different, free tile
//! next to it instead of on top of the trike.
//! Should currently PASS.
TEST_F(GameFixture, CarryallLandingSpotBlockedByUnitPicksAnotherTile)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i carryallTile = findFreePassableTile(*this->manager, map, spiceTile, 8, {2, 2}, GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_ALL_CAN_PASS);
	Vector2i farTile = findFreePassableTile(*this->manager, map, spiceTile, 20, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i trikeTile = findFreePassableTile(*this->manager, map, spiceTile, 2, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);

	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, tileCenterAbs(map, carryallTile));
	GameObject *harvester = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, farTile), "", "d2k:harv");
	GameObject *trike = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, trikeTile), "", "d2k:trik");

	ASSERT_NE(carryall, nullptr);
	ASSERT_NE(harvester, nullptr);
	ASSERT_NE(trike, nullptr);

	Vector2i spiceAbs = tileCenterAbs(map, spiceTile);
	auto *mockedUi = installMockedUI(*this->player);

	mockedUi->orderQueue.emplace_back(UI::HarvestUnitEvent{{harvester->index}, Vector2u(spiceAbs.to<unsigned>())});
	// The carryall flies to the harvester and brings it back to the spice, where the trike goes too.
	this->focusCamera({carryall, harvester, trike, FocusRegion::tiles(map, spiceTile)});

	// Let the pickup actually start (carryall commits to flying towards the
	// harvester) before moving the trike in: the carryall is a long way off
	// still, so there is plenty of time left for the trike to reach and
	// occupy the target tile well before the carryall's return trip.
	this->tick(20);
	ASSERT_TRUE(harvester->isBeingPickedUp()) << "The pickup should have already started";

	mockedUi->orderQueue.emplace_back(UI::MoveUnitEvent{{trike->index}, Vector2u(spiceAbs.to<unsigned>())});

	for (unsigned i = 0; i < 4000 && harvester->isBeingPickedUp(); i++)
		this->tick(1);
	// Let any post-drop re-routing (e.g. re-pathing towards the actual spice)
	// settle down.
	this->tick(20);

	Vector2i trikeFinalTile = map.absToTileCoord(trike->getPosition());
	Vector2i harvesterFinalTile = map.absToTileCoord(harvester->getPosition());

	ASSERT_EQ(trikeFinalTile, spiceTile) << "Setup issue: the trike should have reached and be blocking the requested tile";
	EXPECT_NE(harvesterFinalTile, spiceTile) << "The carryall should not have dropped the harvester on the tile occupied by the trike";
	// The carryall itself is 2x2 tiles and lands centered on the drop tile,
	// so any tile directly touching the blocked one would still have the
	// carryall's own footprint overlapping it (its footprint always
	// extends half a tile past the drop tile on every side) - the closest
	// it can actually land is 2 tiles away (e.g. spiceTile + {2, 0}, or a
	// diagonal {2, 2}, hence up to 2*sqrt(2) away).
	EXPECT_LE(harvesterFinalTile.distance(spiceTile), 2 * std::sqrt(2.0)) << "The carryall should have picked the closest tile it could actually land on";
	EXPECT_GE(harvesterFinalTile.distance(spiceTile), 2) << "The carryall's own 2x2 footprint should have kept it from landing any closer to the blocked tile";
	EXPECT_TRUE(this->manager->isAreaFree(harvester->getPosition() - Vector2i(harvester->getSize()) / 2, harvester->getSize(), {harvester->index}))
		<< "The harvester should not have been dropped on top of the trike";
	EXPECT_TRUE(this->manager->isAreaFree(carryall->getPosition() - Vector2i(carryall->getSize()) / 2, carryall->getSize(), {carryall->index}))
		<< "The carryall should not end up overlapping the trike either";
}

//! Bug #1 again, this time blocked by buildings instead of a moving unit: a
//! refinery is sold and replaced by a 5x5 grid of silos (leaving only the
//! originally requested dump tile, right in the middle of that grid, empty)
//! while a harvester is being carried there to dump its resources. Since the
//! carryall itself needs a 2x2 tile clearance to land, and every tile
//! touching the requested one is now covered by a silo, none of the 4
//! possible 2x2 landing spots including the requested tile actually fit
//! anymore. The fixed behavior is for the carryall to notice this and land
//! the harvester on some other free tile further out instead.
//! Should currently PASS.
TEST_F(GameFixture, CarryallLandingSpotBlockedByBuildingsPicksAnotherTile)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i carryallTile = findFreePassableTile(*this->manager, map, spiceTile, 4, {2, 2}, GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_ALL_CAN_PASS);
	// Big enough margin (7x7) around the refinery for the later 5x5 silo grid
	// to fit without needing to re-search for space.
	Vector2i refineryTile = findFreeBuildableTile(*this->manager, map, spiceTile, 12, {7, 7});

	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, tileCenterAbs(map, carryallTile));
	GameObject *refinery = this->manager->spawnObject(this->player->state.index, map.topLeftCoordToAbs(refineryTile + Vector2i{2, 2}), "", "d2k:refi");
	auto harvList = this->player->getObjects("d2k:harv");
	ASSERT_EQ(harvList.size(), 1);
	GameObject *harvester = harvList.front();

	ASSERT_NE(carryall, nullptr);
	ASSERT_NE(harvester, nullptr);
	ASSERT_NE(refinery, nullptr);

	Vector2i dumpTile = map.absToTileCoord(refinery->getPosition() + refinery->getPoint());
	auto *mockedUi = installMockedUI(*this->player);

	// The carryall brings the harvester that came with the refinery to it.
	this->focusCamera({carryall, harvester, FocusRegion::tiles(map, refineryTile, {7, 7})});

	// Wait for harvester to be delivered
	for (unsigned i = 0; i < 4000 && harvester->isBeingPickedUp(); i++)
		this->tick();
	ASSERT_TRUE(!harvester->isBeingPickedUp()) << "The harvester should be delivered";

	// Move away from refinery
	auto old = harvester->getPosition();

	mockedUi->orderQueue.emplace_back(UI::MoveUnitEvent{{harvester->index}, old + Vector2f{700, 0}});
	// The harvester goes away from the refinery.
	this->focusCamera({carryall, harvester, FocusRegion::tiles(map, refineryTile, {7, 7}), Vector2f(old + Vector2i{700, 0})});
	for (unsigned i = 0; i < 4000 && harvester->getPosition().x - old.x < 500; i++)
		this->tick();
	ASSERT_TRUE(harvester->getPosition().x - old.x >= 500) << "The harvester should be far";

	// Wait for harvester to be picked up (from where it stands, to the refinery)
	mockedUi->orderQueue.emplace_back(UI::DumpUnitEvent{{harvester->index}, refinery->index});
	for (unsigned i = 0; i < 4000 && !harvester->isBeingPickedUp(); i++)
		this->tick();
	ASSERT_TRUE(harvester->isBeingPickedUp()) << "The pickup should have already started";

	// Sell the refinery and cover its former footprint (and then some) with a 7x7 dummy building.
	refinery->sell();
	GameObject *dummy = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, dumpTile - Vector2i{3, 3}), "", "test:dummy7x7");
	EXPECT_NE(dummy, nullptr);

	for (unsigned i = 0; i < 4000 && harvester->isBeingPickedUp(); i++)
		this->tick();
	this->tick(20);

	Vector2i harvesterFinalTile = map.absToTileCoord(harvester->getPosition());

	EXPECT_NE(harvesterFinalTile, dumpTile) << "The carryall cannot fit its cargo 1x1 footprint on the requested tile anymore (it's surrounded by silos), so it should have landed elsewhere";
	EXPECT_TRUE(this->manager->isAreaFree(carryall->getPosition() - Vector2i(carryall->getSize()) / 2, carryall->getSize(), {carryall->index})) << "The carryall should not end up overlapping any of the silos";
}

//! Bug #2 regression test: a carryall that is already committed to carrying
//! one unit must not be picked by GameManager::findPickup() for a second,
//! unrelated pickup request, since it obviously cannot service both at
//! once (GameObject::canPickup() rejects carriers with a non-empty order
//! queue). Here, a second harvester requests a pickup while the (single)
//! carryall on the map is already flying out to fetch the first one: the
//! busy carryall must be skipped, so the second harvester simply walks to
//! the spice field by itself instead of waiting on a pickup that will
//! never come for it.
//! Should currently PASS.
TEST_F(GameFixture, BusyCarryallIsNotAssignedToASecondPickup)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i carryallTile = findFreePassableTile(*this->manager, map, spiceTile, 4, {2, 2}, GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_ALL_CAN_PASS);
	Vector2i farTile1 = findFreePassableTile(*this->manager, map, spiceTile, 20, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i farTile2 = findFreePassableTile(*this->manager, map, farTile1, 3, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS, 5);

	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, tileCenterAbs(map, carryallTile));
	GameObject *harvester1 = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, farTile1), "", "d2k:harv");
	GameObject *harvester2 = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, farTile2), "", "d2k:harv");

	ASSERT_NE(carryall, nullptr);
	ASSERT_NE(harvester1, nullptr);
	ASSERT_NE(harvester2, nullptr);

	// d2k:harv comes with "spawn_orders": ["harvest"], which would otherwise
	// have both harvesters race for the only carryall on the map on their
	// own (before either of our own, explicit HarvestUnitEvents below even
	// gets a chance to run) - cancel that so this test unambiguously
	// controls which harvester asks for a pickup, and in which order.
	harvester1->clearOrders();
	harvester2->clearOrders();

	Vector2i spiceAbs = tileCenterAbs(map, spiceTile);
	Vector2i harvester2Start = harvester2->getPosition();
	auto *mockedUi = installMockedUI(*this->player);

	this->focusCamera({carryall, harvester1, harvester2, FocusRegion::tiles(map, spiceTile)});

	mockedUi->orderQueue.emplace_back(UI::HarvestUnitEvent{{harvester1->index}, Vector2u(spiceAbs.to<unsigned>())});
	this->tick(20);
	ASSERT_TRUE(harvester1->isBeingPickedUp()) << "The first harvester's pickup should have already started";

	mockedUi->orderQueue.emplace_back(UI::HarvestUnitEvent{{harvester2->index}, Vector2u(spiceAbs.to<unsigned>())});
	this->tick(300);

	EXPECT_FALSE(harvester2->isBeingPickedUp()) << "The only carryall on the map is already busy with the first harvester, so the second one should not have been assigned a pickup at all";
	EXPECT_NE(harvester2->getPosition(), harvester2Start) << "With no carryall available, the second harvester should have started walking to the spice field by itself instead of waiting indefinitely";

	// The first harvester's pickup should not have been disrupted by the
	// second harvester's (bogus) request.
	for (unsigned i = 0; i < 4000 && harvester1->isBeingPickedUp(); i++)
		this->tick(1);
	EXPECT_EQ(map.absToTileCoord(harvester1->getPosition()), spiceTile) << "The first harvester should still have been flown all the way to the requested tile";
}

//! Bug #3: flying units (e.g. a carryall sitting at its cruising altitude)
//! are still counted by GameManager::isAreaFree(), so they incorrectly
//! prevent building on the ground tile(s) below/around them. Here, a
//! building is placed, through the same UI mouse-input path a real player
//! would use (Player::onMouseMoved()/onLeftMouseClicked(), forwarded to the
//! real D2KUI), on a tile directly under a flying carryall; since the
//! carryall is airborne, this should succeed exactly as if the tile were
//! empty.
//! Should currently PASS.
TEST_F(GameFixture, BuildingCanBePlacedUnderAFlyingCarryall)
{
	GameObject *mcv = this->getMcv();
	MapState &map = *mcv->map;

	// Build a construction yard first (the same way DeployTests.cpp does),
	// since every building queue needs one before anything can be unlocked.
	mcv->deploy();
	this->tick(100);

	GameObject *conyard = nullptr;
	for (auto obj : this->player->state.myObjects)
		if (obj->getId() == "d2k:a_cony")
			conyard = obj;
	ASSERT_NE(conyard, nullptr) << "The MCV should have deployed into a construction yard";

	ASSERT_TRUE(this->player->state.queues["d2k:building"].entries.contains("d2k:wtra"));
	auto &queue = this->player->state.queues["d2k:building"].entries.at("d2k:wtra");
	ASSERT_TRUE(queue.unlocked) << "The wind trap should be unlocked once a construction yard has been built";

	// Fast-forward straight to "fully built, waiting to be placed": how a
	// build queue actually fills up over time is thoroughly unrelated to this
	// bug and already covered by DeployTests.cpp/other tests.
	queue.buildProgress = queue.buildTime;

	// Find a spot right next to the construction yard (build_max_distance is
	// only 1 tile for regular buildings) where the wind trap could otherwise
	// be placed without any issue.
	Vector2i conyardTile = map.absToTileCoord(conyard->getPosition());
	Vector2i placementTile = findFreeBuildableTile(*this->manager, map, conyardTile, 1, {2, 2});
	Vector2i placementAbs = map.topLeftCoordToAbs(placementTile);

	this->focusCamera({conyard, FocusRegion::tiles(map, placementTile, {2, 2})});

	ASSERT_TRUE(this->manager->canSpawnBuilding(this->player->state.index, placementAbs, "d2k:wtra", {}))
		<< "Setup issue: the wind trap should be placeable there before the carryall shows up";

	// Park a flying carryall exactly over that spot.
	GameObject *carryall = spawnIdleCarryallAt(*this->manager, this->player->state.index, placementAbs + Vector2i(map.tileSize.to<int>()));
	ASSERT_NE(carryall, nullptr);
	ASSERT_GT(carryall->getAltitude(), 0) << "Setup issue: the carryall should be flying, not sitting on the ground";

	// Select the (now ready) wind trap cameo and click the target tile,
	// exactly like a real player would with the mouse.
	auto &ui = this->player->getCombinedUI().getUIs()[this->player->state.factionIndex];

	ui->selectBuildingForPlacement("d2k:wtra");
	this->player->onMouseMoved(placementAbs);
	this->player->onLeftMouseClicked(placementAbs);
	this->tick(1);

	GameObject *newBuilding = nullptr;
	for (auto obj : this->player->state.myObjects)
		if (obj->getId() == "d2k:wtra")
			newBuilding = obj;

	EXPECT_NE(newBuilding, nullptr) << "The wind trap should have been built: the only thing over its tile is a flying carryall, which shouldn't block ground construction";
}
