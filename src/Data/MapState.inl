//
// Created by PinkySmile on 26/04/2026.
//

#ifndef MIXRTS_MAPSTATE_INL
#define MIXRTS_MAPSTATE_INL


#include "Vector.hpp"
#include "Resources/Assert.hpp"
#include "GameManager.hpp"

inline Vector2i MapState::absToTileCoord(const Vector2i &p) const
{
	return {
		p.x / static_cast<int>(this->tileSize.x),
		p.y / static_cast<int>(this->tileSize.y)
	};
}

inline Vector2i MapState::topLeftCoordToAbs(const Vector2i &p) const
{
	return {static_cast<int>(p.x * this->tileSize.x), static_cast<int>(p.y * this->tileSize.y)};
}

inline MapState::Quad MapState::tileCoordToAbs(const Vector2i &p) const
{
	return {
		{static_cast<float>(p.x)     * this->tileSize.x, static_cast<float>(p.y)     * this->tileSize.y},
		{static_cast<float>(p.x + 1) * this->tileSize.x, static_cast<float>(p.y)     * this->tileSize.y},
		{static_cast<float>(p.x + 1) * this->tileSize.x, static_cast<float>(p.y + 1) * this->tileSize.y},
		{static_cast<float>(p.x)     * this->tileSize.x, static_cast<float>(p.y + 1) * this->tileSize.y}
	};
}

inline void MapState::clampedPosition(const MapState::Quad &abs, Vector2i &topLeft, Vector2i &bottomRight) const
{
	topLeft = this->absToTileCoord(abs.pt1);
	bottomRight = this->absToTileCoord(abs.pt3 - Vector2i{1, 1});
	topLeft.x = std::max(topLeft.x, 0);
	topLeft.y = std::max(topLeft.y, 0);
	topLeft.x = std::min<int>(topLeft.x, this->gridSize.x - 1);
	topLeft.y = std::min<int>(topLeft.y, this->gridSize.y - 1);
	bottomRight.x = std::max(bottomRight.x, 0);
	bottomRight.y = std::max(bottomRight.y, 0);
	bottomRight.x = std::min<int>(bottomRight.x, this->gridSize.x - 1);
	bottomRight.y = std::min<int>(bottomRight.y, this->gridSize.y - 1);
}

inline void MapState::updatePlacementGrid(const Vector2i &rel, unsigned int player, int delta)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));

	auto &b = this->placementGrid[player][rel.x + rel.y * this->gridSize.x];

	assert_exp(delta < 0 || b < 256 - delta);
	assert_exp(delta > 0 || b >= delta);
	b += delta;
}

inline void MapState::updatePlacementGrid(const MapState::Quad &abs, unsigned int player, int delta)
{
	Vector2i topLeft;
	Vector2i bottomRight;

	assert_exp(delta != 0);
	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++) {
			auto &b = this->placementGrid[player][x + y * this->gridSize.x];

			assert_exp(delta < 0 || b < 256 - delta);
			assert_exp(delta > 0 || b >= delta);
			b += delta;
		}
}

inline void MapState::setGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	this->grid[rel.x + rel.y * this->gridSize.x] |= bits;
}

inline void MapState::setGridBits(const MapState::Quad &abs, uint32_t bits)
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			this->grid[x + y * this->gridSize.x] |= bits;
}

inline void MapState::resetGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	this->grid[rel.x + rel.y * this->gridSize.x] &= ~bits;
}

inline void MapState::resetGridBits(const MapState::Quad &abs, uint32_t bits)
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			this->grid[x + y * this->gridSize.x] &= ~bits;
}

inline bool MapState::hasAnyGridBits(const Vector2i &rel, uint32_t bits) const
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) != 0;
}

inline bool MapState::hasAllGridBits(const Vector2i &rel, uint32_t bits) const
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) == bits;
}

inline bool MapState::hasExactlySomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits) const
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) == expectedBits;
}

inline bool MapState::anyHasAnyGridBits(const MapState::Quad &abs, uint32_t bits) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) != 0)
				return true;
	return false;
}

inline bool MapState::anyHasAllGridBits(const MapState::Quad &abs, uint32_t bits) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) == bits)
				return true;
	return false;
}

inline bool MapState::anyHasExactlySomeGridBits(const MapState::Quad &abs, uint32_t bits, uint32_t expected) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) == expected)
				return true;
	return false;
}

inline bool MapState::allHasAnyGridBits(const MapState::Quad &abs, uint32_t bits) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) == 0)
				return false;
	return true;
}

inline bool MapState::allHasAllGridBits(const MapState::Quad &abs, uint32_t bits) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) != bits)
				return false;
	return true;
}

inline bool MapState::allHasExactlySomeGridBits(const MapState::Quad &abs, uint32_t bits, uint32_t expected) const
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			if ((this->grid[x + y * this->gridSize.x] & bits) != expected)
				return false;
	return true;
}


#endif //MIXRTS_MAPSTATE_INL
