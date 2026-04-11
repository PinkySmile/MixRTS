//
// Created by PinkySmile on 10/04/2026.
//

#ifndef MIXRTS_REQ_HPP
#define MIXRTS_REQ_HPP


#include <list>
#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"
#include "IReq.hpp"

class Req : public IReq {
private:
	std::string _id;

public:
	Req(const std::string &base, const std::string &value);
	bool operator()(const std::unordered_map<std::string, std::list<Building *>> &) const override;
};


#endif //MIXRTS_REQ_HPP
