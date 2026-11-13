//
// Created by PinkySmile on 29/03/2026.
//

#ifndef MIXRTS_UI_HPP
#define MIXRTS_UI_HPP


#include <list>
#include <deque>
#include <string>
#include <unordered_map>
#include <sol/forward.hpp>
#include <TGUI/Widgets/Panel.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "../forward.hpp"
#include "../Data/Vector.hpp"
#include "Sound.hpp"

class MYDLL_API UI {
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
		EVA_POWER_OFF,
		EVA_POWER_ON,
		EVA_RADAR_UP,
		EVA_MY_BUILDING_CAPTURED,
		EVA_BUILDING_CAPTURED,
		EVA_BUILDING_ATTACKED,
		EVA_BUILDING_LOST,
		EVA_MY_UNIT_CAPTURED,
		EVA_UNIT_CAPTURED,
		EVA_UNIT_ATTACKED,
		EVA_ALLIED_MY_BUILDING_CAPTURED,
		EVA_ALLIED_BUILDING_CAPTURED,
		EVA_ALLIED_BUILDING_ATTACKED,
		EVA_ALLIED_BUILDING_LOST,
		EVA_ALLIED_MY_UNIT_CAPTURED,
		EVA_ALLIED_UNIT_CAPTURED,
		EVA_ALLIED_UNIT_ATTACKED,
		EVA_UNIT_LOST,
		EVA_HARV_UNDER_ATTACK,
		EVA_HAZARD_SPAWNED,
		EVA_HAZARD_ATTACKED,
		EVA_ORDER_PLACED,
		EVA_ORDER_IN_1,
		EVA_ORDER_IN_2,
		EVA_ORDER_IN_3,
		EVA_ORDER_IN_4,
		EVA_ORDER_IN_5,
		EVA_NUKE_PREP,
		EVA_NUKE_LAUNCH,
		EVA_NUKE_READY,
		EVA_COUNT
	};
	static std::string EVAToString(EVASound snd);
	enum Cursor {
		CURSOR_HORZ_NONE,
		CURSOR_HORZ_RIGHT,
		CURSOR_HORZ_LEFT,

		CURSOR_NORMAL = 0,
		CURSOR_START_SELECT,
		CURSOR_MOVE,
		CURSOR_MOVE_RESTRICT,
		CURSOR_NO_MOVE,
		CURSOR_SELECT,
		CURSOR_HARVEST,
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
		CURSOR_NO_REPAIR,
		CURSOR_REPAIR,
		CURSOR_NO_SELL,
		CURSOR_SELL,
		CURSOR_NUKE,
		CURSOR_AIR_DROP,
		CURSOR_EXPLODE,
		CURSOR_COUNT,

		CURSOR_SCROLL_DIFF = CURSOR_NO_SCROLL_RIGHT - CURSOR_SCROLL_RIGHT,
	};
	static std::string cursorToString(Cursor snd);
	static Cursor stringToCursor(const std::string &str);

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

	struct AttackUnitEvent {
		std::vector<unsigned> indices;
		unsigned target;
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

	struct StopEvent {
		std::vector<unsigned> indices;
	};

	struct SetPrimaryEvent {
		unsigned index;
	};

	struct RepairEvent {
		unsigned index;
	};

	struct SellEvent {
		unsigned index;
	};

	struct OpenQueueEvent {
		std::string queue;
	};

	struct CloseQueueEvent {
		std::string queue;
	};

	typedef std::variant<
		StartBuildEvent,
		PlaceBuildEvent,
		CancelBuildEvent,
		DeployUnitEvent,
		MoveUnitEvent,
		HarvestUnitEvent,
		DumpUnitEvent,
		SetPrimaryEvent,
		RepairEvent,
		SellEvent,
		AttackUnitEvent,
		StopEvent,
		OpenQueueEvent,
		CloseQueueEvent
	> Event;

	UIState *state;
	GameManager *manager;
	PlayerState *ownerState;
	MapState *map;
	std::optional<Cursor> forcedCursor;
	bool lost = false;
	bool won = false;
	bool onUi = false;

	UI(const Faction &faction, UIState *state, GameManager *manager, PlayerState *ownerState, MapState *map);
	virtual ~UI();
	virtual void setEnabled(bool enabled);
	bool isEnabled() const;
	tgui::Panel::Ptr getPanel() const;
	const Faction &getFaction() const;
	void selectBuildingForPlacement(const std::string &id);
	bool isMouseOnUi() const;

	virtual void playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	//! Drops cached live-object pointers after a state restore.
	void onStateRestored();
	virtual std::optional<Event> pollEvent();
	virtual void renderAbove();
	virtual void updateRender();

	virtual void onCancel(Vector2i absPos);
	virtual void tileClicked(Vector2i absPos);
	virtual void mouseMoved(Vector2i absPos);
	virtual void mouseReleased(Vector2i absPos);
	virtual void onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta);
	virtual void update();
	virtual void render();

	static void pushUserType(sol::state &state);

