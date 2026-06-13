//
// Created by PinkySmile on 29/03/2026.
//

#include <nlohmann/json.hpp>
#include "D2KUI.hpp"
#include "D2KCameo.hpp"
#include "Resources/Assert.hpp"
#include "../Data/Faction.hpp"
#include "../Data/Mod.hpp"
#include "../Data/Player.hpp"
#include "../Resources/Game.hpp"
#include "Data/Animation.hpp"
#include "Data/GameManager.hpp"


static std::string __upgradeOrder[] = {
	"cony.upgrade",
	"barr.upgrade",
	"ahtec.upgrade",
	"lfac.upgrade",
	"hfac.upgrade"
};
static std::string __buildOrder[] = {
	"2con",
	"3con",
	"wtra",
	"barr",
	"wall",
	"refi",
	"gunt",
	"outp",
	"roct",
	"ahtec",
	"htec",
	"lfac",
	"silo",
	"hfac",
	"star",
	"repr",
	"ixrc",
	"apala",
	"hpala",
	"opala"
};
static std::string __unitsOrder[] = {
	"rifl",
	"trop",
	"engi",
	"thump",
	"trik",
	"ride",
	"quad",
	"harv",
	"tank",
	"mcv",
	"misl",
	"devi",
	"sieg",
	"soni",
	"deva",
	"carr",
	"orth",
	"frem",
	"sabo",
	"dhmi",
	"gren",
	"srid",
	"sard",
};

void D2KUI::_loadEvaSounds(const std::filesystem::path &base, const std::string &folder)
{
	this->_evaSounds[EVA_BUILDING]         = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "build.ogg");
	this->_evaSounds[EVA_BUILDING_DONE]    = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "building_done.ogg");
	this->_evaSounds[EVA_CANCEL]           = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "cancel.ogg");
	this->_evaSounds[EVA_CANNOT_DEPLOY]    = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "cannot_deploy.ogg");
	this->_evaSounds[EVA_CANNOT_BUILD]     = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "cannot_place.ogg");
	this->_evaSounds[EVA_GUARDING]         = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "guarding.ogg");
	this->_evaSounds[EVA_HOLD_QUEUE]       = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "hold.ogg");
	this->_evaSounds[EVA_LOST]             = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "lose.ogg");
	this->_evaSounds[EVA_WON]              = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "win.ogg");
	this->_evaSounds[EVA_LOW_POWER]        = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "low_power.ogg");
	this->_evaSounds[EVA_UNLOCKED]         = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "new_unlock.ogg");
	this->_evaSounds[EVA_NO_CREDITS]       = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "no_money.ogg");
	this->_evaSounds[EVA_PRIMARY_SELECTED] = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "primary.ogg");
	this->_evaSounds[EVA_RETREAT]          = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "retreat.ogg");
	this->_evaSounds[EVA_SELL]             = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "sell.ogg");
	this->_evaSounds[EVA_TRAINING]         = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "train.ogg");
	this->_evaSounds[EVA_UNIT_READY]       = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "unit_ready.ogg");
	this->_evaSounds[EVA_UPGRADING]        = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "upgrading.ogg");
	this->_evaSounds[EVA_REQUIRE_STORAGE]  = std::make_unique<sf::SoundBuffer>(base / "announcers" / game->locale / folder / "silo_needed.ogg");
	this->_evaSounds[EVA_CASH_DOWN]        = std::make_unique<sf::SoundBuffer>(base / "sounds/CASHTIK1.ogg");
	this->_evaSounds[EVA_CASH_UP]          = std::make_unique<sf::SoundBuffer>(base / "sounds/CASHTIK1.ogg");
}

