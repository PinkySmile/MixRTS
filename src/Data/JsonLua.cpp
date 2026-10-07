//
// Created by PinkySmile on 04/10/2026.
//

#include <sol/sol.hpp>
#include "JsonLua.hpp"

namespace nlohmann {
	nlohmann::json sol_lua_get(sol::types<nlohmann::json>, lua_State *L, int index, sol::stack::record &tracking)
	{
		switch (lua_type(L, index)) {
		case LUA_TNIL:
		case LUA_TNONE:
			tracking.use(1);
			return nullptr;
		case LUA_TBOOLEAN: {
			bool v = sol::stack::get<bool>(L, index);

			tracking.use(1);
			return v;
		}
		case LUA_TNUMBER: {
			if (lua_isinteger(L, index)) {
				long long v = sol::stack::get<long long>(L, index);

				tracking.use(1);
				return v;
			}

			double v = sol::stack::get<double>(L, index);

			tracking.use(1);
			return v;
		}
		case LUA_TSTRING: {
			std::string v = sol::stack::get<std::string>(L, index);

			tracking.use(1);
			return v;
		}
		case LUA_TTABLE: {
			sol::stack_table table(L, index);
			bool isArray = true;
			size_t count = 0;

			for (auto &pair : table) {
				count++;
				if (pair.first.get_type() != sol::type::number) {
					isArray = false;
					break;
				}
			}
			if (isArray && count == table.size()) {
				nlohmann::json array = nlohmann::json::array();

				for (size_t i = 1; i <= count; i++) {
					sol::object value = table[i];

					sol::stack::push(L, value);
					array.push_back(sol_lua_get(sol::types<nlohmann::json>{}, L, -1, tracking));
					lua_pop(L, 1);
				}
				tracking.use(1);
				return array;
			}

			nlohmann::json object = nlohmann::json::object();

			for (auto &pair : table) {
				std::string key = pair.first.as<std::string>();

				sol::stack::push(L, pair.second);
				object[key] = sol_lua_get(sol::types<nlohmann::json>{}, L, -1, tracking);
				lua_pop(L, 1);
			}
			tracking.use(1);
			return object;
		}
		default:
			tracking.use(1);
			return nullptr;
		}
	}

	int sol_lua_push(lua_State *L, const nlohmann::json &json)
	{
		if (json.is_null())
			return sol::stack::push(L, sol::nil);
		if (json.is_boolean())
			return sol::stack::push(L, json.get<bool>());
		if (json.is_number_integer())
			return sol::stack::push(L, json.get<long long>());
		if (json.is_number_float())
			return sol::stack::push(L, json.get<double>());
		if (json.is_string())
			return sol::stack::push(L, json.get<std::string>());
		if (json.is_array()) {
			lua_createtable(L, static_cast<int>(json.size()), 0);
			for (size_t i = 0; i < json.size(); i++) {
				sol_lua_push(L, json[i]);
				lua_seti(L, -2, static_cast<lua_Integer>(i + 1));
			}
			return 1;
		}
		if (json.is_object()) {
			lua_createtable(L, 0, static_cast<int>(json.size()));
			for (auto &[key, value] : json.items()) {
				sol_lua_push(L, value);
				lua_setfield(L, -2, key.c_str());
			}
			return 1;
		}
		return sol::stack::push(L, sol::nil);
	}
}
