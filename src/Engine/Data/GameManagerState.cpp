//
// Created by PinkySmile on 13/11/2026.
//

#include <algorithm>
#include <map>
#include <unordered_set>
#include "GameManager.hpp"
#include "StateContext.hpp"
#include "Objects/ObjectSpawner.hpp"
#include "Projectile.hpp"
#include "Effect.hpp"
#include "Player.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"
#include "Utils.hpp"

namespace {
	constexpr uint8_t STATE_VERSION = 1;
	constexpr uint32_t NULL_REF = UINT32_MAX;

	void patchTerrainBits(pfGridType &cell, uint32_t low)
	{
		cell = (cell & ~0x3FFu) | low;
	}

	void restoreRng(RandomWrapper::SerializedWrapper saved)
	{
		RandomWrapper &rng = game->gameRandom;

		if (rng.ser.seed != saved.seed) {
			rng.seed(saved.seed);
			rng.discard(saved.invoke_count);
		} else
			rng.rollback(saved.invoke_count);
	}
}

void StateSaveContext::obj(const GameObject *object) const
{
	if (!object) {
		this->out.u32(NULL_REF);
		return;
	}

	auto it = this->objects.find(object);

	this->out.u32(it == this->objects.end() ? NULL_REF : it->second);
}

void StateSaveContext::weapon(const Weapon *weapon) const
{
	this->out.u32(this->registry.weaponId(weapon));
}

void StateSaveContext::effect(const Effect *effect) const
{
	this->out.u32(this->registry.effectId(effect));
}

void StateSaveContext::sound(const MultiSound *sound) const
{
	this->out.u32(this->registry.soundId(sound));
}

void StateSaveContext::player(const Player *player) const
{
	this->out.u32(player ? static_cast<uint32_t>(player->state.index) : NULL_REF);
}

void StateSaveContext::mapState(const MapState *state) const
{
	if (!state) {
		this->out.u32(NULL_REF);
		return;
	}
	for (size_t i = 0; i < this->manager._mapStates.size(); i++) {
		if (state == &this->manager._mapStates[i]) {
			this->out.u32(static_cast<uint32_t>(i));
			return;
		}
	}
	if (state == &this->manager._map.grid) {
		this->out.u32(static_cast<uint32_t>(this->manager._mapStates.size()));
		return;
	}
	assert_not_reached_msg("MapState is not part of this match");
}

GameObject *StateLoadContext::obj() const
{
	uint32_t id = this->in.u32();

	if (id == NULL_REF)
		return nullptr;

	auto it = this->objects.find(id);

	assert_msg(it != this->objects.end(), "Saved object " + std::to_string(id) + " does not exist");
	return it->second;
}

const Weapon *StateLoadContext::weapon() const
{
	return this->registry.weapon(this->in.u32());
}

const Effect *StateLoadContext::effect() const
{
	return this->registry.effect(this->in.u32());
}

const MultiSound *StateLoadContext::sound() const
{
	return this->registry.sound(this->in.u32());
}

Player *StateLoadContext::player() const
{
	uint32_t id = this->in.u32();

	if (id == NULL_REF)
		return nullptr;
	assert_msg(id < this->manager._players.size(), "Player index is out of range");
	return &this->manager._players[id];
}

MapState *StateLoadContext::mapState() const
{
	uint32_t id = this->in.u32();

	if (id == NULL_REF)
		return nullptr;
	if (id == this->manager._mapStates.size())
		return &this->manager._map.grid;
	assert_msg(id < this->manager._mapStates.size(), "Map state index is out of range");
	return &this->manager._mapStates[id];
}

