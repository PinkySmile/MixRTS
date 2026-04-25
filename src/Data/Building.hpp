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
	const MultiSound *_buildSound;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _giveQueue;
	std::optional<ConditionalAnim> _buildAnim;
	std::optional<ConditionalAnim> _producingAnim;
	std::optional<ConditionalAnim> _producedAnim;
	std::vector<std::vector<uint8_t>> _tiles;
	std::unordered_map<std::string, unsigned> _storage;
	std::set<Vector2i> _checkTiles;
	std::optional<std::string> _proxy;
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
	bool _fake;

public:
	enum BuildingMask {
		MASK_SKIRT              = 1 << 0,
		MASK_SOLID              = 1 << 1,
		MASK_PLACEMENT_REQUIRED = 1 << 2,
		MASK_PLACEMENT_CONCRETE = 2 << 2,
	};


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
	void built();
	void playBuiltSound() const;
	void produced();
	void producing(bool producing);
	int getPower() const;
	bool isFake() const;
	unsigned char getBuildMaxDistance() const;
	bool isSelectable() const;
	bool isSellable() const;
	bool isRepairable() const;
	bool isPrimary() const;
	void setPrimary(bool primary = true);
	const std::optional<std::string> &getBuildQueue() const;
	const std::optional<std::string> &getGivenQueue() const;
	const std::vector<std::vector<uint8_t>> &getTileData() const;
	const std::optional<std::string> &getProxy() const;
	const std::unordered_map<std::string, unsigned> &getStorage() const;
	const std::set<Vector2i> &getCheckTiles() const;
};


#endif //MIXRTS_BUILDING_HPP