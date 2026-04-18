//
// Created by PinkySmile on 07/04/2026.
//

#include <fstream>
#include "GameManager.hpp"
#include "Faction.hpp"
#include "Building.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "Player.hpp"
#include "Mod.hpp"
#include "UI.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

GameManager::GameManager(const std::vector<Faction> &factions) :
	_factions(factions)
{
	for (auto &f : this->_factions) {
		f.load();
		this->_mapStates.emplace_back();
	}
}

void GameManager::loadMap(Mod &mod, const std::filesystem::path &path)
{
	this->_map.load(path);

	// TODO: Properly convert
	for (auto &state : this->_mapStates)
		state = this->_map.grid;
	//for (auto &state : this->_mapStates) {
	//	state.tileSize = {GRID_SIZE, GRID_SIZE};
	//	state.gridSize = {GRID_DIMS_X, GRID_DIMS_Y};
	//	state.grid.resize(GRID_DIMS_X * GRID_DIMS_Y);
	//	for (size_t i = 0; i < GRID_DIMS_X * GRID_DIMS_Y; i++)
	//		state.grid[i] = ~GRIDMASK_HAS_CONCRETE;
	//	state.startAreas.emplace_back(1, 2);
	//}
}

void GameManager::addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea)
{
	size_t index = this->_players.size();
	auto &faction = this->_factions[factionIndex];

	this->_players.emplace_back(*this, this->_factions, factionIndex, color, this->_mapStates);
	this->_players[index].state.systemMoney = faction.getStartingMoney();
	for (auto &b : faction.getStartingBuildings()) {
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		start += b.offset;
		this->spawnBuilding(index, start, b.queue, b.name, false);
	}
}

void GameManager::update()
{
	for (auto &player : this->_players)
		player.update();
	for (auto &b : this->_liveBuildings)
		b.update();
	for (auto &player : this->_players)
		player.handleUiEvents();

	for (auto &list : this->_renderBuildings | std::ranges::views::values) {
		auto it = list.begin();

		while (it != list.end())
			if ((*it)->isDisposable())
				it = list.erase(it);
			else
				++it;
	}

	auto it = this->_liveBuildings.begin();

	while (it != this->_liveBuildings.end())
		if (it->isDisposable())
			it = this->_liveBuildings.erase(it);
		else
			++it;
}

void GameManager::updateRender()
{
	for (auto &player : this->_players)
		player.updateRender();
}

void GameManager::render()
{
	sf::Sprite sprite{this->_map.renderCache.getTexture()};

	game->screen->draw(sprite);
	for (auto &list : this->_renderBuildings | std::ranges::views::values)
		for (auto b : list)
			b->render(0);
	for (auto &list : this->_renderBuildings | std::ranges::views::values)
		for (auto b : list)
			b->render(1);
	for (auto &player : this->_players)
		player.render();
}

void GameManager::renderAbove()
{
	for (auto &player : this->_players)
		player.renderAbove();
}

std::vector<Player> &GameManager::getPlayers()
{
	return this->_players;
}

