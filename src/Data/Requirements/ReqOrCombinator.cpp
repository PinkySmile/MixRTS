//
// Created by PinkySmile on 10/04/2026.
//

#include <algorithm>
#include <nlohmann/json.hpp>
#include "ReqOrCombinator.hpp"
#include "ReqAndCombinator.hpp"
#include "Req.hpp"
#include "ReqConst.hpp"

ReqOrCombinator::ReqOrCombinator(const std::string &base, const nlohmann::json &value)
{
	for (auto &v : value) {
		if (v.is_array())
			this->_elems.emplace_back(new ReqAndCombinator(base, v));
		else if (v.is_string())
			this->_elems.emplace_back(new Req(base, v));
		else
			this->_elems.emplace_back(new ReqConst(v));
	}
}

bool ReqOrCombinator::operator()(const std::unordered_map<std::string, unsigned> &m) const
{
	return std::ranges::any_of(this->_elems, [&m](const std::unique_ptr<IReq> &c){
		return (*c)(m);
	});
}
