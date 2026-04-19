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
	return this->systemMoney[key] + this->money[key];
}

Player::Player(GameManager &parent, const std::vector<Faction> &factions, size_t index, const HSLColor &color, std::vector<MapState> &mapState) :
	_index(index),
	_parent(parent),
	_mapState(mapState[index]),
	_currentUi(index),
	_color(color),
	_primaryFaction(factions[index]),
	_factions(factions)
{
	for (size_t i = 0; i < factions.size(); i++) {
		auto &f = factions[i];

		this->_uis.emplace_back(f.createUI());
		this->_uis.back()->state = &this->state;
		this->_uis.back()->map = &mapState[i];
		game->gui.add(this->_uis.back()->getPanel());
		if (i == index)
			continue;
		for (auto &[key, val] : f.getBuildings())
			this->_loadedBuildings[key] = &val;
	}
	for (auto &[key, val] : this->_primaryFaction.getBuildings())
		this->_loadedBuildings[key] = &val;
}

void PlayerState::clean()
{
	auto it = this->myBuildings.begin();

	while (it != this->myBuildings.end())
		if ((*it)->isDisposable())
			it = this->myBuildings.erase(it);
		else
			++it;

	for (auto &list : this->buildingHitboxes | std::ranges::views::values) {
		it = list.begin();
		while (it != list.end())
			if ((*it)->isDisposable())
				it = list.erase(it);
			else
				++it;
	}

	for (auto &list : this->queueBuildings | std::ranges::views::values) {
		it = list.begin();
		while (it != list.end())
			if ((*it)->isDisposable())
				it = list.erase(it);
			else
				++it;
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
				this->_consume(p.first, p.second);
			q->buildProgress++;
			q->spent = expected;
			if (q->buildProgress == q->buildTime)
				this->playEVA(UI::EVA_BUILDING_DONE, EXPIRE_BUILD, PRIORITY_BUILD, 0, qname);
		}
	}
	if (this->state.producedPower < this->state.consumedPower)
		this->playEVA(UI::EVA_LOW_POWER, EXPIRE_POWER, PRIORITY_POWER, COOLDOWN_POWER);
}

void Player::render()
{
	for (auto &ui : this->_uis)
		ui->render();
}

void Player::renderAbove()
{
	this->_uis[this->_currentUi]->renderAbove();
}

void Player::setEnabled(bool enabled)
{
	this->_enabled = enabled;
	for (auto &ui : this->_uis)
		ui->setEnabled(enabled);
}

const std::map<std::string, const Building *> &Player::getBuildings() const
{
	return this->_loadedBuildings;
}

const Faction &Player::getPrimaryFaction() const
{
	return this->_primaryFaction;
}

HSLColor Player::getColor() const
{
	return this->_color;
}

extern float guiScale;

void Player::onMouseMoved(Vector2i pos)
{
	for (auto &ui : this->_uis)
		ui->mouseMovedToTile(ui->map->absToTileCoord(pos));
}

void Player::onLeftMouseClicked(Vector2i pos)
{
	for (auto &ui : this->_uis)
		ui->tileClicked(ui->map->absToTileCoord(pos));
}

void Player::onRightMouseClicked(Vector2i)
{
	for (auto &ui : this->_uis)
		ui->onCancel();
}

