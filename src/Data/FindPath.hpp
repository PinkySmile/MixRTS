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

#define PATHFINDINGGRID_TYPE uint16_t
struct PathFindingGrid {
	bool textureOutOfSync;
	sf::Image gridImage;
	sf::Texture gridImageTexture;
	std::vector<PATHFINDINGGRID_TYPE> grid;
	Vector2u gridSize;
};

#define NB_PATHFINDING_BITS 6
#define NB_PATHFINDING_OFFSET ((sizeof(PATHFINDINGGRID_TYPE) * 8) - NB_PATHFINDING_BITS)
#define NB_PATHFINDING_MAX ((1 << NB_PATHFINDING_BITS) - 1)


inline unsigned char PFGRIDMASK_NB_UNITS(PATHFINDINGGRID_TYPE tile)
{
	return tile >> NB_PATHFINDING_OFFSET;
}
inline Color __getPFDebugColor(PATHFINDINGGRID_TYPE tile)
{
	Color color;

	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 128;
	if (PFGRIDMASK_NB_UNITS(tile) > 5) {
		color.a = 255;
		return color;
	}
	if (PFGRIDMASK_NB_UNITS(tile) == 5) {
		color.r = 255;
		return color;
	}
	if (PFGRIDMASK_NB_UNITS(tile) != 0) {
		color.r = 255;
		color.g = 128;
		return color;
	}
	if ((tile & ((1 << NB_PATHFINDING_OFFSET) - 1)) == 0) {
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

inline unsigned char PFGRIDMASK_NB_UNITS(PathFindingGrid &g, unsigned x, unsigned y)
{
	assert_exp(x < g.gridSize.x && y < g.gridSize.y);
	return PFGRIDMASK_NB_UNITS(g.grid[x + y * g.gridSize.x]);
}
inline void PFGRIDMASK_ADD_UNIT(PathFindingGrid &g, unsigned x, unsigned y, unsigned count)
{
	assert_exp(PFGRIDMASK_NB_UNITS(g, x, y) + count <= NB_PATHFINDING_MAX);
	g.grid[x + y * g.gridSize.x] += (count << NB_PATHFINDING_OFFSET);

	sf::Color color = __getPFDebugColor(g.grid[x + y * g.gridSize.x]);

	if (color != g.gridImage.getPixel({x, y})) {
		g.gridImage.setPixel({x, y}, color);
		g.textureOutOfSync = true;
	}
}
inline void PFGRIDMASK_DEL_UNIT(PathFindingGrid &g, unsigned x, unsigned y, unsigned count)
{
	assert_exp(PFGRIDMASK_NB_UNITS(g, x, y) >= count);
	g.grid[x + y * g.gridSize.x] -= (count << NB_PATHFINDING_OFFSET);

	sf::Color color = __getPFDebugColor(g.grid[x + y * g.gridSize.x]);

	if (color != g.gridImage.getPixel({x, y})) {
		g.gridImage.setPixel({x, y}, color);
		g.textureOutOfSync = true;
	}
}

namespace AStar
{
	void computePath(
		const PathFindingGrid &grid,
		Vector2i currentPos,
		Vector2i dest,
		Vector2u hitBoxSize,
		Vector2u step,
		uint16_t mask,
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
		Vector2u hitBoxSize,
		Vector2u step,
		uint16_t mask,
		std::vector<Vector2i> &buffer
	);
	sf::Image getDebugImage();
}


#endif //MIXRTS_FINDPATH_HPP
