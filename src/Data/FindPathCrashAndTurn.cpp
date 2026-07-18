//
// Created by PinkySmile on 26/04/2026.
//

#include "FindPath.hpp"

// Algorithm details:
// 1. Find all the obstacles in the straight line
// 2. For each obstructed paths, follow both edges of the obstacles until one of the exit points is reached
// 3. Only keep the shortest path found by merging all the small portions
// 4. Finally, trim the final path
void CrashAndTurn::computePath(
	const PathFindingGrid &grid,
	Vector2i start,
	Vector2i dest,
	Vector2u step,
	IsObstructedCb isObstructed,
	GetTileCostCb getCost,
	void *context,
	std::vector<Vector2i> &buffer
)
{
	buffer.clear();
	buffer.push_back(dest);
}
