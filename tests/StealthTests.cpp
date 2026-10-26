//
// Created for the MixRTS test target.
//
// Behavior tests for stealthed (and camouflaged) objects:
//  - they can't be targeted (nor automatically attacked) while cloaked,
//  - they get revealed by nearby revealers ("reveal_stealth") or detectors ("detector"),
//    depending on the type of stealth,
//  - permanent stealths are only revealed for a while,
//  - damaged objects can fail to cloak and flicker instead,
//  - temporary stealths need to be charged and activated using the deploy action.
//
// Test objects/animations live in tests/ (objects.json, animations.json).
// The last test checks the real "d2k" mod's data instead.
//

#include <algorithm>
#include <set>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Data/Faction.hpp"
#include "Data/GameManager.hpp"
#include "Data/GameObject.hpp"
#include "Data/Player.hpp"

//! Stealthed objects belong to the second player (team 1) unless the test
//! says otherwise. Everything else (attackers, revealers...) belongs to
//! GameFixture's own player (team 0), which is also the one being controlled.
//!
//! The "test:stealth_*" and "test:camo_*" objects need 4 ticks to (un)cloak
//! (4 frames in the mask, no delay) and, for the permanent ones, try to cloak
//! again 60 ticks after being revealed (see tests/objects.json).
class StealthTests : public GameFixture {
protected:
	static constexpr unsigned SHOT_DAMAGE = 100;
	static constexpr unsigned ACQUIRE_TICKS = 40;
	static constexpr float IN_RANGE = 200;
	//! Distance (center to center) of something that is on a neighbour tile.
	static constexpr float ADJACENT = 32;

	Player *enemy = nullptr;
	Vector2f base{2000, 2000};

	void SetUp() override
	{
		GameFixture::SetUp();

		this->manager->addPlayer(3, HSLColor{0, 0, 500}, 0, 1);
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

	//! Whether the controlled player (and its team) is unable to see/target \a obj.
	[[nodiscard]] bool hiddenFromUs(const GameObject *obj) const
	{
		return obj->isHiddenFrom(this->player->state.team);
	}

	//! Spawns a unit of ours, with nothing special, that is right next to \a obj.
	GameObject *spawnNextTo(const std::string &id, const GameObject *obj)
	{
		GameObject *result = this->spawnMine(id, obj->getMiddle() + Vector2f{0, 400});

		if (result)
			placeAt(*result, obj->getMiddle() + Vector2f{ADJACENT, 0});
		return result;
	}

	static unsigned damageTaken(const GameObject *obj)
	{
		return obj->getMaxHealth() - obj->getHealth();
	}
};

//! 1. Permanent stealths start cloaked, and only their enemies can't see them.
TEST_F(StealthTests, PermanentStealthIsHiddenFromEnemiesOnly)
{
	GameObject *theirs = this->spawnTheirs("test:stealth_perm", this->base);
	GameObject *mine = this->spawnMine("test:stealth_perm", this->base + Vector2f{0, IN_RANGE});
	GameObject *plain = this->spawnTheirs("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(theirs, nullptr);
	ASSERT_NE(mine, nullptr);
	ASSERT_NE(plain, nullptr);
	this->tick(1);

	EXPECT_EQ(theirs->getStealthState(), STEALTH_CLOAKED);
	EXPECT_TRUE(this->hiddenFromUs(theirs));
	EXPECT_FALSE(theirs->isHiddenFrom(this->enemy->state.team)) << "Their owner always sees it";
	EXPECT_FALSE(this->hiddenFromUs(mine)) << "Our own stealthed objects are not hidden from us";
	EXPECT_TRUE(mine->isHiddenFrom(this->enemy->state.team));
	EXPECT_FALSE(this->hiddenFromUs(plain)) << "Objects without stealth are never hidden";
	EXPECT_FALSE(theirs->isDeployable()) << "Permanent stealths aren't activated by the player";
	EXPECT_FALSE(theirs->getGridFootprint().empty()) << "The minimap relies on it to leave hidden objects out";
}

//! 2. Idle objects don't shoot hidden units, but do as soon as they are revealed.
TEST_F(StealthTests, AutoAttackIgnoresHiddenEnemiesUntilRevealed)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *target = this->spawnTheirs("test:stealth_perm", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);

	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(target->getHealth(), target->getMaxHealth()) << "Hidden enemies must be left alone";

	GameObject *revealer = this->spawnNextTo("test:revealer", target);

	ASSERT_NE(revealer, nullptr);
	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(this->damageTaken(target), SHOT_DAMAGE) << "The enemy was revealed: it should have been shot";
}

//! 3. An attack in progress stops when the target cloaks again.
TEST_F(StealthTests, AttackStopsWhenTargetCloaksAgain)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *target = this->spawnTheirs("test:stealth_perm", this->base + Vector2f{IN_RANGE, 0});
	GameObject *revealer = this->spawnNextTo("test:revealer", target);

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(revealer, nullptr);

