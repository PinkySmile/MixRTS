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
	void play() const;
	//! Whether this sound is currently being played (always false if load()
	//! hasn't been called, or the underlying SoundManager never really loads
	//! anything, e.g. MockedSoundManager in MODE_NORMAL/MODE_RECORD).
	[[nodiscard]] bool isPlaying() const;
	//! Whether load() has ever actually been given a valid, non-zero handle.
	[[nodiscard]] bool isLoaded() const;
	void setVolume(float volume);
};


#endif //MIXRTS_SOUND_HPP