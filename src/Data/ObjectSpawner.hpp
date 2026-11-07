//
// Created by PinkySmile on 07/11/2026.
//

#ifndef MIXRTS_OBJECTSPAWNER_HPP
#define MIXRTS_OBJECTSPAWNER_HPP


#include "GameObject.hpp"

class ObjectSpawner : public GameObject {
private:
	std::string _name;
	std::function<void (GameObject *)> _init;

public:
	ObjectSpawner(const std::string &objName, unsigned count, unsigned delay, const std::function<void (GameObject *)> &initObject);
	~ObjectSpawner() override = default;
	void update() override;
	void render(int layer) override;
};


#endif //MIXRTS_OBJECTSPAWNER_HPP
