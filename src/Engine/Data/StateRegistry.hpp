//
// Created by PinkySmile on 13/11/2026.
//

#ifndef MIXRTS_STATEREGISTRY_HPP
#define MIXRTS_STATEREGISTRY_HPP


#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "Resources/MSVCMacros.hpp"
#include "forward.hpp"
#include "Faction.hpp"

//! Dense, deterministic ids for faction template objects. Id 0 is always null.
//! Ids stay valid for the whole match: factions are not reloaded, and the save
//! file checks faction checksums before a blob is applied.
class StateRegistry {
public:
	enum Kind : uint8_t {
		KIND_OBJECT,
		KIND_WEAPON,
		KIND_EFFECT,
		KIND_SOUND,
		KIND_UPGRADE,
		KIND_QUEUE,
		KIND_COUNT
	};

	void build(const std::vector<Faction> &factions);
	//! Maps another address of the same template (a faction copy) onto an id already registered for \a canonical.
	void alias(Kind kind, const void *canonical, const void *extra);

	[[nodiscard]] uint32_t idOf(Kind kind, const void *ptr) const;
	[[nodiscard]] const void *ptrOf(Kind kind, uint32_t id) const;

	[[nodiscard]] uint32_t objectId(const GameObject *object) const { return this->idOf(KIND_OBJECT, object); }
	[[nodiscard]] uint32_t weaponId(const Weapon *weapon) const { return this->idOf(KIND_WEAPON, weapon); }
	[[nodiscard]] uint32_t effectId(const Effect *effect) const { return this->idOf(KIND_EFFECT, effect); }
	[[nodiscard]] uint32_t soundId(const MultiSound *sound) const { return this->idOf(KIND_SOUND, sound); }
	[[nodiscard]] uint32_t upgradeId(const Upgrade *upgrade) const { return this->idOf(KIND_UPGRADE, upgrade); }

	[[nodiscard]] const GameObject *object(uint32_t id) const;
	[[nodiscard]] const Weapon *weapon(uint32_t id) const;
	[[nodiscard]] const Effect *effect(uint32_t id) const;
	[[nodiscard]] const MultiSound *sound(uint32_t id) const;
	[[nodiscard]] const Upgrade *upgrade(uint32_t id) const;

private:
	struct Entry {
		Kind kind;
		uint32_t id;
	};

	std::unordered_map<const void *, Entry> _ids;
	std::array<std::vector<const void *>, KIND_COUNT> _byKind;

	template<typename T>
	void _addSorted(Kind kind, const std::unordered_map<std::string, T> &map);

	void _add(Kind kind, const void *ptr);
};


#endif //MIXRTS_STATEREGISTRY_HPP
