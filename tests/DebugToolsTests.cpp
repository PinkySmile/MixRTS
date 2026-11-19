//
// Created for the MixRTS test target.
//
// Smoke and behavior tests for the debug tools: the profiler, the warhead
// log, the pause on warhead contact and the overlays (that only get to run
// without crashing here: what they draw is meant to be looked at by humans,
// run the test binary with --mode debug to do so).
//

#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "Debug/Profiler.hpp"
#include "Data/GameManager.hpp"
#include "../src/Engine/Data/Objects/GameObject.hpp"
#include "Data/Player.hpp"

namespace {

void placeAt(GameObject &obj, Vector2f center)
{
	obj.setPosition(center - Vector2f(obj.getSize().to<float>()) / 2.f);
}

GameObject *spawnAt(GameManager &manager, size_t owner, const std::string &id, Vector2f center)
{
	GameObject *obj = manager.spawnObject(owner, Vector2i(center.to<int>()), "", id);

	if (obj)
		placeAt(*obj, center);
	return obj;
}

} // namespace

class DebugToolsTests : public GameFixture {
protected:
	Player *enemy = nullptr;

	void SetUp() override
	{
		GameFixture::SetUp();
		this->manager->addPlayer(3, HSLColor{0, 0, 500}, Vector2i{0, 0}, 1);
		this->player = &this->manager->getPlayers()[0];
		this->enemy = &this->manager->getPlayers()[1];
	}

	void TearDown() override
	{
		Profiler::enabled = false;
		Profiler::get().frozen = false;
		Profiler::get().autoFreezeMs = 0;
		Profiler::get().clear();
	}
};

TEST_F(DebugToolsTests, ProfilerMergesScopesPerParent)
{
	Profiler &profiler = Profiler::get();

	profiler.clear();
	Profiler::enabled = true;
	profiler.beginFrame();
	{
		PROFILE_SCOPE("outer");
		for (int i = 0; i < 5; i++) {
			PROFILE_SCOPE("inner");
		}
		PROFILE_COUNT(PROFCOUNTER_PATHFINDS, 3);
	}
	{
		PROFILE_SCOPE("other");
	}
	profiler.endFrame();

	ASSERT_EQ(profiler.size(), 1U);

	auto &frame = profiler.getFrame(0);

	EXPECT_EQ(frame.nodes.size(), 4U) << "root + outer + inner + other";
	EXPECT_EQ(frame.counters[PROFCOUNTER_PATHFINDS], 3U);

	int inner = Profiler::findPath(frame, "outer/inner");

	ASSERT_GE(inner, 0);
	EXPECT_EQ(frame.nodes[inner].calls, 5U);
	EXPECT_EQ(Profiler::findPath(frame, "inner"), -1);

	auto stats = profiler.computeStats(10);

	ASSERT_EQ(stats.size(), 3U);
	EXPECT_EQ(stats[0].path, "other");
	EXPECT_EQ(stats[1].path, "outer");
	EXPECT_EQ(stats[2].path, "outer/inner");
}

TEST_F(DebugToolsTests, ProfilerIgnoresScopesWhenDisabledAndFreezes)
{
	Profiler &profiler = Profiler::get();

	profiler.clear();
	Profiler::enabled = false;
	profiler.beginFrame();
	{
		PROFILE_SCOPE("ignored");
	}
	profiler.endFrame();
	ASSERT_EQ(profiler.size(), 1U);
	EXPECT_EQ(profiler.getFrame(0).nodes.size(), 1U) << "Only the root node when the profiler is disabled";
	EXPECT_GT(profiler.getFrame(0).frameNs, 0U) << "Frame times are recorded anyway";

	profiler.frozen = true;
	profiler.beginFrame();
	profiler.endFrame();
	EXPECT_EQ(profiler.size(), 1U) << "A frozen profiler doesn't record new frames";
}

