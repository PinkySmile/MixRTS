//
// Created by PinkySmile on 06/04/2026.
//

#ifndef MIXRTS_PLAYER_HPP
#define MIXRTS_PLAYER_HPP


#include <unordered_map>
#include <string>
#include <map>
#include <list>
#include <deque>
#include <memory>
#include <SFML/Window/Event.hpp>
#include "../forward.hpp"
#include "Vector.hpp"
#include "Color.hpp"
#include "Utils.hpp"
#include "UI.hpp"

struct PlayerQueue {
	const Faction *faction;
	const Upgrade *upgrade;
	const GameObject *object;
	const std::string *queueId;
	const std::string *elementId;
	std::unordered_map<std::string, unsigned> spent;
	unsigned buildTime;
	unsigned buildProgress;
	unsigned limit;
	bool unlocked;
	bool buildable;
	bool paused;
};

struct PlayerState {
	size_t team = 0;
	size_t index;
	size_t factionIndex;
	bool hasRadar = false;
	unsigned consumedPower = 0;
	unsigned producedPower = 0;
	std::unordered_map<std::string, unsigned> money;
	std::unordered_map<std::string, unsigned> systemMoney;
	std::unordered_map<std::string, unsigned> totalStorage;
	std::unordered_map<std::string, unsigned> elementsOwned;
	std::unordered_map<std::string, std::list<GameObject *>> queueBuildings;
	std::unordered_map<std::string, GameObject *> primaryBuildings;
	std::list<GameObject *> myObjects;
	std::list<GameObject *> radars;
	std::unordered_map<std::string, PlayerQueue *> buildQueues;
	std::unordered_map<std::string, std::unordered_map<std::string, PlayerQueue>> queues;

	unsigned getCombinedMoney(const std::string &key);
};

struct UIState {
	std::set<const GameObject *> selectedObjects;
};

class Player {
private:
	GameManager &_parent;
	MapState &_mapState;
	bool _enabled = false;
	size_t _currentUi;
	HSLColor _color;
	const Faction &_primaryFaction;
	const std::vector<Faction> &_factions;
	std::vector<std::unique_ptr<UI>> _uis;
	std::unordered_map<std::string, const GameObject *> _loadedObjects;

	void _tryEndBuild(const std::string &name, PlayerQueue &queue);
	void _clean();

public:
	PlayerState state;
	UIState uiState;

	enum CreditStrategy {
		CREDIT_SYSTEM,
		CREDIT_SPILL,
		CREDIT_STRICT
	};

	Player(GameManager &parent, const std::vector<Faction> &factions, size_t index, const HSLColor &color, std::vector<MapState> &state);
	void update();
	void updateRender();
	void render();
	void renderAbove();
	void handleUiEvents();
	void setEnabled(bool enabled);
	HSLColor getColor() const;
	void mouseOnUi();
	void keyPressed(const sf::Event::KeyPressed &key);
	const Faction &getPrimaryFaction() const;
	const std::unordered_map<std::string, const GameObject *> &getObjects() const;
	void playEVA(UI::EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	void recomputeLocked();
	void checkUnlocked(const Faction &faction);
	void lockQueue(const std::string &queueId);
	void resetQueue(const std::string &queueId);
	std::vector<std::unique_ptr<UI>> &getUIs();
	void onMouseMoved(Vector2i pos);
	void onLeftMouseClicked(Vector2i pos);
	void onRightMouseClicked(Vector2i pos);
	bool credit(const std::string &type, unsigned amount, CreditStrategy strategy);
	bool consume(const std::string &type, unsigned amount);
	GameObject *getPrimaryBuilding(const std::string &queue);
	GameObject *getSpawnBuilding(const std::string &queue);
	void onObjectLost(GameObject *obj);
	void onObjectGained(GameObject *obj);

	[[nodiscard]] bool isEnabled() const;

	template<typename T>
	void unlockObject(const Faction &f, const std::string &name, T &object, bool debugTime = false);
};


#endif //MIXRTS_PLAYER_HPP
