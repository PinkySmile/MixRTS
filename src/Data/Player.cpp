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
#include "Resources/Assert.hpp"

#define PRIORITY_BUILD 3
#define EXPIRE_BUILD 300
#define PRIORITY_POWER 0
#define EXPIRE_POWER 1800
#define COOLDOWN_POWER 1800

void PlayerQueue::init(const std::string *id, const Faction &faction, const FactionQueue *params, const std::unordered_map<std::string, const Upgrade *> &upgrade, const std::unordered_map<std::string, const GameObject *> &objs)
{
	this->params = params;
	if (params->behavior == FactionQueue::TYPE_STARPORT) {
		for (auto &u : params->starportParams->units) {
			assert_msg(objs.contains(u), u);
			const GameObject *obj = objs.at(u);
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
		for (auto &[nid, obj] : objs) {
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
		for (auto &[nid, obj] : upgrade) {
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
}

void PlayerQueue::update()
{
	if (this->params->behavior == FactionQueue::TYPE_STARPORT) {
		if (this->timer == 0 && !this->checkingMenu) {
			for (auto &[n, v] : this->stock) {
				if (v >= this->params->starportParams->stockMax)
					v = this->params->starportParams->stockMax;
				else {
					auto r = random_distrib(game->gameRandom, 0, 99);

					if (r < this->params->starportParams->stockIncreaseProba)
						v++;
				}

				float percent = (100 - random_distrib(game->gameRandom, this->params->starportParams->discountMin, this->params->starportParams->discountMax)) / 100.f;

				this->prices[n] = this->entries[n].object->getCost();
				for (auto &p : this->prices[n] | std::ranges::views::values)
					p *= percent;
			}
			this->timer = this->params->starportParams->stockIncreaseDelay;
		} else
			this->timer--;
	}
}

unsigned PlayerState::getCombinedMoney(const std::string &key)
{
	auto sys = this->systemMoney[key];
	auto other = this->money[key];

	return sys + other;
}

Player::Player(GameManager &parent, const std::vector<Faction> &factions, size_t index, const HSLColor &color, std::vector<MapState> &mapState) :
	_parent(parent),
	_mapState(mapState[index]),
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
			this->state.queues[key].init(&key, f, &params, this->_loadedUpgrades, this->_loadedObjects);
	}
	for (auto &[key, params] : this->_primaryFaction.getQueues())
		this->state.queues[key].init(&key, this->_primaryFaction, &params, this->_loadedUpgrades, this->_loadedObjects);

	for (size_t i = 0; i < factions.size(); i++) {
		const Faction &f = factions[i];
		UI *ui = f.createUI(&this->uiState, &parent, &this->state, &mapState[i]);

		this->_uis.emplace_back(ui);
		game->gui.add(ui->getPanel());
	}
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

	unsigned amount;
	float energy = (200.f + this->state.producedPower) / (this->state.consumedPower + 200.f);

	if (energy >= 1)
		amount = 8;
	else if (energy >= 0.5)
		amount = 4;
	else
		amount = 2;
	switch (this->state.queueBuildings[*entry.queueId].size()) {
	case 0:
	case 1:
		break;
	case 2:
		amount += amount / 2;
		break;
	default:
		amount *= 2;
		break;
	}
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
	entry.buildProgress++;
	if (entry.buildProgress % (entry.buildTime / 4) == 0)
		switch (entry.buildProgress / (entry.buildTime / 4)) {
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

void Player::_spawnDelivery(const std::string &queue, Vector2f target, const ExtraSpawn &extra, const std::vector<std::string> &objects)
{
	GameObject *eobj = this->_parent.spawnObject(
		this->state.index,
		{0, 0},
		"",
		extra.unit
	);

	assert_exp(extra.dest);
	assert_exp(extra.carry.empty());
	eobj->setPosition(extra.pos.getPosition(target, this->_mapState, this->_mapState.tileSize + eobj->getSize()));
	eobj->deliverTo(objects, queue, *extra.dest, this->_mapState);
	eobj->execCb([queue, this] {
		this->state.queues[queue].busy = false;
		return true;
	});
	if (extra.out)
		eobj->moveOut(extra.out->getPosition(target, this->_mapState, this->_mapState.tileSize + eobj->getSize()));
}

void Player::_tryEndBuildSingle(PlayerQueueEntry &entry)
{
	auto defaultPos = this->_parent.getLoadedMap().pathFindingGrid.gridSize / 2;

	switch (entry.queue->params->unitSpawn) {
	case FactionQueue::SPAWN_NONE:
		this->state.elementsOwned[*entry.elementId]++;
		entry.spent.clear();
		this->resetQueue(*entry.queueId);
		this->checkUnlocked();
		break;
	case FactionQueue::SPAWN_INSIDE:
		this->_parent.spawnObject(this->state.index, defaultPos, *entry.queueId, *entry.elementId);
		entry.spent.clear();
		this->resetQueue(*entry.queueId);
		this->checkUnlocked();
		break;
	case FactionQueue::SPAWN_DELIVERY: {
		GameObject *s = this->getSpawnBuilding(*entry.queueId);

		assert_exp(s);
		this->_spawnDelivery(*entry.queueId, s->getPosition() + s->getPoint(), *entry.queue->params->delivery, {*entry.elementId});
		entry.spent.clear();
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

	GameObject *s = this->getSpawnBuilding(*entry.queueId);

	assert_exp(s);
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

void Player::update()
{
	if (this->_primaryFaction.hasRadar())
		this->state.hasRadar = true;
	else if (this->state.producedPower < this->state.consumedPower && this->_primaryFaction.isRadarPowered())
		this->state.hasRadar = false;
	else
		this->state.hasRadar = std::ranges::any_of(this->state.radars, [](const GameObject *o) { return !o->isDead(); });
	for (auto &ui : this->_uis)
		if (ui)
			ui->update();
	this->_clean();
	for (auto &q : this->state.buildQueues | std::ranges::views::values) {
		if (!q)
			continue;
		this->_updateQueue(*q);
	}
	for (auto &q : this->state.queues | std::ranges::views::values)
		q.update();
	if (this->state.producedPower < this->state.consumedPower)
		this->playEVA(UI::EVA_LOW_POWER, EXPIRE_POWER, PRIORITY_POWER, COOLDOWN_POWER);
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
		this->_uis[this->_currentUi]->mouseMovedToTile(pos);
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

void Player::mouseOnUi()
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->mouseOnUi();
}

void Player::keyPressed(const Event::KeyPressed &key)
{
	if (!this->_enabled)
		return;
	// TODO: Hardcoded key
	if (key.code == sf::Keyboard::Key::H) {
		for (auto &b : this->state.myObjects)
			if (b->isCentral()) {
				this->_parent.centerCameraOn(*b);
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
	assert_exp(q.params->unitSpawn == FactionQueue::SPAWN_PLACEMENT);
	auto &sq = q.entries[event.id];

	this->_parent.spawnObject(this->state.index, event.position, event.queue, event.id);
	sq.spent.clear();
	this->resetQueue(event.queue);
}

void Player::_handleUiEvent(const UI::CancelBuildEvent &event)
{
	assert_msg(this->state.queues.contains(event.queue), event.queue);
	auto &q = this->state.queues.at(event.queue);

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
	GameObject *obj = this->_parent.getObjectById(event.index);

	obj->clearOrders();
	obj->deploy();
	obj->playDoSound();
}

void Player::_handleUiEvent(const UI::MoveUnitEvent &event)
{
	for (unsigned i : event.indices) {
		GameObject *obj = this->_parent.getObjectById(i);

		obj->clearOrders();
		obj->moveTo(event.position, MOVEREASON_MOVE_COMMAND);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::HarvestUnitEvent &event)
{
	for (unsigned i : event.indices) {
		GameObject *obj = this->_parent.getObjectById(i);

		obj->clearPrimaryDump();
		obj->clearOrders();
		obj->harvest(event.position, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::DumpUnitEvent &event)
{
	GameObject *hole = this->_parent.getObjectById(event.hole);

	for (unsigned i : event.indices) {
		GameObject *obj = this->_parent.getObjectById(i);

		obj->clearOrders();
		obj->dumpResourcesTo(hole, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::SetPrimaryEvent &event)
{
	GameObject *building = this->_parent.getObjectById(event.index);
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
	this->_parent.getObjectById(event.index)->toggleRepair();
}

void Player::_handleUiEvent(const UI::SellEvent &event)
{
	this->_parent.getObjectById(event.index)->sell();
	this->playEVA(UI::EVA_SELL, 120, 1, 0);
}

void Player::_handleUiEvent(const UI::AttackUnitEvent &event)
{
	GameObject *target = this->_parent.getObjectById(event.target);

	for (unsigned o : event.indices) {
		GameObject *obj = this->_parent.getObjectById(o);

		obj->clearOrders();
		obj->attack(target, false, false);
		obj->playDoSound();
	}
}

void Player::_handleUiEvent(const UI::StopEvent &event)
{
	for (unsigned i : event.indices)
		this->_parent.getObjectById(i)->clearOrders();
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
			if (
				(q.object && !q.object->getRequirements()(this->state.elementsOwned)) ||
				(q.upgrade && !q.upgrade->getRequirements()(this->state.elementsOwned))
			) {
				this->resetQueue(*q.queueId);
				q.unlocked = false;
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

std::vector<std::unique_ptr<UI>> &Player::getUIs()
{
	return this->_uis;
}

GameObject *Player::getPrimaryBuilding(const std::string &queue)
{
	auto parent = this->state.primaryBuildings[queue];

	if (parent)
		return parent;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			return p;
	return nullptr;
}

GameObject *Player::getSpawnBuilding(const std::string &queue)
{
	auto parent = this->state.primaryBuildings[queue];

	if (parent && !parent->isObstructed())
		return parent;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			if (!p->isObstructed())
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
				PlayerQueueEntry *q1 = this->state.buildQueues[*queue];

				for (auto &[c, v] : q1->spent)
					this->credit(c, v, CREDIT_SYSTEM);
				q1->spent.clear();
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

	if (!debugTime) {
		if (this->state.queueBuildings[*bqueue].empty() && object.requiresQueueBuilding())
			return;
		if (!object.getRequirements()(this->state.elementsOwned))
			return;
	}

	assert_msg(this->state.queues.contains(*bqueue), *bqueue);
	auto &queue = this->state.queues.at(*bqueue);
	assert_msg(queue.entries.contains(name), name);
	auto &entry = queue.entries[name];

	if (!debugTime && entry.unlocked)
		return;

	entry.unlocked = true;
	entry.limit = object.getLimit();
	entry.buildable = this->state.buildQueues[*bqueue] == nullptr && this->state.elementsOwned[name] < entry.limit;
	entry.paused = false;
	entry.buildProgress = 0;
	entry.buildTime = debugTime ? 1 : object.getBuildSpeed() * 8;
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
	type["queue_id"] = &PlayerQueueEntry::queueId;
	type["element_id"] = &PlayerQueueEntry::elementId;
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
	type["mouse_on_ui"] = &Player::mouseOnUi;
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
