//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_UTILS_HPP
#define MIXRTS_UTILS_HPP


#include "nlohmann/json.hpp"
#include "Color.hpp"

nlohmann::json &mergeObjects(nlohmann::json &dst, const nlohmann::json &t);

#define TOHEX_ENDIAN_LITTLE false
#define TOHEX_ENDIAN_BIG true
std::string toHex(const unsigned char *buffer, size_t size, bool endian);

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


#endif //MIXRTS_UTILS_HPP