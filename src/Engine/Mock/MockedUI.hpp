//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_MOCKEDUI_HPP
#define MIXRTS_MOCKEDUI_HPP


#include <memory>
#include "../UI/UI.hpp"

//! UI implementation used by tests. It owns another, real UI implementation
//! (e.g. D2KUI) given to its constructor and forwards every call to it, so
//! that the wrapped UI keeps behaving normally (updating cursors, playing EVA
//! sounds, rendering its panel, etc).
//!
//! The one exception is pollEvent(): it is NOT forwarded to the wrapped UI.
//! Instead, MockedUI keeps using its own (inherited) UI::_eventQueue -
//! exposed publicly here as _orderQueue - so that tests can inject fake UI
//! events directly. Player::handleUiEvents() will then pick them up and act
//! on them exactly as if a real player had triggered them from the UI.
class MYDLL_API MockedUI : public UI {
public:
	//! Takes ownership of wrapped: every call (other than pollEvent()) made on
	//! this MockedUI is forwarded to it.
	explicit MockedUI(UI *wrapped);
	~MockedUI() override;

	void setEnabled(bool enabled) override;
	void playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "") override;
	void renderAbove() override;
	void updateRender() override;
	void mouseOnUi() override;
	void onCancel(Vector2i absPos) override;
	void tileClicked(Vector2i absPos) override;
	void mouseMovedToTile(Vector2i absPos) override;
	void onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta) override;
	void update() override;
	void render() override;

	//! Fake events pushed here (typically by tests) are what pollEvent() (inherited
	//! from UI, left unforwarded on purpose) will return. This is simply a public
	//! reference to the otherwise-protected UI::_eventQueue.
	std::deque<Event> &orderQueue;

protected:
	//! Never actually reached: render() is forwarded to the wrapped UI's own
	//! render(), which calls the wrapped UI's own _displaySelectedObjectUI()
	//! instead of this one.
	void _displaySelectedObjectUI(const GameObject &object) override;

private:
	std::unique_ptr<UI> _wrapped;
};


#endif //MIXRTS_MOCKEDUI_HPP
