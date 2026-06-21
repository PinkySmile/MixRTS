//
// Created by PinkySmile on 06/04/2026.
//

#include "Player.hpp"
#include "Faction.hpp"
#include "Upgrade.hpp"
#include "Building.hpp"
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
		for (auto &[key, val] : f.getBuildings())
			this->_loadedBuildings[key] = &val;
		for (auto &[key, val] : f.getUnits())
			this->_loadedUnits[key] = &val;
	}
	for (auto &[key, val] : this->_primaryFaction.getBuildings())
		this->_loadedBuildings[key] = &val;
	for (auto &[key, val] : this->_primaryFaction.getUnits())
		this->_loadedUnits[key] = &val;
}

void PlayerState::clean()
{
	auto it = this->myObjects.begin();

	while (it != this->myObjects.end())
		if ((*it)->isDisposable())
			it = this->myObjects.erase(it);
		else
			++it;

	for (auto &list : this->queueBuildings | std::ranges::views::values) {
		auto it_ = list.begin();
		while (it_ != list.end())
			if ((*it_)->isDisposable())
				it_ = list.erase(it_);
			else
				++it_;
	}

	auto mit = this->buildQueues.begin();
	while (mit != this->buildQueues.end()) {
		if (mit->second && mit->second->buildProgress == 0)
			mit->second = nullptr;
		++mit;
	}
	for (auto &[name, b] : this->primaryBuildings)
		if (b && b->isDisposable())
			b = nullptr;
}

void Player::update()
{
	for (auto &ui : this->_uis)
		ui->update();
	this->state.clean();
	for (auto &[qname, q] : this->state.buildQueues) {
		if (!q)
			continue;
		if (q->buildProgress == q->buildTime) {
			this->_tryEndBuild(*q);
		} else if (!q->paused) {
			float percent = static_cast<float>(q->buildProgress + 1) / q->buildTime;
			std::unordered_map<std::string, unsigned> expected;
			std::unordered_map<std::string, unsigned> extra;

			if (q->building)
				expected = q->building->getCost();
			else if (q->upgrade)
				expected = q->upgrade->getCost();
			else if (q->unit)
				expected = q->unit->getCost();
			for (auto &f : expected | std::ranges::views::values)
				f *= percent;
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

			percent = 100 + (static_cast<float>(this->state.producedPower) - this->state.consumedPower) / 2;
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

const std::unordered_map<std::string, const Building *> &Player::getBuildings() const
{
	return this->_loadedBuildings;
}

const std::unordered_map<std::string, const Unit *> &Player::getUnits() const
{
	return this->_loadedUnits;
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

				this->_parent.spawnBuilding(this->state.index, place->position, place->queue, place->id);
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

				Building *building = this->getPrimaryBuilding(startBuild->queue);

				if (building)
					building->producing(true);
				this->playEVA(UI::EVA_BUILDING, EXPIRE_BUILD, PRIORITY_BUILD, 0, startBuild->queue);
			} else if (auto cancelBuild = std::get_if<UI::CancelBuildEvent>(&*event)) {
				if (this->state.buildQueues[cancelBuild->queue] == nullptr)
					continue;

				auto &sq = this->state.queues[cancelBuild->queue][cancelBuild->id];

				if (sq.paused || sq.buildProgress == sq.buildTime) {
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
				checked_cast(hole, Building, this->_parent.getObjectById(dump->hole));

				for (auto i: dump->indices) {
					auto obj = this->_parent.getObjectById(i);

					obj->clearOrders();
					obj->dumpResourcesTo(hole);
					obj->playDoSound();
				}
			} else
				assert_not_reached_msg("Not implemented: Event index " + std::to_string(event->index()));
		}
}

void Player::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args)
{
	if (this->_enabled)
		this->_uis[this->_currentUi]->playEVA(sound, expireIn, priority, cooldown, args);
}

void Player::checkUnlocked(const Faction &faction)
{
	for (auto &[name, building] : faction.getBuildings())
		this->unlockObject(faction, name, building);
	for (auto &[name, upgrade] : faction.getUpgrades())
		this->unlockObject(faction, name, upgrade);
	for (auto &[name, units] : faction.getUnits())
		this->unlockObject(faction, name, units);
}

void Player::_tryEndBuild(PlayerQueue &queue)
{
	if (queue.building) {
		// TODO: For place-first-build-later factions
	} else if (queue.upgrade) {
		this->state.elementsOwned[*queue.elementId]++;
		queue.spent.clear();
		this->resetQueue(*queue.queueId);
		this->checkUnlocked(*queue.faction);
	} else if (queue.unit) {
		this->_parent.spawnUnit(this->state.index, this->_parent.getLoadedMap().pathFindingGrid.gridSize / 2, *queue.queueId, *queue.elementId);
		queue.spent.clear();
		this->resetQueue(*queue.queueId);
		this->checkUnlocked(*queue.faction);
	} else
		assert_not_reached();
}

void Player::resetQueue(const std::string &queueId)
{
	Building *building = this->getPrimaryBuilding(queueId);

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

Building *Player::getPrimaryBuilding(const std::string &queue)
{
	auto parent = this->state.primaryBuildings[queue];

	if (parent)
		return parent;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			return p;
	return nullptr;
}

Building *Player::getSpawnBuilding(const std::string &queue)
{
	auto parent = this->state.primaryBuildings[queue];

	if (parent)
		return parent;
	if (this->state.queueBuildings.contains(queue))
		for (auto p: this->state.queueBuildings[queue])
			if (!p->isObstructed())
				return p;
	return nullptr;
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
	if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*queue.building)>>) {
		queue.building = &object;
		queue.upgrade = nullptr;
		queue.unit = nullptr;
	} else if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*queue.upgrade)>>) {
		queue.building = nullptr;
		queue.upgrade = &object;
		queue.unit = nullptr;
	} else if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<decltype(*queue.unit)>>) {
		queue.building = nullptr;
		queue.upgrade = nullptr;
		queue.unit = &object;
	} else
		static_assert(false);
	queue.elementId = &name;
	queue.queueId = &*bqueue;
	this->playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
}

// Only these 3 types are valid for that template
template void Player::unlockObject(const Faction &f, const std::string &name, Unit &object, bool debugTime);
template void Player::unlockObject(const Faction &f, const std::string &name, Upgrade &object, bool debugTime);
template void Player::unlockObject(const Faction &f, const std::string &name, Building &object, bool debugTime);