	// Revealed by the revealer, which can then be targeted
	this->tick(2);
	ASSERT_FALSE(this->hiddenFromUs(target));
	attacker->attack(target, false, true);
	this->tick(20);
	ASSERT_EQ(this->damageTaken(target), SHOT_DAMAGE);

	// The weapon is ready again after 200 ticks, but the target cloaked in the meantime.
	this->tick(300);
	EXPECT_TRUE(this->hiddenFromUs(target)) << "It should have cloaked again, even with the revealer around";
	EXPECT_EQ(this->damageTaken(target), SHOT_DAMAGE) << "The attack should have stopped when the target cloaked";
}

//! 4. Stealth is revealed by "reveal_stealth" only, camouflage by "detector" only.
TEST_F(StealthTests, StealthAndCamouflageAreRevealedByDifferentObjects)
{
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base);
	GameObject *stealth = this->spawnTheirs("test:stealth_perm", this->base + Vector2f{IN_RANGE, 0});
	GameObject *camo = this->spawnTheirs("test:camo_perm", this->base + Vector2f{0, IN_RANGE});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(camo, nullptr);

	// Each one is next to the kind of object that doesn't reveal it
	GameObject *detector = this->spawnNextTo("test:detector", stealth);
	GameObject *revealer = this->spawnNextTo("test:revealer", camo);

	ASSERT_NE(detector, nullptr);
	ASSERT_NE(revealer, nullptr);
	this->tick(ACQUIRE_TICKS);
	EXPECT_TRUE(this->hiddenFromUs(stealth)) << "A detector doesn't reveal stealth";
	EXPECT_TRUE(this->hiddenFromUs(camo)) << "A reveal_stealth object doesn't reveal camouflage";
	EXPECT_EQ(stealth->getHealth(), stealth->getMaxHealth());
	EXPECT_EQ(camo->getHealth(), camo->getMaxHealth());

	// And now the right kind
	GameObject *revealer2 = this->spawnNextTo("test:revealer", stealth);
	GameObject *detector2 = this->spawnNextTo("test:detector", camo);

	ASSERT_NE(revealer2, nullptr);
	ASSERT_NE(detector2, nullptr);
	this->tick(2);
	EXPECT_FALSE(this->hiddenFromUs(stealth)) << "The revealer should have revealed the stealth";
	EXPECT_FALSE(this->hiddenFromUs(camo)) << "The detector should have revealed the camouflage";
	this->tick(ACQUIRE_TICKS);
	// The attacker only has time to shoot once
	EXPECT_EQ(this->damageTaken(stealth) + this->damageTaken(camo), SHOT_DAMAGE);
}

