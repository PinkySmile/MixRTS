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
#ifndef PROPAGATE_CRASH
	try {
#endif
		this->_map.load(mod, path);
#ifndef PROPAGATE_CRASH
	} catch (std::exception &e) {
		throw _AssertionFailedException("While loading map \"" + path.string() + "\"\n" + e.what());
	}
#endif

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

	// OPTIMIZE: Split map into quadrants
	//size_t x = std::ceil(this->_map.grid.gridSize.x * this->_map.grid.tileSize.x / static_cast<float>(QUADRANT_SIZE_X));
	//size_t y = std::ceil(this->_map.grid.gridSize.y * this->_map.grid.tileSize.y / static_cast<float>(QUADRANT_SIZE_Y));

	//this->_map.quadrants.resize(x * y);
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
		this->spawnBuilding(index, start, b.queue, b.name);
	}
	for (auto &u : faction.getStartingUnits()) {
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		start += u.offset;
		this->spawnUnit(index, start, u.queue, u.name);
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

void GameManager::_renderPathFindingGrid(PathFindingGrid &grid, Vector2u scale, bool drawGrid)
{
	if (grid.textureOutOfSync) {
		assert_exp(grid.gridImageTexture.loadFromImage(grid.gridImage));
		grid.textureOutOfSync = false;
	}

	sf::Sprite sprite{grid.gridImageTexture};

	sprite.setScale(scale);
	game->screen->draw(sprite);
	for (auto &b : this->_liveBuildings)
		b.renderPathFindingPath();
	for (auto &b : this->_liveUnits)
		b.renderPathFindingPath();

	if (!drawGrid)
		return;

	sf::View view = game->screen->getView();
	Vector2f size = view.getSize();
	Vector2f pos = Vector2f(view.getCenter()) - size / 2;
	Vector2f end = pos + size;
	sf::RectangleShape shape;

	pos.x -= fmod(pos.x, scale.x);
	pos.y -= fmod(pos.y, scale.y);
	shape.setOutlineThickness(1);
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineColor(sf::Color::Black);
	shape.setSize(scale - Vector2u{2, 2});

	auto start = pos.x;
	size_t y = pos.y / scale.y;
	size_t x;

	while (pos.y < end.y && y < grid.gridSize.y) {
		pos.x = start;
		x = pos.x / scale.x;
		while (pos.x < end.x && x < grid.gridSize.x) {
			shape.setPosition(pos + Vector2f{1, 1});
			game->screen->draw(shape);
			pos.x += scale.x;
			x++;
		}
		pos.y += scale.y;
		y++;
	}
}

static void displayTile(uint32_t tile, sf::RectangleShape &shape)
{
	Color color;

	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 128;
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
	shape.setFillColor(color);
	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 255;
	if (tile & GRIDMASK_BUILDABLE) {
		if (tile & GRIDMASK_HAS_CONCRETE) {
			color.g = 255;
		} else {
			color.r = 255;
			color.g = 128;
		}
	}
	shape.setOutlineColor(color);
	if (!(tile & GRIDMASK_IS_FREE)) {
		sf::VertexArray array{sf::PrimitiveType::TriangleStrip, 6};

		array[0].position = shape.getPosition();
		array[0].color = sf::Color::Black;
		array[5].position = Vector2f(shape.getPosition()) + Vector2f(shape.getSize());
		array[5].color = sf::Color::Black;

		array[1] = array[0];
		array[1].position.x += 2;
		array[2] = array[0];
		array[2].position.y += 2;

		array[3] = array[5];
		array[3].position.y -= 2;
		array[4] = array[5];
		array[4].position.x -= 2;

		game->screen->draw(array);
	}
	game->screen->draw(shape);
}

static void displayGrid(const MapState &state)
{
	sf::View view = game->screen->getView();
	Vector2f size = view.getSize();
	Vector2f pos = Vector2f(view.getCenter()) - size / 2;
	Vector2f end = pos + size;
	sf::RectangleShape shape;

	pos.x -= fmod(pos.x, state.tileSize.x);
	pos.y -= fmod(pos.y, state.tileSize.y);
	shape.setOutlineThickness(1);
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineColor(sf::Color::Black);
	shape.setSize(state.tileSize - Vector2u{2, 2});

	auto start = pos.x;
	size_t y = pos.y / state.tileSize.y;
	size_t x;

	while (pos.y < end.y && y < state.gridSize.y) {
		pos.x = start;
		x = pos.x / state.tileSize.x;
		while (pos.x < end.x && x < state.gridSize.x) {
			shape.setPosition(pos + Vector2f{1, 1});
			displayTile(state.grid[x + y * state.gridSize.x], shape);
			pos.x += state.tileSize.x;
			x++;
		}
		pos.y += state.tileSize.y;
		y++;
	}
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
	if (this->_debugGrid == 0)
		this->_renderPathFindingGrid(this->_map.pathFindingGrid);
	else if (this->_debugGrid == 1)
		displayGrid(this->_map.grid);
	else if (this->_debugGrid == 2)
		this->_renderPathFindingGrid(this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, true);
	else if (this->_debugGrid != 255) {
		if ((this->_debugGrid - 3) % 2 == 1) {
			auto &map = this->_mapStates[(this->_debugGrid - 3) / 2];

			this->_renderPathFindingGrid(map.pathFindingGrid, map.tileSize, true);
		} else
			displayGrid(this->_mapStates[(this->_debugGrid - 3) / 2]);
	}
	for (auto &player : this->_players)
		player.render();
	game->screen->setView(old);
}

void GameManager::renderAbove()
{
	for (auto &player : this->_players)
		player.renderAbove();
	if (this->_debugGrid != 255) {
		sf::Text text{game->font};
		sf::View old = game->screen->getView();
		sf::View v = sf::View{sf::FloatRect{
			{0, 0},
			{
				static_cast<float>(game->screen->getSize().x),
				static_cast<float>(game->screen->getSize().y)
			}
		}};

		text.setCharacterSize(12 * guiScale);
		text.setOutlineThickness(1);
		text.setOutlineColor(sf::Color::Black);
		text.setPosition({2, 20 * guiScale});
		if (this->_debugGrid == 0)
			text.setString("Path finding grid");
		else if (this->_debugGrid == 1)
			text.setString("Original map grid");
		else if (this->_debugGrid == 2)
			text.setString("Original map grid (path finding)");
		else if ((this->_debugGrid - 3) % 2 == 0)
			text.setString("Map for " + this->_factions[(this->_debugGrid - 3) / 2].getName());
		else
			text.setString("Map for " + this->_factions[(this->_debugGrid - 3) / 2].getName() + " (path finding)");
		game->screen->setView(v);
		game->screen->draw(text);
		game->screen->setView(old);
	}
}

std::vector<Player> &GameManager::getPlayers()
{
	return this->_players;
}

void GameManager::_unlockAll()
{
	for (auto &player : this->_players) {
		for (auto &f : this->_factions) {
			for (auto &[name, b] : f.getBuildings())
				player.unlockObject(f, name, b, true);
			for (auto &[name, u] : f.getUpgrades())
				player.unlockObject(f, name, u, true);
			for (auto &[name, u] : f.getUnits())
				player.unlockObject(f, name, u, true);
		}
		for (auto &[name, q] : player.state.buildQueues)
			player.resetQueue(name);
	}
}

template<typename T>
void GameManager::_checkObjectLocked(const std::string &name, T &object, Player &owner)
{
	auto &bqueue = object.getBuildQueue();

	if (!bqueue)
		return;

	auto &q = owner.state.queues[*bqueue][name];
	auto &req = object.getRequirements();

	if (
		(object.requiresQueueBuilding() && owner.state.queueBuildings[*bqueue].empty()) ||
		!req(owner.state.elementsOwned)
	) {
		q.unlocked = false;
		if (q.buildTime != 0) {
			for (auto &p : q.spent)
				owner.credit(p.first, p.second, Player::CREDIT_SYSTEM);
			owner.resetQueue(*bqueue);
		}
	}
	q.buildTime = object.getBuildSpeed();
}

void GameManager::_recomputeLocked()
{
	for (auto &player : this->_players)
		for (auto &f : this->_factions) {
			for (auto &[name, b] : f.getBuildings())
				this->_checkObjectLocked(name, b, player);
			for (auto &[name, u] : f.getUpgrades())
				this->_checkObjectLocked(name, u, player);
			for (auto &[name, u] : f.getUnits())
				this->_checkObjectLocked(name, u, player);
		}
}

void GameManager::consumeEvent(const Event &event)
{
	if (auto keyPressed = event.getIf<sf::Event::KeyPressed>()) {
	#ifdef _DEBUG
		if (keyPressed->code == sf::Keyboard::Key::F1) {
			if (this->_debugGrid == this->_mapStates.size() * 2 + 3 - 1)
				this->_debugGrid = 255;
			else
				this->_debugGrid++;
		} else if (keyPressed->code == sf::Keyboard::Key::F2) {
			if (this->_debugGrid == 255)
				this->_debugGrid = this->_mapStates.size() * 2 + 3 - 1;
			else
				this->_debugGrid--;
		} else if (keyPressed->code == sf::Keyboard::Key::F3) {
			if (this->_isDebugUnlocked)
				this->_recomputeLocked();
			else
				this->_unlockAll();
			this->_isDebugUnlocked = !this->_isDebugUnlocked;
			DEBUG_MODE = this->_isDebugUnlocked;
		}
	#endif
	} else if (auto mouseMove = event.getIf<sf::Event::MouseMoved>()) {
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

bool GameManager::canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id, const Vector2i &basePosAbs)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &buildings = player.getBuildings();
	assert_msg(buildings.contains(id), id);
	auto &base = *buildings.at(id);
	auto &tiles = base.getTileData();
	Vector2i pos = map.absToTileCoord(absPos);
	Vector2i basePos = map.absToTileCoord(basePosAbs);

	if (!tiles)
		return this->isAreaFree(absPos, base.getSize());

	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < map.gridSize.x && static_cast<unsigned>(pos.y) < map.gridSize.y);
	for (size_t y = 0; y < tiles->size(); y++) {
		auto &line = (*tiles)[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};

			if ((line[x] & GameObject::MASK_PLACEMENT_REQUIRED)) {
				uint32_t mask = GRIDMASK_CAN_BUILD;

				if (basePos == fullPos)
					mask &= ~GRIDMASK_IS_FREE;
				else if (!this->isAreaFree(map.topLeftCoordToAbs(fullPos), map.tileSize))
					return false;
				if (!map.hasAllGridBits(fullPos, mask))
					return false;
			}
		}
	}
	return true;
}

