//
// Created by PinkySmile on 01/04/2026.
//

#include "UI.hpp"
#include "../Data/GameManager.hpp"
#include "../Data/Player.hpp"
#include "../Resources/Game.hpp"
#include "Resources/Assert.hpp"


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
	if (this->_evaQueue.empty()) {
		if (this->_globalEvaCooldown != 0)
			this->_globalEvaCooldown--;
		return;
	}
	if (this->_evaSound.getStatus() != sf::Sound::Status::Playing) {
		if (this->_globalEvaCooldown == 0) {
			auto &e = this->_evaQueue.front();

			game->logger.debug("Playing EVA sound " + std::to_string(e.soundId));
			assert_msg(this->_evaSounds[e.soundId], "Trying to play invalid EVA " + std::to_string(e.soundId));
			this->_evaSound.setBuffer(*this->_evaSounds[e.soundId]);
			this->_evaSound.play();
			this->_evaCooldown[e.soundId] = e.cooldown;
			this->_evaQueue.pop_front();
			this->_globalEvaCooldown = 30;
		} else
			this->_globalEvaCooldown--;
	}

	for (auto &q : this->_evaQueue)
		if (q.expireTimer != 0)
			q.expireTimer--;
	while (!this->_evaQueue.empty() && (
		this->_evaQueue.front().expireTimer == 0 ||
		this->_evaCooldown[this->_evaQueue.front().soundId]
	)) {
		auto &f = this->_evaQueue.front();

		game->logger.debug("Expiring EVA sound " +
			std::to_string(f.soundId) + " " +
			std::to_string(static_cast<int>(this->_evaSound.getStatus())) + " " +
			std::to_string(this->_globalEvaCooldown)
		);
		this->_evaQueue.pop_front();
	}
}

CursorAnimation &UI::getCurrentCursor()
{
	if (this->forcedCursor)
		return this->_cursors.at(*this->forcedCursor);
	return this->_cursors.at(this->_cursor);
}

void UI::renderAbove()
{
	this->getCurrentCursor().render();
}

void UI::updateRender()
{
	for (auto &c : this->_cursors)
		c.second.update();
}
