//
// Created by PinkySmile on 26/10/2025.
//

#ifndef MIXRTSCOLORPLANEWIDGET_HPP
#define MIXRTSCOLORPLANEWIDGET_HPP


#include <array>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <TGUI/Widgets/ClickableWidget.hpp>
#include "Utils.hpp"

class ColorPlaneWidget : public tgui::ClickableWidget {
public:
	using Ptr = std::shared_ptr<ColorPlaneWidget>; //!< Shared widget pointer
	using ConstPtr = std::shared_ptr<const ColorPlaneWidget>; //!< Shared constant widget pointer

	static constexpr const char StaticWidgetType[] = "ColorPlaneWidget"; //!< Type name of the widget

	ColorPlaneWidget();
	void draw(tgui::BackendRenderTarget &target, tgui::RenderStates states) const override;
	void setColor(HSLColor color);
	HSLColor getColor() const;

	void mouseMovedAbs(tgui::Vector2f pos);
	void leftMouseButtonNoLongerDown() override;
	bool leftMousePressed(tgui::Vector2f pos) override;

	TGUI_NODISCARD static ColorPlaneWidget::Ptr create(const tgui::Layout2d &size = {"100%", "100%"});

	tgui::SignalTyped<HSLColor> onChanged = {"Changed"};
	tgui::Signal onChangeStarted = {"ChangedStarted"};
	tgui::Signal onChangeEnded = {"ChangedEnded"};

private:
	HSLColor _color = {0, 0, 0};
	mutable tgui::Vector2f _lastSize{0, 0};
	mutable sf::Texture _space;
	mutable sf::Sprite _sprite{this->_space};
	mutable sf::Texture _cross;
	mutable sf::Sprite _crossSprite{this->_cross};
	bool _dragging = false;
	mutable bool _dirty = false;

	void _setPointerPosition() const;
	void _recreateTexture() const;
};


#endif //MIXRTSCOLORPLANEWIDGET_HPP