void GameManager::_pushOrder(GameObject &obj, const std::string &order)
{
	if (order == "harvest")
		obj.harvest();
	else
		assert_not_reached_msg("Unknown order '" + order + "'");
}

void GameManager::_onObjectSpawned(Vector2i pos, MapState &map, Player &owner, GameObject &obj, const std::string &id)
{
	auto topLeftAbs = map.topLeftCoordToAbs(pos);
	auto fullPosAbs = map.tileCoordToAbs(pos);
	auto group = obj.getGroup();

	obj.manager = this;
	obj.owner = &owner;
	obj.map = &map;
	obj.pfGrid = &this->_map.pathFindingGrid;
	obj.setIndex(this->_lastObjectId);
	obj.setPosition(topLeftAbs);
	obj.setColor(owner.getColor());
	obj.built();

	++owner.state.elementsOwned[group];
	if (group != id)
		++owner.state.elementsOwned[id];
	owner.state.myObjects.emplace_back(&obj);

	this->_objectMap[this->_lastObjectId++] = &obj;
	this->_map.grid.updatePlacementGrid(fullPosAbs, owner.state.team, 1);
	for (auto &m : this->_mapStates)
		m.updatePlacementGrid(fullPosAbs, owner.state.team, 1);
	for (auto &order : obj.getSpawnOrders())
		this->_pushOrder(obj, order);

	auto extra = obj.getExtraSpawn();

	if (extra)
		this->spawnUnit(owner.state.index, obj.getPosition() + extra->pos, "", extra->unit);
	// FIXME: Dynamic cast
	auto b = dynamic_cast<Building *>(&obj);
	if (b) {
		auto &gqueue = b->getGivenQueue();

		if (gqueue)
			owner.state.queueBuildings[*gqueue].push_back(b);
	}
	owner.checkUnlocked(obj.getFaction());
}

