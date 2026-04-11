//
// Created by PinkySmile on 10/04/2026.
//

#ifndef MIXRTS_IREQ_HPP
#define MIXRTS_IREQ_HPP


#include <list>
#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"

class IReq {
public:
	virtual ~IReq() = default;
	virtual bool operator()(const std::unordered_map<std::string, std::list<Building *>> &) const = 0;
};


#endif //MIXRTS_IREQ_HPP
