//
// Created by PinkySmile on 11/04/2026.
//

#ifndef MIXRTS_REQCONST_HPP
#define MIXRTS_REQCONST_HPP


#include <list>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"
#include "IReq.hpp"

class ReqConst : public IReq {
private:
	bool _value;

public:
	ReqConst(bool value);
	bool operator()(const std::unordered_map<std::string, std::list<Building *>> &) const override;
};


#endif //MIXRTS_REQCONST_HPP