template<typename T>
T *GameManager::_spawnObj(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id, std::list<T> &out, const std::unordered_map<std::string, const T *> &in)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[owner];
	auto &buildings = player.getBuildings();
	std::string concrete = player.getPrimaryFaction().getBase().getName() + ":conc";
	assert_msg(in.contains(id), id);
	assert_msg(buildings.contains(concrete), concrete);
	auto &base = *in.at(id);
	auto &concBase = *buildings.at(concrete);
	auto &tiles = base.getTileData();
	unsigned nbConcreteTiles = 0;
	unsigned nbTiles = 0;
	Vector2i pos = map.absToTileCoord(absPos);

	if (!tiles)
		goto noGrid;
	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < map.gridSize.x && static_cast<unsigned>(pos.y) < map.gridSize.y);
	for (size_t y = 0; y < tiles->size(); y++) {
		auto &line = (*tiles)[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};
			auto fullPosAbs = map.tileCoordToAbs(fullPos);

			if (
				(line[x] & GameObject::MASK_PLACEMENT_CONCRETE) &&
				// OPTIMIZE: Can use a reference instead of calling helpers
				map.hasAllGridBits(fullPos, GRIDMASK_CAN_BUILD) &&
				this->isAreaFree(map.topLeftCoordToAbs(fullPos), map.tileSize)
			) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.setGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
				for (auto &m : this->_mapStates)
					m.setGridBits(fullPosAbs, GRIDMASK_HAS_CONCRETE);
				this->_liveBuildings.emplace_back(concBase);
				this->_onObjectSpawned(fullPos, map, player, this->_liveBuildings.back(), concrete);
			}
			if (line[x] & GameObject::MASK_PLACEMENT_REQUIRED) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.resetGridBits(fullPos, GRIDMASK_IS_FREE);
				for (auto &m : this->_mapStates)
					m.resetGridBits(fullPosAbs, GRIDMASK_IS_FREE);
			}
			if (line[x] & GameObject::MASK_SOLID) {
				nbTiles++;
				// OPTIMIZE: Can use a reference instead of calling helpers
				nbConcreteTiles += map.hasAnyGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
			}
		}
	}

