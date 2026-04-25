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

extern float guiScale;

static bool isMouseOverUI()
{
	Vector2i pos = sf::Mouse::getPosition(*game->screen);

	pos.x /= guiScale;
	pos.y /= guiScale;
	for (auto widget : game->gui.getWidgets()) {
		if (!widget->isVisible())
			continue;
		if (auto container = widget->cast<tgui::Container>())
			for (auto w : container->getWidgets())
				if (w->isMouseOnWidget(pos))
					return true;
	}
	return false;
}

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

void GameManager::addPlayer(size_t factionIndex, const HSLColor &color, unsigned startingArea, unsigned team)
{
	size_t index = this->_players.size();
	auto &faction = this->_factions[factionIndex];

	while (team >= this->_map.grid.placementGrid.size()) {
		this->_map.grid.placementGrid.emplace_back(this->_map.grid.grid.size(), 0);
		for (auto &m: this->_mapStates)
			m.placementGrid.emplace_back(m.grid.size(), 0);
	}
	this->_players.emplace_back(*this, this->_factions, factionIndex, color, this->_mapStates);
	this->_players[index].state.team = team;
	this->_players[index].state.systemMoney = faction.getStartingMoney();
	for (auto &b : faction.getStartingBuildings()) {
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		start += b.offset;
		this->spawnBuilding(index, start, b.queue, b.name, false);
	}
	for (auto &u : faction.getStartingUnits()) {
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		start += u.offset;
		this->spawnUnit(index, start, u.queue, u.name, false);
	}
	// FIXME: Always teleport to player 0 starting point
	if (index == 0) {
		Vector2u size = game->screen->getSize();
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		// TODO: Hardcoded UI offset
		size.x -= 160 * guiScale;
		size.y -= 20 * guiScale;
		// size /= this->_cameraZoom;
		this->_cameraZoom = 1;
		this->_cameraTranslate = {
			(start.x + 0.5f) * this->_map.grid.tileSize.x - size.x / 2,
			(start.y + 0.5f) * this->_map.grid.tileSize.y - size.y / 2 + 20 * guiScale
		};
		if (this->_cameraTranslate.x < 0)
			this->_cameraTranslate.x = 0;
		if (this->_cameraTranslate.y < 0)
			this->_cameraTranslate.y = 0;
		if (this->_cameraTranslate.x >= this->_map.grid.gridSize.x * this->_map.grid.tileSize.x - size.x)
			this->_cameraTranslate.x = 0;
		if (this->_cameraTranslate.y >= this->_map.grid.gridSize.y * this->_map.grid.tileSize.y - size.y)
			this->_cameraTranslate.y = 0;
	}
}

#define SCROLL_SPEED 8

void GameManager::_checkScroll()
{
	Vector2u size = game->screen->getSize();
	auto pos = sf::Mouse::getPosition(*game->screen);
	std::string name = "scroll_";
	auto old = this->_cameraTranslate;

	if (pos.y == 0) {
		name += "t";
		this->_cameraTranslate.y -= SCROLL_SPEED;
	} else if (pos.y == static_cast<int>(size.y) - 1) {
		name += "b";
		this->_cameraTranslate.y += SCROLL_SPEED;
	}
	if (pos.x == 0) {
		name += "l";
		this->_cameraTranslate.x -= SCROLL_SPEED;
	} else if (pos.x == static_cast<int>(size.x) - 1) {
		name += "r";
		this->_cameraTranslate.x += SCROLL_SPEED;
	}

	size /= this->_cameraZoom;
	// TODO: Hardcoded UI offset
	size.x -= 160 * guiScale;
	this->_cameraTranslate.x = std::max<float>(this->_cameraTranslate.x, 0);
	this->_cameraTranslate.y = std::max<float>(this->_cameraTranslate.y, -20 * guiScale);
	this->_cameraTranslate.x = std::min<float>(this->_cameraTranslate.x, this->_map.grid.gridSize.x * this->_map.grid.tileSize.x - size.x);
	this->_cameraTranslate.y = std::min<float>(this->_cameraTranslate.y, this->_map.grid.gridSize.y * this->_map.grid.tileSize.y - size.y);
	if (name != "scroll_") {
		if (old == this->_cameraTranslate)
			name = "no_" + name;
		for (auto &p : this->_players)
			for (auto &u : p.getUIs())
				u->forcedCursor = name;
	} else {
		for (auto &p : this->_players)
			for (auto &u : p.getUIs())
				u->forcedCursor.reset();
	}
}

