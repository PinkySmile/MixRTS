//
// Created by PinkySmile on 18/09/2021.
//

#include <sys/stat.h>
#include "SoundManager.hpp"
#include "Resources/Game.hpp"
#include "Data/Logger.hpp"

#define MAX_DISTANCE 2048.f

unsigned SoundManager::load(const std::filesystem::path &file, bool directionalSound)
{
	std::string id = file.string() + (directionalSound ? " directional" : " static");

	if (this->_allocatedSounds[id].second != 0) {
		this->_allocatedSounds[id].second++;
		game->logger.verbose("Returning already loaded file " + id);
		return this->_allocatedSounds[id].first;
	}

	std::error_code err;

#ifndef __ANDROID__
	if (std::filesystem::is_directory(file, err)) {
		game->logger.error(file.string() + ": Is a directory");
		return 0;
	}
	if (err) {
		game->logger.error(file.string() + ": Is a directory");
		return 0;
	}
#endif

	unsigned index;

	if (this->_freedIndexes.empty()) {
		this->_lastIndex += this->_lastIndex == 0;
		index = this->_lastIndex;
		this->_lastIndex++;
	} else {
		index = this->_freedIndexes.back();
		this->_freedIndexes.pop_back();
	}

	auto &snd = this->_sounds[index];

	game->logger.debug("Loading sound file " + id);
	if (!snd.buffer.loadFromFile(file))
		game->logger.warn("Failed to sound load " + file.string());
	else
		game->logger.verbose("Loaded sound " + file.string() + " successfully");

	snd.spacial = directionalSound;
	this->_allocatedSounds[id].first = index;
	this->_allocatedSounds[id].second = 1;
	return index;
}

void SoundManager::remove(unsigned int id)
{
	if (!id)
		return;

	for (auto &[name, attr] : this->_allocatedSounds)
		if (attr.first == id && attr.second) {
			attr.second--;
			if (attr.second) {
				game->logger.verbose("Remove ref to " + name);
				return;
			}
			game->logger.debug("Destroying sound " + name);
			break;
		}

	auto it = this->_sounds.find(id);

	if (it == this->_sounds.end())
		return game->logger.warn("Trying to remove invalid index " + std::to_string(id));
	this->_sounds.erase(it);
	this->_freedIndexes.push_back(id);
}

void SoundManager::setCamera(Vector2f pos, Vector2f size)
{
	if (pos == this->_pos && this->_size == size / 2)
		return;
	this->_pos = pos;
	this->_size = size / 2;
	for (auto &pair : this->_sounds | std::ranges::views::values)
		this->_initSoundPos(pair);
}

void SoundManager::_initSoundPos(SoundPair &pair)
{
	auto &snd = pair.sound;

	if (!pair.spacial) {
		snd.setPan(0);
		snd.setVolume(this->_volume * pair.volume);
		return;
	}

	Vector2f pos = pair.pos;
	float volume = 0;

	pos.x -= this->_pos.x;
	pos.y -= this->_pos.y;
	if (pos.x < -this->_size.x - MAX_DISTANCE)
		;
	else if (pos.x < -this->_size.x) {
		snd.setPan(-1);
		volume = 1 + (pos.x + this->_size.x) / MAX_DISTANCE;
	} else if (pos.x <= this->_size.x) {
		snd.setPan(pos.x / this->_size.x);
		volume = 1;
	} else if (pos.x <= this->_size.x + MAX_DISTANCE) {
		snd.setPan(1);
		volume = 1 - (pos.x - this->_size.x) / MAX_DISTANCE;
	}

	if (pos.y < -this->_size.y - MAX_DISTANCE || pos.y > this->_size.y + MAX_DISTANCE)
		volume = 0;
	else if (pos.y < -this->_size.y) {
		volume *= 1 + (pos.y + this->_size.y) / MAX_DISTANCE;
	} else if (pos.y <= this->_size.y) {
	} else if (pos.y <= this->_size.y + MAX_DISTANCE) {
		volume *= 1 - (pos.y - this->_size.y) / MAX_DISTANCE;
	}
	snd.setVolume(volume * this->_volume * pair.volume);
}

void SoundManager::play(unsigned id, float volume, Vector2f pos)
{
	if (id == 0)
		return;
	game->logger.debug("Playing sound " + std::to_string(id));

	auto &snd = this->_sounds.at(id);

	snd.pos = pos;
	snd.volume = volume / 100;
	this->_initSoundPos(snd);
	snd.sound.play();
}

void SoundManager::addRef(unsigned int id)
{
	if (id == 0)
		return;
	for (auto &[name, attr] : this->_allocatedSounds)
		if (attr.first == id && attr.second) {
			if (attr.second < 1)
				return game->logger.warn("Cannot add ref to " + name + " (" + std::to_string(id) + ") because it was unloaded");
			game->logger.verbose("Adding ref to " + name);
			attr.second++;
			return;
		}
	game->logger.warn("Cannot add ref to unknown sound " + std::to_string(id));
}

bool SoundManager::isPlaying(unsigned id) const
{
	if (id == 0)
		return false;
	return this->_sounds.at(id).sound.getStatus() == sf::Sound::Status::Playing;
}

void SoundManager::setVolume(float volume)
{
	this->_volume = volume;
	for (auto &pair : this->_sounds)
		pair.second.sound.setVolume(volume);
}

const sf::SoundBuffer &SoundManager::getBuffer(unsigned id) const
{
	return this->_sounds.at(id).buffer;
}
