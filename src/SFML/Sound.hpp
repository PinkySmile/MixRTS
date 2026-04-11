//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_SOUND_HPP
#define MIXRTS_SOUND_HPP


#include <string>
#include "forward.hpp"

class Sound {
private:
	unsigned _handle = 0;

public:
	Sound();
	Sound(unsigned handle);
	Sound(const Sound &o);
	~Sound();
	void load(const std::string &file);
	void setHandle(unsigned handle);
	void play();
};


#endif //MIXRTS_SOUND_HPP