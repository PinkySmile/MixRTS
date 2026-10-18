//
// Created for the MixRTS test target.
//
// Regression tests for units fighting over the same tile.
//
// Grid-bound units now reserve the tile they are stepping into (instead of
// the one they are standing on) as soon as the step starts, which lets other
// units see that the tile is taken and not try to enter it as well. When the
// final destination of a unit is taken like this, the unit never asks the
// occupant to move aside: a regular mover simply stops next to it, and a
// harvester looks for another resource tile.
//

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Data/GameObject.hpp"
#include "Data/GameManager.hpp"
#include "Data/Map.hpp"
#include "Data/GridMask.hpp"

namespace {

int chebyshev(Vector2i a, Vector2i b)
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

//! Position (see GameObject::getPosition()) of a unit standing in the exact
//! middle of \a tile.
Vector2i restingPosition(const MapState &map, const GameObject &obj, Vector2i tile)
{
	return map.topLeftCoordToAbs(tile) + (map.tileSize.to<int>() - obj.getSize().to<int>()) / 2;
}

bool resourceLeftOn(const GameManager &manager, Vector2i tile)
{
	auto &grid = manager.getLoadedMap().grid;

	return GRID_RESOURCE_TYPE(grid.grid[tile.x + tile.y * grid.gridSize.x]) != 0;
}

}

//! Two harvesters are sent to harvest the very same tile. Only one of them can
//! stand on it: the other one must not push it around (the old behavior had
//! both walking in and calling moveAside() on each other), but should go
//! harvest another tile instead.
TEST_F(GameFixture, TwoHarvestersDoNotFightOverTheSameTile)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i startTile1 = findFreePassableTile(*this->manager, map, spiceTile, 3, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i startTile2 = findFreePassableTile(*this->manager, map, startTile1, 1, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);

	GameObject *harvesters[2] = {
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, startTile1), "", "d2k:harv"),
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, startTile2), "", "d2k:harv")
	};

	for (auto *h : harvesters) {
		ASSERT_NE(h, nullptr);
		// Don't let the spawn order ("harvest") start doing its own thing.
		h->clearOrders();
	}
	// Spawn (and the free harvester that may come with other buildings) aside,
	// the ones above are the only harvesters on the map.
	ASSERT_EQ(this->player->getObjects("d2k:harv").size(), 2);

	Vector2i spiceAbs = tileCenterAbs(map, spiceTile);

	for (auto *h : harvesters)
		h->harvest(spiceAbs, false);

	std::array<bool, 2> harvested = {false, false};
	std::array<Vector2i, 2> harvestTile;
	bool shared = false;
	bool pushed = false;

	for (unsigned i = 0; i < 4000 && !(harvested[0] && harvested[1]); i++) {
		this->tick();
		if (harvesters[0]->getOccupiedTile() == harvesters[1]->getOccupiedTile())
			shared = true;
		for (size_t j = 0; j < 2; j++) {
			auto storage = harvesters[j]->getStorageDisplay();
			bool full = storage && *storage >= 1.f;

			if (harvested[j]) {
				// Once it started harvesting, a harvester only leaves its tile
				// when there is nothing left to harvest on it (or it's full).
				if (harvesters[j]->getOccupiedTile() != harvestTile[j]) {
					if (!full && resourceLeftOn(*this->manager, harvestTile[j]))
						pushed = true;
					harvestTile[j] = harvesters[j]->getOccupiedTile();
				}
			} else if (storage && *storage > 0.f) {
				harvested[j] = true;
				harvestTile[j] = harvesters[j]->getOccupiedTile();
			}
		}
	}

	EXPECT_FALSE(shared) << "Two harvesters should never hold the same tile";
	EXPECT_FALSE(pushed) << "A harvester that started harvesting should never have been pushed away from its tile";
	EXPECT_TRUE(harvested[0]) << "The first harvester never harvested anything";
	EXPECT_TRUE(harvested[1]) << "The second harvester never harvested anything: it should have found another tile";
}

