//
// Created by PinkySmile on 15/08/2026.
//

#include <algorithm>
#include <sol/sol.hpp>
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
	this->_blockedByWall = data.blockedByWall;
	this->_homing = data.homing;
	this->_trail = data.trail;
	this->_trailInterval = data.trailInterval;
	this->_speed = data.speed;
	this->_range = data.range;
	this->_damageRamp = data.damageRamp;
	this->_rampDelay = data.rampDelay;
	this->_hitTravel = data.hitTravel;
	this->_arc = data.arc;
	this->_shadow = data.shadow;
	this->_velocity = data.velocity;
	this->_gravity = data.gravity;
	this->_height = data.startY;
	this->_falling = data.falling;
	this->_bounce = data.bounce.first;
	if (this->_bounce != data.bounce.second)
		this->_bounce = random_distrib(game->gameRandom, data.bounce.first, data.bounce.second);

	this->_damage = weapon->getDamage();
	if (data.range && !this->_homing)
		this->_targetPos = pos + (this->_targetPos - pos).normalized() * data.range;
	if (this->_falling) {
		this->_speed *= pos.distance(this->_targetPos);
		this->_targetPos += (this->_targetPos - this->_position) * 100;
	}
}

void Projectile::_hit()
{
	this->manager->hitSource = this;
	this->manager->weaponUsedAt(this->_damage, this->_weapon, this->_position, this->_ownerIndex);
	this->manager->hitSource = nullptr;
}

