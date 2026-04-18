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
	float _volume = 100;

public:
	Sound();
	Sound(unsigned handle);
	Sound(const Sound &o);
	~Sound();
	void load(const std::filesystem::path &file);
	void setHandle(unsigned handle);
	void play();
	void setVolume(float volume);
};


#endif //MIXRTS_SOUND_HPP