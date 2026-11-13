//
// Created by PinkySmile on 01/04/2026.
//

#include <sol/sol.hpp>
#include "UI.hpp"
#include "Data/GameManager.hpp"
#include "Data/Faction.hpp"
#include "Data/Cursor.hpp"
#include "Data/CursorAnimation.hpp"
#include "Data/Animation.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Effect.hpp"
#include "Data/Weapon.hpp"
#include "Data/Warhead.hpp"
#include "Data/GameObject.hpp"
#include "Data/Player.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"


static bool isMouseOverUI(const tgui::Container::Ptr &base = nullptr)
{
	Vector2i pos = game->screen->getMousePosition();

	pos.x /= guiScale;
	pos.y /= guiScale;
	for (const auto &widget : base ? base->getWidgets() : game->gui.getWidgets()) {
		if (!widget->isVisible() || widget->getIgnoreMouseEvents())
			continue;
		// Floating windows (debug panel, ...) count fully
		if (widget->getWidgetType() == "Panel") {
			if (isMouseOverUI(widget->cast<tgui::Container>()))
				return true;
		} else if (widget->isMouseOnWidget(pos))
			return true;
	}
	return false;
}

std::string UI::EVAToString(EVASound snd)
{
	switch (snd) {
	case EVA_BUILDING:
		return "EVA_BUILDING";
	case EVA_BUILDING_DONE:
		return "EVA_BUILDING_DONE";
	case EVA_CANCEL:
		return "EVA_CANCEL";
	case EVA_CANNOT_DEPLOY:
		return "EVA_CANNOT_DEPLOY";
	case EVA_CANNOT_BUILD:
		return "EVA_CANNOT_BUILD";
	case EVA_GUARDING:
		return "EVA_GUARDING";
	case EVA_HOLD_QUEUE:
		return "EVA_HOLD_QUEUE";
	case EVA_LOST:
		return "EVA_LOST";
	case EVA_WON:
		return "EVA_WON";
	case EVA_LOW_POWER:
		return "EVA_LOW_POWER";
	case EVA_UNLOCKED:
		return "EVA_UNLOCKED";
	case EVA_NO_CREDITS:
		return "EVA_NO_CREDITS";
	case EVA_PRIMARY_SELECTED:
		return "EVA_PRIMARY_SELECTED";
	case EVA_RETREAT:
		return "EVA_RETREAT";
	case EVA_SELL:
		return "EVA_SELL";
	case EVA_TRAINING:
		return "EVA_TRAINING";
	case EVA_UNIT_READY:
		return "EVA_UNIT_READY";
	case EVA_UPGRADING:
		return "EVA_UPGRADING";
	case EVA_CASH_UP:
		return "EVA_CASH_UP";
	case EVA_CASH_DOWN:
		return "EVA_CASH_DOWN";
	case EVA_REQUIRE_STORAGE:
		return "EVA_REQUIRE_STORAGE";
	case EVA_ALREADY_BUILDING:
		return "EVA_ALREADY_BUILDING";
	case EVA_POWER_OFF:
		return "EVA_POWER_OFF";
	case EVA_POWER_ON:
		return "EVA_POWER_ON";
	case EVA_RADAR_UP:
		return "EVA_RADAR_UP";
	case EVA_MY_BUILDING_CAPTURED:
		return "EVA_MY_BUILDING_CAPTURED";
	case EVA_BUILDING_CAPTURED:
		return "EVA_BUILDING_CAPTURED";
	case EVA_BUILDING_ATTACKED:
		return "EVA_BUILDING_ATTACKED";
	case EVA_BUILDING_LOST:
		return "EVA_BUILDING_LOST";
	case EVA_MY_UNIT_CAPTURED:
		return "EVA_MY_UNIT_CAPTURED";
	case EVA_UNIT_CAPTURED:
		return "EVA_UNIT_CAPTURED";
	case EVA_UNIT_ATTACKED:
		return "EVA_UNIT_ATTACKED";
	case EVA_ALLIED_MY_BUILDING_CAPTURED:
		return "EVA_ALLIED_MY_BUILDING_CAPTURED";
	case EVA_ALLIED_BUILDING_CAPTURED:
		return "EVA_ALLIED_BUILDING_CAPTURED";
	case EVA_ALLIED_BUILDING_ATTACKED:
		return "EVA_ALLIED_BUILDING_ATTACKED";
	case EVA_ALLIED_BUILDING_LOST:
		return "EVA_ALLIED_BUILDING_LOST";
	case EVA_ALLIED_MY_UNIT_CAPTURED:
		return "EVA_ALLIED_MY_UNIT_CAPTURED";
	case EVA_ALLIED_UNIT_CAPTURED:
		return "EVA_ALLIED_UNIT_CAPTURED";
	case EVA_ALLIED_UNIT_ATTACKED:
		return "EVA_ALLIED_UNIT_ATTACKED";
	case EVA_UNIT_LOST:
		return "EVA_UNIT_LOST";
	case EVA_HARV_UNDER_ATTACK:
		return "EVA_HARV_UNDER_ATTACK";
	case EVA_HAZARD_SPAWNED:
		return "EVA_HAZARD_SPAWNED";
	case EVA_HAZARD_ATTACKED:
		return "EVA_HAZARD_ATTACKED";
	case EVA_NUKE_PREP:
		return "EVA_NUKE_PREP";
	case EVA_NUKE_LAUNCH:
		return "EVA_NUKE_LAUNCH";
	case EVA_NUKE_READY:
		return "EVA_NUKE_READY";
	default:
		return "Invalid EVA " + std::to_string(snd);
	}
}

