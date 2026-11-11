//
// Created by PinkySmile on 06/04/2026.
//

#include <sol/sol.hpp>
#include "Player.hpp"
#include "Faction.hpp"
#include "Upgrade.hpp"
#include "Animation.hpp"
#include "Effect.hpp"
#include "Weapon.hpp"
#include "Warhead.hpp"
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "../Resources/Game.hpp"
#include "../UI/UI.hpp"
#include "Debug/Profiler.hpp"
#include "Resources/Assert.hpp"

#define PRIORITY_BUILD 3
#define EXPIRE_BUILD 300
#define PRIORITY_POWER 0
#define EXPIRE_POWER 1800
#define COOLDOWN_POWER 1800

CustomQueue::CustomQueue(Player *player, PlayerQueue *queue) :
	queue(queue),
	player(player)
{
}

CustomQueueLua::CustomQueueLua(Player *player, PlayerQueue *queue, sol::state &lua, const std::filesystem::path &path) :
	CustomQueue(player, queue)
{
	this->_env["super"] = static_cast<CustomQueue *>(this);
	// safe_script_file (rather than do_file) throws automatically if the
	// script fails to load/parse or errors at the top level while running.
	this->_function = lua.safe_script_file(path.string(), this->_env);
}

bool CustomQueueLua::update(PlayerQueue &)
{
	// TODO:
	return true;
}

bool CustomQueueLua::update(PlayerQueueEntry *)
{
	// TODO:
	return true;
}

bool CustomQueueLua::tryEndBuild(PlayerQueueEntry *)
{
	// TODO:
	return true;
}

bool CustomQueueLua::handleStartBuildEvent(PlayerQueueEntry *, const UI::StartBuildEvent &)
{
	// TODO:
	return true;
}

bool CustomQueueLua::handlePlaceBuildEvent(PlayerQueueEntry *, const UI::PlaceBuildEvent &)
{
	// TODO:
	return true;
}

bool CustomQueueLua::handleCancelBuildEvent(PlayerQueueEntry *, const UI::CancelBuildEvent &)
{
	// TODO:
	return true;
}

CustomQueueDll::CustomQueueDll(Player *player, PlayerQueue *queue, const std::filesystem::path &path) :
	CustomQueue(player, queue)
{
	this->_library = dlopen(path.string().c_str(), RTLD_LAZY);
	assert_msg(this->_library, path.string() + ": " + dlerror());

	auto version = static_cast<unsigned *>(dlsym(this->_library, "_ABI_VERSION"));
	assert_exp(version && *version == ABI_VERSION);

	this->_update = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueue &)>(dlsym(this->_library, "update"));
	this->_updateEntry = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueueEntry *)>(dlsym(this->_library, "updateEntry"));
	this->_tryEndBuild = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueueEntry *)>(dlsym(this->_library, "tryEndBuild"));
	this->_handleStartBuildEvent = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueueEntry *, const UI::StartBuildEvent &)>(dlsym(this->_library, "handleStartBuildEvent"));
	this->_handlePlaceBuildEvent = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueueEntry *, const UI::PlaceBuildEvent &)>(dlsym(this->_library, "handlePlaceBuildEvent"));
	this->_handleCancelBuildEvent = reinterpret_cast<bool (*)(CustomQueue &, PlayerQueueEntry *, const UI::CancelBuildEvent &)>(dlsym(this->_library, "handleCancelBuildEvent"));
}

CustomQueueDll::~CustomQueueDll()
{
	if (this->_library)
		dlclose(this->_library);
}

bool CustomQueueDll::update(PlayerQueue &entry)
{
	if (this->_update)
		return this->_update(*this, entry);
	return true;
}

bool CustomQueueDll::update(PlayerQueueEntry *entry)
{
	if (this->_updateEntry)
		return this->_updateEntry(*this, entry);
	return true;
}

bool CustomQueueDll::tryEndBuild(PlayerQueueEntry *entry)
{
	if (this->_tryEndBuild)
		return this->_tryEndBuild(*this, entry);
	return true;
}

bool CustomQueueDll::handleStartBuildEvent(PlayerQueueEntry *entry, const UI::StartBuildEvent &event)
{
	if (this->_handleStartBuildEvent)
		return this->_handleStartBuildEvent(*this, entry, event);
	return true;
}

bool CustomQueueDll::handlePlaceBuildEvent(PlayerQueueEntry *entry, const UI::PlaceBuildEvent &event)
{
	if (this->_handlePlaceBuildEvent)
		return this->_handlePlaceBuildEvent(*this, entry, event);
	return true;
}

bool CustomQueueDll::handleCancelBuildEvent(PlayerQueueEntry *entry, const UI::CancelBuildEvent &event)
{
	if (this->_handleCancelBuildEvent)
		return this->_handleCancelBuildEvent(*this, entry, event);
	return true;
}

