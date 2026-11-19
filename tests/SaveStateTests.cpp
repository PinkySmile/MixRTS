//
// Created for the MixRTS test target.
//

#include <chrono>
#include <iostream>
#include <memory>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "TestHelpers.hpp"
#include "Data/GameManager.hpp"
#include "../src/Engine/Data/Objects/GameObject.hpp"
#include "Data/Map.hpp"
#include "Data/Player.hpp"
#include "UI/CombinedUI.hpp"

namespace {
	GameManager::UIFactory testUi()
	{
		return [](const std::vector<Faction> &factions, UIState *uiState, GameManager *manager, PlayerState *ownerState, std::vector<MapState> &mapStates, size_t playerIndex) {
			return std::make_shared<CombinedUI>(factions, uiState, manager, ownerState, mapStates, playerIndex);
		};
	}

	void expectSameBytes(const std::vector<uint8_t> &saved, const std::vector<uint8_t> &loaded)
	{
		if (saved == loaded)
			return;

		size_t at = 0;
		size_t n = std::min(saved.size(), loaded.size());

		while (at < n && saved[at] == loaded[at])
			at++;
		ADD_FAILURE() << "State bytes differ at offset " << at
			<< " (saved " << saved.size() << " bytes, loaded " << loaded.size() << " bytes)";
	}
}

//! Plays a short match, rolls it back in place, and loads the same bytes into a new manager.
//! Both must reproduce the recorded checksums. A resave that is not byte-identical means a field was lost.
TEST_F(GameFixture, RollbackAndFreshLoadMatch)
{
	const auto &areas = this->manager->getLoadedMap().startAreas;

	ASSERT_GE(areas.size(), 2u);
	this->manager->addPlayer(1, HSLColor{0, 800, 500}, areas[1].to<int>(), 1);

	MapState &map = *this->getMcv()->map;
	Vector2i spice = findResourceTile(map);
	Vector2i moveTarget = findFreePassableTile(*this->manager, map, spice, 4, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i trikeTile = findFreePassableTile(*this->manager, map, moveTarget, 6, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i enemyTile = findFreePassableTile(*this->manager, map, trikeTile, 3, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);
	Vector2i harvestTile = findFreePassableTile(*this->manager, map, spice, 2, {1, 1}, GRIDMASK_VEHICLES_CAN_PASS);

	GameObject *trike = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, trikeTile), "", "d2k:trik");
	GameObject *enemy = this->manager->spawnObject(1, tileCenterAbs(map, enemyTile), "", "d2k:trik");
	GameObject *harvester = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, harvestTile), "", "d2k:harv");
	GameObject *carryall = this->manager->spawnObject(this->player->state.index, tileCenterAbs(map, trikeTile), "", "d2k:carr");

	ASSERT_NE(trike, nullptr);
	ASSERT_NE(enemy, nullptr);
	ASSERT_NE(harvester, nullptr);
	ASSERT_NE(carryall, nullptr);
	carryall->setPosition(Vector2f(tileCenterAbs(map, trikeTile)));
	carryall->clearOrders();
	trike->moveTo(tileCenterAbs(map, moveTarget), MOVEREASON_MOVE_COMMAND);
	trike->attack(enemy, false, false);
	harvester->harvest(tileCenterAbs(map, spice), false);
	this->tick(300);

	auto &factory = this->player->state.queues.at("d2k:light_vehicle").entries.at("d2k:trik");

	factory.buildProgress = 12.f;
	factory.paused = true;
	this->player->state.queues.at("d2k:starport").stock["d2k:trik"] = 2;
	this->player->state.queues.at("d2k:starport").busy = true;

	GameManager::GameStateSnapshot saved;
	auto saveStarted = std::chrono::steady_clock::now();

	this->manager->saveState(saved);

	auto saveMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - saveStarted).count();

	std::cout << "saveState: " << saved.data.size() << " bytes in " << saveMs << " ms\n";
	ASSERT_FALSE(saved.data.empty());
	EXPECT_EQ(saved.tick, this->manager->getTick());
	EXPECT_EQ(saved.checksum, GameManager::checksumState(saved.data.data(), saved.data.size()));

	std::vector<uint64_t> checksums;
	std::vector<uint64_t> grids;

	checksums.reserve(200);
	grids.reserve(200);
	for (int i = 0; i < 200; i++) {
		this->tick(1);

		GameManager::GameStateSnapshot frame;

		this->manager->saveState(frame);
		checksums.push_back(frame.checksum);
		grids.push_back(this->manager->debugGridChecksum());
	}

	uint64_t gridAtSave = 0;
	auto loadStarted = std::chrono::steady_clock::now();

	this->manager->loadState(saved);
	gridAtSave = this->manager->debugGridChecksum();

	auto loadMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - loadStarted).count();

	std::cout << "loadState: " << loadMs << " ms\n";
	EXPECT_EQ(this->manager->getTick(), saved.tick);

	GameManager::GameStateSnapshot rolledBack;

	this->manager->saveState(rolledBack);
	expectSameBytes(saved.data, rolledBack.data);
	EXPECT_EQ(rolledBack.checksum, saved.checksum);

	for (int i = 0; i < 200; i++) {
		this->tick(1);

		GameManager::GameStateSnapshot frame;

		this->manager->saveState(frame);
		EXPECT_EQ(frame.checksum, checksums[i]) << "tick " << i << " after an in-place rollback";
		EXPECT_EQ(this->manager->debugGridChecksum(), grids[i]) << "grid tick " << i << " after an in-place rollback";
		if (frame.checksum != checksums[i])
			break;
	}

	std::vector<Faction> factions = this->manager->getFactions();
	auto fresh = std::make_unique<GameManager>(factions, 2, GameManager::GameSettings{.shortGame = false}, testUi());

	fresh->loadMap("d2k:maps/8PLAY1");
	fresh->addPlayer(0, HSLColor{214, 375, 919}, areas[0].to<int>(), 0, false);
	fresh->addPlayer(1, HSLColor{0, 800, 500}, areas[1].to<int>(), 1, false);

	auto freshLoadStarted = std::chrono::steady_clock::now();

	fresh->loadState(saved);

	auto freshLoadMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - freshLoadStarted).count();

	std::cout << "fresh loadState: " << freshLoadMs << " ms\n";

	GameManager::GameStateSnapshot freshSaved;

	fresh->saveState(freshSaved);
	expectSameBytes(saved.data, freshSaved.data);
	EXPECT_EQ(fresh->debugGridChecksum(), gridAtSave);

	this->manager = std::move(fresh);
	this->player = &this->manager->getPlayers()[0];
	for (int i = 0; i < 200; i++) {
		this->tick(1);

		GameManager::GameStateSnapshot frame;

		this->manager->saveState(frame);
		EXPECT_EQ(frame.checksum, checksums[i]) << "tick " << i << " after a fresh load";
		EXPECT_EQ(this->manager->debugGridChecksum(), grids[i]) << "grid tick " << i << " after a fresh load";
		if (frame.checksum != checksums[i])
			break;
	}
}
