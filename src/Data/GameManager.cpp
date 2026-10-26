//
// Created by PinkySmile on 07/04/2026.
//

#include <set>
#include <fstream>
#include <sol/sol.hpp>
#include "GameManager.hpp"
#include "Faction.hpp"
#include "Animation.hpp"
#include "Upgrade.hpp"
#include "Effect.hpp"
#include "Weapon.hpp"
#include "Warhead.hpp"
#include "Projectile.hpp"
#include "Player.hpp"
#include "Mod.hpp"
#include "UI/UI.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"
#include "Debug/Profiler.hpp"
#include "Debug/DebugOverlay.hpp"
#include "Debug/DebugPanel.hpp"

static bool isMouseOverUI()
{
	Vector2i pos = game->screen->getMousePosition();

	pos.x /= guiScale;
	pos.y /= guiScale;
	for (auto widget : game->gui.getWidgets()) {
		if (!widget->isVisible() || widget->getIgnoreMouseEvents())
			continue;
		// Floating windows (debug panel, ...) count fully
		if (widget->getWidgetType() == "ChildWindow") {
			if (widget->isMouseOnWidget(pos))
				return true;
			continue;
		}
		if (auto container = widget->cast<tgui::Container>())
			for (auto w : container->getWidgets())
				if (w->isVisible() && w->isMouseOnWidget(pos + container->getPosition()))
					return true;
	}
	return false;
}

ShakeParams::ShakeParams(const nlohmann::json &j)
{
	assert_exp(j.contains("time") && j["time"].is_number());
	assert_exp(j.contains("power") && j["power"].is_number());
	assert_exp(!j.contains("interval") || j["interval"].is_number());
	assert_exp(!j.contains("freeze_time") || j["freeze_time"].is_boolean());

	this->time = j["time"];
	this->power = j["power"];
	if (j.contains("interval"))
		this->interval = j["interval"];
	if (j.contains("freeze_time"))
		this->freezeTime = j["freeze_time"];
}

GameManager::GameManager(const std::vector<Faction> &factions, size_t nbPlayers, LobbySettings settings) :
	_text{game->font},
	_settings(settings),
	_factions(factions)
{
	std::vector<std::string> allFactions;

	this->_text.setCharacterSize(12 * guiScale);
	this->_text.setOutlineThickness(1);
	this->_text.setOutlineColor(sf::Color::Black);
	this->_text.setPosition({2, 20 * guiScale});
	allFactions.reserve(this->_factions.size());
	for (auto &f : this->_factions)
		allFactions.push_back(f.getName());
	for (auto &f : this->_factions) {
		f.load(allFactions);
		this->_mapStates.emplace_back();
	}
	this->_validateArmors();
	this->_players.reserve(nbPlayers);
	this->_debugOverlay = std::make_unique<DebugOverlay>(*this);
}

GameManager::~GameManager()
{
	for (unsigned handle : this->_preloadedSprites)
		game->textureMgr.remove(handle);
}

void GameManager::_validateArmors() const
{
	std::set<std::string> armors;

	for (auto &f : this->_factions)
		for (auto &object : f.getObjects() | std::ranges::views::values)
			armors.insert(object.getArmor());
	for (auto &f : this->_factions)
		for (auto &[whId, wh] : f.getWarheads())
			for (auto &armor : armors)
				assert_msg(wh.getArmorMultipliers().contains(armor), "Warhead \"" + whId + "\" is missing a multiplier for armor \"" + armor + "\"");
}

void GameManager::loadMap(Mod &mod, const std::filesystem::path &path)
{
	TRY {
		this->_map.load(mod, path);
	} CATCH_AND_RETHROW("While loading map \"" + path.string() + "\"\n")

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

	assert_exp(team < MAX_TEAM_COUNT);
	while (team >= this->_map.grid.placementGrid.size()) {
		this->_map.grid.placementGrid.emplace_back(this->_map.grid.grid.size(), 0);
		for (auto &m: this->_mapStates)
			m.placementGrid.emplace_back(m.grid.size(), 0);
	}
	this->_players.emplace_back(*this, this->_factions, factionIndex, color, this->_mapStates);
	this->_players[index].state.index = index;
	this->_players[index].state.team = team;
	this->_players[index].state.systemMoney = faction.getStartingMoney();
	for (auto &b : faction.getStartingObjects()) {
		auto start = this->_mapStates[factionIndex].startAreas[startingArea];

		if (static_cast<int>(start.x) < -b.offset.x)
			start.x = 0;
		else
			start.x = std::min(start.x + b.offset.x, this->_mapStates[factionIndex].gridSize.x * this->_mapStates[factionIndex].tileSize.x - 1);
		if (static_cast<int>(start.y) < -b.offset.y)
			start.y = 0;
		else
			start.y = std::min(start.y + b.offset.y, this->_mapStates[factionIndex].gridSize.y * this->_mapStates[factionIndex].tileSize.y - 1);
		this->spawnObject(index, start, b.queue, b.name);
	}
	// FIXME: Always teleport to player 0 starting point
	if (index == 0)
		this->centerCameraOn(this->_mapStates[factionIndex].startAreas[startingArea]);
}

#define SCROLL_SPEED 24

void GameManager::_checkScroll()
{
	if (game->paused)
		return;

	Vector2u size = game->screen->getSize();
	auto pos = game->screen->getMousePosition();
	unsigned cursor = UI::CURSOR_SCROLL_ALL_DIRS;
	auto old = this->_cameraTranslate;

	if (pos.y == 0) {
		cursor = UI::CURSOR_SCROLL_TOP;
		this->_cameraTranslate.y -= SCROLL_SPEED;
	} else if (pos.y == static_cast<int>(size.y) - 1) {
		cursor = UI::CURSOR_SCROLL_BOTTOM;
		this->_cameraTranslate.y += SCROLL_SPEED;
	}
	if (pos.x == 0) {
		cursor += UI::CURSOR_HORZ_LEFT;
		this->_cameraTranslate.x -= SCROLL_SPEED;
	} else if (pos.x == static_cast<int>(size.x) - 1) {
		cursor += UI::CURSOR_HORZ_RIGHT;
		this->_cameraTranslate.x += SCROLL_SPEED;
	}

	size /= this->_cameraZoom;
	// TODO: Hardcoded UI offset
	size.x -= 160 * guiScale;
	if (this->debugOptions.limitCamera) {
		this->_cameraTranslate.x = std::min<float>(this->_cameraTranslate.x, static_cast<float>(this->_map.grid.gridSize.x) * this->_map.grid.tileSize.x - size.x);
		this->_cameraTranslate.y = std::min<float>(this->_cameraTranslate.y, static_cast<float>(this->_map.grid.gridSize.y) * this->_map.grid.tileSize.y - size.y);
		this->_cameraTranslate.x = std::max<float>(this->_cameraTranslate.x, 0);
		this->_cameraTranslate.y = std::max<float>(this->_cameraTranslate.y, -20 * guiScale);
	}
	if (cursor != UI::CURSOR_SCROLL_ALL_DIRS) {
		if (old == this->_cameraTranslate)
			cursor += UI::CURSOR_SCROLL_DIFF;
		for (auto &p : this->_players)
			for (auto &u : p.getUIs())
				if (u)
					u->forcedCursor = static_cast<UI::Cursor>(cursor);
	} else for (auto &p : this->_players)
		for (auto &u : p.getUIs())
			if (u)
				u->forcedCursor.reset();
	game->soundMgr.setCamera(this->_cameraTranslate + size / 2, size);
}

