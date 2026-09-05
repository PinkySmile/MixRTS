//
// Created by PinkySmile on 01/04/2026.
//

#include "UI.hpp"
#include "../Data/GameManager.hpp"
#include "../Data/Player.hpp"
#include "../Resources/Game.hpp"
#include "Resources/Assert.hpp"


std::string UI::EVAToString(EVASound snd)
{
	switch (snd) {
	case EVA_BUILDING:
		return "EVA_BUILDING";
	case EVA_BUILDING_DONE:
		return "EVA_BUILDING_DONE";
	case EVA_CANCEL:
		return "EVA_CANCEL";
	case EVA_CANNOT_DEPLOY:
		return "EVA_CANNOT_DEPLOY";
	case EVA_CANNOT_BUILD:
		return "EVA_CANNOT_BUILD";
	case EVA_GUARDING:
		return "EVA_GUARDING";
	case EVA_HOLD_QUEUE:
		return "EVA_HOLD_QUEUE";
	case EVA_LOST:
		return "EVA_LOST";
	case EVA_WON:
		return "EVA_WON";
	case EVA_LOW_POWER:
		return "EVA_LOW_POWER";
	case EVA_UNLOCKED:
		return "EVA_UNLOCKED";
	case EVA_NO_CREDITS:
		return "EVA_NO_CREDITS";
	case EVA_PRIMARY_SELECTED:
		return "EVA_PRIMARY_SELECTED";
	case EVA_RETREAT:
		return "EVA_RETREAT";
	case EVA_SELL:
		return "EVA_SELL";
	case EVA_TRAINING:
		return "EVA_TRAINING";
	case EVA_UNIT_READY:
		return "EVA_UNIT_READY";
	case EVA_UPGRADING:
		return "EVA_UPGRADING";
	case EVA_CASH_UP:
		return "EVA_CASH_UP";
	case EVA_CASH_DOWN:
		return "EVA_CASH_DOWN";
	case EVA_REQUIRE_STORAGE:
		return "EVA_REQUIRE_STORAGE";
	case EVA_ALREADY_BUILDING:
		return "EVA_ALREADY_BUILDING";
	case EVA_POWER_OFF:
		return "EVA_POWER_OFF";
	case EVA_POWER_ON:
		return "EVA_POWER_ON";
	case EVA_RADAR_UP:
		return "EVA_RADAR_UP";
	case EVA_MY_BUILDING_CAPTURED:
		return "EVA_MY_BUILDING_CAPTURED";
	case EVA_BUILDING_CAPTURED:
		return "EVA_BUILDING_CAPTURED";
	case EVA_BUILDING_ATTACKED:
		return "EVA_BUILDING_ATTACKED";
	case EVA_BUILDING_LOST:
		return "EVA_BUILDING_LOST";
	case EVA_MY_UNIT_CAPTURED:
		return "EVA_MY_UNIT_CAPTURED";
	case EVA_UNIT_CAPTURED:
		return "EVA_UNIT_CAPTURED";
	case EVA_UNIT_ATTACKED:
		return "EVA_UNIT_ATTACKED";
	case EVA_ALLIED_MY_BUILDING_CAPTURED:
		return "EVA_ALLIED_MY_BUILDING_CAPTURED";
	case EVA_ALLIED_BUILDING_CAPTURED:
		return "EVA_ALLIED_BUILDING_CAPTURED";
	case EVA_ALLIED_BUILDING_ATTACKED:
		return "EVA_ALLIED_BUILDING_ATTACKED";
	case EVA_ALLIED_BUILDING_LOST:
		return "EVA_ALLIED_BUILDING_LOST";
	case EVA_ALLIED_MY_UNIT_CAPTURED:
		return "EVA_ALLIED_MY_UNIT_CAPTURED";
	case EVA_ALLIED_UNIT_CAPTURED:
		return "EVA_ALLIED_UNIT_CAPTURED";
	case EVA_ALLIED_UNIT_ATTACKED:
		return "EVA_ALLIED_UNIT_ATTACKED";
	case EVA_UNIT_LOST:
		return "EVA_UNIT_LOST";
	case EVA_HARV_UNDER_ATTACK:
		return "EVA_HARV_UNDER_ATTACK";
	case EVA_HAZARD_SPAWNED:
		return "EVA_HAZARD_SPAWNED";
	case EVA_HAZARD_ATTACKED:
		return "EVA_HAZARD_ATTACKED";
	default:
		return "Invalid EVA " + std::to_string(snd);
	}
}