//! 5. The radius is respected.
TEST_F(StealthTests, DetectorHasARange)
{
	GameObject *camo = this->spawnTheirs("test:camo_perm", this->base);
	GameObject *detector = this->spawnMine("test:detector", this->base + Vector2f{0, 400});

	ASSERT_NE(camo, nullptr);
	ASSERT_NE(detector, nullptr);

	// The detector has a radius of 3 tiles
	placeAt(*detector, this->base + Vector2f{ADJACENT * 5, 0});
	this->tick(2);
	EXPECT_TRUE(this->hiddenFromUs(camo)) << "5 tiles away is too far";

	placeAt(*detector, this->base + Vector2f{ADJACENT * 3, 0});
	this->tick(2);
	EXPECT_FALSE(this->hiddenFromUs(camo)) << "3 tiles away is in range";
}

//! 5b. Objects marked with "auto_attack": false never shoot on their own, but obey orders.
TEST_F(StealthTests, ObjectsWithoutAutoAttackOnlyShootWhenTold)
{
	GameObject *manual = this->spawnMine("test:atk_hitscan_manual", this->base);
	GameObject *automatic = this->spawnMine("test:atk_hitscan", this->base + Vector2f{0, -1500});
	GameObject *target = this->spawnTheirs("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(manual, nullptr);
	ASSERT_NE(automatic, nullptr);
	ASSERT_NE(target, nullptr);
	EXPECT_FALSE(manual->autoAttacks());
	EXPECT_TRUE(automatic->autoAttacks()) << "Auto attack is on by default";

	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(target->getHealth(), target->getMaxHealth()) << "Nothing is in range of the automatic one, and the other one stays quiet";

	manual->attack(target, false, true);
	this->tick(ACQUIRE_TICKS);
	EXPECT_EQ(this->damageTaken(target), SHOT_DAMAGE) << "Direct orders still work";
}

//! 6. A permanent stealth uncloaks completely before it fires, and stays visible
//! for as long as it keeps firing. Its timer to cloak again starts after the last shot.
TEST_F(StealthTests, FiringKeepsTheShooterVisible)
{
	// Fires every 20 ticks, and tries to cloak again 50 ticks after being revealed.
	GameObject *shooter = this->spawnTheirs("test:stealth_atk", this->base);
	GameObject *victim = this->spawnMine("test:target_trigger", this->base + Vector2f{IN_RANGE, 0});

	ASSERT_NE(shooter, nullptr);
	ASSERT_NE(victim, nullptr);
	ASSERT_TRUE(this->hiddenFromUs(shooter));

	bool fired = false;

	for (unsigned i = 1; i <= 100 && !fired; i++) {
		this->tick(1);
		fired = victim->getHealth() != victim->getMaxHealth();
		if (!fired)
			continue;
		EXPECT_EQ(shooter->getStealthState(), STEALTH_VISIBLE) << "It has to be fully uncloaked when shooting";
		EXPECT_GT(i, 4u) << "It can't shoot before the uncloak animation is over";
	}
	ASSERT_TRUE(fired) << "It should have fired";

	// Still shooting: it has to stay visible long after the 50 ticks of delay
	for (unsigned i = 0; i < 120; i++) {
		this->tick(1);
		ASSERT_EQ(shooter->getStealthState(), STEALTH_VISIBLE) << "tick " << i << ": it flickers while shooting";
		ASSERT_FALSE(this->hiddenFromUs(shooter)) << "tick " << i;
	}
	EXPECT_GE(this->damageTaken(victim), 5 * SHOT_DAMAGE) << "It was still shooting";

	// Stops shooting
	placeAt(*victim, this->base + Vector2f{0, 1000});
	this->tick(25);
	EXPECT_FALSE(this->hiddenFromUs(shooter)) << "Less than 50 ticks since the last shot";
	this->tick(35);
	EXPECT_TRUE(this->hiddenFromUs(shooter)) << "More than 50 ticks since the last shot: it cloaks again";
}

//! 7. Getting too close reveals a permanent stealth for a while only, and
//! the revealer has to leave and come back to reveal it again.
TEST_F(StealthTests, ProximityRevealIsTemporary)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_perm", this->base);
	GameObject *revealer = this->spawnMine("test:revealer", this->base + Vector2f{0, 400});

	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(revealer, nullptr);
	this->tick(5);
	ASSERT_TRUE(this->hiddenFromUs(stealth));

	placeAt(*revealer, this->base + Vector2f{ADJACENT, 0});

	// The delay (60) only starts once it is completely visible
	unsigned uncloaking = 0;

	while (stealth->getStealthState() != STEALTH_VISIBLE && uncloaking < 20) {
		this->tick(1);
		uncloaking++;
	}
	ASSERT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);

	unsigned visible = 0;

	while (stealth->getStealthState() == STEALTH_VISIBLE && visible < 100) {
		this->tick(1);
		visible++;
	}
	EXPECT_GE(visible, 59u) << "It has to stay uncloaked for a bit";
	EXPECT_LE(visible, 62u);
	EXPECT_TRUE(this->hiddenFromUs(stealth)) << "It cloaks again despite the revealer being there";

	// Leaves and comes back
	placeAt(*revealer, this->base + Vector2f{0, 400});
	this->tick(2);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
	placeAt(*revealer, this->base + Vector2f{ADJACENT, 0});
	this->tick(2);
	EXPECT_FALSE(this->hiddenFromUs(stealth)) << "The revealer got in range again";
}

