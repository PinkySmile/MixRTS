//
// Created by PinkySmile on 12/11/2026.
//

#ifndef MIXRTS_COMBINEDUI_HPP
#define MIXRTS_COMBINEDUI_HPP


#include <functional>
#include <memory>
#include <vector>
#include "Data/Replay.hpp"
#include "Data/Faction.hpp"
#include "Data/Map.hpp"

class MYDLL_API CombinedUI {
public:
	CombinedUI(
		const std::vector<Faction> &factions,
		UIState *uiState,
		GameManager *manager,
		PlayerState *ownerState,
		std::vector<MapState> &mapStates,
		size_t playerIndex
	);
	virtual ~CombinedUI();

	virtual void update();
	void updateRender();
	void render();
	void renderAbove();
	void setEnabled(bool enabled);
	//! Switches the displayed interface and records a UIChanged action.
	virtual void setEnabled(size_t index);
	void mouseMoved(Vector2i pos);
	void tileClicked(Vector2i pos);
	void onCancel(Vector2i pos);
	void mouseReleased(Vector2i pos);
	[[nodiscard]] bool isMouseOnUi() const;
	void onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta);
	void playEVA(UI::EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	void setResult(bool won, bool lost);
	void setForcedCursor(std::optional<UI::Cursor> cursor);
	std::vector<std::shared_ptr<UI>> &getUIs();

	//! Drains every owned UI. Each returned event is recorded with the current tick.
	virtual std::optional<UI::Event> pollEvent();
	[[nodiscard]] const std::vector<RecordedAction> &getRecordedActions() const;

protected:
	void _updateUis();
	void _applyUi(size_t index);
	void _record(const ReplayAction &action);
	void _record(const UI::Event &event);
	void _captureEnvironment();

	GameManager *_manager;
	std::vector<std::shared_ptr<UI>> _uis;
	size_t _currentUi = 0;
	bool _enabled = false;
	std::vector<RecordedAction> _recorded;

private:
	bool _haveEnvironment = false;
	Vector2u _screenSize;
	float _uiScale = 1;
};

//! Plays a preloaded action list back. Events produced by the real UIs are discarded.
class MYDLL_API ReplayCombinedUI : public CombinedUI {
public:
	ReplayCombinedUI(
		const std::vector<Faction> &factions,
		UIState *uiState,
		GameManager *manager,
		PlayerState *ownerState,
		std::vector<MapState> &mapStates,
		size_t playerIndex,
		std::vector<RecordedAction> actions
	);

	void update() override;
	std::optional<UI::Event> pollEvent() override;

private:
	std::vector<RecordedAction> _playback;
	size_t _playbackIndex = 0;
};


#endif //MIXRTS_COMBINEDUI_HPP
