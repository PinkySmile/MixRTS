//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_ANIMATION_HPP
#define MIXRTS_ANIMATION_HPP


#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include "Screen.hpp"
#include "Sprite.hpp"
#include "Vector.hpp"
#include "Utils.hpp"

class Animation {
public:
	enum Blending {
		BLEND_ALPHA,
		BLEND_ADD,
		BLEND_SUB
	};

	Animation(const std::filesystem::path &base, const nlohmann::json &json);
	Sprite &getSprite();
	void setColor(const HSLColor &color);
	Blending getBlending() const;
	Vector2f getOffset() const;
	Vector2u getSize() const;

private:
	enum Paletting {
		PAL_NONE,
		PAL_PALETTE,
		PAL_HUE,
		PAL_MASK
	};

	void _reloadPalette();

	Sprite _sprite;
	Vector2f _offset = {0, 0};
	IntRect _bounds = {0, 0, 0, 0};
	Paletting _pal = PAL_NONE;
	Blending _blending = BLEND_ALPHA;
	HSLColor _color = {0, 0, 0};
	std::optional<std::filesystem::path> _file;
	std::vector<unsigned> _palIndices;
	std::string _palMask;
	bool _requiresPaletteReload = false;


	std::optional<std::array<Color, 256>> _cache;
};


#endif //MIXRTS_ANIMATION_HPP