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

void Sound::load(const std::string &file)
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

void Sound::play()
{
	game->soundMgr.play(this->_handle, this->_volume);
}

void Sound::setVolume(float volume)
{
	this->_volume = volume;
}
