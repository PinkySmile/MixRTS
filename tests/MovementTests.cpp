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
#include <stdexcept>
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
	this->focusCamera({harvesters[0], harvesters[1], FocusRegion::tiles(map, spiceTile)});
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
	this->focusCamera({trikes[0], trikes[1], FocusRegion::tiles(map, target)});
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
	this->focusCamera({FocusRegion::tiles(map, left, {3, 1})});

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
	const std::function<void(const std::vector<FocusRegion> &)> &focus,
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
	focus({factory, unit, FocusRegion::tiles(map, spawnTile), FocusRegion::tiles(map, exitTile)});

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
	checkFactoryUnitLeavesTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, [this](const std::vector<FocusRegion> &regions) { this->focusCamera(regions); }, "d2k:a_lfac", "d2k:light_vehicle", "d2k:trik");
}

//! Same as above for the heavy factory.
TEST_F(GameFixture, HeavyFactoryUnitMovesFromSpawnPointToExit)
{
	checkFactoryUnitLeavesTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, [this](const std::vector<FocusRegion> &regions) { this->focusCamera(regions); }, "d2k:a_hfac", "d2k:heavy_vehicle", "d2k:misl");
}

namespace {

//! Spawns two units one after the other out of the same factory. The first
//! one parks on the exit tile (its destination), so when the second one comes
//! out its destination is held by an allied unit: it must ask it to move
//! aside rather than staying stuck on the spawn tile forever.
void checkSecondFactoryUnitCanLeaveTheBuilding(
	GameManager &manager, Player &player, MapState &map, const std::function<void()> &tick,
	const std::function<void(const std::vector<FocusRegion> &)> &focus,
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
	focus({factory, first, FocusRegion::tiles(map, spawnTile), FocusRegion::tiles(map, exitTile)});
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
	checkSecondFactoryUnitCanLeaveTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, [this](const std::vector<FocusRegion> &regions) { this->focusCamera(regions); }, "d2k:a_lfac", "d2k:light_vehicle", "d2k:trik");
}

//! Two units coming out of the same heavy factory.
TEST_F(GameFixture, TwoUnitsCanLeaveTheSameHeavyFactory)
{
	checkSecondFactoryUnitCanLeaveTheBuilding(*this->manager, *this->player, *this->getMcv()->map, [this] { this->tick(); }, [this](const std::vector<FocusRegion> &regions) { this->focusCamera(regions); }, "d2k:a_hfac", "d2k:heavy_vehicle", "d2k:misl");
}

namespace {

//! Horizontal lane of 4 tiles, walled in on every side (the mover starts on
//! the first tile and wants to go to the last one). Without walls the units
//! would simply walk around whatever stands in the lane, and nothing would
//! ever have to be pushed or crushed.
using Lane = std::array<Vector2i, 4>;

constexpr unsigned BLOB_SIZE = 5;
constexpr uint32_t LANE_PASS_FLAGS = GRIDMASK_VEHICLES_CAN_PASS | GRIDMASK_INFANTRY_CAN_PASS;

//! Finds a free spot for a lane (and its walls) and builds the walls out of
//! unmovable "test:target" objects.
Lane buildLane(GameManager &manager, Player &wallOwner, MapState &map)
{
	// The region is 6x3 tiles: the lane is its middle row without the first
	// and last tiles, which, like the rest, are walls.
	constexpr int W = 6;
	constexpr int H = 3;
	Vector2u tileSize = map.tileSize;

	for (int ty = 0; ty + H <= static_cast<int>(map.gridSize.y); ty++)
		for (int tx = 0; tx + W <= static_cast<int>(map.gridSize.x); tx++) {
			bool ok = true;

			for (int y = 0; y < H && ok; y++)
				for (int x = 0; x < W && ok; x++) {
					Vector2i tile = {tx + x, ty + y};
					bool isLane = y == 1 && x >= 1 && x < W - 1;
					auto attrs = map.grid[tile.x + tile.y * map.gridSize.x];

					if (!manager.isAreaFree(map.topLeftCoordToAbs(tile), tileSize))
						ok = false;
					else if (isLane && ((attrs & LANE_PASS_FLAGS) != LANE_PASS_FLAGS || GRID_RESOURCE_TYPE(attrs) != 0))
						ok = false;
				}
			if (!ok)
				continue;

			Lane lane;

			for (int y = 0; y < H; y++)
				for (int x = 0; x < W; x++) {
					Vector2i tile = {tx + x, ty + y};

					if (y == 1 && x >= 1 && x < W - 1) {
						lane[x - 1] = tile;
						continue;
					}
					if (!manager.spawnObject(wallOwner.state.index, map.topLeftCoordToAbs(tile), "", "test:target"))
						throw std::runtime_error("Cannot spawn wall");
				}
			return lane;
		}
	throw std::runtime_error("No free space for a lane found");
}

}