//! 8. Being revealed for a second time doesn't postpone cloaking again.
TEST_F(StealthTests, RevealedAgainDoesNotResetTheTimer)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_perm", this->base);
	GameObject *revealer = this->spawnMine("test:revealer", this->base + Vector2f{0, 400});
	GameObject *second = this->spawnMine("test:revealer", this->base + Vector2f{400, 0});

	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(revealer, nullptr);
	ASSERT_NE(second, nullptr);
	this->tick(2);

	placeAt(*revealer, this->base + Vector2f{ADJACENT, 0});
	this->tick(40);
	ASSERT_FALSE(this->hiddenFromUs(stealth));
	// Another one shows up, 40 ticks after the first
	placeAt(*second, this->base + Vector2f{-ADJACENT, 0});
	this->tick(40);
	EXPECT_TRUE(this->hiddenFromUs(stealth)) << "The first reveal started the timer: 80 ticks in, it should be cloaked";
}

//! 9. A damaged object that can't cloak flickers, and is a regular target while doing so.
TEST_F(StealthTests, DamagedObjectFailsToCloakAndFlickers)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_flicker", this->base);
	GameObject *attacker = this->spawnMine("test:atk_hitscan", this->base + Vector2f{-IN_RANGE, 0});
	GameObject *revealer = this->spawnNextTo("test:revealer", stealth);

	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(revealer, nullptr);
	// Under half of the health: red
	stealth->setHealth(400);

	// Revealed for 20 ticks, then tries to cloak and can't
	this->tick(40);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_FLICKERING);
	for (unsigned i = 0; i < 150; i++) {
		this->tick(1);
		ASSERT_FALSE(this->hiddenFromUs(stealth)) << "A flickering object isn't cloaked (tick " << i << ")";
		ASSERT_EQ(stealth->getStealthState(), STEALTH_FLICKERING) << "tick " << i;
	}
	EXPECT_LT(stealth->getHealth(), 400u) << "It was visible: the attacker should have shot it";

	// Repaired: the next try works
	stealth->setHealth(stealth->getMaxHealth());
	this->tick(20);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_CLOAKED);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
}

//! 10. With a 100% chance, a damaged object cloaks just fine.
TEST_F(StealthTests, DamagedObjectWithCertainRecloakChanceCloaks)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_lucky", this->base);
	GameObject *revealer = this->spawnNextTo("test:revealer", stealth);

	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(revealer, nullptr);
	stealth->setHealth(400);
	this->tick(40);

	EXPECT_EQ(stealth->getStealthState(), STEALTH_CLOAKED);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
}