TEST_F(DebugToolsTests, GameLoopIsProfiled)
{
	Profiler &profiler = Profiler::get();

	profiler.clear();
	Profiler::enabled = true;
	profiler.beginFrame();
	this->tick(1);
	profiler.endFrame();

	ASSERT_EQ(profiler.size(), 1U);

	auto &frame = profiler.getFrame(0);

	EXPECT_GE(Profiler::findPath(frame, "update/objects"), 0);
	EXPECT_GE(Profiler::findPath(frame, "render"), 0);
	EXPECT_GT(frame.counters[PROFCOUNTER_OBJECTS], 0U);
}

TEST_F(DebugToolsTests, WarheadHitIsRecordedWithItsTargets)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_hitscan", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({attacker, target});
	this->manager->debugOptions.warheads = true;
	attacker->attack(target, false, true);
	this->tick(5);

	auto &hits = this->manager->getWarheadHits();

	ASSERT_FALSE(hits.empty());

	auto &hit = hits.front();

	EXPECT_LE(hit.tick, this->manager->getTick());
	EXPECT_EQ(hit.baseDamage, 100U);
	ASSERT_FALSE(hit.targets.empty());
	EXPECT_EQ(hit.targets.front().objectId, target->index);
	EXPECT_EQ(hit.targets.front().damage, 100U);
}

TEST_F(DebugToolsTests, WarheadsAreNotRecordedWhenDisabled)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_hitscan", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({attacker, target});
	attacker->attack(target, false, true);
	this->tick(5);
	EXPECT_EQ(target->getHealth(), target->getMaxHealth() - 100);
	EXPECT_TRUE(this->manager->getWarheadHits().empty());
}

TEST_F(DebugToolsTests, PauseOnWarheadContactFreezesTheTickOfContact)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_projectile", base);
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({attacker, target});
	this->manager->debugOptions.pauseOnWarhead = true;
	attacker->attack(target, false, true);
	this->tick(120);

	ASSERT_FALSE(this->manager->getWarheadHits().empty());
	EXPECT_TRUE(this->manager->isPaused());
	EXPECT_EQ(this->manager->getWarheadHits().back().tick, this->manager->getTick()) << "The game should stop on the very tick of the contact";

	auto tickOfContact = this->manager->getTick();

	this->tick(10);
	EXPECT_EQ(this->manager->getTick(), tickOfContact) << "Paused games don't advance";
	this->manager->requestStep();
	this->tick(1);
	EXPECT_EQ(this->manager->getTick(), tickOfContact + 1) << "Stepping advances a single tick";
}

TEST_F(DebugToolsTests, EveryOverlayRendersWithoutCrashing)
{
	Vector2f base{2000, 2000};
	GameObject *attacker = spawnAt(*this->manager, this->player->state.index, "test:atk_projectile", base);
	GameObject *homing = spawnAt(*this->manager, this->player->state.index, "test:atk_homing", base + Vector2f{0, 100});
	GameObject *target = spawnAt(*this->manager, this->enemy->state.index, "test:target", base + Vector2f{200, 0});
	auto &opts = this->manager->debugOptions;
	auto &profiler = Profiler::get();

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(homing, nullptr);
	ASSERT_NE(target, nullptr);
	this->focusCamera({attacker, homing, target});
	this->player->uiState.selectedObjects.insert(attacker);
	this->player->uiState.selectedObjects.insert(homing);
	// The player UI (selection boxes...) is drawn by the mod's own shared library,
	// which isn't linked against the mocked core in this test binary: leave it out.
	opts.showPlayerOverlays = false;
	opts.inspector = true;
	opts.inspectorBoxes = true;
	opts.inspectorPath = true;
	opts.projectiles = true;
	opts.warheads = true;
	opts.perfGraph = true;
	Profiler::enabled = true;
	attacker->attack(target, false, true);
	homing->attack(target, false, true);

	for (int view = 0; view < 4; view++) {
		opts.perfView = static_cast<PerfView>(view);
		opts.perfSubtree = "update";
		for (int filter = 0; filter < 4; filter++) {
			opts.renderFilter = static_cast<RenderOrderFilter>(filter);
			for (int i = 0; i < 25; i++) {
				profiler.beginFrame();
				this->tick(1);
				profiler.endFrame();
			}
		}
	}

	// Selected objects that die must not leave dangling pointers behind the overlay
	target->damage(target->getHealth(), DMGTYPE_GENERIC);
	attacker->damage(attacker->getHealth(), DMGTYPE_GENERIC);
	for (int i = 0; i < 120; i++) {
		profiler.beginFrame();
		this->tick(1);
		profiler.endFrame();
	}
	SUCCEED();
}