void Player::mouseOnUi()
{
	for (auto &ui : this->_uis)
		ui->mouseOnUi();
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
				this->_parent.spawnBuilding(this->_index, place->position, place->queue, place->id, false);
				this->resetQueue(place->queue);
			} else if (auto startBuild = std::get_if<UI::StartBuildEvent>(&*event)) {
				bool b = true;

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
					this->state.queues[startBuild->queue][startBuild->id].buildable = true;
					this->state.queues[startBuild->queue][startBuild->id].buildProgress = 1;
					this->state.buildQueues[startBuild->queue] = &this->state.queues[startBuild->queue][startBuild->id];
				}
				this->playEVA(UI::EVA_BUILDING, EXPIRE_BUILD, PRIORITY_BUILD, 0, startBuild->queue);
			} else if (auto cancelBuild = std::get_if<UI::CancelBuildEvent>(&*event)) {
				if (this->state.buildQueues[cancelBuild->queue] == nullptr)
					continue;

				auto &sq = this->state.queues[cancelBuild->queue][cancelBuild->id];

				for (auto &ui : this->_uis)
					ui->onCancel();
				if (sq.paused || sq.buildProgress == sq.buildTime) {
					this->playEVA(UI::EVA_CANCEL, EXPIRE_BUILD, PRIORITY_BUILD, 0, cancelBuild->queue);
					for (auto &p : sq.spent)
						this->_credit(p.first, p.second, CREDIT_SYSTEM);
					this->resetQueue(cancelBuild->queue);
				} else {
					this->playEVA(UI::EVA_HOLD_QUEUE, EXPIRE_BUILD, PRIORITY_BUILD, 0, cancelBuild->queue);
					sq.paused = true;
				}
			} else
				assert_not_reached_msg("Not implemented: Event index " + std::to_string(event->index()));
		}
}

void Player::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args)
{
	for (auto &ui : this->_uis)
		ui->playEVA(sound, expireIn, priority, cooldown, args);
}

void Player::checkUnlocked(const Faction &faction)
{
	for (auto &[name, building] : faction.getBuildings()) {
		if (building.getRequirements()(this->state.elementsOwned)) {
			auto &bqueue = building.getBuildQueue();

			if (!bqueue)
				continue;

			auto &queue = this->state.queues[*bqueue][name];

			if (queue.unlocked)
				continue;
			queue.unlocked = true;
			queue.limit = building.getLimit();
			queue.buildable = this->state.buildQueues[*bqueue] == nullptr && this->state.elementsOwned[name] < queue.limit;
			queue.paused = false;
			queue.buildProgress = 0;
			queue.buildTime = building.getBuildSpeed();
			queue.faction = &faction;
			queue.building = &building;
			queue.upgrade = nullptr;
			queue.unit = nullptr;
			queue.elementId = &name;
			queue.queueId = &*bqueue;
			this->playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
		}
	}
	for (auto &[name, upgrade] : faction.getUpgrades()) {
		if (upgrade.getRequirements()(this->state.elementsOwned)) {
			auto &bqueue = upgrade.getBuildQueue();

			if (!bqueue)
				continue;

			auto &queue = this->state.queues[*bqueue][name];

			if (queue.unlocked)
				continue;
			queue.unlocked = true;
			queue.limit = upgrade.getLimit();
			queue.buildable = this->state.buildQueues[*bqueue] == nullptr && this->state.elementsOwned[name] < queue.limit;
			queue.paused = false;
			queue.buildProgress = 0;
			queue.buildTime = upgrade.getBuildSpeed();
			queue.faction = &faction;
			queue.building = nullptr;
			queue.upgrade = &upgrade;
			queue.unit = nullptr;
			queue.elementId = &name;
			queue.queueId = &*bqueue;
			this->playEVA(UI::EVA_UNLOCKED, 1800, 3, 60, *bqueue);
		}
	}
}

void Player::_tryEndBuild(PlayerQueue &queue)
{
	if (queue.building) {
		// TODO: For place-first-build-later factions
	} else if (queue.upgrade) {
		this->state.elementsOwned[*queue.elementId]++;
		this->resetQueue(*queue.queueId);
		this->checkUnlocked(*queue.faction);
	} else if (queue.unit) {
		// TODO: Units
	} else
		assert_not_reached();
}

void Player::resetQueue(const std::string &queueId)
{
	for (auto &[name, q] : this->state.queues[queueId]) {
		q.buildable = this->state.elementsOwned[name] < q.limit;
		q.paused = false;
		q.buildProgress = 0;
		q.spent.clear();
	}
}

bool Player::_credit(const std::string &type, unsigned int amount, CreditStrategy strategy)
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
			this->playEVA(UI::EVA_CASH_UP, 1, 0, 0);
			this->state.money[type] = this->state.totalStorage[type];
		}
		return false;
	}
}

bool Player::_consume(const std::string &type, unsigned int amount)
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
