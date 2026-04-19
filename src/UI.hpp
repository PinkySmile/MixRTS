//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_UI_HPP
#define MIXRTS_UI_HPP


#include <string>
#include <unordered_map>
#include <TGUI/Widgets/Panel.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include "../forward.hpp"
#include "../Data/Vector.hpp"

class UI {
public:
	enum EVASound {
		EVA_BUILDING,
		EVA_BUILDING_DONE,
		EVA_CANCEL,
		EVA_CANNOT_DEPLOY,
		EVA_CANNOT_BUILD,
		EVA_GUARDING,
		EVA_HOLD_QUEUE,
		EVA_LOST,
		EVA_WON,
		EVA_LOW_POWER,
		EVA_UNLOCKED,
		EVA_NO_CREDITS,
		EVA_PRIMARY_SELECTED,
		EVA_RETREAT,
		EVA_SELL,
		EVA_TRAINING,
		EVA_UNIT_READY,
		EVA_UPGRADING,
		EVA_CASH_UP,
		EVA_CASH_DOWN,
		EVA_COUNT
	};

	struct StartBuildEvent {
		std::string id;
		std::string queue;
	};

	struct CancelBuildEvent {
		std::string id;
		std::string queue;
	};

	struct PlaceBuildEvent {
		std::string id;
		std::string queue;
		Vector2u position;
	};

	typedef std::variant<StartBuildEvent, PlaceBuildEvent, CancelBuildEvent> Event;

	PlayerState *state;
	MapState *map;
	std::optional<std::string> forcedCursor;

	UI();
	virtual ~UI();
	virtual void setEnabled(bool enabled);
	bool isEnabled() const;
	tgui::Panel::Ptr getPanel() const;
	virtual void playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	virtual std::optional<Event> pollEvent();
	virtual void renderAbove();
	virtual void updateRender();

	virtual void mouseOnUi() = 0;
	virtual void onCancel() = 0;
	virtual void tileClicked(Vector2i pos) = 0;
	virtual void mouseMovedToTile(Vector2i pos) = 0;
	virtual void update();
	virtual void render() = 0;

protected:
	struct EVAQueue {
		EVASound soundId;
		unsigned expireTimer;
		unsigned priority;
		unsigned cooldown;
	};

	CursorAnimation &getCurrentCursor();

	unsigned _globalEvaCooldown = 0;
	std::deque<EVAQueue> _evaQueue;
	std::array<unsigned, EVA_COUNT> _evaCooldown{0};
	std::array<std::unique_ptr<sf::SoundBuffer>, EVA_COUNT> _evaSounds{nullptr};
	sf::Sound _evaSound;
	std::deque<Event> _eventQueue;
	tgui::Panel::Ptr _panel = tgui::Panel::create();
	std::unordered_map<std::string, CursorAnimation> _cursors;
	std::string _cursor = "normal";
	bool _enabled = true;
};


#endif //MIXRTS_UI_HPP
