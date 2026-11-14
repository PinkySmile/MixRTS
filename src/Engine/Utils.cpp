//
// Created by PinkySmile on 10/01/2026.
//

#ifdef __GNUG__
#include <cxxabi.h>
#endif
#include <cmath>
#include <cstring>
#include <array>
#include <fstream>
#include <vector>
#include <algorithm>
#include <bit>
#include "Utils.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"


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

tgui::FileDialog::Ptr openFileDialog(const std::string &title, const std::filesystem::path &basePath, bool overWriteWarning, bool mustExist)
{
	auto dialog = tgui::FileDialog::create(title, overWriteWarning ? "Save" : "Open", overWriteWarning);

	dialog->setFileMustExist(mustExist);
	dialog->setPath(tgui::Filesystem::Path(absolute(basePath)));
	openWindowWithFocus(game->gui, 0, 0, dialog);
	return dialog;
}

tgui::FileDialog::Ptr saveFileDialog(const std::string &title, const std::filesystem::path &basePath)
{
	return openFileDialog(title, basePath, true, false);
}

static uint32_t crc32ForByte(uint32_t r)
{
	for (int j = 0; j < 8; ++j)
		r = (r & 1 ? 0 : static_cast<uint32_t>(0xEDB88320L)) ^ r >> 1;
	return r ^ static_cast<uint32_t>(0xFF000000L);
}

uint32_t crc32(const void *data, size_t size, uint32_t crc)
{
	static const auto table = [] {
		std::array<uint32_t, 256> values{};

		for (uint32_t i = 0; i < values.size(); i++)
			values[i] = crc32ForByte(i);
		return values;
	}();
	auto bytes = static_cast<const uint8_t *>(data);

	for (size_t i = 0; i < size; i++)
		crc = table[static_cast<uint8_t>(crc) ^ bytes[i]] ^ crc >> 8;
	return crc;
}

uint32_t crc32File(const std::filesystem::path &path, uint32_t crc, bool normalizeNewlines)
{
	std::ifstream stream{path, std::ios::binary};
	std::vector<char> data;

	assert_msg(stream, "Failed to open " + path.string());
	data.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	if (normalizeNewlines)
		data.erase(std::remove(data.begin(), data.end(), '\r'), data.end());
	return crc32(data.data(), data.size(), crc);
}

namespace {
	constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
	constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
	constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
	constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
	constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

	uint64_t rotl64(uint64_t value, int shift)
	{
		return (value << shift) | (value >> (64 - shift));
	}

	uint64_t read64(const uint8_t *data)
	{
		uint64_t value;

		memcpy(&value, data, sizeof(value));
		return value;
	}

	uint32_t read32(const uint8_t *data)
	{
		uint32_t value;

		memcpy(&value, data, sizeof(value));
		return value;
	}

	uint64_t xxhRound(uint64_t acc, uint64_t input)
	{
		acc += input * PRIME64_2;
		acc = rotl64(acc, 31);
		acc *= PRIME64_1;
		return acc;
	}

	uint64_t xxhMerge(uint64_t acc, uint64_t value)
	{
		value = xxhRound(0, value);
		acc ^= value;
		acc = acc * PRIME64_1 + PRIME64_4;
		return acc;
	}
}

uint64_t hash64(const void *data, size_t size, uint64_t seed)
{
	auto *bytes = static_cast<const uint8_t *>(data);
	const uint8_t *end = bytes + size;
	uint64_t hash;

	static_assert(std::endian::native == std::endian::little, "hash64 reads integers in host order");
	if (size >= 32) {
		const uint8_t *limit = end - 32;
		uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
		uint64_t v2 = seed + PRIME64_2;
		uint64_t v3 = seed;
		uint64_t v4 = seed - PRIME64_1;

		do {
			v1 = xxhRound(v1, read64(bytes));
			bytes += 8;
			v2 = xxhRound(v2, read64(bytes));
			bytes += 8;
			v3 = xxhRound(v3, read64(bytes));
			bytes += 8;
			v4 = xxhRound(v4, read64(bytes));
			bytes += 8;
		} while (bytes <= limit);
		hash = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
		hash = xxhMerge(hash, v1);
		hash = xxhMerge(hash, v2);
		hash = xxhMerge(hash, v3);
		hash = xxhMerge(hash, v4);
	} else
		hash = seed + PRIME64_5;
	hash += size;
	while (bytes + 8 <= end) {
		uint64_t k1 = xxhRound(0, read64(bytes));

		hash ^= k1;
		hash = rotl64(hash, 27) * PRIME64_1 + PRIME64_4;
		bytes += 8;
	}
	if (bytes + 4 <= end) {
		hash ^= static_cast<uint64_t>(read32(bytes)) * PRIME64_1;
		hash = rotl64(hash, 23) * PRIME64_2 + PRIME64_3;
		bytes += 4;
	}
	while (bytes < end) {
		hash ^= (*bytes) * PRIME64_5;
		hash = rotl64(hash, 11) * PRIME64_1;
		bytes++;
	}
	hash ^= hash >> 33;
	hash *= PRIME64_2;
	hash ^= hash >> 29;
	hash *= PRIME64_3;
	hash ^= hash >> 32;
	return hash;
}

