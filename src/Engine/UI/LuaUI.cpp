//
// Created by PinkySmile on 04/10/2026.
//

#include "../Data/Objects/GameObject.hpp"
#include "LuaUI.hpp"
#include "TGUILua.hpp"

static void checkLuaResult(const sol::protected_function_result &result)
{
	if (!result.valid()) {
		sol::error err = result;
		throw err;
	}
}

template<typename... Args>
static bool callLuaFunc(const sol::object &func, Args&&... args)
{
	if (func.valid()) {
		assert_exp(func.is<sol::function>());
		sol::protected_function_result result = static_cast<sol::function>(func)(std::forward<Args>(args)...);

		checkLuaResult(result);
		return true;
	}
	return false;
}

LuaUI::LuaUI(const Faction &faction, UIState *ostate, GameManager *manager, Player *owner, MapState *map, sol::state &state, const std::filesystem::path &path) :
	UI(faction, ostate, manager, owner, map),
	_env{state, sol::create, state.globals()}
{
	this->_env["super"] = std::ref(*static_cast<UI *>(this));
	// safe_script_file (rather than do_file) throws automatically if the
	// script fails to load/parse or errors at the top level while running.
	this->_table = state.safe_script_file(path.string(), this->_env);
}

void LuaUI::setEnabled(bool enabled)
{
	if (!callLuaFunc(this->_table["set_enabled"], enabled))
		UI::setEnabled(enabled);
}

void LuaUI::playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args)
{
	if (!callLuaFunc(this->_table["play_eva"], sound, expireIn, priority, cooldown, args))
		UI::playEVA(sound, expireIn, priority, cooldown, args);
}

std::optional<UI::Event> LuaUI::pollEvent()
{
	std::optional<Event> event;
	sol::object v = this->_table["poll_event"];

	if (v.valid()) {
		assert_exp(v.is<sol::function>());
		sol::protected_function_result result = static_cast<sol::function>(v)();

		checkLuaResult(result);
		event = result;
	} else
		event = UI::pollEvent();
	return event;
}

void LuaUI::renderAbove()
{
	if (!callLuaFunc(this->_table["render_above"]))
		UI::renderAbove();
}

void LuaUI::updateRender()
{
	if (!callLuaFunc(this->_table["update_render"]))
		UI::updateRender();
}

void LuaUI::onCancel(Vector2i absPos)
{
	if (!callLuaFunc(this->_table["on_cancel"], absPos))
		UI::onCancel(absPos);
}

void LuaUI::tileClicked(Vector2i absPos)
{
	if (!callLuaFunc(this->_table["tile_clicked"], absPos))
		UI::tileClicked(absPos);
}

void LuaUI::mouseMoved(Vector2i absPos)
{
	if (!callLuaFunc(this->_table["mouse_moved_to_tile"], absPos))
		UI::mouseMoved(absPos);
}

void LuaUI::onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta)
{
	if (!callLuaFunc(this->_table["on_key_pressed"], key, ctrl, shift, alt, meta))
		UI::onKeyPressed(key, ctrl, shift, alt, meta);
}

void LuaUI::update()
{
	if (!callLuaFunc(this->_table["update"]))
		UI::update();
}

void LuaUI::render()
{
	if (!callLuaFunc(this->_table["render"]))
		UI::render();
}

void LuaUI::_displaySelectedObjectUI(const GameObject &object)
{
	assert_exp(callLuaFunc(this->_table["_display_selected_object_ui"], std::ref(object)));
}
