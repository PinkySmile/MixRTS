//
// Created by PinkySmile on 04/10/2026.
//
// Generic Lua <-> nlohmann::json conversion. Picked up by sol2 through ADL
// (nlohmann::json's enclosing namespace is `nlohmann`, which is where these
// free functions live). A json value is represented in Lua as the most
// natural equivalent: object -> table with string keys, array -> sequence
// table (1-indexed), string/number/bool -> the matching Lua primitive,
// null -> nil.
//
// This is used for "free-form" per-mod JSON blobs that are handed to UI
// scripts as-is (e.g. Faction::getUIData()/getRepairAnim()), where the shape
// isn't known/fixed by the engine ahead of time, unlike the other ADL
// conversions in this codebase which target one specific, fixed-shape C++
// struct.

#ifndef MIXRTS_JSONLUA_HPP
#define MIXRTS_JSONLUA_HPP

#include <nlohmann/json.hpp>
#include <sol/forward.hpp>

namespace nlohmann {
	template <typename Handler>
	bool sol_lua_check(sol::types<nlohmann::json>, lua_State *, int, Handler &&, sol::stack::record &tracking)
	{
		// Any Lua value can be represented as (or coerced into) a json value.
		tracking.use(1);
		return true;
	}

	nlohmann::json sol_lua_get(sol::types<nlohmann::json>, lua_State *L, int index, sol::stack::record &tracking);
	int sol_lua_push(lua_State *L, const nlohmann::json &json);
}

#endif //MIXRTS_JSONLUA_HPP
