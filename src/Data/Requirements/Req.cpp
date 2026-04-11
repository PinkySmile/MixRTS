//
// Created by PinkySmile on 10/04/2026.
//

#include <utility>
#include "Req.hpp"
#include "Utils.hpp"

Req::Req(const std::string &base, const std::string &value) :
	_id(RESOURCE(base, value))
{
}

bool Req::operator()(const std::unordered_map<std::string, std::list<Building *>> &m) const
{
	return m.contains(this->_id) && !m.at(this->_id).empty();
}
