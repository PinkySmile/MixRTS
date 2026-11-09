//
// Created for the MixRTS test target.
//

#include <sstream>
#include <stdexcept>
#include <utility>
#include "MockedScreen.hpp"

#ifdef _WIN32
	// MinGW/MSVC expose the exact same functionality under these names.
	#define popen _popen
	#define pclose _pclose
#endif

MockedScreen::MockedScreen(const std::string &title, MockMode mode, std::filesystem::path videoPath, sf::Vector2u offscreenSize) :
	_mode(mode),
	_videoPath(std::move(videoPath.replace_extension("mkv"))),
	_title(title)
{
	switch (this->_mode) {
	case MODE_DEBUG:
		this->_screen = std::make_unique<Screen>(title);
		break;
	case MODE_NORMAL:
	case MODE_RECORD:
		if (!this->_renderTexture.resize(offscreenSize))
			throw std::runtime_error("MockedScreen: could not create the off-screen render texture");
		this->_renderTexture.clear();
		if (this->_mode == MODE_RECORD)
			this->_startFfmpeg(offscreenSize);
		break;
	}
}

MockedScreen::~MockedScreen()
{
	this->_stopFfmpeg();
}

void MockedScreen::finishRecording()
{
	this->_stopFfmpeg();
}

MockMode MockedScreen::getMode() const
{
	return this->_mode;
}

const std::filesystem::path &MockedScreen::getVideoPath() const
{
	return this->_videoPath;
}

sf::RenderTarget &MockedScreen::getRenderTarget()
{
	if (this->_mode == MODE_DEBUG)
		return *this->_screen;
	return this->_renderTexture;
}

const std::string &MockedScreen::getTitle() const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->getTitle();
	return this->_title;
}

void MockedScreen::setTitle(const std::string &title)
{
	this->_title = title;
	if (this->_mode == MODE_DEBUG)
		this->_screen->setTitle(title);
}

void MockedScreen::setFont(const sf::Font &font)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->setFont(font);
	else
		this->_text.setFont(font);
}

float MockedScreen::getTextSize(const std::string &txt) const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->getTextSize(txt);
	return computeTextWidth(this->_text, txt);
}

void MockedScreen::borderColor(float thickness, const Color &color)
{
	if (this->_mode == MODE_DEBUG) {
		this->_screen->borderColor(thickness, color);
		return;
	}
	this->_rect.setOutlineColor(color);
	this->_text.setOutlineColor(color);
	this->_rect.setOutlineThickness(thickness);
	this->_text.setOutlineThickness(thickness);
}

void MockedScreen::fillColor(const Color &color)
{
	if (this->_mode == MODE_DEBUG) {
		this->_screen->fillColor(color);
		return;
	}
	this->_rect.setFillColor(color);
	this->_text.setFillColor(color);
}

void MockedScreen::textSize(const size_t &size)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->textSize(size);
	else
		this->_text.setCharacterSize(size);
}

void MockedScreen::displayElement(IntRect rect, Color color)
{
	if (this->_mode == MODE_DEBUG) {
		this->_screen->displayElement(rect, color);
		return;
	}
	this->_rect.setPosition(Vector2(rect.position).to<float>());
	this->_rect.setSize(Vector2(rect.size).to<float>());
	this->_rect.setFillColor(color);
	this->draw(this->_rect);
}

void MockedScreen::displayElement(const sf::String &str, sf::Vector2f pos, float boxSize, Screen::TextAlign align)
{
	if (this->_mode == MODE_DEBUG) {
		this->_screen->displayElement(str, pos, boxSize, align);
		return;
	}
	switch (align) {
	case Screen::ALIGN_RIGHT:
		pos.x += boxSize - this->getTextSize(str.toAnsiString());
		break;
	case Screen::ALIGN_CENTER:
		pos.x += (boxSize - this->getTextSize(str.toAnsiString())) / 2;
		break;
	default:
		break;
	}
	this->_text.setPosition(pos);
	this->_text.setString(str);
	this->draw(this->_text);
}

void MockedScreen::displayElement(sf::Sprite &sprite, sf::Vector2f pos)
{
	sprite.setPosition(pos);
	this->displayElement(sprite);
}

void MockedScreen::displayElement(const sf::Sprite &sprite)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->displayElement(sprite);
	else
		this->draw(sprite);
}