UI::UI(const Faction &faction) :
	_faction(faction),
	_evaSound(game->dummySoundBuffer)
{
	this->_hasRadar = faction.hasRadar();
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
}

UI::~UI() = default;

void UI::setEnabled(bool enabled)
{
	this->_panel->setVisible(enabled);
	this->_enabled = enabled;
}

bool UI::isEnabled() const
{
	return this->_enabled;
}

tgui::Panel::Ptr UI::getPanel() const
{
	return this->_panel;
}

std::optional<UI::Event> UI::pollEvent()
{
	if (this->_eventQueue.empty())
		return {};

	std::optional result = this->_eventQueue.front();

	this->_eventQueue.pop_front();
	return result;
}

void UI::playEVA(EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &)
{
	if (!this->_enabled)
		return;
	if (this->_evaCooldown[sound])
		return;

	auto it = this->_evaQueue.begin();

	while (it != this->_evaQueue.end() && it->priority >= priority)
		it++;
	this->_evaQueue.insert(it, {sound, expireIn, priority, cooldown});
}

void UI::onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta)
{
	// TODO: Hardcoded key
	if (key == sf::Keyboard::Key::S && !ctrl && !shift && !alt && !meta) {
		if (!this->state->selectedObjects.empty()) {
			StopEvent event;

			for (auto &s : this->state->selectedObjects)
				event.indices.push_back(s->index);
			this->_eventQueue.emplace_front(event);
		}
	}
}

void UI::update()
{
	auto it = this->state->selectedObjects.begin();

	while (it != this->state->selectedObjects.end())
		if ((*it)->isDisposable() || (*it)->isDead())
			it = this->state->selectedObjects.erase(it);
		else
			++it;

	auto &placement = this->_faction.getPlacementCursors();
	auto &anim = this->_placementAnimation;

	if (!placement[0].empty()) {
		anim[0] = (anim[0] + 1) % placement[0].size();
		anim[1] = (anim[1] + 1) % placement[1].size();
		anim[2] = (anim[2] + 1) % placement[2].size();
	}
	this->_updateCursor();
	if (this->ownerState->hasRadar != this->_hasRadar) {
		this->_hasRadar = this->ownerState->hasRadar;
		if (this->_hasRadar)
			this->playEVA(EVA_RADAR_UP, 300, 1, 0);
	}

	bool lowPower = this->ownerState->consumedPower > this->ownerState->producedPower;

	if (lowPower != this->_hasLowPower) {
		this->_hasLowPower = lowPower;
		this->playEVA(lowPower ? EVA_POWER_OFF : EVA_POWER_ON, 300, 1, 0);
	}
	this->_settings->setVisible(game->paused);
	for (auto &i : this->_evaCooldown)
		if (i)
			i--;
	if (!this->_enabled)
		return this->_evaQueue.clear();
	if (this->_evaQueue.empty()) {
		if (this->_globalEvaCooldown != 0)
			this->_globalEvaCooldown--;
		return;
	}
	if (this->_evaSound.getStatus() != sf::Sound::Status::Playing) {
		if (this->_globalEvaCooldown == 0) {
			auto &e = this->_evaQueue.front();

			game->logger.verbose("Playing EVA sound " + std::to_string(e.soundId));
			assert_msg(this->_evaSounds[e.soundId], "Trying to play invalid EVA " + EVAToString(e.soundId));
			this->_evaSound.setBuffer(*this->_evaSounds[e.soundId]);
			this->_evaSound.play();
			this->_evaCooldown[e.soundId] = e.cooldown;
			this->_evaQueue.pop_front();
			this->_globalEvaCooldown = 30;
		} else
			this->_globalEvaCooldown--;
	}

	for (auto &q : this->_evaQueue)
		if (q.expireTimer != 0)
			q.expireTimer--;
	while (!this->_evaQueue.empty() && (
		this->_evaQueue.front().expireTimer == 0 ||
		this->_evaCooldown[this->_evaQueue.front().soundId]
	)) {
		auto &f = this->_evaQueue.front();

		game->logger.verbose("Expiring EVA sound " +
			std::to_string(f.soundId) + " " +
			std::to_string(static_cast<int>(this->_evaSound.getStatus())) + " " +
			std::to_string(this->_globalEvaCooldown)
		);
		this->_evaQueue.pop_front();
	}
}

