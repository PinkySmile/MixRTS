//
// Created for the MixRTS test target.
//

#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Mock/MockedUI.hpp"
#include "Data/GameObject.hpp"
#include "Data/GridMask.hpp"
#include "Data/Map.hpp"
#include "Data/Player.hpp"

//! With no orders at all, the MCV spawned for a freshly created player should
//! just sit still.
TEST_F(GameFixture, McvDoesNotMoveWithoutOrders)
{
	GameObject *mcv = this->getMcv();

	ASSERT_NE(mcv, nullptr) << "The freshly created player should own a MCV";

	Vector2i startPos = mcv->getPosition();

	this->tick(10);
	mcv = this->getMcv();
	ASSERT_NE(mcv, nullptr) << "The MCV should still be there";
	EXPECT_EQ(mcv->getPosition(), startPos) << "The MCV moved despite never being given any order";
}

//! Injecting a fake "deploy" UI event (as MockedUI lets tests do) for the MCV
//! should make it deploy into a construction yard, which in turn should
//! unlock buildings that only require having a construction yard (such as the
//! wind trap).
TEST_F(GameFixture, DeployingMcvBuildsConstructionYardAndUnlocksBuildings)
{
	GameObject *mcv = this->getMcv();

	ASSERT_NE(mcv, nullptr) << "The freshly created player should own a MCV";

	// The wind trap ("d2k:wtra") has no requirements of its own, but every
	// object in the "building" queue also needs an actual queue producer (i.e.
	// a construction yard) before it can be unlocked: it must not be unlocked
	// yet, since no construction yard has been built.
	{
		auto &entries = this->player->state.queues["d2k:building"].entries;

		ASSERT_TRUE(!entries.contains("d2k:wtra") || !entries.at("d2k:wtra").unlocked);
	}

	// Replace the player's own UI with a MockedUI wrapping it, so we can inject
	// a fake deploy event exactly like a real UI would generate when the
	// player clicks the MCV to deploy it.
	auto &uis = this->player->getUIs();
	auto *mockedUi = new MockedUI(uis[this->player->state.factionIndex].release());

	uis[this->player->state.factionIndex].reset(mockedUi);
	mockedUi->orderQueue.emplace_back(UI::DeployUnitEvent{mcv->index});

	// Give the simulation enough ticks for the event to be consumed, the MCV
	// to turn to face the deploy direction, and then actually deploy.
	this->tick(100);

	GameObject *conyard = nullptr;

	for (auto obj : this->player->state.myObjects)
		if (obj->getId() == "d2k:a_cony")
			conyard = obj;

	ASSERT_NE(conyard, nullptr) << "The MCV should have deployed into a construction yard";
	this->focusCamera({conyard});
	EXPECT_EQ(this->getMcv(), nullptr) << "The MCV should be gone once it has deployed";

	ASSERT_TRUE(this->player->state.queues["d2k:building"].entries.contains("d2k:wtra"));
	EXPECT_TRUE(this->player->state.queues["d2k:building"].entries.at("d2k:wtra").unlocked) << "The wind trap should be unlocked once a construction yard has been built";
}

//! Deploying a Devastator starts a countdown. Once that countdown is the order
//! being executed, a move command does nothing, and when the delay elapses the
//! unit explodes and damages whatever stands next to it.
TEST_F(GameFixture, DevastatorSelfDestructDamagesAndCannotBeStopped)
{
	// "test:dummy" (faction index 3) owns the bystander: it starts with nothing,
	// so this doesn't drop an MCV on top of the explosion.
	this->manager->addPlayer(3, HSLColor{0, 0, 500}, 0, 1);
	this->player = &this->manager->getPlayers()[0];

	Player *enemy = &this->manager->getPlayers()[1];
	// Far enough from the blast center (the Devastator's own center) to survive
	// the 5000 damage falling off over the warhead's 96px radius, but close
	// enough to actually be hit.
	Vector2f base{2000, 2000};
	GameObject *deva = spawnAt(*this->manager, this->player->state.index, "d2k:deva", base);
	GameObject *target = spawnAt(*this->manager, enemy->state.index, "test:target", base + Vector2f{80, 0});

	ASSERT_NE(deva, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({deva, target});

	MockedUI *ui = installMockedUI(*this->player);

	ui->orderQueue.emplace_back(UI::DeployUnitEvent{deva->index});

	bool started = false;

	for (unsigned i = 0; i < 30; i++) {
		this->tick(1);
		if (deva->isSelfDestructing()) {
			started = true;
			break;
		}
	}
	ASSERT_TRUE(started) << "The countdown should start once the Devastator has nothing else to finish";
	ASSERT_FALSE(deva->isDead());

	Vector2i stillPos = deva->getPosition();

	ui->orderQueue.emplace_back(UI::MoveUnitEvent{{deva->index}, Vector2u{static_cast<unsigned>(stillPos.x + 400), static_cast<unsigned>(stillPos.y)}});
	this->tick(20);
	EXPECT_EQ(deva->getPosition(), stillPos) << "A move order must not interrupt the countdown";
	EXPECT_FALSE(deva->isDead());

	unsigned before = target->getHealth();
	bool died = false;

	// Stop on the tick it dies: the corpse stays for its death "stay" time, but
	// the bystander would be freed the same tick if the blast killed it too.
	for (unsigned i = 0; i < 120 && !deva->isDisposable(); i++) {
		this->tick(1);
		if (deva->isDead()) {
			died = true;
			break;
		}
	}
	EXPECT_TRUE(died) << "The Devastator should explode once the delay elapses";
	EXPECT_LT(target->getHealth(), before) << "The explosion should damage whatever stands next to it";
}

//! Deploying while the unit is still walking only queues the countdown behind
//! the step it is finishing. Another command before that step ends drops it.
TEST_F(GameFixture, DevastatorSelfDestructCanBeCancelledBeforeItStarts)
{
	MapState &map = *this->getMcv()->map;
	Vector2i origin = map.absToTileCoord(this->getMcv()->getPosition());
	Vector2i start = findFreePassableTile(*this->manager, map, origin, 6, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS, 4);
	Vector2i away = findFreePassableTile(*this->manager, map, start, 4, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS, 4);
	GameObject *deva = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, start), "", "d2k:deva");

	ASSERT_NE(deva, nullptr);
	deva->clearOrders();
	this->focusCamera({deva, FocusRegion::tiles(map, away)});
	deva->moveTo(tileCenterAbs(map, away), MOVEREASON_MOVE_COMMAND);

	Vector2i spawnedAt = deva->getPosition();
	bool moving = false;

	for (unsigned i = 0; i < 40; i++) {
		this->tick(1);
		if (deva->getPosition() != spawnedAt) {
			moving = true;
			break;
		}
	}
	ASSERT_TRUE(moving) << "The Devastator should have started its move";

	MockedUI *ui = installMockedUI(*this->player);
	Vector2i back = tileCenterAbs(map, start);

	// Both events are consumed on the next tick, while the current step is still
	// the order in front, so the countdown never gets to start.
	ui->orderQueue.emplace_back(UI::DeployUnitEvent{deva->index});
	ui->orderQueue.emplace_back(UI::MoveUnitEvent{{deva->index}, Vector2u{static_cast<unsigned>(back.x), static_cast<unsigned>(back.y)}});
	this->tick(160);
	EXPECT_FALSE(deva->isDead()) << "A command given before the countdown starts should cancel it";
	EXPECT_FALSE(deva->isSelfDestructing());
}
