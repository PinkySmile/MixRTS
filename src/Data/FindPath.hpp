//
// Created by PinkySmile on 26/04/2026.
//

#ifndef MIXRTS_FINDPATH_HPP
#define MIXRTS_FINDPATH_HPP


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
};

constexpr int PATHFINDING_FREE_NB_POS = 5;
constexpr int PATHFINDING_FREE_BITS = 10;
constexpr int PATHFINDING_FREE_OFFSET = ((sizeof(pfGridType) * 8) - PATHFINDING_FREE_BITS);
constexpr int PATHFINDING_FREE_MAX = (1 << (PATHFINDING_FREE_BITS - PATHFINDING_FREE_NB_POS)) - 1;
constexpr int PATHFINDING_METADATA_MASK = (1 << PATHFINDING_FREE_OFFSET) - 1;
constexpr int PFGRIDMASK_STATIC_BIT = 1 << 10;

enum PFFreeMask {
	PFFREEMASK_TOP_LEFT     = 1 << 0,
	PFFREEMASK_TOP_RIGHT    = 1 << 1,
	PFFREEMASK_BOTTOM_LEFT  = 1 << 2,
	PFFREEMASK_BOTTOM_RIGHT = 1 << 3,
	PFFREEMASK_MIDDLE       = 1 << 4
};

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
inline unsigned char PFGRIDMASK_FREE(pfGridType tile)
{
	return tile >> PATHFINDING_FREE_OFFSET;
}
inline unsigned char PFGRIDMASK_FREE_NB(pfGridType tile)
{
	return tile >> (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS);
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
	if (tile & GRIDMASK_VEHICULES_CAN_PASS)
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
	assert_exp(mask < PFFREEMASK_MIDDLE);
	if (mask == 0) {
		assert_exp(PFGRIDMASK_FREE_NB(g, x, y) < PATHFINDING_FREE_MAX);
		g.grid[x + y * g.gridSize.x] += (1 << (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS));
	} else {
		assert_exp((g.grid[x + y * g.gridSize.x] & mask) == 0);
		g.grid[x + y * g.gridSize.x] |= mask;
	}

	sf::Color color = __getPFDebugColor(g.grid[x + y * g.gridSize.x]);

	if (color != g.gridImage.getPixel({x, y})) {
		g.gridImage.setPixel({x, y}, color);
		g.textureOutOfSync = true;
	}
}
inline void PFGRIDMASK_DEL_UNIT(PathFindingGrid &g, unsigned x, unsigned y, pfGridType mask)
{
	assert_exp(mask < PFFREEMASK_MIDDLE);
	if (mask == 0) {
		assert_exp(PFGRIDMASK_FREE_NB(g, x, y) != 0);
		g.grid[x + y * g.gridSize.x] -= (1 << (PATHFINDING_FREE_OFFSET + PATHFINDING_FREE_NB_POS));
	} else {
		assert_exp((g.grid[x + y * g.gridSize.x] & mask) == mask);
		g.grid[x + y * g.gridSize.x] &= mask;
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
	void computePath(
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
	void computePath(
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
