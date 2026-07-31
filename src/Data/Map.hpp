//
// Created by PinkySmile on 10/05/2026.
//

#ifndef MIXRTS_MAP_HPP
#define MIXRTS_MAP_HPP


#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include "Vector.hpp"
#include "FindPath.hpp"
#include "forward.hpp"

#define MAX_TEAM_COUNT 16

struct MapState {
	// OPTIMIZE: Align line size to PAGE_SIZE, or a power of 2
	std::vector<uint32_t> grid;
	PathFindingGrid pathFindingGrid;
	std::vector<std::vector<uint8_t>> placementGrid;
	Vector2u tileSize;
	Vector2u gridSize;
	Vector2i translate;
	std::vector<Vector2u> startAreas;

	struct Quad {
		Vector2f pt1;
		Vector2f pt2;
		Vector2f pt3;
		Vector2f pt4;
	};

	void clampedPosition(const Quad &abs, Vector2i &topLeft, Vector2i &bottomRight) const;
	void updatePlacementGrid(const Vector2i &rel, unsigned player, int delta);
	void updatePlacementGrid(const Quad &abs, unsigned player, int delta);
	void resetGridBits(const Vector2i &rel, uint32_t bits);
	void resetGridBits(const Quad &abs, uint32_t bits);
	void setGridBits(const Vector2i &rel, uint32_t bits);
	void setGridBits(const Quad &abs, uint32_t bits);
	[[nodiscard]] bool hasAnyGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] bool hasAllGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] bool hasExactlySomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits) const;
	[[nodiscard]] bool anyHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool anyHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool anyHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] bool allHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool allHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] bool allHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] Vector2i absToTileCoord(const Vector2i &) const;
	[[nodiscard]] Vector2f absToRelCoord(const Vector2i &) const;
	[[nodiscard]] Vector2i topLeftCoordToAbs(const Vector2i &) const;
	[[nodiscard]] Vector2i topLeftCoordToAbs(size_t index) const;
	[[nodiscard]] Vector2i relCoordToAbs(const Vector2f &) const;
	[[nodiscard]] Vector2i indexToRelPos(size_t index) const;
	[[nodiscard]] Quad tileCoordToAbs(const Vector2i &) const;
	[[nodiscard]] Quad tileCoordToAbs(size_t index) const;
};

struct MapResource {
	enum Strategy {
		STRAT_RANDOM,
		STRAT_CONNECTED
	};

	Strategy strategy;
	std::vector<unsigned> tiles;
	unsigned lots;
	unsigned storage;
	unsigned id;
	unsigned deplete;
	std::unordered_map<std::string, unsigned> value;
	std::vector<unsigned> onlyAdjCollectible;

	MapResource(const std::filesystem::path &folder, const nlohmann::json &json);
};

struct MapMetadata {
	std::unordered_map<std::string, std::vector<Animation>> animations;
	std::unordered_map<unsigned, MapResource> resources;
	std::map<Vector2u, std::vector<unsigned>> skirtsConcrete;
	std::map<Vector2u, std::vector<unsigned>> skirts;
	std::vector<sf::Color> mapColors;

	void load(Mod &mod, const std::filesystem::path &path);
	void loadColors(const std::filesystem::path &path);
};

struct LoadedMap {
	MapState grid;
	MapMetadata metadata;
	PathFindingGrid pathFindingGrid;
	std::vector<uint32_t> originalIndices;
	std::vector<uint32_t> displayedIndices;
	sf::Image minimap;
	std::vector<std::vector<uint8_t>> usedSpaces;
	//std::vector<std::set<GameObject *>> quadrants;
	//std::vector<std::unique_ptr<GameObject>> mapObjects;
	//std::vector<ResourcePatch> resources;
	// ???? tileMeta

	sf::Texture tileSet;
	sf::Sprite sprite{this->tileSet};
	sf::RenderTexture renderCache;

	void load(Mod &mod, const std::filesystem::path &path);
	void buildCache();
	void refreshTile(unsigned x, unsigned y, bool commit = true, bool reload = true);
};


#endif //MIXRTS_MAP_HPP