//! Common setup for the tests on units blocked by a blob of 5 stackable units
//! (infantry sharing the second tile of a lane).
class BlobMovementTests : public GameFixture {
protected:
	Player *enemy = nullptr;
	Lane lane;
	std::array<GameObject *, BLOB_SIZE> blob;

	void SetUp() override
	{
		GameFixture::SetUp();
		// Harkonnen, on the second starting area, in another team.
		this->manager->addPlayer(1, HSLColor{0, 800, 750}, 1, 1);
		// GameManager::getPlayers() can reallocate when a player is added.
		this->player = &this->manager->getPlayers()[0];
		this->enemy = &this->manager->getPlayers()[1];
	}

	MapState &map()
	{
		return *this->getMcv()->map;
	}

	//! Builds the lane and puts the blob (owned by the player) on its second tile.
	void setupBlob()
	{
		this->lane = buildLane(*this->manager, *this->player, this->map());
		for (auto &b : this->blob) {
			b = this->manager->spawnObject(this->player->state.index, tileCenterAbs(this->map(), this->lane[1]), "", "d2k:engi");
			ASSERT_NE(b, nullptr);
			ASSERT_EQ(b->getOccupiedTile(), this->lane[1]);
			ASSERT_TRUE(b->isStackable());
			ASSERT_TRUE(b->isCrushable());
		}
		// The lane and the walls around it (see buildLane()).
		this->focusCamera({FocusRegion::tiles(this->map(), this->lane[0] - Vector2i{1, 1}, {6, 3})});
	}

	GameObject *spawnOnLane(Player &owner, const std::string &id)
	{
		GameObject *obj = this->manager->spawnObject(owner.state.index, tileCenterAbs(this->map(), this->lane[0]), "", id);

		if (obj) {
			// Don't let the spawn orders do their own thing.
			obj->clearOrders();
			obj->moveTo(tileCenterAbs(this->map(), this->lane[3]), MOVEREASON_MOVE_COMMAND);
		}
		return obj;
	}

	//! An allied unit crosses the blob: everyone in the blob must get out of the way at once.
	void checkAlliedMoverPushesWholeBlob(const std::string &moverId)
	{
		this->setupBlob();
		if (this->HasFatalFailure())
			return;

		GameObject *mover = this->spawnOnLane(*this->player, moverId);

		ASSERT_NE(mover, nullptr);
		ASSERT_EQ(mover->getOccupiedTile(), this->lane[0]);

		std::array<int, BLOB_SIZE> leftAt;
		int enteredAt = -1;

		leftAt.fill(-1);
		for (int i = 0; i < 1500 && enteredAt < 0; i++) {
			this->tick();
			for (size_t j = 0; j < BLOB_SIZE; j++)
				if (leftAt[j] < 0 && this->blob[j]->getOccupiedTile() != this->lane[1])
					leftAt[j] = i;
			if (mover->getOccupiedTile() == this->lane[1])
				enteredAt = i;
		}

		for (size_t j = 0; j < BLOB_SIZE; j++) {
			EXPECT_GE(leftAt[j], 0) << "Unit " << j << " of the blob never moved out of the way";
			EXPECT_GT(this->blob[j]->getHealth(), 0) << "Allied unit " << j << " should never have been hurt";
		}
		ASSERT_GE(enteredAt, 0) << "The unit never made it through the blob";

		int first = *std::ranges::min_element(leftAt);
		int last = *std::ranges::max_element(leftAt);

		// Asking them one after the other takes a full blocked delay (60 ticks) per unit.
		EXPECT_LE(last - first, 10) << "The blob should move out of the way all at once (first one left at tick " << first << ", last one at tick " << last << ")";
		EXPECT_GE(enteredAt, last) << "The unit went in before the whole blob was out of the way";
	}
};

//! A unit that can't stack (a trike) is blocked by 5 allied stackable units:
//! all 5 have to move out of the way at the same time.
TEST_F(BlobMovementTests, NonStackableAllyPushesWholeBlobAtOnce)
{
	this->checkAlliedMoverPushesWholeBlob("d2k:trik");
}

//! Same, but the unit is stackable too. It has to ask all 5 to move as well
//! instead of taking the place of the first one to leave.
TEST_F(BlobMovementTests, StackableAllyPushesWholeBlobAtOnce)
{
	this->checkAlliedMoverPushesWholeBlob("d2k:engi");
}

