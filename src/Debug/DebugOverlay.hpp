//
// Debug tools: everything the debug tools draw on top of the game
// (object inspector, projectiles, warheads, render order, performance graph).
//

#ifndef MIXRTS_DEBUGOVERLAY_HPP
#define MIXRTS_DEBUGOVERLAY_HPP


#include <vector>
#include <string>
#include <unordered_set>
#include <SFML/Graphics.hpp>
#include "Resources/MSVCMacros.hpp"
#include "Data/Vector.hpp"
#include "Debug/Profiler.hpp"
#include "forward.hpp"

class MYDLL_API DebugOverlay {
public:
	explicit DebugOverlay(GameManager &manager);

	//! Draws the overlays living in the game world. The screen view must be the world view.
	void renderWorld();
	//! Draws the overlays living on the screen. The screen view is changed and restored by this function.
	void renderScreen();

private:
	GameManager &_manager;
	sf::Text _text;
	std::unordered_set<const GameObject *> _alive;
	bool _aliveValid = false;
	// Cache of the breakdown table
	std::vector<Profiler::SectionStat> _stats;
	uint64_t _statsSerial = UINT64_MAX;
	size_t _statsFrames = 0;

	void _ensureAlive();

	// Drawing helpers
	void _drawLine(Vector2f a, Vector2f b, sf::Color color);
	void _drawRect(Vector2f pos, Vector2f size, sf::Color outline, sf::Color fill = sf::Color::Transparent);
	void _drawCircle(Vector2f center, float radius, sf::Color outline, sf::Color fill = sf::Color::Transparent);
	void _drawCross(Vector2f pos, float size, sf::Color color);
	void _drawText(const std::string &str, Vector2f pos, sf::Color color, unsigned size = 10);
	float _textWidth(const std::string &str, unsigned size);

	// World
	void _renderRenderOrder();
	void _renderInspectorWorld();
	void _renderObjectTags();
	void _renderProjectiles();
	void _renderWarheads();

	// Screen
	void _renderInspectorPanel();
	void _drawInfoColumns(const std::vector<std::pair<std::string, std::string>> &info, Vector2f origin);
	void _renderPerformance();
	void _renderPerfBars(Vector2f pos, Vector2f size);
	void _renderPerfCounters(Vector2f pos, Vector2f size);
	void _renderPerfTable(Vector2f pos, Vector2f size);
};


#endif //MIXRTS_DEBUGOVERLAY_HPP