D2KUI::D2KUI(const Faction &faction, const std::filesystem::path &base, const nlohmann::json &params) :
	_faction(faction)
{
	auto path = base / "ui" / game->locale / "ui.gui";
	faction.getBase().loadCursors("cursors.json", this->_cursors);

	this->_panel->loadWidgetsFromFile(path.native());
	this->_sell            = this->_panel->get<tgui::Button>("Sell");
	this->_guard           = this->_panel->get<tgui::Button>("Guard");
	this->_repair          = this->_panel->get<tgui::Button>("Repair");
	this->_retreat         = this->_panel->get<tgui::Button>("Retreat");
	this->_world           = this->_panel->get<tgui::Button>("Map");
	this->_up1             = this->_panel->get<tgui::Button>("Up1");
	this->_up2             = this->_panel->get<tgui::Button>("Up2");
	this->_down1           = this->_panel->get<tgui::Button>("Down1");
	this->_down2           = this->_panel->get<tgui::Button>("Down2");
	this->_options         = this->_panel->get<tgui::Button>("Options");
	this->_main1           = this->_panel->get<tgui::Button>("Main1");
	this->_main2           = this->_panel->get<tgui::Button>("Main2");
	this->_upgrade         = this->_panel->get<tgui::Button>("Upgrade");
	this->_starport        = this->_panel->get<tgui::Button>("Starport");
	this->_blazon          = this->_panel->get<tgui::Picture>("Blazon");
	this->_power           = this->_panel->get<tgui::Panel>("Power");
	this->_settings        = this->_panel->get<tgui::Panel>("Settings");
	this->_greenPower      = this->_power->get<tgui::Picture>("GreenPower");
	this->_yellowPower     = this->_power->get<tgui::Picture>("YellowPower");
	this->_redPower        = this->_power->get<tgui::Picture>("RedPower");
	this->_buildQueue      = this->_panel->get<tgui::ScrollablePanel>("BuildingQueue");
	this->_unitsQueue      = this->_panel->get<tgui::ScrollablePanel>("UnitsQueue");
	this->_upgradeQueue    = this->_panel->get<tgui::ScrollablePanel>("UpgradeQueue");
	this->_starportQueue   = this->_panel->get<tgui::ScrollablePanel>("StarportQueue");
	this->_creditDigits[0] = this->_panel->get<tgui::Picture>("Credit0");
	this->_creditDigits[1] = this->_panel->get<tgui::Picture>("Credit1");
	this->_creditDigits[2] = this->_panel->get<tgui::Picture>("Credit2");
	this->_creditDigits[3] = this->_panel->get<tgui::Picture>("Credit3");
	this->_creditDigits[4] = this->_panel->get<tgui::Picture>("Credit4");
	this->_creditDigits[5] = this->_panel->get<tgui::Picture>("Credit5");
	this->_creditDigits[6] = this->_panel->get<tgui::Picture>("Credit6");
	this->_creditDigits[7] = this->_panel->get<tgui::Picture>("Credit7");
	this->_resumeGame      = this->_settings->get<tgui::Button>("Resume");
	this->_creditRawDigits[0] = tgui::Texture{(base / "numbers/credit_0.png").native()};
	this->_creditRawDigits[1] = tgui::Texture{(base / "numbers/credit_1.png").native()};
	this->_creditRawDigits[2] = tgui::Texture{(base / "numbers/credit_2.png").native()};
	this->_creditRawDigits[3] = tgui::Texture{(base / "numbers/credit_3.png").native()};
	this->_creditRawDigits[4] = tgui::Texture{(base / "numbers/credit_4.png").native()};
	this->_creditRawDigits[5] = tgui::Texture{(base / "numbers/credit_5.png").native()};
	this->_creditRawDigits[6] = tgui::Texture{(base / "numbers/credit_6.png").native()};
	this->_creditRawDigits[7] = tgui::Texture{(base / "numbers/credit_7.png").native()};
	this->_creditRawDigits[8] = tgui::Texture{(base / "numbers/credit_8.png").native()};
	this->_creditRawDigits[9] = tgui::Texture{(base / "numbers/credit_9.png").native()};
	this->_creditRawDigits[10]= tgui::Texture{(base / "numbers/credit_empty.png").native()};
	for (size_t i = 0; i < this->_selectCorners.size(); i++)
		assert_exp(this->_selectCorners[i].loadFromFile(base / ("cursor/select_" + std::to_string(i) + ".png")));

	this->_blazon->getRenderer()->setTexture({(base / params["blazon"].get<std::string>()).native()});
	this->_settings->setVisible(false);
	this->_main1->setVisible(false);
	this->_main2->setVisible(false);
	this->_upgrade->setVisible(false);
	this->_starport->setVisible(false);
	this->_upgradeQueue->setVisible(false);
	this->_starportQueue->setVisible(false);
	this->_buildQueue->getVerticalScrollbar()->setScrollAmount(47);
	this->_unitsQueue->getVerticalScrollbar()->setScrollAmount(47);

	assert_exp(this->_move.loadFromFile(base / "sounds/SIDEBAR1.ogg"));
	assert_exp(this->_button.loadFromFile(base / "sounds/BUTTON1.ogg"));
	assert_exp(this->_moveBlocked.loadFromFile(base / "sounds/ENDLIST1.ogg"));

	this->_up1->onMousePress([this] {
		auto amount = this->_buildQueue->getContentOffset();

		this->_buildQueue->scrolled(1, {0, 0}, false);
		if (amount == this->_buildQueue->getContentOffset())
			this->_soundMoveBlocked.play();
		else
			this->_soundMove.play();
	});
	this->_up2->onMousePress([this] {
		auto amount = this->_unitsQueue->getContentOffset();

		this->_unitsQueue->scrolled(1, {0, 0}, false);
		if (amount == this->_unitsQueue->getContentOffset())
			this->_soundMoveBlocked.play();
		else
			this->_soundMove.play();
	});
	this->_down1->onMousePress([this] {
		auto amount = this->_buildQueue->getContentOffset();

		this->_buildQueue->scrolled(-1, {0, 0}, false);
		if (amount == this->_buildQueue->getContentOffset())
			this->_soundMoveBlocked.play();
		else
			this->_soundMove.play();
	});
	this->_down2->onMousePress([this] {
		auto amount = this->_unitsQueue->getContentOffset();

		this->_unitsQueue->scrolled(-1, {0, 0}, false);
		if (amount == this->_unitsQueue->getContentOffset())
			this->_soundMoveBlocked.play();
		else
			this->_soundMove.play();
	});
	this->_upgrade->onMousePress([this]{
		this->_upgrade->setVisible(false);
		this->_starport->setVisible(false);
		this->_main1->setVisible(true);
		this->_buildQueue->setVisible(false);
		this->_unitsQueue->setVisible(false);
		this->_upgradeQueue->setVisible(true);
		this->_up1->setVisible(false);
		this->_up2->setVisible(false);
		this->_down1->setVisible(false);
		this->_down2->setVisible(false);
	});
	this->_main1->onMousePress([this]{
		this->_upgrade->setVisible(this->_hasUpgrades());
		this->_starport->setVisible(this->state->queues.contains(this->_faction.getBase().getName() + ":starport"));
		this->_main1->setVisible(false);
		this->_buildQueue->setVisible(true);
		this->_unitsQueue->setVisible(true);
		this->_upgradeQueue->setVisible(false);
		this->_up1->setVisible(true);
		this->_up2->setVisible(true);
		this->_down1->setVisible(true);
		this->_down2->setVisible(true);
	});
	this->_main2->onMousePress([this]{
		this->_upgrade->setVisible(this->_hasUpgrades());
		this->_starport->setVisible(this->state->queues.contains(this->_faction.getBase().getName() + ":starport"));
		this->_main1->setVisible(false);
		this->_buildQueue->setVisible(true);
		this->_unitsQueue->setVisible(true);
		this->_upgradeQueue->setVisible(false);
		this->_up1->setVisible(true);
		this->_up2->setVisible(true);
		this->_down1->setVisible(true);
		this->_down2->setVisible(true);
	});
	this->_options->onMousePress([this]{
		this->_soundButton.play();
	});
	this->_options->onClick([]{
		game->paused = true;
	});
	this->_resumeGame->onMousePress([this]{
		this->_soundButton.play();
	});
	this->_resumeGame->onClick([]{
		game->paused = false;
	});

	const std::string &prefix = params["cameo_prefix"];
	auto start = faction.getBase().getName();

	start.push_back(':');
	for (const auto &entry : std::filesystem::directory_iterator(base / "cameos")) {
		const auto &entryPath = entry.path();
		auto name = entryPath.filename().stem().string().substr(2);

		if (name.size() >= prefix.size() && name[prefix.size()] == '_') {
			if (!name.starts_with(prefix))
				continue;
			name = name.substr(prefix.size() + 1);
		}
		this->_cameos[start + name] = tgui::Texture{entryPath.native(), {}, {}, true};
	}

	this->_upgradeOrder.reserve(std::size(__upgradeOrder));
	for (auto &b : __upgradeOrder)
		this->_upgradeOrder.push_back(start + b);
	this->_buildOrder.reserve(std::size(__buildOrder));
	for (auto &b : __buildOrder)
		this->_buildOrder.push_back(start + b);
	this->_unitsOrder.reserve(std::size(__unitsOrder));
	for (auto &u : __unitsOrder)
		this->_unitsOrder.push_back(start + u);
	this->_loadEvaSounds(base, params["eva_folder"]);
}

