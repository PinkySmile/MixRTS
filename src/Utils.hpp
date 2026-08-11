//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_UTILS_HPP
#define MIXRTS_UTILS_HPP


#include <TGUI/TGUI.hpp>
#include <nlohmann/json.hpp>
#include "Color.hpp"

nlohmann::json &mergeObjects(nlohmann::json &dst, const nlohmann::json &t);

#define TOHEX_ENDIAN_LITTLE false
#define TOHEX_ENDIAN_BIG true
std::string toHex(const unsigned char *buffer, size_t size, bool endian);
std::wstring wtoHex(const unsigned char *buffer, size_t size, bool endian);

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

struct HSLColor {
	short h = 0;
	short s = 0;
	short l = 0;
	unsigned char a = 255;

	HSLColor() = default;
	HSLColor(short h, short s, short l, unsigned char a = 255);
	HSLColor(Color color);
	Color toRGB() const;
};


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
std::string getLastExceptionName();


#endif //MIXRTS_UTILS_HPP