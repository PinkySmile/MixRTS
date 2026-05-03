//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_BUILDING_HPP
#define MIXRTS_BUILDING_HPP


#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include "ConditionalAnim.hpp"
#include "Sprite.hpp"
#include "GameObject.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"

class Building : public GameObject {
private:
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _giveQueue;
	std::optional<ConditionalAnim> _buildAnim;
	std::optional<ConditionalAnim> _producingAnim;
	std::optional<ConditionalAnim> _producedAnim;
	std::unordered_map<std::string, unsigned> _storage;
	unsigned _showAfterBuild = 0;
	int _power;
	unsigned char _buildMaxDistance;
	bool _selectable;
	bool _sellable;
	bool _repairable;
	bool _building = false;
	bool _producing = false;
	bool _produced = false;
	bool _isPrimary = false;
	bool _lowerPowerWhenDamaged;

public:
	Building(const Building &b);
	Building(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const nlohmann::json &json
	);
	void update() override;
	void render(int layer) override;
	void setColor(const HSLColor &color) override;
	void built() override;
	void produced();
	void producing(bool producing);
	void setPrimary(bool primary = true);
	[[nodiscard]] int getPower() const;
	[[nodiscard]] unsigned char getBuildMaxDistance() const;
	[[nodiscard]] bool isSelectable() const;
	[[nodiscard]] bool isSellable() const;
	[[nodiscard]] bool isRepairable() const;
	[[nodiscard]] bool isPrimary() const;
	[[nodiscard]] const std::optional<std::string> &getBuildQueue() const;
	[[nodiscard]] const std::optional<std::string> &getGivenQueue() const;
	[[nodiscard]] const std::unordered_map<std::string, unsigned> &getStorage() const;
};


#endif //MIXRTS_BUILDING_HPP