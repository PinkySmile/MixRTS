//
// Created for the MixRTS test target.
//
// Small helpers shared by several test files to place units/buildings on the
// test map without hardcoding map-specific coordinates.
//

#ifndef MIXRTS_TESTS_TESTHELPERS_HPP
#define MIXRTS_TESTS_TESTHELPERS_HPP


#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "Mock/MockedUI.hpp"
#include "Data/GameManager.hpp"
#include "Data/Map.hpp"
#include "Data/GridMask.hpp"
#include "Data/Player.hpp"

inline Vector2i tileCenterAbs(const MapState &map, Vector2i tile)
{
	return map.topLeftCoordToAbs(tile) + Vector2i(map.tileSize.to<int>()) / 2;
}

//! Scans the whole map for a tile carrying spice (or any other harvestable
//! resource) and returns its tile coordinates. Used instead of hardcoding
//! map-specific coordinates so these tests keep working if the test map ever
//! changes.
inline Vector2i findResourceTile(const MapState &map)
{
	for (unsigned y = 0; y < map.gridSize.y; y++)
		for (unsigned x = 0; x < map.gridSize.x; x++)
			if (GRID_RESOURCE_TYPE(map.grid[x + y * map.gridSize.x]) != 0)
				return {static_cast<int>(x), static_cast<int>(y)};
	throw std::runtime_error("No resource tile found on the test map");
}

//! Searches outward (in growing square rings) from originTile for a tile
//! whose footprintTiles x footprintTiles area is fully passable (per
//! passFlags), isn't sitting on a resource, and is currently unoccupied.
//! Used to place movers ("far away", "next to the field", etc.) without
//! hardcoding coordinates specific to the test map. `borderMargin` rejects
//! candidates too close to the map's edge, where terrain tends to be a
//! narrow, easily fully-enclosed dead end (e.g. good for "put a unit
//! somewhere passable" but bad for "and it should be able to walk all the
//! way to some far away tile on its own").
inline Vector2i findFreePassableTile(
	GameManager &manager, const MapState &map, Vector2i originTile,
	unsigned minRingTiles, Vector2u footprintTiles, uint32_t passFlags,
	unsigned borderMargin = 0
) {
	unsigned maxRing = std::max(map.gridSize.x, map.gridSize.y);

	for (unsigned r = minRingTiles; r < maxRing; r++) {
		for (int dy = -static_cast<int>(r); dy <= static_cast<int>(r); dy++)
			for (int dx = -static_cast<int>(r); dx <= static_cast<int>(r); dx++) {
				if (std::max(std::abs(dx), std::abs(dy)) != static_cast<int>(r))
					continue;

				Vector2i topLeft = originTile + Vector2i{dx, dy};
				bool ok = true;

				for (unsigned ty = 0; ty < footprintTiles.y && ok; ty++)
					for (unsigned tx = 0; tx < footprintTiles.x && ok; tx++) {
						Vector2i t = topLeft + Vector2i{static_cast<int>(tx), static_cast<int>(ty)};

						if (
							t.x < static_cast<int>(borderMargin) || t.y < static_cast<int>(borderMargin) ||
							static_cast<unsigned>(t.x) + borderMargin >= map.gridSize.x ||
							static_cast<unsigned>(t.y) + borderMargin >= map.gridSize.y
						) {
							ok = false;
							break;
						}
						auto v = map.grid[t.x + t.y * map.gridSize.x];
						if (!(v & passFlags) || GRID_RESOURCE_TYPE(v) != 0) {
							ok = false;
							break;
						}
					}
				if (!ok)
					continue;

				Vector2i absPos = map.topLeftCoordToAbs(topLeft);
				Vector2u absSize = {footprintTiles.x * map.tileSize.x, footprintTiles.y * map.tileSize.y};

				if (!manager.isAreaFree(absPos, absSize))
					continue;
				return topLeft;
			}
	}
	throw std::runtime_error("No free passable tile found");
}

//! Same idea as findFreePassableTile(), but for a fully buildable (not just
//! passable) footprint, used to place buildings (refineries, silos, ...)
//! without hardcoding map-specific coordinates.
inline Vector2i findFreeBuildableTile(GameManager &manager, const MapState &map, Vector2i originTile, unsigned minRingTiles, Vector2u footprintTiles)
{
	return findFreePassableTile(manager, map, originTile, minRingTiles, footprintTiles, GRIDMASK_CAN_BUILD);
}

inline MockedUI *installMockedUI(Player &player)
{
	auto &uis = player.getUIs();
	auto *mockedUi = new MockedUI(uis[player.state.factionIndex].release());

	uis[player.state.factionIndex].reset(mockedUi);
	return mockedUi;
}

//! Places `obj` so that its *center* - not its top-left corner, which is
//! what GameObject::setPosition() actually takes - ends up exactly at
//! `center`. Every weapon-range/hitbox computation in the engine
//! (GameObject::getHitboxCenter(), the myCenter/opCenter pair in
//! GameObject::_updateAttack(), ...) is center-based, so this keeps test
//! geometry exact instead of being off by half the object's size.
inline void placeAt(GameObject &obj, Vector2f center)
{
	obj.setPosition(center - Vector2f(obj.getSize().to<float>()) / 2.f);
}

//! Spawns `id` for `owner` and immediately centers it on `center` (see
//! placeAt()), overriding whatever tile-snapped position spawnObject()
//! itself picked.
inline GameObject *spawnAt(GameManager &manager, size_t owner, const std::string &id, Vector2f center)
{
	GameObject *obj = manager.spawnObject(owner, Vector2i(center.to<int>()), "", id);

	if (obj)
		placeAt(*obj, center);
	return obj;
}


#endif //MIXRTS_TESTS_TESTHELPERS_HPP