protected:
	struct EVAQueue {
		EVASound soundId;
		unsigned expireTimer;
		unsigned priority;
		unsigned cooldown;
	};

	CursorAnimation &_getCurrentCursor();
	void _openEndGameDialog(const std::filesystem::path &path);

	void _queueStartBuild(const std::string &id, const std::string &queue);
	void _queueCancelBuild(const std::string &id, const std::string &queue);
	void _queueOpenQueue(const std::string &queue);
	void _queueCloseQueue(const std::string &queue);

	void _loadEvaSound(EVASound sound, const std::string &path, bool relativeToCamera);
	void _loadSelectCorner(unsigned index, const std::string &path);
	void _setCursorMapping(Cursor cursor, const std::string &name);

	void _tileClickedNormal();
	void _tileClickedRepair();
	void _tileClickedSell();

	void _updateCursor();
	void _updateCursorNormal();
	void _updateCursorRepair();
	void _updateCursorSell();
	void _handleTryPlace();
	void _displayPlacementPreview();
	bool _isSelectionTooFar() const;
	void _displaySelectBox(Vector2i pos, Vector2u size);
	virtual void _displaySelectedObjectUI(const GameObject &object) = 0;

	const Faction &_faction;
	std::optional<Vector2i> _startDrag;
	std::array<sf::Texture, 4> _selectCorners;
	unsigned _placementAnimation[3] = {0, 0, 0};
	unsigned _globalEvaCooldown = 0;
	Vector2i _placementPos;
	const GameObject *_placementBuilding = nullptr;
	std::list<EVAQueue> _evaQueue;
	std::array<unsigned, EVA_COUNT> _evaCooldown{0};
	std::array<Sound, EVA_COUNT> _evaSounds;
	std::vector<const GameObject *> _objectBuffer;
	Sound *_currentEvaSound = nullptr;
	Sound _soundButton;
	std::deque<Event> _eventQueue;
	tgui::Panel::Ptr _settings;
	tgui::Panel::Ptr _panel = tgui::Panel::create();
	std::array<std::string, CURSOR_COUNT> _cursorMapping;
	std::unordered_map<std::string, CursorAnimation> _cursors;
	Cursor _cursor = CURSOR_NORMAL;
	Vector2f _minimapPos;
	Vector2f _minimapSize;
	bool _dragging = false;
	bool _enabled = false;
	bool _hasLowPower = false;
	bool _hasRadar = false;
	bool _repairing = false;
	bool _selling = false;
};

#include <sol/sol.hpp>
// --- Lua conversion handlers for UI's event structs ---
// Each event is represented in Lua as a plain table of its fields, in the
// same style as Vector2<T>. These are picked up by sol2 through ADL (UI's
// enclosing namespace is the global namespace, which is where these free
// functions live), and are what allows UI::Event (a std::variant of all of
// these) to be pushed/retrieved from Lua, e.g. from LuaUI::pollEvent().

template <typename Handler>
bool sol_lua_check(sol::types<UI::StartBuildEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {id, queue}");
	return false;
}

inline UI::StartBuildEvent sol_lua_get(sol::types<UI::StartBuildEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::StartBuildEvent event;

	event.id = table.get<std::string>("id");
	event.queue = table.get<std::string>("queue");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::StartBuildEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, event.id);
	lua_setfield(L, -2, "id");
	sol::stack::push(L, event.queue);
	lua_setfield(L, -2, "queue");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::CancelBuildEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {id, queue}");
	return false;
}

inline UI::CancelBuildEvent sol_lua_get(sol::types<UI::CancelBuildEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::CancelBuildEvent event;

	event.id = table.get<std::string>("id");
	event.queue = table.get<std::string>("queue");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::CancelBuildEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, event.id);
	lua_setfield(L, -2, "id");
	sol::stack::push(L, event.queue);
	lua_setfield(L, -2, "queue");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::PlaceBuildEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {id, queue, position}");
	return false;
}

inline UI::PlaceBuildEvent sol_lua_get(sol::types<UI::PlaceBuildEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::PlaceBuildEvent event;

	event.id = table.get<std::string>("id");
	event.queue = table.get<std::string>("queue");
	event.position = table.get<Vector2u>("position");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::PlaceBuildEvent &event)
{
	lua_createtable(L, 0, 3);
	sol::stack::push(L, event.id);
	lua_setfield(L, -2, "id");
	sol::stack::push(L, event.queue);
	lua_setfield(L, -2, "queue");
	sol::stack::push(L, event.position);
	lua_setfield(L, -2, "position");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::DeployUnitEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {index}");
	return false;
}