std::string UI::cursorToString(Cursor cur)
{
	switch (cur) {
	case CURSOR_NORMAL:
		return "CURSOR_NORMAL";
	case CURSOR_MOVE:
		return "CURSOR_MOVE";
	case CURSOR_MOVE_RESTRICT:
		return "CURSOR_MOVE_RESTRICT";
	case CURSOR_NO_MOVE:
		return "CURSOR_NO_MOVE";
	case CURSOR_SELECT:
		return "CURSOR_SELECT";
	case CURSOR_HARVEST:
		return "CURSOR_HARVEST";
	case CURSOR_ATTACK:
		return "CURSOR_ATTACK";
	case CURSOR_ATTACK_MOVE:
		return "CURSOR_ATTACK_MOVE";
	case CURSOR_ENTER:
		return "CURSOR_ENTER";
	case CURSOR_DUMP:
		return "CURSOR_DUMP";
	case CURSOR_SCROLL_ALL_DIRS:
		return "CURSOR_SCROLL_ALL_DIRS";
	case CURSOR_SCROLL_RIGHT:
		return "CURSOR_SCROLL_RIGHT";
	case CURSOR_SCROLL_LEFT:
		return "CURSOR_SCROLL_LEFT";
	case CURSOR_SCROLL_TOP:
		return "CURSOR_SCROLL_TOP";
	case CURSOR_SCROLL_TOP_RIGHT:
		return "CURSOR_SCROLL_TOP_RIGHT";
	case CURSOR_SCROLL_TOP_LEFT:
		return "CURSOR_SCROLL_TOP_LEFT";
	case CURSOR_SCROLL_BOTTOM:
		return "CURSOR_SCROLL_BOTTOM";
	case CURSOR_SCROLL_BOTTOM_RIGHT:
		return "CURSOR_SCROLL_BOTTOM_RIGHT";
	case CURSOR_SCROLL_BOTTOM_LEFT:
		return "CURSOR_SCROLL_BOTTOM_LEFT";
	case CURSOR_NO_SCROLL_RIGHT:
		return "CURSOR_NO_SCROLL_RIGHT";
	case CURSOR_NO_SCROLL_LEFT:
		return "CURSOR_NO_SCROLL_LEFT";
	case CURSOR_NO_SCROLL_TOP:
		return "CURSOR_NO_SCROLL_TOP";
	case CURSOR_NO_SCROLL_TOP_RIGHT:
		return "CURSOR_NO_SCROLL_TOP_RIGHT";
	case CURSOR_NO_SCROLL_TOP_LEFT:
		return "CURSOR_NO_SCROLL_TOP_LEFT";
	case CURSOR_NO_SCROLL_BOTTOM:
		return "CURSOR_NO_SCROLL_BOTTOM";
	case CURSOR_NO_SCROLL_BOTTOM_RIGHT:
		return "CURSOR_NO_SCROLL_BOTTOM_RIGHT";
	case CURSOR_NO_SCROLL_BOTTOM_LEFT:
		return "CURSOR_NO_SCROLL_BOTTOM_LEFT";
	case CURSOR_DEPLOY:
		return "CURSOR_DEPLOY";
	case CURSOR_NO_DEPLOY:
		return "CURSOR_NO_DEPLOY";
	case CURSOR_PRIMARY:
		return "CURSOR_PRIMARY";
	case CURSOR_NO_REPAIR:
		return "CURSOR_NO_REPAIR";
	case CURSOR_REPAIR:
		return "CURSOR_REPAIR";
	case CURSOR_NO_SELL:
		return "CURSOR_NO_SELL";
	case CURSOR_SELL:
		return "CURSOR_SELL";
	case CURSOR_NUKE:
		return "CURSOR_NUKE";
	case CURSOR_AIR_DROP:
		return "CURSOR_AIR_DROP";
	case CURSOR_EXPLODE:
		return "CURSOR_EXPLODE";
	case CURSOR_COUNT:
		return "CURSOR_COUNT";
	default:
		return "Invalid cursor " + std::to_string(cur);
	}
}

UI::Cursor UI::stringToCursor(const std::string &str)
{
	if (str == "normal")
		return CURSOR_NORMAL;
	if (str == "start_select")
		return CURSOR_START_SELECT;
	if (str == "move")
		return CURSOR_MOVE;
	if (str == "move_restrict")
		return CURSOR_MOVE_RESTRICT;
	if (str == "no_move")
		return CURSOR_NO_MOVE;
	if (str == "select")
		return CURSOR_SELECT;
	if (str == "harvest")
		return CURSOR_HARVEST;
	if (str == "attack")
		return CURSOR_ATTACK;
	if (str == "attack_move")
		return CURSOR_ATTACK_MOVE;
	if (str == "enter")
		return CURSOR_ENTER;
	if (str == "dump")
		return CURSOR_DUMP;
	if (str == "scroll_all_dirs")
		return CURSOR_SCROLL_ALL_DIRS;
	if (str == "scroll_right")
		return CURSOR_SCROLL_RIGHT;
	if (str == "scroll_left")
		return CURSOR_SCROLL_LEFT;
	if (str == "scroll_top")
		return CURSOR_SCROLL_TOP;
	if (str == "scroll_top_right")
		return CURSOR_SCROLL_TOP_RIGHT;
	if (str == "scroll_top_left")
		return CURSOR_SCROLL_TOP_LEFT;
	if (str == "scroll_bottom")
		return CURSOR_SCROLL_BOTTOM;
	if (str == "scroll_bottom_right")
		return CURSOR_SCROLL_BOTTOM_RIGHT;
	if (str == "scroll_bottom_left")
		return CURSOR_SCROLL_BOTTOM_LEFT;
	if (str == "no_scroll_right")
		return CURSOR_NO_SCROLL_RIGHT;
	if (str == "no_scroll_left")
		return CURSOR_NO_SCROLL_LEFT;
	if (str == "no_scroll_top")
		return CURSOR_NO_SCROLL_TOP;
	if (str == "no_scroll_top_right")
		return CURSOR_NO_SCROLL_TOP_RIGHT;
	if (str == "no_scroll_top_left")
		return CURSOR_NO_SCROLL_TOP_LEFT;
	if (str == "no_scroll_bottom")
		return CURSOR_NO_SCROLL_BOTTOM;
	if (str == "no_scroll_bottom_right")
		return CURSOR_NO_SCROLL_BOTTOM_RIGHT;
	if (str == "no_scroll_bottom_left")
		return CURSOR_NO_SCROLL_BOTTOM_LEFT;
	if (str == "deploy")
		return CURSOR_DEPLOY;
	if (str == "no_deploy")
		return CURSOR_NO_DEPLOY;
	if (str == "primary")
		return CURSOR_PRIMARY;
	if (str == "no_repair")
		return CURSOR_NO_REPAIR;
	if (str == "repair")
		return CURSOR_REPAIR;
	if (str == "no_sell")
		return CURSOR_NO_SELL;
	if (str == "sell")
		return CURSOR_SELL;
	if (str == "nuke")
		return CURSOR_NUKE;
	if (str == "air_drop")
		return CURSOR_AIR_DROP;
	if (str == "explode")
		return CURSOR_EXPLODE;
	assert_not_reached_msg("Invalid cursor \"" + str + "\"");
}

