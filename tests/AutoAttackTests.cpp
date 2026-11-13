//
// Created for the MixRTS test target.
//
// Behavior tests for automatic attacks: an idle object that has a weapon
// should start shooting at an enemy that is within reach, but only if that
// enemy has the "triggers_auto_attack" property (units and turrets, but not
// regular buildings).
//
// Test objects/weapons live in tests/ (objects.json, weapons/weapons.json).
// The very last test checks the real "d2k" mod's data instead, to make sure
// its units and turrets are flagged, and its other buildings are not.
//

#include <set>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Data/Faction.hpp"
#include "Data/GameManager.hpp"
#include "Data/GameObject.hpp"
#include "Data/Player.hpp"

//! Every attacker is owned by GameFixture's own player (team 0), and every
//! target by a second player (team 1, "test:dummy").
//!
//! Test attackers have no speed: they can never move, only shoot. With the
//! 640px range of the test weapons, a target 200px away is in range and one
//! 800px away is not. Their cooldown (200 ticks) is much longer than the
//! ticks run by each test, so an attacker fires at most once per test.
class AutoAttackTests : public GameFixture {
protected:
	static constexpr unsigned SHOT_DAMAGE = 100;
	//! Damage of weapon.multi_ground (see tests/weapons/weapons.json).
	static constexpr unsigned GROUND_ONLY_SHOT_DAMAGE = 50;
	static constexpr unsigned ACQUIRE_TICKS = 40;
	static constexpr float IN_RANGE = 200;
	static constexpr float OUT_OF_RANGE = 800;

	Player *enemy = nullptr;
	Vector2f base{2000, 2000};

	void SetUp() override
	{
		GameFixture::SetUp();

		this->manager->addPlayer(3, HSLColor{0, 0, 500}, Vector2i{0, 0}, 1);
		// GameManager::getPlayers() can reallocate when a player is added.
		this->player = &this->manager->getPlayers()[0];
		this->enemy = &this->manager->getPlayers()[1];
	}

	GameObject *spawnMine(const std::string &id, Vector2f center)
	{
		GameObject *obj = spawnAt(*this->manager, this->player->state.index, id, center);

		EXPECT_NE(obj, nullptr) << id;
		return obj;
	}

	GameObject *spawnTheirs(const std::string &id, Vector2f center)
	{
		GameObject *obj = spawnAt(*this->manager, this->enemy->state.index, id, center);

		EXPECT_NE(obj, nullptr) << id;
		return obj;
	}

	static unsigned damageTaken(const GameObject *obj)
	{
		return obj->getMaxHealth() - obj->getHealth();
	}
};

//! 1. The basic case: an idle object shoots an enemy unit in range, without
//! ever being given any order.
TEST_F(AutoAttackTests, IdleObjectShootsEnemyUnitInRange)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *target = this->spawnTheirs("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({attacker, target});

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(this->damageTaken(target), SHOT_DAMAGE) << "The idle attacker should have shot the enemy unit in range";
}

//! 2. Enemy objects that don't trigger automatic attacks (here a regular
//! building, and a plain object) are left alone, however close they are.
TEST_F(AutoAttackTests, IdleObjectIgnoresEnemiesThatDoNotTrigger)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *building = this->spawnTheirs("test:target_building", this->base + Vector2f{IN_RANGE, 0});
	GameObject *plain = this->spawnTheirs("test:target", this->base + Vector2f{0, IN_RANGE});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(building, nullptr);
	ASSERT_NE(plain, nullptr);
	this->focusCamera({attacker, building, plain});

	// Buildings placed without concrete start (and stay) damaged: take the
	// reference once the building has settled instead of assuming full health.
	this->tick(1);

	unsigned buildingHealth = building->getHealth();

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(building->getHealth(), buildingHealth) << "Regular buildings must not cause automatic attacks";
	EXPECT_EQ(plain->getHealth(), plain->getMaxHealth()) << "Objects without the property must not cause automatic attacks";
}

//! 3. A turret does trigger automatic attacks.
TEST_F(AutoAttackTests, IdleObjectShootsEnemyTurretInRange)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *turret = this->spawnTheirs("test:target_turret", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(turret, nullptr);
	ASSERT_TRUE(turret->triggersAutoAttack());
	this->focusCamera({attacker, turret});

	this->tick(ACQUIRE_TICKS);

	EXPECT_TRUE(this->damageTaken(turret) >= SHOT_DAMAGE) << "The idle attacker should have shot the enemy turret in range";
}

