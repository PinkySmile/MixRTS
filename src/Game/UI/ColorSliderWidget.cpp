//
// Created by PinkySmile on 26/10/2025.
//

#include <cmath>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <TGUI/Backend/Renderer/SFML-Graphics/BackendRenderTargetSFML.hpp>
#include <utility>
#include "ColorSliderWidget.hpp"
#include "Resources/Assert.hpp"

ColorSliderWidget::ColorSliderWidget(bool dir) :
	tgui::ClickableWidget(ColorSliderWidget::StaticWidgetType),
	_dir(dir)
{
	assert_exp(this->_selector.loadFromFile("assets/selectorBar.png"));
	this->_selectorSprite.setTexture(this->_selector, true);
	this->_selectorSprite.setOrigin({
		std::floor(this->_selector.getSize().x / 2.f),
		std::floor(this->_selector.getSize().y / 2.f)
	});
	this->_sprite.setPosition({0, 0});
}

void ColorSliderWidget::draw(tgui::BackendRenderTarget &target, tgui::RenderStates states) const
{
	auto &sfmlTarget = dynamic_cast<tgui::BackendRenderTargetSFML &>(target);
	const std::array<float, 16> &transformMatrix = states.transform.getMatrix();
	sf::RenderStates statesSFML;
	auto realTarget = sfmlTarget.getTarget();

	tgui::Vector2f size = this->getSize();

	if (size == this->_lastSize || this->_dirty) {
		this->_lastSize = size;
		this->_recreateTexture();
		this->_setPointerPosition();
	}
	statesSFML.transform = sf::Transform(
		transformMatrix[0], transformMatrix[4], transformMatrix[12],
		transformMatrix[1], transformMatrix[5], transformMatrix[13],
		transformMatrix[3], transformMatrix[7], transformMatrix[15]
	);
	statesSFML.coordinateType = sf::CoordinateType::Normalized;
	realTarget->draw(this->_sprite, statesSFML);
	realTarget->draw(this->_selectorSprite, statesSFML);
}

void ColorSliderWidget::_recreateTexture() const
{
	tgui::Vector2f size = this->getSize();
	sf::Vector2u actualSize = {
		static_cast<unsigned int>(std::ceil(size.x)),
		static_cast<unsigned int>(std::ceil(size.y))
	};
	sf::Image image{actualSize};
	HSLColor temp = this->_color;

	this->_dirty = false;
	for (unsigned y = 0; y < actualSize.y; y++)
		for (unsigned x = 0; x < actualSize.x; x++) {
			temp.l = 500 * (this->_dir ? y : x) / (this->_dir ? actualSize.y : actualSize.x);
			image.setPixel({x, y}, temp.toRGB());
		}
	assert_exp(this->_space.loadFromImage(image));
	this->_sprite.setTexture(this->_space, true);
}

ColorSliderWidget::Ptr ColorSliderWidget::create(
	bool dir,
	const tgui::Layout2d &size
)
{
	auto widget = std::make_shared<ColorSliderWidget>(dir);

	widget->setSize(size);
	return widget;
}

void ColorSliderWidget::setColor(HSLColor color)
{
	this->_color = std::move(color);
	// TODO: Check if actually need to recreate texture
	this->_dirty = true;
	this->_setPointerPosition();
}

HSLColor ColorSliderWidget::getColor() const
{
	return this->_color;
}

void ColorSliderWidget::mouseMovedAbs(tgui::Vector2f pos)
{
	if (!this->_dragging)
		return;

	auto size = this->getSize();
	float actualSize = size.y;
	float planePos = pos.y;

	if (planePos < 0)
		planePos = 0;
	if (planePos >= actualSize)
		planePos = actualSize;
	this->_color.l = static_cast<unsigned>(planePos * 1000.f / actualSize);
	this->_setPointerPosition();
	this->onChanged.emit(this, this->_color);
}

void ColorSliderWidget::leftMouseButtonNoLongerDown()
{
	if (this->_dragging)
		this->onChangeEnded.emit(this);
	this->_dragging = false;
	Widget::leftMouseButtonNoLongerDown();
}

bool ColorSliderWidget::leftMousePressed(tgui::Vector2f pos)
{
	this->_dragging = true;
	this->onChangeStarted.emit(this);

	auto size = this->getSize();
	float actualSize = size.y;
	float planePos = pos.y - 10;

	if (planePos < 0)
		planePos = 0;
	if (planePos >= actualSize)
		planePos = actualSize;
	this->_color.l = planePos * 1000.f / actualSize;
	this->_setPointerPosition();
	this->onChanged.emit(this, this->_color);
	return ClickableWidget::leftMousePressed(pos);
}

void ColorSliderWidget::_setPointerPosition() const
{
	sf::Vector2f pos = this->getSize();

	pos.x /= 2;
	pos.y *= this->_color.l / 1000.f;
	pos += this->_sprite.getPosition();
	this->_selectorSprite.setPosition(pos);
}
