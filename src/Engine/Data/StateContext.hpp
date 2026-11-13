//
// Created by PinkySmile on 13/11/2026.
//

#ifndef MIXRTS_STATECONTEXT_HPP
#define MIXRTS_STATECONTEXT_HPP


#include <cstdint>
#include <unordered_map>
#include "StateStream.hpp"
#include "StateRegistry.hpp"

struct StateSaveContext {
	StateWriter &out;
	const std::unordered_map<const GameObject *, uint32_t> &objects;
	const StateRegistry &registry;
	const GameManager &manager;

	void obj(const GameObject *object) const;
	void weapon(const Weapon *weapon) const;
	void effect(const Effect *effect) const;
	void sound(const MultiSound *sound) const;
	void player(const Player *player) const;
	void mapState(const MapState *state) const;
};

struct StateLoadContext {
	StateReader &in;
	const std::unordered_map<uint32_t, GameObject *> &objects;
	const StateRegistry &registry;
	GameManager &manager;

	[[nodiscard]] GameObject *obj() const;
	[[nodiscard]] const Weapon *weapon() const;
	[[nodiscard]] const Effect *effect() const;
	[[nodiscard]] const MultiSound *sound() const;
	[[nodiscard]] Player *player() const;
	[[nodiscard]] MapState *mapState() const;
};


#endif //MIXRTS_STATECONTEXT_HPP
