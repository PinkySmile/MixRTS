//
// Created by PinkySmile on 07/11/2026.
//

#ifndef MIXRTS_OBJECTSPAWNER_HPP
#define MIXRTS_OBJECTSPAWNER_HPP


#include "GameObject.hpp"

struct ObjectSpawnerRestoreTag {};

class MYDLL_API ObjectSpawner : public GameObject {
private:
	std::string _name;
	std::vector<Order> _initOrders;

public:
	ObjectSpawner(const std::string &objName, unsigned count, unsigned delay, std::vector<Order> initOrders);
	explicit ObjectSpawner(ObjectSpawnerRestoreTag);
	~ObjectSpawner() override = default;
	void update() override;
	void render(int layer) override;
	[[nodiscard]] uint8_t getStateKind() const override { return 1; }
	void saveState(StateSaveContext &ctx) const override;
	void loadState(StateLoadContext &ctx) override;
};


#endif //MIXRTS_OBJECTSPAWNER_HPP