D2KUI::~D2KUI() noexcept = default;

bool D2KUI::_hasUpgrades()
{
	auto it = this->state->queues.find(this->_faction.getBase().getName() + ":upgrade");

	if (it == this->state->queues.end())
		return false;
	return std::ranges::any_of(it->second | std::ranges::views::values, [](const PlayerQueue &q){
		return q.buildable && q.unlocked;
	});
}

static void updateList(
	const std::filesystem::path &base,
	const std::unordered_map<std::string, PlayerQueue> &list,
	const std::vector<std::string> &order,
	tgui::ScrollablePanel::Ptr panel,
	std::vector<std::string> &added,
	std::unordered_map<std::string, tgui::Texture> &cameos,
	const std::function<void (const std::string &id, bool rightClick)> &&queueCb
)
{
	bool needGenerate = false;

	for (auto &b : order) {
		auto it = list.find(b);
		bool wasAdded = std::ranges::find(added, b) != added.end();
		bool mustBeAdded = it != list.end() && it->second.unlocked;

		if (wasAdded != mustBeAdded) {
			needGenerate = true;
			break;
		}
	}
	if (needGenerate) {
		unsigned scroll = panel->getVerticalScrollbar()->getValue();
		std::string last;

		panel->removeAllWidgets();
		added.clear();
		for (auto &b : order) {
			auto it = list.find(b);

			if (it == list.end() || !it->second.unlocked)
				continue;

			auto pic = D2KCameo::create(base);
			auto r = pic->getRenderer();
			auto size = cameos[b].getImageSize();

			if (!last.empty()) {
				std::string pos;

				pos.reserve(21 + 6);
				pos += "cameo";
				pos += last;
				pos += ".y + cameo";
				pos += last;
				pos += ".h - 1"; // "cameo" + last + ".y + cameo" + last + ".h - 1"
				pic->setPosition({0, tgui::Layout{pos}});
			}
			last = b;
			pic->setSize("&.w", size.y + 1);
			pic->onMousePress([queueCb, b]{ queueCb(b, false); });
			pic->onRightMousePress([queueCb, b]{ queueCb(b, true); });
			r->setTexture(cameos[b]);
			panel->add(pic, "cameo" + b);
			added.push_back(b);
		}
		panel->getVerticalScrollbar()->setValue(scroll);
	}

	for (auto &b : order) {
		auto it = list.find(b);

		if (it == list.end() || !it->second.unlocked)
			continue;

		auto w = panel->get<D2KCameo>("cameo" + std::string(b));

		w->setOnHold(it->second.paused);
		if (!it->second.buildable) {
			w->setFill(0);
			w->setReady(false);
			continue;
		}
		if (it->second.buildProgress == 0) {
			w->setFill(1);
			w->setReady(false);
		} else {
			w->setFill(it->second.buildProgress / static_cast<float>(it->second.buildTime));
			w->setReady(it->second.buildProgress == it->second.buildTime);
		}
	}
}

