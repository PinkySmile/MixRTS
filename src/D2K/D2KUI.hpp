//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_D2KUI_HPP
#define MIXRTS_D2KUI_HPP


#include <map>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/Picture.hpp>
#include <TGUI/Widgets/Button.hpp>
#include "../forward.hpp"
#include "UI.hpp"
#include "Sprite.hpp"
#include "Sound.hpp"


class D2KUI : public UI {
private:
	std::string _prefix;

	tgui::Button::Ptr _resumeGame;
	tgui::Panel::Ptr _power;
	tgui::Button::Ptr _sell;
	tgui::Button::Ptr _guard;
	tgui::Button::Ptr _repair;
	tgui::Button::Ptr _retreat;
	tgui::Button::Ptr _world;
	tgui::Button::Ptr _up1;
	tgui::Button::Ptr _up2;
	tgui::Button::Ptr _down1;
	tgui::Button::Ptr _down2;
	tgui::Button::Ptr _options;
	tgui::Button::Ptr _main1;
	tgui::Button::Ptr _main2;
	tgui::Button::Ptr _upgrade;
	tgui::Button::Ptr _starport;
	tgui::Picture::Ptr _blazon;
	tgui::Picture::Ptr _greenPower;
	tgui::Picture::Ptr _yellowPower;
	tgui::Picture::Ptr _redPower;
	std::vector<std::string> _buildOrder;
	std::vector<std::string> _unitsOrder;
	std::vector<std::string> _upgradeOrder;
	std::vector<std::string> _starportOrder;
	tgui::ScrollablePanel::Ptr _buildQueue;
	tgui::ScrollablePanel::Ptr _unitsQueue;
	tgui::ScrollablePanel::Ptr _upgradeQueue;
	tgui::ScrollablePanel::Ptr _starportQueue;

	std::unordered_map<std::string, tgui::Texture> _cameos;
	std::array<tgui::Picture::Ptr, 8> _creditDigits;
	std::array<tgui::Texture, 11> _creditRawDigits;
	std::vector<std::string> _buildQueueAdded;
	std::vector<std::string> _unitsQueueAdded;
	std::vector<std::string> _upgradeQueueAdded;
	std::vector<std::string> _starportQueueAdded;

	Sound _soundMoveBlocked;
	Sound _soundButton;
	Sound _soundMove;
	sf::Texture _primary;

	void _loadEvaSounds(const std::filesystem::path &base, const std::string &folder);
	bool _hasUpgrades();

protected:
	void _displaySelectedObjectUI(const GameObject &object) override;

public:
	D2KUI(const Faction &faction, const std::filesystem::path &base, const nlohmann::json &params);
	~D2KUI() noexcept override;
	void update() override;
	void playEVA(EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args) override;
};



#endif //MIXRTS_D2KUI_HPP
