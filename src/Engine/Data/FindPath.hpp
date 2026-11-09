//
// Created by PinkySmile on 26/04/2026.
//

#ifndef MIXRTS_FINDPATH_HPP
#define MIXRTS_FINDPATH_HPP


#include <bit>
#include <vector>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "Vector.hpp"
#include "Color.hpp"
#include "Resources/Assert.hpp"
#include "GridMask.hpp"

typedef uint32_t pfGridType;

struct PathFindingGrid {
	bool textureOutOfSync;
	sf::Image gridImage;
	sf::Texture gridImageTexture;
	std::vector<pfGridType> grid;
	Vector2u gridSize;
	std::vector<std::vector<uint8_t>> usedSpaces;
};

constexpr int PATHFINDING_FREE_NB_POS = 5;
constexpr int PATHFINDING_FREE_BITS = 8;
constexpr int PATHFINDING_FREE_OFFSET = 8;
constexpr int PATHFINDING_FREE_MAX = (1 << (PATHFINDING_FREE_BITS - PATHFINDING_FREE_NB_POS)) - 1;

constexpr int PATHFINDING_METADATA_MASK = 0xFF;
constexpr int PFGRIDMASK_STATIC_OFFSET = PATHFINDING_FREE_BITS + PATHFINDING_FREE_OFFSET;
constexpr int PFGRIDMASK_STATIC_BIT = 1 << PFGRIDMASK_STATIC_OFFSET;
constexpr int PFGRIDMASK_CRUSHABLE_OFFSET = PFGRIDMASK_STATIC_OFFSET + 1;
constexpr int PFGRIDMASK_NON_CRUSHABLE_BITS = 3;
constexpr int PFGRIDMASK_NON_CRUSHABLE_BIT = 1 << PFGRIDMASK_CRUSHABLE_OFFSET;

enum PFFreeMask : unsigned char {
	PFFREEMASK_NONE         = 0,
	PFFREEMASK_TOP_LEFT     = 1 << 0,
	PFFREEMASK_TOP_RIGHT    = 1 << 1,
	PFFREEMASK_BOTTOM_LEFT  = 1 << 2,
	PFFREEMASK_BOTTOM_RIGHT = 1 << 3,
	PFFREEMASK_MIDDLE       = 1 << 4
};

constexpr unsigned PATHFINDING_STACK_LIMIT = 5;
constexpr pfGridType PFGRIDMASK_STACK_MASK =
	PFFREEMASK_TOP_LEFT | PFFREEMASK_TOP_RIGHT |
	PFFREEMASK_BOTTOM_LEFT | PFFREEMASK_BOTTOM_RIGHT |
	PFFREEMASK_MIDDLE;

inline unsigned PFGRIDMASK_STACK_COUNT(pfGridType tile)
{
	return std::popcount((tile >> PATHFINDING_FREE_OFFSET) & PFGRIDMASK_STACK_MASK);
}
inline unsigned PFGRIDMASK_STACK_COUNT(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	return PFGRIDMASK_STACK_COUNT(g.grid[x + y * g.gridSize.x]);
}
inline PFFreeMask PFGRIDMASK_FIND_FREE_SLOT(pfGridType tile)
{
	static constexpr PFFreeMask slots[PATHFINDING_STACK_LIMIT] = {
		PFFREEMASK_TOP_LEFT, PFFREEMASK_TOP_RIGHT,
		PFFREEMASK_BOTTOM_LEFT, PFFREEMASK_BOTTOM_RIGHT,
		PFFREEMASK_MIDDLE
	};
	pfGridType free = tile >> PATHFINDING_FREE_OFFSET;

	for (auto slot : slots)
		if (!(free & slot))
			return slot;
	return PFFREEMASK_NONE;
}
inline PFFreeMask PFGRIDMASK_FIND_FREE_SLOT(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	return PFGRIDMASK_FIND_FREE_SLOT(g.grid[x + y * g.gridSize.x]);
}
inline bool PFGRIDMASK_IS_SLOT_FREE(pfGridType tile, PFFreeMask mask)
{
	return ((tile >> PATHFINDING_FREE_OFFSET) & mask) == 0;
}
inline bool PFGRIDMASK_IS_SLOT_FREE(PathFindingGrid &g, unsigned x, unsigned y, PFFreeMask mask)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	return PFGRIDMASK_IS_SLOT_FREE(g.grid[x + y * g.gridSize.x], mask);
}

inline bool PFGRIDMASK_IS_STATIC(pfGridType tile)
{
	return (tile & PFGRIDMASK_STATIC_BIT) != 0;
}
inline void PFGRIDMASK_ADD_STATIC(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	g.grid[x + y * g.gridSize.x] |= PFGRIDMASK_STATIC_BIT;
}
inline void PFGRIDMASK_DEL_STATIC(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	g.grid[x + y * g.gridSize.x] &= ~PFGRIDMASK_STATIC_BIT;
}