void UI::pushUserType(sol::state &state)
{
	sol::usertype<UI> type = state.new_usertype<UI>("UI");

	type["state"] = &UI::state;
	type["manager"] = &UI::manager;
	type["owner_state"] = &UI::ownerState;
	type["map"] = &UI::map;
	type["forced_cursor"] = &UI::forcedCursor;
	type["panel"] = sol::property(&UI::getPanel);
	type["faction"] = sol::property(&UI::getFaction);
	type["select_building_for_placement"] = &UI::selectBuildingForPlacement;
	type["enabled"] = sol::property(&UI::isEnabled, [](UI &ui, bool b) { ui.UI::setEnabled(b); });
	type["play_eva"] = [](UI &ui, EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args) {
		ui.UI::playEVA(sound, expireIn, priority, cooldown, args);
	};
	type["poll_event"] = [](UI &ui) { ui.UI::pollEvent(); };
	type["render_above"] = [](UI &ui) { ui.UI::renderAbove(); };
	type["update_render"] = [](UI &ui) { ui.UI::updateRender(); };
	type["mouse_on_ui"] = sol::property([](UI &ui) { return ui.UI::isMouseOnUi(); });
	type["on_cancel"] = [](UI &ui, Vector2i absPos) { ui.UI::onCancel(absPos); };
	type["tile_clicked"] = [](UI &ui, Vector2i absPos) { ui.UI::tileClicked(absPos); };
	type["mouse_moved"] = [](UI &ui, Vector2i absPos) { ui.UI::mouseMoved(absPos); };
	type["on_key_pressed"] = [](UI &ui, sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta) {
		ui.UI::onKeyPressed(key, ctrl, shift, alt, meta);
	};
	type["update"] = [](UI &ui) { ui.UI::update(); };
	type["render"] = [](UI &ui) { ui.UI::render(); };

	type["_current_cursor"] = sol::property(&UI::_getCurrentCursor);
	type["_queue_start_build"] = &UI::_queueStartBuild;
	type["_queue_cancel_build"] = &UI::_queueCancelBuild;
	type["_queue_open_queue"] = &UI::_queueOpenQueue;
	type["_queue_close_queue"] = &UI::_queueCloseQueue;
	type["_load_eva_sound"] = &UI::_loadEvaSound;
	type["_load_select_corner"] = &UI::_loadSelectCorner;
	type["_set_cursor_mapping"] = &UI::_setCursorMapping;
	type["_tile_clicked_normal"] = &UI::_tileClickedNormal;
	type["_tile_clicked_repair"] = &UI::_tileClickedRepair;
	type["_tile_clicked_sell"] = &UI::_tileClickedSell;
	type["_update_cursor"] = &UI::_updateCursor;
	type["_update_cursor_normal"] = &UI::_updateCursorNormal;
	type["_update_cursor_repair"] = &UI::_updateCursorRepair;
	type["_update_cursor_sell"] = &UI::_updateCursorSell;
	type["_handle_try_place"] = &UI::_handleTryPlace;
	type["_display_placement_preview"] = &UI::_displayPlacementPreview;
	type["_selection_too_far"] = sol::property(&UI::_isSelectionTooFar);
	type["_display_select_box"] = &UI::_displaySelectBox;
	type["_select_corners"] = &UI::_selectCorners;
	type["_placement_animation"] = &UI::_placementAnimation;
	type["_placement_pos"] = &UI::_placementPos;
	type["_placement_building"] = &UI::_placementBuilding;
	type["_global_eva_cooldown"] = &UI::_globalEvaCooldown;
	type["_eva_queue"] = &UI::_evaQueue;
	type["_eva_cooldown"] = &UI::_evaCooldown;
	type["_eva_sounds"] = &UI::_evaSounds;
	type["_object_buffer"] = &UI::_objectBuffer;
	type["_current_eva_sound"] = &UI::_currentEvaSound;
	type["_event_queue"] = &UI::_eventQueue;
	type["_settings"] = &UI::_settings;
	type["_panel"] = &UI::_panel;
	type["_cursor_mapping"] = &UI::_cursorMapping;
	type["_cursors"] = &UI::_cursors;
	type["_cursor"] = &UI::_cursor;
	type["_minimap_pos"] = &UI::_minimapPos;
	type["_minimap_size"] = &UI::_minimapSize;
	type["_enabled"] = &UI::_enabled;
	type["_has_low_power"] = &UI::_hasLowPower;
	type["_has_radar"] = &UI::_hasRadar;
	type["_repairing"] = &UI::_repairing;
	type["_selling"] = &UI::_selling;

	state.new_enum<EVASound>("UI_EVASound", {
		{ "EVA_BUILDING",                    EVA_BUILDING },
		{ "EVA_BUILDING_DONE",               EVA_BUILDING_DONE },
		{ "EVA_CANCEL",                      EVA_CANCEL },
		{ "EVA_CANNOT_DEPLOY",               EVA_CANNOT_DEPLOY },
		{ "EVA_CANNOT_BUILD",                EVA_CANNOT_BUILD },
		{ "EVA_GUARDING",                    EVA_GUARDING },
		{ "EVA_HOLD_QUEUE",                  EVA_HOLD_QUEUE },
		{ "EVA_LOST",                        EVA_LOST },
		{ "EVA_WON",                         EVA_WON },
		{ "EVA_LOW_POWER",                   EVA_LOW_POWER },
		{ "EVA_UNLOCKED",                    EVA_UNLOCKED },
		{ "EVA_NO_CREDITS",                  EVA_NO_CREDITS },
		{ "EVA_PRIMARY_SELECTED",            EVA_PRIMARY_SELECTED },
		{ "EVA_RETREAT",                     EVA_RETREAT },
		{ "EVA_SELL",                        EVA_SELL },
		{ "EVA_TRAINING",                    EVA_TRAINING },
		{ "EVA_UNIT_READY",                  EVA_UNIT_READY },
		{ "EVA_UPGRADING",                   EVA_UPGRADING },
		{ "EVA_CASH_UP",                     EVA_CASH_UP },
		{ "EVA_CASH_DOWN",                   EVA_CASH_DOWN },
		{ "EVA_REQUIRE_STORAGE",             EVA_REQUIRE_STORAGE },
		{ "EVA_ALREADY_BUILDING",            EVA_ALREADY_BUILDING },
		{ "EVA_POWER_OFF",                   EVA_POWER_OFF },
		{ "EVA_POWER_ON",                    EVA_POWER_ON },
		{ "EVA_RADAR_UP",                    EVA_RADAR_UP },
		{ "EVA_MY_BUILDING_CAPTURED",        EVA_MY_BUILDING_CAPTURED },
		{ "EVA_BUILDING_CAPTURED",           EVA_BUILDING_CAPTURED },
		{ "EVA_BUILDING_ATTACKED",           EVA_BUILDING_ATTACKED },
		{ "EVA_BUILDING_LOST",               EVA_BUILDING_LOST },
		{ "EVA_MY_UNIT_CAPTURED",            EVA_MY_UNIT_CAPTURED },
		{ "EVA_UNIT_CAPTURED",               EVA_UNIT_CAPTURED },
		{ "EVA_UNIT_ATTACKED",               EVA_UNIT_ATTACKED },
		{ "EVA_ALLIED_MY_BUILDING_CAPTURED", EVA_ALLIED_MY_BUILDING_CAPTURED },
		{ "EVA_ALLIED_BUILDING_CAPTURED",    EVA_ALLIED_BUILDING_CAPTURED },
		{ "EVA_ALLIED_BUILDING_ATTACKED",    EVA_ALLIED_BUILDING_ATTACKED },
		{ "EVA_ALLIED_BUILDING_LOST",        EVA_ALLIED_BUILDING_LOST },
		{ "EVA_ALLIED_MY_UNIT_CAPTURED",     EVA_ALLIED_MY_UNIT_CAPTURED },
		{ "EVA_ALLIED_UNIT_CAPTURED",        EVA_ALLIED_UNIT_CAPTURED },
		{ "EVA_ALLIED_UNIT_ATTACKED",        EVA_ALLIED_UNIT_ATTACKED },
		{ "EVA_UNIT_LOST",                   EVA_UNIT_LOST },
		{ "EVA_HARV_UNDER_ATTACK",           EVA_HARV_UNDER_ATTACK },
		{ "EVA_HAZARD_SPAWNED",              EVA_HAZARD_SPAWNED },
		{ "EVA_HAZARD_ATTACKED",             EVA_HAZARD_ATTACKED },
		{ "EVA_ORDER_PLACED",                EVA_ORDER_PLACED },
		{ "EVA_ORDER_IN_1",                  EVA_ORDER_IN_1 },
		{ "EVA_ORDER_IN_2",                  EVA_ORDER_IN_2 },
		{ "EVA_ORDER_IN_3",                  EVA_ORDER_IN_3 },
		{ "EVA_ORDER_IN_4",                  EVA_ORDER_IN_4 },
		{ "EVA_ORDER_IN_5",                  EVA_ORDER_IN_5 },
	});
	state.new_enum<Cursor>("UI_Cursor", {
		{ "CURSOR_HORZ_NONE",              CURSOR_HORZ_NONE },
		{ "CURSOR_HORZ_RIGHT",             CURSOR_HORZ_RIGHT },
		{ "CURSOR_HORZ_LEFT",              CURSOR_HORZ_LEFT },
		{ "CURSOR_NORMAL",                 CURSOR_NORMAL },
		{ "CURSOR_START_SELECT",           CURSOR_START_SELECT },
		{ "CURSOR_MOVE",                   CURSOR_MOVE },
		{ "CURSOR_MOVE_RESTRICT",          CURSOR_MOVE_RESTRICT },
		{ "CURSOR_NO_MOVE",                CURSOR_NO_MOVE },
		{ "CURSOR_SELECT",                 CURSOR_SELECT },
		{ "CURSOR_HARVEST",                CURSOR_HARVEST },
		{ "CURSOR_ATTACK",                 CURSOR_ATTACK },
		{ "CURSOR_ATTACK_MOVE",            CURSOR_ATTACK_MOVE },
		{ "CURSOR_ENTER",                  CURSOR_ENTER },
		{ "CURSOR_DUMP",                   CURSOR_DUMP },
		{ "CURSOR_SCROLL_ALL_DIRS",        CURSOR_SCROLL_ALL_DIRS },
		{ "CURSOR_SCROLL_RIGHT",           CURSOR_SCROLL_RIGHT },
		{ "CURSOR_SCROLL_LEFT",            CURSOR_SCROLL_LEFT },
		{ "CURSOR_SCROLL_TOP",             CURSOR_SCROLL_TOP },
		{ "CURSOR_SCROLL_TOP_RIGHT",       CURSOR_SCROLL_TOP_RIGHT },
		{ "CURSOR_SCROLL_TOP_LEFT",        CURSOR_SCROLL_TOP_LEFT },
		{ "CURSOR_SCROLL_BOTTOM",          CURSOR_SCROLL_BOTTOM },
		{ "CURSOR_SCROLL_BOTTOM_RIGHT",    CURSOR_SCROLL_BOTTOM_RIGHT },
		{ "CURSOR_SCROLL_BOTTOM_LEFT",     CURSOR_SCROLL_BOTTOM_LEFT },
		{ "CURSOR_NO_SCROLL_RIGHT",        CURSOR_NO_SCROLL_RIGHT },
		{ "CURSOR_NO_SCROLL_LEFT",         CURSOR_NO_SCROLL_LEFT },
		{ "CURSOR_NO_SCROLL_TOP",          CURSOR_NO_SCROLL_TOP },
		{ "CURSOR_NO_SCROLL_TOP_RIGHT",    CURSOR_NO_SCROLL_TOP_RIGHT },
		{ "CURSOR_NO_SCROLL_TOP_LEFT",     CURSOR_NO_SCROLL_TOP_LEFT },
		{ "CURSOR_NO_SCROLL_BOTTOM",       CURSOR_NO_SCROLL_BOTTOM },
		{ "CURSOR_NO_SCROLL_BOTTOM_RIGHT", CURSOR_NO_SCROLL_BOTTOM_RIGHT },
		{ "CURSOR_NO_SCROLL_BOTTOM_LEFT",  CURSOR_NO_SCROLL_BOTTOM_LEFT },
		{ "CURSOR_DEPLOY",                 CURSOR_DEPLOY },
		{ "CURSOR_NO_DEPLOY",              CURSOR_NO_DEPLOY },
		{ "CURSOR_PRIMARY",                CURSOR_PRIMARY },
		{ "CURSOR_NO_REPAIR",              CURSOR_NO_REPAIR },
		{ "CURSOR_REPAIR",                 CURSOR_REPAIR },
		{ "CURSOR_NO_SELL",                CURSOR_NO_SELL },
		{ "CURSOR_SELL",                   CURSOR_SELL },
		{ "CURSOR_COUNT",                  CURSOR_COUNT },
		{ "CURSOR_SCROLL_DIFF",            CURSOR_SCROLL_DIFF }
	});
}