void GameManager::update()
{
	this->_checkScroll();
	if (isMouseOverUI())
		for (auto &p: this->_players)
			p.mouseOnUi();
	for (auto &b : this->_liveBuildings)
		b.update();
	for (auto &u : this->_liveUnits)
		u.update();
	for (auto &player : this->_players)
		player.update();
	for (auto &player : this->_players)
		player.handleUiEvents();

	for (auto &list : this->_renderObjects | std::ranges::views::values) {
		auto it = list.begin();

		while (it != list.end())
			if ((*it)->isDisposable())
				it = list.erase(it);
			else
				++it;
	}

	auto itb = this->_liveBuildings.begin();
	while (itb != this->_liveBuildings.end())
		if (itb->isDisposable()) {
			auto &group = itb->getGroup();
			auto &id = itb->getId();

			--itb->owner->state.elementsOwned[group];
			if (group != id)
				--itb->owner->state.elementsOwned[id];
			itb = this->_liveBuildings.erase(itb);
		} else
			++itb;

	auto itu = this->_liveUnits.begin();
	while (itu != this->_liveUnits.end())
		if (itu->isDisposable()) {
			auto &group = itu->getGroup();
			auto &id = itu->getId();

			--itu->owner->state.elementsOwned[group];
			if (group != id)
				--itu->owner->state.elementsOwned[id];
			itu = this->_liveUnits.erase(itu);
		} else
			++itu;
}

void GameManager::updateRender()
{
	for (auto &player : this->_players)
		player.updateRender();
}

void GameManager::render()
{
	sf::View old = game->screen->getView();
	sf::Sprite sprite{this->_map.renderCache.getTexture()};

	game->screen->setView(sf::View{sf::FloatRect{
		this->_cameraTranslate,
		sf::Vector2f{
			old.getSize().x * this->_cameraZoom,
			old.getSize().y * this->_cameraZoom
		}
	}});
	game->screen->draw(sprite);
	for (auto &list : this->_renderObjects | std::ranges::views::values)
		for (auto b : list)
			b->render(0);
	for (auto &list : this->_renderObjects | std::ranges::views::values)
		for (auto b : list)
			b->render(1);
	for (auto &player : this->_players)
		player.render();
	game->screen->setView(old);
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

void GameManager::consumeEvent(const Event &event)
{
	if (auto mouseMove = event.getIf<sf::Event::MouseMoved>()) {
		Vector2i pos = mouseMove->position;

		pos /= this->_cameraZoom;
		pos += this->_cameraTranslate;
		if (!isMouseOverUI()) {
			for (auto &p : this->_players)
				p.onMouseMoved(pos);
		}
	} else if (auto mouseDown = event.getIf<sf::Event::MouseButtonPressed>()) {
		Vector2i pos = mouseDown->position;

		pos /= this->_cameraZoom;
		pos += this->_cameraTranslate;
		if (mouseDown->button == sf::Mouse::Button::Left) {
			if (!isMouseOverUI()) {
				for (auto &p: this->_players)
					p.onLeftMouseClicked(pos);
			}
		} else if (mouseDown->button == sf::Mouse::Button::Right) {
			if (!isMouseOverUI()) {
				for (auto &p : this->_players)
					p.onRightMouseClicked(pos);
			}
		}
	}
}

bool GameManager::canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &buildings = player.getBuildings();
	assert_msg(buildings.contains(id), id);
	auto &base = *buildings.at(id);
	auto &tiles = base.getTileData();
	Vector2i pos = map.absToTileCoord(absPos);

	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < map.gridSize.x && static_cast<unsigned>(pos.y) < map.gridSize.y);
	for (size_t y = 0; y < tiles.size(); y++) {
		auto &line = tiles[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};

			if (line[x] & Building::MASK_PLACEMENT_REQUIRED)
				if (!map.hasAllGridBits(fullPos, GRIDMASK_CAN_BUILD))
					return false;
		}
	}
	return true;
}