static void updateList2(
	const std::filesystem::path &base,
	const std::unordered_map<std::string, PlayerQueue> &list,
	const std::vector<std::string> &order,
	tgui::ScrollablePanel::Ptr panel,
	std::vector<std::string> &added,
	std::unordered_map<std::string, tgui::Texture> &cameos,
	const std::function<void (const std::string &id, bool rightClick)> &&queueCb
)
{
	bool needGenerate = false;

	for (auto &b : order) {
		auto it = list.find(b);
		bool wasAdded = std::ranges::find(added, b) != added.end();
		bool mustBeAdded = it != list.end() && it->second.unlocked;

		if (wasAdded != mustBeAdded) {
			needGenerate = true;
			break;
		}
	}
	if (needGenerate) {
		unsigned scroll = panel->getVerticalScrollbar()->getValue();
		std::string last;

		panel->removeAllWidgets();
		added.clear();
		for (size_t i = 0; i < order.size(); i++) {
			auto &b = order[i];
			auto it = list.find(b);

			if (it == list.end() || !it->second.unlocked)
				continue;

			auto pos = b.find('.');
			auto pic = D2KCameo::create(base);
			auto r = pic->getRenderer();
			auto &cameo = cameos[pos == std::string::npos ? b : b.substr(0, pos)];

			pic->setPosition({i % 2 * 60, i / 2 * 47});
			last = b;
			pic->setSize(60, 48);
			pic->onMousePress([queueCb, b]{ queueCb(b, false); });
			pic->onRightMousePress([queueCb, b]{ queueCb(b, true); });
			r->setTexture(cameo);
			panel->add(pic, "cameo" + b);
			added.push_back(b);
		}
		panel->getVerticalScrollbar()->setValue(scroll);
	}

	for (auto &b : order) {
		auto it = list.find(b);

		if (it == list.end() || !it->second.unlocked)
			continue;

		auto w = panel->get<D2KCameo>("cameo" + std::string(b));

		w->setOnHold(it->second.paused);
		if (!it->second.buildable) {
			w->setFill(0);
			w->setReady(false);
			continue;
		}
		if (it->second.buildProgress == 0) {
			w->setFill(1);
			w->setReady(false);
		} else {
			w->setFill(it->second.buildProgress / static_cast<float>(it->second.buildTime));
			w->setReady(it->second.buildProgress == it->second.buildTime);
		}
	}
}