void PlayerQueue::init(Player &player, const std::string *id, const Faction &faction, const FactionQueue *params)
{
	const std::unordered_map<std::string, const Upgrade *> &upgrades = player.getLoadedUpgrades();
	const std::unordered_map<std::string, const GameObject *> &objects = player.getLoadedObjects();

	this->player = &player;
	this->params = params;
	if (params->behavior == FactionQueue::TYPE_STARPORT) {
		for (auto &u : params->starportParams->units) {
			assert_msg(objects.contains(u), u);
			const GameObject *obj = objects.at(u);
			auto &entry = this->entries[u];

			entry.object = obj;
			entry.queue = this;
			entry.queueId = id;
			entry.elementId = &u;
			this->stock[u] = 0;
			this->prices[u] = obj->getCost();
			this->basket[u] = 0;
		}
	} else {
		for (auto &[nid, obj] : objects) {
			auto q = obj->getBuildQueue();
			if (!q || *q != *id) continue;
			auto &entry = this->entries[nid];

			entry.unlocked = false;
			entry.queue = this;
			entry.queueId = id;
			entry.elementId = &nid;
			entry.limit = obj->getLimit();
			entry.buildable = false;
			entry.paused = false;
			entry.buildProgress = 0;
			entry.faction = &faction;
			entry.object = obj;
			entry.upgrade = nullptr;
		}
		for (auto &[nid, obj] : upgrades) {
			auto q = obj->getBuildQueue();
			if (!q || *q != *id) continue;
			auto &entry = this->entries[nid];

			entry.unlocked = false;
			entry.queue = this;
			entry.queueId = id;
			entry.elementId = &nid;
			entry.limit = obj->getLimit();
			entry.buildable = false;
			entry.paused = false;
			entry.buildProgress = 0;
			entry.faction = &faction;
			entry.object = nullptr;
			entry.upgrade = obj;
		}
	}
	if (params->loader) {
		if (params->loader->string().ends_with(".lua"))
			this->customLib = std::make_shared<CustomQueueLua>(&player, this, game->lua, *params->loader);
		else
			this->customLib = std::make_shared<CustomQueueDll>(&player, this, *params->loader);
	}
}

void PlayerQueue::update()
{
	if (this->customLib && !this->customLib->update(*this))
		return;
	switch (this->params->behavior) {
	case FactionQueue::TYPE_STARPORT:
		if (this->timer == 0 && !this->checkingMenu) {
			for (auto &[n, v] : this->stock) {
				if (v < this->params->starportParams->stockMax) {
					auto r = random_distrib(game->gameRandom, 0, 99);

					if (r < this->params->starportParams->stockIncreaseProba)
						v++;
				} else
					v = this->params->starportParams->stockMax;

				float percent = (100 - random_distrib(game->gameRandom, this->params->starportParams->discountMin, this->params->starportParams->discountMax)) / 100.f;

				this->prices[n] = this->entries[n].object->getCost();
				for (auto &p : this->prices[n] | std::ranges::views::values)
					p *= percent;
			}
			this->timer = this->params->starportParams->stockIncreaseDelay;
		} else
			this->timer--;
		break;
	case FactionQueue::TYPE_AUTO_TICK:
		for (auto &[n, q] : this->entries)
			if (q.unlocked)
				this->player->_updateAutoTickQueue(q);
		break;
	default:
		break;
	}
}

unsigned PlayerState::getCombinedMoney(const std::string &key)
{
	auto sys = this->systemMoney[key];
	auto other = this->money[key];

	return sys + other;
}

Player::Player(GameManager &parent, const std::vector<Faction> &factions, size_t index, const HSLColor &color, std::vector<MapState> &mapState) :
	parent(parent),
	mapState(mapState[index]),
	_currentUi(index),
	_color(color),
	_primaryFaction(factions[index]),
	_factions(factions)
{
	this->state.factionIndex = index;
	for (size_t i = 0; i < factions.size(); i++) {
		auto &f = factions[i];

		if (i == index)
			continue;
		for (auto &[key, val] : f.getObjects())
			this->_loadedObjects[key] = &val;
		for (auto &[key, val] : f.getUpgrades())
			this->_loadedUpgrades[key] = &val;
	}
	for (auto &[key, val] : this->_primaryFaction.getObjects())
		this->_loadedObjects[key] = &val;
	for (auto &[key, val] : this->_primaryFaction.getUpgrades())
		this->_loadedUpgrades[key] = &val;

	for (size_t i = 0; i < factions.size(); i++) {
		auto &f = factions[i];

		if (i == index)
			continue;
		for (auto &[key, params] : f.getQueues())
			this->state.queues[key].init(*this, &key, f, &params);
	}
	for (auto &[key, params] : this->_primaryFaction.getQueues())
		this->state.queues[key].init(*this, &key, this->_primaryFaction, &params);

	for (size_t i = 0; i < factions.size(); i++) {
		const Faction &f = factions[i];
		UI *ui = f.createUI(&this->uiState, &parent, &this->state, &mapState[i]);

		this->_uis.emplace_back(ui);
	}
}

Player::~Player()
{
	for (auto &ui : this->_uis)
		if (ui)
			game->gui.remove(ui->getPanel());
}

bool Player::lost() const
{
	return this->_lost;
}

void Player::_clean()
{
	auto it = this->state.myObjects.begin();
	while (it != this->state.myObjects.end()) {
		if ((*it)->isDisposable())
			this->onObjectLost(*it++);
		else
			++it;
	}

	auto mit = this->state.buildQueues.begin();
	while (mit != this->state.buildQueues.end()) {
		if (mit->second && mit->second->buildProgress == 0)
			mit->second = nullptr;
		++mit;
	}

	it = this->state.radars.begin();
	while (it != this->state.radars.end())
		assert_exp(!(*it++)->isDisposable());

	for (auto &[name, list] : this->state.queueBuildings) {
		auto it_ = list.begin();
		while (it_ != list.end())
			assert_exp(!(*it_++)->isDisposable());
	}

	for (auto &[name, b] : this->state.primaryBuildings)
		assert_exp(!b || !b->isDisposable());
}

void Player::_updateQueue(PlayerQueueEntry &entry)
{
	if (entry.queue->customLib && !entry.queue->customLib->update(&entry))
		return;
	switch (entry.queue->params->behavior) {
	case FactionQueue::TYPE_SINGLE_QUEUE:
		return this->_updateSingleQueue(entry);
	case FactionQueue::TYPE_STARPORT:
		return this->_updateStarportQueue(entry);
	default:
		assert_not_reached();
	}
}