void MockedScreen::displayShrunkRect(const PreparedShrunkRect &sprite, sf::IntRect rect)
{
	if (this->_mode == MODE_DEBUG) {
		this->_screen->displayShrunkRect(sprite, rect);
		return;
	}

	sf::Sprite temp{sprite.topLeft.getTexture()};
	int w1 = std::floor(rect.size.x / 2);
	int w2 = rect.size.x - w1;
	int h1 = std::floor(rect.size.y / 2);
	int h2 = rect.size.y - h1;

	temp.setPosition(Vector2(rect.position).to<float>());
	temp.setTexture(sprite.topLeft.getTexture());
	temp.setTextureRect({{0, 0}, {w1, h1}});
	this->draw(temp);

	temp.setPosition(Vector2f(rect.position.x + w1, rect.position.y));
	temp.setTexture(sprite.topRight.getTexture());
	temp.setTextureRect({
		{static_cast<int>(sprite.texSize.x / 2 - w2), 0},
		{w2, h1}
	});
	this->draw(temp);

	temp.setPosition(Vector2f(rect.position.x, rect.position.y + h1));
	temp.setTexture(sprite.bottomLeft.getTexture());
	temp.setTextureRect({
		{0, static_cast<int>(sprite.texSize.y / 2 - h2)},
		{w1, h2}
	});
	this->draw(temp);

	temp.setPosition(Vector2f(rect.position.x + w1, rect.position.y + h1));
	temp.setTexture(sprite.bottomRight.getTexture());
	temp.setTextureRect({
		{static_cast<int>(sprite.texSize.x / 2 - w2), static_cast<int>(sprite.texSize.y / 2 - h2)},
		{w2, h2}
	});
	this->draw(temp);
}

std::unique_ptr<PreparedShrunkRect> MockedScreen::prepareShrunkRect(sf::Sprite &sprite)
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->prepareShrunkRect(sprite);
	return prepareShrunkRectFromSprite(sprite);
}

void MockedScreen::draw(const sf::Drawable &drawable, const sf::RenderStates &states)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->draw(drawable, states);
	else
		this->_renderTexture.draw(drawable, states);
}

sf::View MockedScreen::getView() const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->getView();
	return this->_renderTexture.getView();
}

void MockedScreen::setView(const sf::View &view)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->setView(view);
	else
		this->_renderTexture.setView(view);
}

Vector2u MockedScreen::getSize() const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->getSize();
	return this->_renderTexture.getSize();
}

void MockedScreen::clear(const Color &color)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->clear(color);
	else
		this->_renderTexture.clear(color);
}

void MockedScreen::display()
{
	switch (this->_mode) {
	case MODE_DEBUG:
		this->_screen->display();
		break;
	case MODE_RECORD:
		this->_renderTexture.display();
		this->_pushFrame();
		break;
	case MODE_NORMAL:
		// Nothing is ever read back: let the driver just keep overwriting the
		// same off-screen texture as cheaply as possible.
		break;
	}
}

bool MockedScreen::isOpen() const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->isOpen();
	return this->_open;
}

void MockedScreen::close()
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->close();
	else
		this->_open = false;
}

std::optional<Event> MockedScreen::pollEvent()
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->pollEvent();
	// There is no real window, so there is nothing to poll for.
	return std::nullopt;
}

void MockedScreen::setFramerateLimit(unsigned limit)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->setFramerateLimit(limit);
	// Nothing to rate-limit without a real window/vsync.
}

void MockedScreen::setMouseCursorGrabbed(bool grabbed)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->setMouseCursorGrabbed(grabbed);
}

void MockedScreen::setMouseCursorVisible(bool visible)
{
	if (this->_mode == MODE_DEBUG)
		this->_screen->setMouseCursorVisible(visible);
}

Vector2i MockedScreen::getMousePosition() const
{
	if (this->_mode == MODE_DEBUG)
		return this->_screen->getMousePosition();
	// There is no real window/cursor to report a position for.
	return {-1, -1};
}

void MockedScreen::_startFfmpeg(sf::Vector2u size)
{
	std::ostringstream cmd;

	// Frames are fed as raw, bottom-to-top-free RGBA8 (sf::Image::getPixelsPtr()
	// already returns top-to-bottom rows, matching rawvideo's expected order).
	cmd << "ffmpeg"
		<< " -y"
		<< " -loglevel error"
		<< " -f rawvideo"
		<< " -pixel_format rgba"
		<< " -video_size " << size.x << "x" << size.y
		<< " -framerate " << RECORD_FRAMERATE
		<< " -i -"
		<< " -pix_fmt yuv420p"
		<< " \"" << this->_videoPath.string() << "\"";

	this->_ffmpeg = popen(cmd.str().c_str(), "w");
	if (!this->_ffmpeg)
		throw std::runtime_error("MockedScreen: failed to start ffmpeg (is it installed and in the PATH?)");
}

void MockedScreen::_stopFfmpeg()
{
	if (this->_ffmpeg) {
		pclose(this->_ffmpeg);
		this->_ffmpeg = nullptr;
	}
}

void MockedScreen::_pushFrame()
{
	if (!this->_ffmpeg)
		return;

	sf::Image image = this->_renderTexture.getTexture().copyToImage();
	auto size = image.getSize();

	fwrite(image.getPixelsPtr(), 1, static_cast<size_t>(size.x) * size.y * 4, this->_ffmpeg);
}
