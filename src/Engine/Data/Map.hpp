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
#include <sol/forward.hpp>
#include "Vector.hpp"
#include "FindPath.hpp"
#include "forward.hpp"

#define MAX_TEAM_COUNT 32
#define NEUTRAL_TEAM (MAX_TEAM_COUNT - 1)
#define NEUTRAL_OWNER UINT32_MAX

struct MYDLL_API MapState {
	// OPTIMIZE: Align line size to PAGE_SIZE, or a power of 2
	std::vector<uint32_t> grid;
	PathFindingGrid pathFindingGrid;
	std::vector<std::vector<uint8_t>> placementGrid;
	Vector2u tileSize;
	Vector2u gridSize;
	Vector2i translate;

	struct Quad {
		Vector2f pt1;
		Vector2f pt2;
		Vector2f pt3;
		Vector2f pt4;

		static void pushUserType(sol::state &state);
	};

	void clampedPosition(const Quad &abs, Vector2i &topLeft, Vector2i &bottomRight) const;
	void updatePlacementGrid(const Vector2i &rel, unsigned player, int delta);
	void updatePlacementGrid(const Quad &abs, unsigned player, int delta);
	void resetGridBits(const Vector2i &rel, uint32_t bits);
	void resetGridBits(const Quad &abs, uint32_t bits);
	void setGridBits(const Vector2i &rel, uint32_t bits);
	void setGridBits(const Quad &abs, uint32_t bits);
	[[nodiscard]] inline bool hasAnyGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] inline bool hasAllGridBits(const Vector2i &rel, uint32_t bits) const;
	[[nodiscard]] inline bool hasExactlySomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits) const;
	[[nodiscard]] inline bool anyHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] inline bool anyHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] inline bool anyHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] inline bool allHasAnyGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] inline bool allHasAllGridBits(const Quad &abs, uint32_t bits) const;
	[[nodiscard]] inline bool allHasExactlySomeGridBits(const Quad &abs, uint32_t bits, uint32_t expected) const;
	[[nodiscard]] inline Vector2i absToTileCoord(const Vector2i &) const;
	[[nodiscard]] inline Vector2f absToRelCoord(const Vector2i &) const;
	[[nodiscard]] inline Vector2i topLeftCoordToAbs(const Vector2i &) const;
	[[nodiscard]] inline Vector2i topLeftCoordToAbs(size_t index) const;
	[[nodiscard]] inline Vector2i relCoordToAbs(const Vector2f &) const;
	[[nodiscard]] inline Vector2i indexToRelPos(size_t index) const;
	[[nodiscard]] inline Quad tileCoordToAbs(const Vector2i &) const;
	[[nodiscard]] inline Quad tileCoordToAbs(size_t index) const;

	static void pushUserType(sol::state &state);
};

struct MYDLL_API MapResource {
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

	static void pushUserType(sol::state &state);
};

struct MYDLL_API MapMetadata {
	std::unordered_map<std::string, std::vector<Animation>> animations;
	std::unordered_map<unsigned, MapResource> resources;
	std::map<Vector2u, std::vector<unsigned>> skirtsConcrete;
	std::map<Vector2u, std::vector<unsigned>> skirts;
	std::vector<sf::Color> mapColors;
	std::unordered_map<std::string, std::vector<float>> speedMuls;
	Vector2u tileSize;

	void load(Mod &mod, const std::filesystem::path &path);
	void loadColors(const std::filesystem::path &path);
};

struct MYDLL_API LoadedMap {
	struct ObjectParam {
		std::string type;
		Vector2i pos;
	};

	std::vector<ObjectParam> objects;
	GameManager *manager;
	size_t neutral;
	MapState grid;
	MapMetadata metadata;
	PathFindingGrid pathFindingGrid;
	std::vector<uint32_t> originalIndices;
	std::vector<uint32_t> displayedIndices;
	sf::Image minimap;
	std::vector<std::vector<uint8_t>> usedSpaces;
	std::vector<Vector2u> startAreas;
	//std::vector<std::set<GameObject *>> quadrants;
	//std::vector<std::unique_ptr<GameObject>> mapObjects;
	//std::vector<ResourcePatch> resources;
	// ???? tileMeta

	sf::Texture tileSet;
	sf::Sprite sprite{this->tileSet};
	sf::RenderTexture renderCache;

	void load(Mod &mod, const std::filesystem::path &path);
	void spawnObjects(size_t faction);
	void buildCache();
	void refreshTile(unsigned x, unsigned y, bool commit = true, bool reload = true);
	float getSpeedMul(const std::vector<std::string> &tags, unsigned terrain) const;

	//! Checksum of the map's gameplay files (json, tiles, tileset attributes and metadata). Images are excluded.
	//! Takes the faction crc as param and base for this crc.
	[[nodiscard]] static uint32_t computeChecksum(const std::string &path, uint32_t crc);
	static sf::Image renderDetailedPreview(const std::string &path);
};

struct MYDLL_API MapInfo {
	std::string path;
	sf::Image preview;
	std::string name;
	std::string origin;
	std::string author;
	Vector2u gridSize;
	Vector2u tileSize;
	std::string faction;
	std::vector<Vector2u> startAreas;

	void load(const std::string &path);
};


#endif //MIXRTS_MAP_HPP
