//
// Created by PinkySmile on 04/10/2026.
//

#include <sol/sol.hpp>
#include <TGUI/Widget.hpp>
#include <TGUI/Container.hpp>
#include <TGUI/Signal.hpp>
#include <TGUI/Texture.hpp>
#include <TGUI/Widgets/ClickableWidget.hpp>
#include <TGUI/Widgets/Panel.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/Scrollbar.hpp>
#include <TGUI/Widgets/Button.hpp>
#include <TGUI/Widgets/Picture.hpp>
#include <TGUI/Renderers/WidgetRenderer.hpp>
#include <TGUI/Renderers/PictureRenderer.hpp>
#include "TGUILua.hpp"
#include "Data/Vector.hpp"

//! Builds a tgui::Layout out of either a Lua number (constant pixel value)
//! or a Lua string (a layout expression, e.g. "50%" or "&.w - 5").
static tgui::Layout toLayout(const sol::object &obj)
{
	if (obj.is<std::string>())
		return tgui::Layout{obj.as<std::string>()};
	return tgui::Layout{obj.as<float>()};
}

void pushTGUIUserTypes(sol::state &state)
{
	sol::usertype<tgui::WidgetRenderer> rendererType = state.new_usertype<tgui::WidgetRenderer>("WidgetRenderer");

	rendererType["opacity"] = sol::property(&tgui::WidgetRenderer::getOpacity, &tgui::WidgetRenderer::setOpacity);
	rendererType["opacity_disabled"] = sol::property(&tgui::WidgetRenderer::getOpacityDisabled, &tgui::WidgetRenderer::setOpacityDisabled);

	sol::usertype<tgui::PictureRenderer> pictureRendererType = state.new_usertype<tgui::PictureRenderer>("PictureRenderer",
		sol::base_classes, sol::bases<tgui::WidgetRenderer>()
	);

	pictureRendererType["texture"] = sol::property(&tgui::PictureRenderer::getTexture, &tgui::PictureRenderer::setTexture);

	sol::usertype<tgui::Signal> signalType = state.new_usertype<tgui::Signal>("Signal");

	// Ignores the (optional) typed parameter some signals provide (e.g. the
	// mouse position for onMousePress): none of the engine's own usages need
	// it, callbacks are always registered as plain argument-less functions.
	//
	// sol::function is sol::protected_function under SOL_SAFE_FUNCTION_OBJECTS
	// (turned on by SOL_ALL_SAFETIES_ON), so calling it catches Lua errors
	// internally instead of letting them propagate; the result must be
	// checked explicitly or a Lua-side error() in a callback would be
	// silently swallowed.
	signalType["connect"] = [](tgui::Signal &signal, const sol::function &fn) {
		return signal.connect([fn]{
			sol::protected_function_result result = fn();

			if (!result.valid()) {
				sol::error err = result;
				throw err;
			}
		});
	};
	signalType["disconnect"] = &tgui::Signal::disconnect;
	signalType["disconnect_all"] = &tgui::Signal::disconnectAll;

	sol::usertype<tgui::Widget> widgetType = state.new_usertype<tgui::Widget>("Widget");

	widgetType["position"] = sol::property(
		[](const tgui::Widget &w) { return Vector2f(w.getPosition()); },
		[](tgui::Widget &w, const Vector2<sol::object> &s) { w.setPosition(toLayout(s.x), toLayout(s.y)); }
	);
	widgetType["size"] = sol::property(
		[](const tgui::Widget &w) { return Vector2f(w.getSize()); },
		[](tgui::Widget &w, const Vector2<sol::object> &s) { w.setSize(toLayout(s.x), toLayout(s.y)); }
	);
	widgetType["visible"] = sol::property(&tgui::Widget::isVisible, &tgui::Widget::setVisible);
	widgetType["enabled"] = sol::property(&tgui::Widget::isEnabled, &tgui::Widget::setEnabled);
	widgetType["renderer"] = sol::property(&tgui::Widget::getRenderer, &tgui::Widget::setRenderer);

	sol::usertype<tgui::Container> containerType = state.new_usertype<tgui::Container>("Container",
		sol::base_classes, sol::bases<tgui::Widget>()
	);

	containerType["add"] = sol::overload(
		[](tgui::Container &c, const tgui::Widget::Ptr &widget, const std::string &name) {
			c.add(widget, name);
		}
	);
	containerType["remove_all_widgets"] = &tgui::Container::removeAllWidgets;
	containerType["load_widgets_from_file"] = [](tgui::Container &c, const std::string &filename) {
		c.loadWidgetsFromFile(filename);
	};
	containerType["get_widget"] = [](const tgui::Container &c, const std::string &name) {
		return c.get(name);
	};
	containerType["get_button"] = [](const tgui::Container &c, const std::string &name) {
		return c.get<tgui::Button>(name);
	};
	containerType["get_panel"] = [](const tgui::Container &c, const std::string &name) {
		return c.get<tgui::Panel>(name);
	};
	containerType["get_picture"] = [](const tgui::Container &c, const std::string &name) {
		return c.get<tgui::Picture>(name);
	};
	containerType["get_scrollable_panel"] = [](const tgui::Container &c, const std::string &name) {
		return c.get<tgui::ScrollablePanel>(name);
	};

	sol::usertype<tgui::ClickableWidget> clickableType = state.new_usertype<tgui::ClickableWidget>("ClickableWidget",
		sol::base_classes, sol::bases<tgui::Widget>()
	);

	clickableType["on_mouse_press"] = sol::property([](tgui::ClickableWidget &w) -> tgui::Signal & { return w.onMousePress; });
	clickableType["on_click"] = sol::property([](tgui::ClickableWidget &w) -> tgui::Signal & { return w.onClick; });
	clickableType["on_right_mouse_press"] = sol::property([](tgui::ClickableWidget &w) -> tgui::Signal & { return w.onRightMousePress; });
	// emit() lets scripts simulate a click/press from code, like the engine
	// itself does (e.g. D2KUI's "Buy" button programmatically re-triggering
	// "Main2"'s onMousePress to switch back to the build menu).
	clickableType["emit_mouse_press"] = [](tgui::ClickableWidget &w) { w.onMousePress.emit(&w, tgui::Vector2f{}); };
	clickableType["emit_click"] = [](tgui::ClickableWidget &w) { w.onClick.emit(&w, tgui::Vector2f{}); };
	clickableType["emit_right_mouse_press"] = [](tgui::ClickableWidget &w) { w.onRightMousePress.emit(&w, tgui::Vector2f{}); };

	sol::usertype<tgui::Panel> panelType = state.new_usertype<tgui::Panel>("Panel",
		sol::base_classes, sol::bases<tgui::Container, tgui::Widget>(),
		"create", sol::overload(
			[]() { return tgui::Panel::create(); },
			[](float w, float h) { return tgui::Panel::create({w, h}); }
		)
	);

	sol::usertype<tgui::ScrollbarAccessor> scrollbarType = state.new_usertype<tgui::ScrollbarAccessor>("ScrollbarAccessor");

	scrollbarType["value"] = sol::property(&tgui::ScrollbarAccessor::getValue, &tgui::ScrollbarAccessor::setValue);
	scrollbarType["scroll_amount"] = sol::property(&tgui::ScrollbarAccessor::getScrollAmount, &tgui::ScrollbarAccessor::setScrollAmount);
	scrollbarType["max_value"] = sol::property(&tgui::ScrollbarAccessor::getMaxValue);

	sol::usertype<tgui::ScrollablePanel> scrollablePanelType = state.new_usertype<tgui::ScrollablePanel>("ScrollablePanel",
		sol::base_classes, sol::bases<tgui::Panel, tgui::Container, tgui::Widget>(),
		"create", sol::overload(
			[]() { return tgui::ScrollablePanel::create(); },
			[](float w, float h) { return tgui::ScrollablePanel::create({w, h}); }
		)
	);

	scrollablePanelType["content_offset"] = sol::property([](const tgui::ScrollablePanel &p) { return Vector2f(p.getContentOffset()); });
	scrollablePanelType["scrolled"] = [](tgui::ScrollablePanel &p, float delta) { return p.scrolled(delta, {0, 0}, false); };
	scrollablePanelType["horizontal_scrollbar"] = sol::property(sol::resolve<tgui::ScrollbarAccessor *()>(&tgui::ScrollablePanel::getHorizontalScrollbar));
	scrollablePanelType["vertical_scrollbar"] = sol::property(sol::resolve<tgui::ScrollbarAccessor *()>(&tgui::ScrollablePanel::getVerticalScrollbar));

	sol::usertype<tgui::Button> buttonType = state.new_usertype<tgui::Button>("Button",
		sol::base_classes, sol::bases<tgui::ClickableWidget, tgui::Widget>(),
		"create", [](const std::string &text) { return tgui::Button::create(text); }
	);

	sol::usertype<tgui::Picture> pictureType = state.new_usertype<tgui::Picture>("Picture",
		sol::base_classes, sol::bases<tgui::ClickableWidget, tgui::Widget>(),
		"create", []() { return tgui::Picture::create(); }
	);

	// Shadows Widget::get_renderer with the covariant PictureRenderer*
	// overload so scripts get a renderer with set_texture available.
	pictureType["renderer"] = sol::property(&tgui::Picture::getRenderer);

	sol::usertype<tgui::Texture> textureType = state.new_usertype<tgui::Texture>("Texture",
		sol::constructors<tgui::Texture(), tgui::Texture(const std::string &)>()
	);

	textureType["image_size"] = sol::property([](const tgui::Texture &t) { return Vector2u(t.getImageSize()); });
}
