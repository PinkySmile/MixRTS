//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_UI_HPP
#define MIXRTS_UI_HPP


#include <string>
#include <unordered_map>
#include <TGUI/Widgets/Panel.hpp>
#include <SFML/Audio/Sound.hpp>
#include "../forward.hpp"
#include "../Data/Vector.hpp"

class UI {
public:
	struct StartBuildEvent {
		std::string id;
		std::string queue;
	};

	struct PlaceBuildEvent {
		std::string id;
		std::string queue;
		Vector2u position;
	};

	typedef std::variant<StartBuildEvent, PlaceBuildEvent> Event;

	PlayerState *state;
	MapState *map;

	UI();
	virtual ~UI();
	virtual void setEnabled(bool enabled);
	bool isEnabled() const;
	tgui::Panel::Ptr getPanel() const;
	virtual std::optional<Event> pollEvent();

	virtual void mouseOnUi() = 0;
	virtual void onCancel() = 0;
	virtual void tileClicked(Vector2i pos) = 0;
	virtual void mouseMovedToTile(Vector2i pos) = 0;
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void renderAbove() = 0;

protected:
	std::deque<Event> _eventQueue;
	tgui::Panel::Ptr _panel = tgui::Panel::create();
	bool _enabled = true;

	void _playSound(sf::Sound &sound);
};


#endif //MIXRTS_UI_HPP
