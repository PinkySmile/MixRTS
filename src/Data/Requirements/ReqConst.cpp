//
// Created by PinkySmile on 11/04/2026.
//

#include "ReqConst.hpp"

ReqConst::ReqConst(bool value) :
	_value(value)
{
}

bool ReqConst::operator()(const std::unordered_map<std::string, std::list<Building *>> &) const
{
	return this->_value;
}