static void updateList3(
	const std::filesystem::path &base,
	const std::vector<std::unordered_map<std::string, PlayerQueue> *> &&list,
	const std::vector<std::string> &order,
	tgui::ScrollablePanel::Ptr panel,
	std::vector<std::string> &added,
	std::unordered_map<std::string, tgui::Texture> &cameos,
	const std::function<void (const std::string &id, size_t i, bool rightClick)> &&queueCb
)
{
	bool needGenerate = false;

	for (auto &b : order) {
		size_t i = 0;
		bool mustBeAdded = false;
		std::unordered_map<std::string, PlayerQueue>::iterator it;

		while (i != list.size()) {
			it = list[i]->find(b);
			if (it != list[i]->end() && it->second.unlocked)
				break;
			i++;
		}
		if (i == list.size())
			mustBeAdded = false;
		else if (it->second.unlocked)
			mustBeAdded = true;

		bool wasAdded = std::ranges::find(added, b) != added.end();

		if (wasAdded != mustBeAdded) {
			needGenerate = true;
			break;
		}
	}
	if (needGenerate) {
		unsigned scroll = panel->getVerticalScrollbar()->getValue();
		std::string last;

		panel->removeAllWidgets();
		added.clear();
		for (auto &b : order) {
			size_t i = 0;

			while (i != list.size()) {
				auto it = list[i]->find(b);

				if (it != list[i]->end() && it->second.unlocked)
					break;
				i++;
			}
			if (i == list.size())
				continue;

			auto pic = D2KCameo::create(base);
			auto r = pic->getRenderer();
			auto size = cameos[b].getImageSize();

			if (!last.empty()) {
				std::string pos;

				pos.reserve(21 + 6);
				pos += "cameo";
				pos += last;
				pos += ".y + cameo";
				pos += last;
				pos += ".h - 1"; // "cameo" + last + ".y + cameo" + last + ".h - 1"
				pic->setPosition({0, tgui::Layout{pos}});
			}
			last = b;
			pic->setSize("&.w", size.y + 1);
			pic->onMousePress([queueCb, b, i]{ queueCb(b, i, false); });
			pic->onRightMousePress([queueCb, b, i]{ queueCb(b, i, true); });
			r->setTexture(cameos[b]);
			panel->add(pic, "cameo" + b);
			added.push_back(b);
		}
		panel->getVerticalScrollbar()->setValue(scroll);
	}

	for (auto &b : order) {
		size_t i = 0;
		std::unordered_map<std::string, PlayerQueue>::iterator it;

		while (i != list.size()) {
			it = list[i]->find(b);
			if (it != list[i]->end() && it->second.unlocked)
				break;
			i++;
		}
		if (i == list.size())
			continue;

		auto w = panel->get<D2KCameo>("cameo" + std::string(b));

		w->setOnHold(it->second.paused);
		if (!it->second.buildable) {
			w->setFill(0);
			w->setReady(false);
			continue;
		}
		if (it->second.buildProgress == 0) {
			w->setFill(1);
			w->setReady(false);
		} else {
			w->setFill(it->second.buildProgress / static_cast<float>(it->second.buildTime));
			w->setReady(it->second.buildProgress == it->second.buildTime);
		}
	}
}

void D2KUI::update()
{
	auto it = this->_selectedObjects.begin();

	while (it != this->_selectedObjects.end())
		if ((*it)->isDisposable())
			it = this->_selectedObjects.erase(it);
		else
			++it;

	UI::update();

	unsigned i = 0;
	unsigned credits = std::min(99999999U, this->state->getCombinedMoney("base:credits"));
	auto &bQueue = this->state->queues[this->_faction.getBase().getName() + ":building"];
	auto &pQueue = this->state->queues[this->_faction.getBase().getName() + ":upgrade"];
	auto &iQueue = this->state->queues[this->_faction.getBase().getName() + ":infantry"];
	auto &lvQueue = this->state->queues[this->_faction.getBase().getName() + ":light_vehicule"];
	auto &hvQueue = this->state->queues[this->_faction.getBase().getName() + ":heavy_vehicule"];
	auto &placement = this->_faction.getPlacementCursors();
	auto &anim = this->_placementAnimation;

	if (!placement[0].empty()) {
		anim[0] = (anim[0] + 1) % placement[0].size();
		anim[1] = (anim[1] + 1) % placement[1].size();
		anim[2] = (anim[2] + 1) % placement[2].size();
	}

	do {
		if (credits != 0 || i == 0)
			this->_creditDigits[i]->getRenderer()->setTexture(this->_creditRawDigits[credits % 10]);
		else
			this->_creditDigits[i]->getRenderer()->setTexture(this->_creditRawDigits[10]);
		credits /= 10;
		i++;
	} while (i < 8);

	updateList(
		this->_faction.getBase().getBase(),
		bQueue, this->_buildOrder,
		this->_buildQueue, this->_buildQueueAdded,
		this->_cameos,
		[this, &bQueue](const std::string &id, bool rightClick){
			if (rightClick) {
				this->_eventQueue.emplace_back(CancelBuildEvent{
					id, this->_faction.getBase().getName() + ":building"
				});
				this->_placementBuilding = nullptr;
			} else if (bQueue[id].buildProgress == bQueue[id].buildTime) {
				auto &map = this->_faction.getBuildings();
				auto it = map.find(id);

				if (it != map.end())
					this->_placementBuilding = &it->second;
				else
					game->logger.debug("Cannot find " + id);
			} else
				this->_eventQueue.emplace_back(StartBuildEvent{
					id, this->_faction.getBase().getName() + ":building"
				});
		}
	);
	updateList2(
		this->_faction.getBase().getBase(),
		pQueue, this->_upgradeOrder,
		this->_upgradeQueue, this->_upgradeQueueAdded,
		this->_cameos,
		[this](const std::string &id, bool rightClick){
			if (rightClick)
				this->_eventQueue.emplace_back(CancelBuildEvent{
					id, this->_faction.getBase().getName() + ":upgrade"
				});
			else
				this->_eventQueue.emplace_back(StartBuildEvent{
					id, this->_faction.getBase().getName() + ":upgrade"
				});
		}
	);
	updateList3(
		this->_faction.getBase().getBase(),
		{&iQueue, &lvQueue, &hvQueue}, this->_unitsOrder,
		this->_unitsQueue, this->_unitsQueueAdded,
		this->_cameos,
		[&iQueue, &lvQueue, &hvQueue, this](const std::string &id, size_t i, bool rightClick){
			auto queue = i == 0 ? iQueue : i == 1 ? lvQueue : hvQueue;
			auto queueId = this->_faction.getBase().getName() + (i == 0 ? ":infantry" : i == 1 ? ":light_vehicule" : ":heavy_vehicule");

			if (rightClick)
				this->_eventQueue.emplace_back(CancelBuildEvent{id, queueId});
			else
				this->_eventQueue.emplace_back(StartBuildEvent{id, queueId});
		}
	);

	float percent = ceil(100 + (static_cast<float>(this->state->producedPower) - this->state->consumedPower) / 2);

	this->_power->setSize({10, percent});
	this->_greenPower->setVisible(percent >= 100);
	this->_yellowPower->setVisible(percent > 50 && percent < 100);
	this->_redPower->setVisible(percent <= 50);

	this->_upgrade->setVisible(this->_hasUpgrades());
	this->_updateCursor();
}