void Player::_updateSingleQueue(PlayerQueueEntry &entry)
{
	if (entry.buildProgress == entry.buildTime)
		return this->_tryEndBuildSingle(entry);
	if (entry.paused)
		return;

	double percent = static_cast<double>(entry.buildProgress + 1) / entry.buildTime;
	std::unordered_map<std::string, unsigned> expected;
	std::unordered_map<std::string, unsigned> extra;

	if (entry.object)
		expected = entry.object->getCost();
	else if (entry.upgrade)
		expected = entry.upgrade->getCost();
	else
		assert_not_reached();
	for (auto &f : expected | std::ranges::views::values)
		f = round(f * percent);
	for (auto &[n, f] : expected)
		extra[n] = f - entry.spent[n];
	if (std::ranges::any_of(extra, [this](const std::pair<std::string, unsigned> &p){
		if (this->state.getCombinedMoney(p.first) >= p.second)
			return false;
		this->playEVA(UI::EVA_NO_CREDITS, 15, 2, 1800, p.first);
		return true;
	}))
		return;

	for (auto &p : extra)
		this->consume(p.first, p.second);

	float amount = 1;
	float tmp = 1;
	float energy = (200.f + this->state.producedPower) / (this->state.consumedPower + 200.f);
	size_t size = this->state.queueBuildings[*entry.queueId].size();

	for (auto [count, mul] : entry.queue->params->powerMul) {
		tmp = mul;
		if (count <= energy)
			break;
	}
	amount *= tmp;
	for (auto [count, mul] : entry.queue->params->producerMul) {
		tmp = mul;
		if (count <= size)
			break;
	}
	amount *= tmp;

	entry.buildProgress += amount;
	entry.spent = expected;
	if (entry.buildProgress >= entry.buildTime) {
		this->playEVA(UI::EVA_BUILDING_DONE, EXPIRE_BUILD, PRIORITY_BUILD, 0, *entry.queueId);
		entry.buildProgress = entry.buildTime;
	}
}

void Player::_updateStarportQueue(PlayerQueueEntry &entry)
{
	PlayerQueue &q = *entry.queue;

	assert_exp(q.params->starportParams->deliveryEvaTimes == 5);
	if (entry.buildProgress == entry.buildTime)
		return this->_tryEndBuildStarport(entry);

	unsigned oldP = entry.buildProgress++ / (entry.buildTime / 4);
	unsigned newP = entry.buildProgress / (entry.buildTime / 4);

	if (oldP != newP)
		switch (newP) {
		case 4:
			this->playEVA(UI::EVA_ORDER_IN_1, 300, 4, 300);
			break;
		case 3:
			this->playEVA(UI::EVA_ORDER_IN_2, 300, 4, 300);
			break;
		case 2:
			this->playEVA(UI::EVA_ORDER_IN_3, 300, 4, 300);
			break;
		case 1:
			this->playEVA(UI::EVA_ORDER_IN_4, 300, 4, 300);
			break;
		default:
			break;
		}
}

void Player::_updateAutoTickQueue(PlayerQueueEntry &entry)
{
	if (entry.buildProgress == entry.buildTime)
		return this->_tryEndBuildAutoTick(entry);

	float tmp = 1;
	float amount = 1;
	float energy = (200.f + this->state.producedPower) / (this->state.consumedPower + 200.f);
	size_t size = this->state.queueBuildings[*entry.queueId].size();

	for (auto [count, mul] : entry.queue->params->powerMul) {
		tmp = mul;
		if (count <= energy)
			break;
	}
	amount *= tmp;
	for (auto [count, mul] : entry.queue->params->producerMul) {
		tmp = mul;
		if (count <= size)
			break;
	}
	amount *= tmp;

	entry.paused = amount == 0;
	entry.buildProgress += amount;
	if (entry.buildProgress >= entry.buildTime) {
		this->playEVA(UI::EVA_BUILDING_DONE, EXPIRE_BUILD, PRIORITY_BUILD, 0, *entry.elementId);
		entry.buildProgress = entry.buildTime;
	}
}

void Player::_spawnDelivery(const std::string &queue, Vector2f target, const ExtraSpawn &extra, const std::vector<std::string> &objects)
{
	GameObject *eobj = this->parent.spawnObject(
		this->state.index,
		{0, 0},
		"",
		extra.unit
	);

	assert_exp(extra.dest);
	assert_exp(extra.carry.empty());
	eobj->setPosition(extra.pos.getPosition(target, this->mapState, this->mapState.tileSize + eobj->getSize()));
	eobj->deliverTo(objects, queue, *extra.dest, this->mapState);
	eobj->execCb([queue, this] {
		this->state.queues[queue].busy = false;
		return true;
	});
	if (extra.out)
		eobj->moveOut(extra.out->getPosition(target, this->mapState, this->mapState.tileSize + eobj->getSize()));
}

void Player::_tryEndBuildSingle(PlayerQueueEntry &entry)
{
	if (entry.queue->customLib && !entry.queue->customLib->tryEndBuild(&entry))
		return;
	switch (entry.queue->params->unitSpawn) {
	case FactionQueue::SPAWN_NONE:
		this->state.elementsOwned[*entry.elementId]++;
		entry.spent.clear();
		this->resetQueue(*entry.queueId);
		this->checkUnlocked();
		break;
	case FactionQueue::SPAWN_INSIDE:
		if (!this->parent.spawnObject(this->state.index, {-1, -1}, *entry.queueId, *entry.elementId))
			return;
		entry.spent.clear();
		this->resetQueue(*entry.queueId);
		this->checkUnlocked();
		break;
	case FactionQueue::SPAWN_DELIVERY: {
		GameObject *s = this->getSpawnBuilding(*entry.queueId, false);

		if (s) {
			this->_spawnDelivery(*entry.queueId, s->getPosition() + s->getPoint(), *entry.queue->params->delivery, {*entry.elementId});
			entry.spent.clear();
		}
		break;
	}
	case FactionQueue::SPAWN_PLACEMENT:
		break;
	}
}

