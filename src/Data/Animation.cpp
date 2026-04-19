//
// Created by PinkySmile on 10/01/2026.
//

#include "nlohmann/json.hpp"
#include "Animation.hpp"
#include "../Resources/Assert.hpp"
#include "../Resources/Game.hpp"


Animation::Animation(const std::filesystem::path &base, const nlohmann::json &json)
{
	assert_exp(json.contains("sprite"));
	assert_exp(json["sprite"].is_string());
	assert_exp(json.contains("offset"));
	assert_exp(json["offset"].is_object());
	assert_exp(json["offset"].contains("x"));
	assert_exp(json["offset"].contains("y"));
	assert_exp(json["offset"]["x"].is_number());
	assert_exp(json["offset"]["y"].is_number());
	assert_exp(json.contains("bounds"));
	assert_exp(json["bounds"].is_object());
	assert_exp(json["bounds"].contains("x"));
	assert_exp(json["bounds"].contains("y"));
	assert_exp(json["bounds"].contains("w"));
	assert_exp(json["bounds"].contains("h"));
	assert_exp(json["bounds"]["x"].is_number());
	assert_exp(json["bounds"]["y"].is_number());
	assert_exp(json["bounds"]["h"].is_number());
	assert_exp(json["bounds"]["w"].is_number());
	assert_exp(!json.contains("blending") || json["blending"].is_string());
	assert_exp(!json.contains("coloring") || json["coloring"].is_object());

	this->_file = base / json["sprite"].get<std::string>();
	if (json.contains("blending")) {
		if (json["blending"] == "alpha")
			this->_blending = BLEND_ALPHA;
		else if (json["blending"] == "add")
			this->_blending = BLEND_ADD;
		else
			assert_not_reached_msg("Invalid json[\"blending\"]");
	}
	if (json.contains("coloring")) {
		std::string coloring = json["coloring"]["type"];

		if (coloring == "palette") {
			this->_pal = PAL_PALETTE;
			this->_palIndices = json["coloring"]["indices"].get<std::vector<unsigned>>();
			this->_cache = game->textureMgr.getEmbeddedPalette(this->_file);
			if (!this->_cache)
				this->_pal = PAL_NONE;
		} else if (coloring == "mask") {
			// TODO:
			assert_not_reached_msg("Coloring \"mask\": Not implemented");
		} else if (coloring == "hue") {
			// TODO:
			assert_not_reached_msg("Coloring \"hue\": Not implemented");
		} else
			assert_not_reached_msg("Invalid coloring");
	}
	if (this->_pal == PAL_NONE) {
		this->_sprite.load(this->_file);
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

	if (rect.size.x == 0)
		rect.size.x = this->_sprite.getTextureSize().x;
	if (rect.size.y == 0)
		rect.size.y = this->_sprite.getTextureSize().y;
	this->_sprite.setTextureRect(rect);
	this->_sprite.setPosition(this->_offset);
	return this->_sprite;
}

void Animation::setColor(const HSLColor &color)
{
	if (this->_pal == PAL_NONE)
		return;
	this->_color = color;
	this->_requiresPaletteReload = true;
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

void Animation::_reloadPalette()
{
	this->_requiresPaletteReload = false;
	if (this->_pal == PAL_NONE)
		this->_sprite.load(this->_file);
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
		this->_sprite.load(this->_file, *this->_cache);
	} else
		assert_not_reached();
}