void D2KUI::_displaySelectedObjectUI(const GameObject &object)
{
	auto select = object.getSelectBox();
	Vector2i size = select.size;
	Vector2i top = select.position;
	sf::Sprite sprite{this->_selectCorners[0]};
	sf::RectangleShape rect;

	top.x = 0;
	top += object.getPosition();
	rect.setPosition({top.x + 1.f, top.y + 1.f});
	rect.setSize({size.x - 2.f, 2});
	rect.setOutlineThickness(1);
	rect.setOutlineColor(sf::Color::Black);
	rect.setFillColor(sf::Color{0, 0, 0, 128});
	game->screen->draw(rect);

	rect.setSize({(size.x - 2.f) * object.animationState.health, 2});
	if (object.animationState.health >= 0.5)
		rect.setFillColor(sf::Color::Green);
	else if (object.animationState.health >= 0.3)
		rect.setFillColor(sf::Color{0xFF, 0x80, 0x00});
	else
		rect.setFillColor(sf::Color::Red);
	game->screen->draw(rect);

	top.y += 4;
	sprite.setPosition(Vector2f(top.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[1], true);
	sprite.setPosition(Vector2f(top.x - this->_selectCorners[1].getSize().x + size.x, top.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[2], true);
	sprite.setPosition(Vector2f(top.x, top.y - this->_selectCorners[2].getSize().y + size.y));
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[3], true);
	sprite.setPosition(Vector2f(
		top.x - this->_selectCorners[3].getSize().x + size.x,
		top.y - this->_selectCorners[3].getSize().y + size.y
	));
	game->screen->draw(sprite);

	auto percent = object.getStorageDisplay();

	if (percent) {
		sf::Vector2f pos = {top.x + 2.f, top.y + size.y - 5.f};
		unsigned count = 7;

		if (size.x >= 64)
			count = 15;
		rect.setSize({2, 2});
		for (size_t i = 0; i < count; i++) {
			rect.setPosition(pos);
			if (*percent * count > i && percent != 0)
				rect.setFillColor(sf::Color{0, 255, 0, 255});
			else
				rect.setFillColor(sf::Color{0, 0, 0, 128});
			game->screen->draw(rect);
			pos.x += 3;
		}
	}
}

bool D2KUI::_isSelectionTooFar() const
{
	if (DEBUG_MODE)
		return false;

	const std::set<Vector2i> &okTiles = this->_placementBuilding->getCheckTiles();
	auto &placementGrid = this->map->placementGrid[this->state->team];
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	if (okTiles.empty())
		return false;
	for (auto &pos : okTiles) {
		auto result = pos + gridPos;

		if (result.x < 0 || result.y < 0 || result.x >= static_cast<int>(this->map->gridSize.x) || result.y >= static_cast<int>(this->map->gridSize.y))
			continue;
		if (placementGrid[result.x + result.y * this->map->gridSize.x])
			return false;
	}
	return true;
}

void D2KUI::render()
{
	if (!this->_enabled)
		return;
	if (this->_placementBuilding != nullptr)
		this->_displayPlacementPreview();
	for (auto &b : this->_selectedObjects)
		this->_displaySelectedObjectUI(*b);
}

void D2KUI::_displayPlacementPreview()
{
	auto &placement = this->_faction.getPlacementCursors();

	if (placement[0].empty())
		return;

	auto &tileData = this->_placementBuilding->getTileData();
	bool tooFar = this->_isSelectionTooFar();
	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	assert_exp(tileData);
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			uint8_t buildingMask = line[x];

			if ((buildingMask & (GameObject::MASK_PLACEMENT_REQUIRED | GameObject::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			Sprite *sprite = &placement[2][this->_placementAnimation[2]];
			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (
				full.x >= 0 && full.y >= 0
				&& static_cast<size_t>(full.x) < this->map->gridSize.x && static_cast<size_t>(full.y) < this->map->gridSize.y
			) {
				if (tooFar || !this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) || (
					this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) &&
					(buildingMask & GameObject::MASK_PLACEMENT_CONCRETE) &&
					!(buildingMask & GameObject::MASK_PLACEMENT_REQUIRED)
				) || !this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize))
					sprite = &placement[2][this->_placementAnimation[2]];
				else if (this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE) || (buildingMask & GameObject::MASK_PLACEMENT_CONCRETE))
					sprite = &placement[0][this->_placementAnimation[0]];
				else
					sprite = &placement[1][this->_placementAnimation[1]];
			}
			sprite->setPosition(this->map->topLeftCoordToAbs(full));
			game->screen->draw(*sprite);
		}
	}
}