void GameManager::_updateStealthDetection()
{
	std::vector<GameObject *> stealthed;
	std::vector<GameObject *> revealers;

	for (auto &o : this->_liveObjects) {
		if (o->isDisposable())
			continue;
		if (o->getStealthParams())
			stealthed.push_back(&*o);
		if ((o->getRevealStealthRange() > 0 || o->getDetectorRange() > 0) && o->isOperational())
			revealers.push_back(&*o);
	}
	for (GameObject *obj : stealthed) {
		const auto type = obj->getStealthParams()->type;
		const size_t team = obj->owner->state.team;
		const Vector2f pos = obj->getMiddle();
		std::vector<unsigned> ids;
		std::set<size_t> teams;

		for (const GameObject *r : revealers) {
			const size_t rteam = r->owner->state.team;

			if (rteam == team)
				continue;

			// Radius is in tiles. The half tile of tolerance makes a radius of 1 cover all the 8 neighbouring tiles.
			const float range = (type == StealthParams::TYPE_STEALTH ? r->getRevealStealthRange() : r->getDetectorRange());

			if (range <= 0)
				continue;

			const Vector2f diff = r->getMiddle() - pos;
			const float limit = (range + 0.5f) * static_cast<float>(obj->map->tileSize.x);

			if (diff.magnitude2() > limit * limit)
				continue;
			ids.push_back(r->index);
			teams.insert(rteam);
		}
		std::ranges::sort(ids);
		obj->updateStealthDetection(ids, teams);
	}
}

void GameManager::update()
{
	if (this->_paused && !this->_forceStep)
		return;
	PROFILE_SCOPE("update");
	this->_forceStep = false;
	this->_tick++;
	this->_hitThisTick = false;
	if (this->_shake.time) {
		this->_shakeTimer++;
		if (this->_shakeTimer >= this->_shake.interval) {
			this->_shake.time--;
			this->_shakeTimer = 0;
		}
		if (this->_shake.freezeTime)
			return;
	}
	{
		PROFILE_SCOPE("stealth");
		this->_updateStealthDetection();
	}
	{
		PROFILE_SCOPE("objects");
		for (auto &o : this->_liveObjects)
			o->update();
	}
	{
		PROFILE_SCOPE("projectiles");
		for (auto &p : this->_liveProjectiles)
			p.update();
	}
	{
		PROFILE_SCOPE("players");
		for (auto &player : this->_players) {
			player.update();
			if (
				this->_settings.shortGame && !player.lost() &&
				!std::ranges::any_of(player.state.myObjects, [](const GameObject *obj) { return obj->countsForShortGame(); })
			)
				for (GameObject *obj : player.state.myObjects)
					if (!obj->isDead() && obj->countsForGame())
						obj->damage(obj->getHealth(), DMGTYPE_GENERIC);
		}
		for (auto &player : this->_players)
			player.checkWon();
		for (auto &player : this->_players)
			player.handleUiEvents();
		if (isMouseOverUI())
			for (auto &p: this->_players)
				p.mouseOnUi();
	}

	{
		PROFILE_SCOPE("effects");
		for (auto &arr : this->_effects) {
			auto ite = arr.begin();
			while (ite != arr.end())
				if (ite->update())
					ite = arr.erase(ite);
				else
					++ite;
		}
	}

	PROFILE_SCOPE("cleanup");
	for (auto &list : this->_renderObjects | std::ranges::views::values) {
		auto it = list.begin();

		while (it != list.end())
			if ((*it)->isDisposable())
				it = list.erase(it);
			else
				++it;
	}

	auto itp = this->_liveProjectiles.begin();
	while (itp != this->_liveProjectiles.end())
		// TODO: Objects above expect the pointers to stay valid until they can check isDisposable
		//       which isn't the case here. We should hold on to disposable objects for a full tick
		//       to allow all objects to properly dispose of the pointers to dead objects.
		if (itp->isDisposable())
			itp = this->_liveProjectiles.erase(itp);
		else
			++itp;

	auto itb = this->_liveObjects.begin();
	std::set<Player *> toRecompute;
	while (itb != this->_liveObjects.end())
		// TODO: Objects above expect the pointers to stay valid until they can check isDisposable
		//       which isn't the case here. We should hold on to disposable objects for a full tick
		//       to allow all objects to properly dispose of the pointers to dead objects.
		if ((*itb)->isDisposable()) {
			(*itb)->disposeOf();
			itb = this->_liveObjects.erase(itb);
		} else
			++itb;
	for (auto p : toRecompute)
		p->recomputeLocked();

	PROFILE_SET(PROFCOUNTER_OBJECTS, this->_liveObjects.size());
	PROFILE_SET(PROFCOUNTER_PROJECTILES, this->_liveProjectiles.size());
	PROFILE_SET(PROFCOUNTER_EFFECTS, this->_effects[0].size() + this->_effects[1].size() + this->_effects[2].size() + this->_effects[3].size() + this->_effects[4].size());
#ifdef _DEBUG
	if (this->_hitThisTick && this->debugOptions.pauseOnWarhead)
		this->_paused = true;
#endif
}