void Player::_tryEndBuildStarport(PlayerQueueEntry &entry)
{
	std::vector<std::string> pool;

	for (auto &[n, v] : entry.queue->basket) {
		while (v--)
			pool.push_back(n);
		v = 0;
	}
	this->state.buildQueues[*entry.queueId] = nullptr;

	GameObject *s = this->getSpawnBuilding(*entry.queueId, false);

	if (!s)
		return;
	switch (entry.queue->params->unitSpawn) {
	case FactionQueue::SPAWN_DELIVERY: {
		this->_spawnDelivery(*entry.queueId, s->getPosition() + s->getPoint(), *entry.queue->params->delivery, pool);
		entry.spent.clear();
		break;
	}
	case FactionQueue::SPAWN_NONE:
	case FactionQueue::SPAWN_INSIDE:
	case FactionQueue::SPAWN_PLACEMENT:
		assert_not_reached();
	}
}

void Player::_tryEndBuildAutoTick(PlayerQueueEntry &entry)
{
	this->_tryEndBuildSingle(entry);
}

void Player::update()
{
	PROFILE_SCOPE("update");
	if (this->_primaryFaction.hasRadar() || DEBUG_MODE)
		this->state.hasRadar = true;
	else if (this->state.producedPower < this->state.consumedPower && this->_primaryFaction.isRadarPowered())
		this->state.hasRadar = false;
	else
		this->state.hasRadar = std::ranges::any_of(this->state.radars, [](const GameObject *o) { return !o->isDead(); });
	for (auto &ui : this->_uis) {
		PROFILE_SCOPE("ui_update");
		if (ui)
			ui->update();
	}
	this->_clean();
	for (auto &q : this->state.buildQueues | std::ranges::views::values) {
		PROFILE_SCOPE("update_queue");
		if (!q)
			continue;
		this->_updateQueue(*q);
	}
	for (auto &q : this->state.queues | std::ranges::views::values) {
		PROFILE_SCOPE("queue_update");
		q.update();
	}
	if (this->state.producedPower < this->state.consumedPower)
		this->playEVA(UI::EVA_LOW_POWER, EXPIRE_POWER, PRIORITY_POWER, COOLDOWN_POWER);
	this->_lost = !std::ranges::any_of(this->state.myObjects, [](const GameObject *object) {
		return object->countsForGame();
	});
}

void Player::checkWon()
{
	auto &players = this->parent.getPlayers();

	this->_won = players.size() > 1 && std::ranges::all_of(players, [this](const Player &p) {
		return this->state.team == p.state.team || p.lost();
	});
	for (auto &ui : this->_uis)
		if (ui) {
			ui->won = this->_won;
			ui->lost = this->_lost;
		}
}

void Player::render()
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->render();
}

void Player::renderAbove()
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->renderAbove();
}

void Player::setEnabled(bool enabled)
{
	this->_enabled = enabled;
	for (size_t i = 0; i < this->_uis.size(); i++)
		if (this->_uis[i])
			this->_uis[i]->setEnabled(i == this->_currentUi && enabled);
}

bool Player::isEnabled() const
{
	return this->_enabled;
}

const std::unordered_map<std::string, const GameObject *> &Player::getObjects() const
{
	return this->_loadedObjects;
}

const Faction &Player::getPrimaryFaction() const
{
	return this->_primaryFaction;
}

HSLColor Player::getColor() const
{
	return this->_color;
}

void Player::onMouseMoved(Vector2i pos)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->mouseMoved(pos);
}

void Player::onLeftMouseClicked(Vector2i pos)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->tileClicked(pos);
}

void Player::onRightMouseClicked(Vector2i pos)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->onCancel(pos);
}

void Player::onLeftMouseReleased(Vector2i pos)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->mouseReleased(pos);
}

void Player::onRightMouseReleased(Vector2i)
{
}

bool Player::isMouseOnUi() const
{
	return this->_uis[this->_currentUi]->isMouseOnUi();
}

void Player::keyPressed(const Event::KeyPressed &key)
{
	if (!this->_enabled)
		return;
	// TODO: Hardcoded key
	if (key.code == sf::Keyboard::Key::H) {
		for (auto &b : this->state.myObjects)
			if (b->isCentral()) {
				this->parent.centerCameraOn(*b);
				return;
			}
	}
	this->_uis[this->_currentUi]->onKeyPressed(key.code, key.control, key.shift, key.alt, key.system);
}

void Player::updateRender()
{
	for (auto &ui : this->_uis)
		if (ui)
			ui->updateRender();
}

void Player::_handleUiEvent(const UI::StartBuildEvent &event)
{
	bool b = true;
	assert_msg(this->state.queues.contains(event.queue), event.queue);
	auto &q = this->state.queues.at(event.queue);

	if (q.customLib && !q.customLib->handleStartBuildEvent(this->state.buildQueues[event.queue], event))
		return;
	if (q.params->behavior == FactionQueue::TYPE_STARPORT) {
		if (q.busy || q.providers == 0)
			return;

		if (event.id == "base:order") {
			if (q.nb != 0) {
				auto &entry = q.entries.begin()->second;

				this->playEVA(UI::EVA_ORDER_PLACED, 900, 4, 0);
				this->playEVA(UI::EVA_ORDER_IN_5, 1200, 4, 300);
				this->state.buildQueues[event.queue] = &entry;
				entry.buildTime = q.params->starportParams->deliveryDelay;
				entry.buildProgress = 1;
				q.busy = true;
				q.nb = 0;
			}
			return;
		}
		if (q.nb >= q.params->starportParams->deliveryMax || q.stock.at(event.id) == 0)
			return;
		for (auto &p : q.prices[event.id])
			if (!this->consume(p.first, p.second))
				// TODO: Handle refund the ones before
				return;
		q.stock[event.id]--;
		q.basket[event.id]++;
		q.nb++;
		return;
	}

	auto &entry = q.entries[event.id];

	if ((entry.buildProgress != 0 && !entry.paused) || !entry.buildable) {
		if (this->state.buildQueues[event.queue] != nullptr)
			this->playEVA(UI::EVA_ALREADY_BUILDING, 300, 2, 300);
		return;
	}
	for (auto &[name, entry] : q.entries) {
		if (entry.paused) {
			if (name == event.id)
				entry.paused = false;
			b = false;
			break;
		}
		entry.buildable = false;
	}
	if (b) {
		entry.buildable = true;
		entry.buildProgress = 1;
		this->state.buildQueues[event.queue] = &entry;
	}

	GameObject *building = this->getPrimaryBuilding(event.queue);

	if (building)
		building->producing(true);
	this->playEVA(UI::EVA_BUILDING, EXPIRE_BUILD, PRIORITY_BUILD, 0, event.queue);
}

