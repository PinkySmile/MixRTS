//
// Created by PinkySmile on 27/09/2026.
//

#include <nlohmann/json.hpp>
#include "Extra.hpp"
#include "Mod.hpp"
#include "Map.hpp"
#include "Utils.hpp"
#include "Resources/Assert.hpp"

Position::Position(const nlohmann::json &json)
{
	assert_exp(json.contains("x") && (json["x"].is_string() || json["x"].is_number()));
	assert_exp(json.contains("y") && (json["y"].is_string() || json["y"].is_number()));
	this->pos.x = this->_getPos(json["x"]);
	this->pos.y = this->_getPos(json["y"]);
}

float Position::_getPos(const nlohmann::json &json) const
{
	if (json.is_number())
		return json;
	assert_exp(json.is_string());

	std::string pos = json;
	unsigned value = 0x7FFF0000;

	if (pos == "close_border")
		value |= POS_CLOSE_BORDER;
	else if (pos == "far_border")
		value |= POS_FAR_BORDER;
	else if (pos == "min_border")
		value |= POS_MIN_BORDER;
	else if (pos == "max_border")
		value |= POS_MAX_BORDER;
	else if (pos == "close_border_close")
		value |= POS_CLOSE_BORDER_CLOSE;
	else if (pos == "far_border_close")
		value |= POS_FAR_BORDER_CLOSE;
	else if (pos == "min_border_close")
		value |= POS_MIN_BORDER_CLOSE;
	else if (pos == "max_border_close")
		value |= POS_MAX_BORDER_CLOSE;
	else if (pos == "close_border_far")
		value |= POS_CLOSE_BORDER_FAR;
	else if (pos == "far_border_far")
		value |= POS_FAR_BORDER_FAR;
	else if (pos == "min_border_far")
		value |= POS_MIN_BORDER_FAR;
	else if (pos == "max_border_far")
		value |= POS_MAX_BORDER_FAR;
	else
		assert_not_reached_msg(pos);
	return *reinterpret_cast<float *>(&value);
}

float Position::_getPos(float base, float target, float border, float extraOffset) const
{
	if (!std::isnan(base))
		return target + base;

	unsigned type = *reinterpret_cast<unsigned *>(&base) & 0xFFFF;

	switch (type) {
	case POS_CLOSE_BORDER:
	case POS_CLOSE_BORDER_FAR:
	case POS_CLOSE_BORDER_CLOSE:
		if (target + 1 < border - target)
			return -extraOffset;
		return border + extraOffset;
	case POS_FAR_BORDER:
	case POS_FAR_BORDER_FAR:
	case POS_FAR_BORDER_CLOSE:
		if (target + 1 > border - target)
			return -extraOffset;
		return border + extraOffset;
	case POS_MIN_BORDER:
	case POS_MIN_BORDER_FAR:
	case POS_MIN_BORDER_CLOSE:
		return -extraOffset;
	case POS_MAX_BORDER:
	case POS_MAX_BORDER_FAR:
	case POS_MAX_BORDER_CLOSE:
		return border + extraOffset;
	default:
		assert_not_reached_msg(std::to_string(type));
	}
}

Vector2f Position::getPosition(const Vector2f &target, const MapState &map, Vector2f extraOffset) const
{
	Vector2f result = this->pos;
	auto rxMode = *reinterpret_cast<unsigned *>(&result.x) & 0xFFFF;

	result.x = this->_getPos(result.x, target.x, map.gridSize.x * map.tileSize.x, extraOffset.x);
	result.y = this->_getPos(result.y, target.y, map.gridSize.y * map.tileSize.y, extraOffset.y);
	if (!std::isnan(result.x))
		;
	else if (rxMode >= POS_CLOSE_BORDER_FAR) {
		Vector2f result2 = this->pos;

		result2.x = this->_getPos(this->pos.y, target.x, map.gridSize.x * map.tileSize.x, extraOffset.x);
		result2.y = this->_getPos(this->pos.x, target.y, map.gridSize.y * map.tileSize.y, extraOffset.y);
		if (target.distance2(result) < target.distance2(result2))
			return result2;
	} else if (rxMode >= POS_CLOSE_BORDER_CLOSE) {
		Vector2f result2 = this->pos;

		result2.x = this->_getPos(this->pos.y, target.x, map.gridSize.x * map.tileSize.x, extraOffset.x);
		result2.y = this->_getPos(this->pos.x, target.y, map.gridSize.y * map.tileSize.y, extraOffset.y);
		if (target.distance2(result) > target.distance2(result2))
			return result2;
	}
	return result;
}

ExtraSpawn::ExtraSpawn(const Mod &base, const nlohmann::json &json)
{
	assert_exp(json.contains("unit") && json["unit"].is_string());
	assert_exp(!json.contains("delivery") || json["delivery"].is_array());
	assert_exp(!json.contains("invincible") || json["invincible"].is_boolean());

	this->unit = RESOURCE(base.getName(), json["unit"]);
	this->pos = Position{json["pos"]};
	this->invincible = json.contains("invincible") && json["invincible"];
	if (json.contains("dest"))
		this->dest = Position{json["dest"]};
	if (json.contains("out"))
		this->out = Position{json["out"]};
	if (json.contains("carry"))
		for (auto &d : json["carry"])
			this->carry.emplace_back(base, d);
}