//! Two trikes are sent to the same tile. Only one of them can arrive, the
//! other one has to stop right next to it, without ever asking the one on the
//! tile to move.
TEST_F(GameFixture, TwoTrikesToSameTileOnlyOneArrives)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	Vector2i target = findFreePassableTile(*this->manager, map, spiceTile, 4, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i startTile1 = findFreePassableTile(*this->manager, map, target, 6, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i startTile2 = findFreePassableTile(*this->manager, map, startTile1, 2, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);

	GameObject *trikes[2] = {
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, startTile1), "", "d2k:trik"),
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, startTile2), "", "d2k:trik")
	};

	for (auto *t : trikes)
		ASSERT_NE(t, nullptr);
	for (auto *t : trikes)
		t->moveTo(tileCenterAbs(map, target), MOVEREASON_MOVE_COMMAND);

	std::array<bool, 2> arrived = {false, false};
	std::array<Vector2i, 2> arrivedAt;
	bool shared = false;
	bool moved = false;

	for (unsigned i = 0; i < 3000; i++) {
		this->tick();
		if (trikes[0]->getOccupiedTile() == trikes[1]->getOccupiedTile())
			shared = true;
		for (size_t j = 0; j < 2; j++) {
			if (arrived[j]) {
				// Nobody should have asked it to move since it got there.
				if (trikes[j]->getPosition() != arrivedAt[j])
					moved = true;
			} else if (trikes[j]->getOccupiedTile() == target && trikes[j]->getPosition() == restingPosition(map, *trikes[j], target)) {
				arrived[j] = true;
				arrivedAt[j] = trikes[j]->getPosition();
			}
		}
	}

	EXPECT_FALSE(shared) << "Two trikes should never hold the same tile";
	EXPECT_FALSE(moved) << "The trike on the destination should never have been pushed away";
	ASSERT_NE(arrived[0], arrived[1]) << "Exactly one trike should have made it to the destination";

	GameObject *winner = trikes[arrived[0] ? 0 : 1];
	GameObject *loser = trikes[arrived[0] ? 1 : 0];

	EXPECT_EQ(winner->getOccupiedTile(), target);
	EXPECT_NE(loser->getOccupiedTile(), target);
	EXPECT_EQ(chebyshev(loser->getOccupiedTile(), target), 1) << "The other trike should have stopped right next to the destination";
	EXPECT_EQ(loser->getPosition(), restingPosition(map, *loser, loser->getOccupiedTile())) << "The other trike should have finished its step and stopped";
}

//! Two trikes, one on each side of a tile, are sent to that tile at the same
//! time. As soon as one starts stepping into it the tile is reserved, so the
//! other one is blocked and must never start moving.
TEST_F(GameFixture, TwoAdjacentTrikesMeetingInTheMiddleOnlyOneStarts)
{
	MapState &map = *this->getMcv()->map;
	Vector2i spiceTile = findResourceTile(map);
	// A free 3x1 row: left trike, middle (destination), right trike.
	Vector2i left = findFreePassableTile(*this->manager, map, spiceTile, 4, {3, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i middle = left + Vector2i{1, 0};
	Vector2i right = left + Vector2i{2, 0};

	GameObject *trikes[2] = {
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, left), "", "d2k:trik"),
		this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, right), "", "d2k:trik")
	};

	for (auto *t : trikes)
		ASSERT_NE(t, nullptr);

	std::array<Vector2i, 2> start = {trikes[0]->getPosition(), trikes[1]->getPosition()};

	for (auto *t : trikes)
		t->moveTo(tileCenterAbs(map, middle), MOVEREASON_MOVE_COMMAND);

	std::array<bool, 2> started = {false, false};
	bool bothHeldIt = false;

	for (unsigned i = 0; i < 600; i++) {
		this->tick();

		int holding = 0;

		for (size_t j = 0; j < 2; j++) {
			if (trikes[j]->getOccupiedTile() == middle)
				holding++;
			if (trikes[j]->getPosition() != start[j])
				started[j] = true;
		}
		if (holding > 1)
			bothHeldIt = true;
	}

	EXPECT_FALSE(bothHeldIt) << "Both trikes held the middle tile at the same time";
	EXPECT_NE(started[0], started[1]) << "Exactly one trike should have started moving";

	size_t winner = started[0] ? 0 : 1;
	size_t loser = 1 - winner;

	EXPECT_EQ(trikes[winner]->getOccupiedTile(), middle);
	EXPECT_EQ(trikes[winner]->getPosition(), restingPosition(map, *trikes[winner], middle)) << "The winner should have arrived";
	EXPECT_EQ(trikes[loser]->getPosition(), start[loser]) << "The blocked trike should never have moved";
	EXPECT_EQ(trikes[loser]->getOccupiedTile(), loser == 0 ? left : right);
}

namespace {

//! Spawns `factoryId` on free buildable land, then a unit of `unitId` "inside"
//! of it (the way a finished build queue does it: GameManager::spawnObject()
//! with an explicit build queue and no position). The unit appears on the
//! factory's spawn tile and is then ordered to walk to the factory's exit
//! tile. Both tiles are part of the factory's footprint, so the building must
//! not be considered as an obstacle on that trip.
void checkFactoryUnitLeavesTheBuilding(
	GameManager &manager, Player &player, MapState &map, const std::function<void()> &tick,
	const std::string &factoryId, const std::string &queue, const std::string &unitId
) {
	Vector2i spiceTile = findResourceTile(map);
	Vector2i factoryTile = findFreeBuildableTile(manager, map, spiceTile, 6, {3, 3});

	GameObject *factory = manager.spawnObject(player.state.index, map.topLeftCoordToAbs(factoryTile), "", factoryId);

	ASSERT_NE(factory, nullptr);
	ASSERT_TRUE(factory->getSpawnOffset().has_value()) << "The factory should have a spawn point";
	ASSERT_TRUE(factory->getExitOffset().has_value()) << "The factory should have an exit point";

	Vector2i spawnTile = map.absToTileCoord(*factory->getSpawnOffset() + factory->getPosition());
	Vector2i exitTile = map.absToTileCoord(*factory->getExitOffset() + factory->getPosition());

	ASSERT_NE(spawnTile, exitTile);

	GameObject *unit = manager.spawnObject(player.state.index, {-1, -1}, queue, unitId);

	ASSERT_NE(unit, nullptr) << "The factory should have been able to spawn a unit";
	ASSERT_EQ(unit->getOccupiedTile(), spawnTile) << "The unit should appear on the factory's spawn tile";

	Vector2i start = unit->getPosition();
	bool moved = false;

	for (unsigned i = 0; i < 600; i++) {
		tick();
		if (unit->getPosition() != start)
			moved = true;
		if (unit->getOccupiedTile() == exitTile && unit->getPosition() == restingPosition(map, *unit, exitTile))
			break;
	}

	EXPECT_TRUE(moved) << "The unit never left the factory's spawn tile: it thinks it is stuck";
	EXPECT_EQ(unit->getOccupiedTile(), exitTile) << "The unit should have reached the factory's exit tile";
	EXPECT_EQ(unit->getPosition(), restingPosition(map, *unit, exitTile));
}

}