void GameManager::updateRender()
{
	PROFILE_SCOPE("updateRender");
	this->_checkScroll();
	for (auto &player : this->_players)
		player.updateRender();
	if (this->_debugPanel)
		this->_debugPanel->refresh();
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
	for (auto &p : this->_players)
		if (p.isEnabled())
			for (auto &o : p.uiState.selectedObjects)
				o->renderPathFindingPath();

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
	if (tile & GRIDMASK_VEHICLES_CAN_PASS)
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

template<typename F>
void GameManager::_forEachRenderObject(F fct)
{
	auto first = this->_renderObjects.begin();
	auto last = this->_renderObjects.end();

	if (this->debugOptions.renderFilter != RENDERFILTER_ALL) {
		int64_t key = this->_getCursorRenderKey();
		int64_t tolerance = this->debugOptions.renderFilterTolerance;

		switch (this->debugOptions.renderFilter) {
		case RENDERFILTER_BELOW:
			last = this->_renderObjects.upper_bound(key);
			break;
		case RENDERFILTER_ABOVE:
			first = this->_renderObjects.lower_bound(key);
			break;
		case RENDERFILTER_AT:
			first = this->_renderObjects.lower_bound(key - tolerance);
			last = this->_renderObjects.upper_bound(key + tolerance);
			break;
		default:
			break;
		}
	}
	for (; first != last; ++first)
		for (auto b : first->second)
			fct(b);
}

Vector2f GameManager::_getWorldMouse() const
{
	Vector2f pos = Vector2f(game->screen->getMousePosition());

	pos.x /= this->_cameraZoom;
	pos.y /= this->_cameraZoom;
	pos += this->_cameraTranslate;
	return pos;
}

int64_t GameManager::_getCursorRenderKey() const
{
	return std::floor(this->_getWorldMouse().y);
}

void GameManager::render()
{
	PROFILE_SCOPE("render");

	static const char *const layerNames[] = {"layer -2", "layer -1", "layer 0", "layer 1", "layer 2"};
	auto &opts = this->debugOptions;
	auto renderLayer = [this, &opts](int layer) {
		if (!opts.isLayerVisible(layer))
			return;
		PROFILE_SCOPE(layerNames[layer + 2]);
		if (opts.showObjects)
			this->_forEachRenderObject([layer](GameObject *b) {
				b->render(layer);
			});
		if (opts.showEffects)
			for (auto &e : this->_effects[layer + 2])
				e.render();
		if (opts.showProjectiles)
			for (auto &p : this->_liveProjectiles)
				p.render(layer);
	};
	sf::View old = game->screen->getView();
	sf::Sprite sprite{this->_map.renderCache.getTexture()};
	bool dPFGrid = false;
	sf::RectangleShape shape;
	Vector2f extraTranslate = {0, 0};

	if (this->_shake.time != 0) {
		extraTranslate.y = this->_shake.power;
		if (this->_shake.time % 2 == 1)
			extraTranslate.y *= -1;
	}
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineColor(sf::Color::Black);
	shape.setOutlineThickness(2);
	game->screen->setView(sf::View{sf::FloatRect{
		this->_cameraTranslate + extraTranslate,
		sf::Vector2f{
			old.getSize().x * this->_cameraZoom,
			old.getSize().y * this->_cameraZoom
		}
	}});
	if (opts.showTerrain) {
		PROFILE_SCOPE("terrain");
		game->screen->draw(sprite);
	}
	for (int layer = -2; layer <= 0; layer++)
		renderLayer(layer);

	unsigned char gridView = opts.gridView;

	if (gridView != 255 && gridView >= this->getGridViewCount())
		gridView = opts.gridView = 255;
	if (gridView == 0) {
		this->_renderPathFindingGrid(this->_map.pathFindingGrid);
		dPFGrid = true;
	} else if (gridView == 1)
		displayGrid(this->_map.grid);
	else if (gridView == 2) {
		this->_renderPathFindingGrid(this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, true);
		dPFGrid = true;
	} else if (gridView != 255) {
		if ((gridView - 3) % 2 == 1) {
			auto &map = this->_mapStates[(gridView - 3) / 2];

			this->_renderPathFindingGrid(map.pathFindingGrid, map.tileSize, true);
			dPFGrid = true;
		} else
			displayGrid(this->_mapStates[(gridView - 3) / 2]);
	}
	for (int layer = 1; layer <= 2; layer++)
		renderLayer(layer);

	if (dPFGrid) {
		for (auto &list : this->_renderObjects | std::ranges::views::values)
			for (auto b : list) {
				shape.setFillColor(sf::Color::Transparent);
				shape.setOutlineColor(sf::Color::Black);
				shape.setPosition(b->getPosition());
				shape.setSize(b->getSize());
				game->screen->draw(shape);
				shape.setFillColor(sf::Color::Black);
				shape.setOutlineColor(sf::Color::White);
				shape.setPosition(b->getMiddle());
				shape.setSize({3, 3});
				game->screen->draw(shape);
			}
	}
	if (opts.showPlayerOverlays) {
		PROFILE_SCOPE("playerOverlays");
		for (auto &player : this->_players)
			player.render();
	}
	{
		PROFILE_SCOPE("debugWorld");
		this->_debugOverlay->renderWorld();
	}
	game->screen->setView(old);
}

void GameManager::renderAbove()
{
	PROFILE_SCOPE("renderAbove");
	if (this->debugOptions.showPlayerOverlays)
		for (auto &player : this->_players)
			player.renderAbove();
	if (this->debugOptions.gridView != 255) {
		sf::View old = game->screen->getView();
		sf::View v = sf::View{sf::FloatRect{
			{0, 0},
			{
				static_cast<float>(game->screen->getSize().x),
				static_cast<float>(game->screen->getSize().y)
			}
		}};

		this->_text.setString(this->getGridViewLabel(this->debugOptions.gridView));
		game->screen->setView(v);
		game->screen->draw(this->_text);
		game->screen->setView(old);
	}
	PROFILE_SCOPE("debugScreen");
	this->_debugOverlay->renderScreen();
}

size_t GameManager::getGridViewCount() const
{
	return this->_mapStates.size() * 2 + 3;
}

std::string GameManager::getGridViewLabel(unsigned char view) const
{
	if (view == 255)
		return "None";
	if (view == 0)
		return "Path finding grid";
	if (view == 1)
		return "Original map grid";
	if (view == 2)
		return "Original map grid (path finding)";
	if (view >= this->getGridViewCount())
		return "?";
	if ((view - 3) % 2 == 0)
		return "Map for " + this->_factions[(view - 3) / 2].getName();
	return "Map for " + this->_factions[(view - 3) / 2].getName() + " (path finding)";
}

const std::deque<WarheadHit> &GameManager::getWarheadHits() const
{
	return this->_warheadHits;
}

uint64_t GameManager::getTick() const
{
	return this->_tick;
}

bool GameManager::isPaused() const
{
	return this->_paused;
}

void GameManager::requestStep()
{
	this->_forceStep = true;
}

bool GameManager::isDebugUnlocked() const
{
	return this->_isDebugUnlocked;
}

void GameManager::toggleDebugUnlock()
{
#ifdef _DEBUG
	if (this->_isDebugUnlocked)
		this->_recomputeLocked();
	else
		this->_unlockAll();
	this->_isDebugUnlocked = !this->_isDebugUnlocked;
	DEBUG_MODE = this->_isDebugUnlocked;
#endif
}

void GameManager::setControlledPlayer(size_t index)
{
	for (size_t i = 0; i < this->_players.size(); i++)
		if (this->_players[i].isEnabled() != (index == i))
			this->_players[i].setEnabled(index == i);
}

void GameManager::debugNextPlayer()
{
	this->setControlledPlayer((this->getControlledPlayer() + 1) % this->_players.size());
}

size_t GameManager::getControlledPlayer() const
{
	for (size_t i = 0; i < this->_players.size(); i++)
		if (this->_players[i].isEnabled())
			return i;
	return UINT32_MAX;
}

void GameManager::debugRotateSelected()
{
	for (auto &p : this->_players)
		if (p.isEnabled())
			for (auto o : p.uiState.selectedObjects)
				const_cast<GameObject *>(o)->nextDirection();
}

bool GameManager::isDebugPanelOpen() const
{
	return this->_debugPanel && this->_debugPanel->isVisible();
}

void GameManager::toggleDebugPanel()
{
	if (!this->_debugPanel)
		this->_debugPanel = std::make_unique<DebugPanel>(*this);
	this->_debugPanel->toggle();
}

void GameManager::setPaused(bool paused)
{
	this->_paused = paused;
}

void GameManager::renderMinimap(Vector2f pos, Vector2u size)
{
	Vector2f rsize;
	Vector2f rpos;
	sf::Image result = this->_map.minimap;

	if (this->_map.grid.gridSize.x < this->_map.grid.gridSize.y) {
		rsize.x = size.x * this->_map.grid.gridSize.x / this->_map.grid.gridSize.y;
		rsize.y = size.y;
	} else {
		rsize.x = size.x;
		rsize.y = size.y * this->_map.grid.gridSize.y / this->_map.grid.gridSize.x;
	}
	rpos.x = pos.x + (size.x - rsize.x) / 2;
	rpos.y = pos.y + (size.y - rsize.y) / 2;

	// Objects the viewer can't see (cloaked) don't show up. They are removed from the occupied spaces.
	std::map<size_t, std::vector<uint8_t>> hidden;
	size_t viewer = this->getControlledPlayer();

	if (viewer < this->_players.size()) {
		size_t viewerTeam = this->_players[viewer].state.team;
		auto tileSize = this->_map.grid.tileSize;
		auto gridSize = this->_map.grid.gridSize;

		for (auto &list : this->_renderObjects | std::ranges::views::values)
			for (const GameObject *obj : list) {
				if (!obj->isHiddenFrom(viewerTeam))
					continue;

				auto &counts = hidden[obj->owner->state.index];

				counts.resize(static_cast<size_t>(gridSize.x) * gridSize.y, 0);
				for (auto &quad : obj->getGridFootprint()) {
					Vector2i minv = {
						static_cast<int>(floor(std::min(std::min(quad[0].x, quad[1].x), std::min(quad[2].x, quad[3].x)) / tileSize.x)),
						static_cast<int>(floor(std::min(std::min(quad[0].y, quad[1].y), std::min(quad[2].y, quad[3].y)) / tileSize.y))
					};
					Vector2i maxv = {
						static_cast<int>(ceil(std::max(std::max(quad[0].x, quad[1].x), std::max(quad[2].x, quad[3].x)) / tileSize.x)),
						static_cast<int>(ceil(std::max(std::max(quad[0].y, quad[1].y), std::max(quad[2].y, quad[3].y)) / tileSize.y))
					};

					for (int y = std::max(minv.y, 0); y < std::min<int>(maxv.y, gridSize.y); y++)
						for (int x = std::max(minv.x, 0); x < std::min<int>(maxv.x, gridSize.x); x++)
							if (counts[x + y * gridSize.x] != UINT8_MAX)
								counts[x + y * gridSize.x]++;
				}
			}
	}

	for (unsigned y = 0; y < this->_map.grid.gridSize.y; y++)
		for (unsigned x = 0; x < this->_map.grid.gridSize.x; x++)
			for (size_t player = 0; player < this->_players.size(); player++) {
				size_t index = this->_map.grid.gridSize.x * y + x;
				auto it = hidden.find(player);
				unsigned used = this->_map.usedSpaces[player][index];

				if (it != hidden.end())
					used = used > it->second[index] ? used - it->second[index] : 0;
				if (used) {
					auto c = this->_players[player].getColor();

					c.l /= 2;
					result.setPixel({x, y}, c.toRGB());
					break;
				}
			}

	sf::Texture texture{result};
	sf::Sprite sprite{texture};

	sprite.setPosition(rpos);
	sprite.setScale({
		rsize.x / texture.getSize().x,
		rsize.y / texture.getSize().y
	});
	game->screen->draw(sprite);
}

std::vector<Player> &GameManager::getPlayers()
{
	return this->_players;
}

void GameManager::_unlockAll()
{
	for (auto &player : this->_players) {
		for (auto &[name, b] : player.getLoadedObjects())
			player.unlockObject(name, *b, true);
		for (auto &[name, u] : player.getLoadedUpgrades())
			player.unlockObject(name, *u, true);
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

	assert_msg(owner.state.queues.contains(*bqueue), *bqueue);
	auto &queue = owner.state.queues.at(*bqueue);
	auto &entry = queue.entries[name];
	auto &req = object.getRequirements();

	if (
		(queue.params->requiresProducer && owner.state.queueBuildings[*bqueue].empty()) ||
		!req(owner.state.elementsOwned)
	) {
		entry.unlocked = false;
		if (entry.buildTime != 0) {
			for (auto &p : entry.spent)
				owner.credit(p.first, p.second, Player::CREDIT_SYSTEM);
			owner.resetQueue(*bqueue);
		}
	}
	entry.buildTime = object.getBuildSpeed();
}

void GameManager::_recomputeLocked()
{
	for (auto &player : this->_players)
		for (auto &f : this->_factions) {
			for (auto &[name, o] : f.getObjects())
				this->_checkObjectLocked(name, o, player);
			for (auto &[name, u] : f.getUpgrades())
				this->_checkObjectLocked(name, u, player);
		}
}

void GameManager::consumeEvent(const Event &event)
{
	PROFILE_SCOPE("manager");
	if (auto keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		for (auto &p : this->_players)
			p.keyPressed(*keyPressed);
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
	} else if (auto mouseUp = event.getIf<sf::Event::MouseButtonReleased>()) {
		Vector2i pos = mouseUp->position;

		pos /= this->_cameraZoom;
		pos += this->_cameraTranslate;
		if (mouseUp->button == sf::Mouse::Button::Left) {
			if (!isMouseOverUI()) {
				for (auto &p: this->_players)
					p.onLeftMouseReleased(pos);
			}
		} else if (mouseUp->button == sf::Mouse::Button::Right) {
			if (!isMouseOverUI()) {
				for (auto &p : this->_players)
					p.onRightMouseReleased(pos);
			}
		}
	} else if (event.is<sf::Event::FocusLost>()) {
		game->paused = true;
	}
}

void GameManager::consumeGeneralEvent(const sf::Event &event)
{
	PROFILE_SCOPE("debug");
	if (auto keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		// F3 opens the debug panel in every build (it only contains the profiler in release builds).
		if (keyPressed->code == sf::Keyboard::Key::F3)
			this->toggleDebugPanel();
	#ifdef _DEBUG
		else if (keyPressed->code == sf::Keyboard::Key::F1)
			this->toggleDebugUnlock();
		else if (keyPressed->code == sf::Keyboard::Key::F2)
			this->debugNextPlayer();
		else if (keyPressed->code == sf::Keyboard::Key::F11)
			this->_paused = !this->_paused;
		else if (keyPressed->code == sf::Keyboard::Key::F12)
			this->_forceStep = true;
	#endif
	}
}

bool GameManager::canSpawnBuilding(size_t owner, Vector2i absPos, const std::string &id, const std::vector<unsigned> &exclusionList) const
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[player.state.factionIndex];
	auto &objects = player.getObjects();
	assert_msg(objects.contains(id), id);
	auto &base = *objects.at(id);
	auto &tiles = base.getTileData();
	Vector2i pos = map.absToTileCoord(absPos);

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

			if (line[x].placementRequired) {
				uint32_t mask = GRIDMASK_CAN_BUILD;

				if (!this->isAreaFree(map.topLeftCoordToAbs(fullPos), map.tileSize, exclusionList))
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

void GameManager::_initObject(MapState &map, Player &owner, GameObject &obj)
{
	obj.manager = this;
	obj.owner = &owner;
	obj.map = &map;
	obj.pfGrid = &this->_map.pathFindingGrid;
	obj.index = this->_lastObjectId;
	obj.setColor(owner.getColor());
	this->_objectMap[this->_lastObjectId++] = &obj;
}

GameObject *GameManager::_spawnExtra(GameObject &parent, const ExtraSpawn &extra)
{
	GameObject *eobj = this->spawnObject(
		parent.owner->state.index,
		extra.pos.getPosition(parent.getPosition(), *parent.map, {0, 0}),
		"",
		extra.unit
	);

	eobj->setPosition(extra.pos.getPosition(parent.getPosition(), *parent.map, Vector2f{32, 32} + eobj->getSize()));
	if (!extra.carry.empty()) {
		std::vector<GameObject *> objects;

		eobj->clearOrders();
		for (auto &cobj : extra.carry)
			objects.push_back(this->_spawnExtra(*eobj, cobj));
		assert_exp(extra.dest);
		eobj->deliverTo(objects, extra.dest->getPosition(parent.getPosition(), *parent.map, Vector2f{0, 0}));
	}
	if (extra.out)
		eobj->moveOut(extra.out->getPosition(parent.getPosition(), *parent.map, Vector2f{32, 32} + eobj->getSize()));
	return eobj;
}

void GameManager::_onObjectSpawned(Vector2i pos, GameObject &obj)
{
	auto p = obj.map->topLeftCoordToAbs(pos);

	obj.spawn(p);
	for (auto &order : obj.getSpawnOrders())
		this->_pushOrder(obj, order);

	const auto &extra = obj.getExtraSpawn();

	if (extra)
		this->_spawnExtra(obj, *extra);
	obj.owner->onObjectGained(&obj);
	obj.built();
}

float GameManager::initUnitTiles(const std::vector<std::vector<BuildingMask>> &tiles, Vector2i pos, MapState &map)
{
	unsigned nbConcreteTiles = 0;
	unsigned nbTiles = 0;

	for (size_t y = 0; y < tiles.size(); y++) {
		auto &line = tiles[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};
			auto fullPosAbs = map.tileCoordToAbs(fullPos);

			if (line[x].placementRequired) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.resetGridBits(fullPos, GRIDMASK_IS_FREE);
				for (auto &m : this->_mapStates)
					m.resetGridBits(fullPosAbs, GRIDMASK_IS_FREE);
			}
			if (line[x].solid) {
				nbTiles++;
				// OPTIMIZE: Can use a reference instead of calling helpers
				nbConcreteTiles += map.hasAnyGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
			}
		}
	}
	if (nbTiles == 0)
		return 1;
	return static_cast<float>(nbConcreteTiles) / static_cast<float>(nbTiles);
}

void GameManager::deinitUnitTiles(const std::vector<std::vector<BuildingMask>> &tiles, Vector2i pos, MapState &map)
{
	for (size_t y = 0; y < tiles.size(); y++) {
		auto &line = tiles[y];

		if (y + pos.y >= map.gridSize.y)
			break;
		for (size_t x = 0; x < line.size(); x++) {
			if (x + pos.x >= map.gridSize.x)
				break;

			Vector2i fullPos = {static_cast<int>(pos.x + x), static_cast<int>(pos.y + y)};
			auto fullPosAbs = map.tileCoordToAbs(fullPos);

			if (line[x].placementRequired) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.setGridBits(fullPos, GRIDMASK_IS_FREE);
				for (auto &m : this->_mapStates)
					m.setGridBits(fullPosAbs, GRIDMASK_IS_FREE);
			}
		}
	}
}

