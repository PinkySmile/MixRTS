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

D2KUI::D2KUI(const Faction &faction, const std::filesystem::path &base, const nlohmann::json &params) :
	_faction(faction)
{
	auto path = base / params["path"].get<std::string>();
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
	this->_greenPower      = this->_power->get<tgui::Picture>("GreenPower");
	this->_yellowPower     = this->_power->get<tgui::Picture>("YellowPower");
	this->_redPower        = this->_power->get<tgui::Picture>("RedPower");
	this->_buildQueue      = this->_panel->get<tgui::ScrollablePanel>("BuildingQueue");
	this->_unitsQueue      = this->_panel->get<tgui::ScrollablePanel>("UnitsQueue");
	this->_creditDigits[0] = this->_panel->get<tgui::Picture>("Credit0");
	this->_creditDigits[1] = this->_panel->get<tgui::Picture>("Credit1");
	this->_creditDigits[2] = this->_panel->get<tgui::Picture>("Credit2");
	this->_creditDigits[3] = this->_panel->get<tgui::Picture>("Credit3");
	this->_creditDigits[4] = this->_panel->get<tgui::Picture>("Credit4");
	this->_creditDigits[5] = this->_panel->get<tgui::Picture>("Credit5");
	this->_creditDigits[6] = this->_panel->get<tgui::Picture>("Credit6");
	this->_creditDigits[7] = this->_panel->get<tgui::Picture>("Credit7");
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
	this->_main1->setVisible(false);
	this->_main2->setVisible(false);
	this->_upgrade->setVisible(false);
	this->_starport->setVisible(false);
	this->_buildQueue->getVerticalScrollbar()->setScrollAmount(47);
	this->_unitsQueue->getVerticalScrollbar()->setScrollAmount(47);

	assert_exp(this->_move.loadFromFile(base / "sounds/SIDEBAR1.WAV"));
	assert_exp(this->_button.loadFromFile(base / "sounds/BUTTON1.WAV"));
	assert_exp(this->_moveBlocked.loadFromFile(base / "sounds/ENDLIST1.WAV"));

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

	this->_buildOrder.reserve(std::size(__buildOrder));
	for (auto &b : __buildOrder)
		this->_buildOrder.push_back(start + b);
	this->_unitsOrder.reserve(std::size(__unitsOrder));
	for (auto &u : __unitsOrder)
		this->_unitsOrder.push_back(start + u);
}

D2KUI::~D2KUI() noexcept = default;

static void updateList(
	const std::filesystem::path &base,
	const std::unordered_map<std::string, PlayerQueue> &list,
	const std::vector<std::string> &order,
	tgui::ScrollablePanel::Ptr panel,
	std::vector<std::string> &added,
	std::unordered_map<std::string, tgui::Texture> &cameos,
	const std::function<void (const std::string &id)> &&queueCb
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
			pic->onClick([queueCb, b]{ queueCb(b); });
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
	unsigned i = 0;
	unsigned credits = std::min(99999999U, this->state->getCombinedMoney("base:credits"));
	auto &bQueue = this->state->queues["d2k:building"];
	auto &uQueue = this->state->queues["units"];
	auto &placement = this->_faction.getPlacementCursors();
	auto &anim = this->_placementAnimation;

	this->_cursors.at(this->_cursor).update();
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
		[this](const std::string &id){
			auto &map = this->_faction.getBuildings();
			auto it = map.find(id);

			if (it != map.end())
				this->_placementBuilding = &it->second;
			else
				game->logger.debug("Cannot find " + id);
		}
	);
	updateList(
		this->_faction.getBase().getBase(),
		uQueue, this->_unitsOrder,
		this->_unitsQueue, this->_unitsQueueAdded,
		this->_cameos,
		[](const std::string &){}
	);

	float percent;

	if (this->state->producedPower == 0)
		percent = 0;
	else if (this->state->consumedPower == 0)
		percent = 1 + this->state->producedPower / 200.f;
	else
		percent = static_cast<float>(this->state->producedPower) / this->state->consumedPower;

	this->_power->setSize({10, ceil(100 * percent)});
	this->_greenPower->setVisible(percent >= 1);
	this->_yellowPower->setVisible(percent >= 0.7 && percent < 1);
	this->_redPower->setVisible(percent < 0.7);
}