void D2KUI::_updateCursor()
{
	Vector2i pos = this->map->absToTileCoord(this->_placementPos);

	if (this->_placementBuilding != nullptr) {
		this->_cursor = "normal";
		return;
	}
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer);

	if (!this->_objectBuffer.empty()) {
		// TODO: Handle non snap-to-grid units/buildings
		if (this->_selectedObjects.size() == 1) {
			auto obj = *this->_selectedObjects.begin();

			if (obj->isDeployable() && std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()) {
				if (obj->canDeploy())
					this->_cursor = "deploy";
				else
					this->_cursor = "no_deploy";
				return;
			}
		}
		this->_cursor = "select";
		return;
	}

	bool canMove = !this->_selectedObjects.empty();

	for (auto o : this->_selectedObjects) {
		if (!o->canMove())
			canMove = false;
	}

	if (!canMove)
		this->_cursor = "normal";
	else {
		auto obj = *this->_selectedObjects.begin();
		auto map = obj->map;
		auto relPos = map->absToTileCoord(this->_placementPos);
		auto tl = this->map->topLeftCoordToAbs(pos);
		auto passFlags = obj->getPassFlags();

		// TODO: Handle non snap-to-grid units
		if (obj->getResourceStorage() != 0 && map->hasAnyGridBits(relPos, GRIDMASK_RESOURCE)) {
			auto res = GRID_RESOURCE_TYPE(map->grid[relPos.x + relPos.y * map->gridSize.x]);

			if (res != 0) {
				this->_cursor = "attack";
				return;
			}
		}
		if (
			!this->manager->isAreaFree(tl, map->tileSize) ||
			!map->hasAnyGridBits(relPos, passFlags)
		)
			this->_cursor = "no_move";
		else if (
			map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_INFANTRY_CAN_PASS) ||
			map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_VEHICULES_CAN_PASS) || (
				map->hasExactlySomeGridBits(relPos, GRIDMASK_ALL_CAN_PASS, GRIDMASK_SHIP_CAN_PASS) &&
				passFlags != GRIDMASK_SHIP_CAN_PASS
			)
		)
			this->_cursor = "move_inf";
		else
			this->_cursor = "move";
	}
}