void Player::_handleUiEvent(const UI::PlaceBuildEvent &event)
{
	assert_msg(this->state.queues.contains(event.queue), event.queue);
	auto &q = this->state.queues.at(event.queue);

	if (q.customLib && !q.customLib->handlePlaceBuildEvent(this->state.buildQueues[event.queue], event))
		return;

	assert_exp(q.params->unitSpawn == FactionQueue::SPAWN_PLACEMENT);
	auto &sq = q.entries[event.id];

	this->parent.spawnObject(this->state.index, event.position, event.queue, event.id);
	sq.spent.clear();
	this->resetQueue(event.queue);
}

void Player::_handleUiEvent(const UI::CancelBuildEvent &event)
{
	assert_msg(this->state.queues.contains(event.queue), event.queue);
	auto &q = this->state.queues.at(event.queue);

	if (q.customLib && !q.customLib->handleCancelBuildEvent(this->state.buildQueues[event.queue], event))
		return;
	if (q.params->behavior == FactionQueue::TYPE_STARPORT) {
		if (q.basket[event.id] == 0)
			return;
		for (auto &p : q.prices[event.id])
			this->credit(p.first, p.second, CREDIT_SYSTEM);
		q.stock[event.id]++;
		q.basket[event.id]--;
		q.nb--;
		return;
	}

	if (this->state.buildQueues[event.queue] == nullptr)
		return;
	auto &sq = q.entries[event.id];

	if (sq.buildProgress == 0);
	else if (sq.paused || sq.buildProgress == sq.buildTime) {
		this->playEVA(UI::EVA_CANCEL, EXPIRE_BUILD, PRIORITY_BUILD, 0, event.queue);
		this->resetQueue(event.queue);
	} else {
		this->playEVA(UI::EVA_HOLD_QUEUE, EXPIRE_BUILD, PRIORITY_BUILD, 0, event.queue);
		sq.paused = true;
	}
}

void Player::_handleUiEvent(const UI::DeployUnitEvent &event)
{
	GameObject *obj = this->parent.getObjectById(event.index);

	// The countdown has started: nothing can interrupt it, not even deploying again.
	if (obj->isSelfDestructing())
		return;
	// Activating a stealth is not an order: the unit keeps doing what it was doing.
	if (obj->deployKeepsOrders()) {
		obj->deploy();
		return;
	}
	obj->clearOrders();
	obj->deploy();
	obj->playDoSound();
}