void Projectile::update()
{
#ifdef _DEBUG
	if (this->manager && this->manager->debugOptions.projectiles) {
		this->_history.push_back({this->_position, this->_height || !this->_arc ? this->_height : this->getArcHeight(this->_position)});
		if (this->_history.size() > DEBUG_PROJECTILE_TRAIL)
			this->_history.pop_front();
	}
#endif
	this->_trailTimer++;
	if (this->_trail && this->_trailTimer > this->_trailInterval) {
		this->_trailTimer = 0;
		this->manager->spawnEffect(*this->_trail, {this->_position.x, this->_position.y - this->_height}, this->_owner);
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

	if (this->_falling) {
		this->_height += this->_velocity;
		this->_velocity -= this->_gravity;
		if (this->_height <= 0) {
			if (this->_bounce == 0) {
				const Effect *hitEffect = this->_weapon->getHitEffect();

				this->_disposable = false;
				if (hitEffect)
					this->manager->spawnEffect(*hitEffect, this->_position, this->_owner);
				this->_hit();
				this->_disposable = true;
			} else {
				this->_velocity *= -0.5;
				this->_height = 0;
				this->_bounce--;
			}
		}
	} else if (
		this->_position == this->_targetPos ||
		(diff.x > 0) != (diff2.x > 0) ||
		(diff.y > 0) != (diff2.y > 0) ||
		(this->_range && this->_position.distance(this->_startPos) >= this->_range) ||
		(this->_blockedByWall && this->manager->getObjectInArea(this->_position, {1, 1}, true, [](const GameObject *o){
			return o->isWall();
		}))
	) {
		const Effect *hitEffect = this->_weapon->getHitEffect();

		this->_disposable = false;
		if (hitEffect)
			this->manager->spawnEffect(*hitEffect, this->_position, this->_owner);
		this->_hit();
		this->_disposable = true;
		return;
	}
	if (this->_hitTravel)
		this->_hit();

	float angle = this->_position.angle(this->_targetPos);
	auto &anim = this->_anims[angleToDirection(angle, this->_anims.size())];

	if (anim())
		anim.update();
}

void Projectile::_displayAnim(Animation &anim, Vector2f offset) const
{
	Sprite &sprite = anim.getSprite();
	auto pos = sprite.getPosition();

	if (this->_shadow || this->_height) {
		sprite.setPosition({
			pos.x + this->_position.x + offset.x,
			pos.y + this->_position.y + offset.y
		});
		sprite.setColor(sf::Color{0, 0, 0, 0x80});
		game->screen->draw(sprite);
	}
	sprite.setColor(sf::Color::White);
	if (this->_height) {
		sprite.setPosition({
			pos.x + this->_position.x + offset.x,
			pos.y + this->_position.y + offset.y - this->_height
		});
	} else if (this->_arc) {
		float height = this->getArcHeight(this->_position);

		sprite.setPosition({
			pos.x + this->_position.x + offset.x,
			pos.y + this->_position.y + offset.y - height
		});
	} else
		sprite.setPosition({
			pos.x + this->_position.x + offset.x,
			pos.y + this->_position.y + offset.y
		});
	using Factor = sf::BlendMode::Factor;
	using Equation = sf::BlendMode::Equation;
	switch (anim.getBlending()) {
	case Animation::BLEND_ADD:
		game->screen->draw(sprite, sf::BlendAdd);
		break;
	case Animation::BLEND_SUB:
		game->screen->draw(sprite, sf::BlendMode{
			Factor::SrcAlpha, Factor::One, Equation::ReverseSubtract,
			Factor::One, Factor::One, Equation::ReverseSubtract
		});
		break;
	case Animation::BLEND_ALPHA:
		game->screen->draw(sprite, sf::BlendAlpha);
		break;
	}
}

void Projectile::render(int layer)
{
	float angle = this->_position.angle(this->_targetPos);
	auto &anim = this->_anims[angleToDirection(angle, this->_anims.size())];

	if (anim.getLayer() == layer && anim())
		this->_displayAnim(anim.getAnim());
}

bool Projectile::isDisposable() const
{
	return this->_disposable;
}

const GameObject *Projectile::getParent() const
{
	return this->_parent;
}

const GameObject *Projectile::getTarget() const
{
	return this->_target;
}

const Weapon *Projectile::getWeapon() const
{
	return this->_weapon;
}

Vector2f Projectile::getPosition() const
{
	return this->_position;
}

Vector2f Projectile::getStartPosition() const
{
	return this->_startPos;
}

Vector2f Projectile::getTargetPosition() const
{
	return this->_targetPos;
}

float Projectile::getHeight() const
{
	return this->_height;
}

float Projectile::getSpeed() const
{
	return this->_speed;
}

unsigned Projectile::getDamage() const
{
	return this->_damage;
}

unsigned Projectile::getRange() const
{
	return this->_range;
}

bool Projectile::isHoming() const
{
	return this->_homing;
}

bool Projectile::isArc() const
{
	return this->_arc;
}

bool Projectile::isFalling() const
{
	return this->_falling;
}

bool Projectile::isHitTravel() const
{
	return this->_hitTravel;
}

const std::deque<Projectile::TrailPoint> &Projectile::getTrail() const
{
	return this->_history;
}

float Projectile::getArcHeight(Vector2f position) const
{
	const double a = 16;
	double b = this->_targetPos.distance(this->_startPos);

	if (b <= 0)
		return 0;

	double x = this->_targetPos.distance(position);
	double xmb2 = x - b / 2;

	return a - a * ((xmb2 * xmb2) / (b / 4) / b);
}

std::vector<Projectile::TrailPoint> Projectile::simulateFall() const
{
	std::vector<TrailPoint> result;
	Vector2f pos = this->_position;
	float height = this->_height;
	float velocity = this->_velocity;
	unsigned bounce = this->_bounce;

	for (unsigned i = 0; i < 1000; i++) {
		Vector2f diff = this->_targetPos - pos;

		if (diff.magnitude2() > 0) {
			diff.normalize();
			diff *= this->_speed;
			pos += diff;
		}
		height += velocity;
		velocity -= this->_gravity;
		result.push_back({pos, std::max(height, 0.f)});
		if (height <= 0) {
			if (bounce == 0)
				break;
			velocity *= -0.5;
			height = 0;
			bounce--;
		}
	}
	return result;
}

void Projectile::pushUserType(sol::state &state)
{
	sol::usertype<Projectile> type = state.new_usertype<Projectile>("Projectile");

	type["manager"] = &Projectile::manager;
	type["update"] = &Projectile::update;
	type["render"] = &Projectile::render;
	type["disposable"] = sol::property(&Projectile::isDisposable);
}
