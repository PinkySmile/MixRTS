//
// Created by PinkySmile on 18/09/2021.
//

#ifndef MIXRTS_SPRITE_HPP
#define MIXRTS_SPRITE_HPP


#include <SFML/Graphics.hpp>
#include "Screen.hpp"

struct Sprite : public sf::Sprite {
private:
	unsigned _textureHandle = 0;

public:
	Sprite();
	Sprite(unsigned handle);
	Sprite(const Sprite &o);
	~Sprite() override;
	Sprite &operator=(const Sprite &sprite);

	using sf::Sprite::setTexture;
	void setTexture(unsigned handle, bool resetRect = false);
	void load(const std::filesystem::path &file, Vector2u *size = nullptr, bool repeated = false, bool smooth = false);
	void load(const std::filesystem::path &file, const std::array<Color, 256> &palette, Vector2u *size = nullptr, bool repeated = false, bool smooth = false);
	unsigned getHandle() const;
	Vector2u getTextureSize() const;
	void setTextureRect(IntRect rect);
};


#endif //MIXRTS_SPRITE_HPP