noGrid:
	auto proxy = base.getProxy();

	if (proxy)
		assert_msg(in.contains(*proxy), *proxy);
	base.playBuiltSound();
	if (!base.isFake()) {
		if (proxy)
			out.push_back(*in.at(*proxy));
		else
			out.push_back(base);

		auto &obj = out.back();

		if (nbTiles != nbConcreteTiles)
			obj.damage(obj.getHealth() * (nbTiles - nbConcreteTiles) / (nbTiles * 2));

		auto parent = player.getSpawnBuilding(buildQueue);

		if (parent) {
			auto exit = parent->getExitOffset();
			auto spawn = parent->getSpawnOffset();

			if (spawn) {
				this->_onObjectSpawned(map.absToTileCoord(*spawn + parent->getPosition()), map, player, obj, id);
				if (exit) {
					auto next = *exit + parent->getPosition();

					obj.moveTo(next, true);
					obj.setDirection(obj.getPosition().angle(next));
				}
				return &obj;
			}
		}
		this->_onObjectSpawned(pos, map, player, obj, id);
		if constexpr (std::is_same_v<T, Building>) {
			auto &gqueue = obj.getGivenQueue();
			auto &bqueue = obj.getBuildQueue();

			if (gqueue && bqueue && !parent && *gqueue == *bqueue)
				obj.produced();
		}
		return &obj;
	}
	return nullptr;
}

Building *GameManager::spawnBuilding(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id)
{
	Player &p = this->_players[owner];
	Building *b = this->_spawnObj(owner, absPos, buildQueue, id, this->_liveBuildings, p.getBuildings());

	if (b) {
		auto power = b->getPower();
		auto &storage = b->getStorage();

		if (power < 0)
			p.state.producedPower += -power;
		else
			p.state.consumedPower += power;
		for (auto &pair : storage)
			p.state.totalStorage[pair.first] += pair.second;
	}
	return b;
}

Unit *GameManager::spawnUnit(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id)
{
	return this->_spawnObj(owner, absPos, buildQueue, id, this->_liveUnits, this->_players[owner].getUnits());
}

