//
// Created by PinkySmile on 01/04/2026.
//

#include "UI.hpp"
#include "../Data/Player.hpp"
#include "../Resources/Game.hpp"


UI::UI() :
	_evaSound(game->dummySoundBuffer)
{
	this->_panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);
}

UI::~UI() = default;

void UI::setEnabled(bool enabled)
{
	this->_panel->setVisible(enabled);
	this->_enabled = enabled;
}

bool UI::isEnabled() const
{
	return this->_enabled;
}

tgui::Panel::Ptr UI::getPanel() const
{
	return this->_panel;
}

std::optional<UI::Event> UI::pollEvent()
{
	if (this->_eventQueue.empty())
		return {};

	std::optional result = this->_eventQueue.front();

	this->_eventQueue.pop_front();
	return result;
}

void UI::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &)
{
	if (!this->_enabled)
		return;
	if (this->_evaCooldown[sound])
		return;
	this->_evaQueue.push_back({sound, expireIn, priority, cooldown});
}

void UI::update()
{
	for (auto &i : this->_evaCooldown)
		if (i)
			i--;
	if (!this->_enabled)
		return this->_evaQueue.clear();
	if (this->_evaQueue.empty())
		return;
	while (!this->_evaSounds[this->_evaQueue.front().soundId])
		this->_evaQueue.pop_front();
	if (this->_evaQueue.empty())
		return;
	if (this->_evaSound.getStatus() != sf::Sound::Status::Playing) {
		this->_evaSound.setBuffer(*this->_evaSounds[this->_evaQueue.front().soundId]);
		this->_evaSound.play();
		this->_evaCooldown[this->_evaQueue.front().soundId] = this->_evaQueue.front().cooldown;
		this->_evaQueue.pop_front();
	}
	for (auto &q : this->_evaQueue)
		q.expireTimer--;
	while (!this->_evaQueue.empty() && (
		this->_evaQueue.front().expireTimer == 0 ||
		this->_evaCooldown[this->_evaQueue.front().soundId]
	))
		this->_evaQueue.pop_front();
}
