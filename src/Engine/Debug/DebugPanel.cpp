//
// Debug tools: the TGUI window (F3) driving DebugOptions and the profiler.
//

#include <cstdio>
#include <memory>
#include <algorithm>
#include "DebugPanel.hpp"
#include "Debug/Profiler.hpp"
#include "Data/GameManager.hpp"
#include "Data/Player.hpp"
#include "Data/Faction.hpp"
#include "Resources/Game.hpp"

DebugPanel::DebugPanel(GameManager &manager) :
	_manager(manager)
{
	this->_window = tgui::ChildWindow::create("Debug tools (F3)", tgui::ChildWindow::TitleButton::Close);
	this->_window->setSize(340, 460);
	this->_window->setPosition("max(&.w - w - 170, 0)", 30);
	this->_window->onClosing([this](bool *abort) {
		// Hide instead of destroying the window
		*abort = true;
		this->_window->setVisible(false);
	});
	this->_currentPlayer = manager.getControlledPlayer();

	this->_tabs = tgui::Tabs::create();
	this->_tabs->setPosition(0, 0);
	this->_tabs->setSize("&.w", 26);
	this->_tabs->setTextSize(13);
	this->_window->add(this->_tabs);

#ifdef _DEBUG
	this->_buildGeneralPage();
	this->_buildDisplayPage();
	this->_buildObjectsPage();
	this->_buildCombatPage();
#endif
	this->_perfPage = this->_pages.size();
	this->_buildPerformancePage();

	auto showSelectedPage = [this]{
		int selected = this->_tabs->getSelectedIndex();

		for (size_t i = 0; i < this->_pages.size(); i++)
			this->_pages[i].panel->setVisible(static_cast<int>(i) == selected);
	};

	this->_tabs->onTabSelect(showSelectedPage);
	this->_tabs->select(this->_pages.size() > 1 ? 0 : this->_perfPage);
	showSelectedPage();
	this->_window->setVisible(false);
	game->gui.add(this->_window);
}

DebugPanel::~DebugPanel()
{
	if (game)
		game->gui.remove(this->_window);
}

void DebugPanel::toggle()
{
	this->_window->setVisible(!this->_window->isVisible());
	if (this->_window->isVisible())
		this->_window->moveToFront();
}

bool DebugPanel::isVisible() const
{
	return this->_window->isVisible();
}

DebugPanel::Page &DebugPanel::_addPage(const std::string &name)
{
	auto &page = this->_pages.emplace_back();

	page.panel = tgui::ScrollablePanel::create({"100%", "&.h - 26"});
	page.panel->setPosition(0, 26);
	page.panel->getHorizontalScrollbar()->setPolicy(tgui::Scrollbar::Policy::Never);
	page.panel->getRenderer()->setBackgroundColor(tgui::Color{0, 0, 0, 90});
	this->_window->add(page.panel);
	this->_tabs->add(name, false);
	return page;
}

void DebugPanel::_addLabel(Page &page, const std::string &text, bool header)
{
	auto label = tgui::Label::create(text);

	label->setTextSize(header ? 14 : 12);
	if (header) {
		label->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
		page.y += 6;
	}
	label->setMaximumTextWidth(290);
	label->setPosition(8, page.y);
	page.panel->add(label);
	page.y += label->getSize().y + 4;
}

tgui::CheckBox::Ptr DebugPanel::_addCheck(Page &page, const std::string &text, bool initial, const std::function<void (bool)> &cb)
{
	auto box = tgui::CheckBox::create(text);

	box->setSize(18, 18);
	box->setTextSize(13);
	box->setPosition(8, page.y);
	box->setChecked(initial);
	box->onChange([cb](bool checked) {
		cb(checked);
	});
	page.panel->add(box);
	page.y += 24;
	return box;
}

tgui::CheckBox::Ptr DebugPanel::_addCheck(Page &page, const std::string &text, bool &value)
{
	return this->_addCheck(page, text, value, [&value](bool checked) {
		value = checked;
	});
}

