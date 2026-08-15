//
// Created by PinkySmile on 15/08/2026.
//

#ifndef MIXRTS_PROJECTILE_HPP
#define MIXRTS_PROJECTILE_HPP


#include <vector>
#include "forward.hpp"

class Projectile {
private:
	std::vector<ConditionalAnim> _anims;
	const Weapon *_weapon;
	const Effect *_trail;
	Vector2f _position;
	Vector2f _target;
	unsigned _trailInterval;
	unsigned _speed;
	unsigned _trailTimer = 0;
	unsigned _ownerIndex = UINT32_MAX;
	bool _disposable = false;

	void _displayAnim(Animation &anim, Vector2f offset = {}) const;

public:
	GameManager *manager = nullptr;

	Projectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, Vector2f target);
	void update();
	void render(int layer);
	[[nodiscard]] bool isDisposable() const;
};


#endif //MIXRTS_PROJECTILE_HPP
