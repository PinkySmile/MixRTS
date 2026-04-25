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
	void setVolume(float volume);
};


#endif //MIXRTS_SOUNDMANAGER_HPP
