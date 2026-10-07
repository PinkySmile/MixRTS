//
// Created by PinkySmile on 18/09/2021.
//

#ifndef MIXRTS_COLOR_HPP
#define MIXRTS_COLOR_HPP


#include <SFML/Graphics/Color.hpp>
#include "Resources/MSVCMacros.hpp"

union MYDLL_API Color {
	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	unsigned value;

	Color() {};
	explicit Color(const sf::Color &c) : r(c.r), g(c.g), b(c.b), a(c.a) {};
	explicit Color(unsigned v) : value(v) {}
	Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) : r(r), g(g), b(b), a(a) {}

	bool operator==(const Color &other) const
	{
		return this->value == other.value;
	}

	bool operator!=(const Color &other) const
	{
		return !(*this == other);
	}

	void tint(const Color &other)
	{
		this->r = other.r * this->r / 255;
		this->g = other.g * this->g / 255;
		this->b = other.b * this->b / 255;
		this->a = other.a * this->a / 255;
	}

	operator sf::Color() const
	{
		return {this->r, this->g, this->b, this->a};
	}

	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Magenta;
	static const Color Cyan;
	static const Color Transparent;
};


#include <sol/sol.hpp>
// --- Lua conversion handlers for Color ---
// Found through ADL from sol2. Color is represented in Lua as a plain
// table of the form { r = ..., g = ..., b = ..., a = ... } (alpha optional,
// defaults to 255 on get).

template <typename Handler>
bool sol_lua_check(sol::types<Color>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		sol::stack_table table(L, index);
		sol::object r = table["r"];
		sol::object g = table["g"];
		sol::object b = table["b"];

		if (r.is<unsigned>() && g.is<unsigned>() && b.is<unsigned>()) {
			tracking.use(1);
			return true;
		}
	}

	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {r, g, b, a}");
	return false;
}

inline Color sol_lua_get(sol::types<Color>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	unsigned char r = table.get<unsigned char>("r");
	unsigned char g = table.get<unsigned char>("g");
	unsigned char b = table.get<unsigned char>("b");
	unsigned char a = table.get_or<unsigned char>("a", 255);

	tracking.use(1);
	return Color(r, g, b, a);
}

inline int sol_lua_push(lua_State *L, const Color &color)
{
	lua_createtable(L, 0, 4);
	sol::stack::push(L, color.r);
	lua_setfield(L, -2, "r");
	sol::stack::push(L, color.g);
	lua_setfield(L, -2, "g");
	sol::stack::push(L, color.b);
	lua_setfield(L, -2, "b");
	sol::stack::push(L, color.a);
	lua_setfield(L, -2, "a");
	return 1;
}


#endif //MIXRTS_COLOR_HPP
