//
// Created by PinkySmile on 04/10/2026.
//

#ifndef MIXRTS_TGUILUA_HPP
#define MIXRTS_TGUILUA_HPP

#include <sol/sol.hpp>

//! Registers Lua usertypes for the subset of TGUI widgets/helpers needed to
//! build UI panels (e.g. D2K's cameo queues) entirely from Lua: Widget,
//! Container, ClickableWidget, Panel, ScrollablePanel, Button, Picture,
//! their renderers, Signal (for onMousePress/onClick/... callbacks), Texture
//! and the scrollbar accessor. Must be called before D2KCameo::pushUserType,
//! which registers itself as a tgui::ClickableWidget subclass.
void pushTGUIUserTypes(sol::state &state);

/*
namespace tgui {
	template <typename Handler>
	bool sol_lua_check(sol::types<String>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
	{
		if (lua_type(L, index) == LUA_TSTRING) {
			tracking.use(1);
			return true;
		}

		handler(L, index, sol::type_of(L, index), sol::type::string, "expected string");
		return false;
	}

	inline String sol_lua_get(sol::types<String>, lua_State *L, int index, sol::stack::record &tracking)
	{
		std::string r = lua_tostring(L, index);

		tracking.use(1);
		return r;
	}

	inline int sol_lua_push(lua_State *L, const String &str)
	{
		puts("PUSH");
		sol::stack::push(L, str.toStdString());
		return 1;
	}
}
*/


#endif //MIXRTS_TGUILUA_HPP
