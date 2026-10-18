//
// Created for the MixRTS test target.
//

#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "Mock/MockedUI.hpp"
#include "Data/GameObject.hpp"

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
	EXPECT_EQ(this->getMcv(), nullptr) << "The MCV should be gone once it has deployed";

	ASSERT_TRUE(this->player->state.queues["d2k:building"].entries.contains("d2k:wtra"));
	EXPECT_TRUE(this->player->state.queues["d2k:building"].entries.at("d2k:wtra").unlocked) << "The wind trap should be unlocked once a construction yard has been built";
}
