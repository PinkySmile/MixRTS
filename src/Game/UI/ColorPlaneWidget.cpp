//
// Created by PinkySmile on 26/10/2025.
//

#include <cmath>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <TGUI/Backend/Renderer/SFML-Graphics/BackendRenderTargetSFML.hpp>
#include <utility>
#include "ColorPlaneWidget.hpp"
#include "Resources/Assert.hpp"

ColorPlaneWidget::ColorPlaneWidget() :
	tgui::ClickableWidget(ColorPlaneWidget::StaticWidgetType)
{
	assert_exp(this->_cross.loadFromFile("assets/cross.png"));
	this->_sprite.setPosition({0, 0});
	this->_crossSprite.setTexture(this->_cross, true);
	this->_crossSprite.setOrigin({this->_cross.getSize().x / 2.f, this->_cross.getSize().y / 2.f});
}

void ColorPlaneWidget::draw(tgui::BackendRenderTarget &target, tgui::RenderStates states) const
{
	auto &sfmlTarget = dynamic_cast<tgui::BackendRenderTargetSFML &>(target);
	const std::array<float, 16> &transformMatrix = states.transform.getMatrix();
	sf::RenderStates statesSFML;
	auto realTarget = sfmlTarget.getTarget();
	tgui::Vector2f size = this->getSize();

	if (size != this->_lastSize || this->_dirty) {
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
	realTarget->draw(this->_crossSprite, statesSFML);
}

void ColorPlaneWidget::_recreateTexture() const
{
	tgui::Vector2f size = this->getSize();
	sf::Vector2u actualSize = {
		static_cast<unsigned int>(std::ceil(size.x)),
		static_cast<unsigned int>(std::ceil(size.y))
	};
	sf::Image image{actualSize};
	HSLColor temp;

	this->_dirty = false;
	temp.a = 255;
	temp.l = this->_color.l / 2;
	for (unsigned y = 0; y < actualSize.y; y++) {
		temp.s = 1000 * y / actualSize.y;
		for (unsigned x = 0; x < actualSize.x; x++) {
			temp.h = 360 * x / actualSize.x;
			image.setPixel({x, y}, temp.toRGB());
		}
	}
	assert_exp(this->_space.loadFromImage(image));
	this->_sprite.setTexture(this->_space, true);
}

ColorPlaneWidget::Ptr ColorPlaneWidget::create(
	const tgui::Layout2d &size
)
{
	auto widget = std::make_shared<ColorPlaneWidget>();

	widget->setSize(size);
	return widget;
}

void ColorPlaneWidget::setColor(HSLColor color)
{
	auto old = this->_color.l;

	this->_color = std::move(color);
	this->_dirty = this->_dirty || old != this->_color.l;
	this->_setPointerPosition();
	this->onChanged.emit(this, this->_color);
}

HSLColor ColorPlaneWidget::getColor() const
{
	return this->_color;
}

void ColorPlaneWidget::mouseMovedAbs(tgui::Vector2f pos)
{
	if (!this->_dragging)
		return;

	auto size = this->getSize();
	sf::Vector2f actualSize = {
		std::ceil(size.x),
		std::ceil(size.y)
	};
	sf::Vector2f planePos = {
		pos.x,
		pos.y
	};

	if (planePos.x < 0)
		planePos.x = 0;
	if (planePos.y < 0)
		planePos.y = 0;
	if (planePos.x >= actualSize.x)
		planePos.x = actualSize.x;
	if (planePos.y >= actualSize.y)
		planePos.y = actualSize.y;
	this->_color.h = planePos.x * 360.f / actualSize.x;
	this->_color.s = planePos.y * 1000.f / actualSize.y;
	this->_setPointerPosition();
	this->onChanged.emit(this, this->_color);
}

void ColorPlaneWidget::leftMouseButtonNoLongerDown()
{
	if (this->_dragging)
		this->onChangeEnded.emit(this);
	this->_dragging = false;
	Widget::leftMouseButtonNoLongerDown();
}

bool ColorPlaneWidget::leftMousePressed(tgui::Vector2f pos)
{
	this->_dragging = true;
	this->onChangeStarted.emit(this);

	auto size = this->getSize();
	sf::Vector2f actualSize = {
		std::ceil(size.x),
		std::ceil(size.y)
	};
	sf::Vector2f planePos = {
		pos.x - 10,
		pos.y - 10
	};

	if (planePos.x < 0)
		planePos.x = 0;
	if (planePos.y < 0)
		planePos.y = 0;
	if (planePos.x >= actualSize.x)
		planePos.x = actualSize.x;
	if (planePos.y >= actualSize.y)
		planePos.y = actualSize.y;
	this->_color.h = planePos.x * 360.f / actualSize.x;
	this->_color.s = planePos.y * 1000.f / actualSize.y;
	this->_setPointerPosition();
	this->onChanged.emit(this, this->_color);
	return ClickableWidget::leftMousePressed(pos);
}

void ColorPlaneWidget::_setPointerPosition() const
{
	sf::Vector2f pos = this->getSize();

	pos.x *= this->_color.h / 360.f;
	pos.y *= this->_color.s / 1000.f;
	pos += this->_sprite.getPosition();
	this->_crossSprite.setPosition(pos);
}
