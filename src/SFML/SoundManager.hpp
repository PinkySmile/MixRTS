//
// Created by PinkySmile on 18/09/2021.
//

#ifndef MIXRTS_SOUNDMANAGER_HPP
#define MIXRTS_SOUNDMANAGER_HPP


#include <unordered_map>
#include <string>
#include <SFML/Audio.hpp>
#include "Sprite.hpp"
#include "Data/Vector.hpp"
#include "Color.hpp"

class SoundManager {
private:
	struct SoundPair {
		sf::SoundBuffer buffer;
		mutable sf::Sound sound{this->buffer};
	};

	float _volume = 100;
	unsigned _lastIndex = 0;
	std::vector<unsigned> _freedIndexes;
	std::unordered_map<unsigned, SoundPair> _sounds;
	std::unordered_map<std::filesystem::path, std::pair<unsigned, unsigned>> _allocatedSounds;

public:
	SoundManager() = default;

	unsigned load(const std::filesystem::path &file);
	void addRef(unsigned id);
	void remove(unsigned id);
	void play(unsigned id, float volume) const;
	//! Whether id is currently being played through the audio device (always
	//! false for id == 0, i.e. an unset/unloaded handle).
	bool isPlaying(unsigned id) const;
	void setVolume(float volume);
	//! Exposes the raw samples backing id, e.g. so MockedSoundManager can write
	//! them out to a WAV file in MODE_RECORD without actually playing them
	//! through a real audio device.
	const sf::SoundBuffer &getBuffer(unsigned id) const;
};


#endif //MIXRTS_SOUNDMANAGER_HPP