tgui::ComboBox::Ptr DebugPanel::_addCombo(Page &page, const std::string &label, const std::vector<std::string> &items, size_t selected, const std::function<void (size_t)> &cb)
{
	auto combo = tgui::ComboBox::create();

	this->_addLabel(page, label);
	page.y -= 2;
	combo->setPosition(8, page.y);
	combo->setSize("&.w - 30", 22);
	combo->setTextSize(13);
	combo->setItemsToDisplay(10);
	for (size_t i = 0; i < items.size(); i++)
		combo->addItem(items[i], std::to_string(i));
	combo->setSelectedItemByIndex(selected);
	combo->onItemSelect([cb](int index) {
		if (index >= 0)
			cb(index);
	});
	page.panel->add(combo);
	page.y += 28;
	return combo;
}

tgui::Slider::Ptr DebugPanel::_addSlider(Page &page, const std::string &label, float min, float max, float step, float value, const std::function<void (float)> &cb)
{
	auto slider = tgui::Slider::create(min, max);
	auto text = tgui::Label::create();
	auto format = [label, step](float v) {
		char buffer[32];

		snprintf(buffer, sizeof(buffer), step < 1 ? "%.1f" : "%.0f", v);
		return label + ": " + buffer;
	};

	text->setTextSize(12);
	text->setText(format(value));
	text->setPosition(8, page.y);
	text->setSize("&.w - 30", 16);
	page.panel->add(text);
	page.y += 18;
	slider->setStep(step);
	slider->setValue(value);
	slider->setPosition(8, page.y);
	slider->setSize("&.w - 30", 14);
	slider->onValueChange([cb, text, format](float v) {
		text->setText(format(v));
		cb(v);
	});
	page.panel->add(slider);
	page.y += 24;
	return slider;
}

tgui::Button::Ptr DebugPanel::_addButton(Page &page, const std::string &text, const std::function<void ()> &cb)
{
	auto button = tgui::Button::create(text);

	button->setTextSize(13);
	button->setPosition(8, page.y);
	button->setSize("&.w - 30", 24);
	button->onPress([cb]{
		cb();
	});
	page.panel->add(button);
	page.y += 30;
	return button;
}

void DebugPanel::_buildGeneralPage()
{
	auto &page = this->_addPage("General");
	auto &opts = this->_manager.debugOptions;
	std::vector<std::string> views;
	std::vector<std::string> players;

	this->_addLabel(page, "F1: unlock all   F2: switch player\nF3: this panel   F5: save state   F11: pause   F12: step");

	for (auto &p : this->_manager.getPlayers())
		players.push_back("Player " + std::to_string(p.state.index) + " - " + p.getPrimaryFaction().getName());
	if (!players.empty())
		this->_playerCombo = this->_addCombo(page, "Controlled player (F2)", players, this->_manager.getControlledPlayer(), [this](size_t index) {
			if (!this->_refreshing) {
				this->_manager.setControlledPlayer(index);
				this->_currentPlayer = index;
			}
		});
	this->_unlockBox = this->_addCheck(page, "Everything unlocked (F1)", this->_manager.isDebugUnlocked(), [this](bool checked) {
		if (checked != this->_manager.isDebugUnlocked())
			this->_manager.toggleDebugUnlock();
	});
	this->_pausedBox = this->_addCheck(page, "Paused (F11)", this->_manager.isPaused(), [this](bool checked) {
		this->_manager.setPaused(checked);
	});
	this->_addButton(page, "Step one tick (F12)", [this]{
		this->_manager.requestStep();
	});
	this->_addButton(page, "Save state (F5)", [this]{
		this->_manager.requestStateSave();
	});
	this->_addCheck(page, "Limit camera to the map", opts.limitCamera);
	this->_addButton(page, "Rotate selected objects", [this]{
		this->_manager.debugRotateSelected();
	});

	// Grid views: "None" then every view of the game manager
	views.push_back(this->_manager.getGridViewLabel(255));
	for (size_t i = 0; i < this->_manager.getGridViewCount(); i++)
		views.push_back(this->_manager.getGridViewLabel(i));
	this->_addCombo(page, "Grid view", views, opts.gridView == 255 ? 0 : opts.gridView + 1, [this](size_t index) {
		this->_manager.debugOptions.gridView = index == 0 ? 255 : index - 1;
	});
}

