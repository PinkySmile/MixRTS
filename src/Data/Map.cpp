//
// Created by PinkySmile on 10/05/2026.
//

#include <fstream>
#include <nlohmann/json.hpp>
#include <cpptrace/from_current.hpp>
#include "Resources/Game.hpp"
#include "Mod.hpp"
#include "Map.hpp"

MapResource::MapResource(const std::filesystem::path &folder, const nlohmann::json &json)
{
	assert_exp(json.contains("strategy") && json["strategy"].is_string());
	assert_exp(json.contains("lots") && json["lots"].is_number());
	assert_exp(json.contains("storage") && json["storage"].is_number());
	assert_exp(json.contains("id") && json["id"].is_number());
	assert_exp(json.contains("deplete") && json["deplete"].is_number());
	assert_exp(!json.contains("only_adjacent_collectible") || json["only_adjacent_collectible"].is_array());
	assert_exp(json.contains("value") && json["value"].is_object());
	for (auto &[key, v] : json["value"].items())
		assert_msg(v.is_number(), "json[\"value\"][\"" + key + "\"]");

	std::string strat = json["strategy"];

	if (strat == "random") {
		assert_exp(json.contains("tile_pool") && json["tile_pool"].is_array());
		for (size_t i = 0; i < json["tile_pool"].size(); i++)
			assert_msg(json["tile_pool"][i].is_number_unsigned(), std::to_string(i));
		this->tiles = json["tile_pool"].get<std::vector<unsigned>>();
		this->strategy = MapResource::STRAT_RANDOM;
	} else if (strat == "connected") {
		assert_exp(json.contains("tiles") && json["tiles"].is_string());
		std::filesystem::path path = folder / json["tiles"];
		std::ifstream streamTiles{path, std::fstream::binary};

		this->tiles.resize(256);
		assert_exp(streamTiles);
		static_assert(256 * sizeof(unsigned) == 1024);
		assert_exp(streamTiles.read(reinterpret_cast<char *>(this->tiles.data()), 1024));
		this->strategy = MapResource::STRAT_CONNECTED;
	} else
		assert_not_reached_msg("Invalid strategy: " + strat);
	this->lots = json["lots"];
	this->storage = json["storage"];
	this->id = json["id"];
	this->deplete = json["deplete"];
	this->value = json["value"];

	if (json.contains("only_adjacent_collectible")) {
		for (size_t i = 0; i < json["only_adjacent_collectible"].size(); i++)
			assert_msg(json["only_adjacent_collectible"][i].is_number_unsigned(), std::to_string(i));
		this->onlyAdjCollectible = json["only_adjacent_collectible"].get<std::vector<unsigned>>();
	}
}

void MapMetadata::loadColors(const std::filesystem::path &path)
{
	std::ifstream stream{ path };
	unsigned char count;
	std::vector<sf::Color> colors;
	size_t totalSize = std::filesystem::file_size(path);

	stream.read(reinterpret_cast<char *>(&count), 1);
	colors.resize(count);
	for (size_t i = 0; i < count; i++) {
		stream.read(reinterpret_cast<char *>(&colors[i].b), 1);
		stream.read(reinterpret_cast<char *>(&colors[i].g), 1);
		stream.read(reinterpret_cast<char *>(&colors[i].r), 1);
		colors[i].a = 255;
	}
	this->mapColors.reserve(totalSize - stream.tellg());
	while (stream.read(reinterpret_cast<char *>(&count), 1)) {
		assert_exp(count < colors.size());
		this->mapColors.push_back(colors[count]);
	}
}

