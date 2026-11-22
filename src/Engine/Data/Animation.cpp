//
// Created by PinkySmile on 10/01/2026.
//

#include "nlohmann/json.hpp"
#include <sol/sol.hpp>
#include "Animation.hpp"
#include "../Resources/Assert.hpp"
#include "../Resources/Game.hpp"


Animation::Animation(const Mod &base, const nlohmann::json &json)
{
	if (!json.contains("sprite"))
		return;

	assert_exp(json["sprite"].is_string());
	assert_exp(json.contains("offset") && json["offset"].is_object());
	assert_exp(json["offset"].contains("x") && json["offset"]["x"].is_number());
	assert_exp(json["offset"].contains("y") && json["offset"]["y"].is_number());
	assert_exp(json.contains("bounds") && json["bounds"].is_object());
	assert_exp(json["bounds"].contains("x") && json["bounds"]["x"].is_number());
	assert_exp(json["bounds"].contains("y") && json["bounds"]["y"].is_number());
	assert_exp(json["bounds"].contains("w") && json["bounds"]["h"].is_number());
	assert_exp(json["bounds"].contains("h") && json["bounds"]["w"].is_number());
	assert_exp(!json.contains("blending") || json["blending"].is_string());
	assert_exp(!json.contains("coloring") || json["coloring"].is_object());
	assert_exp(!json.contains("mirror") || json["mirror"].is_object());

	std::string relPath = RESOURCE(base.getName(), json["sprite"]);

	this->_file = game->modManager.getPath(relPath).second;
	if (json.contains("blending")) {
		if (json["blending"] == "alpha")
			this->_blending = BLEND_ALPHA;
		else if (json["blending"] == "add")
			this->_blending = BLEND_ADD;
		else if (json["blending"] == "sub")
			this->_blending = BLEND_SUB;
		else
			assert_not_reached_msg("Invalid json[\"blending\"]");
	}
	if (json.contains("coloring")) {
		std::string coloring = json["coloring"]["type"];

		if (coloring == "palette") {
			this->_pal = PAL_PALETTE;
			this->_palIndices = json["coloring"]["indices"].get<std::vector<unsigned>>();
			this->_cache = game->textureMgr.getEmbeddedPalette(*this->_file);
			if (!this->_cache)
				this->_pal = PAL_NONE;
		} else if (coloring == "mask") {
			// TODO:
			assert_not_reached_msg("Coloring \"mask\": Not implemented");
		} else if (coloring == "hue") {
			// TODO:
			assert_not_reached_msg("Coloring \"hue\": Not implemented");
		} else if (coloring == "none")
			this->_pal = PAL_NONE;
		else
			assert_not_reached_msg("Invalid coloring");
	}
	if (json.contains("mirror")) {
		assert_exp(json["mirror"].contains("x") && json["mirror"]["x"].is_boolean());
		assert_exp(json["mirror"].contains("y") && json["mirror"]["y"].is_boolean());
		this->_mirror.x = json["mirror"]["x"];
		this->_mirror.y = json["mirror"]["y"];
	}
	if (this->_pal == PAL_NONE) {
		this->_sprite.load(*this->_file);
		this->_requiresPaletteReload = false;
	} else
		this->_requiresPaletteReload = true;
	this->_offset.x = json["offset"]["x"];
	this->_offset.y = json["offset"]["y"];
	this->_bounds.position.x = json["bounds"]["x"];
	this->_bounds.position.y = json["bounds"]["y"];
	this->_bounds.size.x = json["bounds"]["w"];
	this->_bounds.size.y = json["bounds"]["h"];
}

Sprite &Animation::getSprite()
{
	if (this->_requiresPaletteReload)
		this->_reloadPalette();

	IntRect rect = this->_bounds;
	Vector2f pos = this->_offset;
	Vector2f scale;

	if (this->_file && rect.size.x == 0)
		rect.size.x = this->_sprite.getTextureSize().x;
	if (this->_file && rect.size.y == 0)
		rect.size.y = this->_sprite.getTextureSize().y;
	this->_sprite.setTextureRect(rect);
	if (this->_mirror.x) {
		scale.x = -1;
		pos.x += rect.size.x;
	} else
		scale.x = 1;
	if (this->_mirror.y) {
		scale.y = -1;
		pos.y += rect.size.y;
	} else
		scale.y = 1;
	this->_sprite.setScale(scale);
	this->_sprite.setPosition(pos);
	return this->_sprite;
}

void Animation::setColor(const HSLColor &color)
{
	if (!this->_file || this->_pal == PAL_NONE)
		return;
	this->_color = color;
	this->_requiresPaletteReload = true;
}

HSLColor Animation::getColor() const
{
	return this->_color;
}

Animation::Blending Animation::getBlending() const
{
	return this->_blending;
}

Vector2f Animation::getOffset() const
{
	return this->_offset;
}

Vector2u Animation::getSize() const
{
	auto result = Vector2u(this->_bounds.size.x, this->_bounds.size.y);

	if (result.x == 0)
		result.x = this->_sprite.getTextureSize().x;
	if (result.y == 0)
		result.y = this->_sprite.getTextureSize().y;
	return result;
}

void Animation::pushUserType(sol::state &state)
{
	state.new_enum<Animation::Blending>("Animation_Blending", {
		{"ALPHA", BLEND_ALPHA},
		{"ADD",   BLEND_ADD},
		{"SUB",   BLEND_SUB},
	});

	sol::usertype<Animation> type = state.new_usertype<Animation>("Animation");

	type["sprite"] = sol::property(&Animation::getSprite);
	type["color"] = sol::property(&Animation::getColor, &Animation::setColor);
	type["blending"] = sol::property(&Animation::getBlending);
	type["offset"] = sol::property(&Animation::getOffset);
	type["size"] = sol::property(&Animation::getSize);
}

void Animation::_reloadPalette()
{
	this->_requiresPaletteReload = false;
	if (this->_pal == PAL_NONE)
		this->_sprite.load(*this->_file);
	else if (this->_pal == PAL_PALETTE) {
		const auto size = this->_palIndices.size();
		HSLColor mid = this->_color;
		short base = mid.l;

		assert_exp(this->_cache);
		for (size_t i = 0; i < size; i++) {
			auto &result = (*this->_cache)[this->_palIndices[i]];

			mid.l = base * (i + 1) / (size + 1);
			result = mid.toRGB();
		}
		this->_sprite.load(*this->_file, *this->_cache);
	} else
		assert_not_reached();
}

unsigned Animation::preload(HSLColor color) const
{
	if (!this->_file)
		return 0;
	if (this->_pal == PAL_NONE)
		return game->textureMgr.load(*this->_file, nullptr, false, false);
	if (this->_pal == PAL_PALETTE) {
		const auto size = this->_palIndices.size();
		short base = color.l;
		std::array<Color, 256> palette = *this->_cache;

		assert_exp(this->_cache);
		for (size_t i = 0; i < size; i++) {
			auto &result = palette[this->_palIndices[i]];

			color.l = base * (i + 1) / (size + 1);
			result = color.toRGB();
		}
		return game->textureMgr.load(*this->_file, palette, nullptr, false, false);
	}
	assert_not_reached();
}