void GameManager::saveState(GameStateSnapshot &out) const
{
	std::unordered_map<const GameObject *, uint32_t> ids;
	StateWriter writer(out.data);
	StateSaveContext ctx{writer, ids, this->_registry, *this};

	out.data.clear();
	writer.u8(STATE_VERSION);
	writer.u64(this->_tick);
	writer.u32(this->_lastObjectId);
	writer.boolean(this->finished);
	writer.u32(this->_shake.time);
	writer.u32(this->_shake.power);
	writer.u32(this->_shake.interval);
	writer.boolean(this->_shake.freezeTime);
	writer.u32(this->_shakeTimer);
	writer.u32(game->gameRandom.ser.seed);
	writer.u32(game->gameRandom.ser.invoke_count);
	writer.span(this->_map.grid.grid);
	writer.size(this->_mapStates.size());
	for (const MapState &state : this->_mapStates)
		writer.span(state.grid);

	for (const auto &object : this->_liveObjects)
		ids.emplace(&*object, object->index);
	writer.size(this->_liveObjects.size());
	for (const auto &object : this->_liveObjects) {
		writer.u32(object->index);
		writer.u8(object->getStateKind());
		writer.u32(object->owner ? static_cast<uint32_t>(object->owner->state.index) : NULL_REF);
		if (object->getStateKind() == 0) {
			const GameObject *templ = &*object->getFaction().getObjects().at(object->getId());

			writer.u32(this->_registry.objectId(templ));
		}
	}
	for (const auto &object : this->_liveObjects)
		object->saveState(ctx);

	writer.size(this->_players.size());
	for (const Player &player : this->_players)
		player.saveState(ctx);

	writer.size(this->_liveProjectiles.size());
	for (const Projectile &projectile : this->_liveProjectiles) {
		ctx.weapon(projectile.getWeapon());
		ctx.obj(projectile.getParent());
		projectile.saveState(ctx);
	}

	writer.u8(static_cast<uint8_t>(this->_effects.size()));
	for (const auto &layer : this->_effects) {
		writer.size(layer.size());
		for (const Effect &effect : layer) {
			writer.u32(effect.getTemplateId());
			writer.u32(effect.getOwnerIndex());
			effect.savePlayback(writer);
		}
	}
	out.tick = this->_tick;
	out.checksum = hash64(out.data.data(), out.data.size());
}

uint64_t GameManager::checksumState(const uint8_t *data, size_t size)
{
	return hash64(data, size);
}

void GameManager::loadState(const GameStateSnapshot &in)
{
	StateReader reader(in.data);

	this->loadState(reader);
}