void UI::render()
{
	if (!this->_enabled)
		return;
	if (this->_placementBuilding != nullptr)
		this->_displayPlacementPreview();
	for (auto &b : this->state->selectedObjects)
		this->_displaySelectedObjectUI(*b);
}

CursorAnimation &UI::getCurrentCursor()
{
	Cursor cursor = !game->paused && this->forcedCursor ? *this->forcedCursor : this->_cursor;

	assert_exp(cursor < CURSOR_COUNT);

	auto &cursorName = this->_cursorMapping[cursor];
	auto it = this->_cursors.find(cursorName);

	assert_msg(it != this->_cursors.end(), cursorName);
	return it->second;
}

void UI::renderAbove()
{
	this->getCurrentCursor().render();

	if (this->ownerState->hasRadar) {
		sf::View oldView = game->screen->getView();
		tgui::FloatRect newView = game->gui.getView().getRect();

		game->screen->setView(sf::View{
			sf::FloatRect{
				sf::Vector2f{newView.left, newView.top},
				sf::Vector2f{newView.width, newView.height}
			}
		});
		this->manager->renderMinimap(this->_minimapPos, this->_minimapSize);
		game->screen->setView(oldView);
	}
}

void UI::updateRender()
{
	for (auto &c : this->_cursors)
		c.second.update();
}

void UI::_updateCursorNormal()
{
	Vector2i pos = this->map->absToTileCoord(this->_placementPos);

	if (!this->_objectBuffer.empty()) {
		// TODO: Handle non snap-to-grid units/buildings
		if (this->state->selectedObjects.size() == 1) {
			auto obj = *this->state->selectedObjects.begin();
			auto ownObj = this->ownerState->index == obj->owner->state.index;

			if (ownObj) {
				if (std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()) {
					if (obj->isDeployable()) {
						if (obj->canDeploy())
							this->_cursor = CURSOR_DEPLOY;
						else
							this->_cursor = CURSOR_NO_DEPLOY;
						return;
					} else if (auto &gq = obj->getGivenQueue()) {
						if (this->ownerState->queueBuildings[*gq].size() > 1 && this->ownerState->primaryBuildings[*gq] != obj) {
							this->_cursor = CURSOR_DEPLOY;
							return;
						}
					}
				}
			}
		}

		if (!this->state->selectedObjects.empty()) {
			auto obj = *this->state->selectedObjects.begin();
			auto &other = this->_objectBuffer.front();
			auto ownObj = this->ownerState->index == obj->owner->state.index;
			auto otherAllied = this->ownerState->team == other->owner->state.team;

			if (ownObj && !otherAllied) {
				if (obj->canAttack(other)) {
					this->_cursor = CURSOR_ATTACK;
					return;
				}
			}
		}

		bool canDeposit = std::ranges::any_of(this->_objectBuffer, [this](const GameObject *o){
			// We can provide income to our teammate by sending harvesters to their resource dump.
			return o->owner->state.team == this->ownerState->team && !o->getAcceptResources().empty();
		});
		bool canHarvest = std::ranges::any_of(this->state->selectedObjects, [this](const GameObject *o){
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			return o->owner->state.index == this->ownerState->index && o->getResourceStorage() != 0;
		});

		if (canDeposit && canHarvest)
			this->_cursor = CURSOR_DUMP;
		else if (
			this->_objectBuffer.back()->isSelectable() &&
			std::ranges::find(this->state->selectedObjects, this->_objectBuffer.back()) == this->state->selectedObjects.end()
		)
			this->_cursor = CURSOR_SELECT;
		else
			this->_cursor = CURSOR_NORMAL;
		return;
	}

	if (std::ranges::all_of(this->state->selectedObjects, [this](const GameObject *o){
		// TODO: Wrong check. Should have a different index per controlling player.
		//       This one is only the player index that were declared at the start but
		//       a player can control multiple in case their ally leaves.
		return this->ownerState->index != o->owner->state.index;
	})) {
		this->_cursor = CURSOR_NORMAL;
		return;
	}

	bool canMove = !this->state->selectedObjects.empty() && std::ranges::all_of(this->state->selectedObjects, [this](const GameObject *o){
		return o->canMove();
	});
	auto &lmap = this->manager->getLoadedMap().grid;

	if (canMove) {
		auto obj = *this->state->selectedObjects.begin();
		auto relPos = obj->map->absToTileCoord(this->_placementPos);
		auto tl = this->map->topLeftCoordToAbs(pos);
		auto passFlags = obj->getPassFlags();
		auto abs = obj->map->tileCoordToAbs(relPos);

		// TODO: Handle non snap-to-grid units
		if (obj->getResourceStorage() != 0 && lmap.anyHasAnyGridBits(abs, GRIDMASK_RESOURCE)) {
			this->_cursor = CURSOR_HARVEST;
			return;
		}
		if (obj->ignoresTerrain())
			this->_cursor = CURSOR_MOVE;
		else if (
			!this->manager->isAreaFree(tl, obj->map->tileSize) ||
			!obj->map->hasAnyGridBits(relPos, passFlags)
		)
			this->_cursor = CURSOR_NO_MOVE;
		else if (
			obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_INFANTRY_CAN_PASS) ||
			obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_VEHICLES_CAN_PASS) || (
				obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_SHIP_CAN_PASS) &&
				passFlags != GRIDMASK_SHIP_CAN_PASS
			)
		)
			this->_cursor = CURSOR_MOVE_RESTRICT;
		else
			this->_cursor = CURSOR_MOVE;
	} else
		this->_cursor = CURSOR_NORMAL;
}