GameObject *GameManager::spawnObject(size_t owner, Vector2i absPos, const std::string &buildQueue, const std::string &id)
{
	auto &in = this->_players[owner].getObjects();
	assert_msg(in.contains(id), id);
	auto &base = *in.at(id);
	auto &player = this->_players[owner];
	GameObject *parent = player.getSpawnBuilding(buildQueue, base.isStackable());

	if (absPos.x == -1 && absPos.y == -1 && !parent)
		return nullptr;

	auto &out = this->_liveObjects;
	auto &map = this->_mapStates[player.state.factionIndex];
	auto &objects = player.getObjects();
	std::string concrete = player.getPrimaryFaction().getBase().getName() + ":conc";
	assert_msg(objects.contains(concrete), concrete);
	auto &concBase = *objects.at(concrete);
	auto &tiles = base.getTileData();
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
				line[x].placementConcrete &&
				// OPTIMIZE: Can use a reference instead of calling helpers
				map.hasAllGridBits(fullPos, GRIDMASK_CAN_BUILD) &&
				this->isAreaFree(map.topLeftCoordToAbs(fullPos), map.tileSize, {}, false, true)
			) {
				// OPTIMIZE: Can use a reference instead of calling helpers
				this->_map.grid.setGridBits(fullPos, GRIDMASK_HAS_CONCRETE);
				for (auto &m : this->_mapStates)
					m.setGridBits(fullPosAbs, GRIDMASK_HAS_CONCRETE);
				this->_liveObjects.push_back(std::make_unique<GameObject>(concBase));
				this->_initObject(map, player, *this->_liveObjects.back());
				this->_onObjectSpawned(fullPos, *this->_liveObjects.back());
			}
		}
	}

