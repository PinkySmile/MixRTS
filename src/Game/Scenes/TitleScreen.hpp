//
// Created by PinkySmile on 09/11/2026.
//

#ifndef MIXRTS_TITLESCREEN_HPP
#define MIXRTS_TITLESCREEN_HPP


#include <TGUI/Widgets/Panel.hpp>
#include "Resources/IScene.hpp"
#include "Resources/SceneArgument.hpp"

class TitleScreen : public IScene {
private:
	struct PlayerUI {
		tgui::Panel::Ptr panel;
		tgui::Button::Ptr color;
		tgui::Button::Ptr player;
		tgui::ComboBox::Ptr team;
		tgui::ComboBox::Ptr spawn;
		tgui::ComboBox::Ptr faction;
	};

	MapInfo _map;
	tgui::Panel::Ptr _panel;
	tgui::Panel::Ptr _addButton;
	std::vector<PlayerUI> _playerPanels;
	tgui::Panel::Ptr _mapContainer;
	std::vector<tgui::Panel::Ptr> _mapPanels;

	void _initTitle();
	void _initLobby();
	void _initMapSelect();
	void _openReplayDialog();
	void _loadReplay(const std::filesystem::path &path);
	void _openLoadDialog();
	void _loadState(const std::filesystem::path &path);
	void _updateMapLobby(
		tgui::Picture::Ptr map,
		tgui::Label::Ptr size,
		tgui::Label::Ptr name,
		tgui::Label::Ptr origin,
		tgui::Label::Ptr author,
		tgui::Label::Ptr objects,
		Vector2u maxSize
	);
	void _addPlayerPanels(tgui::Panel::Ptr players, tgui::Button::Ptr start);
	void _updatePlayerList(tgui::Button::Ptr start);
	void _placeMapsInPanel();

public:
	explicit TitleScreen(std::optional<std::string> message = {});
	~TitleScreen() override;
	void render() const override;
	void renderAbove() const override;
	void update() override;
	void consumeEvent(const sf::Event &event) override;
	[[nodiscard]] tgui::Panel::Ptr getPanel() const override;

	static TitleScreen *create(SceneArguments *);
};

void genSpawnsFurthest(size_t size, std::vector<size_t> &array, const std::vector<char> &teams, const std::vector<Vector2u> &areas);
void genSpawnsRandom(size_t size, std::vector<size_t> &array, const std::vector<char> &teams, const std::vector<Vector2u> &areas);
void genSpawns(const std::vector<PlayerArg> &arg, GameStartParams &params);


#endif //MIXRTS_TITLESCREEN_HPP
