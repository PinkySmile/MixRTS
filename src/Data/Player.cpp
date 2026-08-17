//
// Created by PinkySmile on 06/04/2026.
//

#include "Player.hpp"
#include "Faction.hpp"
#include "Upgrade.hpp"
#include "GameManager.hpp"
#include "../Resources/Game.hpp"
#include "../UI.hpp"
#include "Resources/Assert.hpp"

#define PRIORITY_BUILD 3
#define EXPIRE_BUILD 300
#define PRIORITY_POWER 0
#define EXPIRE_POWER 1800
#define COOLDOWN_POWER 1800

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

		this->_uis.emplace_back(f.createUI());
		this->_uis.back()->manager = &parent;
		this->_uis.back()->state = &this->uiState;
		this->_uis.back()->ownerState = &this->state;
		this->_uis.back()->map = &mapState[i];
		game->gui.add(this->_uis.back()->getPanel());
		if (i == index)
			continue;
		for (auto &[key, val] : f.getObjects())
			this->_loadedObjects[key] = &val;
	}
	for (auto &[key, val] : this->_primaryFaction.getObjects())
		this->_loadedObjects[key] = &val;
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

void Player::update()
{
	if (this->_primaryFaction.hasRadar())
		this->state.hasRadar = true;
	else if (this->state.producedPower < this->state.consumedPower && this->_primaryFaction.isRadarPowered())
		this->state.hasRadar = false;
	else
		this->state.hasRadar = std::ranges::any_of(this->state.radars, [](const GameObject *o) { return !o->isDead(); });
	for (auto &ui : this->_uis)
		ui->update();
	this->_clean();
	for (auto &[qname, q] : this->state.buildQueues) {
		if (!q)
			continue;
		if (q->buildProgress == q->buildTime) {
			this->_tryEndBuild(qname, *q);
		} else if (!q->paused) {
			double percent = static_cast<double>(q->buildProgress + 1) / q->buildTime;
			std::unordered_map<std::string, unsigned> expected;
			std::unordered_map<std::string, unsigned> extra;

			if (q->object)
				expected = q->object->getCost();
			else if (q->upgrade)
				expected = q->upgrade->getCost();
			for (auto &f : expected | std::ranges::views::values)
				f = round(f * percent);
			for (auto &[n, f] : expected)
				extra[n] = f - q->spent[n];
			if (std::ranges::any_of(extra, [this](const std::pair<std::string, unsigned> &p){
				if (this->state.getCombinedMoney(p.first) >= p.second)
					return false;
				this->playEVA(UI::EVA_NO_CREDITS, 15, 2, 1800, p.first);
				return true;
			}))
				continue;

			for (auto &p : extra)
				this->consume(p.first, p.second);

			unsigned amount;

			percent = 100 + (static_cast<double>(this->state.producedPower) - this->state.consumedPower) / 2;
			if (percent >= 100)
				amount = 8;
			else if (percent >= 50)
				amount = 4;
			else
				amount = 2;
			switch (this->state.queueBuildings[qname].size()) {
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
			q->buildProgress += amount;
			q->spent = expected;
			if (q->buildProgress >= q->buildTime) {
				this->playEVA(UI::EVA_BUILDING_DONE, EXPIRE_BUILD, PRIORITY_BUILD, 0, qname);
				q->buildProgress = q->buildTime;
			}
		}
	}
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
		ui->updateRender();
}

void Player::handleUiEvents()
{
	for (auto &ui : this->_uis)
		while (auto event = ui->pollEvent()) {
			if (auto place = std::get_if<UI::PlaceBuildEvent>(&*event)) {
				auto &sq = this->state.queues[place->queue][place->id];

				this->_parent.spawnObject(this->state.index, place->position, place->queue, place->id);
				sq.spent.clear();
				this->resetQueue(place->queue);
			} else if (auto startBuild = std::get_if<UI::StartBuildEvent>(&*event)) {
				bool b = true;
				auto &queue = this->state.queues[startBuild->queue][startBuild->id];

				if ((queue.buildProgress != 0 && !queue.paused) || !queue.buildable) {
					if (this->state.buildQueues[startBuild->queue] != nullptr)
						this->playEVA(UI::EVA_ALREADY_BUILDING, 300, 2, 300);
					continue;
				}
				for (auto &[name, q] : this->state.queues[startBuild->queue]) {
					if (q.paused) {
						if (name == startBuild->id)
							q.paused = false;
						b = false;
						break;
					}
					q.buildable = false;
				}
				if (b) {
					queue.buildable = true;
					queue.buildProgress = 1;
					this->state.buildQueues[startBuild->queue] = &queue;
				}

				GameObject *building = this->getPrimaryBuilding(startBuild->queue);

				if (building)
					building->producing(true);
				this->playEVA(UI::EVA_BUILDING, EXPIRE_BUILD, PRIORITY_BUILD, 0, startBuild->queue);
			} else if (auto cancelBuild = std::get_if<UI::CancelBuildEvent>(&*event)) {
				if (this->state.buildQueues[cancelBuild->queue] == nullptr)
					continue;

				auto &sq = this->state.queues[cancelBuild->queue][cancelBuild->id];

				if (sq.buildProgress == 0);
				else if (sq.paused || sq.buildProgress == sq.buildTime) {
					this->playEVA(UI::EVA_CANCEL, EXPIRE_BUILD, PRIORITY_BUILD, 0, cancelBuild->queue);
					this->resetQueue(cancelBuild->queue);
				} else {
					this->playEVA(UI::EVA_HOLD_QUEUE, EXPIRE_BUILD, PRIORITY_BUILD, 0, cancelBuild->queue);
					sq.paused = true;
				}
			} else if (auto deploy = std::get_if<UI::DeployUnitEvent>(&*event)) {
				auto obj = this->_parent.getObjectById(deploy->index);

				obj->clearOrders();
				obj->deploy();
				obj->playDoSound();
			} else if (auto move = std::get_if<UI::MoveUnitEvent>(&*event)) {
				for (auto i: move->indices) {
					auto obj = this->_parent.getObjectById(i);

					obj->clearOrders();
					obj->moveTo(move->position);
					obj->playDoSound();
				}
			} else if (auto harvest = std::get_if<UI::HarvestUnitEvent>(&*event)) {
				for (auto i: harvest->indices) {
					auto obj = this->_parent.getObjectById(i);

					obj->clearPrimaryDump();
					obj->clearOrders();
					obj->harvest(harvest->position);
					obj->playDoSound();
				}
			} else if (auto dump = std::get_if<UI::DumpUnitEvent>(&*event)) {
				GameObject *hole = this->_parent.getObjectById(dump->hole);

				for (auto i: dump->indices) {
					auto obj = this->_parent.getObjectById(i);

					obj->clearOrders();
					obj->dumpResourcesTo(hole);
					obj->playDoSound();
				}
			} else if (auto primary = std::get_if<UI::SetPrimaryEvent>(&*event)) {
				GameObject *building = this->_parent.getObjectById(primary->index);
				auto gq = building->getGivenQueue();
				assert_exp(gq);
				auto old = this->state.primaryBuildings[*gq];
				auto producing = false;

				if (old)
					old->setPrimary(false);
				for (auto &o : this->state.queueBuildings[*gq]) {
					producing |= o->isProducing();
					o->producing(false);
				}
				this->state.primaryBuildings[*gq] = building;
				building->setPrimary();
				building->producing(producing);
				this->playEVA(UI::EVA_PRIMARY_SELECTED, 120, 1, 0, *gq);
			} else if (auto repair = std::get_if<UI::RepairEvent>(&*event)) {
				this->_parent.getObjectById(repair->index)->toggleRepair();
			} else if (auto sell = std::get_if<UI::SellEvent>(&*event)) {
				this->_parent.getObjectById(sell->index)->sell();
				this->playEVA(UI::EVA_SELL, 120, 1, 0);
			} else if (auto attack = std::get_if<UI::AttackUnitEvent>(&*event)) {
				auto target = this->_parent.getObjectById(attack->target);
				for (unsigned o : attack->indices) {
					GameObject *obj = this->_parent.getObjectById(o);

					obj->clearOrders();
					obj->attack(target);
					obj->playDoSound();
				}
			} else if (auto stop = std::get_if<UI::StopEvent>(&*event)) {
				for (auto i: stop->indices)
					this->_parent.getObjectById(i)->clearOrders();
			} else
				assert_not_reached_msg("Not implemented: Event index " + std::to_string(event->index()));
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
		for (auto &q : list | std::ranges::views::values) {
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

void Player::checkUnlocked(const Faction &faction)
{
	for (auto &[name, object] : faction.getObjects())
		this->unlockObject(faction, name, object);
	for (auto &[name, upgrade] : faction.getUpgrades())
		this->unlockObject(faction, name, upgrade);
}

void Player::_tryEndBuild(const std::string &name, PlayerQueue &queue)
{
	if (queue.upgrade) {
		this->state.elementsOwned[*queue.elementId]++;
		queue.spent.clear();
		this->resetQueue(*queue.queueId);
		this->checkUnlocked(*queue.faction);
	} else if (queue.object) {
		if (
			!queue.object->requiresPlacement() &&
			(!this->getPrimaryBuilding(name) || this->getSpawnBuilding(name))
		) {
			auto defaultPos = this->_parent.getLoadedMap().pathFindingGrid.gridSize / 2;

			this->_parent.spawnObject(this->state.index, defaultPos, *queue.queueId, *queue.elementId);
			queue.spent.clear();
			this->resetQueue(*queue.queueId);
			this->checkUnlocked(*queue.faction);
		}
	} else
		assert_not_reached();
}

void Player::lockQueue(const std::string &queueId)
{
	for (auto &[name, q] : this->state.queues[queueId]) {
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
	for (auto &[name, q] : this->state.queues[queueId]) {
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
		auto &q = this->state.queueBuildings[*queue];
		bool empty = q.empty();
		auto it = std::ranges::find(q, obj);

		assert_exp(it != q.end());
		q.erase(it);
		if (!empty && q.empty())
			this->lockQueue(*queue);
		if (this->state.primaryBuildings[*queue] == obj)
			this->state.primaryBuildings[*queue] = nullptr;
	}

	--this->state.elementsOwned[group];
	if (group != id)
		--this->state.elementsOwned[id];
	// TODO: Bugs out in debug unlocked
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
	if (queue)
		this->state.queueBuildings[*queue].push_back(obj);

	++this->state.elementsOwned[group];
	if (group != id)
		++this->state.elementsOwned[id];

	this->checkUnlocked(obj->getFaction());
}

template<typename T>
void Player::unlockObject(const Faction &f, const std::string &name, T &object, bool debugTime)
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

	auto &queue = this->state.queues[*bqueue][name];

	if (!debugTime && queue.unlocked)
		return;

	queue.unlocked = true;
	queue.limit = object.getLimit();
	queue.buildable = this->state.buildQueues[*bqueue] == nullptr && this->state.elementsOwned[name] < queue.limit;
	queue.paused = false;
	queue.buildProgress = 0;
	queue.buildTime = debugTime ? 1 : object.getBuildSpeed() * 8;
	queue.faction = &f;
	if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*queue.object)>>) {
		queue.object = &object;
		queue.upgrade = nullptr;
	} else if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*queue.upgrade)>>) {
		queue.object = nullptr;
		queue.upgrade = &object;
	} else
		static_assert(false);
	queue.elementId = &name;
	queue.queueId = &*bqueue;
	this->playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
}

// Only these 2 types are valid for that template
template void Player::unlockObject(const Faction &f, const std::string &name, Upgrade &object, bool debugTime);
template void Player::unlockObject(const Faction &f, const std::string &name, GameObject &object, bool debugTime);
