//
// Created by PinkySmile on 10/11/2026.
//

#ifndef MIXRTS_INGAME_HPP
#define MIXRTS_INGAME_HPP


#include <atomic>
#include <memory>
#include <unordered_map>
#include "Data/Faction.hpp"
#include "Data/Replay.hpp"
#include "Data/SaveState.hpp"
#include "Resources/IScene.hpp"
#include "Resources/SceneArgument.hpp"

class InGame : public IScene {
protected:
	tgui::Panel::Ptr _panel;
	GameStartParams _startParams;
	std::deque<float> _tickTimes;
	std::deque<float> _tickTimestamp;
	std::unordered_map<std::string, uint32_t> _factionCrcs;
	uint32_t _mapCrc = 0;
	//! Set from the constructor. The destructor reads this directly so a subclass that throws still opts out.
	bool _saveReplays = true;
	bool _replaySaved = false;
	bool _crashAnchorPublished = false;
	std::vector<uint8_t> _crashHeader;
	std::vector<CrashReplayStream> _crashStreams;
	//! Two slots so a crash during a capture still sees the previous completed image.
	std::vector<uint8_t> _crashState[2];
	unsigned _crashStateSlot = 0;
	//! An event may queue an order without a tick. The next update captures once, then clears this.
	bool _crashStateDirty = false;

	void _publishCrashAnchor();
	void _retractCrashAnchor();
	void _captureCrashState();
	void _retractCrashState();
	[[nodiscard]] SaveStateFile _makeStateFile() const;
	void _writeState();
	std::vector<Faction> _loadedFactions;
	sf::Clock _clock;
	sf::Clock _clock2;
	float _timer = 0;

	InGame(GameStartParams params, std::function<void (const std::string &)> &cb, const GameManager::UIFactory &factory, bool saveReplays, bool spawnStartingObjects = true);
	InGame(GameStartParams params, std::shared_ptr<const SaveStateFile> state, std::function<void (const std::string &)> &cb);
	virtual bool _recordsReplay() const;

public:
	InGame(GameStartParams &params, std::function<void (const std::string &)> &cb);
	~InGame() override;
	void render() const override;
	void renderAbove() const override;
	void update() override;
	void consumeEvent(const sf::Event &event) override;
	[[nodiscard]] tgui::Panel::Ptr getPanel() const override;
	//! Writes dir/<date-time>.replay. Does nothing when this scene does not record, or after a successful save.
	void saveReplay(const std::filesystem::path &dir);
	//! Writes dir/state.state from the last completed tick, or from the live match if none was captured yet.
	void saveCrashState(const std::filesystem::path &dir);

	static InGame *create(SceneArguments *);
};


#endif //MIXRTS_INGAME_HPP
