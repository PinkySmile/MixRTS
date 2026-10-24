//
// Debug tools: every runtime toggle driven by the debug panel (F3).
//

#ifndef MIXRTS_DEBUGOPTIONS_HPP
#define MIXRTS_DEBUGOPTIONS_HPP


#include <array>
#include <string>
#include <cstdint>
#include "Resources/Assert.hpp"

// Compile time tunables (override with -D...)

//! Number of frames kept by the profiler (and shown by the performance graph)
#ifndef PROFILER_HISTORY
#define PROFILER_HISTORY 300
#endif
//! Tolerance (in pixels) of the "exactly at cursor" render order filter
#ifndef DEBUG_RENDER_TOLERANCE
#define DEBUG_RENDER_TOLERANCE 2
#endif
//! Number of ticks a warhead impact stays on screen
#ifndef DEBUG_WARHEAD_FADE_TICKS
#define DEBUG_WARHEAD_FADE_TICKS 60
#endif
//! Maximum number of warhead impacts remembered
#ifndef DEBUG_WARHEAD_LOG_SIZE
#define DEBUG_WARHEAD_LOG_SIZE 256
#endif
//! Maximum number of past positions remembered per projectile
#ifndef DEBUG_PROJECTILE_TRAIL
#define DEBUG_PROJECTILE_TRAIL 128
#endif

enum RenderOrderFilter {
	RENDERFILTER_ALL,
	RENDERFILTER_BELOW,
	RENDERFILTER_ABOVE,
	RENDERFILTER_AT
};

enum PerfView {
	PERFVIEW_SECTIONS, //!< Stacked bars of the top level sections
	PERFVIEW_SUBTREE,  //!< Stacked bars of the children of the selected section
	PERFVIEW_COUNTERS, //!< Line graphs of the counters
	PERFVIEW_TABLE     //!< Breakdown table
};

enum PerfSort {
	PERFSORT_AVERAGE,
	PERFSORT_MAX,
	PERFSORT_CALLS,
	PERFSORT_NAME
};

struct DebugOptions {
	// --- Views
	//! Index of the grid view shown (255: none). See GameManager::getGridViewLabel.
	unsigned char gridView = 255;
	bool limitCamera = true;

	// --- Display control
	bool showTerrain = true;
	//! Indexed by layer + 2
	std::array<bool, 5> showLayers = {true, true, true, true, true};
	bool showObjects = true;
	bool showEffects = true;
	bool showProjectiles = true;
	bool showSkirts = true;
	bool showPlayerOverlays = true;
	RenderOrderFilter renderFilter = RENDERFILTER_ALL;
	unsigned renderFilterTolerance = DEBUG_RENDER_TOLERANCE;
	bool showRenderOrderLine = true;

	// --- Object inspector
	bool inspector = false;
	bool inspectorTags = false;
	bool inspectorPath = false;
	bool inspectorBoxes = false;

	// --- Projectiles
	bool projectiles = false;

	// --- Warheads
	bool warheads = false;
	bool pauseOnWarhead = false;

	// --- Performance
	bool perfGraph = false;
	PerfView perfView = PERFVIEW_SECTIONS;
	PerfSort perfSort = PERFSORT_AVERAGE;
	//! Path of the section shown by PERFVIEW_SUBTREE (names separated by '/')
	std::string perfSubtree;
	//! Number of frames shown by the graph
	unsigned perfFrames = 150;

	[[nodiscard]] inline bool isLayerVisible(int layer) const
	{
		assert_exp(layer >= -2);
		assert_exp(layer <= 2);
		return this->showLayers[layer + 2];
	}
};


#endif //MIXRTS_DEBUGOPTIONS_HPP