void D2KUI::_displaySelectedBuildingUI(const Building &building)
{
	Vector2u size = building.getSize();
	Vector2f top = building.getBaseAnims().front().getAnims().front().getOffset();
	sf::Sprite sprite{this->_selectCorners[0]};
	sf::RectangleShape rect;

	top.x = 0;
	top += building.getPosition();
	rect.setPosition({top.x + 1, top.y + 1});
	rect.setSize({size.x - 2.f, 2});
	rect.setOutlineThickness(1);
	rect.setOutlineColor(sf::Color::Black);
	rect.setFillColor(sf::Color{0, 0, 0, 128});
	game->screen->draw(rect);

	rect.setSize({(size.x - 2.f) * building.animationState.health, 2});
	if (building.animationState.health >= 0.5)
		rect.setFillColor(sf::Color::Green);
	else if (building.animationState.health >= 0.3)
		rect.setFillColor(sf::Color{0xFF, 0x80, 0x00});
	else
		rect.setFillColor(sf::Color::Red);
	game->screen->draw(rect);

	top.y += 4;
	sprite.setPosition({top.x, top.y});
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[1], true);
	sprite.setPosition({top.x - this->_selectCorners[1].getSize().x + size.x, top.y});
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[2], true);
	sprite.setPosition({top.x, top.y - this->_selectCorners[2].getSize().y + size.y});
	game->screen->draw(sprite);

	sprite.setTexture(this->_selectCorners[3], true);
	sprite.setPosition({
		top.x - this->_selectCorners[3].getSize().x + size.x,
		top.y - this->_selectCorners[3].getSize().y + size.y
	});
	game->screen->draw(sprite);
}

void D2KUI::render()
{
	if (!this->_enabled)
		return;
	if (this->_placementBuilding != nullptr) {
		this->_placementSprite.setTexture(this->_placementTexture.getTexture(), true);
		this->_placementSprite.setPosition({
			static_cast<float>(this->_placementPos.x * 32),
			static_cast<float>(this->_placementPos.y * 32)
		});
		game->screen->draw(this->_placementSprite);
	}


	for (auto &b : this->_selectedBuildings)
		this->_displaySelectedBuildingUI(*b);
}

void D2KUI::renderAbove()
{
	this->_cursors.at(this->_cursor).render();
}