noGrid:
	auto proxy = base.getProxy();
	GameObject *obj = nullptr;

	if (proxy)
		assert_msg(in.contains(*proxy), *proxy);
	base.playBuiltSound(absPos);
	if (!base.isFake()) {
		if (proxy)
			out.push_back(std::make_unique<GameObject>(*in.at(*proxy)));
		else
			out.push_back(std::make_unique<GameObject>(base));
		obj = &*out.back();

		this->_initObject(map, player, *obj);
		if (parent) {
			auto exit = parent->getExitOffset();
			auto spawn = parent->getSpawnOffset();

			if (spawn) {
				pos = map.absToTileCoord(*spawn + parent->getPosition());
				if (exit) {
					auto next = *exit + parent->getPosition();

					// Both the spawn and exit points may be inside the factory: it must not block the unit.
					obj->moveTo(next, MOVEREASON_MOVE_SPAWN, true, parent);
					obj->setDirection(obj->getPosition().angle(next));
				}
			}
		}
		this->_onObjectSpawned(pos, *obj);

		auto &gqueue = obj->getGivenQueue();
		auto &bqueue = obj->getBuildQueue();

		if (parent)
			parent->produced();
		else if (gqueue && bqueue && !parent && *gqueue == *bqueue)
			obj->produced();
	}
	return obj;
}

void GameManager::spawnCustomObject(size_t owner, Vector2i absPos, GameObject *obj)
{
	auto &player = this->_players[owner];
	auto &map = this->_mapStates[player.state.factionIndex];
	Vector2i pos = map.absToTileCoord(absPos);

	this->_liveObjects.emplace_back(obj);
	this->_onObjectSpawned(pos, *obj);
}

std::vector<Faction> & GameManager::getFactions()
{
	return this->_factions;
}

const std::vector<Faction> &GameManager::getFactions() const
{
	return this->_factions;
}

const LoadedMap &GameManager::getLoadedMap() const
{
	return this->_map;
}

bool GameManager::_isInArea(const GameObject &obj, const Vector2i &absPos, const Vector2u &size, bool onlySolid, bool partial)
{
	if (obj.isDisposable())
		return false;
	if (onlySolid && !obj.isSolid())
		return false;

	auto objPos = obj.getPosition();
	auto &tiles = obj.getTileData();

	if (!tiles) {
		auto s = obj.getSize();

		if (partial) {
			if (absPos.x + static_cast<int>(size.x) <= objPos.x)
				return false;
			if (absPos.y + static_cast<int>(size.y) <= objPos.y)
				return false;
			if (absPos.x >= objPos.x + static_cast<int>(s.x))
				return false;
			if (absPos.y >= objPos.y + static_cast<int>(s.y))
				return false;
		} else {
			if (absPos.x + static_cast<int>(size.x) <= objPos.x + static_cast<int>(s.x))
				return false;
			if (absPos.y + static_cast<int>(size.y) <= objPos.y + static_cast<int>(s.y))
				return false;
			if (absPos.x >= objPos.x)
				return false;
			if (absPos.y >= objPos.y)
				return false;
		}
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
			if (!line[x].solid)
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

bool GameManager::isAreaFree(const Vector2i &absPos, const Vector2u &size, const std::vector<unsigned> &exclusion, bool stackable, bool notSolid) const
{
	PROFILE_SCOPE("isAreaFree");
	PROFILE_COUNT(PROFCOUNTER_AREA_QUERIES, 1);
	PROFILE_COUNT(PROFCOUNTER_OBJECTS_QUERIED, this->_liveObjects.size());
	// OPTIMIZE: Split the map into quadrants
	unsigned stackCount = 0;

	for (auto &o : this->_liveObjects) {
		if (std::ranges::find(exclusion, o->index) != exclusion.end())
			continue;
		if (!GameManager::_isInArea(*o, absPos, size, notSolid, true))
			continue;
		// A stackable mover can share the area with other stackable units,
		// up to PATHFINDING_STACK_LIMIT of them; anything else blocks it.
		if (!stackable || !o->isStackable())
			return false;
		if (++stackCount >= PATHFINDING_STACK_LIMIT)
			return false;
	}
	return true;
}

GameObject *GameManager::getObjectInArea(const Vector2i &absPos, const Vector2u &size, bool partial, const std::function<bool(GameObject *)> &fct)
{
	PROFILE_SCOPE("getObjectInArea");
	PROFILE_COUNT(PROFCOUNTER_AREA_QUERIES, 1);
	PROFILE_COUNT(PROFCOUNTER_OBJECTS_QUERIED, this->_liveObjects.size());
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveObjects)
		if ((!fct || fct(&*b)) && GameManager::_isInArea(*b, absPos, size, false, partial))
			return &*b;
	return nullptr;
}

void GameManager::getObjectsInArea(const Vector2i &absPos, const Vector2u &size, std::vector<GameObject *> &objects, bool partial, const std::function<bool (GameObject *)> &fct)
{
	PROFILE_SCOPE("getObjectsInArea");
	PROFILE_COUNT(PROFCOUNTER_AREA_QUERIES, 1);
	PROFILE_COUNT(PROFCOUNTER_OBJECTS_QUERIED, this->_liveObjects.size());
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveObjects)
		if ((!fct || fct(&*b)) && GameManager::_isInArea(*b, absPos, size, false, partial))
			objects.push_back(&*b);
}

