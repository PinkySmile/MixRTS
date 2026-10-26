//
// Created for the MixRTS test target.
//
// Behavior tests for the weapon system: hitscan weapons, straight-line and
// homing projectiles (including how they react to their target teleporting
// away mid-flight), homing self-destruction when a target can never be
// caught, and per-target weapon selection when a unit is equipped with more
// than one weapon.
//
// Every object/weapon/warhead used here is defined entirely under tests/
// (objects.json, weapons/weapons.json, weapons/warheads.json, armors/*.json)
// and never touches the "d2k" mod's own combat data.
//

#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Data/GameManager.hpp"
#include "Data/GameObject.hpp"
#include "Data/Map.hpp"
#include "Data/Player.hpp"

//! Base fixture for every weapon test: on top of GameFixture's own player
//! (team 0, "d2k:atreides", used below to own every attacker), spawns a
//! second player (team 1, "test:dummy") to own every target/bystander, since
//! GameObject::attack() is a no-op against a same-team target.
class WeaponTests : public GameFixture {
protected:
	Player *enemy = nullptr;

	void SetUp() override
	{
		GameFixture::SetUp();

		// Faction index 3 is "test:dummy" (see GameFixture::SetUpTestSuite():
		// allFactions == {d2k:atreides, d2k:harkonnen, d2k:ordos, test:dummy}).
		// "test:dummy" has no starting objects, so this doesn't spawn anything.
		this->manager->addPlayer(3, HSLColor{0, 0, 500}, 0, 1);
		// GameManager::getPlayers() can reallocate when a player is added:
		// refresh GameFixture's own cached pointer, then grab ours.
		this->player = &this->manager->getPlayers()[0];
		this->enemy = &this->manager->getPlayers()[1];
	}
};

//! 1. Hitscan test: a hitscan weapon should damage its target for its full
//! listed damage the very first time it is able to fire, with no travel
//! delay at all (unlike a projectile).
TEST_F(WeaponTests, HitscanDamagesTargetImmediately)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_hitscan", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);

	attacker->attack(target, false, true);
	this->tick(5);

	EXPECT_EQ(target->getHealth(), target->getMaxHealth() - 100) << "The hitscan weapon should have dealt its full 100 damage instantly";
}

//! 2. Basic projectile test: a non-homing projectile fired at a target that
//! never moves should simply travel to it and hit it for full damage.
TEST_F(WeaponTests, BasicProjectileHitsStationaryTarget)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_projectile", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);

	attacker->attack(target, false, true);
	this->tick(60);

	EXPECT_EQ(target->getHealth(), target->getMaxHealth() - 100) << "The projectile should have travelled to and hit the stationary target";
}

//! 3. Basic projectile, target teleported away: once fired, a non-homing
//! projectile commits to the target's *old* position - it has no idea the
//! target ever moved. Here the target is teleported 5 tiles away,
//! perpendicular to the line of fire, right after the shot leaves the
//! barrel; a decoy left behind at the target's old spot should take the hit
//! instead, and the (now safely elsewhere) real target should take none.
TEST_F(WeaponTests, BasicProjectileMissesWhenTargetTeleportsAway)
{
	Vector2f base{2000, 2000};
	Vector2f targetStart = base + Vector2f{200, 0};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_projectile", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", targetStart);
	// Left behind exactly where the target used to be: since the projectile
	// isn't homing, it should still hit *there*, proving it actually flew
	// towards the old location instead of just never having fired at all.
	GameObject *decoy = spawnAt(*this->manager, this->enemy->state.index, "test:target", targetStart);

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(decoy, nullptr);

	attacker->attack(target, false, true);
	this->tick(1);

	Vector2f dir = (targetStart - base).normalized();
	Vector2f perp{-dir.y, dir.x};
	Vector2u tileSize = this->getMcv()->map->tileSize;
	Vector2f newPos = targetStart + perp * (tileSize.x * 5.f);

	placeAt(*target, newPos);
	this->tick(60);

	EXPECT_EQ(target->getHealth(), target->getMaxHealth()) << "The projectile is not homing, so the relocated target should not have been hit";
	EXPECT_EQ(decoy->getHealth(), decoy->getMaxHealth() - 100) << "The projectile should have still hit the target's old location";
}

//! 4. Homing projectile test: same setup as above (target teleported 5
//! tiles away, perpendicular to the line of fire, right after the shot is
//! fired), but with a homing projectile this time - it should re-aim at the
//! target's *new* position and still hit it for (close to) full damage.
//!
//! "Close to" rather than exactly full damage: since the projectile's speed
//! (see weapon.homing in tests/weapons/weapons.json) won't in general divide
//! its post-teleport travel distance evenly, it can overshoot its target by
//! up to one speed-step before the engine notices and detonates it - and the
//! warhead's damage is computed at that (slightly-off) impact point, not by
//! teleporting the explosion onto the target the way a perfectly precise hit
//! would. wh_direct's radius (40px, see tests/weapons/warheads.json) is
//! large relative to that worst-case overshoot, so the resulting splash
//! falloff is small; EXPECT_NEAR tolerates it instead of demanding bit-exact
//! damage.
TEST_F(WeaponTests, HomingProjectileFollowsTeleportedTarget)
{
	Vector2f base{2000, 2000};
	Vector2f targetStart = base + Vector2f{200, 0};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_homing", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", targetStart);

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);

	attacker->attack(target, false, true);
	this->tick(1);

	Vector2f dir = (targetStart - base).normalized();
	Vector2f perp{-dir.y, dir.x};
	Vector2u tileSize = this->getMcv()->map->tileSize;
	Vector2f newPos = targetStart + perp * (tileSize.x * 5.f);

	placeAt(*target, newPos);
	this->tick(150);

	unsigned damageDealt = target->getMaxHealth() - target->getHealth();

	EXPECT_NEAR(damageDealt, 100, 20) << "The homing projectile should have re-aimed and hit the target at its new location for close to its full 100 damage";
}

