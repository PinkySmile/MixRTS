//
// Created by PinkySmile on 11/04/2026.
//

#ifndef MIXRTS_UPGRADE_HPP
#define MIXRTS_UPGRADE_HPP


#include <string>
#include <optional>
#include <nlohmann/json_fwd.hpp>
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"

class MYDLL_API Upgrade {
public:
	const Faction &_base;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _giveQueue;
	std::string _id;
	std::string _group;
	unsigned _buildSpeed;
	unsigned _limit;
	std::unordered_map<std::string, unsigned> _cost;
	std::shared_ptr<ReqAndCombinator> _requirements;

public:
	Upgrade(const Faction &base, const std::string &id, const nlohmann::json &data);
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;
	[[nodiscard]] const std::optional<std::string> &getGivenQueue() const;
	[[nodiscard]] const std::string &getId() const;
	[[nodiscard]] const std::string &getGroup() const;
	[[nodiscard]] unsigned getBuildSpeed() const;
	[[nodiscard]] const ReqAndCombinator &getRequirements() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getCost() const;
	[[nodiscard]] unsigned getLimit() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_UPGRADE_HPP
