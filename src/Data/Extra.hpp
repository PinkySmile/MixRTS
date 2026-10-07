//
// Created by PinkySmile on 27/09/2026.
//

#ifndef MIXRTS_EXTRA_HPP
#define MIXRTS_EXTRA_HPP


#include <nlohmann/json_fwd.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Data/Vector.hpp"

struct MYDLL_API Position {
	enum Pos {
		POS_CLOSE_BORDER,
		POS_FAR_BORDER,
		POS_MIN_BORDER,
		POS_MAX_BORDER,
		POS_CLOSE_BORDER_CLOSE,
		POS_FAR_BORDER_CLOSE,
		POS_MIN_BORDER_CLOSE,
		POS_MAX_BORDER_CLOSE,
		POS_CLOSE_BORDER_FAR,
		POS_FAR_BORDER_FAR,
		POS_MIN_BORDER_FAR,
		POS_MAX_BORDER_FAR,
	};

	Vector2f pos = {0, 0};

	Position() = default;
	Position(const nlohmann::json &json);
	[[nodiscard]] Vector2f getPosition(const Vector2f &target, const MapState &map, Vector2f extraOffset) const;

	static void pushUserType(sol::state &state);

private:
	[[nodiscard]] float _getPos(const nlohmann::json &json) const;
	[[nodiscard]] float _getPos(float base, float target, float border, float extraOffset) const;
};

struct MYDLL_API ExtraSpawn {
	std::string unit;
	Position pos;
	bool invincible;
	std::optional<Position> dest;
	std::optional<Position> out;
	std::vector<ExtraSpawn> carry;

	ExtraSpawn(const Mod &base, const nlohmann::json &json);

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_EXTRA_HPP