void MapMetadata::load(Mod &mod, const std::filesystem::path &path)
{
	std::ifstream streamMeta{path};
	nlohmann::json json;

	assert_msg(streamMeta, path.string());
	streamMeta >> json;
	assert_exp(json.contains("map_colors") && json["map_colors"].is_string());
	assert_exp(json.contains("animations") && json["animations"].is_array());
	assert_exp(json.contains("generators") && json["generators"].is_array());
	assert_exp(json.contains("resources") && json["resources"].is_array());
	assert_exp(!json.contains("skirts_conc") || json["skirts"].is_object());
	assert_exp(json.contains("skirts") && json["skirts"].is_object());

	this->loadColors(path.parent_path() / json["map_colors"].get<std::string>());
	for (size_t i = 0; i < json["animations"].size(); i++) {
		auto &anim = json["animations"][i];

		assert_msg(anim.is_string(), "json[\"animations\"][" + std::to_string(i) + "]");
		cpptrace::try_catch([&]{
			mod.loadAnimations(path.parent_path() / anim.get<std::string>(), this->animations);
		}, [&](cpptrace::lazy_exception &e) {
			throw _AssertionFailedException("In key [\"animations\"][" + std::to_string(i) + "]\n" + e.what(), e);
		}, [&](std::exception &e) {
			cpptrace::raw_trace trace = cpptrace::raw_trace_from_current_exception();

			throw _AssertionFailedException("In key [\"animations\"][" + std::to_string(i) + "]\n" + e.what(), std::move(trace));
		});
	}
	for (size_t i = 0; i < json["generators"].size(); i++) {
		auto &anim = json["generators"][i];

		assert_msg(anim.is_object(), "json[\"generators\"][" + std::to_string(i) + "]");
	}
	for (size_t i = 0; i < json["resources"].size(); i++) {
		auto &anim = json["resources"][i];

		assert_msg(anim.is_object(), "json[\"resources\"][" + std::to_string(i) + "]");

		cpptrace::try_catch([&]{
			MapResource res{path.parent_path(), anim};

			this->resources.emplace(res.id, res);
		}, [&](cpptrace::lazy_exception &e) {
			throw _AssertionFailedException("In key [\"resources\"][" + std::to_string(i) + "]\n" + e.what(), e);
		}, [&](std::exception &e) {
			cpptrace::raw_trace trace = cpptrace::raw_trace_from_current_exception();

			throw _AssertionFailedException("In key [\"resources\"][" + std::to_string(i) + "]\n" + e.what(), std::move(trace));
		});
	}
	for (auto &[key, value] : json["skirts"].items()) {
		assert_msg(value.is_array(), "json[\"skirts\"][" + key + "]");
		for (size_t i = 0; i < value.size(); i++)
			assert_msg(value[i].is_number_unsigned(), "json[\"skirts\"][" + key + "][" + std::to_string(i) + "]");
		cpptrace::try_catch([&]{
			Vector2u size;
			auto pos = key.find('x');

			assert_exp(pos != std::string::npos);
			size.x = std::stoul(key.substr(0, pos));
			size.y = std::stoul(key.substr(pos + 1));
			assert_exp(!this->skirts.contains(size));
			this->skirts[size] = value.get<std::vector<unsigned>>();
			assert_exp(this->skirts[size].size() == size.x * size.y);
		}, [&](cpptrace::lazy_exception &e) {
			throw _AssertionFailedException("In key [\"skirts\"][" + key + "]\n" + e.what(), e);
		}, [&](std::exception &e) {
			cpptrace::raw_trace trace = cpptrace::raw_trace_from_current_exception();

			throw _AssertionFailedException("In key [\"skirts\"][" + key + "]\n" + e.what(), std::move(trace));
		});
	}
	if (!json.contains("skirts_conc"))
		this->skirtsConcrete = this->skirts;
	else for (auto &[key, value] : json["skirts_conc"].items()) {
		assert_msg(value.is_array(), "json[\"skirts_conc\"][" + key + "]");
		for (size_t i = 0; i < value.size(); i++)
			assert_msg(value[i].is_number_unsigned(), "json[\"skirts_conc\"][" + key + "][" + std::to_string(i) + "]");
		cpptrace::try_catch([&]{
			Vector2u size;
			auto pos = key.find('x');

			assert_exp(pos != std::string::npos);
			size.x = std::stoul(key.substr(0, pos));
			size.y = std::stoul(key.substr(pos + 1));
			assert_exp(!this->skirtsConcrete.contains(size));
			this->skirtsConcrete[size] = value.get<std::vector<unsigned>>();
			assert_exp(this->skirtsConcrete[size].size() == size.x * size.y);
		}, [&](cpptrace::lazy_exception &e) {
			throw _AssertionFailedException("In key [\"skirts_conc\"][" + key + "]\n" + e.what(), e);
		}, [&](std::exception &e) {
			cpptrace::raw_trace trace = cpptrace::raw_trace_from_current_exception();

			throw _AssertionFailedException("In key [\"skirts_conc\"][" + key + "]\n" + e.what(), std::move(trace));
		});
	}
}