void Player::_handleUiEvent(const UI::MoveUnitEvent &event)
{
	for (unsigned i : event.indices) {
		GameObject *obj = this->parent.getObjectById(i);

		if (obj->isSelfDestructing())
			continue;
		obj->clearOrders();
		obj->moveTo(event.position, MOVEREASON_MOVE_COMMAND);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::HarvestUnitEvent &event)
{
	for (unsigned i : event.indices) {
		GameObject *obj = this->parent.getObjectById(i);

		if (obj->isSelfDestructing())
			continue;
		obj->clearPrimaryDump();
		obj->clearOrders();
		obj->harvest(event.position, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::DumpUnitEvent &event)
{
	GameObject *hole = this->parent.getObjectById(event.hole);

	for (unsigned i : event.indices) {
		GameObject *obj = this->parent.getObjectById(i);

		if (obj->isSelfDestructing())
			continue;
		obj->clearOrders();
		obj->dumpResourcesTo(hole, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::SetPrimaryEvent &event)
{
	GameObject *building = this->parent.getObjectById(event.index);
	std::optional<std::string> gq = building->getGivenQueue();
	assert_exp(gq);
	GameObject *old = this->state.primaryBuildings[*gq];
	bool producing = false;

	if (old)
		old->setPrimary(false);
	for (GameObject *o : this->state.queueBuildings[*gq]) {
		producing |= o->isProducing();
		o->producing(false);
	}
	this->state.primaryBuildings[*gq] = building;
	building->setPrimary();
	building->producing(producing);
	this->playEVA(UI::EVA_PRIMARY_SELECTED, 120, 1, 0, *gq);
}

void Player::_handleUiEvent(const UI::RepairEvent &event)
{
	this->parent.getObjectById(event.index)->toggleRepair();
}

void Player::_handleUiEvent(const UI::SellEvent &event)
{
	this->parent.getObjectById(event.index)->sell();
	this->playEVA(UI::EVA_SELL, 120, 1, 0);
}

void Player::_handleUiEvent(const UI::AttackUnitEvent &event)
{
	GameObject *target = this->parent.getObjectById(event.target);

	for (unsigned o : event.indices) {
		GameObject *obj = this->parent.getObjectById(o);

		if (obj->isSelfDestructing())
			continue;
		obj->clearOrders();
		obj->attack(target, false, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::StopEvent &event)
{
	for (unsigned i : event.indices) {
		GameObject *obj = this->parent.getObjectById(i);

		if (obj->isSelfDestructing())
			continue;
		obj->clearOrders();
	}
}

void Player::_handleUiEvent(const UI::OpenQueueEvent &event)
{
	assert_msg(this->state.queues.contains(event.queue), event.queue);
	this->state.queues.at(event.queue).checkingMenu = true;
}

void Player::_handleUiEvent(const UI::CloseQueueEvent &event)
{
	assert_msg(this->state.queues.contains(event.queue), event.queue);

	auto &q = this->state.queues.at(event.queue);

	if (!q.busy)
		for (auto &b : q.basket) {
			for (auto &[c, v] : q.prices[b.first])
				this->credit(c, v * b.second, CREDIT_SYSTEM);
			q.stock[b.first] += b.second;
			b.second = 0;
		}
	q.checkingMenu = false;
	q.nb = 0;
}

void Player::handleUiEvents()
{
	for (auto &ui : this->_uis) {
		if (!ui)
			continue;
		while (auto event = ui->pollEvent())
			std::visit([this](auto &&arg) {
				this->_handleUiEvent(arg);
			}, *event);
	}
}

void Player::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->playEVA(sound, expireIn, priority, cooldown, args);
}

void Player::recomputeLocked()
{
	for (auto &list : this->state.queues | std::ranges::views::values) {
		if (list.params->behavior == FactionQueue::TYPE_STARPORT)
			continue;
		for (auto &q : list.entries | std::ranges::views::values) {
			if (!q.faction->regressesTech())
				continue;
			if (q.unlocked && (
				(q.object && !q.object->getRequirements()(this->state.elementsOwned)) ||
				(q.upgrade && !q.upgrade->getRequirements()(this->state.elementsOwned))
			)) {
				this->refreshQueue(*q.queueId);
				break;
			}
		}
	}
}

void Player::checkUnlocked()
{
	for (auto &[name, object] : this->_loadedObjects)
		this->unlockObject(name, *object);
	for (auto &[name, upgrade] : this->_loadedUpgrades)
		this->unlockObject(name, *upgrade);
}

void Player::lockQueue(const std::string &queueId)
{
	assert_msg(this->state.queues.contains(queueId), queueId);
	for (auto &[name, q] : this->state.queues.at(queueId).entries) {
		q.buildable = false;
		q.unlocked = false;
		q.paused = false;
		q.buildProgress = 0;
		for (auto &p : q.spent)
			this->credit(p.first, p.second, CREDIT_SYSTEM);
		q.spent.clear();
	}
}

void Player::resetQueue(const std::string &queueId)
{
	GameObject *building = this->getPrimaryBuilding(queueId);

	if (building)
		building->producing(false);
	assert_msg(this->state.queues.contains(queueId), queueId);
	for (auto &[name, q] : this->state.queues.at(queueId).entries) {
		q.buildable = this->state.elementsOwned[name] < q.limit;
		q.paused = false;
		q.buildProgress = 0;
		for (auto &p : q.spent)
			this->credit(p.first, p.second, CREDIT_SYSTEM);
		q.spent.clear();
	}
}

void Player::refreshQueue(const std::string &queueId)
{
	assert_msg(this->state.queues.contains(queueId), queueId);
	PlayerQueue &pq = this->state.queues.at(queueId);

	for (auto &[name, q] : pq.entries) {
		auto &req = q.upgrade ? q.upgrade->getRequirements() : q.object->getRequirements();

		if (!req(this->state.elementsOwned)) {
			q.unlocked = false;
			q.buildable = this->state.elementsOwned[name] < q.limit;
			q.paused = false;
			q.buildProgress = 0;
			for (auto &p : q.spent)
				this->credit(p.first, p.second, CREDIT_SYSTEM);
			q.spent.clear();
		}
	}

	auto it = this->state.buildQueues.find(queueId);
	if (it != this->state.buildQueues.end() && it->second) {
		auto &req = it->second->upgrade ? it->second->upgrade->getRequirements() : it->second->object->getRequirements();

		if (!req(this->state.elementsOwned))
			return this->resetQueue(queueId);
	}
}

bool Player::credit(const std::string &type, unsigned int amount, CreditStrategy strategy)
{
	if (amount == 0)
		return true;
	switch (strategy) {
	case CREDIT_SYSTEM:
		this->playEVA(UI::EVA_CASH_UP, 1, 0, 0);
		this->state.systemMoney[type] += amount;
		return true;
	default:
		unsigned result = this->state.money[type] + amount;

		if (result <= this->state.totalStorage[type]) {
			this->playEVA(UI::EVA_CASH_UP, 1, 0, 0);
			this->state.money[type] = result;
			return true;
		}
		if (strategy == CREDIT_SPILL) {
			this->playEVA(UI::EVA_REQUIRE_STORAGE, 300, 1, COOLDOWN_POWER);
			this->state.money[type] = this->state.totalStorage[type];
		}
		return false;
	}
}

bool Player::consume(const std::string &type, unsigned int amount)
{
	if (amount == 0)
		return true;

	auto &stored = this->state.money[type];

	if (stored >= amount) {
		this->playEVA(UI::EVA_CASH_DOWN, 1, 0, 0);
		stored -= amount;
		return true;
	}

	auto &system = this->state.systemMoney[type];

	if (stored + system < amount)
		return false;
	this->playEVA(UI::EVA_CASH_DOWN, 1, 0, 0);
	system -= amount - stored;
	stored = 0;
	return true;
}

std::vector<std::shared_ptr<UI>> &Player::getUIs()
{
	return this->_uis;
}

GameObject *Player::getPrimaryBuilding(const std::string &queue)
{
	auto it = this->state.primaryBuildings.find(queue);

	if (it != this->state.primaryBuildings.end() && it->second)
		return it->second;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			return p;
	return nullptr;
}

GameObject *Player::getSpawnBuilding(const std::string &queue, bool stackable)
{
	auto it = this->state.primaryBuildings.find(queue);

	if (it != this->state.primaryBuildings.end() && it->second && !it->second->isObstructed(stackable))
		return it->second;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			if (!p->isObstructed(stackable))
				return p;
	return nullptr;
}

void Player::onObjectLost(GameObject *obj)
{
	auto it1 = std::ranges::find(this->state.myObjects, obj);
	auto power = obj->getPower();
	auto queue = obj->getGivenQueue();
	auto &group = obj->getGroup();
	auto &id = obj->getId();
	auto &storage = obj->getStorage();

	assert_exp(it1 != this->state.myObjects.end());
	this->state.myObjects.erase(it1);

	if (power > 0)
		this->state.consumedPower -= power;
	else
		this->state.producedPower += power;
	for (auto &pair : storage)
		this->state.totalStorage[pair.first] -= pair.second;
	if (obj->isRadar()) {
		it1 = std::ranges::find(this->state.radars, obj);
		assert_exp(it1 != this->state.radars.end());
		this->state.radars.erase(it1);
	}
	if (queue) {
		assert_msg(this->state.queues.contains(*queue), *queue);

		auto &pq = this->state.queues.at(*queue);

		pq.providers--;
		if (pq.providers == 0) {
			if (pq.params->behavior == FactionQueue::TYPE_STARPORT) {
				if (!pq.busy) {
					if (pq.checkingMenu)
						for (auto &b : pq.basket) {
							for (auto &[c, v] : pq.prices[b.first])
								this->credit(c, v * b.second, CREDIT_SYSTEM);
							pq.stock[b.first] += b.second;
							b.second = 0;
						}
				} else {
					pq.busy = false;
					pq.nb = 0;
					pq.timer = 0;
				}
				this->state.buildQueues[*queue] = nullptr;
			} else if (this->state.buildQueues[*queue]) {
				this->resetQueue(*queue);
				this->state.buildQueues[*queue] = nullptr;
			}
		}

		auto &q = this->state.queueBuildings[*queue];
		bool empty = q.empty();
		auto it = std::ranges::find(q, obj);

		assert_exp(it != q.end());
		q.erase(it);
		if (!empty && q.empty() && !DEBUG_MODE)
			this->lockQueue(*queue);
		if (this->state.primaryBuildings[*queue] == obj)
			this->state.primaryBuildings[*queue] = nullptr;
	}

	--this->state.elementsOwned[group];
	if (group != id)
		--this->state.elementsOwned[id];
	if (!DEBUG_MODE)
		this->recomputeLocked();
}

void Player::onObjectGained(GameObject *obj)
{
	assert_exp(std::ranges::find(this->state.myObjects, obj) == this->state.myObjects.end());

	auto power = obj->getPower();
	auto queue = obj->getGivenQueue();
	auto &group = obj->getGroup();
	auto &id = obj->getId();
	auto &storage = obj->getStorage();

	this->state.myObjects.push_back(obj);
	if (power > 0)
		this->state.consumedPower += power;
	else
		this->state.producedPower -= power;
	for (auto &pair : storage)
		this->state.totalStorage[pair.first] += pair.second;
	if (obj->isRadar())
		this->state.radars.push_back(obj);
	if (queue) {
		assert_msg(this->state.queues.contains(*queue), *queue);
		this->state.queues.at(*queue).providers++;
		this->state.queueBuildings[*queue].push_back(obj);
	}

	++this->state.elementsOwned[group];
	if (group != id)
		++this->state.elementsOwned[id];

	this->checkUnlocked();
}

std::vector<GameObject *> Player::getObjects(const std::string &id)
{
	std::vector<GameObject *> obj;

	for (auto &o : this->state.myObjects)
		if (o->getId() == id)
			obj.push_back(o);
	return obj;
}

const std::unordered_map<std::string, const GameObject *> & Player::getLoadedObjects() const
{
	return this->_loadedObjects;
}

const std::unordered_map<std::string, const Upgrade *> & Player::getLoadedUpgrades() const
{
	return this->_loadedUpgrades;
}

template<typename T>
void Player::unlockObject(const std::string &name, const T &object, bool debugTime)
{
	auto &bqueue = object.getBuildQueue();

	if (!bqueue)
		return;

	assert_msg(this->state.queues.contains(*bqueue), *bqueue);
	PlayerQueue &queue = this->state.queues.at(*bqueue);

	if (!debugTime) {
		if (this->state.queueBuildings[*bqueue].empty() && queue.params->requiresProducer)
			return;
		if (!object.getRequirements()(this->state.elementsOwned))
			return;
	}

	assert_msg(queue.entries.contains(name), name);
	PlayerQueueEntry &entry = queue.entries[name];

	if (!debugTime && entry.unlocked)
		return;

	entry.unlocked = true;
	if (this->state.buildQueues[*bqueue] != &entry) {
		entry.limit = object.getLimit();
		entry.buildable = this->state.buildQueues[*bqueue] == nullptr && this->state.elementsOwned[name] < entry.limit;
		entry.paused = false;
	} else
		entry.buildProgress = 0;
	entry.buildTime = debugTime ? 1 : object.getBuildSpeed();
	entry.queue = &queue;
	if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*entry.object)>>) {
		entry.object = &object;
		entry.upgrade = nullptr;
	} else if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*entry.upgrade)>>) {
		entry.object = nullptr;
		entry.upgrade = &object;
	} else
		static_assert(false);
	entry.elementId = &name;
	entry.queueId = &*bqueue;
	this->playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
}

// Only these 2 types are valid for that template
template void Player::unlockObject(const std::string &name, const Upgrade &object, bool debugTime);
template void Player::unlockObject(const std::string &name, const GameObject &object, bool debugTime);

void PlayerQueueEntry::pushUserType(sol::state &state)
{
	sol::usertype<PlayerQueueEntry> type = state.new_usertype<PlayerQueueEntry>("PlayerQueueEntry");

	type["queue"] = &PlayerQueueEntry::queue;
	type["faction"] = &PlayerQueueEntry::faction;
	type["upgrade"] = &PlayerQueueEntry::upgrade;
	type["object"] = &PlayerQueueEntry::object;
	type["queue_id"] = sol::property([](PlayerQueueEntry &entry) -> const std::string & {
		return *entry.queueId;
	});
	type["element_id"] = sol::property([](PlayerQueueEntry &entry) -> const std::string & {
		return *entry.elementId;
	});
	type["spent"] = &PlayerQueueEntry::spent;
	type["build_time"] = &PlayerQueueEntry::buildTime;
	type["build_progress"] = &PlayerQueueEntry::buildProgress;
	type["limit"] = &PlayerQueueEntry::limit;
	type["unlocked"] = &PlayerQueueEntry::unlocked;
	type["buildable"] = &PlayerQueueEntry::buildable;
	type["paused"] = &PlayerQueueEntry::paused;
}

void PlayerQueue::pushUserType(sol::state &state)
{
	PlayerQueueEntry::pushUserType(state);

	sol::usertype<PlayerQueue> type = state.new_usertype<PlayerQueue>("PlayerQueue");

	type["entries"] = &PlayerQueue::entries;
	type["params"] = &PlayerQueue::params;
	type["providers"] = &PlayerQueue::providers;
	type["timer"] = &PlayerQueue::timer;
	type["nb"] = &PlayerQueue::nb;
	type["stock"] = &PlayerQueue::stock;
	type["basket"] = &PlayerQueue::basket;
	type["prices"] = &PlayerQueue::prices;
	type["checking_menu"] = &PlayerQueue::checkingMenu;
	type["busy"] = &PlayerQueue::busy;
	type["update"] = &PlayerQueue::update;
	// NOTE: init() is intentionally not exposed, it's an internal setup
	// routine called by the engine while loading a Player, not meant to be
	// called from scripts.
}

void PlayerState::pushUserType(sol::state &state)
{
	PlayerQueue::pushUserType(state);

	sol::usertype<PlayerState> type = state.new_usertype<PlayerState>("PlayerState");

	type["team"] = &PlayerState::team;
	type["index"] = &PlayerState::index;
	type["action_index"] = &PlayerState::factionIndex;
	type["has_radar"] = &PlayerState::hasRadar;
	type["consumed_power"] = &PlayerState::consumedPower;
	type["produced_power"] = &PlayerState::producedPower;
	type["money"] = &PlayerState::money;
	type["system_money"] = &PlayerState::systemMoney;
	type["total_storage"] = &PlayerState::totalStorage;
	type["elements_owned"] = &PlayerState::elementsOwned;
	type["queue_buildings"] = &PlayerState::queueBuildings;
	type["primary_buildings"] = &PlayerState::primaryBuildings;
	type["my_objects"] = &PlayerState::myObjects;
	type["radars"] = &PlayerState::radars;
	type["build_queues"] = &PlayerState::buildQueues;
	type["queues"] = &PlayerState::queues;
	type["get_combined_money"] = &PlayerState::getCombinedMoney;
}

void UIState::pushUserType(sol::state &state)
{
	sol::usertype<UIState> type = state.new_usertype<UIState>("UIState");

	type["selected_objects"] = &UIState::selectedObjects;
}

void Player::pushUserType(sol::state &state)
{
	state.new_enum<Player::CreditStrategy>("CreditStrategy", {
		{"SYSTEM", Player::CREDIT_SYSTEM},
		{"SPILL",  Player::CREDIT_SPILL},
		{"STRICT", Player::CREDIT_STRICT},
	});

	PlayerState::pushUserType(state);
	UIState::pushUserType(state);

	sol::usertype<Player> type = state.new_usertype<Player>("Player");

	type["state"] = &Player::state;
	type["ui_state"] = &Player::uiState;
	type["debug_unlock"] = &Player::debugUnlock;
	type["update"] = &Player::update;
	type["update_render"] = &Player::updateRender;
	type["render"] = &Player::render;
	type["render_above"] = &Player::renderAbove;
	type["handle_ui_events"] = &Player::handleUiEvents;
	type["color"] = sol::property(&Player::getColor);
	type["mouse_on_ui"] = sol::property(&Player::isMouseOnUi);
	type["primary_faction"] = sol::property(&Player::getPrimaryFaction);
	type["play_eva"] = &Player::playEVA;
	type["recompute_locked"] = &Player::recomputeLocked;
	type["check_unlocked"] = &Player::checkUnlocked;
	type["lock_queue"] = &Player::lockQueue;
	type["reset_queue"] = &Player::resetQueue;
	type["on_mouse_moved"] = &Player::onMouseMoved;
	type["on_left_mouse_clicked"] = &Player::onLeftMouseClicked;
	type["on_right_mouse_clicked"] = &Player::onRightMouseClicked;
	type["credit"] = &Player::credit;
	type["consume"] = &Player::consume;
	type["get_primary_building"] = &Player::getPrimaryBuilding;
	type["get_spawn_building"] = &Player::getSpawnBuilding;
	type["on_object_lost"] = &Player::onObjectLost;
	type["on_object_gained"] = &Player::onObjectGained;
	type["get_objects"] = sol::overload(
		sol::resolve<const std::unordered_map<std::string, const GameObject *> &() const>(&Player::getObjects),
		sol::resolve<std::vector<GameObject *>(const std::string &)>(&Player::getObjects)
	);
	type["enabled"] = sol::property(&Player::isEnabled, &Player::setEnabled);
	// NOTE: keyPressed() (raw sf::Event::KeyPressed, not wrapped for Lua),
	// unlockObject() (template, can't be bound generically) and getUIs()
	// (vector<unique_ptr<UI>>, ownership not meant to be handed to scripts)
	// are intentionally not exposed.
}
