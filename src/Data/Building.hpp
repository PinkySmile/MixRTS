//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_BUILDING_HPP
#define MIXRTS_BUILDING_HPP


#include <filesystem>
#include <map>
#include <vector>
#include "ConditionalAnim.hpp"
#include "Sound.hpp"
#include "Sprite.hpp"
#include "Data/Requirements/ReqAndCombinator.hpp"

class Building {
private:
	const Faction &_base;
	mutable Sound _buildSound;
	std::optional<std::string> _buildQueue;
	std::optional<std::string> _giveQueue;
	std::vector<ConditionalAnim> _anims;
	std::optional<ConditionalAnim> _buildAnim;
	std::optional<ConditionalAnim> _producingAnim;
	std::optional<ConditionalAnim> _producedAnim;
	std::vector<std::vector<uint8_t>> _tiles;
	std::shared_ptr<ReqAndCombinator> _requirements;
	std::unordered_map<std::string, unsigned> _storage;
	std::string _id;
	std::string _group;
	IntRect _selectBox;
	Vector2u _size;
	Vector2i _position;
	std::optional<std::string> _proxy;
	unsigned _showAfterBuild = 0;
	unsigned _maxHp;
	unsigned _hp;
	unsigned _sight;
	unsigned _buildSpeed;
	int _power;
	bool _building = false;
	bool _producing = false;
	bool _produced = false;
	bool _lowerPowerWhenDamaged;
	bool _fake;

	void _displayAnim(Animation &anim);

public:
	enum BuildingMask {
		MASK_SKIRT              = 1 << 0,
		MASK_SOLID              = 1 << 1,
		MASK_PLACEMENT_REQUIRED = 1 << 2,
		MASK_PLACEMENT_CONCRETE = 2 << 2,
	};

	struct AnimationState {
		std::string storageKey;
		float health;
	};

	AnimationState animationState;
	PlayerState *playerState = nullptr;


	Building(const Building &b);
	Building(const Faction &base, const std::string &id, const std::map<std::string, std::vector<Animation>> &animList, const nlohmann::json &json);
	void update();
	void render(int layer);
	void built();
	void playBuiltSound() const;
	void produced();
	void producing(bool producing);
	unsigned getHealth() const;
	void heal(unsigned amount);
	void damage(unsigned amount);
	void setPosition(const sf::Vector2i &pos);
	void setColor(const HSLColor &color);
	int getPower() const;
	bool isFake() const;
	bool isDead() const;
	bool isDisposable() const;
	const Vector2u &getSize() const;
	const Vector2i &getPosition() const;
	const std::vector<ConditionalAnim> &getBaseAnims() const;
	const std::optional<std::string> &getBuildQueue() const;
	const std::optional<std::string> &getGivenQueue() const;
	const std::string &getId() const;
	const Mod &getBase() const;
	const Faction &getFaction() const;
	const std::vector<std::vector<uint8_t>> &getTileData() const;
	const std::optional<std::string> &getProxy() const;
	const std::unordered_map<std::string, unsigned> &getStorage() const;
	const ReqAndCombinator &getRequirements() const;
	unsigned getBuildSpeed() const;
	const std::string &getGroup() const;
	bool isObstructed() const;
};


#endif //MIXRTS_BUILDING_HPP