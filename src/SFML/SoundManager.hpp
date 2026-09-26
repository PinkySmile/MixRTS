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
		sf::Sound sound{this->buffer};
		Vector2f pos;
		float volume;
		bool spacial;
	};

	float _volume = 100;
	unsigned _lastIndex = 0;
	Vector2f _pos;
	Vector2f _size;
	std::vector<unsigned> _freedIndexes;
	std::unordered_map<unsigned, SoundPair> _sounds;
	std::unordered_map<std::string, std::pair<unsigned, unsigned>> _allocatedSounds;

	void _initSoundPos(SoundPair &pair);
public:
	SoundManager() = default;

	unsigned load(const std::filesystem::path &file, bool directionalSound);
	void addRef(unsigned id);
	void remove(unsigned id);
	void setCamera(Vector2f pos, Vector2f size);
	void play(unsigned id, float volume, Vector2f pos);
	bool isPlaying(unsigned id) const;
	void setVolume(float volume);
	const sf::SoundBuffer &getBuffer(unsigned id) const;
};


#endif //MIXRTS_SOUNDMANAGER_HPP
