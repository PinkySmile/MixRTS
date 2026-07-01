//
// Created by PinkySmile on 01/04/2026.
//

#include "UI.hpp"
#include "../Data/GameManager.hpp"
#include "../Data/Player.hpp"
#include "../Resources/Game.hpp"
#include "Resources/Assert.hpp"


UI::UI(const Faction &faction) :
	_faction(faction),
	_evaSound(game->dummySoundBuffer)
{
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

void UI::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &)
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

void UI::update()
{
	auto it = this->state->selectedObjects.begin();

	while (it != this->state->selectedObjects.end())
		if ((*it)->isDisposable())
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
			assert_msg(this->_evaSounds[e.soundId], "Trying to play invalid EVA " + std::to_string(e.soundId));
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
}

void UI::updateRender()
{
	for (auto &c : this->_cursors)
		c.second.update();
}

void UI::_displaySelectedObjectUI(const GameObject &object)
{
	auto select = object.getSelectBox();
	Vector2i size = select.size;
	Vector2i top = select.position;
	sf::Sprite sprite{this->_selectCorners[0]};
	sf::RectangleShape rect;

	top += object.getPosition();
	rect.setPosition({top.x + 1.f, top.y + 1.f});
	rect.setSize({size.x - 2.f, 2});
	rect.setOutlineThickness(1);
	rect.setOutlineColor(sf::Color::Black);
	rect.setFillColor(sf::Color{0, 0, 0, 128});
	game->screen->draw(rect);

	rect.setSize({(size.x - 2.f) * object.animationState.health, 2});
	if (object.animationState.health >= 0.5)
		rect.setFillColor(sf::Color::Green);
	else if (object.animationState.health >= 0.3)
		rect.setFillColor(sf::Color{0xFF, 0x80, 0x00});
	else
		rect.setFillColor(sf::Color::Red);
	game->screen->draw(rect);

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

	auto percent = object.getStorageDisplay();

	if (percent) {
		sf::Vector2f pos = {top.x + 2.f, top.y + size.y - 5.f};
		unsigned count = 7;

		if (size.x >= 64)
			count = 15;
		rect.setSize({2, 2});
		for (size_t i = 0; i < count; i++) {
			rect.setPosition(pos);
			if (*percent * count > i && percent != 0)
				rect.setFillColor(sf::Color{0, 255, 0, 255});
			else
				rect.setFillColor(sf::Color{0, 0, 0, 128});
			game->screen->draw(rect);
			pos.x += 3;
		}
	}
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
			uint8_t buildingMask = line[x];

			if ((buildingMask & (GameObject::MASK_PLACEMENT_REQUIRED | GameObject::MASK_PLACEMENT_CONCRETE)) == 0)
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
					(buildingMask & GameObject::MASK_PLACEMENT_CONCRETE) &&
					!(buildingMask & GameObject::MASK_PLACEMENT_REQUIRED)
				) || !this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize))
					sprite = &placement[2][this->_placementAnimation[2]];
				else if (this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) || (buildingMask & GameObject::MASK_PLACEMENT_CONCRETE))
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
			uint8_t buildingMask = line[x];

			if ((buildingMask & (GameObject::MASK_PLACEMENT_REQUIRED | GameObject::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (full.x < 0 || full.y < 0 || static_cast<size_t>(full.x) >= this->map->gridSize.x || static_cast<size_t>(full.y) >+ this->map->gridSize.y)
				continue;
			if (buildingMask & GameObject::MASK_PLACEMENT_REQUIRED) {
				if (
					!this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) ||
					!this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize)
					) {
					this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
					return;
				}
				foundGood = true;
			} else if (buildingMask & GameObject::MASK_PLACEMENT_CONCRETE) {
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

void UI::tileClicked(Vector2i absPos)
{
	this->_placementPos = absPos;
	if (this->_placementBuilding != nullptr)
		return this->_handleTryPlace();
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer);
	// TODO: Hardcoded key
	if (!this->state->selectedObjects.empty() && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
		if (this->state->selectedObjects.size() == 1) {
			auto obj = *this->state->selectedObjects.begin();

			if (
				obj->owner->state.index == this->ownerState->index &&
				obj->isDeployable() &&
				std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()
				) {
				if (!obj->canDeploy())
					this->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300, "");
				else
					this->_eventQueue.emplace_back(DeployUnitEvent{obj->index});
				return;
			}
		}

		unsigned dumpId = 0;
		bool canDeposit = std::ranges::any_of(this->_objectBuffer, [this, &dumpId](const GameObject *o){
			// We can provide income to our teammate by sending harvesters to their resource dump.
			if (o->owner->state.team == this->ownerState->team && !o->getAcceptResources().empty()) {
				dumpId = o->index;
				return true;
			}
			return false;
		});
		bool hasHarvester = false;
		bool canHarvest = false;
		unsigned res = 0;

		for (auto o : this->state->selectedObjects) {
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (o->owner->state.index != this->ownerState->index || o->getResourceStorage() == 0)
				continue;
			hasHarvester = true;

			auto relPos = o->map->absToTileCoord(this->_placementPos);

			canHarvest = o->map->hasAnyGridBits(relPos, GRIDMASK_RESOURCE);
			if (canHarvest) {
				res = GRID_RESOURCE_TYPE(o->map->grid[relPos.x + relPos.y * o->map->gridSize.x]);
				canHarvest = res != 0;
			}
			if (canHarvest)
				break;
		}

		if (!this->_objectBuffer.empty() && (!hasHarvester || !canDeposit)) {
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
	} else if (!this->_objectBuffer.empty()) {
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
	this->mouseMovedToTile(absPos);
}

void UI::mouseOnUi()
{
	this->_cursor = CURSOR_NORMAL;
}
