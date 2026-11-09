//
// Created by PinkySmile on 15/08/2026.
//

#ifndef MIXRTS_PROJECTILE_HPP
#define MIXRTS_PROJECTILE_HPP


#include <deque>
#include <vector>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Vector.hpp"

class MYDLL_API Projectile {
public:
	//! One past position of the projectile (debug tool)
	struct TrailPoint {
		Vector2f position;
		float height;
	};

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
	bool _blockedByWall;
	bool _disposable = false;
	std::deque<TrailPoint> _history;

	void _displayAnim(Animation &anim, Vector2f offset = {}) const;
	void _hit();

public:
	GameManager *manager = nullptr;

	Projectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, const GameObject *target, Vector2f targetPos);
	void update();
	void render(int layer);
	[[nodiscard]] bool isDisposable() const;

	// Accessors used by the debug tools
	[[nodiscard]] const GameObject *getParent() const;
	[[nodiscard]] const GameObject *getTarget() const;
	[[nodiscard]] const Weapon *getWeapon() const;
	[[nodiscard]] Vector2f getPosition() const;
	[[nodiscard]] Vector2f getStartPosition() const;
	[[nodiscard]] Vector2f getTargetPosition() const;
	[[nodiscard]] float getHeight() const;
	[[nodiscard]] float getSpeed() const;
	[[nodiscard]] unsigned getDamage() const;
	[[nodiscard]] unsigned getRange() const;
	[[nodiscard]] bool isHoming() const;
	[[nodiscard]] bool isArc() const;
	[[nodiscard]] bool isFalling() const;
	[[nodiscard]] bool isHitTravel() const;
	[[nodiscard]] const std::deque<TrailPoint> &getTrail() const;
	//! Height of the projectile above the ground, when at \a position, for arc projectiles.
	[[nodiscard]] float getArcHeight(Vector2f position) const;
	//! Simulates the end of the flight of a falling projectile and returns the points it goes through (position and height).
	[[nodiscard]] std::vector<TrailPoint> simulateFall() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_PROJECTILE_HPP
