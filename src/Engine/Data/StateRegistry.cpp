//
// Created by PinkySmile on 13/11/2026.
//

#include <algorithm>
#include <string>
#include <vector>
#include "StateRegistry.hpp"
#include "Faction.hpp"
#include "Objects/GameObject.hpp"
#include "Weapon.hpp"
#include "Effect.hpp"
#include "MultiSound.hpp"
#include "Upgrade.hpp"
#include "Resources/Assert.hpp"

void StateRegistry::_add(Kind kind, const void *ptr)
{
	assert_exp(ptr);
	assert_exp(!this->_ids.contains(ptr));

	uint32_t id = static_cast<uint32_t>(this->_byKind[kind].size()) + 1;

	this->_byKind[kind].push_back(ptr);
	this->_ids.emplace(ptr, Entry{kind, id});
}

// TODO: Handle all smart ptr instead of just that one
template<>
void StateRegistry::_addSorted(Kind kind, const std::unordered_map<std::string, std::shared_ptr<GameObject>> &map)
{
	std::vector<std::string> keys;

	keys.reserve(map.size());
	for (const auto &entry : map)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	for (const auto &key : keys)
		this->_add(kind, &*map.at(key));
}

template<typename T>
void StateRegistry::_addSorted(Kind kind, const std::unordered_map<std::string, T> &map)
{
	std::vector<std::string> keys;

	keys.reserve(map.size());
	for (const auto &entry : map)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	for (const auto &key : keys)
		this->_add(kind, &map.at(key));
}

void StateRegistry::build(const std::vector<Faction> &factions)
{
	this->_ids.clear();
	for (auto &list : this->_byKind)
		list.clear();
	for (const Faction &faction : factions) {
		this->_addSorted(KIND_OBJECT,  faction.getObjects());
		this->_addSorted(KIND_WEAPON,  faction.getWeapons());
		this->_addSorted(KIND_EFFECT,  faction.getEffects());
		this->_addSorted(KIND_SOUND,   faction.getSounds());
		this->_addSorted(KIND_UPGRADE, faction.getUpgrades());
		this->_addSorted(KIND_QUEUE,   faction.getQueues());
	}
}

void StateRegistry::alias(Kind kind, const void *canonical, const void *extra)
{
	if (!extra || extra == canonical)
		return;

	auto it = this->_ids.find(canonical);

	assert_msg(it != this->_ids.end(), "Canonical template is not registered");
	assert_exp(it->second.kind == kind);

	auto [slot, inserted] = this->_ids.emplace(extra, it->second);

	if (!inserted)
		assert_exp(slot->second.id == it->second.id && slot->second.kind == kind);
}

uint32_t StateRegistry::idOf(Kind kind, const void *ptr) const
{
	if (!ptr)
		return 0;

	auto it = this->_ids.find(ptr);

	assert_msg(it != this->_ids.end(), "Template pointer is not in the state registry");
	assert_exp(it->second.kind == kind);
	return it->second.id;
}

const void *StateRegistry::ptrOf(Kind kind, uint32_t id) const
{
	if (id == 0)
		return nullptr;
	assert_msg(id <= this->_byKind[kind].size(), "Template id is out of range");
	return this->_byKind[kind][id - 1];
}

const GameObject *StateRegistry::object(uint32_t id) const
{
	return static_cast<const GameObject *>(this->ptrOf(KIND_OBJECT, id));
}

const Weapon *StateRegistry::weapon(uint32_t id) const
{
	return static_cast<const Weapon *>(this->ptrOf(KIND_WEAPON, id));
}

const Effect *StateRegistry::effect(uint32_t id) const
{
	return static_cast<const Effect *>(this->ptrOf(KIND_EFFECT, id));
}

const MultiSound *StateRegistry::sound(uint32_t id) const
{
	return static_cast<const MultiSound *>(this->ptrOf(KIND_SOUND, id));
}

const Upgrade *StateRegistry::upgrade(uint32_t id) const
{
	return static_cast<const Upgrade *>(this->ptrOf(KIND_UPGRADE, id));
}