const std::vector<Faction> &GameManager::getFactions() const
{
	return this->_factions;
}

const LoadedMap &GameManager::getLoadedMap() const
{
	return this->_map;
}

bool GameManager::_isInArea(const GameObject &obj, const Vector2i &absPos, const Vector2u &size) const
{
	if (!obj.isSolid())
		return false;

	auto objPos = obj.getPosition();
	auto &tiles = obj.getTileData();

	if (!tiles) {
		auto s = obj.getSize();

		if (absPos.x + static_cast<int>(size.x) <= objPos.x)
			return false;
		if (absPos.y + static_cast<int>(size.y) <= objPos.y)
			return false;
		if (absPos.x >= objPos.x + static_cast<int>(s.x))
			return false;
		if (absPos.y >= objPos.y + static_cast<int>(s.y))
			return false;
		return true;
	}

	Vector2i pos = obj.map->absToTileCoord(objPos);
	std::set<std::pair<Vector2i, Vector2u>> points;

	for (size_t y = 0; y < tiles->size(); y++) {
		auto &line = (*tiles)[y];

		if (y + pos.y >= obj.map->gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= obj.map->gridSize.x)
				break;
			if ((line[x] & GameObject::MASK_SOLID) == 0)
				continue;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};
			auto fullPosAbs = obj.map->topLeftCoordToAbs(fullPos);

			points.emplace(fullPosAbs, obj.map->tileSize);
		}
	}
	for (auto &[p, s] : points) {
		if (absPos.x + static_cast<int>(size.x) <= p.x)
			continue;
		if (absPos.y + static_cast<int>(size.y) <= p.y)
			continue;
		if (absPos.x >= p.x + static_cast<int>(s.x))
			continue;
		if (absPos.y >= p.y + static_cast<int>(s.y))
			continue;
		return true;
	}
	return false;
}

bool GameManager::isAreaFree(const Vector2i &absPos, const Vector2u &size)
{
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveBuildings)
		if (this->_isInArea(b, absPos, size))
			return false;
	for (auto &u : this->_liveUnits)
		if (this->_isInArea(u, absPos, size))
			return false;
	return true;
}

void GameManager::getObjectsInArea(const Vector2i &absPos, const Vector2u &size, std::vector<GameObject *> &objects)
{
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveBuildings)
		if (this->_isInArea(b, absPos, size))
			objects.push_back(&b);
	for (auto &u : this->_liveUnits)
		if (this->_isInArea(u, absPos, size))
			objects.push_back(&u);
}

void GameManager::registerObject(GameObject *o)
{
	auto &l = this->_renderObjects[o->getBottom()];

	assert_exp(std::find(l.begin(), l.end(), o) == l.end());
	l.push_back(o);
}

void GameManager::deregisterObject(GameObject *o)
{
	auto &l = this->_renderObjects[o->getBottom()];
	auto it = std::find(l.begin(), l.end(), o);

	if (it != l.end())
		l.erase(it);
}

void GameManager::_modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, int count)
{
	// TODO: Handle non-square grids
	Vector2i minv = {
		static_cast<int>(floor(std::min(std::min(quad.pt1.x, quad.pt2.x), std::min(quad.pt3.x, quad.pt4.x)) / tileSize.x)),
		static_cast<int>(floor(std::min(std::min(quad.pt1.y, quad.pt2.y), std::min(quad.pt3.y, quad.pt4.y)) / tileSize.y))
	};
	Vector2i maxv = {
		static_cast<int>(ceil(std::max(std::max(quad.pt1.x, quad.pt2.x), std::max(quad.pt3.x, quad.pt4.x)) / tileSize.x)),
		static_cast<int>(ceil(std::max(std::max(quad.pt1.y, quad.pt2.y), std::max(quad.pt3.y, quad.pt4.y)) / tileSize.y))
	};

	for (int y = std::max(minv.y, 0); y < std::min<int>(maxv.y, state.gridSize.y); y++)
		for (int x = std::max(minv.x, 0); x < std::min<int>(maxv.x, state.gridSize.x); x++)
			if (count < 0)
				PFGRIDMASK_DEL_UNIT(state, x, y, -count);
			else
				PFGRIDMASK_ADD_UNIT(state, x, y, count);
}

