//
// Created by PinkySmile on 07/04/2026.
//

#include "GameManager.hpp"
#include "Faction.hpp"
#include "Building.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "Player.hpp"
#include "Mod.hpp"
#include "UI.hpp"
#include "Resources/Assert.hpp"

#define GRID_SIZE 32
#define GRID_DIMS_X 40
#define GRID_DIMS_Y 20

GameManager::GameManager(const std::vector<Faction> &factions) :
	_factions(factions)
{
	for (auto &f : this->_factions) {
		f.load();
		this->_mapStates.emplace_back();
	}
}

void GameManager::loadMap()
{
	// TODO:
	for (auto &state : this->_mapStates) {
		state.tileSize = {GRID_SIZE, GRID_SIZE};
		state.gridSize = {GRID_DIMS_X, GRID_DIMS_Y};
		state.grid = new uint32_t[GRID_DIMS_X * GRID_DIMS_Y];
		for (size_t i = 0; i < GRID_DIMS_X * GRID_DIMS_Y; i++)
			state.grid[i] = ~GRIDMASK_HAS_CONCRETE;
		state.startAreas.emplace_back(1, 2);
	}
}

void GameManager::addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea)
{
	this->_players.emplace_back(*this, this->_factions, factionIndex, color, this->_mapStates);
	// TODO:
	this->spawnBuilding(0, {0, 1}, "d2k:building", "d2k:a_cony", false);
	this->_players[0].state.money["base:credits"] = 10000;
	this->_players[0].state.systemMoney["base:credits"] = 10000;
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

			uint32_t &tile = map.grid[x + pos.x + (y + pos.y) * GRID_DIMS_X];

			if (
				(
					(line[x] & Building::MASK_PLACEMENT_CONCRETE) ||
					((line[x] & Building::MASK_PLACEMENT_REQUIRED) && forceConcrete)
				) &&
				(tile & GRIDMASK_IS_FREE)
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