inline UI::DeployUnitEvent sol_lua_get(sol::types<UI::DeployUnitEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::DeployUnitEvent event;

	event.index = table.get<unsigned>("index");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::DeployUnitEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.index);
	lua_setfield(L, -2, "index");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::AttackUnitEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {indices, target}");
	return false;
}

inline UI::AttackUnitEvent sol_lua_get(sol::types<UI::AttackUnitEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::AttackUnitEvent event;

	event.indices = table.get<std::vector<unsigned>>("indices");
	event.target = table.get<unsigned>("target");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::AttackUnitEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, sol::as_table(event.indices));
	lua_setfield(L, -2, "indices");
	sol::stack::push(L, event.target);
	lua_setfield(L, -2, "target");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::MoveUnitEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {indices, position}");
	return false;
}

inline UI::MoveUnitEvent sol_lua_get(sol::types<UI::MoveUnitEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::MoveUnitEvent event;

	event.indices = table.get<std::vector<unsigned>>("indices");
	event.position = table.get<Vector2u>("position");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::MoveUnitEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, sol::as_table(event.indices));
	lua_setfield(L, -2, "indices");
	sol::stack::push(L, event.position);
	lua_setfield(L, -2, "position");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::HarvestUnitEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {indices, position}");
	return false;
}

inline UI::HarvestUnitEvent sol_lua_get(sol::types<UI::HarvestUnitEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::HarvestUnitEvent event;

	event.indices = table.get<std::vector<unsigned>>("indices");
	event.position = table.get<Vector2u>("position");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::HarvestUnitEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, sol::as_table(event.indices));
	lua_setfield(L, -2, "indices");
	sol::stack::push(L, event.position);
	lua_setfield(L, -2, "position");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::DumpUnitEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {indices, hole}");
	return false;
}

inline UI::DumpUnitEvent sol_lua_get(sol::types<UI::DumpUnitEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::DumpUnitEvent event;

	event.indices = table.get<std::vector<unsigned>>("indices");
	event.hole = table.get<unsigned>("hole");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::DumpUnitEvent &event)
{
	lua_createtable(L, 0, 2);
	sol::stack::push(L, sol::as_table(event.indices));
	lua_setfield(L, -2, "indices");
	sol::stack::push(L, event.hole);
	lua_setfield(L, -2, "hole");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::StopEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {indices}");
	return false;
}

inline UI::StopEvent sol_lua_get(sol::types<UI::StopEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::StopEvent event;

	event.indices = table.get<std::vector<unsigned>>("indices");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::StopEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, sol::as_table(event.indices));
	lua_setfield(L, -2, "indices");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::SetPrimaryEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {index}");
	return false;
}

inline UI::SetPrimaryEvent sol_lua_get(sol::types<UI::SetPrimaryEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::SetPrimaryEvent event;

	event.index = table.get<unsigned>("index");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::SetPrimaryEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.index);
	lua_setfield(L, -2, "index");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::RepairEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {index}");
	return false;
}

inline UI::RepairEvent sol_lua_get(sol::types<UI::RepairEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::RepairEvent event;

	event.index = table.get<unsigned>("index");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::RepairEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.index);
	lua_setfield(L, -2, "index");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::SellEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {index}");
	return false;
}

inline UI::SellEvent sol_lua_get(sol::types<UI::SellEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::SellEvent event;

	event.index = table.get<unsigned>("index");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::SellEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.index);
	lua_setfield(L, -2, "index");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::OpenQueueEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {queue}");
	return false;
}

inline UI::OpenQueueEvent sol_lua_get(sol::types<UI::OpenQueueEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::OpenQueueEvent event;

	event.queue = table.get<std::string>("queue");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::OpenQueueEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.queue);
	lua_setfield(L, -2, "queue");
	return 1;
}

template <typename Handler>
bool sol_lua_check(sol::types<UI::CloseQueueEvent>, lua_State *L, int index, Handler &&handler, sol::stack::record &tracking)
{
	if (lua_type(L, index) == LUA_TTABLE) {
		tracking.use(1);
		return true;
	}
	handler(L, index, sol::type_of(L, index), sol::type::table, "expected table {queue}");
	return false;
}

inline UI::CloseQueueEvent sol_lua_get(sol::types<UI::CloseQueueEvent>, lua_State *L, int index, sol::stack::record &tracking)
{
	sol::stack_table table(L, index);
	UI::CloseQueueEvent event;

	event.queue = table.get<std::string>("queue");
	tracking.use(1);
	return event;
}

inline int sol_lua_push(lua_State *L, const UI::CloseQueueEvent &event)
{
	lua_createtable(L, 0, 1);
	sol::stack::push(L, event.queue);
	lua_setfield(L, -2, "queue");
	return 1;
}


#endif //MIXRTS_UI_HPP