UI::UI(const Faction &faction, UIState *state, GameManager *manager, PlayerState *ownerState, MapState *map) :
	state(state),
	manager(manager),
	ownerState(ownerState),
	map(map),
	_faction(faction)
{
	this->_hasRadar = faction.hasRadar();
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
}

UI::~UI() = default;

void UI::onStateRestored()
{
	this->_objectBuffer.clear();
}

void UI::setEnabled(bool enabled)
{
	this->_panel->setVisible(enabled);
	this->_enabled = enabled;
	if (enabled)
		this->onUi = isMouseOverUI();
}

bool UI::isEnabled() const
{
	return this->_enabled;
}

tgui::Panel::Ptr UI::getPanel() const
{
	return this->_panel;
}

const Faction &UI::getFaction() const
{
	return this->_faction;
}

std::optional<UI::Event> UI::pollEvent()
{
	if (this->_eventQueue.empty())
		return {};

	std::optional result = this->_eventQueue.front();

	this->_eventQueue.pop_front();
	return result;
}

void UI::playEVA(EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &)
{
	if (!this->_enabled)
		return;
	if (this->_evaCooldown[sound])
		return;

	auto it = this->_evaQueue.begin();

	while (it != this->_evaQueue.end() && it->priority >= priority)
		it++;
	this->_evaQueue.insert(it, {sound, expireIn, priority, cooldown});
}