void GameManager::loadState(StateReader &in)
{
	struct Header {
		uint32_t index;
		uint8_t kind;
		uint32_t owner;
		uint32_t templateId;
	};

	assert_eq(in.u8(), STATE_VERSION);
	this->_tick = in.u64();
	this->_lastObjectId = in.u32();
	this->finished = in.boolean();
	this->_shake.time = in.u32();
	this->_shake.power = in.u32();
	this->_shake.interval = in.u32();
	this->_shake.freezeTime = in.boolean();
	this->_shakeTimer = in.u32();

	uint32_t savedSeed = in.u32();
	uint32_t savedInvokes = in.u32();
	RandomWrapper::SerializedWrapper savedRng{savedSeed, savedInvokes};
	std::vector<uint32_t> terrain;
	std::vector<char> terrainChanged;
	std::vector<std::vector<uint32_t>> selected(this->_players.size());

	for (size_t i = 0; i < this->_players.size(); i++)
		for (const GameObject *object : this->_players[i].uiState.selectedObjects)
			if (object)
				selected[i].push_back(object->index);
	in.span(terrain);
	assert_eq(terrain.size(), this->_map.grid.grid.size());
	terrainChanged.resize(terrain.size());
	for (size_t i = 0; i < terrain.size(); i++) {
		uint32_t previous = this->_map.grid.grid[i];

		if (previous == terrain[i])
			continue;
		this->_map.grid.grid[i] = terrain[i];
		terrainChanged[i] = 1;
		if ((previous & 0x3FF) != (terrain[i] & 0x3FF)) {
			uint32_t low = terrain[i] & 0x3FF;
			unsigned x = static_cast<unsigned>(i % this->_map.grid.gridSize.x);
			unsigned y = static_cast<unsigned>(i / this->_map.grid.gridSize.x);

			patchTerrainBits(this->_map.grid.pathFindingGrid.grid[i], low);
			for (unsigned py = y * this->_map.grid.tileSize.y; py < (y + 1) * this->_map.grid.tileSize.y; py++)
				for (unsigned px = x * this->_map.grid.tileSize.x; px < (x + 1) * this->_map.grid.tileSize.x; px++)
					patchTerrainBits(this->_map.pathFindingGrid.grid[px + py * this->_map.pathFindingGrid.gridSize.x], low);
		}
	}
	for (size_t i = 0; i < terrainChanged.size(); i++) {
		if (!terrainChanged[i])
			continue;
		this->_map.refreshTile(
			static_cast<unsigned>(i % this->_map.grid.gridSize.x),
			static_cast<unsigned>(i / this->_map.grid.gridSize.x),
			false,
			true
		);
	}
	if (std::find(terrainChanged.begin(), terrainChanged.end(), 1) != terrainChanged.end())
		this->_map.renderCache.display();

	size_t mapStates = in.size();

	assert_exp(mapStates == this->_mapStates.size());
	for (MapState &state : this->_mapStates) {
		std::vector<uint32_t> saved;

		in.span(saved);
		assert_exp(saved.size() == state.grid.size());
		for (size_t i = 0; i < saved.size(); i++) {
			if ((state.grid[i] & 0x3FF) != (saved[i] & 0x3FF))
				patchTerrainBits(state.pathFindingGrid.grid[i], saved[i] & 0x3FF);
			state.grid[i] = saved[i];
		}
	}

	size_t objectCount = in.size();
	std::vector<Header> headers;

	headers.reserve(static_cast<size_t>(objectCount));
	for (uint64_t i = 0; i < objectCount; i++) {
		Header header;

		header.index = in.u32();
		header.kind = in.u8();
		header.owner = in.u32();
		header.templateId = header.kind == 0 ? in.u32() : 0;
		assert_msg(header.owner < this->_players.size(), "Saved object has no player");
		assert_msg(std::ranges::none_of(headers, [&](const Header &other) { return other.index == header.index; }), "Duplicate object index");
		headers.push_back(header);
	}

	std::unordered_set<uint32_t> wanted;
	std::map<uint32_t, std::unique_ptr<GameObject>> kept;

	for (const Header &header : headers)
		wanted.insert(header.index);
	for (auto &object : this->_liveObjects)
		if (object->_initGrid) {
			object->_applyGridFootprint(-1);
			object->_initGrid = false;
		}
	this->_renderObjects.clear();
	for (auto it = this->_liveObjects.begin(); it != this->_liveObjects.end(); ) {
		uint32_t index = (*it)->index;

		if (wanted.contains(index))
			kept.emplace(index, std::move(*it));
		it = this->_liveObjects.erase(it);
	}
	this->_objectMap.clear();
	for (const Header &header : headers) {
		std::unique_ptr<GameObject> object;
		auto existing = kept.find(header.index);
		bool reusable = existing != kept.end() && existing->second->getStateKind() == header.kind;

		if (reusable && header.kind == 0) {
			const GameObject *templ = this->_registry.object(header.templateId);

			reusable = templ && existing->second->getId() == templ->getId();
		}
		if (reusable)
			object = std::move(existing->second);
		else if (header.kind == 0) {
			const GameObject *templ = this->_registry.object(header.templateId);
			Player &owner = this->_players[header.owner];

			assert_exp(templ);
			object = std::unique_ptr<GameObject>(templ->copy());
			object->manager = this;
			object->owner = &owner;
			object->map = &this->_mapStates[owner.state.factionIndex];
			object->pfGrid = &this->_map.pathFindingGrid;
			object->index = header.index;
			object->setColor(owner.getColor());
		} else {
			Player &owner = this->_players[header.owner];

			assert_exp(header.kind == 1);
			object = std::make_unique<ObjectSpawner>(ObjectSpawnerRestoreTag{});
			object->manager = this;
			object->owner = &owner;
			object->map = &this->_mapStates[owner.state.factionIndex];
			object->pfGrid = &this->_map.pathFindingGrid;
			object->index = header.index;
		}
		if (existing != kept.end())
			kept.erase(existing);
		this->_objectMap[object->index] = &*object;
		this->_liveObjects.push_back(std::move(object));
	}
	kept.clear();

	std::unordered_map<uint32_t, GameObject *> byIndex(this->_objectMap.begin(), this->_objectMap.end());
	StateLoadContext ctx{in, byIndex, this->_registry, *this};

	for (auto &object : this->_liveObjects)
		object->loadState(ctx);

	size_t players = in.size();

	assert_exp(players == this->_players.size());
	for (Player &player : this->_players)
		player.loadState(ctx);

	size_t projectiles = in.size();

	this->_liveProjectiles.clear();
	for (uint64_t i = 0; i < projectiles; i++) {
		const Weapon *weapon = ctx.weapon();
		GameObject *parent = ctx.obj();

		this->_liveProjectiles.emplace_back(weapon, parent);
		this->_liveProjectiles.back().manager = this;
		this->_liveProjectiles.back().loadState(ctx);
	}

	uint8_t layers = in.u8();

	assert_exp(layers == this->_effects.size());
	for (size_t layer = 0; layer < this->_effects.size(); layer++) {
		size_t count = in.size();

		this->_effects[layer].clear();
		for (uint64_t i = 0; i < count; i++) {
			uint32_t templateId = in.u32();
			uint32_t ownerIndex = in.u32();
			const Effect *templ = this->_registry.effect(templateId);

			assert_exp(templ);
			this->_effects[layer].emplace_back(*templ);
			this->_effects[layer].back().loadPlayback(in);
			this->_effects[layer].back().setTemplate(templateId, ownerIndex);
			assert_exp(this->_effects[layer].back().getLayer() + 2 == static_cast<int>(layer));
			if (ownerIndex != NULL_REF) {
				assert_msg(ownerIndex < this->_players.size(), "Effect owner is out of range");
				this->_effects[layer].back().setColor(this->_players[ownerIndex].getColor());
			}
		}
	}
	assert_msg(in.remaining() == 0, "Simulation state has trailing bytes");

	for (auto &object : this->_liveObjects)
		if (object->_initGrid)
			object->_applyGridFootprint(1);
	for (auto &object : this->_liveObjects)
		if (object->_registered)
			this->registerObject(&*object);
	restoreRng(savedRng);
	this->hitSource = nullptr;
	for (size_t i = 0; i < this->_players.size(); i++) {
		this->_players[i].uiState.selectedObjects.clear();
		for (uint32_t id : selected[i]) {
			auto it = this->_objectMap.find(id);

			if (it != this->_objectMap.end())
				this->_players[i].uiState.selectedObjects.insert(it->second);
		}
		this->_players[i].getCombinedUI().onStateRestored();
	}
}

uint64_t GameManager::debugGridChecksum() const
{
	uint64_t hash = 0;
	auto mix = [&hash](const void *data, size_t size) {
		hash = hash64(data, size, hash);
	};
	auto mixGrid = [&mix](const PathFindingGrid &grid) {
		mix(grid.grid.data(), grid.grid.size() * sizeof(pfGridType));
		for (const auto &row : grid.usedSpaces)
			mix(row.data(), row.size());
	};

	mix(this->_map.grid.grid.data(), this->_map.grid.grid.size() * sizeof(uint32_t));
	mixGrid(this->_map.pathFindingGrid);
	mixGrid(this->_map.grid.pathFindingGrid);
	for (const auto &row : this->_map.usedSpaces)
		mix(row.data(), row.size());
	for (const auto &row : this->_map.grid.placementGrid)
		mix(row.data(), row.size());
	for (const MapState &state : this->_mapStates) {
		mix(state.grid.data(), state.grid.size() * sizeof(uint32_t));
		mixGrid(state.pathFindingGrid);
		for (const auto &row : state.placementGrid)
			mix(row.data(), row.size());
	}
	return hash;
}
