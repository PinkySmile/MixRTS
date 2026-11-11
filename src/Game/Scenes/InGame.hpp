//
// Created by PinkySmile on 10/11/2026.
//

#ifndef MIXRTS_INGAME_HPP
#define MIXRTS_INGAME_HPP


#include "Data/Faction.hpp"
#include "Resources/IScene.hpp"
#include "Resources/SceneArgument.hpp"

class InGame : public IScene {
private:
	tgui::Panel::Ptr _panel;
	LobbySettings _settings;
	std::vector<Faction> _loadedFactions;
	sf::Clock _clock;
	float _timer = 0;

public:
	InGame(GameStartParams &params, std::function<void (const std::string &)> &cb);
	~InGame() override;
	void render() const override;
	void renderAbove() const override;
	void update() override;
	void consumeEvent(const sf::Event &event) override;
	[[nodiscard]] tgui::Panel::Ptr getPanel() const override;

	static InGame *create(SceneArguments *);
};


#endif //MIXRTS_INGAME_HPP
