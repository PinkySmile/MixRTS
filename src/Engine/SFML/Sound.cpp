//
// Created by PinkySmile on 10/01/2026.
//

#include <sol/sol.hpp>
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

void Sound::load(const std::filesystem::path &file, bool relativeToCamera)
{
	if (this->_handle)
		game->soundMgr.remove(this->_handle);
	this->_handle = game->soundMgr.load(file, relativeToCamera);
}

void Sound::setHandle(unsigned handle)
{
	game->soundMgr.remove(this->_handle);
	this->_handle = handle;
	game->soundMgr.addRef(handle);
}

void Sound::play(Vector2f pos) const
{
	game->soundMgr.play(this->_handle, this->_volume, pos);
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

void Sound::pushUserType(sol::state &state)
{
	sol::usertype<Sound> type = state.new_usertype<Sound>("Sound",
		sol::constructors<Sound(), Sound(unsigned), Sound(const Sound &)>()
	);

	type["load"] = [](Sound &sound, const std::string &file, bool relativeToCamera) {
		sound.load(file, relativeToCamera);
	};
	type["play"] = sol::overload(
		[](const Sound &sound) { sound.play(); },
		sol::resolve<void(Vector2f) const>(&Sound::play)
	);
	type["is_playing"] = sol::property(&Sound::isPlaying);
	type["is_loaded"] = sol::property(&Sound::isLoaded);
	type["volume"] = sol::writeonly_property(&Sound::setVolume);
}
