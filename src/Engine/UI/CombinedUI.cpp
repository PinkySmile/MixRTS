//
// Created by PinkySmile on 12/11/2026.
//

#include "CombinedUI.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

CombinedUI::CombinedUI(
	const std::vector<Faction> &factions,
	UIState *uiState,
	GameManager *manager,
	PlayerState *ownerState,
	std::vector<MapState> &mapStates,
	size_t
) :
	_manager(manager),
	_currentUi(ownerState->factionIndex)
{
	assert_exp(this->_currentUi < factions.size());
	for (size_t i = 0; i < factions.size(); i++)
		this->_uis.emplace_back(factions[i].createUI(uiState, manager, ownerState, &mapStates[i]));
}

CombinedUI::~CombinedUI()
{
	if (!game)
		return;
	for (auto &ui : this->_uis)
		if (ui)
			game->gui.remove(ui->getPanel());
}

void CombinedUI::_updateUis()
{
	for (auto &ui : this->_uis)
		if (ui)
			ui->update();
}

void CombinedUI::_applyUi(size_t index)
{
	this->_currentUi = index;
	for (size_t i = 0; i < this->_uis.size(); i++)
		if (this->_uis[i])
			this->_uis[i]->setEnabled(i == this->_currentUi && this->_enabled);
}

void CombinedUI::_record(const ReplayAction &action)
{
	uint32_t frame = static_cast<uint32_t>(this->_manager->getTick());

	// Tick 0 is before the first simulation step. File frame 0 is tick 1, so keep the tag in range.
	if (frame == 0)
		frame = 1;

	RecordedAction recorded{frame, action};

	this->_recorded.push_back(recorded);

	std::vector<uint8_t> bytes;
	ReplayData::encodeAction(bytes, recorded);
	this->_journal.append(bytes.data(), bytes.size());
}

void CombinedUI::_record(const UI::Event &event)
{
	this->_record(std::visit([](auto &&arg) -> ReplayAction {
		return arg;
	}, event));
}

void CombinedUI::_captureEnvironment()
{
	auto size = game->screen->getSize();
	Vector2u current{size.x, size.y};

	if (!this->_haveEnvironment) {
		this->_screenSize = current;
		this->_uiScale = guiScale;
		this->_haveEnvironment = true;
		this->_record(ScreenSizeChanged{current});
		this->_record(SettingsChanged{guiScale});
		return;
	}
	if (current != this->_screenSize) {
		this->_screenSize = current;
		this->_record(ScreenSizeChanged{current});
	}
	if (guiScale != this->_uiScale) {
		this->_uiScale = guiScale;
		this->_record(SettingsChanged{guiScale});
	}
}

void CombinedUI::update()
{
	this->_updateUis();
	this->_captureEnvironment();
}

void CombinedUI::updateRender()
{
	for (auto &ui : this->_uis)
		if (ui)
			ui->updateRender();
}

void CombinedUI::render()
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->render();
}

void CombinedUI::renderAbove()
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->renderAbove();
}

void CombinedUI::setEnabled(bool enabled)
{
	this->_enabled = enabled;
	this->_applyUi(this->_currentUi);
}

void CombinedUI::setEnabled(size_t index)
{
	assert_msg(index < this->_uis.size(), std::to_string(index));
	this->_applyUi(index);
	this->_record(UIChanged{static_cast<uint32_t>(index)});
}

void CombinedUI::mouseMoved(Vector2i pos)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->mouseMoved(pos);
}

void CombinedUI::tileClicked(Vector2i pos)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->tileClicked(pos);
}

void CombinedUI::onCancel(Vector2i pos)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->onCancel(pos);
}

void CombinedUI::mouseReleased(Vector2i pos)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->mouseReleased(pos);
}

bool CombinedUI::isMouseOnUi() const
{
	return this->_uis[this->_currentUi] && this->_uis[this->_currentUi]->isMouseOnUi();
}

void CombinedUI::onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->onKeyPressed(key, ctrl, shift, alt, meta);
}

void CombinedUI::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args)
{
	if (this->_uis[this->_currentUi])
		this->_uis[this->_currentUi]->playEVA(sound, expireIn, priority, cooldown, args);
}

void CombinedUI::setResult(bool won, bool lost)
{
	for (auto &ui : this->_uis)
		if (ui) {
			ui->won = won;
			ui->lost = lost;
		}
}

void CombinedUI::onStateRestored()
{
	for (auto &ui : this->_uis)
		if (ui)
			ui->onStateRestored();
}

void CombinedUI::setForcedCursor(std::optional<UI::Cursor> cursor)
{
	for (auto &ui : this->_uis)
		if (ui)
			ui->forcedCursor = cursor;
}

std::vector<std::shared_ptr<UI>> &CombinedUI::getUIs()
{
	return this->_uis;
}

std::optional<UI::Event> CombinedUI::pollEvent()
{
	for (auto &ui : this->_uis) {
		if (!ui)
			continue;
		if (auto event = ui->pollEvent()) {
			this->_record(*event);
			return event;
		}
	}
	return std::nullopt;
}

const std::vector<RecordedAction> &CombinedUI::getRecordedActions() const
{
	return this->_recorded;
}

CrashJournal &CombinedUI::getCrashJournal()
{
	return this->_journal;
}

ReplayCombinedUI::ReplayCombinedUI(
	const std::vector<Faction> &factions,
	UIState *uiState,
	GameManager *manager,
	PlayerState *ownerState,
	std::vector<MapState> &mapStates,
	size_t playerIndex,
	std::vector<RecordedAction> actions
) :
	CombinedUI(factions, uiState, manager, ownerState, mapStates, playerIndex),
	_playback(std::move(actions))
{
}

void ReplayCombinedUI::update()
{
	// The viewer's screen and scale are not part of the recorded game.
	this->_updateUis();
}

static std::optional<UI::Event> asUiEvent(const ReplayAction &action)
{
	return std::visit([](auto &&arg) -> std::optional<UI::Event> {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, ScreenSizeChanged> || std::is_same_v<T, SettingsChanged> || std::is_same_v<T, UIChanged>)
			return std::nullopt;
		else
			return UI::Event{arg};
	}, action);
}

std::optional<UI::Event> ReplayCombinedUI::pollEvent()
{
	for (auto &ui : this->_uis)
		if (ui)
			while (ui->pollEvent());

	uint32_t tick = static_cast<uint32_t>(this->_manager->getTick());

	if (tick == 0)
		tick = 1;
	while (this->_playbackIndex < this->_playback.size() && this->_playback[this->_playbackIndex].frame == tick) {
		ReplayAction action = this->_playback[this->_playbackIndex++].action;

		// Re-record so a replay saved from playback contains the same actions.
		this->_record(action);
		if (auto *changed = std::get_if<UIChanged>(&action)) {
			assert_msg(changed->index < this->_uis.size(), std::to_string(changed->index));
			this->_applyUi(changed->index);
			continue;
		}
		if (std::holds_alternative<ScreenSizeChanged>(action) || std::holds_alternative<SettingsChanged>(action))
			continue;
		return asUiEvent(action);
	}
	return std::nullopt;
}
