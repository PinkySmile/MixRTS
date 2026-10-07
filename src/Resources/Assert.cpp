//
// Created by PinkySmile on 10/02/24.
//

#include "Assert.hpp"
#include "Game.hpp"

_AssertionFailedException::_AssertionFailedException(const std::string &&msg) :
	_msg(msg)
{
	std::string f = game->lua["debug"]["traceback"]();

	std::cout << f << std::endl;
}

#ifndef NO_TRACE
_AssertionFailedException::_AssertionFailedException(const std::string &&msg, cpptrace::raw_trace &&trace) :
	lazy_exception(std::move(trace)),
	_msg(msg)
{
}

_AssertionFailedException::_AssertionFailedException(const std::string &&msg, const lazy_exception &parent) :
	lazy_exception(parent),
	_msg(msg)
{
}
#endif

const char *_AssertionFailedException::what() const noexcept
{
	return this->_msg.c_str();
}
