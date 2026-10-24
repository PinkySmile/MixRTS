//
// Debug tools: lightweight scoped-timer profiler.
//

#include <cstring>
#include <algorithm>
#include <unordered_map>
#include "Profiler.hpp"

bool Profiler::enabled = false;

const char *getProfilerCounterName(ProfilerCounter counter)
{
	switch (counter) {
	case PROFCOUNTER_OBJECTS:
		return "Live objects";
	case PROFCOUNTER_PROJECTILES:
		return "Live projectiles";
	case PROFCOUNTER_EFFECTS:
		return "Live effects";
	case PROFCOUNTER_PATHFINDS:
		return "Path finding calls";
	case PROFCOUNTER_AREA_QUERIES:
		return "Area queries";
	case PROFCOUNTER_OBJECTS_QUERIED:
		return "Objects tested by area queries";
	default:
		return "?";
	}
}

static void resetFrame(Profiler::Frame &frame)
{
	frame.nodes.clear();
	frame.counters.fill(0);
	frame.frameNs = 0;

	Profiler::Node root;

	root.name = "Frame";
	root.calls = 1;
	frame.nodes.push_back(root);
}

Profiler::Profiler() :
	_history(PROFILER_HISTORY),
	_frameStart(std::chrono::steady_clock::now())
{
	this->_starts.reserve(32);
	this->_current.nodes.reserve(128);
	resetFrame(this->_current);
}

Profiler &Profiler::get()
{
	static Profiler instance;

	return instance;
}

void Profiler::beginFrame()
{
	resetFrame(this->_current);
	this->_starts.clear();
	this->_cur = 0;
	this->_frameStart = std::chrono::steady_clock::now();
}

void Profiler::endFrame()
{
	auto now = std::chrono::steady_clock::now();
	uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - this->_frameStart).count();

	this->_current.frameNs = ns;
	this->_current.nodes[0].ns = ns;
	if (this->frozen)
		return;
	this->_current.serial = ++this->_serial;
	std::swap(this->_history[this->_head], this->_current);
	this->_head = (this->_head + 1) % this->_history.size();
	this->_count = std::min(this->_count + 1, this->_history.size());
	if (this->autoFreezeMs > 0 && ns / 1e6 > this->autoFreezeMs)
		this->frozen = true;
}

int Profiler::enter(const char *name)
{
	auto &nodes = this->_current.nodes;
	int found = -1;

	if (this->_cur < 0 || static_cast<size_t>(this->_cur) >= nodes.size())
		this->_cur = 0;
	for (int c = nodes[this->_cur].firstChild; c != -1; c = nodes[c].nextSibling)
		if (nodes[c].name == name || strcmp(nodes[c].name, name) == 0) {
			found = c;
			break;
		}
	if (found == -1) {
		Node node;

		node.name = name;
		node.parent = this->_cur;
		node.depth = nodes[this->_cur].depth + 1;
		found = nodes.size();
		nodes.push_back(node);

		auto &parent = nodes[this->_cur];

		if (parent.lastChild == -1)
			parent.firstChild = found;
		else
			nodes[parent.lastChild].nextSibling = found;
		parent.lastChild = found;
	}
	nodes[found].calls++;
	this->_cur = found;
	this->_starts.push_back(std::chrono::steady_clock::now());
	return found;
}

void Profiler::leave(int index)
{
	auto now = std::chrono::steady_clock::now();
	auto &nodes = this->_current.nodes;

	if (this->_starts.empty() || index < 0 || static_cast<size_t>(index) >= nodes.size())
		return;
	nodes[index].ns += std::chrono::duration_cast<std::chrono::nanoseconds>(now - this->_starts.back()).count();
	this->_starts.pop_back();
	this->_cur = nodes[index].parent;
}

size_t Profiler::size() const
{
	return this->_count;
}

const Profiler::Frame &Profiler::getFrame(size_t index) const
{
	size_t oldest = (this->_head + this->_history.size() - this->_count) % this->_history.size();

	return this->_history[(oldest + index) % this->_history.size()];
}

uint64_t Profiler::getSerial() const
{
	return this->_serial;
}

Profiler::FrameStats Profiler::getFrameStats(size_t frames) const
{
	FrameStats result;
	size_t n = std::min(frames, this->_count);

	for (size_t i = this->_count - n; i < this->_count; i++) {
		double ms = this->getFrame(i).frameNs / 1e6;

		result.averageMs += ms;
		result.worstMs = std::max(result.worstMs, ms);
		result.lastMs = ms;
	}
	if (n)
		result.averageMs /= n;
	return result;
}

std::string Profiler::pathOf(const Frame &frame, int index)
{
	std::string result;

	while (index > 0) {
		auto &node = frame.nodes[index];

		result = result.empty() ? node.name : std::string(node.name) + "/" + result;
		index = node.parent;
	}
	return result;
}

std::vector<std::string> Profiler::splitPath(const std::string &path)
{
	std::vector<std::string> result;
	size_t start = 0;

	while (start <= path.size() && !path.empty()) {
		size_t end = path.find('/', start);

		if (end == std::string::npos)
			end = path.size();
		result.push_back(path.substr(start, end - start));
		start = end + 1;
	}
	return result;
}

int Profiler::findPath(const Frame &frame, const std::vector<std::string> &components)
{
	int current = 0;

	for (auto &name : components) {
		int child = frame.nodes[current].firstChild;

		while (child != -1 && name != frame.nodes[child].name)
			child = frame.nodes[child].nextSibling;
		if (child == -1)
			return -1;
		current = child;
	}
	return current == 0 ? -1 : current;
}

int Profiler::findPath(const Frame &frame, const std::string &path)
{
	return findPath(frame, splitPath(path));
}

std::vector<Profiler::SectionStat> Profiler::computeStats(size_t frames) const
{
	std::vector<SectionStat> result;
	std::unordered_map<std::string, size_t> indices;
	size_t n = std::min(frames, this->_count);
	std::vector<std::string> paths;

	for (size_t i = this->_count - n; i < this->_count; i++) {
		auto &frame = this->getFrame(i);

		paths.assign(frame.nodes.size(), "");
		// Children are always created after their parent
		for (size_t j = 1; j < frame.nodes.size(); j++) {
			auto &node = frame.nodes[j];

			paths[j] = node.parent <= 0 ? std::string(node.name) : paths[node.parent] + "/" + node.name;

			auto it = indices.find(paths[j]);

			if (it == indices.end()) {
				it = indices.emplace(paths[j], result.size()).first;
				result.push_back({paths[j], node.name, node.depth - 1, 0, 0, 0});
			}

			auto &stat = result[it->second];
			double ms = node.ns / 1e6;

			stat.averageMs += ms;
			stat.maxMs = std::max(stat.maxMs, ms);
			stat.averageCalls += node.calls;
		}
	}
	for (auto &stat : result) {
		stat.averageMs /= n;
		stat.averageCalls /= n;
	}
	// Put children right after their parent
	auto key = [](std::string s) {
		std::replace(s.begin(), s.end(), '/', '\x01');
		return s;
	};

	std::stable_sort(result.begin(), result.end(), [&key](const SectionStat &a, const SectionStat &b) {
		return key(a.path) < key(b.path);
	});
	return result;
}

std::vector<std::string> Profiler::listPaths(size_t frames) const
{
	std::vector<std::string> result;

	for (auto &stat : this->computeStats(frames))
		result.push_back(stat.path);
	return result;
}

void Profiler::clear()
{
	this->_count = 0;
	this->_head = 0;
}