void UI::onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta)
{
	// TODO: Hardcoded key
	if (key == sf::Keyboard::Key::S && !ctrl && !shift && !alt && !meta) {
		if (!this->state->selectedObjects.empty()) {
			StopEvent event;

			for (auto &s : this->state->selectedObjects)
				event.indices.push_back(s->index);
			this->_eventQueue.emplace_front(event);
		}
	}
}

void UI::update()
{
	auto it = this->state->selectedObjects.begin();

	while (it != this->state->selectedObjects.end())
		if ((*it)->isDisposable() || (*it)->isDead())
			it = this->state->selectedObjects.erase(it);
		else
			++it;

	auto &placement = this->_faction.getPlacementCursors();
	auto &anim = this->_placementAnimation;

	if (!placement[0].empty()) {
		anim[0] = (anim[0] + 1) % placement[0].size();
		anim[1] = (anim[1] + 1) % placement[1].size();
		anim[2] = (anim[2] + 1) % placement[2].size();
	}
	if (this->ownerState->hasRadar != this->_hasRadar) {
		this->_hasRadar = this->ownerState->hasRadar;
		if (this->_hasRadar)
			this->playEVA(EVA_RADAR_UP, 300, 1, 0);
	}

	bool lowPower = this->ownerState->consumedPower > this->ownerState->producedPower;

	if (lowPower != this->_hasLowPower) {
		this->_hasLowPower = lowPower;
		this->playEVA(lowPower ? EVA_POWER_OFF : EVA_POWER_ON, 300, 1, 0);
	}
	for (auto &i : this->_evaCooldown)
		if (i)
			i--;
	if (!this->_enabled)
		return this->_evaQueue.clear();
	if (this->_evaQueue.empty()) {
		if (this->_globalEvaCooldown != 0)
			this->_globalEvaCooldown--;
		return;
	}
	if (!this->_currentEvaSound || !this->_currentEvaSound->isPlaying()) {
		if (this->_globalEvaCooldown == 0) {
			auto &e = this->_evaQueue.front();

			game->logger.verbose("Playing EVA sound " + std::to_string(e.soundId));
			assert_msg(this->_evaSounds[e.soundId].isLoaded(), "Trying to play invalid EVA " + EVAToString(e.soundId));
			this->_currentEvaSound = &this->_evaSounds[e.soundId];
			this->_currentEvaSound->play();
			this->_evaCooldown[e.soundId] = e.cooldown;
			this->_evaQueue.pop_front();
			this->_globalEvaCooldown = 30;
		} else
			this->_globalEvaCooldown--;
	}

	for (auto &q : this->_evaQueue)
		if (q.expireTimer != 0)
			q.expireTimer--;
	while (!this->_evaQueue.empty() && (
		this->_evaQueue.front().expireTimer == 0 ||
		this->_evaCooldown[this->_evaQueue.front().soundId]
	)) {
		auto &f = this->_evaQueue.front();

		game->logger.verbose("Expiring EVA sound " +
			std::to_string(f.soundId) + " " +
			std::to_string(this->_currentEvaSound && this->_currentEvaSound->isPlaying()) + " " +
			std::to_string(this->_globalEvaCooldown)
		);
		this->_evaQueue.pop_front();
	}
}

void UI::render()
{
	if (!this->_enabled)
		return;
	if (this->_placementBuilding != nullptr)
		this->_displayPlacementPreview();
	for (auto &b : this->state->selectedObjects)
		this->_displaySelectedObjectUI(*b);
}

void UI::_queueStartBuild(const std::string &id, const std::string &queue)
{
	this->_eventQueue.emplace_back(StartBuildEvent{id, queue});
}

void UI::_queueCancelBuild(const std::string &id, const std::string &queue)
{
	this->_eventQueue.emplace_back(CancelBuildEvent{id, queue});
}

void UI::_queueOpenQueue(const std::string &queue)
{
	this->_eventQueue.emplace_back(OpenQueueEvent{queue});
}

void UI::_queueCloseQueue(const std::string &queue)
{
	this->_eventQueue.emplace_back(CloseQueueEvent{queue});
}

void UI::_loadEvaSound(EVASound sound, const std::string &path, bool relativeToCamera)
{
	this->_evaSounds[sound].load(path, relativeToCamera);
}

void UI::_loadSelectCorner(unsigned index, const std::string &path)
{
	assert_exp(index < this->_selectCorners.size());
	assert_exp(this->_selectCorners[index].loadFromFile(path));
}

void UI::_setCursorMapping(Cursor cursor, const std::string &name)
{
	this->_cursorMapping[cursor] = name;
}

CursorAnimation &UI::_getCurrentCursor()
{
	Cursor cursor = !game->paused && this->forcedCursor ? *this->forcedCursor : this->_cursor;
	assert_exp(cursor < CURSOR_COUNT);
	const std::string &cursorName = this->_cursorMapping[cursor];
	auto it = this->_cursors.find(cursorName);

	assert_msg(it != this->_cursors.end(), cursorName);
	return it->second;
}

void UI::_openEndGameDialog(const std::filesystem::path &path)
{
	auto panel = tgui::Panel::create({"100%", "100%"});

	panel->loadWidgetsFromFile(path.string());
	panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

	auto cont = panel->get<tgui::Button>("Continue");
	auto title = panel->get<tgui::Button>("Title");

	cont->onMousePress([this] {
		this->_soundButton.play();
	});
	title->onMousePress([this] {
		this->_soundButton.play();
	});
	cont->onClick([this, panel = std::weak_ptr(panel)] {
		this->_panel->remove(panel.lock());
	});
	title->onClick([this, panel = std::weak_ptr(panel)] {
		this->_panel->remove(panel.lock());
		this->manager->finished = true;
	});
	this->_panel->add(panel);
}

void UI::renderAbove()
{
	this->_getCurrentCursor().render();

	if (this->ownerState->hasRadar) {
		sf::View oldView = game->screen->getView();
		tgui::FloatRect newView = game->gui.getView().getRect();

		game->screen->setView(sf::View{
			sf::FloatRect{
				sf::Vector2f{newView.left, newView.top},
				sf::Vector2f{newView.width, newView.height}
			}
		});
		this->manager->renderMinimap(this->_minimapPos, this->_minimapSize);
		game->screen->setView(oldView);
	}
}