//! 11. Temporary stealth: charge, activate with the deploy action, it costs energy
//! until there is none left.
TEST_F(StealthTests, TemporaryStealthChargesActivatesAndWearsOff)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp", this->base);

	ASSERT_NE(stealth, nullptr);
	this->tick(1);

	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);
	EXPECT_FALSE(this->hiddenFromUs(stealth));
	EXPECT_TRUE(stealth->isDeployable());
	EXPECT_TRUE(stealth->deployKeepsOrders());
	EXPECT_FALSE(stealth->canDeploy()) << "Not enough energy yet (it needs 10)";
	EXPECT_EQ(stealth->getMaxEnergy(), 20u);

	// Trying anyway does nothing
	stealth->deploy();
	this->tick(2);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);

	this->tick(40);
	EXPECT_EQ(stealth->getEnergy(), stealth->getMaxEnergy()) << "1 energy per tick, up to the maximum";
	ASSERT_TRUE(stealth->canDeploy());

	stealth->deploy();
	this->tick(10);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_CLOAKED);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
	EXPECT_EQ(stealth->getEnergy(), 20u) << "The first payment is 100 ticks after the activation";

	// 10 energy every 100 ticks
	this->tick(95);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
	EXPECT_EQ(stealth->getEnergy(), 10u);
	this->tick(100);
	EXPECT_FALSE(this->hiddenFromUs(stealth)) << "No energy left: the effect is over";
	EXPECT_LT(stealth->getEnergy(), 10u) << "It charges again once the effect is over";
	EXPECT_FALSE(stealth->canDeploy());
	this->tick(10);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);
	EXPECT_TRUE(stealth->canDeploy());
}

//! 11b. A cancelable stealth can be ended by hand, and keeps the energy that is left.
TEST_F(StealthTests, CancelableStealthCanBeEndedByHand)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp", this->base);

	ASSERT_NE(stealth, nullptr);
	this->tick(40);
	ASSERT_TRUE(stealth->canDeploy());
	stealth->deploy();
	this->tick(10);
	ASSERT_TRUE(this->hiddenFromUs(stealth));
	EXPECT_TRUE(stealth->canDeploy()) << "Deploying again ends it";

	stealth->deploy();
	this->tick(1);
	EXPECT_FALSE(this->hiddenFromUs(stealth));
	this->tick(10);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);
	EXPECT_EQ(stealth->getEnergy(), 20u) << "Nothing was spent";
	EXPECT_TRUE(stealth->canDeploy());
}

//! 11c. A stealth that can't be canceled lasts until all the energy is spent.
TEST_F(StealthTests, StealthThatIsNotCancelableRunsOut)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp_locked", this->base);

	ASSERT_NE(stealth, nullptr);
	this->tick(40);
	ASSERT_TRUE(stealth->canDeploy());
	stealth->deploy();
	this->tick(10);
	ASSERT_TRUE(this->hiddenFromUs(stealth));
	EXPECT_FALSE(stealth->canDeploy()) << "It can't be canceled";

	stealth->deploy();
	this->tick(20);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
	this->tick(150);
	EXPECT_TRUE(this->hiddenFromUs(stealth)) << "The energy lasts 200 ticks";
	this->tick(40);
	EXPECT_FALSE(this->hiddenFromUs(stealth)) << "All the energy was spent";
}

//! 12. A temporary stealth is revealed for as long as revealers are around.
TEST_F(StealthTests, TemporaryStealthIsRevealedWhileRevealersAreClose)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp", this->base);

	ASSERT_NE(stealth, nullptr);
	this->tick(40);
	ASSERT_TRUE(stealth->canDeploy());
	stealth->deploy();
	this->tick(10);
	ASSERT_TRUE(this->hiddenFromUs(stealth));

	GameObject *revealer = this->spawnNextTo("test:revealer", stealth);

	ASSERT_NE(revealer, nullptr);
	this->tick(2);
	EXPECT_FALSE(this->hiddenFromUs(stealth));
	EXPECT_EQ(stealth->getStealthState(), STEALTH_CLOAKED) << "Still cloaked, only seen by those who are close";
	this->tick(100);
	EXPECT_FALSE(this->hiddenFromUs(stealth)) << "Revealers around reveal temporary stealths for good";

	placeAt(*revealer, this->base + Vector2f{0, 400});
	this->tick(2);
	EXPECT_TRUE(this->hiddenFromUs(stealth));
}