void GameManager::getObjectsInArea(const Vector2i &absPos, const Vector2u &size, std::vector<const GameObject *> &objects, bool partial, const std::function<bool (const GameObject *)> &fct)
{
	PROFILE_SCOPE("getObjectsInArea");
	PROFILE_COUNT(PROFCOUNTER_AREA_QUERIES, 1);
	PROFILE_COUNT(PROFCOUNTER_OBJECTS_QUERIED, this->_liveObjects.size());
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveObjects)
		if ((!fct || fct(&*b)) && GameManager::_isInArea(*b, absPos, size, false, partial))
			objects.push_back(&*b);
}

void GameManager::registerObject(GameObject *o)
{
	auto &l = this->_renderObjects[o->getBottom() + o->getAltitude() * this->_map.pathFindingGrid.gridSize.y];
	auto it = std::ranges::find_if(this->_renderObjects, [o](const std::pair<unsigned, std::list<GameObject *>> &l){
		return std::ranges::find(l.second, o) != l.second.end();
	});

	assert_msg(it == this->_renderObjects.end(), std::to_string(it->first));
	l.push_back(o);
}

void GameManager::deregisterObject(GameObject *o)
{
	auto &l = this->_renderObjects[o->getBottom() + o->getAltitude() * this->_map.pathFindingGrid.gridSize.y];
	auto it = std::find(l.begin(), l.end(), o);

	assert_exp(it != l.end());
	l.erase(it);
}


void GameManager::_modUnitInArea(const MapState::Quad &quad, LoadedMap &map, Vector2u tileSize, size_t player, char mod)
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

	for (int y = std::max(minv.y, 0); y < std::min<int>(maxv.y, map.grid.gridSize.y); y++)
		for (int x = std::max(minv.x, 0); x < std::min<int>(maxv.x, map.grid.gridSize.x); x++)
			if (mod < 0) {
				assert_exp(map.usedSpaces[player][x + y * map.grid.gridSize.x] != 0);
				map.usedSpaces[player][x + y * map.grid.gridSize.x]--;
			} else if (mod > 0) {
				assert_exp(map.usedSpaces[player][x + y * map.grid.gridSize.x] < UINT8_MAX);
				map.usedSpaces[player][x + y * map.grid.gridSize.x]++;
			}
}

void GameManager::_modUnitInArea(const MapState::Quad &quad, PathFindingGrid &state, Vector2u tileSize, pfGridType mask, size_t team, char mod, bool isStatic, bool crushable)
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
			if (mod < 0) {
				PFGRIDMASK_DEL_UNIT(state, x, y, mask);
				if (isStatic)
					PFGRIDMASK_DEL_STATIC(state, x, y);
				if (!crushable)
					PFGRIDMASK_DEL_NON_CRUSHABLE(state, x, y);
				assert_exp(state.usedSpaces[team][x + y * state.gridSize.x] != 0);
				state.usedSpaces[team][x + y * state.gridSize.x]--;
			} else if (mod > 0) {
				PFGRIDMASK_ADD_UNIT(state, x, y, mask);
				if (isStatic)
					PFGRIDMASK_ADD_STATIC(state, x, y);
				if (!crushable)
					PFGRIDMASK_ADD_NON_CRUSHABLE(state, x, y);
				assert_exp(state.usedSpaces[team][x + y * state.gridSize.x] < UINT8_MAX);
				state.usedSpaces[team][x + y * state.gridSize.x]++;
			}
}

void GameManager::modUnitInArea(const MapState::Quad &quad, size_t player, int placementCount, pfGridType mask, char mod, bool isStatic, bool crushable)
{
	size_t team = this->_players[player].state.team;

	this->_modUnitInArea(quad, this->_map, this->_map.grid.tileSize, player, mod);
	this->_modUnitInArea(quad, this->_map.pathFindingGrid, {1, 1}, mask, team, mod, isStatic, crushable);
	this->_modUnitInArea(quad, this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, mask, team, mod, isStatic, crushable);
	for (auto &map : this->_mapStates)
		this->_modUnitInArea(quad, map.pathFindingGrid, map.tileSize, mask, team, mod, isStatic, crushable);
	if (placementCount != 0) {
		this->_map.grid.updatePlacementGrid(quad, team, placementCount);
		for (auto &m : this->_mapStates)
			m.updatePlacementGrid(quad, team, placementCount);
	}
}

void GameManager::modUnitInArea(Vector2i rel, Vector2u tileSize, size_t player, int placementCount, pfGridType mask, char mod, bool isStatic, bool crushable)
{
	const MapState::Quad quad = {
		rel,
		Vector2f(rel.x + static_cast<int>(tileSize.x), rel.y),
		Vector2f(rel.x + static_cast<int>(tileSize.x), rel.y + static_cast<int>(tileSize.y)),
		Vector2f(rel.x, rel.y + static_cast<int>(tileSize.y)),
	};
	size_t team = this->_players[player].state.team;

	this->_modUnitInArea(quad, this->_map, this->_map.grid.tileSize, player, mod);
	this->_modUnitInArea(quad, this->_map.pathFindingGrid, {1, 1}, mask, team, mod, isStatic, crushable);
	this->_modUnitInArea(quad, this->_map.grid.pathFindingGrid, this->_map.grid.tileSize, mask, team, mod, isStatic, crushable);
	for (auto &map : this->_mapStates)
		this->_modUnitInArea(quad, map.pathFindingGrid, map.tileSize, mask, team, mod, isStatic, crushable);
	if (placementCount != 0) {
		this->_map.grid.updatePlacementGrid(quad, team, placementCount);
		for (auto &m : this->_mapStates)
			m.updatePlacementGrid(quad, team, placementCount);
	}
}

GameObject *GameManager::getObjectById(unsigned int index)
{
	assert_exp(this->_objectMap.contains(index));
	return this->_objectMap[index];
}

GameObject *GameManager::findPickup(size_t owner, MoveReason reason, double dist, GameObject *ignore)
{
	auto &l = this->_players[owner].state.myObjects;

	for (auto &o : l)
		if (o != ignore && o->canPickup(reason, dist))
			return o;
	return nullptr;
}

