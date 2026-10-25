//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_UTILS_HPP
#define MIXRTS_UTILS_HPP


#include <TGUI/TGUI.hpp>
#include <nlohmann/json.hpp>
#include "Color.hpp"

MYDLL_API nlohmann::json &mergeObjects(nlohmann::json &dst, const nlohmann::json &t);

#define TOHEX_ENDIAN_LITTLE false
#define TOHEX_ENDIAN_BIG true
MYDLL_API std::string toHex(const unsigned char *buffer, size_t size, bool endian);
MYDLL_API std::wstring wtoHex(const unsigned char *buffer, size_t size, bool endian);

template<typename T, size_t size = sizeof(T)>
std::enable_if_t<std::is_integral_v<T>, std::string> toHex(const T &v)
{
	if constexpr (std::endian::native == std::endian::big)
		return toHex(reinterpret_cast<const unsigned char *>(&v), size, TOHEX_ENDIAN_BIG);
	else if constexpr (std::endian::native == std::endian::little)
		return toHex(reinterpret_cast<const unsigned char *>(&v), size, TOHEX_ENDIAN_LITTLE);
	else
		static_assert(false, "Unknown endianness");
}

template<typename T, size_t size = sizeof(T)>
std::enable_if_t<std::is_integral_v<T>, std::wstring> wtoHex(const T &v)
{
	if constexpr (std::endian::native == std::endian::big)
		return wtoHex(reinterpret_cast<const unsigned char *>(&v), size, TOHEX_ENDIAN_BIG);
	else if constexpr (std::endian::native == std::endian::little)
		return wtoHex(reinterpret_cast<const unsigned char *>(&v), size, TOHEX_ENDIAN_LITTLE);
	else
		static_assert(false, "Unknown endianness");
}

#define RESOURCE(declaredPkg, name) (std::string(name).find(':') == std::string::npos ? std::string(declaredPkg) + ":" + std::string(name) : std::string(name))

struct MYDLL_API HSLColor {
	unsigned short h = 0;
	unsigned short s = 0;
	unsigned short l = 0;
	unsigned char a = 255;

	HSLColor() = default;
	HSLColor(unsigned short h, unsigned short s, unsigned short l, unsigned char a = 255);
	HSLColor(Color color);
	Color toRGB() const;
};

inline std::vector<std::string> split(const std::string &str, const std::string &delims) {
	std::vector<std::string> vec;
	std::size_t old_pos = 0;
	std::size_t pos = 0;

	while ((pos = str.find_first_of(delims, old_pos)) != std::string::npos) {
		vec.emplace_back(str.substr(old_pos, pos - old_pos));
		old_pos = pos + 1;
	}
	vec.emplace_back(str.substr(old_pos));
	return vec;
}

#include <sol/sol.hpp>
// --- Lua conversion handlers for HSLColor ---
// Found through ADL from sol2. HSLColor is represented in Lua as a plain
// table of the form { h = ..., s = ..., l = ..., a = ... } (alpha optional,
// defaults to 255 on get).

template <typename Handler>
bool sol_lua_check(sol::types<HSLColor>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		sol::stack_table table(L, index);
		sol::object h = table["h"];
		sol::object s = table["s"];
		sol::object l = table["l"];

		if (h.is<short>() && s.is<short>() && l.is<short>()) {
			tracking.use(1);
			return true;
		}
	}

	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {h, s, l, a}");
	return false;
}

inline HSLColor sol_lua_get(sol::types<HSLColor>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	short h = table.get<short>("h");
	short s = table.get<short>("s");
	short l = table.get<short>("l");
	unsigned char a = table.get_or<unsigned char>("a", 255);

	tracking.use(1);
	return HSLColor(h, s, l, a);
}

inline int sol_lua_push(lua_State *L, const HSLColor &color)
{
	lua_createtable(L, 0, 4);
	sol::stack::push(L, color.h);
	lua_setfield(L, -2, "h");
	sol::stack::push(L, color.s);
	lua_setfield(L, -2, "s");
	sol::stack::push(L, color.l);
	lua_setfield(L, -2, "l");
	sol::stack::push(L, color.a);
	lua_setfield(L, -2, "a");
	return 1;
}


//! @brief Display a focused window.
//! @param gui The gui handling the window.
//! @param width The width of the window.
//! @param height The height of the window.
//! @return A pointer to the window
template<typename T, typename WindowType = tgui::ChildWindow, typename ...Args>
typename WindowType::Ptr openWindowWithFocus(T &gui, tgui::Layout width, tgui::Layout height, typename WindowType::Ptr window = nullptr, bool closeOut = true, Args ...args)
{
	auto panel = tgui::Panel::create({"100%", "100%"});

	panel->getRenderer()->setBackgroundColor({0, 0, 0, 175});
	panel->setUserData(false);
	gui.add(panel);

	if (!window) {
		window = WindowType::create(args...);
		if (!width.isConstant() || width.getValue() != 0)
			window->setSize(width, height);
	}/* else
		setRenderer(static_cast<tgui::Container::Ptr>(window));*/
	window->setPosition("(&.w - w) / 2", "(&.h - h) / 2");
	gui.add(window);

	window->setFocused(true);

	auto closeWindow = [&gui, window, panel]{
		gui.remove(window);
		gui.remove(panel);
	};

	if (closeOut)
		panel->onClick(closeWindow);
	window->onClose(closeWindow);
	window->onEscapeKeyPress(closeWindow);
	return window;
}

//! @brief Get the last Exception Name
//! @details Return the last type of Exception name
//! @return std::string The last Exception name
MYDLL_API std::string getLastExceptionName();


#endif //MIXRTS_UTILS_HPP