void D2KUI::_handleTryPlace()
{
	auto &tileData = this->_placementBuilding->getTileData();
	auto &placement = this->_faction.getPlacementCursors();
	bool foundGood = false;

	if (placement[0].empty())
		return;
	if (this->_isSelectionTooFar()) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	auto gridPos = this->map->absToTileCoord(this->_placementPos);

	assert_exp(tileData);
	for (size_t y = 0; y < tileData->size(); y++) {
		auto &line = (*tileData)[y];

		for (size_t x = 0; x < line.size(); x++) {
			uint8_t buildingMask = line[x];

			if ((buildingMask & (GameObject::MASK_PLACEMENT_REQUIRED | GameObject::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			Vector2i full = {
				static_cast<int>(x) + gridPos.x,
				static_cast<int>(y) + gridPos.y
			};

			if (full.x < 0 || full.y < 0 || static_cast<size_t>(full.x) >= this->map->gridSize.x || static_cast<size_t>(full.y) >+ this->map->gridSize.y)
				continue;
			if (buildingMask & GameObject::MASK_PLACEMENT_REQUIRED) {
				if (
					!this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) ||
					!this->manager->isAreaFree(this->map->topLeftCoordToAbs(full), this->map->tileSize)
				) {
					this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
					return;
				}
				foundGood = true;
			} else if (buildingMask & GameObject::MASK_PLACEMENT_CONCRETE) {
				if (this->map->hasAllGridBits(full, GRIDMASK_CAN_BUILD) && !this->map->hasAllGridBits(full, GRIDMASK_HAS_CONCRETE))
					foundGood = true;
			}
		}
	}
	if (!foundGood) {
		this->playEVA(EVA_CANNOT_BUILD, 600, 2, 60, "");
		return;
	}

	this->_eventQueue.emplace_back(PlaceBuildEvent{
		this->_placementBuilding->getId(),
		this->_faction.getBase().getName() + ":building",
		sf::Vector2u(this->_placementPos.x, this->_placementPos.y)
	});
	this->_placementBuilding = nullptr;
}

void D2KUI::onCancel(Vector2i absPos)
{
	if (this->_placementBuilding)
		this->_placementBuilding = nullptr;
	else if (!this->_selectedObjects.empty())
		this->_selectedObjects.clear();
	this->mouseMovedToTile(absPos);
}

void D2KUI::tileClicked(Vector2i absPos)
{
	this->_placementPos = absPos;
	if (this->_placementBuilding != nullptr)
		return this->_handleTryPlace();
	this->_objectBuffer.clear();
	this->manager->getObjectsInArea(this->_placementPos, {1, 1}, this->_objectBuffer);
	// TODO: Hardcoded key
	if (!this->_selectedObjects.empty() && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
		if (this->_selectedObjects.size() == 1) {
			auto obj = *this->_selectedObjects.begin();

			if (obj->isDeployable() && std::find(this->_objectBuffer.begin(), this->_objectBuffer.end(), obj) != this->_objectBuffer.end()) {
				if (!obj->canDeploy())
					this->playEVA(UI::EVA_CANNOT_DEPLOY, 300, 2, 300, "");
				else
					this->_eventQueue.emplace_back(DeployUnitEvent{obj->index});
				return;
			}
		}

		if (!this->_objectBuffer.empty()) {
			this->_selectedObjects.clear();
			this->_selectedObjects.emplace(this->_objectBuffer.back());
			this->_objectBuffer.back()->select();
		} else if (!this->_selectedObjects.empty()) {
			HarvestUnitEvent hevent;
			MoveUnitEvent event;

			event.position = this->_placementPos;
			event.indices.reserve(this->_selectedObjects.size());
			hevent.position = this->_placementPos;
			hevent.indices.reserve(this->_selectedObjects.size());
			for (auto s : this->_selectedObjects) {
				auto map = s->map;
				auto relPos = map->absToTileCoord(this->_placementPos);

				// TODO: Handle non snap-to-grid units
				if (s->getResourceStorage() != 0 && map->hasAnyGridBits(relPos, GRIDMASK_RESOURCE)) {
					auto res = GRID_RESOURCE_TYPE(map->grid[relPos.x + relPos.y * map->gridSize.x]);

					if (res != 0) {
						hevent.indices.push_back(s->index);
						continue;
					}
				}
				event.indices.push_back(s->index);
			}
			if (!event.indices.empty())
				this->_eventQueue.emplace_back(event);
			if (!hevent.indices.empty())
				this->_eventQueue.emplace_back(hevent);
		}
	} else if (!this->_objectBuffer.empty()) {
		auto b = this->_objectBuffer.back();
		auto it = this->_selectedObjects.find(b);

		if (it == this->_selectedObjects.end()) {
			this->_selectedObjects.emplace(b);
			b->select();
		} else
			this->_selectedObjects.erase(it);
	}
}

void D2KUI::mouseMovedToTile(Vector2i absPos)
{
	this->_placementPos = absPos;
}

void D2KUI::mouseOnUi()
{
	this->_cursor = "normal";
}

void D2KUI::playEVA(UI::EVASound sound, unsigned int expireIn, unsigned int priority, unsigned int cooldown, const std::string &args)
{
	if (sound == EVA_ALREADY_BUILDING)
		return;
	if (sound == EVA_BUILDING) {
		if (args == this->_faction.getBase().getName() + ":upgrade")
			sound = EVA_UPGRADING;
		else if (args == this->_faction.getBase().getName() + ":infantry")
			sound = EVA_TRAINING;
	} else if (sound == EVA_BUILDING_DONE) {
		if (args == this->_faction.getBase().getName() + ":upgrade")
			// No built sound for upgrade queue
			return;
		else if (args != this->_faction.getBase().getName() + ":building")
			sound = EVA_UNIT_READY;
	}
	UI::playEVA(sound, expireIn, priority, cooldown, args);
}
