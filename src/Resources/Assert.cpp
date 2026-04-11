//
// Created by PinkySmile on 10/02/24.
//

#include "Assert.hpp"
#include "Game.hpp"

_AssertionFailedException::_AssertionFailedException(const std::string &&msg) :
	_msg(msg)
{
	game->logger.fatal("AssertionFailedException: " + msg);
}

const char *_AssertionFailedException::what() const noexcept
{
	return this->_msg.c_str();
}
