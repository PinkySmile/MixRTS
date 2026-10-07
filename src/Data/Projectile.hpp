//
// Created by PinkySmile on 15/08/2026.
//

#ifndef MIXRTS_PROJECTILE_HPP
#define MIXRTS_PROJECTILE_HPP


#include <vector>
#include <sol/forward.hpp>
#include "forward.hpp"

class MYDLL_API Projectile {
private:
	std::vector<ConditionalAnim> _anims;
	const GameObject *_parent;
	const GameObject *_target;
	const Weapon *_weapon;
	const Effect *_trail;
	const Player *_owner;
	Vector2f _position;
	Vector2f _startPos;
	Vector2f _targetPos;
	unsigned _trailInterval;
	unsigned _trailTimer = 0;
	unsigned _ownerIndex = UINT32_MAX;
	unsigned _damageRamp;
	unsigned _rampDelay;
	unsigned _rampTimer = 0;
	unsigned _damage;
	unsigned _range;
	unsigned _bounce;
	float _velocity;
	float _gravity;
	float _height;
	float _speed;
	bool _hitTravel;
	bool _homing;
	bool _arc;
	bool _shadow;
	bool _falling;
	bool _disposable = false;

	void _displayAnim(Animation &anim, Vector2f offset = {}) const;

public:
	GameManager *manager = nullptr;

	Projectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, const GameObject *target, Vector2f targetPos);
	void update();
	void render(int layer);
	[[nodiscard]] bool isDisposable() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_PROJECTILE_HPP
