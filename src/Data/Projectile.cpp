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

Projectile::Projectile(const GameObject *parent, const Weapon *weapon, Vector2f pos, Vector2f target) :
	_weapon(weapon),
	_position(pos),
	_target(target)
{
	assert_exp(weapon->getProjectileData());
	const WeaponProjectile &data = *weapon->getProjectileData();

	this->_anims.reserve(data.anims.size());
	for (auto &anim : data.anims) {
		this->_anims.emplace_back(parent, anim);
		if (parent)
			this->_anims.back().setColor(parent->owner->getColor());
	}
	if (parent)
		this->_ownerIndex = parent->owner->state.index;
	this->_trail = data.trail;
	this->_trailInterval = data.trailInterval;
	this->_speed = data.speed;
}

void Projectile::update()
{
	this->_trailTimer++;
	if (this->_trailTimer > this->_trailInterval) {
		this->_trailTimer = 0;
		this->manager->spawnEffect(*this->_trail, this->_position);
	}

	auto diff = this->_target - this->_position;

	diff.normalize();
	diff *= this->_speed;
	this->_position += diff;

	auto diff2 = this->_target - this->_position;

	if (this->_position == this->_target || (diff.x > 0) != (diff2.x > 0) || (diff.y > 0) != (diff2.y > 0)) {
		const Effect *hitEffect = this->_weapon->getHitEffect();

		this->_disposable = false;
		if (hitEffect)
			this->manager->spawnEffect(*hitEffect, this->_target);
		this->manager->weaponUsedAt(this->_weapon->getDamage(), this->_weapon, this->_target, this->_ownerIndex);
		this->_disposable = true;
	}
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
	float angle = this->_position.angle(this->_target);
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