std::optional<Vector2i> GameManager::getFirstHarvestableTile(const GameObject &harvester, Vector2i pos) const
{
	Vector2i topLeft;
	Vector2i bottomRight;
	auto &grid = this->_map.grid;
	auto &state = *harvester.map;

#define CHECK_RES(x_, y_) do {                                                                                          \
	if ((x_) < 0 || (y_) < 0 || (x_) >= (int)grid.gridSize.x || (y_) >= (int)grid.gridSize.y)                       \
		break;                                                                                                  \
	auto r = GRID_RESOURCE_TYPE(grid.grid[(x_) + (y_) * grid.gridSize.x]);                                          \
	if (r == 0) break;                                                                                              \
	if (!this->isAreaFree(grid.topLeftCoordToAbs({x_, y_}), grid.tileSize)) break;                                  \
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
			if (!harvester.canStandOn(state.absToTileCoord(grid.topLeftCoordToAbs({x, y}))))
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

std::optional<Vector2i> GameManager::getClosestHarvestableTile(const GameObject &harvester, Vector2i pos) const
{
	auto &grid = this->_map.grid;
	auto &state = *harvester.map;
	int w = grid.gridSize.x;
	int h = grid.gridSize.y;
	auto cb = [this, &grid, &state, &harvester](int x, int y) -> std::optional<Vector2i> {
		if (x < 0 || y < 0 || x >= static_cast<int>(grid.gridSize.x) || y >= static_cast<int>(grid.gridSize.y))
			return {};

		auto res = GRID_RESOURCE_TYPE(grid.grid[x + y * grid.gridSize.x]);
		auto tile = this->_map.displayedIndices[x + y * grid.gridSize.x];
		auto mapTile = state.absToTileCoord(grid.topLeftCoordToAbs({x, y}));

		if (res == 0)
			return {};
		if (!harvester.canStandOn(mapTile))
			return {};
		CHECK_RES(x - 1, y - 1);
		CHECK_RES(x,     y - 1);
		CHECK_RES(x + 1, y - 1);
		CHECK_RES(x - 1, y);
		CHECK_RES(x + 1, y);
		CHECK_RES(x - 1, y + 1);
		CHECK_RES(x,     y + 1);
		CHECK_RES(x + 1, y + 1);
		return mapTile;
	keepGoing:
		return {};
	};
	std::optional<Vector2i> result;

	pos = grid.absToTileCoord(state.topLeftCoordToAbs(pos));
	result = cb(pos.x, pos.y);
	if (result) return result;
	for (int i = 1; ; i++) {
		int x0 = pos.x - i;
		int x1 = pos.x + i;
		int y0 = pos.y - i;
		int y1 = pos.y + i;

		// The whole ring is outside of the grid: nothing left to search.
		if (x0 < 0 && y0 < 0 && x1 >= w && y1 >= h)
			return {};
		// Top and bottom sides (corners included).
		for (int x = std::max(x0, 0); x <= std::min(x1, w - 1); x++) {
			if (y0 >= 0) {
				result = cb(x, y0);
				if (result) return result;
			}
			if (y1 < h) {
				result = cb(x, y1);
				if (result) return result;
			}
		}
		// Left and right sides (corners already done).
		for (int y = std::max(y0 + 1, 0); y <= std::min(y1 - 1, h - 1); y++) {
			if (x0 >= 0) {
				result = cb(x0, y);
				if (result) return result;
			}
			if (x1 < w) {
				result = cb(x1, y);
				if (result) return result;
			}
		}
	}
#undef CHECK_RES
}

GameObject *GameManager::getClosestResourceDump(const std::vector<std::string> &preferredResources, size_t player, Vector2i pos)
{
	if (this->_liveObjects.empty())
		return nullptr;

	GameObject *best = nullptr;
	double dist = std::numeric_limits<double>::infinity();
	size_t resourceScore = 0;
	size_t team = this->_players[player].state.team;

	for (auto &b : this->_liveObjects) {
		if (b->isDead() || b->owner->state.team != team)
			continue;

		auto &accept = b->getAcceptResources();

		if (accept.empty())
			continue;

		size_t score = 0;
		auto d = pos.distance2(b->getPosition() + b->getPoint() + b->map->tileSize / 2);

		if (b->owner->state.index == player)
			score += 1000000;
		for (auto &r : preferredResources)
			score += std::ranges::find(accept, r) != accept.end();
		if (resourceScore > score)
			continue;
		if (resourceScore < score || d < dist) {
			resourceScore = score;
			best = &*b;
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
}

std::pair<std::string, unsigned> GameManager::convertResource(const std::string &source, const std::vector<std::string> &expected, unsigned int value)
{
	assert_exp(!expected.empty());
	// TODO:
	assert_not_reached();
}

Effect &GameManager::spawnEffect(const Effect &effect, Vector2f pos, const Player *owner)
{
	assert_exp(effect.getLayer() >= -2 && effect.getLayer() <= 2);
	std::vector<Effect> &arr = this->_effects[effect.getLayer() + 2];
	arr.emplace_back(effect);
	Effect &result = arr.back();

	result.setPosition(pos);
	if (owner)
		result.setColor(owner->getColor());
	result.playSound();
	return result;
}

Projectile &GameManager::spawnProjectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, const GameObject *target, Vector2f targetPos)
{
	this->_liveProjectiles.emplace_back(parent, weapon, pos, target, targetPos);
	this->_liveProjectiles.back().manager = this;
	return this->_liveProjectiles.back();
}

void GameManager::_recordWarheadHit(const Weapon *weapon, Vector2f position, unsigned damage, size_t owner)
{
	const Warhead *warhead = weapon->getWarhead();

	this->_hitThisTick = true;
	// Projectiles hitting every tick (hitTravel) only get one record
	if (this->hitSource) {
		size_t checked = 0;

		for (auto it = this->_warheadHits.rbegin(); it != this->_warheadHits.rend() && checked < 32; ++it, ++checked)
			if (it->source == this->hitSource && it->weapon == weapon && it->lastTick + 1 >= this->_tick) {
				if (it->lastTick != this->_tick)
					it->hits++;
				it->lastTick = this->_tick;
				this->_currentHit = &*it;
				this->_currentHit->position = position;
				return;
			}
	}
	this->_warheadHits.push_back(WarheadHit{
		this->_tick, this->_tick, position, warhead->getRadius(), damage, 1,
		warhead->getDmgType(), owner, weapon, this->hitSource, {}
	});
	if (this->_warheadHits.size() > DEBUG_WARHEAD_LOG_SIZE)
		this->_warheadHits.pop_front();
	this->_currentHit = &this->_warheadHits.back();
}

void GameManager::_recordWarheadTarget(const Weapon *weapon, const GameObject &obj, Vector2f impact, double dist, float multiplier, unsigned damage, size_t owner)
{
	Vector2f hitPoint = obj.getHitboxCenter(impact);

	// Direct application on an object (enter weapons): no area, create a record for it
	if (!this->_currentHit) {
		this->_recordWarheadHit(weapon, hitPoint, weapon->getDamage(), owner);
		this->_currentHit->hits = 1;
	}
	for (auto &t : this->_currentHit->targets)
		if (t.objectId == obj.index) {
			t.damage += damage;
			t.distance = std::min(t.distance, dist);
			t.hitPoint = hitPoint;
			return;
		}
	this->_currentHit->targets.push_back({obj.index, hitPoint, dist, multiplier, damage});
}

void GameManager::weaponUsedAt(unsigned damage, const Weapon *weapon, Vector2f position, size_t owner)
{
	PROFILE_SCOPE("weaponUsedAt");

	const Warhead *warhead = weapon->getWarhead();
	unsigned radius = warhead->getRadius();
	double dist = radius * radius;

#ifdef _DEBUG
	WarheadHit *previousHit = this->_currentHit;

	this->_currentHit = nullptr;
	if (this->debugOptions.warheads || this->debugOptions.pauseOnWarhead)
		this->_recordWarheadHit(weapon, position, damage, owner);
#endif
	// OPTIMIZE: Split the map into quadrants
	for (auto &b : this->_liveObjects) {
		if (b->isDisposable() || b->isDead())
			continue;

		double d = b->getHitboxCenter(position).distance2(position);

		if (d < dist)
			this->weaponUsedOn(damage, weapon, sqrt(d), owner, *b);
	}
#ifdef _DEBUG
	this->_currentHit = previousHit;
#endif
}

void GameManager::weaponUsedOn(unsigned damage, const Weapon *weapon, double dist, size_t owner, GameObject &obj)
{
	const Warhead *warhead = weapon->getWarhead();
	unsigned radius = warhead->getRadius();
	double result = damage;
	float mult = warhead->getMultiplier(obj.getArmor());

	if (radius)
		result *= (radius - dist) / radius;
	result *= mult;
	if (result < 0)
		result = 0;
	obj.damage(std::ceil(result), warhead->getDmgType());
#ifdef _DEBUG
	if (this->debugOptions.warheads || this->debugOptions.pauseOnWarhead) {
		WarheadHit *previousHit = this->_currentHit;

		this->_recordWarheadTarget(
			weapon, obj,
			this->_currentHit ? this->_currentHit->position : Vector2f(obj.getMiddle()),
			dist, mult, std::ceil(result), owner
		);
		// A record created for a direct application must not leak into the next calls
		if (!previousHit)
			this->_currentHit = nullptr;
	}
#endif

	if (auto &takeControl = weapon->getTakeControl()) {
		bool canTakeControl = false;

		for (auto &tag : takeControl->targets) {
			auto &tags = obj.getTags();

			if (std::ranges::find(tags, tag) != tags.end()) {
				canTakeControl = true;
				break;
			}
		}
		if (canTakeControl) {
			assert_exp(owner <= this->_players.size());
			obj.setOwner(&this->_players[owner], takeControl->baseTime, takeControl->timeLossHit);
		}
	}
}

void GameManager::shakeScreen(const ShakeParams &params)
{
	if (this->_shake.time == 0 || params.freezeTime || params.power > this->_shake.power) {
		this->_shake = params;
		this->_shake.time *= 2;
	}
}

void GameManager::preloadAllSprites()
{
	for (auto &faction : this->_factions)
		for (auto &vec : faction.getAnimations() | std::ranges::views::values)
			for (auto &sprite : vec) {
				for (auto &player : this->_players)
					this->_preloadedSprites.push_back(sprite.preload(player.getColor()));
				// Neutral units are white
				this->_preloadedSprites.push_back(sprite.preload(HSLColor{0, 0, 1000}));
			}
}

void GameManager::preloadSprites()
{
	for (auto &player : this->_players)
		for (auto &vec : player.getPrimaryFaction().getAnimations() | std::ranges::views::values)
			for (auto &sprite : vec)
				this->_preloadedSprites.push_back(sprite.preload(player.getColor()));
}

const GameManager::LobbySettings &GameManager::getSettings() const
{
	return this->_settings;
}

void GameManager::centerCameraOn(Vector2f target)
{
	Vector2u size = game->screen->getSize();

	// TODO: Hardcoded UI offset
	size /= this->_cameraZoom;
	size.x -= 160 * guiScale;
	size.y -= 20 * guiScale;
	this->_cameraTranslate = {
		target.x - size.x / 2,
		target.y - size.y / 2 + 20 * guiScale
	};
	this->_cameraTranslate.x = std::min<float>(this->_cameraTranslate.x, static_cast<float>(this->_map.grid.gridSize.x) * this->_map.grid.tileSize.x - size.x);
	this->_cameraTranslate.y = std::min<float>(this->_cameraTranslate.y, static_cast<float>(this->_map.grid.gridSize.y) * this->_map.grid.tileSize.y - size.y);
	this->_cameraTranslate.x = std::max<float>(this->_cameraTranslate.x, 0);
	this->_cameraTranslate.y = std::max<float>(this->_cameraTranslate.y, -20 * guiScale);
	game->soundMgr.setCamera(this->_cameraTranslate + size / 2, size);
}

void ShakeParams::pushUserType(sol::state &state)
{
	sol::usertype<ShakeParams> type = state.new_usertype<ShakeParams>("ShakeParams");

	type["time"] = &ShakeParams::time;
	type["power"] = &ShakeParams::power;
	type["interval"] = &ShakeParams::interval;
	type["freeze_time"] = &ShakeParams::freezeTime;
}

void GameManager::pushUserType(sol::state &state)
{
	ShakeParams::pushUserType(state);
	MapResource::pushUserType(state);
	MapState::pushUserType(state);

	sol::usertype<GameManager> type = state.new_usertype<GameManager>("GameManager");

	type["load_map"] = &GameManager::loadMap;
	type["add_player"] = &GameManager::addPlayer;
	type["update"] = &GameManager::update;
	type["update_render"] = &GameManager::updateRender;
	type["render"] = &GameManager::render;
	type["render_above"] = &GameManager::renderAbove;
	type["paused"] = sol::property(&GameManager::setPaused);
	type["render_minimap"] = &GameManager::renderMinimap;
	type["get_object_by_id"] = &GameManager::getObjectById;
	type["find_pickup"] = &GameManager::findPickup;
	type["spawn_object"] = &GameManager::spawnObject;
	type["can_spawn_building"] = &GameManager::canSpawnBuilding;
	type["is_area_free"] = &GameManager::isAreaFree;
	type["get_objects_in_area"] = [](GameManager &self, const Vector2i &pos, const Vector2u &size) {
		std::vector<GameObject *> objects;

		self.getObjectsInArea(pos, size, objects);
		return objects;
	};
	type["mod_unit_in_area"] = sol::overload(
		sol::resolve<void(const MapState::Quad &, size_t, int, pfGridType, char, bool, bool)>(&GameManager::modUnitInArea),
		sol::resolve<void(Vector2i, Vector2u, size_t, int, pfGridType, char, bool, bool)>(&GameManager::modUnitInArea)
	);
	type["get_first_harvestable_tile"] = &GameManager::getFirstHarvestableTile;
	type["get_closest_harvestable_tile"] = &GameManager::getClosestHarvestableTile;
	type["get_closest_resource_dump"] = &GameManager::getClosestResourceDump;
	type["get_resource"] = &GameManager::getResource;
	type["consume_resource"] = &GameManager::consumeResource;
	type["center_camera_on"] = sol::overload(
		sol::resolve<void(Vector2f)>(&GameManager::centerCameraOn),
		sol::resolve<void(const GameObject &)>(&GameManager::centerCameraOn)
	);
	type["convert_resource"] = &GameManager::convertResource;
	type["spawn_effect"] = &GameManager::spawnEffect;
	type["spawn_projectile"] = &GameManager::spawnProjectile;
	type["weapon_used_at"] = &GameManager::weaponUsedAt;
	type["weapon_used_on"] = &GameManager::weaponUsedOn;
	type["shake_screen"] = &GameManager::shakeScreen;
	// NOTE: consumeEvent()/consumeGeneralEvent() (raw sf::Event, not wrapped
	// for Lua), registerObject()/deregisterObject() (internal bookkeeping of
	// raw non-owning pointers into engine-managed lists) and
	// initUnitTiles()/deinitUnitTiles() (raw BuildingMask bit-grid, same
	// reasoning as GameObject::getTileData()) are intentionally not exposed.
	// getPlayers()/getFactions() (std::vector<Player>/std::vector<Faction>)
	// and getLoadedMap() (LoadedMap, which owns an sf::RenderTexture) are
	// also not exposed: Player and Faction both hold non-static reference
	// members (so std::vector<T> of them isn't move/copy-assignable, which
	// sol2's container support needs even just to register the usertype),
	// and LoadedMap isn't copyable either, same reasoning as
	// Upgrade::getRequirements()/Player::getUIs().
}