void GameManager::modUnitInArea(const MapState::Quad &quad, int count)
{
	this->_modUnitInArea(quad, this->_map.pathFindingGrid, {1, 1}, count);
	this->_modUnitInArea(quad, this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, count);
	for (auto &map : this->_mapStates)
		this->_modUnitInArea(quad, map.pathFindingGrid, map.tileSize, count);
}

void GameManager::modUnitInArea(const MapState::Quad &quad, PathFindingGrid &special, Vector2i rel, Vector2u tileSize, int count)
{
	if (count < 0)
		PFGRIDMASK_DEL_UNIT(special, rel.x, rel.y, -count);
	else
		PFGRIDMASK_ADD_UNIT(special, rel.x, rel.y, count);
	this->_modUnitInArea(quad, this->_map.pathFindingGrid, {1, 1}, count);
	this->_modUnitInArea(quad, this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, count);
	for (auto &map : this->_mapStates)
		if (&map.pathFindingGrid != &special)
			this->_modUnitInArea(quad, map.pathFindingGrid, map.tileSize, count);
}

GameObject *GameManager::getObjectById(unsigned int index)
{
	assert_exp(this->_objectMap.contains(index));
	return this->_objectMap[index];
}

std::optional<Vector2i> GameManager::getFirstHarvestableTile(MapState &state, Vector2i pos) const
{
	Vector2i topLeft;
	Vector2i bottomRight;
	auto &grid = this->_map.grid;

#define CHECK_RES(x_, y_) do {                                                                                          \
	if ((x_) < 0 || (y_) < 0 || (x_) >= (int)grid.gridSize.x || (y_) >= (int)grid.gridSize.y)                       \
		break;                                                                                                  \
	auto r = GRID_RESOURCE_TYPE(grid.grid[(x_) + (y_) * grid.gridSize.x]);                                          \
	if (r == 0) break;                                                                                              \
	auto &re = this->_map.metadata.resources.at(r);                                                                 \
	if (re.onlyAdjCollectible.empty()) break;                                                                       \
	if (std::find(re.onlyAdjCollectible.begin(), re.onlyAdjCollectible.end(), tile) == re.onlyAdjCollectible.end()) \
		goto keepGoing;                                                                                         \
} while (false)

	grid.clampedPosition(state.tileCoordToAbs(pos), topLeft, bottomRight);
	for (int y = topLeft.y; y <= bottomRight.y; y++)
		for (int x = topLeft.x; x <= bottomRight.x; x++) {
			auto res = GRID_RESOURCE_TYPE(grid.grid[x + y * grid.gridSize.x]);
			auto tile = this->_map.displayedIndices[x + y * grid.gridSize.x];

			if (res == 0)
				continue;
			CHECK_RES(x - 1, y - 1);
			CHECK_RES(x,     y - 1);
			CHECK_RES(x + 1, y - 1);
			CHECK_RES(x - 1, y);
			CHECK_RES(x + 1, y);
			CHECK_RES(x - 1, y + 1);
			CHECK_RES(x,     y + 1);
			CHECK_RES(x + 1, y + 1);
			return Vector2i{x, y};
		keepGoing:
			continue;
		}
	return {};
}

