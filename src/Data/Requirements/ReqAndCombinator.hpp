//
// Created by PinkySmile on 10/04/2026.
//

#ifndef MIXRTS_REQANDCOMBINATOR_HPP
#define MIXRTS_REQANDCOMBINATOR_HPP


#include <list>
#include <string>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"
#include "IReq.hpp"

class ReqAndCombinator : public IReq {
private:
	std::vector<std::unique_ptr<IReq>> _elems;

public:
	ReqAndCombinator(const std::string &base, const nlohmann::json &value);
	bool operator()(const std::unordered_map<std::string, unsigned> &) const override;
};


#endif //MIXRTS_REQANDCOMBINATOR_HPP
