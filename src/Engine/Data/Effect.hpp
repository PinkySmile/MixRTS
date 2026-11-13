//
// Created by PinkySmile on 03/08/2026.
//

#ifndef MIXRTS_EFFECT_HPP
#define MIXRTS_EFFECT_HPP


#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Utils.hpp"
#include "Vector.hpp"

class MYDLL_API Effect {
private:
	std::vector<Animation> _anim;
	const MultiSound *_sound = nullptr;
	Vector2f _position;
	Vector2f _speed = {0, 0};
	unsigned _delay = 0;
	unsigned _totalDelay = 0;
	unsigned _index = 0;
	//! When set, the animation restarts instead of ending, and the effect is removed after this many updates.
	unsigned _repeatFor = 0;
	unsigned _played = 0;
	bool _repeating = false;
	float _opacity = 1;
	int _layer = 0;
	//! Registry id of the faction template this effect was copied from. 0 on the template itself.
	uint32_t _templateId = 0;
	//! Player whose color was applied, or UINT32_MAX when the effect is untinted.
	uint32_t _ownerIndex = UINT32_MAX;

public:
	Effect(
		const Mod &base,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &data
	);
	bool update();
	//! Repeats the animation for \a ticks updates, then removes the effect. A \a ticks of 0 removes it on the next update.
	void repeatFor(unsigned ticks);
	void render();
	void playSound() const;
	void setPosition(Vector2f pos);
	void setColor(HSLColor color);
	void setTemplate(uint32_t templateId, uint32_t ownerIndex);
	[[nodiscard]] uint32_t getTemplateId() const;
	[[nodiscard]] uint32_t getOwnerIndex() const;
	void savePlayback(class StateWriter &out) const;
	void loadPlayback(class StateReader &in);
	[[nodiscard]] Vector2f getPosition() const;
	[[nodiscard]] int getLayer() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_EFFECT_HPP
