//
// Debug tools: the TGUI window (F3) driving DebugOptions and the profiler.
//
// In release builds only the performance tab exists.
//

#ifndef MIXRTS_DEBUGPANEL_HPP
#define MIXRTS_DEBUGPANEL_HPP


#include <deque>
#include <array>
#include <vector>
#include <string>
#include <functional>
#include <TGUI/TGUI.hpp>
#include "Resources/MSVCMacros.hpp"
#include "forward.hpp"

class MYDLL_API DebugPanel {
public:
	explicit DebugPanel(GameManager &manager);
	~DebugPanel();

	void toggle();
	//! Keeps the widgets in sync with what changed outside of the panel (called every frame).
	void refresh();
	[[nodiscard]] bool isVisible() const;

private:
	struct Page {
		tgui::ScrollablePanel::Ptr panel;
		float y = 6;
	};

	GameManager &_manager;
	tgui::ChildWindow::Ptr _window;
	tgui::Tabs::Ptr _tabs;
	std::deque<Page> _pages;
	size_t _perfPage = 0;
	// Widgets that need to be synchronised by refresh()
	tgui::CheckBox::Ptr _pausedBox;
	tgui::CheckBox::Ptr _unlockBox;
	tgui::ComboBox::Ptr _playerCombo;
	tgui::CheckBox::Ptr _frozenBox;
	tgui::CheckBox::Ptr _profilerBox;
	tgui::ComboBox::Ptr _perfViewCombo;
	tgui::ComboBox::Ptr _subtreeCombo;
	std::array<tgui::CheckBox::Ptr, 5> _layerBoxes;
	std::vector<std::string> _subtreeItems;
	unsigned _refreshTimer = 0;
	unsigned _currentPlayer;
	bool _refreshing = false;

	Page &_addPage(const std::string &name);
	void _addLabel(Page &page, const std::string &text, bool header = false);
	tgui::CheckBox::Ptr _addCheck(Page &page, const std::string &text, bool initial, const std::function<void (bool)> &cb);
	tgui::CheckBox::Ptr _addCheck(Page &page, const std::string &text, bool &value);
	tgui::ComboBox::Ptr _addCombo(Page &page, const std::string &label, const std::vector<std::string> &items, size_t selected, const std::function<void (size_t)> &cb);
	tgui::Slider::Ptr _addSlider(Page &page, const std::string &label, float min, float max, float step, float value, const std::function<void (float)> &cb);
	tgui::Button::Ptr _addButton(Page &page, const std::string &text, const std::function<void ()> &cb);

	void _buildGeneralPage();
	void _buildDisplayPage();
	void _buildObjectsPage();
	void _buildCombatPage();
	void _buildPerformancePage();
	void _refreshSubtreeList();
};


#endif //MIXRTS_DEBUGPANEL_HPP