void DebugPanel::_buildDisplayPage()
{
	auto &page = this->_addPage("Display");
	auto &opts = this->_manager.debugOptions;

	this->_addLabel(page, "Draw", true);
	this->_addCheck(page, "Terrain", opts.showTerrain);
	this->_addCheck(page, "Objects", opts.showObjects);
	this->_addCheck(page, "Effects", opts.showEffects);
	this->_addCheck(page, "Projectiles", opts.showProjectiles);
	this->_addCheck(page, "Building skirts", opts.showSkirts);
	this->_addCheck(page, "Player overlays (selection, UI previews)", opts.showPlayerOverlays);

	this->_addLabel(page, "Layers (objects, effects and projectiles)", true);
	for (int layer = -2; layer <= 2; layer++)
		this->_layerBoxes[layer + 2] = this->_addCheck(page, "Layer " + std::to_string(layer), opts.showLayers[layer + 2]);
	this->_addCombo(page, "Show only layer", {"All", "-2", "-1", "0", "1", "2"}, 0, [this](size_t index) {
		auto &options = this->_manager.debugOptions;

		for (size_t i = 0; i < options.showLayers.size(); i++) {
			bool visible = index == 0 || i == index - 1;

			options.showLayers[i] = visible;
			if (this->_layerBoxes[i] && this->_layerBoxes[i]->isChecked() != visible)
				this->_layerBoxes[i]->setChecked(visible);
		}
	});

	this->_addLabel(page, "Render order", true);
	this->_addLabel(page, "Only draws the objects whose render order key (bottom edge + altitude) is relative to the cursor.");
	this->_addCombo(
		page, "Filter",
		{"All objects", "Below the cursor", "Above the cursor", "Exactly at the cursor"},
		opts.renderFilter,
		[this](size_t index) {
			this->_manager.debugOptions.renderFilter = static_cast<RenderOrderFilter>(index);
		}
	);
	this->_addSlider(page, "Tolerance for \"exactly at\"", 0, 64, 1, opts.renderFilterTolerance, [this](float value) {
		this->_manager.debugOptions.renderFilterTolerance = value;
	});
	this->_addCheck(page, "Show the render order line", opts.showRenderOrderLine);
}

void DebugPanel::_buildObjectsPage()
{
	auto &page = this->_addPage("Objects");
	auto &opts = this->_manager.debugOptions;

	this->_addLabel(page, "Select objects with the player that is currently enabled (F2 to switch).");
	this->_addCheck(page, "Inspector: internal values of the selection", opts.inspector);
	this->_addCheck(page, "Short tag above every selected object", opts.inspectorTags);
	this->_addCheck(page, "Path finding path", opts.inspectorPath);
	this->_addCheck(page, "Select boxes", opts.inspectorBoxes);
}

void DebugPanel::_buildCombatPage()
{
	auto &page = this->_addPage("Combat");
	auto &opts = this->_manager.debugOptions;

	this->_addLabel(page, "Projectiles", true);
	this->_addCheck(page, "Trajectories, targets and ranges", opts.projectiles);
	this->_addLabel(page, "Warheads", true);
	this->_addCheck(page, "Show applied warheads", opts.warheads);
	this->_addCheck(page, "Pause on the tick of contact", opts.pauseOnWarhead);
	this->_addLabel(page, "The frame of contact is drawn filled and outlined in white, then fades. F12 steps one tick.");
	this->_addButton(page, "Clear warhead log", [this]{
		this->_manager._warheadHits.clear();
		this->_manager._currentHit = nullptr;
	});
}