void LoadedMap::load(Mod &mod, const std::filesystem::path &path)
{
	std::ifstream streamMap{path / "map.json"};
	nlohmann::json json;

	assert_msg(streamMap, (path / "map.json").string());
	streamMap >> json;

	assert_exp(json.contains("size") && json["size"].is_object());
	assert_exp(json["size"].contains("x") && json["size"]["x"].is_number_unsigned());
	assert_exp(json["size"].contains("y") && json["size"]["y"].is_number_unsigned());
	assert_exp(json.contains("grid") && json["grid"].is_object());
	assert_exp(json["grid"].contains("x") && json["grid"]["x"].is_number_unsigned());
	assert_exp(json["grid"].contains("y") && json["grid"]["y"].is_number_unsigned());
	assert_exp(json.contains("translate") && json["translate"].is_object());
	assert_exp(json["translate"].contains("x") && json["translate"]["x"].is_number_unsigned());
	assert_exp(json["translate"].contains("y") && json["translate"]["y"].is_number_unsigned());
	assert_exp(json.contains("starts") && json["starts"].is_array());
	assert_exp(json.contains("tileset") && json["tileset"].is_string());

	this->grid.gridSize.x = json["size"]["x"];
	this->grid.gridSize.y = json["size"]["y"];
	this->grid.tileSize.x = json["grid"]["x"];
	this->grid.tileSize.y = json["grid"]["y"];
	this->grid.translate.x = json["translate"]["x"];
	this->grid.translate.y = json["translate"]["y"];
	for (size_t i = 0; i < json["starts"].size(); i++) {
		auto &v = json["starts"][i];

		assert_msg(v.contains("x") && v["x"].is_number_unsigned(), "json[\"starts\"][" + std::to_string(i) + "]");
		assert_msg(v.contains("y") && v["y"].is_number_unsigned(), "json[\"starts\"][" + std::to_string(i) + "]");
		this->grid.startAreas.emplace_back(v["x"].get<unsigned>(), v["y"].get<unsigned>());
	}
	this->grid.grid.resize(this->grid.gridSize.x * this->grid.gridSize.y);
	this->originalIndices.resize(this->grid.gridSize.x * this->grid.gridSize.y);
	this->usedSpaces.resize(MAX_TEAM_COUNT);
	for (auto &s : this->usedSpaces)
		s.resize(this->grid.gridSize.x * this->grid.gridSize.y, 0);
	this->minimap.resize(this->grid.gridSize);

	std::string tileset = json["tileset"].get<std::string>();

	cpptrace::try_catch([&]{
		this->metadata.load(mod, path / tileset / "metadata.json");
	}, [&](cpptrace::lazy_exception &e) {
		throw _AssertionFailedException("While loading map metadata " + (path / tileset / "metadata.json").string() + "\n" + e.what(), e);
	}, [&](std::exception &e) {
		cpptrace::raw_trace trace = cpptrace::raw_trace_from_current_exception();

		throw _AssertionFailedException("While loading map metadata " + (path / tileset / "metadata.json").string() + "\n" + e.what(), std::move(trace));
	});
	assert_exp(this->renderCache.resize({
		this->grid.tileSize.x * this->grid.gridSize.x,
		this->grid.tileSize.y * this->grid.gridSize.y
	}));
	assert_exp(this->tileSet.loadFromFile(path / tileset / "tilesheet.png"));

	std::filesystem::path attributesPath = path / tileset / "tileattrs.dat";
	size_t attributesSize = std::filesystem::file_size(attributesPath);
	auto tileSetSize = this->tileSet.getSize();

	assert_exp(tileSetSize.x % this->grid.tileSize.x == 0);
	assert_exp(tileSetSize.y % this->grid.tileSize.y == 0);
	assert_exp(attributesSize / 4 == (tileSetSize.x / this->grid.tileSize.x) * (tileSetSize.y / this->grid.tileSize.y));

	std::ifstream streamTiles{path / "tiles.dat", std::fstream::binary};

	assert_exp(streamTiles);
	assert_exp(streamTiles.read(reinterpret_cast<char *>(this->originalIndices.data()), this->originalIndices.size() * sizeof(*this->originalIndices.data())));
	this->displayedIndices = this->originalIndices;

	std::ifstream streamAttributes{attributesPath, std::fstream::binary};
	std::vector<uint32_t> attributes;

	attributes.resize(attributesSize / 4);
	assert_exp(streamAttributes);
	assert_exp(streamAttributes.read(reinterpret_cast<char *>(attributes.data()), attributes.size() * sizeof(*attributes.data())));

	for (size_t i = 0; i < this->grid.grid.size(); i++) {
		this->grid.grid[i] = attributes[this->originalIndices[i] & 0xFFFFFF] | (this->originalIndices[i] & 0xFF000000);
		this->displayedIndices[i] &= 0xFFFFFF;
	}
	this->buildCache();


	this->grid.pathFindingGrid.gridSize = this->grid.gridSize;
	this->grid.pathFindingGrid.grid.resize(this->grid.pathFindingGrid.gridSize.x * this->grid.pathFindingGrid.gridSize.y);
	for (size_t i = 0; i < this->grid.grid.size(); i++)
		this->grid.pathFindingGrid.grid[i] = this->grid.grid[i] & 0x3FF;
	this->grid.pathFindingGrid.gridImage.resize(this->grid.pathFindingGrid.gridSize, Color::Transparent);
	for (unsigned y = 0; y < this->grid.pathFindingGrid.gridSize.y; y++)
		for (unsigned x = 0; x < this->grid.pathFindingGrid.gridSize.x; x++)
			this->grid.pathFindingGrid.gridImage.setPixel({x, y}, __getPFDebugColor(this->grid.pathFindingGrid.grid[x + y * this->grid.pathFindingGrid.gridSize.x]));
	this->grid.pathFindingGrid.textureOutOfSync = true;
	this->grid.pathFindingGrid.usedSpaces.resize(MAX_TEAM_COUNT);
	for (auto &s : this->grid.pathFindingGrid.usedSpaces)
		s.resize(this->grid.pathFindingGrid.gridSize.x * this->grid.pathFindingGrid.gridSize.y, 0);


	this->pathFindingGrid.gridSize = {
		this->grid.gridSize.x * this->grid.tileSize.x,
		this->grid.gridSize.y * this->grid.tileSize.y
	};
	this->pathFindingGrid.grid.resize(this->pathFindingGrid.gridSize.x * this->pathFindingGrid.gridSize.y);
	for (size_t i = 0; i < this->pathFindingGrid.gridSize.x * this->pathFindingGrid.gridSize.y; i++) {
		size_t x = i % this->pathFindingGrid.gridSize.x / this->grid.tileSize.x;
		size_t y = i / this->pathFindingGrid.gridSize.x / this->grid.tileSize.y;

		this->pathFindingGrid.grid[i] = this->grid.grid[x + y * this->grid.gridSize.x] & 0x3FF;
	}
	this->pathFindingGrid.gridImage.resize(this->pathFindingGrid.gridSize, Color::Transparent);
	for (unsigned y = 0; y < this->pathFindingGrid.gridSize.y; y++)
		for (unsigned x = 0; x < this->pathFindingGrid.gridSize.x; x++)
			this->pathFindingGrid.gridImage.setPixel({x, y}, __getPFDebugColor(this->pathFindingGrid.grid[x + y * this->pathFindingGrid.gridSize.x]));
	this->pathFindingGrid.usedSpaces.resize(MAX_TEAM_COUNT);
	for (auto &s : this->pathFindingGrid.usedSpaces)
		s.resize(static_cast<size_t>(this->pathFindingGrid.gridSize.x) * this->pathFindingGrid.gridSize.y, 0);
	this->pathFindingGrid.textureOutOfSync = true;
}