TEST_F(DebugToolsTests, HiddenLayersAndGridViewsRenderWithoutCrashing)
{
	auto &opts = this->manager->debugOptions;

	opts.showTerrain = false;
	opts.showObjects = false;
	opts.showEffects = false;
	opts.showProjectiles = false;
	opts.showSkirts = false;
	opts.showPlayerOverlays = false;
	opts.showLayers = {false, false, true, false, false};
	for (size_t view = 0; view < this->manager->getGridViewCount(); view++) {
		opts.gridView = view;
		this->tick(1);
		EXPECT_NE(this->manager->getGridViewLabel(view), "?");
	}
	opts.gridView = 255;
	EXPECT_EQ(this->manager->getGridViewLabel(255), "None");
}

TEST_F(DebugToolsTests, ObjectInspectorDescribesTheObject)
{
	GameObject *mcv = this->getMcv();

	ASSERT_NE(mcv, nullptr);

	std::unordered_set<const GameObject *> alive{mcv};
	DebugInfoList info;

	mcv->getDebugInfo(info, alive);
	ASSERT_FALSE(info.empty());
	EXPECT_EQ(info.front().first, "id");
	EXPECT_EQ(info.front().second, "d2k:mcv");
	EXPECT_EQ(mcv->getDebugOrderSummary(alive), "idle");
}

TEST_F(DebugToolsTests, DebugPanelOpensAndCloses)
{
	this->manager->toggleDebugPanel();
	this->tick(2);
	this->manager->toggleDebugPanel();
	this->tick(2);
	this->manager->toggleDebugPanel();
	this->tick(2);
	SUCCEED();
}

TEST_F(DebugToolsTests, ControlledPlayerCanBeSwitched)
{
	EXPECT_EQ(this->manager->getControlledPlayer(), 0U);
	this->manager->setControlledPlayer(1);
	EXPECT_EQ(this->manager->getControlledPlayer(), 1U);
	EXPECT_FALSE(this->manager->getPlayers()[0].isEnabled());
	EXPECT_TRUE(this->manager->getPlayers()[1].isEnabled());
	this->manager->debugNextPlayer();
	EXPECT_EQ(this->manager->getControlledPlayer(), 0U) << "Wraps around";
	this->manager->setControlledPlayer(42);
	EXPECT_EQ(this->manager->getControlledPlayer(), 0U) << "Invalid indices are ignored";

	// The panel (with its player combo box) builds and refreshes. The "test:dummy"
	// player isn't enabled while ticking: it has no cursors to display.
	this->manager->toggleDebugPanel();
	this->tick(2);
	this->manager->toggleDebugPanel();
}

TEST_F(DebugToolsTests, OpenPanelTagsObjectsAndDescribesThePlayer)
{
	// Player UI selection boxes can't be drawn from the test binary, see EveryOverlayRendersWithoutCrashing
	this->manager->debugOptions.showPlayerOverlays = false;
	this->manager->toggleDebugPanel();
	ASSERT_TRUE(this->manager->isDebugPanelOpen());
	// Nothing selected: the left side describes the player. Render as is, then with a selection.
	this->tick(2);
	for (auto obj : this->player->state.myObjects) {
		this->player->uiState.selectedObjects.insert(obj);
		break;
	}
	this->tick(2);
	this->manager->debugOptions.inspector = true;
	this->tick(2);
	this->manager->toggleDebugPanel();
	EXPECT_FALSE(this->manager->isDebugPanelOpen());
	this->tick(2);
}
