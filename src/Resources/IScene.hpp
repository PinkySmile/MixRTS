//
// Created by PinkySmile on 24/09/2021.
//

#ifndef MIXRTS_ISCENE_HPP
#define MIXRTS_ISCENE_HPP


#include <SFML/Window/Event.hpp>

class IScene {
public:
	virtual ~IScene() = default;
	virtual void render() const = 0;
	virtual void update() = 0;
	virtual void consumeEvent(const sf::Event &event) = 0;
};


#endif //MIXRTS_ISCENE_HPP
