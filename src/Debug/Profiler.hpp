//
// Debug tools: lightweight scoped-timer profiler.
//
// Usage: put PROFILE_SCOPE("name") at the top of a block. Scopes nest and are merged
// per parent (calling the same scope 1000 times in a frame gives one node with
// 1000 calls). \a name must be a string literal (or any string that outlives the
// program), it is never copied.
//
// The frame time is always recorded (so FPS statistics work everywhere), the
// section tree is only recorded when Profiler::enabled is true. When disabled a
// scope costs a single boolean check.
//

#ifndef MIXRTS_PROFILER_HPP
#define MIXRTS_PROFILER_HPP


#include <vector>
#include <array>
#include <chrono>
#include <string>
#include <cstdint>
#include "Resources/MSVCMacros.hpp"
#include "Debug/DebugOptions.hpp"

enum ProfilerCounter {
	PROFCOUNTER_OBJECTS,
	PROFCOUNTER_PROJECTILES,
	PROFCOUNTER_EFFECTS,
	PROFCOUNTER_PATHFINDS,
	PROFCOUNTER_AREA_QUERIES,
	PROFCOUNTER_OBJECTS_QUERIED,
	PROFCOUNTER_COUNT
};

MYDLL_API const char *getProfilerCounterName(ProfilerCounter counter);

class MYDLL_API Profiler {
public:
	struct Node {
		const char *name = "";
		int parent = -1;
		int firstChild = -1;
		int lastChild = -1;
		int nextSibling = -1;
		unsigned depth = 0;
		uint32_t calls = 0;
		uint64_t ns = 0;
	};

	struct Frame {
		//! Node 0 is the virtual root, its time is the whole frame.
		std::vector<Node> nodes;
		std::array<uint64_t, PROFCOUNTER_COUNT> counters = {};
		uint64_t frameNs = 0;
		uint64_t serial = 0;
	};

	struct SectionStat {
		std::string path;
		std::string name;
		unsigned depth;
		double averageMs;
		double maxMs;
		double averageCalls;
	};

	struct FrameStats {
		double averageMs = 0;
		double worstMs = 0;
		double lastMs = 0;
	};

	//! Whether the section tree is recorded.
	static bool enabled;
	//! When true, finished frames are not added to the history anymore.
	bool frozen = false;
	//! When non zero, freezes as soon as a frame takes longer than this (in milliseconds).
	double autoFreezeMs = 0;

	static Profiler &get();

	void beginFrame();
	void endFrame();
	int enter(const char *name);
	void leave(int index);

	static inline void count(ProfilerCounter counter, uint64_t amount = 1)
	{
		if (enabled)
			get()._current.counters[counter] += amount;
	}
	static inline void set(ProfilerCounter counter, uint64_t value)
	{
		if (enabled)
			get()._current.counters[counter] = value;
	}

	//! Number of frames in the history.
	[[nodiscard]] size_t size() const;
	//! Frame \a index of the history, 0 being the oldest.
	[[nodiscard]] const Frame &getFrame(size_t index) const;
	//! Incremented every time a frame is added to the history.
	[[nodiscard]] uint64_t getSerial() const;
	[[nodiscard]] FrameStats getFrameStats(size_t frames) const;
	[[nodiscard]] std::vector<SectionStat> computeStats(size_t frames) const;
	void clear();

	static std::string pathOf(const Frame &frame, int index);
	static int findPath(const Frame &frame, const std::string &path);
	static int findPath(const Frame &frame, const std::vector<std::string> &components);
	static std::vector<std::string> splitPath(const std::string &path);
	//! Paths of every section found in the most recent frames, in tree order.
	[[nodiscard]] std::vector<std::string> listPaths(size_t frames) const;

private:
	std::vector<Frame> _history;
	Frame _current;
	std::vector<std::chrono::steady_clock::time_point> _starts;
	std::chrono::steady_clock::time_point _frameStart;
	size_t _head = 0;
	size_t _count = 0;
	uint64_t _serial = 0;
	int _cur = 0;

	Profiler();
};

class ProfileScope {
private:
	int _index;

public:
	explicit ProfileScope(const char *name) :
		_index(Profiler::enabled ? Profiler::get().enter(name) : -1)
	{
	}

	~ProfileScope()
	{
		if (this->_index >= 0)
			Profiler::get().leave(this->_index);
	}

	ProfileScope(const ProfileScope &) = delete;
	ProfileScope &operator=(const ProfileScope &) = delete;
};

#ifdef NO_PROFILER
#define PROFILE_SCOPE(name) do {} while (false)
#define PROFILE_COUNT(counter, amount) do {} while (false)
#define PROFILE_SET(counter, value) do {} while (false)
#else
#define PROFILE_CONCAT2(a, b) a##b
#define PROFILE_CONCAT(a, b) PROFILE_CONCAT2(a, b)
#define PROFILE_SCOPE(name) ProfileScope PROFILE_CONCAT(profileScope_, __LINE__)(name)
#define PROFILE_COUNT(counter, amount) Profiler::count(counter, amount)
#define PROFILE_SET(counter, value) Profiler::set(counter, value)
#endif


#endif //MIXRTS_PROFILER_HPP