void UI::_updateCursorRepair()
{
	this->_cursor = CURSOR_NO_REPAIR;
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (obj->owner->state.index == this->ownerState->index && !obj->isDead() && obj->getHealth() != obj->getMaxHealth() && obj->isRepairable())
			this->_cursor = CURSOR_REPAIR;
	}
}

void UI::_updateCursorSell()
{
	this->_cursor = CURSOR_NO_SELL;
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (obj->owner->state.index == this->ownerState->index && !obj->isDead() && obj->isSellable())
			this->_cursor = CURSOR_SELL;
	}
}

void UI::_updateCursor()
{
	if (
		this->_placementBuilding != nullptr
	#ifdef _DEBUG
		|| this->_placementPos.x < 0 || this->_placementPos.y < 0 ||
		static_cast<unsigned>(this->_placementPos.x) >= this->map->gridSize.x * this->map->tileSize.x ||
		static_cast<unsigned>(this->_placementPos.y) >= this->map->gridSize.y * this->map->tileSize.y
	#endif
	) {
		this->_cursor = CURSOR_NORMAL;
		return;
	}
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer);

	if (this->_selling)
		this->_updateCursorSell();
	else if (this->_repairing)
		this->_updateCursorRepair();
	else
		this->_updateCursorNormal();
}

bool UI::_isSelectionTooFar() const
{
	if (DEBUG_MODE)
		return false;

	const std::set<Vector2i> &okTiles = this->_placementBuilding->getCheckTiles();
	auto &placementGrid = this->map->placementGrid[this->ownerState->team];
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	if (okTiles.empty())
		return false;
	for (auto &pos : okTiles) {
		auto result = pos + gridPos;

		if (
			result.x < 0 || result.y < 0 ||
			result.x >= static_cast<int>(this->map->gridSize.x) ||
			result.y >= static_cast<int>(this->map->gridSize.y)
		)
			continue;
		if (placementGrid[result.x + result.y * this->map->gridSize.x])
			return false;
	}
	return true;
}

void UI::_displayPlacementPreview()
{
	auto &placement = this->_faction.getPlacementCursors();

	if (placement[0].empty())
		return;

	auto &tileData = this->_placementBuilding->getTileData();
	bool tooFar = this->_isSelectionTooFar();
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	assert_exp(tileData);
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			BuildingMask buildingMask = line[x];

			if (!buildingMask.placementRequired && !buildingMask.placementConcrete)
				continue;

			Sprite *sprite = &placement[2][this->_placementAnimation[2]];
			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (
				full.x >= 0 && full.y >= 0 &&
				static_cast<size_t>(full.x) < this->map->gridSize.x &&
				static_cast<size_t>(full.y) < this->map->gridSize.y
			) {
				if (tooFar || !this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) || (
					this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) &&
					buildingMask.placementConcrete && !buildingMask.placementRequired
				) || !this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize))
					sprite = &placement[2][this->_placementAnimation[2]];
				else if (this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) || buildingMask.placementConcrete)
					sprite = &placement[0][this->_placementAnimation[0]];
				else
					sprite = &placement[1][this->_placementAnimation[1]];
			}
			sprite->setPosition(this->map->topLeftCoordToAbs(full));
			game->screen->draw(*sprite);
		}
	}
}