//! 4. Turrets are idle objects like any other: an enemy turret shoots a unit
//! that comes in range, even though the turret itself can't move.
TEST_F(AutoAttackTests, IdleTurretShootsEnemyUnitInRange)
{
	GameObject *turret = this->spawnTheirs("test:target_turret", this->base);
	GameObject *unit = this->spawnMine("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(turret, nullptr);
	ASSERT_NE(unit, nullptr);
	this->focusCamera({turret, unit});

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(this->damageTaken(unit), SHOT_DAMAGE) << "The turret should have shot the unit in range";
}

//! 5. An enemy that is too far away is left alone, until it gets close enough.
TEST_F(AutoAttackTests, IdleObjectOnlyShootsEnemiesInRange)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *target = this->spawnTheirs("test:target_trigger", this->base + Vector2f{OUT_OF_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	// The target starts out of range, then comes in range.
	this->focusCamera({attacker, target, this->base + Vector2f{IN_RANGE, 0}});

	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(target->getHealth(), target->getMaxHealth()) << "The target is out of range and the attacker can't move: nothing should happen";

	placeAt(*target, this->base + Vector2f{IN_RANGE, 0});
	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(this->damageTaken(target), SHOT_DAMAGE) << "The target came in range: it should have been shot";
}

//! 6. Allies never trigger automatic attacks.
TEST_F(AutoAttackTests, IdleObjectIgnoresAllies)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *ally = this->spawnMine("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(ally, nullptr);
	this->focusCamera({attacker, ally});

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(ally->getHealth(), ally->getMaxHealth()) << "Friendly units must never be shot";
}

//! 7. An object that has no weapon able to hit the enemy (here a ground-only
//! weapon against an air unit) doesn't try to shoot it.
TEST_F(AutoAttackTests, IdleObjectIgnoresEnemiesItCannotHit)
{
	GameObject *attacker = this->spawnMine("test:atk_ground_only", this->base);
	GameObject *air = this->spawnTheirs("test:target_trigger_air", this->base + Vector2f{IN_RANGE, 0});
	GameObject *ground = this->spawnTheirs("test:target_trigger", this->base + Vector2f{0, IN_RANGE});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(air, nullptr);
	ASSERT_NE(ground, nullptr);
	this->focusCamera({attacker, air, ground});

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(air->getHealth(), air->getMaxHealth()) << "The weapon can't hit air units";
	EXPECT_EQ(this->damageTaken(ground), GROUND_ONLY_SHOT_DAMAGE) << "The weapon can hit ground units";
}

//! 8. With several candidates, the closest one is picked.
TEST_F(AutoAttackTests, IdleObjectShootsClosestEnemy)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *far = this->spawnTheirs("test:target_trigger", this->base + Vector2f{IN_RANGE * 2, 0});
	GameObject *close = this->spawnTheirs("test:target_trigger", this->base + Vector2f{0, -IN_RANGE});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(far, nullptr);
	ASSERT_NE(close, nullptr);
	this->focusCamera({attacker, far, close});

	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(this->damageTaken(close), SHOT_DAMAGE) << "The closest enemy should have been shot";
	EXPECT_EQ(far->getHealth(), far->getMaxHealth()) << "The farthest enemy should have been left alone";
}

//! 9. Automatic attacks only happen while idle: an object that is following
//! an order keeps doing so instead of being distracted by a triggering enemy.
TEST_F(AutoAttackTests, BusyObjectKeepsItsOrder)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *ordered = this->spawnTheirs("test:target", this->base + Vector2f{IN_RANGE, 0});
	GameObject *bystander = this->spawnTheirs("test:target_trigger", this->base + Vector2f{0, IN_RANGE});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(ordered, nullptr);
	ASSERT_NE(bystander, nullptr);
	this->focusCamera({attacker, ordered, bystander});

	attacker->attack(ordered, false, true);
	this->tick(ACQUIRE_TICKS);

	EXPECT_EQ(this->damageTaken(ordered), SHOT_DAMAGE) << "The ordered target should have been shot";
	EXPECT_EQ(bystander->getHealth(), bystander->getMaxHealth()) << "The attacker was busy: the bystander shouldn't have been shot";
}

//! 10. A target that gets away is dropped, and the object goes back to
//! looking for a new one as soon as its weapon is ready again.
TEST_F(AutoAttackTests, IdleObjectDropsTargetThatLeavesAndLooksForNewOnes)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *first = this->spawnTheirs("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(first, nullptr);
	// The first target leaves, and the second one shows up.
	this->focusCamera({attacker, first, this->base + Vector2f{OUT_OF_RANGE, 0}, this->base + Vector2f{0, IN_RANGE}});

	this->tick(ACQUIRE_TICKS);
	ASSERT_EQ(this->damageTaken(first), SHOT_DAMAGE);

	// The first target runs away (the attacker can't move, and automatic
	// attacks never chase), and another one shows up.
	placeAt(*first, this->base + Vector2f{OUT_OF_RANGE, 0});

	GameObject *second = this->spawnTheirs("test:target_trigger", this->base + Vector2f{0, IN_RANGE});

	ASSERT_NE(second, nullptr);
	// The weapon needs 200 ticks to be ready again.
	this->tick(260);

	EXPECT_EQ(this->damageTaken(first), SHOT_DAMAGE) << "The first target left: it shouldn't have been shot again";
	EXPECT_EQ(this->damageTaken(second), SHOT_DAMAGE) << "A new enemy is in range: it should have been shot";
}

//! 11. The real d2k data: every unit and turret triggers automatic attacks,
//! and no other building does.
TEST_F(AutoAttackTests, Dune2000UnitsAndTurretsTriggerButOtherBuildingsDoNot)
{
	const std::set<std::string> turrets{"d2k:gunt", "d2k:roct"};
	unsigned units = 0;
	unsigned buildings = 0;

	for (auto &faction : this->manager->getFactions())
		for (auto &[id, object] : faction.getObjects()) {
			if (!id.starts_with("d2k:"))
				continue;
			if (!object.getTileData()) {
				units++;
				EXPECT_TRUE(object.triggersAutoAttack()) << id << " is a unit: it should trigger automatic attacks";
			} else if (turrets.contains(id)) {
				buildings++;
				EXPECT_TRUE(object.triggersAutoAttack()) << id << " is a turret: it should trigger automatic attacks";
			} else {
				buildings++;
				EXPECT_FALSE(object.triggersAutoAttack()) << id << " is a regular building: it should not trigger automatic attacks";
			}
		}
	EXPECT_GT(units, 0u);
	EXPECT_GT(buildings, 0u);
}