void D2KUI::_updatePlacementTexture()
{
	auto &tileData = this->_placementBuilding->getTileData();
	auto &placement = this->_faction.getPlacementCursors();

	if (placement[0].empty())
		return;

	assert_exp(this->_placementTexture.resize(sf::Vector2u(tileData.front().size() * 32, tileData.size() * 32)));
	this->_placementTexture.clear(sf::Color::Transparent);
	for (size_t y = 0; y < tileData.size(); y++) {
		auto &line = tileData[y];

		for (size_t x = 0; x < line.size(); x++) {
			uint8_t buildingMask = line[x];

			if ((buildingMask & (Building::MASK_PLACEMENT_REQUIRED | Building::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			Sprite *sprite = &placement[2][this->_placementAnimation[2]];
			int fullX = static_cast<int>(x) + this->_placementPos.x;
			int fullY = static_cast<int>(y) + this->_placementPos.y;

			if (fullX >= 0 && fullY >= 0 && static_cast<size_t>(fullX) < this->map->gridSize.x && static_cast<size_t>(fullY) < this->map->gridSize.y) {
				uint32_t gridMask = this->map->grid[fullX + fullY * this->map->gridSize.x];

				if (!(gridMask & GRIDMASK_IS_FREE) || (
					(gridMask & GRIDMASK_HAS_CONCRETE) &&
					(buildingMask & Building::MASK_PLACEMENT_CONCRETE) &&
					!(buildingMask & Building::MASK_PLACEMENT_REQUIRED)
				))
					sprite = &placement[2][this->_placementAnimation[2]];
				else if ((gridMask & GRIDMASK_HAS_CONCRETE) || (buildingMask & Building::MASK_PLACEMENT_CONCRETE))
					sprite = &placement[0][this->_placementAnimation[0]];
				else
					sprite = &placement[1][this->_placementAnimation[1]];
			}
			sprite->setPosition({x * 32.f, y * 32.f});
			this->_placementTexture.draw(*sprite);
		}
	}
	this->_placementTexture.display();
}

void D2KUI::_handleTryPlace()
{
	auto &tileData = this->_placementBuilding->getTileData();
	auto &placement = this->_faction.getPlacementCursors();
	bool foundGood = false;

	if (placement[0].empty())
		return;

	for (size_t y = 0; y < tileData.size(); y++) {
		auto &line = tileData[y];

		for (size_t x = 0; x < line.size(); x++) {
			uint8_t buildingMask = line[x];

			if ((buildingMask & (Building::MASK_PLACEMENT_REQUIRED | Building::MASK_PLACEMENT_CONCRETE)) == 0)
				continue;

			uint32_t gridMask = 0;
			int fullX = static_cast<int>(x) + this->_placementPos.x;
			int fullY = static_cast<int>(y) + this->_placementPos.y;

			if (fullX >= 0 && fullY >= 0 && static_cast<size_t>(fullX) < this->map->gridSize.x && static_cast<size_t>(fullY) < this->map->gridSize.y)
				gridMask = this->map->grid[fullX + fullY * this->map->gridSize.x];
			if (buildingMask & Building::MASK_PLACEMENT_REQUIRED) {
				if (!(gridMask & GRIDMASK_IS_FREE))
					return;
				foundGood = true;
			} else if (buildingMask & Building::MASK_PLACEMENT_CONCRETE) {
				if ((gridMask & GRIDMASK_IS_FREE) && !(gridMask & GRIDMASK_HAS_CONCRETE))
					foundGood = true;
			}
		}
	}
	if (!foundGood)
		return;

	this->_eventQueue.emplace_back(PlaceBuildEvent{
		this->_placementBuilding->getId(),
		this->_faction.getBase().getName() + ":building",
		sf::Vector2u(this->_placementPos.x, this->_placementPos.y)
	});
	this->_placementBuilding = nullptr;
}

void D2KUI::onCancel()
{
	if (this->_placementBuilding)
		this->_placementBuilding = nullptr;
	else if (!this->_selectedBuildings.empty())
		this->_selectedBuildings.clear();
}

void D2KUI::tileClicked(Vector2i pos)
{
	this->_placementPos = pos;
	if (this->_placementBuilding != nullptr)
		return this->_handleTryPlace();
	// TODO: Hardcoded key
	if (!this->_selectedBuildings.empty() && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
		this->_selectedBuildings.clear();
		for (auto b : this->state->buildingHitboxes[pos.to<unsigned>()])
			this->_selectedBuildings.emplace(b);
	} else {
		for (auto b : this->state->buildingHitboxes[pos.to<unsigned>()]) {
			auto it = this->_selectedBuildings.find(b);

			if (it == this->_selectedBuildings.end())
				this->_selectedBuildings.emplace(b);
			else
				this->_selectedBuildings.erase(it);
		}
	}
}

void D2KUI::mouseMovedToTile(Vector2i pos)
{
	this->_placementPos = pos;
	if (this->_placementBuilding != nullptr) {
		this->_cursor = "normal";
		this->_updatePlacementTexture();
	} else {
		auto it = this->state->buildingHitboxes.find(pos.to<unsigned>());

		if (it != this->state->buildingHitboxes.end())
			this->_cursor = "select";
		else
			this->_cursor = "normal";
	}
}

void D2KUI::mouseOnUi()
{
	this->_cursor = "normal";
}
