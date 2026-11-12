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
#include <sol/forward.hpp>
#include "forward.hpp"
#include "Vector.hpp"
#include "Color.hpp"
#include "Utils.hpp"
#include "UI/UI.hpp"
#include "Data/GameManager.hpp"
#include "SharedLibrary.hpp"

class CustomQueue {
public:
	PlayerQueue *queue;
	Player *player;

	CustomQueue(Player *player, PlayerQueue *queue);
	virtual ~CustomQueue() = default;
	virtual bool update(PlayerQueue &entry) = 0;
	virtual bool update(PlayerQueueEntry *entry) = 0;
	virtual bool tryEndBuild(PlayerQueueEntry *entry) = 0;
	virtual bool handleStartBuildEvent(PlayerQueueEntry *entry, const UI::StartBuildEvent &event) = 0;
	virtual bool handlePlaceBuildEvent(PlayerQueueEntry *entry, const UI::PlaceBuildEvent &event) = 0;
	virtual bool handleCancelBuildEvent(PlayerQueueEntry *entry, const UI::CancelBuildEvent &event) = 0;
};

class CustomQueueLua : public CustomQueue {
private:
	sol::function _function;
	sol::environment _env;

public:
	CustomQueueLua(Player *player, PlayerQueue *queue, sol::state &lua, const std::filesystem::path &path);
	~CustomQueueLua() override = default;
	bool update(PlayerQueue &entry) override;
	bool update(PlayerQueueEntry *entry) override;
	bool tryEndBuild(PlayerQueueEntry *entry) override;
	bool handleStartBuildEvent(PlayerQueueEntry *entry, const UI::StartBuildEvent &event) override;
	bool handlePlaceBuildEvent(PlayerQueueEntry *entry, const UI::PlaceBuildEvent &event) override;
	bool handleCancelBuildEvent(PlayerQueueEntry *entry, const UI::CancelBuildEvent &event) override;
};

class CustomQueueDll : public CustomQueue {
private:
	SharedLibrary _library;
	bool (*_update)(CustomQueue &b, PlayerQueue &entry);
	bool (*_updateEntry)(CustomQueue &b, PlayerQueueEntry *entry);
	bool (*_tryEndBuild)(CustomQueue &b, PlayerQueueEntry *entry);
	bool (*_handleStartBuildEvent)(CustomQueue &b, PlayerQueueEntry *entry, const UI::StartBuildEvent &event);
	bool (*_handlePlaceBuildEvent)(CustomQueue &b, PlayerQueueEntry *entry, const UI::PlaceBuildEvent &event);
	bool (*_handleCancelBuildEvent)(CustomQueue &b, PlayerQueueEntry *entry, const UI::CancelBuildEvent &event);

public:
	CustomQueueDll(Player *player, PlayerQueue *queue, const std::filesystem::path &path);
	~CustomQueueDll() override;
	bool update(PlayerQueue &entry) override;
	bool update(PlayerQueueEntry *entry) override;
	bool tryEndBuild(PlayerQueueEntry *entry) override;
	bool handleStartBuildEvent(PlayerQueueEntry *entry, const UI::StartBuildEvent &event) override;
	bool handlePlaceBuildEvent(PlayerQueueEntry *entry, const UI::PlaceBuildEvent &event) override;
	bool handleCancelBuildEvent(PlayerQueueEntry *entry, const UI::CancelBuildEvent &event) override;
};

struct MYDLL_API PlayerQueue {
	Player *player;
	std::unordered_map<std::string, PlayerQueueEntry> entries;
	const FactionQueue *params;
	unsigned providers = 0;
	unsigned timer = 0;
	unsigned nb = 0;
	std::unordered_map<std::string, unsigned> stock;
	std::unordered_map<std::string, unsigned> basket;
	std::unordered_map<std::string, std::unordered_map<std::string, unsigned>> prices;
	std::shared_ptr<CustomQueue> customLib;
	bool checkingMenu = false;
	bool busy = false;

	void init(Player &player, const std::string *id, const Faction &faction, const FactionQueue *params);
	void update();

	static void pushUserType(sol::state &state);
};

struct MYDLL_API PlayerQueueEntry {
	PlayerQueue *queue;
	const Faction *faction;
	const Upgrade *upgrade;
	const GameObject *object;
	const std::string *queueId;
	const std::string *elementId;
	std::unordered_map<std::string, unsigned> spent;
	float buildTime;
	float buildProgress;
	unsigned limit;
	bool unlocked;
	bool buildable;
	bool paused;

	static void pushUserType(sol::state &state);
};

struct MYDLL_API PlayerState {
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
	std::unordered_map<std::string, PlayerQueueEntry *> buildQueues;
	std::unordered_map<std::string, PlayerQueue> queues;