//! 5. Homing projectile range test: the target is teleported back and forth
//! in front of and behind the attacker, further apart each time than the
//! projectile's own self-destruct range, so the (homing) projectile can
//! never actually catch up to it. It should instead blow itself up mid-
//! flight after a while, dealing splash damage to any other unit caught
//! nearby - here, two bystanders placed on either side of the attacker,
//! within the blast radius of wherever a shot happens to give up.
TEST_F(WeaponTests, HomingProjectileSelfDestructsWhenItCanNeverCatchUp)
{
	Vector2f base{2000, 2000};
	Vector2f front = base + Vector2f{300, 0};
	Vector2f behind = base + Vector2f{-300, 0};

	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_homing_short", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", front);
	GameObject *bystanderFront = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{100, 0});
	GameObject *bystanderBehind = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{-100, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(bystanderFront, nullptr);
	ASSERT_NE(bystanderBehind, nullptr);

	attacker->attack(target, false, true);

	// weapon.homing_shortrange's projectile self-destructs after travelling
	// 120px (see tests/weapons/weapons.json), well under the 300px distance
	// to either teleport spot: every single shot is guaranteed to give up
	// long before it could ever reach the target, wherever it currently is.
	for (unsigned i = 0; i < 400; i++) {
		if (i % 30 == 0)
			placeAt(*target, (i / 30) % 2 == 0 ? front : behind);
		this->tick(1);
	}

	EXPECT_EQ(target->getHealth(), target->getMaxHealth()) << "The target kept evading every shot, so it should never have taken any damage";
	EXPECT_TRUE(bystanderFront->getHealth() < bystanderFront->getMaxHealth() || bystanderBehind->getHealth() < bystanderBehind->getMaxHealth())
		<< "The homing projectile(s) should have self-destructed (dealing splash damage nearby) instead of chasing the target forever";
}

//! 6. Multi weapon test: a unit carrying two different weapons on the very
//! same barrel (one that can only hit ground targets, one that can only hit
//! air targets) should always pick whichever of the two actually applies to
//! the specific target it's told to attack.
//!
//! It should also only ever fire ONE of the two weapons per shot, since they
//! share a single barrel and one shared cooldown (GameObject::_updateAttack()
//! picks a single ObjectWeapon via _getBestWeaponFor() and only ever spawns
//! one shot/projectile per call). This test - along with the more direct
//! OverlappingWeaponsOnSameBarrelFireOnlyOnePerShot below - currently PASSES:
//! contrary to this suite's original assumption of a "both weapons fire at
//! once" engine bug, that behavior was not reproducible here in any
//! configuration that was tried (mutually-exclusive ground/air weapons, and
//! fully-overlapping "can hit either" weapons on one barrel both fire
//! exactly once). If you can still reproduce the bug, it likely needs a more
//! specific setup (e.g. multiple turrets/barrels, priority-based selection,
//! or automatic/auto-acquire targeting) than what's covered here.
TEST_F(WeaponTests, MultiWeaponUnitPicksSingleBestWeaponPerTarget)
{
	constexpr unsigned GROUND_WEAPON_DAMAGE = 50;
	constexpr unsigned AIR_WEAPON_DAMAGE = 77;
	Vector2f base{2000, 2000};

	{
		GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_multi", base);
		GameObject *groundTarget = spawnAt(*this->manager, this->enemy->state.index, "test:target_ground", base + Vector2f{200, 0});

		ASSERT_NE(attacker, nullptr);
		ASSERT_NE(groundTarget, nullptr);

		attacker->attack(groundTarget, false, true);
		this->tick(5);

		EXPECT_EQ(groundTarget->getHealth(), groundTarget->getMaxHealth() - GROUND_WEAPON_DAMAGE)
			<< "Only the ground weapon can hit a ground target, so it alone should have fired";
	}
	{
		GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_multi", base);
		GameObject *airTarget = spawnAt(*this->manager, this->enemy->state.index, "test:target_air", base + Vector2f{200, 0});

		ASSERT_NE(attacker, nullptr);
		ASSERT_NE(airTarget, nullptr);

		attacker->attack(airTarget, false, true);
		this->tick(5);

		EXPECT_EQ(airTarget->getHealth(), airTarget->getMaxHealth() - AIR_WEAPON_DAMAGE)
			<< "Only the air weapon can hit an air target, so it alone should have fired";
	}
}

//! 6b. Sanity check for the same "single weapon per barrel" rule as above,
//! but with two weapons that are *both* able to hit the exact same target
//! (as opposed to test 6's mutually-exclusive ground/air split): confirms
//! that only one of them (whichever is picked) fires, never both at once.
TEST_F(WeaponTests, OverlappingWeaponsOnSameBarrelFireOnlyOnePerShot)
{
	constexpr unsigned WEAPON_A_DAMAGE = 11;
	constexpr unsigned WEAPON_B_DAMAGE = 22;
	Vector2f base{2000, 2000};

	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_overlap", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);

	attacker->attack(target, false, true);
	this->tick(5);

	unsigned damageDealt = target->getMaxHealth() - target->getHealth();

	EXPECT_TRUE(damageDealt == WEAPON_A_DAMAGE || damageDealt == WEAPON_B_DAMAGE)
		<< "Exactly one of the two overlapping weapons should have fired (dealt " << damageDealt << " damage)";
}
