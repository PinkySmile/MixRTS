//
// Created by PinkySmile on 10/01/2026.
//

#include "Resources/Game.hpp"
#include "Sound.hpp"

Sound::Sound()
{
}

Sound::Sound(unsigned handle)
{
	this->_handle = handle;
	game->soundMgr.addRef(handle);
}

Sound::Sound(const Sound &o)
{
	this->_volume = o._volume;
	this->_handle = o._handle;
	game->soundMgr.addRef(o._handle);
}

Sound::~Sound()
{
	game->soundMgr.remove(this->_handle);
}

void Sound::load(const std::filesystem::path &file)
{
	if (this->_handle)
		game->soundMgr.remove(this->_handle);
	this->_handle = game->soundMgr.load(file);
}

void Sound::setHandle(unsigned handle)
{
	game->soundMgr.remove(this->_handle);
	this->_handle = handle;
	game->soundMgr.addRef(handle);
}

void Sound::play() const
{
	game->soundMgr.play(this->_handle, this->_volume);
}

bool Sound::isPlaying() const
{
	return game->soundMgr.isPlaying(this->_handle);
}

bool Sound::isLoaded() const
{
	return this->_handle != 0;
}

void Sound::setVolume(float volume)
{
	this->_volume = volume;
}
