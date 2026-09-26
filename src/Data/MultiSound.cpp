//
// Created by PinkySmile on 22/04/2026.
//

#include "MultiSound.hpp"

#include <cpptrace/from_current.hpp>
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"

void MultiSound::addObject(const std::filesystem::path &base, const nlohmann::json &json)
{
	assert_exp(json.is_object());
	assert_exp(json.contains("path") && json["path"].is_string());
	assert_exp(!json.contains("localized") || json["localized"].is_boolean());
	assert_exp(!json.contains("volume") || json["volume"].is_number());

	std::string path = json["path"];

	for (size_t pos = path.find("<locale>"); pos != std::string::npos; pos = path.find("<locale>"))
		path.replace(pos, strlen("<locale>"), game->locale);

	this->_sounds.emplace_back();
	if (json.contains("volume"))
		this->_sounds.back().setVolume(json["volume"]);
	this->_sounds.back().load(base / path, !json.contains("localized") || json["localized"]);
}

void MultiSound::init(const std::filesystem::path &base, const nlohmann::json &json)
{
	assert_exp(json.is_object() || json.is_array());
	if (json.is_object())
		return this->addObject(base, json);

	for (size_t i = 0; i < json.size(); i++)
		TRY {
			this->addObject(base, json[i]);
		} CATCH_AND_RETHROW("At index " + std::to_string(i) + "\n")
}

void MultiSound::play(Vector2f pos) const
{
	if (this->_sounds.empty())
		return;
	this->_sounds[game->random() % this->_sounds.size()].play(pos);
}

void MultiSound::play() const
{
	if (this->_sounds.empty())
		return;
	this->_sounds[game->random() % this->_sounds.size()].play();
}