void UI::updateRender()
{
	this->_updateCursor();
	if (this->_settings)
		this->_settings->setVisible(game->paused);
	for (auto &c : this->_cursors)
		c.second.update();
}

void UI::_updateCursorNormal()
{
	if (this->_dragging) {
		this->_cursor = CURSOR_START_SELECT;
		return;
	}

	Vector2i pos = this->map->absToTileCoord(this->_placementPos);

	if (!this->_objectBuffer.empty()) {
		// TODO: Handle non snap-to-grid units/buildings
		if (this->state->selectedObjects.size() == 1) {
			auto obj = *this->state->selectedObjects.begin();
			auto ownObj = this->ownerState->index == obj->owner->state.index;

			if (ownObj) {
				if (std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()) {
					if (obj->isDeployable()) {
						if (obj->canDeploy())
							this->_cursor = obj->getDeployCursor().value_or(CURSOR_DEPLOY);
						else
							this->_cursor = CURSOR_NO_DEPLOY;
						return;
					} else if (auto &gq = obj->getGivenQueue()) {
						if (this->ownerState->queueBuildings[*gq].size() > 1 && this->ownerState->primaryBuildings[*gq] != obj) {
							this->_cursor = CURSOR_DEPLOY;
							return;
						}
					}
				}
			}
		}

		if (!this->state->selectedObjects.empty()) {
			auto obj = *this->state->selectedObjects.begin();
			auto &other = this->_objectBuffer.front();
			auto ownObj = this->ownerState->index == obj->owner->state.index;
			auto otherAllied = this->ownerState->team == other->owner->state.team;

			if (ownObj && !otherAllied) {
				const ObjectWeapon *w = obj->getBestWeaponFor(other, UINT32_MAX, false);

				if (w) {
					auto cursor = w->weapon->getCursor();

					this->_cursor = cursor ? *cursor : CURSOR_ATTACK;
					return;
				}
			}
		}

		bool canDeposit = std::ranges::any_of(this->_objectBuffer, [this](const GameObject *o){
			// We can provide income to our teammate by sending harvesters to their resource dump.
			return o->owner->state.team == this->ownerState->team && !o->getAcceptResources().empty();
		});
		bool canHarvest = std::ranges::any_of(this->state->selectedObjects, [this](const GameObject *o){
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			return o->owner->state.index == this->ownerState->index && o->getResourceStorage() != 0;
		});

		if (canDeposit && canHarvest)
			this->_cursor = CURSOR_DUMP;
		else if (
			this->_objectBuffer.back()->isSelectable() &&
			std::ranges::find(this->state->selectedObjects, this->_objectBuffer.back()) == this->state->selectedObjects.end()
		)
			this->_cursor = CURSOR_SELECT;
		else
			this->_cursor = CURSOR_NORMAL;
		return;
	}

	if (std::ranges::all_of(this->state->selectedObjects, [this](const GameObject *o){
		// TODO: Wrong check. Should have a different index per controlling player.
		//       This one is only the player index that were declared at the start but
		//       a player can control multiple in case their ally leaves.
		return this->ownerState->index != o->owner->state.index;
	})) {
		this->_cursor = CURSOR_NORMAL;
		return;
	}

	bool canMove = !this->state->selectedObjects.empty() && std::ranges::all_of(this->state->selectedObjects, [this](const GameObject *o){
		return o->canMove();
	});
	auto &lmap = this->manager->getLoadedMap().grid;

	if (canMove) {
		auto obj = *this->state->selectedObjects.begin();
		auto relPos = obj->map->absToTileCoord(this->_placementPos);
		auto tl = this->map->topLeftCoordToAbs(pos);
		auto passFlags = obj->getPassFlags();
		auto abs = obj->map->tileCoordToAbs(relPos);

		// TODO: Handle non snap-to-grid units
		if (obj->getResourceStorage() != 0 && lmap.anyHasAnyGridBits(abs, GRIDMASK_RESOURCE)) {
			this->_cursor = CURSOR_HARVEST;
			return;
		}
		if (obj->ignoresTerrain())
			this->_cursor = CURSOR_MOVE;
		else if (
			!this->manager->isAreaFree(tl, obj->map->tileSize) ||
			!obj->map->hasAnyGridBits(relPos, passFlags)
		)
			this->_cursor = CURSOR_NO_MOVE;
		else if (
			obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_INFANTRY_CAN_PASS) ||
			obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_VEHICLES_CAN_PASS) || (
				obj->map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_SHIP_CAN_PASS) &&
				passFlags != GRIDMASK_SHIP_CAN_PASS
			)
		)
			this->_cursor = CURSOR_MOVE_RESTRICT;
		else
			this->_cursor = CURSOR_MOVE;
	} else
		this->_cursor = CURSOR_NORMAL;
}

void UI::_updateCursorRepair()
{
	this->_cursor = CURSOR_NO_REPAIR;
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (obj->owner->state.index == this->ownerState->index && !obj->isDead() && obj->getHealth() != obj->getMaxHealth() && obj->isRepairable())
			this->_cursor = CURSOR_REPAIR;
	}
}

void UI::_updateCursorSell()
{
	this->_cursor = CURSOR_NO_SELL;
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (obj->owner->state.index == this->ownerState->index && !obj->isDead() && obj->isSellable())
			this->_cursor = CURSOR_SELL;
	}
}

void UI::_updateCursor()
{
	if (this->onUi) {
		this->_cursor = CURSOR_NORMAL;
		return;
	}
	if (this->_placementBuilding != nullptr) {
		if (!this->_placementBuilding->getTileData()) {
			const auto &queueId = this->_placementBuilding->getBuildQueue();

			this->_cursor = CURSOR_ATTACK;
			if (queueId) {
				auto &queues = this->_faction.getQueues();
				auto it = queues.find(*queueId);

				if (
					it != queues.end() &&
					it->second.unitSpawn == FactionQueue::SPAWN_PLACEMENT &&
					it->second.cursor
				)
					this->_cursor = *it->second.cursor;
			}
		} else
			this->_cursor = CURSOR_NORMAL;
		return;
	}
#ifdef _DEBUG
	if (
		this->_placementPos.x < 0 || this->_placementPos.y < 0 ||
		static_cast<unsigned>(this->_placementPos.x) >= this->map->gridSize.x * this->map->tileSize.x ||
		static_cast<unsigned>(this->_placementPos.y) >= this->map->gridSize.y * this->map->tileSize.y
	) {
		this->_cursor = CURSOR_NORMAL;
		return;
	}
#endif
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer, true, [this](const GameObject *obj) {
		// Stealthed enemies can't be hovered, selected nor targeted.
		return !obj->isHiddenFrom(this->ownerState->team);
	});

	if (this->_selling)
		this->_updateCursorSell();
	else if (this->_repairing)
		this->_updateCursorRepair();
	else
		this->_updateCursorNormal();
}

