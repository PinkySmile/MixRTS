//
// Created by PinkySmile on 15/01/2019.
//

#ifndef THFGAME_RESSOURCES_HPP
#define THFGAME_RESSOURCES_HPP


#include <memory>
#include <SFML/Graphics.hpp>
#include "Color.hpp"
#include "Data/Vector.hpp"

#define EVENT_WINDOW_CLOSED sf::Event::Closed

typedef sf::Event Event;
typedef sf::View ViewPort;

struct PreparedShrunkRect {
	Vector2u texSize;
	sf::RenderTexture topLeft;
	sf::RenderTexture topRight;
	sf::RenderTexture bottomLeft;
	sf::RenderTexture bottomRight;
};

struct IntRect : public sf::IntRect {
	IntRect();
	IntRect(Vector2i pos, Vector2i size);
	IntRect(int x, int y, int w, int h);
};

//! Pure helper shared by Screen and MockedScreen: width in pixels that txt would
//! take when rendered with text's current font/character size.
float computeTextWidth(const sf::Text &text, const std::string &txt);

//! Pure helper shared by Screen and MockedScreen: slices sprite (which must be
//! positioned at the origin and have an odd-sized texture) into 4 overlapping
//! corner textures, used to later draw a "shrunk" (9-slice-like) rect of any
//! size via displayShrunkRect.
std::unique_ptr<PreparedShrunkRect> prepareShrunkRectFromSprite(sf::Sprite &sprite);

class Screen : public sf::RenderWindow {
private:
	sf::Font           _font;
	sf::RectangleShape _rect;
	sf::Text           _text{ this->_font };
	sf::Clock          _clock;
	sf::Texture        _texture;
	sf::Sprite         _sprite{ this->_texture };
	std::string        _title;

public:
	explicit Screen(const std::string &title = "FG");
	Screen(const Screen &);
	~Screen() override;

	enum TextAlign {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	float getTextSize(const std::string &txt) const;
	const std::string &getTitle() const;
	void setTitle(const std::string &);
	void borderColor(float thickness = 0, const Color &color = Color(0, 0, 0, 255));
	void fillColor(const Color &color = Color(255, 255, 255, 255));
	void setFont(const sf::Font &font);
	void textSize(const size_t &size);
	void displayElement(IntRect rect, Color color);
	void displayElement(const sf::String &str, sf::Vector2f pos, float size = 0, TextAlign = ALIGN_LEFT);
	void displayElement(sf::Sprite &sprite, sf::Vector2f);
	void displayElement(const sf::Sprite &sprite);
	void displayShrunkRect(const PreparedShrunkRect &sprite, sf::IntRect rect);
	std::unique_ptr<PreparedShrunkRect> prepareShrunkRect(sf::Sprite &sprite);

	//! Mouse position relative to this window. Exists (rather than using
	//! sf::Mouse::getPosition directly) so that engine code can be written once
	//! and work the same whether SCREEN_CLASS_IMPLEM is Screen or MockedScreen.
	[[nodiscard]] Vector2i getMousePosition() const;
	//! The sf::RenderTarget actually being drawn to (this, for a real Screen).
	//! Exists for the same reason as getMousePosition, e.g. to build a tgui::Gui.
	[[nodiscard]] sf::RenderTarget &getRenderTarget();
};

void libraryInit();
#define libraryUnInit()

#endif //THFGAME_RESSOURCES_HPP