//! An enemy harvester (that can crush) goes through 5 stackable units: all 5
//! have to be crushed at the same time, and not just the first one before
//! driving over the other four.
TEST_F(BlobMovementTests, EnemyCrusherCrushesWholeBlobAtOnce)
{
	this->setupBlob();
	if (this->HasFatalFailure())
		return;

	GameObject *crusher = this->spawnOnLane(*this->enemy, "d2k:harv");

	ASSERT_NE(crusher, nullptr);
	ASSERT_EQ(crusher->getOccupiedTile(), this->lane[0]);

	std::array<int, BLOB_SIZE> crushedAt;
	int enteredAt = -1;

	crushedAt.fill(-1);
	for (int i = 0; i < 1500 && enteredAt < 0; i++) {
		this->tick();
		for (size_t j = 0; j < BLOB_SIZE; j++)
			// Crushed units get removed some ticks after: never look at one again once crushed.
			if (crushedAt[j] < 0 && this->blob[j]->getHealth() == 0)
				crushedAt[j] = i;
		if (crusher->getOccupiedTile() == this->lane[1])
			enteredAt = i;
	}

	for (size_t j = 0; j < BLOB_SIZE; j++)
		EXPECT_GE(crushedAt[j], 0) << "Unit " << j << " of the blob was never crushed";
	ASSERT_GE(enteredAt, 0) << "The crusher never went through the blob";

	int first = *std::ranges::min_element(crushedAt);
	int last = *std::ranges::max_element(crushedAt);

	EXPECT_EQ(first, last) << "The whole blob should be crushed at once (first one at tick " << first << ", last one at tick " << last << ")";
	EXPECT_LE(last, enteredAt) << "The crusher went in before the whole blob was crushed";
}

//! A crush weapon hits only the crusher, for its full damage (distance 0, so the
//! warhead radius does not reduce it). weapon.sardaukar_crush deals 300 with the
//! at_mis warhead, and a harvester's armor takes half of that: 150.
TEST_F(GameFixture, SardaukarDamagesTheUnitThatCrushesIt)
{
	MapState &map = *this->getMcv()->map;
	Vector2i tile = findFreePassableTile(
		*this->manager, map, map.absToTileCoord(this->getMcv()->getPosition()),
		8, {2, 1}, GRIDMASK_VEHICLES_CAN_PASS, 2
	);
	Vector2f base = tileCenterAbs(map, tile).to<float>();
	GameObject *sardaukar = spawnAt(*this->manager, this->player->state.index, "d2k:sard", base);
	GameObject *harvester = spawnAt(*this->manager, this->player->state.index, "d2k:harv", base + Vector2f{static_cast<float>(map.tileSize.x), 0});

	ASSERT_NE(sardaukar, nullptr);
	ASSERT_NE(harvester, nullptr);
	harvester->clearOrders();
	this->focusCamera({sardaukar, harvester});

	sardaukar->crushedBy(harvester);

	EXPECT_EQ(sardaukar->getHealth(), 0u);
	EXPECT_EQ(harvester->getHealth(), harvester->getMaxHealth() - 150);
}

//! A death weapon detonates on the dying unit. weapon.grenadier_death deals 150
//! with the he warhead (radius 64); a trike's wood armor takes 0.7 of that.
//! Point blank, the radius does not reduce it: ceil(150 * 0.7) = 105.
TEST_F(GameFixture, GrenadierExplodesWhenItDies)
{
	MapState &map = *this->getMcv()->map;
	Vector2i tile = findFreePassableTile(
		*this->manager, map, map.absToTileCoord(this->getMcv()->getPosition()),
		8, {3, 1}, GRIDMASK_VEHICLES_CAN_PASS, 2
	);
	Vector2f base = tileCenterAbs(map, tile).to<float>();
	GameObject *grenadier = spawnAt(*this->manager, this->player->state.index, "d2k:gren", base);
	GameObject *bystander = spawnAt(*this->manager, this->player->state.index, "d2k:trik", base + Vector2f{map.tileSize.x * 2.f, 0});

	ASSERT_NE(grenadier, nullptr);
	ASSERT_NE(bystander, nullptr);
	// Sit on the grenadier, so this measures the blast itself and not a range falloff.
	placeAt(*bystander, grenadier->getHitboxCenter({}));
	this->focusCamera({grenadier, bystander});

	grenadier->damage(grenadier->getHealth(), DMGTYPE_BULLET);

	EXPECT_EQ(grenadier->getHealth(), 0u);
	EXPECT_EQ(bystander->getHealth(), bystander->getMaxHealth() - 105);
}