bool UI::_isSelectionTooFar() const
{
	if (DEBUG_MODE)
		return false;

	const std::set<Vector2i> &okTiles = this->_placementBuilding->getCheckTiles();
	auto &placementGrid = this->map->placementGrid[this->ownerState->team];
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	if (okTiles.empty())
		return false;
	for (auto &pos : okTiles) {
		auto result = pos + gridPos;

		if (
			result.x < 0 || result.y < 0 ||
			result.x >= static_cast<int>(this->map->gridSize.x) ||
			result.y >= static_cast<int>(this->map->gridSize.y)
		)
			continue;
		if (placementGrid[result.x + result.y * this->map->gridSize.x])
			return false;
	}
	return true;
}

void UI::_displayPlacementPreview()
{
	auto &placement = this->_faction.getPlacementCursors();

	if (placement[0].empty())
		return;

	auto &tileData = this->_placementBuilding->getTileData();
	bool tooFar = this->_isSelectionTooFar();
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	if (!tileData)
		return;
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			BuildingMask buildingMask = line[x];

			if (!buildingMask.placementRequired && !buildingMask.placementConcrete)
				continue;

			Sprite *sprite = &placement[2][this->_placementAnimation[2]];
			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (
				full.x >= 0 && full.y >= 0 &&
				static_cast<size_t>(full.x) < this->map->gridSize.x &&
				static_cast<size_t>(full.y) < this->map->gridSize.y
			) {
				if (tooFar || !this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) || (
					this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) &&
					buildingMask.placementConcrete && !buildingMask.placementRequired
				) || !this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize, {}, false, true))
					sprite = &placement[2][this->_placementAnimation[2]];
				else if (this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) || buildingMask.placementConcrete)
					sprite = &placement[0][this->_placementAnimation[0]];
				else
					sprite = &placement[1][this->_placementAnimation[1]];
			}
			sprite->setPosition(this->map->topLeftCoordToAbs(full));
			game->screen->draw(*sprite);
		}
	}
}

void UI::selectBuildingForPlacement(const std::string &id)
{
	auto &objects = this->_faction.getObjects();
	auto it = objects.find(id);

	if (it != objects.end())
		this->_placementBuilding = &it->second;
}

void UI::_handleTryPlace()
{
	auto &tileData = this->_placementBuilding->getTileData();
	auto &placement = this->_faction.getPlacementCursors();
	bool foundGood = false;

	if (!tileData) {
		auto gridPos = this->map->absToTileCoord(this->_placementPos);

		if (
			gridPos.x < 0 || gridPos.y < 0 ||
			static_cast<size_t>(gridPos.x) >= this->map->gridSize.x ||
			static_cast<size_t>(gridPos.y) >= this->map->gridSize.y
		) {
			this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
			return;
		}
		assert_exp(this->_placementBuilding->getBuildQueue());
		this->_eventQueue.emplace_back(PlaceBuildEvent{
			this->_placementBuilding->getId(),
			*this->_placementBuilding->getBuildQueue(),
			sf::Vector2u(this->_placementPos.x, this->_placementPos.y)
		});
		this->_placementBuilding = nullptr;
		return;
	}
	if (placement[0].empty())
		return;
	if (this->_isSelectionTooFar()) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	assert_exp(tileData);
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			BuildingMask buildingMask = line[x];

			if (!buildingMask.placementRequired && !buildingMask.placementConcrete)
				continue;

			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (full.x < 0 || full.y < 0 || static_cast<size_t>(full.x) >= this->map->gridSize.x || static_cast<size_t>(full.y) >+ this->map->gridSize.y)
				continue;
			if (buildingMask.placementRequired) {
				if (
					!this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) ||
					!this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize, {}, false, true)
				) {
					this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
					return;
				}
				foundGood = true;
			} else if (buildingMask.placementConcrete) {
				if (this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) && !this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE))
					foundGood = true;
			}
		}
	}
	if (!foundGood) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	assert_exp(this->_placementBuilding->getBuildQueue());
	this->_eventQueue.emplace_back(PlaceBuildEvent{
		this->_placementBuilding->getId(),
		*this->_placementBuilding->getBuildQueue(),
		sf::Vector2u(this->_placementPos.x, this->_placementPos.y)
	});
	this->_placementBuilding = nullptr;
}

