//
// Created by PinkySmile on 10/01/2026.
//

#ifdef __GNUG__
#include <cxxabi.h>
#endif
#include <cmath>
#include "Utils.hpp"
#include "Resources/Assert.hpp"


nlohmann::json &mergeObjects(nlohmann::json &dst, const nlohmann::json &t)
{
	for (auto &[key, value] : t.items()) {
		if (!dst.contains(key)) {
			dst[key] = value;
		} else if (dst[key].is_array()) {
			assert_exp(value.is_array());
			for (auto &item : value)
				dst[key].push_back(item);
		}
	}
	return dst;
}

std::string toHex(const unsigned char *buffer, size_t size, bool endian)
{
	char digits[] = "0123456789ABCDEF";
	std::string result;

	result.resize(size * 2);
	for (size_t i = 0; i < size; i++) {
		size_t index = endian == TOHEX_ENDIAN_BIG ? i : size - i - 1;
		unsigned char c = buffer[index];

		result[i * 2 + 0] = digits[c >> 4];
		result[i * 2 + 1] = digits[c & 0xF];
	}
	return result;
}

std::wstring wtoHex(const unsigned char *buffer, size_t size, bool endian)
{
	wchar_t digits[] = L"0123456789ABCDEF";
	std::wstring result;

	result.resize(size * 2);
	for (size_t i = 0; i < size; i++) {
		size_t index = endian == TOHEX_ENDIAN_BIG ? i : size - i - 1;
		unsigned char c = buffer[index];

		result[i * 2 + 0] = digits[c >> 4];
		result[i * 2 + 1] = digits[c & 0xF];
	}
	return result;
}

HSLColor::HSLColor(unsigned short h, unsigned short s, unsigned short l, unsigned char a) :
	h(h % 360),
	s(s),
	l(l),
	a(a)
{
	assert_exp(s <= 1000);
	assert_exp(l <= 1000);
}

HSLColor::HSLColor(Color color)
{
	double r = color.r / 255.;
	double g = color.g / 255.;
	double b = color.b / 255.;
	double xmax = std::max(r, std::max(g, b));
	double xmin = std::min(r, std::min(g, b));
	double chroma = xmax - xmin;
	double h;
	double s;
	double v = xmax;
	double l = (xmax + xmin) / 2;

	if (chroma == 0)
		h = 0;
	else if (xmax == r)
		h = 60 * std::fmod((g - b) / static_cast<float>(chroma), 6);
	else if (xmax == g)
		h = 60 * ((b - r) / static_cast<float>(chroma) + 2);
	else// if (xmax == b)
		h = 60 * ((r - g) / static_cast<float>(chroma) + 4);
	h = std::fmod(h + 360, 360);
	if (l == 0 || l == 1)
		s = 0;
	else
		s = (v - l) / std::min(l, 1 - l);

	this->h = static_cast<short>(h);
	this->s = static_cast<short>(1000 * s);
	this->l = static_cast<short>(1000 * l);
	this->a = color.a;
}

Color HSLColor::toRGB() const
{
	auto f = [this](unsigned n){
		double s = this->s / 1000.;
		double l = this->l / 1000.;
		double k = std::fmod(n + this->h / 30., 12);
		double a = s * std::min(l, 1 - l);

		return l - a * std::max(-1., std::min(1., std::min(k - 3, 9 - k)));
	};

	return Color(f(0) * 255, f(8) * 255, f(4) * 255, 255);
}

unsigned angleToDirection(double angle, size_t count)
{
	if (count <= 1)
		return 0;

	double scaled = (angle + M_PI_2) * static_cast<double>(count) / (M_PI * 2);
	auto index = static_cast<long long>(std::llround(scaled));
	auto directions = static_cast<long long>(count);

	index %= directions;
	if (index < 0)
		index += directions;
	return static_cast<unsigned>(index);
}

double normalizeAngle(double angle)
{
	angle = std::fmod(angle + M_PI, M_PI * 2);
	if (angle < 0)
		angle += M_PI * 2;
	return angle - M_PI;
}

bool rotateTowards(float &angle, float target, float maxStep)
{
	double delta = normalizeAngle(static_cast<double>(target) - angle);

	if (std::abs(delta) <= static_cast<double>(maxStep)) {
		angle = target;
		return true;
	}
	angle = static_cast<float>(normalizeAngle(angle + std::copysign(static_cast<double>(maxStep), delta)));
	return false;
}

std::string getLastExceptionName()
{
#ifdef __GNUG__
	int status;
	char *value;
	std::string name;

	auto val = abi::__cxa_current_exception_type();

	if (!val)
		return "No exception";

	value = abi::__cxa_demangle(val->name(), nullptr, nullptr, &status);
	name = value;
	free(value);
	return name;
#else
	return "Unknown exception";
#endif
}