Building *GameManager::spawnBuilding(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, bool forceConcrete)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &buildings = player.getBuildings();
	assert_msg(buildings.contains(id), id);
	auto &base = *buildings.at(id);
	auto &tiles = base.getTileData();
	auto proxy = base.getProxy();
	unsigned nbConcreteTiles = 0;
	unsigned nbTiles = 0;
	auto &concBase = *buildings.at(player.getPrimaryFaction().getBase().getName() + ":conc");
	Vector2i pos = map.absToTileCoord(absPos);

	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < map.gridSize.x && static_cast<unsigned>(pos.y) < map.gridSize.y);
	for (size_t y = 0; y < tiles.size(); y++) {
		auto &line = tiles[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};
			auto topLeftAbs = map.topLeftCoordToAbs(fullPos);
			auto fullPosAbs = map.tileCoordToAbs(fullPos);

			if (
				(
					(line[x] & Building::MASK_PLACEMENT_CONCRETE) ||
					((line[x] & Building::MASK_PLACEMENT_REQUIRED) && forceConcrete)
				) &&
				// OPTIMIZE: Can use a reference instead of calling helpers
				map.hasAllGridBits(fullPos, GRIDMASK_CAN_BUILD)
			) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.setGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
				for (auto &m : this->_mapStates)
					m.setGridBits(fullPosAbs, GRIDMASK_HAS_CONCRETE);
				this->_liveBuildings.emplace_back(concBase);
				auto &b = this->_liveBuildings.back();
				b.manager = this;
				b.owner = &player;
				b.setPosition(topLeftAbs);
				b.setColor(player.getColor());
				b.built();
				player.state.myObjects.emplace_back(&b);
				this->_renderObjects[pos.y + y].push_front(&b);
				this->_map.grid.updatePlacementGrid(fullPos, player.state.team, 1);
				for (auto &m : this->_mapStates)
					m.updatePlacementGrid(fullPosAbs, player.state.team, 1);
			}
			if (line[x] & Building::MASK_PLACEMENT_REQUIRED) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.resetGridBits(fullPos, GRIDMASK_IS_FREE);
				for (auto &m : this->_mapStates)
					m.resetGridBits(fullPosAbs, GRIDMASK_IS_FREE);
			}
			if (line[x] & Building::MASK_SOLID) {
				nbTiles++;
				// OPTIMIZE: Can use a reference instead of calling helpers
				nbConcreteTiles += map.hasAnyGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
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
		b.owner = &player;
		b.manager = this;
		b.setPosition(map.topLeftCoordToAbs(pos));
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
		player.state.myObjects.emplace_back(&b);
		this->_renderObjects[tiles.size() + pos.y - 1].push_back(&b);

		for (size_t y = 0; y < tiles.size(); y++) {
			auto &line = tiles[y];

			if (y + pos.y >= map.gridSize.y)
				break;
			for (size_t x = 0; x < line.size(); x++) {
				if (x + pos.x >= map.gridSize.x)
					break;
				if (line[x] & Building::MASK_SOLID)
					player.state.objectHitboxes[Vector2u(pos.x + x, pos.y + y)].push_back(&b);
			}
		}
		player.checkUnlocked(b.getFaction());
		return &b;
	}
	return nullptr;
}

