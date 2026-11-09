//
// Created by PinkySmile on 10/04/2026.
//

#ifndef MIXRTS_REQORCOMBINATOR_HPP
#define MIXRTS_REQORCOMBINATOR_HPP


#include <list>
#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"
#include "IReq.hpp"

class ReqOrCombinator : public IReq {
private:
	std::vector<std::unique_ptr<IReq>> _elems;

public:
	ReqOrCombinator(const std::string &base, const nlohmann::json &value);
	bool operator()(const std::unordered_map<std::string, unsigned> &) const override;
};


#endif //MIXRTS_REQORCOMBINATOR_HPP
