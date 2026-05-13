//
// Created by PinkySmile on 26/04/2026.
//

#include <set>
#include <map>
#include <queue>
#include <functional>
#include "FindPath.hpp"

typedef std::pair<Vector2i, unsigned> Node;
#define GET_OPTIONAL(m, f) (m.contains(f) ? m.at(f) : std::numeric_limits<float>::infinity())

static bool compare(const Node &a, const Node &b)
{
	return a.second > b.second;
}

static Vector2u __lastGridSize;
static Vector2u __lastStepSize;
static std::set<Vector2i> closedSet;
static std::priority_queue<Node, std::vector<Node>, decltype(&compare)> openSet{compare};

static void reconstructPath(const std::map<Vector2i, Vector2i> &cameFrom, Vector2i current, std::vector<Vector2i> &totalPath)
{
	Vector2i dir = {0, 0};
	auto old = current;

	totalPath.push_back(current);
	while (cameFrom.contains(current)) {
		current = cameFrom.at(current);
		if (current - old == dir)
			totalPath.pop_back();
		old = current;
		totalPath.push_back(current);
	}
	totalPath.pop_back();
}

// A* finds a path from start to goal.
// h is the heuristic function. h(n) estimates the cost to reach goal from node n.
static bool aStar(Vector2i start, Vector2i goal, const std::function<bool(Vector2i)> &isObstructed, Vector2u gridSize, Vector2i step, std::vector<Vector2i> &totalPath)
{
	__lastGridSize = gridSize;

	std::vector<std::pair<Vector2i, bool>> neighbors;
	neighbors.reserve(8);

	// For node n, came_from[n] is the node immediately preceding it on the cheapest path from the start
	// to n currently known.
	std::map<Vector2i, Vector2i> cameFrom;

	// For node n, g_score[n] is the currently known cost of the cheapest path from start to n.
	std::map<Vector2i, float> gScore;
	gScore[start] = 0;

	// The set of discovered nodes that may need to be (re-)expanded.
	// Initially, only the start node is known.
	// This is usually implemented as a min-heap or priority queue rather than a hash-set.
	openSet = std::priority_queue<Node, std::vector<Node>, decltype(&compare)>{compare};
	openSet.emplace(start, 0);

	closedSet.clear();

	while (!openSet.empty()) {
		// This operation can occur in O(Log(N)) time if open_set is a min-heap or a priority queue
		auto current = openSet.top().first;

		closedSet.emplace(current);
		openSet.pop();
		if (current == goal) {
			reconstructPath(cameFrom, current, totalPath);
			return true;
		}

		neighbors.clear();
		neighbors.emplace_back(Vector2i{current.x - step.x, current.y - step.y}, true);
		neighbors.emplace_back(Vector2i{current.x - step.x, current.y         }, false);
		neighbors.emplace_back(Vector2i{current.x - step.x, current.y + step.y}, true);
		neighbors.emplace_back(Vector2i{current.x,          current.y - step.y}, false);
		neighbors.emplace_back(Vector2i{current.x,          current.y + step.y}, false);
		neighbors.emplace_back(Vector2i{current.x + step.x, current.y - step.y}, true);
		neighbors.emplace_back(Vector2i{current.x + step.x, current.y         }, false);
		neighbors.emplace_back(Vector2i{current.x + step.x, current.y + step.y}, true);

		for (auto [neighbor, corner] : neighbors) {
			if (isObstructed(neighbor))
				continue;
			if (neighbor.distance2(start) > 250000)
				continue;

			// d(current,neighbor) is the weight of the edge from current to neighbor
			// tentative_g_score is the distance from start to the neighbor through current
			float tentativeGScore = GET_OPTIONAL(gScore, current) + (corner ? 14 : 10);

			if (tentativeGScore >= GET_OPTIONAL(gScore, neighbor))
				continue;

			auto diff = current - goal;

			// This path to neighbor is better than any previous one. Record it!
			gScore[neighbor] = tentativeGScore;
			openSet.emplace(neighbor, tentativeGScore + std::max(std::abs(diff.x), std::abs(diff.y)) * 100);
			cameFrom[neighbor] = current;
		}
	}

	// Open set is empty but goal was never reached
	return false;
}


void AStar::computePath(
	const PathFindingGrid &grid,
	Vector2i currentPos,
	Vector2i dest,
	Vector2u hitBoxSize,
	Vector2u step,
	uint16_t mask,
	std::vector<Vector2i> &buffer
)
{
	auto diff = dest - currentPos;

	__lastStepSize = step;
	diff.y -= diff.x % step.x;
	diff.y -= diff.y % step.y;
	buffer.clear();
	aStar(currentPos, dest, [&grid, mask, dest, hitBoxSize](Vector2i p) -> float {
		if (p.x < 0 || p.y < 0 || p.x + hitBoxSize.x > grid.gridSize.x || p.y + hitBoxSize.y > grid.gridSize.y)
			return true;
		for (size_t y = 0; y < hitBoxSize.y; y++)
			for (size_t x = 0; x < hitBoxSize.x; x++) {
				auto attrs = grid.grid[(p.x + x) + (p.y + y) * grid.gridSize.x];

				if ((attrs & mask) != mask || PFGRIDMASK_NB_UNITS(attrs) != 0)
					return true;
			}
		return false;
	}, grid.gridSize, step, buffer);
}

sf::Image AStar::getDebugImage()
{
	Vector2u size = __lastGridSize;
	size.x /= __lastStepSize.x;
	size.y /= __lastStepSize.y;
	sf::Image img{size, sf::Color{0, 0, 0xFF, 0}};

	while (!openSet.empty()) {
		auto pos = openSet.top().first;

		pos.x /= __lastStepSize.x;
		pos.y /= __lastStepSize.y;
		img.setPixel(pos, sf::Color{0, 0, 0xFF, 0x80});
		openSet.pop();
	}
	for (auto pos : closedSet) {
		pos.x /= __lastStepSize.x;
		pos.y /= __lastStepSize.y;

		auto color = img.getPixel(pos);

		color.a += 0x20;
		img.setPixel(pos, color);
	}
	return img;
}