void UI::_tileClickedNormal()
{
	// TODO: Hardcoded key
	if (!this->state->selectedObjects.empty() && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
		if (this->state->selectedObjects.size() == 1) {
			auto obj = *this->state->selectedObjects.begin();

			if (
				obj->owner->state.index == this->ownerState->index &&
				std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()
			) {
				if (obj->isDeployable()) {
					if (!obj->canDeploy())
						this->playEVA(EVA_CANNOT_DEPLOY, 300, 2, 300, "");
					else
						this->_eventQueue.emplace_back(DeployUnitEvent{obj->index});
					return;
				} else if (auto &gq = obj->getGivenQueue()) {
					if (this->ownerState->queueBuildings[*gq].size() > 1 && this->ownerState->primaryBuildings[*gq] != obj) {
						this->_eventQueue.emplace_back(SetPrimaryEvent{obj->index});
						return;
					}
				}
			}
		}

		unsigned dumpId = 0;
		bool canDeposit = std::ranges::any_of(this->_objectBuffer, [this, &dumpId](const GameObject *o){
			// We can provide income to our teammate by sending harvesters to their resource dump.
			if (o->owner->state.team != this->ownerState->team || o->getAcceptResources().empty())
				return false;
			dumpId = o->index;
			return true;
		});
		bool hasHarvester = false;
		bool canHarvest = false;
		std::set<unsigned> usedUnits;
		auto &lmap = this->manager->getLoadedMap().grid;
		std::unordered_map<unsigned, AttackUnitEvent> attack;

		for (auto o : this->state->selectedObjects) {
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (o->owner->state.index != this->ownerState->index || o->getResourceStorage() == 0)
				continue;
			hasHarvester = true;

			auto absQuad = o->map->tileCoordToAbs(o->map->absToTileCoord(this->_placementPos));

			canHarvest = lmap.anyHasAnyGridBits(absQuad, GRIDMASK_RESOURCE);
			if (canHarvest)
				break;
		}

		for (auto s : this->state->selectedObjects) {
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (s->owner->state.index != this->ownerState->index)
			continueOutter:
				continue;

			for (auto &o : this->_objectBuffer)
				if (o->owner->state.team != s->owner->state.team && s->canAttack(o)) {
					auto &a = attack[o->index];

					a.target = o->index;
					a.indices.push_back(s->index);
					usedUnits.emplace(s->index);
					goto continueOutter;
				}
		}
		for (auto &a : attack | std::ranges::views::values)
			this->_eventQueue.emplace_back(a);

		if (!this->_objectBuffer.empty() && (!hasHarvester || !canDeposit) && usedUnits.empty() && this->_objectBuffer.back()->isSelectable()) {
			this->state->selectedObjects.clear();
			this->state->selectedObjects.emplace(this->_objectBuffer.back());
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (this->_objectBuffer.back()->owner->state.index == this->ownerState->index)
				this->_objectBuffer.back()->select();
		} else if (!this->state->selectedObjects.empty()) {
			HarvestUnitEvent hevent;
			DumpUnitEvent devent;
			MoveUnitEvent event;

			event.position = this->_placementPos;
			event.indices.reserve(this->state->selectedObjects.size());
			hevent.position = this->_placementPos;
			hevent.indices.reserve(this->state->selectedObjects.size());
			devent.hole = dumpId;
			devent.indices.reserve(this->state->selectedObjects.size());
			for (auto s : this->state->selectedObjects) {
				// TODO: Wrong check. Should have a different index per controlling player.
				//       This one is only the player index that were declared at the start but
				//       a player can control multiple in case their ally leaves.
				if (s->owner->state.index != this->ownerState->index)
					continue;
				if (usedUnits.contains(s->index))
					continue;

				// TODO: Handle non snap-to-grid units
				if (s->getResourceStorage() != 0) {
					if (canDeposit) {
						devent.indices.push_back(s->index);
						continue;
					} else if (canHarvest) {
						hevent.indices.push_back(s->index);
						continue;
					}
				}
				if (DEBUG_MODE && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I))
					(*std::ranges::find(this->ownerState->myObjects, s))->setPosition(this->_placementPos);
				else
					event.indices.push_back(s->index);
			}
			if (!event.indices.empty())
				this->_eventQueue.emplace_back(event);
			if (!hevent.indices.empty())
				this->_eventQueue.emplace_back(hevent);
			if (!devent.indices.empty())
				this->_eventQueue.emplace_back(devent);
		}
	} else if (!this->_objectBuffer.empty() && this->_objectBuffer.back()->isSelectable()) {
		auto b = this->_objectBuffer.back();
		auto it = this->state->selectedObjects.find(b);

		if (it == this->state->selectedObjects.end()) {
			this->state->selectedObjects.emplace(b);
			// TODO: Wrong check. Should have a different index per controlling player.
			//       This one is only the player index that were declared at the start but
			//       a player can control multiple in case their ally leaves.
			if (b->owner->state.index == this->ownerState->index)
				b->select();
		} else
			this->state->selectedObjects.erase(it);
	}
}

void UI::_tileClickedRepair()
{
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (!obj->isDead() && obj->getHealth() != obj->getMaxHealth() && obj->isRepairable())
			this->_eventQueue.emplace_back(RepairEvent{obj->index});
	}
}

void UI::_tileClickedSell()
{
	if (this->_objectBuffer.size() == 1) {
		const GameObject *obj = this->_objectBuffer.front();

		if (!obj->isDead() && obj->isSellable())
			this->_eventQueue.emplace_back(SellEvent{obj->index});
	}
}

void UI::tileClicked(Vector2i absPos)
{
	if (!this->onUi)
		this->_startDrag = absPos;
	this->_placementPos = absPos;
}

void UI::mouseMoved(Vector2i absPos)
{
	this->onUi = isMouseOverUI();
	if (this->_startDrag)
		if (std::abs(this->_startDrag->x - absPos.x) > 12 && std::abs(this->_startDrag->y - absPos.y) > 12)
			this->_dragging = true;
	this->_placementPos = absPos;
}

void UI::mouseReleased(Vector2i)
{
	if (this->_dragging) {
		Vector2i tl = {
			std::min(this->_startDrag->x, this->_placementPos.x),
			std::min(this->_startDrag->y, this->_placementPos.y)
		};
		Vector2i br = {
			std::max(this->_startDrag->x, this->_placementPos.x),
			std::max(this->_startDrag->y, this->_placementPos.y)
		};

		this->_dragging = false;
		this->_placementBuilding = nullptr;
		this->_startDrag.reset();
		this->_objectBuffer.clear();
		this->manager->getObjectsInArea(tl, br - tl, this->_objectBuffer, false, [this](const GameObject *obj) {
			return obj->owner->state.index == this->ownerState->index && obj->canMove() && obj->isSelectable();
		});
		if (!this->_objectBuffer.empty()) {
			this->state->selectedObjects.clear();
			this->_objectBuffer.front()->select();
			for (auto t : this->_objectBuffer)
				this->state->selectedObjects.emplace(t);
		}
	} else {
		this->_startDrag.reset();
		if (!this->onUi) {
			if (this->_placementBuilding != nullptr)
				return this->_handleTryPlace();
			this->_objectBuffer.clear();
			this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer, true, [this](const GameObject *obj) {
				// Stealthed enemies can't be hovered, selected nor targeted.
				return !obj->isHiddenFrom(this->ownerState->team);
			});

			if (this->_selling)
				this->_tileClickedSell();
			else if (this->_repairing)
				this->_tileClickedRepair();
			else
				this->_tileClickedNormal();
		}
	}
}

void UI::onCancel(Vector2i absPos)
{
	if (this->onUi)
		return;
	if (this->_placementBuilding)
		this->_placementBuilding = nullptr;
	else if (!this->state->selectedObjects.empty())
		this->state->selectedObjects.clear();
	this->_selling = false;
	this->_repairing = false;
	this->mouseMoved(absPos);
}

bool UI::isMouseOnUi() const
{
	return this->onUi;
}

void UI::_displaySelectBox(Vector2i top, Vector2u size)
{
	sf::Sprite sprite{this->_selectCorners[0]};
	sf::RectangleShape rect;

	top.y += 4;
	sprite.setPosition(Vector2f(top.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[1], true);
	sprite.setPosition(Vector2f(top.x - this->_selectCorners[1].getSize().x + size.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[2], true);
	sprite.setPosition(Vector2f(top.x, top.y - this->_selectCorners[2].getSize().y + size.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[3], true);
	sprite.setPosition(Vector2f(
		top.x - this->_selectCorners[3].getSize().x + size.x,
		top.y - this->_selectCorners[3].getSize().y + size.y
	));
	game->screen->draw(sprite);
}
