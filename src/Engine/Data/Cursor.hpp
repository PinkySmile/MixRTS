//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_CURSOR_HPP
#define MIXRTS_CURSOR_HPP


#include <filesystem>
#include <SFML/Graphics.hpp>
#include <nlohmann/json_fwd.hpp>
#include "Vector.hpp"
#include "Resources/MSVCMacros.hpp"

class MYDLL_API Cursor {
private:
	unsigned _duration;
	Vector2u _offset;
	sf::Texture _texture;

public:
	Cursor(const std::filesystem::path &base, const nlohmann::json &json);
	const sf::Texture &getTexture() const;
	const Vector2u &getOffset() const;
	unsigned getDuration() const;
};


#endif //MIXRTS_CURSOR_HPP