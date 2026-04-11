//
// Created by PinkySmile on 10/04/2026.
//

#include <algorithm>
#include <nlohmann/json.hpp>
#include "ReqAndCombinator.hpp"
#include "ReqOrCombinator.hpp"
#include "Req.hpp"
#include "ReqConst.hpp"

ReqAndCombinator::ReqAndCombinator(const std::string &base, const nlohmann::json &value)
{
	for (auto &v : value) {
		if (v.is_array())
			this->_elems.emplace_back(new ReqOrCombinator(base, v));
		else if (v.is_string())
			this->_elems.emplace_back(new Req(base, v));
		else
			this->_elems.emplace_back(new ReqConst(v));
	}
}

bool ReqAndCombinator::operator()(const std::unordered_map<std::string, std::list<Building *>> &m) const
{
	return std::ranges::all_of(this->_elems, [&m](const std::unique_ptr<IReq> &c){
		return (*c)(m);
	});
}
