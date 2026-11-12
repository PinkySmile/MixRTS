//
// Created by PinkySmile on 12/11/2026.
//

#ifndef MIXRTS_REPLAY_HPP
#define MIXRTS_REPLAY_HPP


#include <cstdint>
#include <iosfwd>
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
	static ReplayData load(std::istream &stream);
	void save(const std::filesystem::path &path) const;
	void save(std::ostream &stream) const;
	//! Appends one recorded action: the simulation tick as a LEB128, then the action bytes.
	static void encodeAction(std::vector<uint8_t> &out, const RecordedAction &action);
	//! Reads every complete action. A truncated trailing record is dropped.
	static std::vector<RecordedAction> decodeActions(const uint8_t *data, size_t size);
	//! header is a replay saved with no frames. Each buffer is the output of encodeAction for one player.
	static bool saveRecovered(
		const uint8_t *header,
		size_t headerSize,
		uint32_t tick,
		const std::vector<std::vector<uint8_t>> &encodedPlayers,
		const std::filesystem::path &path
	);

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

//! One player's encoded actions, as published for a crashed process to be read from outside.
struct CrashReplayStream {
	const uint8_t *data = nullptr;
	size_t size = 0;
};

//! Flat snapshot of the in-progress replay. magic is published last and cleared first.
struct CrashReplayAnchor {
	static constexpr uint32_t MAGIC = 0xC0A5E001;

	uint32_t magic = 0;
	const uint8_t *header = nullptr;
	size_t headerSize = 0;
	uint32_t tick = 0;
	uint32_t playerCount = 0;
	const CrashReplayStream *streams = nullptr;
	//! GameManager's tick. The signal handler reads it; the game never follows it from outside.
	const uint64_t *liveTick = nullptr;
	//! Tick that was running when the crash was captured. 0 if capture did not run.
	uint32_t crashTick = 0;
	uint32_t crashed = 0;

	//! Copies liveTick into crashTick. Safe to call from a signal handler: it does not allocate.
	void captureCrashTick() noexcept;
	//! The tick playback must reach. A captured crash uses the tick that was in progress.
	[[nodiscard]] uint32_t capturedTick() const { return this->crashed ? this->crashTick : this->tick; }
};

MYDLL_API extern CrashReplayAnchor crashReplayAnchor;

//! Append-only byte buffer. The published stream always refers to a complete prefix.
class MYDLL_API CrashJournal {
public:
	CrashJournal() = default;
	CrashJournal(const CrashJournal &) = delete;
	CrashJournal &operator=(const CrashJournal &) = delete;
	~CrashJournal();

	void append(const uint8_t *bytes, size_t count);
	//! Later appends update this stream in place. The stream must outlive the journal's use of it.
	void bind(CrashReplayStream *stream);
	[[nodiscard]] const uint8_t *data() const;
	[[nodiscard]] size_t size() const;

private:
	uint8_t *_data = nullptr;
	size_t _size = 0;
	size_t _capacity = 0;
	CrashReplayStream *_stream = nullptr;
};


#endif //MIXRTS_REPLAY_HPP