	unsigned getCombinedMoney(const std::string &key);

	static void pushUserType(sol::state &state);
};

struct MYDLL_API UIState {
	std::set<const GameObject *> selectedObjects;

	static void pushUserType(sol::state &state);
};

class MYDLL_API Player {
public:
	GameManager &parent;
	MapState &mapState;
	PlayerState state;
	UIState uiState;
	bool debugUnlock = false;

	enum CreditStrategy {
		CREDIT_SYSTEM,
		CREDIT_SPILL,
		CREDIT_STRICT
	};

	Player(GameManager &parent, const std::vector<Faction> &factions, size_t index, const HSLColor &color, std::vector<MapState> &state, const GameManager::UIFactory &factory, size_t playerIndex);
	~Player();
	bool lost() const;
	void update();
	void checkWon();
	void updateRender();
	void render();
	void renderAbove();
	void handleUiEvents();
	void setEnabled(bool enabled);
	HSLColor getColor() const;
	bool isMouseOnUi() const;
	void keyPressed(const sf::Event::KeyPressed &key);
	const Faction &getPrimaryFaction() const;
	const std::unordered_map<std::string, const GameObject *> &getObjects() const;
	void playEVA(UI::EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args = "");
	void recomputeLocked();
	void checkUnlocked();
	void lockQueue(const std::string &queueId);
	void resetQueue(const std::string &queueId);
	void refreshQueue(const std::string &queueId);
	CombinedUI &getCombinedUI();
	void onMouseMoved(Vector2i pos);
	void onLeftMouseClicked(Vector2i pos);
	void onRightMouseClicked(Vector2i pos);
	void onLeftMouseReleased(Vector2i pos);
	void onRightMouseReleased(Vector2i pos);
	bool credit(const std::string &type, unsigned amount, CreditStrategy strategy);
	bool consume(const std::string &type, unsigned amount);
	GameObject *getPrimaryBuilding(const std::string &queue);
	GameObject *getSpawnBuilding(const std::string &queue, bool stackable);
	void onObjectLost(GameObject *obj);
	void onObjectGained(GameObject *obj);
	std::vector<GameObject *> getObjects(const std::string &id);
	const std::unordered_map<std::string, const GameObject *> &getLoadedObjects() const;
	const std::unordered_map<std::string, const Upgrade *> &getLoadedUpgrades() const;

	[[nodiscard]] bool isEnabled() const;

	template<typename T>
	void unlockObject(const std::string &name, const T &object, bool debugTime = false);

	static void pushUserType(sol::state &state);

private:
	bool _enabled = false;
	bool _lost = false;
	bool _won = false;
	HSLColor _color;
	const Faction &_primaryFaction;
	const std::vector<Faction> &_factions;
	std::shared_ptr<CombinedUI> _ui;
	std::unordered_map<std::string, const GameObject *> _loadedObjects;
	std::unordered_map<std::string, const Upgrade *> _loadedUpgrades;

	void _handleUiEvent(const UI::StartBuildEvent &event);
	void _handleUiEvent(const UI::PlaceBuildEvent &event);
	void _handleUiEvent(const UI::CancelBuildEvent &event);
	void _handleUiEvent(const UI::DeployUnitEvent &event);
	void _handleUiEvent(const UI::MoveUnitEvent &event);
	void _handleUiEvent(const UI::HarvestUnitEvent &event);
	void _handleUiEvent(const UI::DumpUnitEvent &event);
	void _handleUiEvent(const UI::SetPrimaryEvent &event);
	void _handleUiEvent(const UI::RepairEvent &event);
	void _handleUiEvent(const UI::SellEvent &event);
	void _handleUiEvent(const UI::AttackUnitEvent &event);
	void _handleUiEvent(const UI::StopEvent &event);
	void _handleUiEvent(const UI::OpenQueueEvent &event);
	void _handleUiEvent(const UI::CloseQueueEvent &event);

	void _clean();

	void _updateQueue(PlayerQueueEntry &entry);
	void _updateSingleQueue(PlayerQueueEntry &entry);
	void _updateStarportQueue(PlayerQueueEntry &entry);
	void _updateAutoTickQueue(PlayerQueueEntry &entry);

	void _spawnDelivery(const std::string &queue, Vector2f target, const ExtraSpawn &params, const std::vector<std::string> &objects);
	void _tryEndBuildSingle(PlayerQueueEntry &entry);
	void _tryEndBuildStarport(PlayerQueueEntry &entry);
	void _tryEndBuildAutoTick(PlayerQueueEntry &entry);

	friend PlayerQueue;
};


#endif //MIXRTS_PLAYER_HPP