std::optional<Vector2i> GameManager::getClosestHarvestableTile(MapState &state, Vector2i pos) const
{
	auto &grid = this->_map.grid;
	int i = 1;
	std::optional<Vector2i> result;
	auto cb = [this, &grid, &state](int x, int y) -> std::optional<Vector2i> {
		auto res = GRID_RESOURCE_TYPE(grid.grid[x + y * grid.gridSize.x]);
		auto tile = this->_map.displayedIndices[x + y * grid.gridSize.x];

		if (res == 0)
			return {};
		CHECK_RES(x - 1, y - 1);
		CHECK_RES(x, y - 1);
		CHECK_RES(x + 1, y - 1);
		CHECK_RES(x - 1, y);
		CHECK_RES(x + 1, y);
		CHECK_RES(x - 1, y + 1);
		CHECK_RES(x, y + 1);
		CHECK_RES(x + 1, y + 1);
		return state.absToTileCoord(grid.topLeftCoordToAbs({x, y}));
	keepGoing:
		return {};
	};

	pos = grid.absToTileCoord(state.topLeftCoordToAbs(pos));
	result = cb(pos.x, pos.y);
	if (result) return result;
	while (true) {
		if (pos.x > i && pos.y > i && static_cast<unsigned>(pos.x + i) >= grid.gridSize.x && static_cast<unsigned>(pos.y + i) >= grid.gridSize.y)
			return {};
		if (pos.y >= i)
			for (int j = std::max(0, pos.x - i); j < std::min<int>(pos.x + i, grid.gridSize.x); j++) {
				result = cb(j, pos.y - i);
				if (result) return result;
			}
		if (static_cast<unsigned>(pos.y + i) < grid.gridSize.y)
			for (int j = std::max(0, pos.x - i); j < std::min<int>(pos.x + i, grid.gridSize.x); j++) {
				result = cb(j, pos.y + i);
				if (result) return result;
			}
		if (pos.x >= i)
			for (int j = std::max(0, pos.y - i); j < std::min<int>(pos.y + i, grid.gridSize.y); j++) {
				result = cb(pos.x - i, j);
				if (result) return result;
			}
		if (static_cast<unsigned>(pos.x + i) < grid.gridSize.x)
			for (int j = std::max(0, pos.y - i); j < std::min<int>(pos.y + i, grid.gridSize.y); j++) {
				result = cb(pos.x + i, j);
				if (result) return result;
			}
		i++;
	}
#undef CHECK_RES
}

Building *GameManager::getClosestResourceDump(const std::vector<std::string> &preferredResources, Vector2i pos)
{
	if (this->_liveBuildings.empty())
		return nullptr;

	Building *best = &this->_liveBuildings.front();
	auto dist = pos.distance2(best->getPosition() + best->getPoint() + best->map->tileSize / 2);
	size_t resourceScore = 0;
	auto *accept = &best->getAcceptResources();

	for (auto &r : preferredResources)
		resourceScore += std::ranges::find(*accept, r) != accept->end();
	for (auto &b : this->_liveBuildings) {
		size_t score = 0;
		auto d = pos.distance2(b.getPosition() + b.getPoint() + b.map->tileSize / 2);

		accept = &best->getAcceptResources();
		for (auto &r : preferredResources)
			score += std::ranges::find(*accept, r) != accept->end();
		if (resourceScore > score)
			continue;
		if (resourceScore < score || d < dist) {
			resourceScore = score;
			best = &b;
			dist = d;
			continue;
		}
	}
	return best;
}

MapResource &GameManager::getResource(Vector2i pos)
{
	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < this->_map.grid.gridSize.x && static_cast<unsigned>(pos.y) < this->_map.grid.gridSize.y);
	auto tile = this->_map.grid.grid[pos.x + pos.y * this->_map.grid.gridSize.x];
	assert_exp(GRID_RESOURCE_TYPE(tile));
	assert_exp(this->_map.metadata.resources.contains(GRID_RESOURCE_TYPE(tile)));
	return this->_map.metadata.resources.at(GRID_RESOURCE_TYPE(tile));
}

void GameManager::consumeResource(Vector2i pos)
{
	assert_exp(pos.x >= 0 && pos.y >= 0 && static_cast<unsigned>(pos.x) < this->_map.grid.gridSize.x && static_cast<unsigned>(pos.y) < this->_map.grid.gridSize.y);
	auto &tile = this->_map.grid.grid[pos.x + pos.y * this->_map.grid.gridSize.x];
	assert_exp(GRID_RESOURCE_TYPE(tile));
	if (GRID_RESOURCE_COUNT(tile) != 0) {
		tile -= 1 << 29;
		return;
	}
	assert_exp(this->_map.metadata.resources.contains(GRID_RESOURCE_TYPE(tile)));

	auto &r = this->_map.metadata.resources.at(GRID_RESOURCE_TYPE(tile));

	tile &= 0xFFFFFF;
	if (r.deplete != 0) {
		tile |= r.deplete << 24;
		assert_exp(this->_map.metadata.resources.contains(r.deplete));
		tile |= (this->_map.metadata.resources.at(r.deplete).lots - 1) << 29;
	}
	this->_map.refreshTile(pos.x, pos.y);
	// TODO: Propagate to other maps
}