void GameManager::spawnBuilding(size_t owner, Vector2u pos, const std::string &buildQueue, const std::string &id, bool forceConcrete)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &buildings = player.getBuildings();
	auto &base = *buildings.at(id);
	auto &tiles = base.getTileData();
	auto proxy = base.getProxy();
	unsigned nbConcreteTiles = 0;
	unsigned nbTiles = 0;
	auto &concBase = *buildings.at(player.getPrimaryFaction().getBase().getName() + ":conc");

	for (size_t y = 0; y < tiles.size(); y++) {
		auto &line = tiles[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			uint32_t &tile = map.grid[x + pos.x + (y + pos.y) * map.gridSize.x];

			if (
				(
					(line[x] & Building::MASK_PLACEMENT_CONCRETE) ||
					((line[x] & Building::MASK_PLACEMENT_REQUIRED) && forceConcrete)
				) &&
				GRID_CAN_BUILD(tile)
			) {
				tile |= GRIDMASK_HAS_CONCRETE;
				this->_liveBuildings.emplace_back(concBase);
				auto &b = this->_liveBuildings.back();
				b.playerState = &player.state;
				b.setPosition({static_cast<int>((pos.x + x) * 32), static_cast<int>((pos.y + y) * 32)});
				b.setColor(player.getColor());
				b.built();
				player.state.myBuildings.emplace_back(&b);
				this->_renderBuildings[pos.y + y].push_front(&b);
			}
			if (line[x] & Building::MASK_PLACEMENT_REQUIRED)
				tile &= ~GRIDMASK_IS_FREE;
			if (line[x] & Building::MASK_SOLID) {
				nbTiles++;
				nbConcreteTiles += (tile & GRIDMASK_HAS_CONCRETE) == GRIDMASK_HAS_CONCRETE;
			}
		}
	}

	base.playBuiltSound();
	if (!base.isFake()) {
		if (proxy)
			this->_liveBuildings.push_back(*buildings.at(*proxy));
		else
			this->_liveBuildings.push_back(base);
		auto &b = this->_liveBuildings.back();
		b.playerState = &player.state;
		b.setPosition({static_cast<int>(pos.x * 32), static_cast<int>(pos.y * 32)});
		b.setColor(player.getColor());
		b.built();

		auto &gqueue = b.getGivenQueue();

		if (gqueue)
			player.state.queueBuildings[*gqueue].push_back(&b);
		if (nbTiles != nbConcreteTiles)
			b.damage(b.getHealth() * (nbTiles - nbConcreteTiles) / (nbTiles * 2));

		auto group = b.getGroup();

		++player.state.elementsOwned[group];
		if (group != id)
			++player.state.elementsOwned[id];

		auto p = b.getPower();
		auto &storage = b.getStorage();

		if (p < 0)
			player.state.producedPower += -p;
		else
			player.state.consumedPower += p;
		for (auto &pair : storage)
			player.state.totalStorage[pair.first] += pair.second;

		auto &parent = player.state.primaryBuildings[buildQueue];

		if (parent)
			parent->produced();
		else if (player.state.queueBuildings.contains(buildQueue))
			for (auto &p: player.state.queueBuildings[buildQueue])
				 if (!p->isObstructed()) {
					p->produced();
					break;
				}
		player.state.myBuildings.emplace_back(&b);
		this->_renderBuildings[tiles.size() + pos.y - 1].push_back(&b);

		for (size_t y = 0; y < tiles.size(); y++) {
			auto &line = tiles[y];

			if (y + pos.y >= map.gridSize.y)
				break;
			for (size_t x = 0; x < line.size(); x++) {
				if (x + pos.x >= map.gridSize.x)
					break;
				if (line[x] & Building::MASK_SOLID)
					player.state.buildingHitboxes[Vector2u(pos.x + x, pos.y + y)].push_back(&b);
			}
		}
		player.checkUnlocked(b.getFaction());
	}
}

const std::vector<Faction> &GameManager::getFactions() const
{
	return this->_factions;
}

const LoadedMap &GameManager::getLoadedMap() const
{
	return this->_map;
}

Vector2i MapState::absToTileCoord(const Vector2i &p)
{
	return {
		p.x / static_cast<int>(this->tileSize.x),
		p.y / static_cast<int>(this->tileSize.y)
	};
}

MapState::Quad MapState::tileCoordToAbs(const Vector2i &)
{
	assert_not_reached_msg("Not implemented");
}

void LoadedMap::load(const std::filesystem::path &path)
{
	std::ifstream streamMap{path / "map.json"};
	nlohmann::json json;

	streamMap >> json;
	this->grid.gridSize.x = json["size"]["x"];
	this->grid.gridSize.y = json["size"]["y"];
	this->grid.tileSize.x = json["grid"]["x"];
	this->grid.tileSize.y = json["grid"]["y"];
	this->grid.translate.x = json["translate"]["x"];
	this->grid.translate.y = json["translate"]["y"];
	for (auto &v : json["starts"])
		this->grid.startAreas.emplace_back(v["x"].get<unsigned>(), v["y"].get<unsigned>());
	this->grid.grid.resize(this->grid.gridSize.x * this->grid.gridSize.y);
	this->originalIndices.resize(this->grid.gridSize.x * this->grid.gridSize.y);
	assert_exp(this->renderCache.resize({
		this->grid.tileSize.x * this->grid.gridSize.x,
		this->grid.tileSize.y * this->grid.gridSize.y
	}));
	assert_exp(this->tileSet.loadFromFile(path / json["tilesheet"].get<std::string>()));

	std::filesystem::path attributesPath = path / json["tileattrs"].get<std::string>();
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

	for (size_t i = 0; i < this->grid.grid.size(); i++)
		this->grid.grid[i] = attributes[this->originalIndices[i] & 0xFFFFFF];
	this->buildCache();
}

void LoadedMap::buildCache()
{
	sf::Sprite sprite{this->tileSet};
	auto textureSize = this->tileSet.getSize();

	textureSize.x /= this->grid.tileSize.x;
	this->renderCache.clear(sf::Color::Transparent);
	for (unsigned y = 0; y < this->grid.gridSize.y; y++)
		for (unsigned x = 0; x < this->grid.gridSize.x; x++) {
			size_t index = this->displayedIndices[x + y * this->grid.gridSize.x] & 0xFFFFFF;

			sprite.setPosition({
				static_cast<float>(x * this->grid.tileSize.x),
				static_cast<float>(y * this->grid.tileSize.y)
			});
			sprite.setTextureRect({
				sf::Vector2i(
					index % textureSize.x * this->grid.tileSize.x,
					index / textureSize.x * this->grid.tileSize.y
				),
				sf::Vector2i(this->grid.tileSize)
			});
			this->renderCache.draw(sprite);
		}
	this->renderCache.display();
}