//! 13. Firing ends a temporary stealth (after uncloaking completely).
TEST_F(StealthTests, FiringEndsTemporaryStealth)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp", this->base);
	GameObject *victim = this->spawnMine("test:target_trigger", this->base + Vector2f{0, 1000});

	ASSERT_NE(stealth, nullptr);
	ASSERT_NE(victim, nullptr);
	this->tick(40);
	ASSERT_TRUE(stealth->canDeploy());
	stealth->deploy();
	this->tick(1);
	ASSERT_TRUE(this->hiddenFromUs(stealth));

	// The victim shows up
	placeAt(*victim, this->base + Vector2f{IN_RANGE, 0});
	// 20 ticks is the cooldown of its weapon (the energy lasts 200 ticks)
	for (unsigned i = 0; i < 40 && victim->getHealth() == victim->getMaxHealth(); i++)
		this->tick(1);
	ASSERT_NE(victim->getHealth(), victim->getMaxHealth()) << "It should have fired";
	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE) << "It was completely uncloaked when it shot";
	EXPECT_FALSE(this->hiddenFromUs(stealth));
	EXPECT_LT(stealth->getEnergy(), stealth->getMaxEnergy());
}

//! 14. A damaged object can't use its temporary stealth properly either.
TEST_F(StealthTests, DamagedTemporaryStealthFlickersUntilItWearsOff)
{
	GameObject *stealth = this->spawnTheirs("test:stealth_temp_damaged", this->base);

	ASSERT_NE(stealth, nullptr);
	stealth->setHealth(400);
	this->tick(2);
	ASSERT_TRUE(stealth->canDeploy());
	stealth->deploy();

	for (unsigned i = 0; i < 80; i++) {
		this->tick(1);
		ASSERT_EQ(stealth->getStealthState(), STEALTH_FLICKERING) << "tick " << i;
		ASSERT_FALSE(this->hiddenFromUs(stealth)) << "tick " << i;
	}
	// Lasts 100 ticks in total
	this->tick(30);
	EXPECT_EQ(stealth->getStealthState(), STEALTH_VISIBLE);
}

//! 15. Drawing the stealthed objects (ours and theirs) in every state doesn't
//! blow up: ticking draws everything.
TEST_F(StealthTests, DrawingStealthedObjectsInEveryStateWorks)
{
	GameObject *ally = this->spawnMine("test:stealth_perm", this->base);
	GameObject *allyTransparent = this->spawnMine("test:stealth_transparent", this->base + Vector2f{IN_RANGE, 0});
	GameObject *enemyObj = this->spawnTheirs("test:stealth_perm", this->base + Vector2f{0, IN_RANGE});
	GameObject *enemyFlicker = this->spawnTheirs("test:stealth_flicker", this->base + Vector2f{IN_RANGE, IN_RANGE});
	GameObject *theirRevealer = this->spawnTheirs("test:revealer", this->base + Vector2f{0, -IN_RANGE});
	GameObject *myRevealer = this->spawnMine("test:revealer", this->base + Vector2f{-IN_RANGE, 0});

	ASSERT_NE(ally, nullptr);
	ASSERT_NE(allyTransparent, nullptr);
	ASSERT_NE(enemyObj, nullptr);
	ASSERT_NE(enemyFlicker, nullptr);
	ASSERT_NE(theirRevealer, nullptr);
	ASSERT_NE(myRevealer, nullptr);
	enemyFlicker->setHealth(100);
	this->tick(5);
	for (unsigned i = 0; i < 4; i++) {
		placeAt(*theirRevealer, ally->getMiddle() + Vector2f{ADJACENT, 0});
		placeAt(*myRevealer, enemyObj->getMiddle() + Vector2f{ADJACENT, 0});
		this->tick(1);
		for (unsigned j = 0; j < 10; j++)
			this->tick(1);
		placeAt(*theirRevealer, allyTransparent->getMiddle() + Vector2f{ADJACENT, 0});
		placeAt(*myRevealer, enemyFlicker->getMiddle() + Vector2f{ADJACENT, 0});
		this->tick(80);
		// The minimap leaves the hidden objects out
		this->manager->renderMinimap({0, 0}, {128, 128});
		placeAt(*theirRevealer, this->base + Vector2f{0, -IN_RANGE});
		placeAt(*myRevealer, this->base + Vector2f{-IN_RANGE, 0});
		this->tick(2);
	}
	SUCCEED();
}

