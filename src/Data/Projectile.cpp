//
// Created by PinkySmile on 15/08/2026.
//

#include "ConditionalAnim.hpp"
#include "Animation.hpp"
#include "Weapon.hpp"
#include "Projectile.hpp"
#include "GameManager.hpp"
#include "GameObject.hpp"
#include "Player.hpp"
#include "Resources/Game.hpp"

Projectile::Projectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, const GameObject *target, Vector2f targetPos) :
	_parent(parent),
	_target(target),
	_weapon(weapon),
	_position(pos),
	_startPos(pos),
	_targetPos(targetPos)
{
	assert_exp(weapon->getProjectileData());
	const WeaponProjectile &data = *weapon->getProjectileData();

	this->_anims.reserve(data.anims.size());
	for (auto &anim : data.anims) {
		this->_anims.emplace_back(parent, anim);
		if (parent)
			this->_anims.back().setColor(parent->owner->getColor());
	}
	if (parent) {
		this->_ownerIndex = parent->owner->state.index;
		this->_owner = parent->owner;
	}
	this->_homing = data.homing;
	this->_trail = data.trail;
	this->_trailInterval = data.trailInterval;
	this->_speed = data.speed;
	this->_range = data.range;
	this->_damageRamp = data.damageRamp;
	this->_rampDelay = data.rampDelay;
	this->_hitTravel = data.hitTravel;
	this->_damage = weapon->getDamage();
	if (data.range && !this->_homing)
		this->_targetPos = pos + (this->_targetPos - pos).normalized() * data.range;
}

void Projectile::update()
{
	this->_trailTimer++;
	if (this->_trail && this->_trailTimer > this->_trailInterval) {
		this->_trailTimer = 0;
		this->manager->spawnEffect(*this->_trail, this->_position, this->_owner);
	}
	if (this->_homing && this->_target) {
		this->_targetPos = this->_target->getPosition() + this->_target->getSize() / 2;
		if (this->_target->isDisposable())
			this->_target = nullptr;
	}
	if (this->_rampDelay <= this->_rampTimer) {
		this->_damage += this->_damageRamp;
		this->_rampTimer = 0;
	} else
		this->_rampTimer++;

	auto diff = this->_targetPos - this->_position;

	diff.normalize();
	diff *= this->_speed;
	this->_position += diff;

	auto diff2 = this->_targetPos - this->_position;

	if (
		this->_position == this->_targetPos ||
		(diff.x > 0) != (diff2.x > 0) ||
		(diff.y > 0) != (diff2.y > 0) ||
		(this->_range && this->_position.distance(this->_startPos) >= this->_range)
	) {
		const Effect *hitEffect = this->_weapon->getHitEffect();

		this->_disposable = false;
		if (hitEffect)
			this->manager->spawnEffect(*hitEffect, this->_targetPos, this->_owner);
		this->manager->weaponUsedAt(this->_damage, this->_weapon, this->_targetPos, this->_ownerIndex);
		this->_disposable = true;
	} else if (this->_hitTravel)
		this->manager->weaponUsedAt(this->_damage, this->_weapon, this->_position, this->_ownerIndex);
}

void Projectile::_displayAnim(Animation &anim, Vector2f offset) const
{
	Sprite &sprite = anim.getSprite();

	sprite.setPosition({
		sprite.getPosition().x + this->_position.x + offset.x,
		sprite.getPosition().y + this->_position.y + offset.y
	});
	switch (anim.getBlending()) {
	case Animation::BLEND_ADD:
		game->screen->draw(sprite, sf::BlendAdd);
		break;
	case Animation::BLEND_ALPHA:
		game->screen->draw(sprite);
		break;
	}
}

void Projectile::render(int layer)
{
	float angle = this->_position.angle(this->_targetPos);
	unsigned direction = round((angle + M_PI_2 + M_PI * 2) * this->_anims.size() / (M_PI * 2));
	direction %= this->_anims.size();
	auto &anim = this->_anims[direction];

	if (anim.getLayer() == layer && anim())
		this->_displayAnim(anim.getAnim());
}

bool Projectile::isDisposable() const
{
	return this->_disposable;
}
