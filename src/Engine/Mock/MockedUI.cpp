//
// Created for the MixRTS test target.
//

#include "MockedUI.hpp"

MockedUI::MockedUI(std::shared_ptr<UI> wrapped) :
	UI(wrapped->getFaction(), wrapped->state, wrapped->manager, wrapped->ownerState, wrapped->map),
	orderQueue(this->_eventQueue),
	_wrapped(std::move(wrapped))
{
	// Keep the fields Player/GameManager expect every UI to have set up in
	// sync with the wrapped UI, in case anything ever reads them straight off
	// of this wrapper instead of going through a virtual call.
	this->manager = this->_wrapped->manager;
	this->state = this->_wrapped->state;
	this->ownerState = this->_wrapped->ownerState;
	this->map = this->_wrapped->map;
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

void MockedUI::onCancel(Vector2i absPos)
{
	this->_wrapped->onCancel(absPos);
}

void MockedUI::tileClicked(Vector2i absPos)
{
	this->_wrapped->tileClicked(absPos);
}

void MockedUI::mouseMoved(Vector2i absPos)
{
	this->_wrapped->mouseMoved(absPos);
}

void MockedUI::mouseReleased(Vector2i absPos)
{
	this->_wrapped->mouseReleased(absPos);
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