//! 16. The real d2k data: fremen, stealth raiders (permanent) and saboteurs (temporary) are stealthed, and
//! infantry and turrets reveal them.
TEST_F(StealthTests, Dune2000StealthData)
{
	const std::set<std::string> stealthed{"d2k:frem", "d2k:srid", "d2k:sabo"};
	std::set<std::string> found;
	unsigned infantry = 0;

	for (auto &faction : this->manager->getFactions())
		for (auto &[id, object] : faction.getObjects()) {
			if (!id.starts_with("d2k:"))
				continue;
			auto &tags = object.getTags();

			if (stealthed.contains(id)) {
				found.insert(id);
				ASSERT_TRUE(object.getStealthParams().has_value()) << id;

				auto &s = *object.getStealthParams();

				EXPECT_EQ(s.type, StealthParams::TYPE_STEALTH) << id;
				if (id == "d2k:sabo") {
					// The saboteur has to charge, then spends its energy
					EXPECT_FALSE(s.permanent) << id;
					EXPECT_GT(object.getMaxEnergy(), 0u) << id;
					EXPECT_EQ(object.getEnergy(), 0u) << id << ": it spawns empty";
					EXPECT_GT(s.cost, 0u) << id;
					EXPECT_EQ(s.activationThreshold, object.getMaxEnergy()) << id << ": it has to be fully charged";
					EXPECT_FALSE(s.cancelable) << id;
				} else
					EXPECT_TRUE(s.permanent) << id;
				EXPECT_FALSE(object.autoAttacks()) << id << ": stealthed units don't shoot on their own";
				EXPECT_TRUE(s.recloakDelay.has_value()) << id;
				ASSERT_TRUE(s.damagedThreshold.has_value()) << id;
				EXPECT_FLOAT_EQ(*s.damagedThreshold, 0.25f) << id;
				EXPECT_FLOAT_EQ(s.recloakChance, 0.f) << id << ": units in red can never cloak again";
				EXPECT_FALSE(s.mask.empty()) << id << ": it should have a cloak animation";
				EXPECT_EQ(s.allyDisplay, StealthParams::DISPLAY_SHADOW) << id;
				EXPECT_EQ(s.allyColor.a, 128) << id;
				EXPECT_EQ(s.enemyDisplay, StealthParams::DISPLAY_INVISIBLE) << id;
				EXPECT_EQ(object.getStealthState(), s.permanent ? STEALTH_CLOAKED : STEALTH_VISIBLE) << id;
			} else
				EXPECT_FALSE(object.getStealthParams().has_value()) << id << " shouldn't be stealthed";
			if (std::ranges::find(tags, "infantry") != tags.end()) {
				infantry++;
				EXPECT_GT(object.getRevealStealthRange(), 0.f) << id << " is infantry: it should reveal stealthed units";
			}
			if (id == "d2k:gunt" || id == "d2k:roct")
				EXPECT_GT(object.getRevealStealthRange(), 0.f) << id << " is a turret: it should reveal stealthed units";
			if (id == "d2k:roct")
				EXPECT_GT(object.getDetectorRange(), 0.f) << id << " is a missile turret: it should be a detector";
		}
	EXPECT_EQ(found, stealthed);
	EXPECT_GT(infantry, 0u);
}