void UI::_handleTryPlace()
{
	auto &tileData = this->_placementBuilding->getTileData();
	auto &placement = this->_faction.getPlacementCursors();
	bool foundGood = false;

	if (placement[0].empty())
		return;
	if (this->_isSelectionTooFar()) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	assert_exp(tileData);
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			BuildingMask buildingMask = line[x];

			if (!buildingMask.placementRequired && !buildingMask.placementConcrete)
				continue;

			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (full.x < 0 || full.y < 0 || static_cast<size_t>(full.x) >= this->map->gridSize.x || static_cast<size_t>(full.y) >+ this->map->gridSize.y)
				continue;
			if (buildingMask.placementRequired) {
				if (
					!this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) ||
					!this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize)
				) {
					this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
					return;
				}
				foundGood = true;
			} else if (buildingMask.placementConcrete) {
				if (this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) && !this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE))
					foundGood = true;
			}
		}
	}
	if (!foundGood) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	this->_eventQueue.emplace_back(PlaceBuildEvent{
		this->_placementBuilding->getId(),
		this->_faction.getBase().getName() + ":building",
		sf::Vector2u(this->_placementPos.x, this->_placementPos.y)
	});
	this->_placementBuilding = nullptr;
}

void UI::_tileClickedNormal()
{
	// TODO: Hardcoded key
	if (!this->state->selectedObjects.empty() && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
		if (this->state->selectedObjects.size() == 1) {
			auto obj = *this->state->selectedObjects.begin();

			if (
				obj->owner->state.index == this->ownerState->index &&
				std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()
			) {
				if (obj->isDeployable()) {
					if (!obj->canDeploy())
						this->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300, "");
					else
						this->_eventQueue.emplace_back(DeployUnitEvent{obj->index});
					return;
				} else if (auto &gq = obj->getGivenQueue()) {
					if (this->ownerState->queueBuildings[*gq].size() > 1 && this->ownerState->primaryBuildings[*gq] != obj) {
						this->_eventQueue.emplace_back(SetPrimaryEvent{obj->index});
						return;
					}
				}
			}
		}

		unsigned dumpId = 0;
		bool canDeposit = std::ranges::any_of(this->_objectBuffer, [this, &dumpId](const GameObject *o){
			// We can provide income to our teammate by sending harvesters to their resource dump.
			if (o->owner->state.team != this->ownerState->team || o->getAcceptResources().empty())
				return false;
			dumpId = o->index;
			return true;
		});
		bool hasHarvester = false;
		bool canHarvest = false;
		std::set<unsigned> usedUnits;
		auto &lmap = this->manager->getLoadedMap().grid;
		std::unordered_map<unsigned, AttackUnitEvent> attack;

		for (auto o : this->state->selectedObjects) {
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (o->owner->state.index != this->ownerState->index || o->getResourceStorage() == 0)
				continue;
			hasHarvester = true;

			auto absQuad = o->map->tileCoordToAbs(o->map->absToTileCoord(this->_placementPos));

			canHarvest = lmap.anyHasAnyGridBits(absQuad, GRIDMASK_RESOURCE);
			if (canHarvest)
				break;
		}

		for (auto s : this->state->selectedObjects) {
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (s->owner->state.index != this->ownerState->index)
			continueOutter:
				continue;

			for (auto &o : this->_objectBuffer)
				if (s->canAttack(o)) {
					auto &a = attack[o->index];

					a.target = o->index;
					a.indices.push_back(s->index);
					usedUnits.emplace(s->index);
					goto continueOutter;
				}
		}
		for (auto &a : attack | std::ranges::views::values)
			this->_eventQueue.emplace_back(a);

		if (!this->_objectBuffer.empty() && (!hasHarvester || !canDeposit) && usedUnits.empty() && this->_objectBuffer.back()->isSelectable()) {
			this->state->selectedObjects.clear();
			this->state->selectedObjects.emplace(this->_objectBuffer.back());
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (this->_objectBuffer.back()->owner->state.index == this->ownerState->index)
				this->_objectBuffer.back()->select();
		} else if (!this->state->selectedObjects.empty()) {
			HarvestUnitEvent hevent;
			DumpUnitEvent devent;
			MoveUnitEvent event;

			event.position = this->_placementPos;
			event.indices.reserve(this->state->selectedObjects.size());
			hevent.position = this->_placementPos;
			hevent.indices.reserve(this->state->selectedObjects.size());
			devent.hole = dumpId;
			devent.indices.reserve(this->state->selectedObjects.size());
			for (auto s : this->state->selectedObjects) {
				// TODO: Wrong check. Should have a different index per controlling player.
				//       This one is only the player index that were declared at the start but
				//       a player can control multiple in case their ally leaves.
				if (s->owner->state.index != this->ownerState->index)
					continue;
				if (usedUnits.contains(s->index))
					continue;

				// TODO: Handle non snap-to-grid units
				if (s->getResourceStorage() != 0) {
					if (canDeposit) {
						devent.indices.push_back(s->index);
						continue;
					} else if (canHarvest) {
						hevent.indices.push_back(s->index);
						continue;
					}
				}
				if (DEBUG_MODE && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I))
					(*std::ranges::find(this->ownerState->myObjects, s))->setPosition(this->_placementPos);
				else
					event.indices.push_back(s->index);
			}
			if (!event.indices.empty())
				this->_eventQueue.emplace_back(event);
			if (!hevent.indices.empty())
				this->_eventQueue.emplace_back(hevent);
			if (!devent.indices.empty())
				this->_eventQueue.emplace_back(devent);
		}
	} else if (!this->_objectBuffer.empty() && this->_objectBuffer.back()->isSelectable()) {
		auto b = this->_objectBuffer.back();
		auto it = this->state->selectedObjects.find(b);

		if (it == this->state->selectedObjects.end()) {
			this->state->selectedObjects.emplace(b);
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (b->owner->state.index == this->ownerState->index)
				b->select();
		} else
			this->state->selectedObjects.erase(it);
	}
}

