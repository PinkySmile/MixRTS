//
// Created for the MixRTS test target.
//

#include "MockedUI.hpp"

MockedUI::MockedUI(UI *wrapped) :
	UI(wrapped->getFaction(), wrapped->state, wrapped->manager, wrapped->ownerState, wrapped->map),
	orderQueue(this->_eventQueue),
	_wrapped(wrapped)
{
	// Keep the fields Player/GameManager expect every UI to have set up in
	// sync with the wrapped UI, in case anything ever reads them straight off
	// of this wrapper instead of going through a virtual call.
	this->manager = wrapped->manager;
	this->state = wrapped->state;
	this->ownerState = wrapped->ownerState;
	this->map = wrapped->map;
}

MockedUI::~MockedUI() = default;

void MockedUI::setEnabled(bool enabled)
{
	this->_wrapped->setEnabled(enabled);
	this->_enabled = enabled;
}

void MockedUI::playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args)
{
	this->_wrapped->playEVA(sound, expireIn, priority, cooldown, args);
}

void MockedUI::renderAbove()
{
	this->_wrapped->renderAbove();
}

void MockedUI::updateRender()
{
	this->_wrapped->updateRender();
}

void MockedUI::mouseOnUi()
{
	this->_wrapped->mouseOnUi();
}

void MockedUI::onCancel(Vector2i absPos)
{
	this->_wrapped->onCancel(absPos);
}

void MockedUI::tileClicked(Vector2i absPos)
{
	this->_wrapped->tileClicked(absPos);
}

void MockedUI::mouseMovedToTile(Vector2i absPos)
{
	this->_wrapped->mouseMovedToTile(absPos);
}

void MockedUI::onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta)
{
	this->_wrapped->onKeyPressed(key, ctrl, shift, alt, meta);
}

void MockedUI::update()
{
	this->_wrapped->update();
}

void MockedUI::render()
{
	this->_wrapped->render();
}

void MockedUI::_displaySelectedObjectUI(const GameObject &)
{
	// Unreachable, see the declaration in MockedUI.hpp.
}
