//
// Created by PinkySmile on 20/04/2026.
//

#ifndef MIXRTS_UNIT_HPP
#define MIXRTS_UNIT_HPP


#include <vector>
#include "ConditionalAnim.hpp"
#include "GameObject.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"
#include "Vector.hpp"

struct UnitDeployParams {
	struct SpawnBuilding {
		std::string building;
		std::string queue;
		Vector2i offset;
	};

	std::optional<unsigned> direction;
	std::optional<SpawnBuilding> spawnBuilding;
};

class Unit : public GameObject {
private:
	std::optional<UnitDeployParams> _onDeploy;
	unsigned _speed;

protected:
	bool _handleOrder(const Order &o) override;

public:
	Unit(const Unit &u);
	Unit(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &json
	);

	bool canDeploy() const override;
	bool isDeployable() const override;

	const std::optional<UnitDeployParams> &getOnDeploy() const;
	unsigned getSpeed() const;

	bool deploy() override;
};


#endif //MIXRTS_UNIT_HPP