std::string getLocale()
{
#ifdef _WIN32
	char locale[LOCALE_NAME_MAX_LENGTH] = {0};
	wchar_t localw[LOCALE_NAME_MAX_LENGTH] = {0};

	GetUserDefaultLocaleName(localw, sizeof(localw));
	for (int i = 0; localw[i]; i++)
		locale[i] = localw[i];

	auto end = strchr(locale, '-');

	if (end)
		*end = 0;
	return locale;
#else
	char locale[1024] = {0};
	char *lang = getenv("LANG");

	if (!lang)
		strcpy(locale, "en_EN");
	else
		strncpy(locale, lang, sizeof(locale) - 1);

	auto end = strchr(locale, '_');

	if (end)
		*end = 0;
	return locale;
#endif
}

void replace(std::string &str, const std::string &from, const std::string &to)
{
	while (true) {
		size_t start_pos = str.find(from);

		if (start_pos == std::string::npos)
			return;
		str.replace(start_pos, from.length(), to);
	}
}

void simplifySym(std::string &sym)
{
	replace(sym, "_AssertionFailedException", "AssertionFailedException");
	replace(sym, "nlohmann::json_abi_v3_12_0::basic_json<std::map, std::vector, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool, long, unsigned long, double, std::allocator, nlohmann::json_abi_v3_12_0::adl_serializer, std::vector<unsigned char, std::allocator<unsigned char> >, void>", "nlohmann::json");
	replace(sym, "nlohmann::json_abi_v3_12_0::basic_json<std::map, std::vector, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool, long long, unsigned long long, double, std::allocator, nlohmann::json_abi_v3_12_0::adl_serializer, std::vector<unsigned char, std::allocator<unsigned char> >, void>", "nlohmann::json");
	replace(sym, "nlohmann::json_abi_v3_12_0", "nlohmann");
	replace(sym, "std::filesystem::__cxx11::path", "std::filesystem::path");
	replace(sym, "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >", "std::string");
	replace(sym, "std::__cxx11::basic_string<char>", "std::string");

	size_t pos = 0;
	while (true) {
		pos = sym.find("std::vector<", pos);
		if (pos == std::string::npos)
			break;
		pos += 12;
		auto startPos = pos;

		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;

		auto type = sym.substr(startPos, pos - startPos);
		auto sPos = pos;

		unsigned nb = 0;
		while (pos < sym.size() && (nb != 0 || sym[pos] != '>')) {
			nb -= sym[pos] == '>';
			nb += sym[pos] == '<';
			pos++;
		}
		if (pos >= sym.size())
			break;

		auto target = sym.substr(sPos, pos - sPos);
		if (target == ", std::allocator<" + type + "> ")
			sym.replace(startPos, pos - startPos - 1, type);
	}

	pos = 0;
	while (true) {
		pos = sym.find("std::unordered_map<", pos);
		if (pos == std::string::npos)
			break;
		pos += 19;
		auto startPos = pos;

		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;
		auto type1 = sym.substr(startPos, pos - startPos);
		pos += 2;
		auto sPos = pos;
		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;
		auto type2 = sym.substr(sPos, pos - sPos);

		sPos = pos;
		unsigned nb = 0;
		while (pos < sym.size() && (nb != 0 || sym[pos] != '>')) {
			nb -= sym[pos] == '>';
			nb += sym[pos] == '<';
			pos++;
		}
		if (pos >= sym.size())
			break;

		auto target = sym.substr(sPos, pos - sPos);
		auto t = ", std::hash<" + type1 + " >, std::equal_to<" + type1 + " >, std::allocator<std::pair<" + type1 + " const, " + type2 + "> > ";
		if (target == t)
			sym.replace(startPos, pos - startPos - 1, type1 + ", " + type2);
	}
}

std::string shortenFctName(std::string name)
{
	simplifySym(name);

	std::vector<std::string> args;
	size_t pos1 = name.find('(') + 1;
	size_t pos2 = name.find(')');
	std::string argsList = name.substr(pos1, pos2 - pos1);
	std::string result = name.substr(0, pos1);
	size_t start = 0;
	size_t f = 0;

	for (size_t i = 0; i < argsList.size(); i++) {
		if (start == i && argsList[i] == ' ')
			start++;
		if (f == 0 && argsList[i] == ',') {
			args.push_back(argsList.substr(start, i - start + 1));
			start = i + 1;
		} else if (argsList[i] == '>')
			f--;
		else if (argsList[i] == '<')
			f++;
	}
	args.push_back(argsList.substr(start));
	if (args.size() <= 1)
		return name;

	for (auto &a : args) {
		result += "\n\t\t";
		result += a;
	}
	result += "\n\t";
	result += name.substr(pos2);
	return result;
}