//! Being crushed is not a normal death: the grenadier's death weapon stays quiet,
//! and with no crush weapon the harvester is not hurt either.
TEST_F(GameFixture, CrushedGrenadierDoesNotExplode)
{
	MapState &map = *this->getMcv()->map;
	Vector2i tile = findFreePassableTile(
		*this->manager, map, map.absToTileCoord(this->getMcv()->getPosition()),
		8, {3, 1}, GRIDMASK_VEHICLES_CAN_PASS, 2
	);
	Vector2f base = tileCenterAbs(map, tile).to<float>();
	float tileWidth = static_cast<float>(map.tileSize.x);
	GameObject *grenadier = spawnAt(*this->manager, this->player->state.index, "d2k:gren", base);
	GameObject *bystander = spawnAt(*this->manager, this->player->state.index, "d2k:trik", base + Vector2f{tileWidth, 0});
	GameObject *harvester = spawnAt(*this->manager, this->player->state.index, "d2k:harv", base + Vector2f{tileWidth * 2, 0});

	ASSERT_NE(grenadier, nullptr);
	ASSERT_NE(bystander, nullptr);
	ASSERT_NE(harvester, nullptr);
	harvester->clearOrders();
	// Close enough that the death weapon (radius 64) would hit if it went off.
	placeAt(*bystander, grenadier->getHitboxCenter({}));
	this->focusCamera({grenadier, bystander, harvester});

	grenadier->crushedBy(harvester);

	EXPECT_EQ(grenadier->getHealth(), 0u);
	EXPECT_EQ(bystander->getHealth(), bystander->getMaxHealth());
	EXPECT_EQ(harvester->getHealth(), harvester->getMaxHealth());
}

//! The sardaukar's crush weapon kills the harvester while it is resolving the
//! collision. The harvester must not reserve the tile it was stepping into
//! (that would leave a dead unit on the pathfinding grid), and once its death
//! animation is over another unit has to be able to take the lane.
TEST_F(BlobMovementTests, CrusherKilledByCrushWeaponLeavesTheGridUsable)
{
	this->lane = buildLane(*this->manager, *this->player, this->map());
	for (auto &b : this->blob) {
		b = this->manager->spawnObject(this->player->state.index, tileCenterAbs(this->map(), this->lane[1]), "", "d2k:sard");
		ASSERT_NE(b, nullptr);
		ASSERT_TRUE(b->isCrushable());
		b->clearOrders();
		// Busy forever, so they don't shoot the harvester before it crushes them.
		b->execCb([] { return false; });
	}
	this->focusCamera({FocusRegion::tiles(this->map(), this->lane[0] - Vector2i{1, 1}, {6, 3})});

	GameObject *crusher = this->spawnOnLane(*this->enemy, "d2k:harv");

	ASSERT_NE(crusher, nullptr);
	// One sardaukar deals 150 to a harvester. 100 dies on the first crush,
	// before a second shot could matter, and the other four still get crushed.
	crusher->setHealth(100);

	int diedAt = -1;

	for (int i = 0; i < 1500 && diedAt < 0; i++) {
		this->tick();
		if (crusher->getHealth() == 0)
			diedAt = i;
	}
	ASSERT_GE(diedAt, 0) << "The harvester was never killed";
	EXPECT_FALSE(crusher->isDisposable()) << "It should still be playing its death animation, not already removed";
	for (auto *b : this->blob)
		EXPECT_EQ(b->getHealth(), 0u);

	size_t enemyTeam = this->enemy->state.team;
	PathFindingGrid &grid = this->map().pathFindingGrid;

	for (Vector2i tile : this->lane) {
		unsigned index = tile.x + tile.y * grid.gridSize.x;

		EXPECT_EQ(grid.usedSpaces[enemyTeam][index], 0) << "Dead harvester still occupies tile " << tile.x << "," << tile.y;
		EXPECT_EQ(grid.usedSpaces[this->player->state.team][index], 0) << "Crushed infantry still occupy tile " << tile.x << "," << tile.y;
	}

	// Stay 30 ticks, then the corpse is removed. Don't touch it after this.
	this->tick(80);

	GameObject *follower = this->spawnOnLane(*this->player, "d2k:trik");

	ASSERT_NE(follower, nullptr);

	int arrived = -1;

	for (int i = 0; i < 1500 && arrived < 0; i++) {
		this->tick();
		if (follower->getOccupiedTile() == this->lane[3])
			arrived = i;
	}
	EXPECT_GE(arrived, 0) << "The lane stayed blocked after the harvester died";
}
