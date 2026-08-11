//
// Created by PinkySmile on 03/08/2026.
//

#ifndef MIXRTS_EFFECT_HPP
#define MIXRTS_EFFECT_HPP


#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"
#include "Vector.hpp"

class Effect {
private:
	std::vector<Animation> _anim;
	const MultiSound *_sound = nullptr;
	Vector2f _position;
	unsigned _delay = 0;
	unsigned _totalDelay = 0;
	unsigned _index = 0;
	int _layer = 0;

public:
	Effect(
		const Mod &base,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &data
	);
	bool update();
	void render();
	void playSound() const;
	void setPosition(Vector2f pos);
	[[nodiscard]] int getLayer() const;
	[[nodiscard]] const MultiSound *getSound() const;
};


#endif //MIXRTS_EFFECT_HPP