void DebugPanel::_buildPerformancePage()
{
	auto &page = this->_addPage("Performance");
	auto &opts = this->_manager.debugOptions;
	Profiler &profiler = Profiler::get();

	this->_profilerBox = this->_addCheck(page, "Collect profiling data", Profiler::enabled, [](bool checked) {
		Profiler::enabled = checked;
	});
	this->_addCheck(page, "Show the performance graph", opts.perfGraph, [this](bool checked) {
		this->_manager.debugOptions.perfGraph = checked;
		if (checked && !Profiler::enabled) {
			Profiler::enabled = true;
			this->_profilerBox->setChecked(true);
		}
	});
	this->_perfViewCombo = this->_addCombo(
		page, "View",
		{"Sections (stacked bars)", "Subtree of a section", "Counters", "Breakdown table"},
		opts.perfView,
		[this](size_t index) {
			this->_manager.debugOptions.perfView = static_cast<PerfView>(index);
		}
	);
	this->_subtreeCombo = this->_addCombo(page, "Section shown by \"Subtree\"", {"(start collecting to list the sections)"}, 0, [this](size_t index) {
		if (this->_refreshing)
			return;
		if (index == 0 || index > this->_subtreeItems.size())
			return;
		this->_manager.debugOptions.perfSubtree = this->_subtreeItems[index - 1];
		this->_manager.debugOptions.perfView = PERFVIEW_SUBTREE;
		this->_perfViewCombo->setSelectedItemByIndex(PERFVIEW_SUBTREE);
	});
	this->_addCombo(page, "Table sorting", {"Average time", "Max time", "Calls", "Name (tree)"}, opts.perfSort, [this](size_t index) {
		this->_manager.debugOptions.perfSort = static_cast<PerfSort>(index);
	});
	this->_addSlider(page, "Frames shown", 30, PROFILER_HISTORY, 10, opts.perfFrames, [this](float value) {
		this->_manager.debugOptions.perfFrames = value;
	});

	this->_addLabel(page, "Capture", true);
	this->_frozenBox = this->_addCheck(page, "Freeze the graph", profiler.frozen, [](bool checked) {
		Profiler::get().frozen = checked;
	});
	{
		auto threshold = std::make_shared<float>(50);
		auto autoFreeze = std::make_shared<bool>(false);
		auto apply = [threshold, autoFreeze]{
			Profiler::get().autoFreezeMs = *autoFreeze ? *threshold : 0;
		};

		this->_addCheck(page, "Freeze when a frame is slower than:", false, [autoFreeze, apply](bool checked) {
			*autoFreeze = checked;
			apply();
		});
		this->_addSlider(page, "Threshold (ms)", 5, 500, 5, *threshold, [threshold, apply](float value) {
			*threshold = value;
			apply();
		});
	}
	this->_addButton(page, "Clear history", []{
		Profiler::get().clear();
	});
	this->_addLabel(page, "Hover a bar of the graph to see the section tree of that frame.");
}

void DebugPanel::_refreshSubtreeList()
{
	auto &opts = this->_manager.debugOptions;
	auto paths = Profiler::get().listPaths(opts.perfFrames);

	if (paths.empty() || paths == this->_subtreeItems)
		return;
	this->_refreshing = true;
	this->_subtreeItems = paths;
	this->_subtreeCombo->removeAllItems();
	this->_subtreeCombo->addItem("(select a section)", "0");
	for (size_t i = 0; i < paths.size(); i++) {
		size_t depth = std::count(paths[i].begin(), paths[i].end(), '/');
		size_t last = paths[i].rfind('/');

		this->_subtreeCombo->addItem(
			std::string(depth * 2, ' ') + (last == std::string::npos ? paths[i] : paths[i].substr(last + 1)),
			std::to_string(i + 1)
		);
	}

	auto it = std::find(paths.begin(), paths.end(), opts.perfSubtree);

	this->_subtreeCombo->setSelectedItemByIndex(it == paths.end() ? 0 : std::distance(paths.begin(), it) + 1);
	this->_refreshing = false;
}

void DebugPanel::refresh()
{
	if (!this->_window->isVisible())
		return;
	this->_refreshing = true;

	auto sync = [](const tgui::CheckBox::Ptr &box, bool value) {
		if (box && box->isChecked() != value)
			box->setChecked(value);
	};

	sync(this->_pausedBox, this->_manager.isPaused());
	sync(this->_unlockBox, this->_manager.isDebugUnlocked());
	sync(this->_frozenBox, Profiler::get().frozen);
	if (this->_playerCombo && this->_currentPlayer != this->_manager.getControlledPlayer())
		this->_playerCombo->setSelectedItemByIndex(this->_manager.getControlledPlayer());
	sync(this->_profilerBox, Profiler::enabled);
	this->_refreshing = false;
	if (
		Profiler::enabled &&
		static_cast<size_t>(this->_tabs->getSelectedIndex()) == this->_perfPage &&
		this->_refreshTimer++ % 30 == 0
	)
		this->_refreshSubtreeList();
}