//! A unit coming out of a light factory has to go from the factory's spawn
//! tile to its exit tile, both blocked by the building itself.
TEST_F(GameFixture, LightFactoryUnitMovesFromSpawnPointToExit)
{
	checkFactoryUnitLeavesTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, "d2k:a_lfac", "d2k:light_vehicle", "d2k:trik");
}

//! Same as above for the heavy factory.
TEST_F(GameFixture, HeavyFactoryUnitMovesFromSpawnPointToExit)
{
	checkFactoryUnitLeavesTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, "d2k:a_hfac", "d2k:heavy_vehicle", "d2k:misl");
}

namespace {

//! Spawns two units one after the other out of the same factory. The first
//! one parks on the exit tile (its destination), so when the second one comes
//! out its destination is held by an allied unit: it must ask it to move
//! aside rather than staying stuck on the spawn tile forever.
void checkSecondFactoryUnitCanLeaveTheBuilding(
	GameManager &manager, Player &player, MapState &map, const std::function<void()> &tick,
	const std::string &factoryId, const std::string &queue, const std::string &unitId
) {
	Vector2i spiceTile = findResourceTile(map);
	Vector2i factoryTile = findFreeBuildableTile(manager, map, spiceTile, 6, {3, 3});
	GameObject *factory = manager.spawnObject(player.state.index, map.topLeftCoordToAbs(factoryTile), "", factoryId);

	ASSERT_NE(factory, nullptr);
	ASSERT_TRUE(factory->getSpawnOffset().has_value());
	ASSERT_TRUE(factory->getExitOffset().has_value());

	Vector2i spawnTile = map.absToTileCoord(*factory->getSpawnOffset() + factory->getPosition());
	Vector2i exitTile = map.absToTileCoord(*factory->getExitOffset() + factory->getPosition());
	GameObject *first = manager.spawnObject(player.state.index, {-1, -1}, queue, unitId);

	ASSERT_NE(first, nullptr);
	for (unsigned i = 0; i < 600 && !(first->getOccupiedTile() == exitTile && first->getPosition() == restingPosition(map, *first, exitTile)); i++)
		tick();
	ASSERT_EQ(first->getOccupiedTile(), exitTile) << "Setup issue: the first unit should be parked on the exit tile";

	GameObject *second = manager.spawnObject(player.state.index, {-1, -1}, queue, unitId);

	ASSERT_NE(second, nullptr) << "The spawn tile is free again, a second unit should be spawnable";
	ASSERT_EQ(second->getOccupiedTile(), spawnTile);

	bool shared = false;

	for (unsigned i = 0; i < 1200; i++) {
		tick();
		if (first->getOccupiedTile() == second->getOccupiedTile())
			shared = true;
		if (second->getOccupiedTile() == exitTile && second->getPosition() == restingPosition(map, *second, exitTile))
			break;
	}

	EXPECT_FALSE(shared) << "Two units should never hold the same tile";
	EXPECT_NE(second->getOccupiedTile(), spawnTile) << "The second unit never came out of the factory";
	EXPECT_EQ(second->getOccupiedTile(), exitTile) << "The second unit should have taken the exit tile";
	EXPECT_NE(first->getOccupiedTile(), exitTile) << "The first unit should have moved aside";
	EXPECT_EQ(first->getPosition(), restingPosition(map, *first, first->getOccupiedTile())) << "The first unit should have finished stepping aside";
}

}

//! Two units coming out of the same light factory.
TEST_F(GameFixture, TwoUnitsCanLeaveTheSameLightFactory)
{
	checkSecondFactoryUnitCanLeaveTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, "d2k:a_lfac", "d2k:light_vehicle", "d2k:trik");
}

//! Two units coming out of the same heavy factory.
TEST_F(GameFixture, TwoUnitsCanLeaveTheSameHeavyFactory)
{
	checkSecondFactoryUnitCanLeaveTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, "d2k:a_hfac", "d2k:heavy_vehicle", "d2k:misl");
}