void UI::_tileClickedRepair()
{
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (!obj->isDead() && obj->getHealth() != obj->getMaxHealth() && obj->isRepairable())
			this->_eventQueue.emplace_back(RepairEvent{obj->index});
	}
}

void UI::_tileClickedSell()
{
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (!obj->isDead() && obj->isSellable())
			this->_eventQueue.emplace_back(SellEvent{obj->index});
	}
}

void UI::tileClicked(Vector2i absPos)
{
	this->_placementPos = absPos;
	if (this->_placementBuilding != nullptr)
		return this->_handleTryPlace();
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer);

	if (this->_selling)
		this->_tileClickedSell();
	else if (this->_repairing)
		this->_tileClickedRepair();
	else
		this->_tileClickedNormal();
}

void UI::mouseMovedToTile(Vector2i absPos)
{
	this->_placementPos = absPos;
}

void UI::onCancel(Vector2i absPos)
{
	if (this->_placementBuilding)
		this->_placementBuilding = nullptr;
	else if (!this->state->selectedObjects.empty())
		this->state->selectedObjects.clear();
	this->_selling = false;
	this->_repairing = false;
	this->mouseMovedToTile(absPos);
}

void UI::mouseOnUi()
{
	this->_cursor = CURSOR_NORMAL;
}

void UI::_displaySelectBox(Vector2i top, Vector2u size)
{
	sf::Sprite sprite{this->_selectCorners[0]};
	sf::RectangleShape rect;

	top.y += 4;
	sprite.setPosition(Vector2f(top.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[1], true);
	sprite.setPosition(Vector2f(top.x - this->_selectCorners[1].getSize().x + size.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[2], true);
	sprite.setPosition(Vector2f(top.x, top.y - this->_selectCorners[2].getSize().y + size.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[3], true);
	sprite.setPosition(Vector2f(
		top.x - this->_selectCorners[3].getSize().x + size.x,
		top.y - this->_selectCorners[3].getSize().y + size.y
	));
	game->screen->draw(sprite);
}