Unit *GameManager::spawnUnit(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, bool forceConcrete)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &units = player.getUnits();
	auto &base = *units.at(id);
	Vector2i pos = map.absToTileCoord(absPos);

	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < map.gridSize.x && static_cast<unsigned>(pos.y) < map.gridSize.y);
	this->_liveUnits.push_back(base);
	auto &u = this->_liveUnits.back();
	u.manager = this;
	u.owner = &player;
	u.setPosition(map.topLeftCoordToAbs(pos));
	u.setColor(player.getColor());

	auto group = u.getGroup();

	++player.state.elementsOwned[group];
	if (group != id)
		++player.state.elementsOwned[id];

	auto &parent = player.state.primaryBuildings[buildQueue];

	if (parent)
		parent->produced();
	else if (player.state.queueBuildings.contains(buildQueue))
		for (auto &p: player.state.queueBuildings[buildQueue])
			if (!p->isObstructed()) {
				p->produced();
				break;
			}
	player.state.myObjects.emplace_back(&u);
	this->_renderObjects[pos.y].push_back(&u);
	player.state.objectHitboxes[Vector2u(pos.x, pos.y)].push_back(&u);
	player.checkUnlocked(u.getFaction());
	return &u;
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

Vector2i MapState::topLeftCoordToAbs(const Vector2i &p)
{
	return {static_cast<int>(p.x * this->tileSize.x), static_cast<int>(p.y * this->tileSize.y)};
}

MapState::Quad MapState::tileCoordToAbs(const Vector2i &p)
{
	return {
		{static_cast<float>(p.x)     * this->tileSize.x, static_cast<float>(p.y)     * this->tileSize.y},
		{static_cast<float>(p.x + 1) * this->tileSize.x, static_cast<float>(p.y)     * this->tileSize.y},
		{static_cast<float>(p.x + 1) * this->tileSize.x, static_cast<float>(p.y + 1) * this->tileSize.y},
		{static_cast<float>(p.x)     * this->tileSize.x, static_cast<float>(p.y + 1) * this->tileSize.y}
	};
}

void MapState::clampedPosition(const MapState::Quad &abs, Vector2i &topLeft, Vector2i &bottomRight)
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

void MapState::updatePlacementGrid(const Vector2i &rel, unsigned int player, int delta)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));

	auto &b = this->placementGrid[player][rel.x + rel.y * this->gridSize.x];

	assert_exp(delta < 0 || b < 256 - delta);
	assert_exp(delta > 0 || b >= delta);
	b += delta;
}

void MapState::updatePlacementGrid(const MapState::Quad &abs, unsigned int player, int delta)
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

void MapState::setGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	this->grid[rel.x + rel.y * this->gridSize.x] |= bits;
}

void MapState::setGridBits(const MapState::Quad &abs, uint32_t bits)
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			this->grid[x + y * this->gridSize.x] |= bits;
}

void MapState::resetGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	this->grid[rel.x + rel.y * this->gridSize.x] &= ~bits;
}

void MapState::resetGridBits(const MapState::Quad &abs, uint32_t bits)
{
	Vector2i topLeft;
	Vector2i bottomRight;

	this->clampedPosition(abs, topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++)
			this->grid[x + y * this->gridSize.x] &= ~bits;
}

bool MapState::hasAnyGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) != 0;
}

bool MapState::hasAllGridBits(const Vector2i &rel, uint32_t bits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) == bits;
}

bool MapState::hasSomeGridBits(const Vector2i &rel, uint32_t bits, uint32_t expectedBits)
{
	assert_msg(rel.x >= 0 && static_cast<unsigned>(rel.x) < this->gridSize.x, std::to_string(rel.x));
	assert_msg(rel.y >= 0 && static_cast<unsigned>(rel.y) < this->gridSize.y, std::to_string(rel.y));
	return (this->grid[rel.x + rel.y * this->gridSize.x] & bits) == expectedBits;
}

bool MapState::anyHasAnyGridBits(const MapState::Quad &abs, uint32_t bits)
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

bool MapState::anyHasAllGridBits(const MapState::Quad &abs, uint32_t bits)
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

bool MapState::anyHasSomeGridBits(const MapState::Quad &abs, uint32_t bits, uint32_t expected)
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

bool MapState::allHasAnyGridBits(const MapState::Quad &abs, uint32_t bits)
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

bool MapState::allHasAllGridBits(const MapState::Quad &abs, uint32_t bits)
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

bool MapState::allHasSomeGridBits(const MapState::Quad &abs, uint32_t bits, uint32_t expected)
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
