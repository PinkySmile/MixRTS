//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_UI_HPP
#define MIXRTS_UI_HPP


#include <list>
#include <deque>
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
		EVA_REQUIRE_STORAGE,
		EVA_ALREADY_BUILDING,
		EVA_COUNT
	};
	enum Cursor {
		CURSOR_HORZ_NONE,
		CURSOR_HORZ_RIGHT,
		CURSOR_HORZ_LEFT,

		CURSOR_NORMAL = 0,
		CURSOR_MOVE,
		CURSOR_MOVE_RESTRICT,
		CURSOR_NO_MOVE,
		CURSOR_SELECT,
		CURSOR_ATTACK,
		CURSOR_ATTACK_MOVE,
		CURSOR_ENTER,
		CURSOR_DUMP,
		CURSOR_SCROLL_ALL_DIRS,
		CURSOR_SCROLL_RIGHT,
		CURSOR_SCROLL_LEFT,
		CURSOR_SCROLL_TOP,
		CURSOR_SCROLL_TOP_RIGHT,
		CURSOR_SCROLL_TOP_LEFT,
		CURSOR_SCROLL_BOTTOM,
		CURSOR_SCROLL_BOTTOM_RIGHT,
		CURSOR_SCROLL_BOTTOM_LEFT,
		CURSOR_NO_SCROLL_RIGHT,
		CURSOR_NO_SCROLL_LEFT,
		CURSOR_NO_SCROLL_TOP,
		CURSOR_NO_SCROLL_TOP_RIGHT,
		CURSOR_NO_SCROLL_TOP_LEFT,
		CURSOR_NO_SCROLL_BOTTOM,
		CURSOR_NO_SCROLL_BOTTOM_RIGHT,
		CURSOR_NO_SCROLL_BOTTOM_LEFT,
		CURSOR_DEPLOY,
		CURSOR_NO_DEPLOY,
		CURSOR_PRIMARY,
		CURSOR_COUNT,

		CURSOR_SCROLL_DIFF = CURSOR_NO_SCROLL_RIGHT - CURSOR_SCROLL_RIGHT,
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

	struct DeployUnitEvent {
		unsigned index;
	};

	struct MoveUnitEvent {
		std::vector<unsigned> indices;
		Vector2u position;
	};

	struct HarvestUnitEvent {
		std::vector<unsigned> indices;
		Vector2u position;
	};

	struct DumpUnitEvent {
		std::vector<unsigned> indices;
		unsigned hole;
	};

	typedef std::variant<
		StartBuildEvent,
		PlaceBuildEvent,
		CancelBuildEvent,
		DeployUnitEvent,
		MoveUnitEvent,
		HarvestUnitEvent,
		DumpUnitEvent
	> Event;

	UIState *state;
	GameManager *manager;
	PlayerState *ownerState;
	MapState *map;
	std::optional<Cursor> forcedCursor;

	UI(const Faction &faction);
	virtual ~UI();
	virtual void setEnabled(bool enabled);
	bool isEnabled() const;
	tgui::Panel::Ptr getPanel() const;
	virtual void playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	virtual std::optional<Event> pollEvent();
	virtual void renderAbove();
	virtual void updateRender();

	virtual void mouseOnUi();
	virtual void onCancel(Vector2i absPos);
	virtual void tileClicked(Vector2i absPos);
	virtual void mouseMovedToTile(Vector2i absPos);
	virtual void update();
	virtual void render();

protected:
	struct EVAQueue {
		EVASound soundId;
		unsigned expireTimer;
		unsigned priority;
		unsigned cooldown;
	};

	CursorAnimation &getCurrentCursor();

	void _handleTryPlace();
	void _displayPlacementPreview();
	void _displaySelectedObjectUI(const GameObject &object);
	bool _isSelectionTooFar() const;

	const Faction &_faction;
	std::array<sf::Texture, 4> _selectCorners;
	unsigned _placementAnimation[3] = {0, 0, 0};
	Vector2i _placementPos;
	const Building *_placementBuilding = nullptr;
	unsigned _globalEvaCooldown = 0;
	std::list<EVAQueue> _evaQueue;
	std::array<unsigned, EVA_COUNT> _evaCooldown{0};
	std::array<std::unique_ptr<sf::SoundBuffer>, EVA_COUNT> _evaSounds{nullptr};
	std::vector<GameObject *> _objectBuffer;
	sf::Sound _evaSound;
	std::deque<Event> _eventQueue;
	tgui::Panel::Ptr _settings;
	tgui::Panel::Ptr _panel = tgui::Panel::create();
	std::array<std::string, CURSOR_COUNT> _cursorMapping;
	std::unordered_map<std::string, CursorAnimation> _cursors;
	Cursor _cursor = CURSOR_NORMAL;
	bool _enabled = false;
};


#endif //MIXRTS_UI_HPP
