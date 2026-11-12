//
// Created by PinkySmile on 12/11/2026.
//

#ifndef MIXRTS_REPLAY_HPP
#define MIXRTS_REPLAY_HPP


#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <filesystem>
#include "UI/UI.hpp"
#include "Utils.hpp"

struct GameStartParams;

//! Screen resolution change recorded in a replay (action id 200).
struct ScreenSizeChanged {
	Vector2u size;
};

//! UI scale change recorded in a replay (action id 201).
struct SettingsChanged {
	float scale = 1;
};

//! The player switched the displayed interface (action id 202).
struct UIChanged {
	uint32_t index = 0;
};

using ReplayAction = std::variant<
	UI::StartBuildEvent,
	UI::PlaceBuildEvent,
	UI::CancelBuildEvent,
	UI::DeployUnitEvent,
	UI::MoveUnitEvent,
	UI::HarvestUnitEvent,
	UI::DumpUnitEvent,
	UI::SetPrimaryEvent,
	UI::RepairEvent,
	UI::SellEvent,
	UI::AttackUnitEvent,
	UI::StopEvent,
	UI::OpenQueueEvent,
	UI::CloseQueueEvent,
	ScreenSizeChanged,
	SettingsChanged,
	UIChanged
>;

//! One recorded action and the simulation tick it happened on (GameManager::getTick()).
struct RecordedAction {
	uint32_t frame = 0;
	ReplayAction action;
};

struct ReplayPlayerInfo {
	std::string name;
	std::string faction;
	uint32_t factionCrc = 0;
	Vector2f spawn;
	HSLColor color;
	uint8_t team = 0;
};

class MYDLL_API ReplayData {
public:
	static constexpr uint32_t MAGIC = 0x7D4E82AF;
	static constexpr uint8_t VERSION = 1;
	static constexpr uint8_t FLAG_SHORT_GAME = 1 << 0;
	static constexpr uint8_t FLAG_RANDOM_SPAWN = 1 << 1;
	static constexpr uint8_t FLAG_INPUTS = 1 << 7;

	uint32_t seed = 0;
	std::string map;
	uint32_t mapCrc = 0;
	float speed = 1;
	uint8_t flags = 0;
	std::vector<ReplayPlayerInfo> players;
	//! Number of simulation ticks stored in the file.
	uint32_t frameCount = 0;
	//! Parallel to players. Actions are sparse and tagged with their simulation tick.
	std::vector<std::vector<RecordedAction>> actions;

	static ReplayData load(const std::filesystem::path &path);
	void save(const std::filesystem::path &path) const;

	//! Builds start parameters. Only player 0 is controlled, which is the view a replay opens on.
	[[nodiscard]] GameStartParams toGameStartParams() const;
	static ReplayData fromGame(
		const GameStartParams &params,
		uint32_t mapCrc,
		const std::vector<uint32_t> &factionCrcs,
		uint32_t frameCount,
		std::vector<std::vector<RecordedAction>> actions
	);
};


#endif //MIXRTS_REPLAY_HPP