inline uint8_t PFGRIDMASK_GET_NON_CRUSHABLE(pfGridType tile)
{
	return ((tile >> PFGRIDMASK_CRUSHABLE_OFFSET) & ((1 << PFGRIDMASK_NON_CRUSHABLE_BITS) - 1));
}
inline void PFGRIDMASK_ADD_NON_CRUSHABLE(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	assert_exp(PFGRIDMASK_GET_NON_CRUSHABLE(g.grid[x + y * g.gridSize.x]) < ((1 << PFGRIDMASK_NON_CRUSHABLE_BITS) - 1));
	g.grid[x + y * g.gridSize.x] += PFGRIDMASK_NON_CRUSHABLE_BIT;
}
inline void PFGRIDMASK_DEL_NON_CRUSHABLE(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	assert_exp(PFGRIDMASK_GET_NON_CRUSHABLE(g.grid[x + y * g.gridSize.x]) > 0);
	g.grid[x + y * g.gridSize.x] -= PFGRIDMASK_NON_CRUSHABLE_BIT;
}

inline unsigned char PFGRIDMASK_FREE(pfGridType tile)
{
	return (tile >> PATHFINDING_FREE_OFFSET) & ((1 << PATHFINDING_FREE_NB_POS) - 1);
}
inline unsigned char PFGRIDMASK_FREE_NB(pfGridType tile)
{
	return (tile >> (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS)) & PATHFINDING_FREE_MAX;
}
inline unsigned char PFGRIDMASK_FREE_NB(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	return PFGRIDMASK_FREE_NB(g.grid[x + y * g.gridSize.x]);
}

inline Color __getPFDebugColor(pfGridType tile)
{
	Color color;
	unsigned char pfFree = PFGRIDMASK_FREE(tile);
	unsigned char pfFreeNb = PFGRIDMASK_FREE_NB(tile);

	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 128;
	if (pfFreeNb >= 2) {
		color.a = 255;
		return color;
	}
	if (pfFree == 0b11111 || pfFreeNb) {
		color.r = 255;
		return color;
	}
	if (pfFree != 0) {
		color.r = 255;
		color.g = 128;
		return color;
	}
	if ((tile & PATHFINDING_METADATA_MASK) == 0) {
		color.r = 255;
		return color;
	}
	if (tile & GRIDMASK_SHIP_CAN_PASS)
		color.b = 255;
	if (tile & GRIDMASK_WORM_CAN_PASS)
		color.r = 255;
	if (tile & GRIDMASK_VEHICLES_CAN_PASS)
		color.g = 255;
	if (tile & GRIDMASK_INFANTRY_CAN_PASS) {
		color.r /= 2;
		color.g /= 2;
		color.b /= 2;
	}
	return color;
}

inline void PFGRIDMASK_ADD_UNIT(PathFindingGrid &g, unsigned x, unsigned y, pfGridType mask)
{
	assert_exp(mask <= PFFREEMASK_MIDDLE);
	if (mask == 0) {
		assert_exp(PFGRIDMASK_FREE_NB(g, x, y) < PATHFINDING_FREE_MAX);
		g.grid[x + y * g.gridSize.x] += (1 << (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS));
	} else {
		// mask is a local slot id (bits 0-4); shift it into the tile's
		// "free" byte so it doesn't collide with the terrain pass-flag bits.
		pfGridType bit = mask << PATHFINDING_FREE_OFFSET;

		assert_exp((g.grid[x + y * g.gridSize.x] & bit) == 0);
		g.grid[x + y * g.gridSize.x] |= bit;
	}

	sf::Color color = __getPFDebugColor(g.grid[x + y * g.gridSize.x]);

	if (color != g.gridImage.getPixel({x, y})) {
		g.gridImage.setPixel({x, y}, color);
		g.textureOutOfSync = true;
	}
}
inline void PFGRIDMASK_DEL_UNIT(PathFindingGrid &g, unsigned x, unsigned y, pfGridType mask)
{
	assert_exp(mask <= PFFREEMASK_MIDDLE);
	if (mask == 0) {
		assert_exp(PFGRIDMASK_FREE_NB(g, x, y) != 0);
		g.grid[x + y * g.gridSize.x] -= (1 << (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS));
	} else {
		pfGridType bit = mask << PATHFINDING_FREE_OFFSET;

		assert_exp((g.grid[x + y * g.gridSize.x] & bit) == bit);
		g.grid[x + y * g.gridSize.x] &= ~bit;
	}

	sf::Color color = __getPFDebugColor(g.grid[x + y * g.gridSize.x]);

	if (color != g.gridImage.getPixel({x, y})) {
		g.gridImage.setPixel({x, y}, color);
		g.textureOutOfSync = true;
	}
}

typedef bool (*IsObstructedCb)(Vector2i pos, void *context);
typedef float (*GetTileCostCb)(Vector2i pos, void *context);

namespace AStar
{
	MYDLL_API void computePath(
		const PathFindingGrid &grid,
		Vector2i currentPos,
		Vector2i dest,
		Vector2u step,
		IsObstructedCb isObstructed,
		GetTileCostCb getCost,
		void *context,
		std::vector<Vector2i> &buffer
	);
	sf::Image getDebugImage();
}
namespace CrashAndTurn
{
	MYDLL_API void computePath(
		const PathFindingGrid &grid,
		Vector2i currentPos,
		Vector2i dest,
		Vector2u step,
		IsObstructedCb isObstructed,
		GetTileCostCb getCost,
		void *context,
		std::vector<Vector2i> &buffer
	);
	sf::Image getDebugImage();
}


#endif //MIXRTS_FINDPATH_HPP
