//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_MOCKEDSCREEN_HPP
#define MIXRTS_MOCKEDSCREEN_HPP


#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>

#include "MockType.hpp"
#include "Screen.hpp"

//! Drop-in replacement for Screen used by the tests target (see SCREEN_CLASS_IMPLEM
//! in Resources/Game.hpp and CMakeLists.txt).
//!
//! Unlike Screen, MockedScreen does NOT derive from sf::RenderWindow. Depending on
//! the Mode it is constructed with, it either owns a real Screen and forwards
//! everything to it (MODE_DEBUG), or draws into an off-screen sf::RenderTexture
//! (MODE_NORMAL / MODE_RECORD). In MODE_RECORD, every displayed frame is additionally
//! read back from that texture and piped, as raw RGBA frames, into an ffmpeg process
//! that encodes them into the video file given to the constructor.
class MockedScreen {
public:
	explicit MockedScreen(
		const std::string &title = "FG",
		MockMode mode = MODE_NORMAL,
		std::filesystem::path videoPath = {},
		sf::Vector2u offscreenSize = {1280, 720}
	);
	MockedScreen(const MockedScreen &) = delete;
	~MockedScreen();

	[[nodiscard]] MockMode getMode() const;
	[[nodiscard]] const std::filesystem::path &getVideoPath() const;
	//! Stops/closes the ffmpeg process started in MODE_RECORD (a no-op
	//! otherwise), finalizing the video file so it can safely be read (e.g.
	//! muxed with the WAV file written by MockedSoundManager, see
	//! tests/main.cpp's mergeRecording()) right away. Also done by the
	//! destructor, but Game (and therefore this MockedScreen) is deliberately
	//! never destroyed/leaked (see GameFixture.hpp), so callers that need the
	//! video file to be complete before process exit must call this
	//! explicitly.
	void finishRecording();

	//! The sf::RenderTarget actually being drawn to: the real Screen's window in
	//! MODE_DEBUG, or the internal off-screen texture otherwise. Used e.g. to
	//! build a tgui::Gui.
	[[nodiscard]] sf::RenderTarget &getRenderTarget();

	// Screen-like helpers. In MODE_DEBUG these forward to the real Screen; otherwise
	// they operate on/through the off-screen texture (see .cpp for details).
	[[nodiscard]] const std::string &getTitle() const;
	void setTitle(const std::string &title);
	void setFont(const sf::Font &font);
	[[nodiscard]] float getTextSize(const std::string &txt) const;
	void borderColor(float thickness = 0, const Color &color = Color(0, 0, 0, 255));
	void fillColor(const Color &color = Color(255, 255, 255, 255));
	void textSize(const size_t &size);
	void displayElement(IntRect rect, Color color);
	void displayElement(const sf::String &str, sf::Vector2f pos, float size = 0, Screen::TextAlign align = Screen::ALIGN_LEFT);
	void displayElement(sf::Sprite &sprite, sf::Vector2f pos);
	void displayElement(const sf::Sprite &sprite);
	void displayShrunkRect(const PreparedShrunkRect &sprite, sf::IntRect rect);
	std::unique_ptr<PreparedShrunkRect> prepareShrunkRect(sf::Sprite &sprite);

	// sf::RenderWindow-like interface used throughout the engine.
	void draw(const sf::Drawable &drawable, const sf::RenderStates &states = sf::RenderStates::Default);
	[[nodiscard]] sf::View getView() const;
	void setView(const sf::View &view);
	[[nodiscard]] Vector2u getSize() const;
	void clear(const Color &color = Color::Black);
	//! Finalizes the current frame. In MODE_RECORD this is also where the frame is
	//! read back and pushed to ffmpeg.
	void display();
	[[nodiscard]] bool isOpen() const;
	void close();
	std::optional<Event> pollEvent();
	void setFramerateLimit(unsigned limit);
	void setMouseCursorGrabbed(bool grabbed);
	void setMouseCursorVisible(bool visible);
	[[nodiscard]] Vector2i getMousePosition() const;

private:
	void _startFfmpeg(sf::Vector2u size);
	void _stopFfmpeg();
	void _pushFrame();

	MockMode _mode;
	std::filesystem::path _videoPath;
	std::string _title;
	bool _open = true;

	std::unique_ptr<Screen> _screen; //!< Only set in MODE_DEBUG.
	sf::RenderTexture _renderTexture; //!< Used in MODE_NORMAL / MODE_RECORD.

	sf::Font _font;
	sf::RectangleShape _rect;
	sf::Text _text{this->_font};

	FILE *_ffmpeg = nullptr; //!< Only set in MODE_RECORD.
};


#endif //MIXRTS_MOCKEDSCREEN_HPP