void LoadedMap::buildCache()
{
	this->renderCache.clear(sf::Color::Transparent);
	for (unsigned y = 0; y < this->grid.gridSize.y; y++)
		for (unsigned x = 0; x < this->grid.gridSize.x; x++)
			this->refreshTile(x, y, false, false);
	this->renderCache.display();
}

#define HAS_RESOURCE(x_, y_) ((x_) >= this->grid.gridSize.x || (y_) >= this->grid.gridSize.y ? true : GRID_RESOURCE_TYPE(this->grid.grid[(x_) + (y_) * this->grid.gridSize.x]) != 0)
void LoadedMap::refreshTile(unsigned int x, unsigned int y, bool commit, bool reload)
{
	auto textureSize = this->tileSet.getSize();
	uint32_t &index = this->displayedIndices[x + y * this->grid.gridSize.x];
	uint32_t old = index;
	size_t attrs = this->grid.grid[x + y * this->grid.gridSize.x];
	unsigned res = GRID_RESOURCE_TYPE(attrs);

	index = this->originalIndices[x + y * this->grid.gridSize.x] & 0xFFFFFF;
	textureSize.x /= this->grid.tileSize.x;
	this->sprite.setPosition({
		static_cast<float>(x * this->grid.tileSize.x),
		static_cast<float>(y * this->grid.tileSize.y)
	});
	this->sprite.setTextureRect({
		sf::Vector2i(
			index % textureSize.x * this->grid.tileSize.x,
			index / textureSize.x * this->grid.tileSize.y
		),
		sf::Vector2i(this->grid.tileSize)
	});
	this->renderCache.draw(this->sprite);

	if (res != 0) {
		assert_exp(this->metadata.resources.contains(res));

		auto &r = this->metadata.resources.at(res);

		switch (r.strategy) {
		case MapResource::STRAT_RANDOM:
			if (reload && std::ranges::find(r.tiles, old) != r.tiles.end())
				index = old;
			else
				index = r.tiles[game->random() % r.tiles.size()];
			this->sprite.setTextureRect({
				sf::Vector2i(
					index % textureSize.x * this->grid.tileSize.x,
					index / textureSize.x * this->grid.tileSize.y
				),
				sf::Vector2i(this->grid.tileSize)
			});
			this->renderCache.draw(this->sprite);
			break;
		case MapResource::STRAT_CONNECTED:
			index = 0;
			index |= HAS_RESOURCE(x - 1, y + 1) << 0;
			index |= HAS_RESOURCE(x,     y + 1) << 1;
			index |= HAS_RESOURCE(x + 1, y + 1) << 2;
			index |= HAS_RESOURCE(x - 1, y)     << 3;
			index |= HAS_RESOURCE(x + 1, y)     << 4;
			index |= HAS_RESOURCE(x - 1, y - 1) << 5;
			index |= HAS_RESOURCE(x,     y - 1) << 6;
			index |= HAS_RESOURCE(x + 1, y - 1) << 7;
			index = r.tiles[index];
			this->sprite.setTextureRect({
				sf::Vector2i(
					index % textureSize.x * this->grid.tileSize.x,
					index / textureSize.x * this->grid.tileSize.y
				),
				sf::Vector2i(this->grid.tileSize)
			});
			this->renderCache.draw(this->sprite);
			break;
		}
	}
	this->minimap.setPixel({x, y}, this->metadata.mapColors[index]);
	if (index != old && reload) {
		this->refreshTile(x - 1, y - 1, false, true);
		this->refreshTile(x - 1, y, false, true);
		this->refreshTile(x - 1, y + 1, false, true);
		this->refreshTile(x, y - 1, false, true);
		this->refreshTile(x, y + 1, false, true);
		this->refreshTile(x + 1, y - 1, false, true);
		this->refreshTile(x + 1, y, false, true);
		this->refreshTile(x + 1, y + 1, false, true);
	}
	if (commit)
		this->renderCache.display();
}
