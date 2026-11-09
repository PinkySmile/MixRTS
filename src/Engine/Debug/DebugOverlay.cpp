//
// Debug tools: everything the debug tools draw on top of the game.
//

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <functional>
#include "DebugOverlay.hpp"
#include "Data/GameManager.hpp"
#include "Data/GameObject.hpp"
#include "Data/Projectile.hpp"
#include "Data/Player.hpp"
#include "Data/Warhead.hpp"
#include "Resources/Game.hpp"

namespace
{
	const sf::Color LIGHT_GRAY{0xC0, 0xC0, 0xC0};
	const sf::Color DARK_GRAY{0x60, 0x60, 0x60};
	const sf::Color ORANGE{0xFF, 0x90, 0x20};
	const sf::Color CYAN{0x30, 0xE0, 0xFF};
	const sf::Color PINK{0xFF, 0x40, 0xFF};

	sf::Color withAlpha(sf::Color color, unsigned alpha)
	{
		color.a = std::min(alpha, 255U);
		return color;
	}

	sf::Color hsv(float hue, float saturation, float value)
	{
		float c = value * saturation;
		float x = c * (1 - std::fabs(std::fmod(hue / 60, 2.f) - 1));
		float m = value - c;
		float r = 0;
		float g = 0;
		float b = 0;

		if (hue < 60)
			r = c, g = x;
		else if (hue < 120)
			r = x, g = c;
		else if (hue < 180)
			g = c, b = x;
		else if (hue < 240)
			g = x, b = c;
		else if (hue < 300)
			r = x, b = c;
		else
			r = c, b = x;
		return sf::Color{
			static_cast<uint8_t>((r + m) * 255),
			static_cast<uint8_t>((g + m) * 255),
			static_cast<uint8_t>((b + m) * 255)
		};
	}

	sf::Color colorForName(const char *name)
	{
		uint32_t h = 2166136261U;

		for (; *name; ++name) {
			h ^= static_cast<uint8_t>(*name);
			h *= 16777619U;
		}
		return hsv(h % 360, 0.65f, 0.95f);
	}

	const char *damageTypeName(DamageType type)
	{
		switch (type) {
		case DMGTYPE_VOID:
			return "void";
		case DMGTYPE_TRUE:
			return "true";
		case DMGTYPE_GENERIC:
			return "generic";
		case DMGTYPE_BULLET:
			return "bullet";
		case DMGTYPE_MISSILE:
			return "missile";
		case DMGTYPE_BLAST:
			return "blast";
		case DMGTYPE_SONIC:
			return "sonic";
		default:
			return "?";
		}
	}

	sf::Color damageTypeColor(DamageType type)
	{
		return hsv(static_cast<unsigned>(type) * 55 % 360, 0.8f, 1.f);
	}

	std::string formatMs(double ms)
	{
		char buffer[32];

		snprintf(buffer, sizeof(buffer), "%.2f", ms);
		return buffer;
	}

	Vector2f toVec(sf::Vector2f v)
	{
		return {v.x, v.y};
	}

	//! Stacked bars (rectangles) builder
	struct BarBuilder {
		sf::VertexArray array{sf::PrimitiveType::Triangles};

		void add(float x, float y0, float width, float y1, sf::Color color)
		{
			sf::Vertex v[4];

			v[0].position = {x, y0};
			v[1].position = {x + width, y0};
			v[2].position = {x + width, y1};
			v[3].position = {x, y1};
			for (auto &vertex : v)
				vertex.color = color;
			this->array.append(v[0]);
			this->array.append(v[1]);
			this->array.append(v[2]);
			this->array.append(v[0]);
			this->array.append(v[2]);
			this->array.append(v[3]);
		}
	};
}

DebugOverlay::DebugOverlay(GameManager &manager) :
	_manager(manager),
	_text(game->font)
{
	this->_text.setOutlineColor(sf::Color::Black);
	this->_text.setOutlineThickness(1);
}

void DebugOverlay::_ensureAlive()
{
	if (this->_aliveValid)
		return;
	this->_alive.clear();
	this->_alive.reserve(this->_manager._liveObjects.size() * 2);
	for (auto &o : this->_manager._liveObjects)
		if (!o->isDisposable())
			this->_alive.insert(o.get());
	this->_aliveValid = true;
}

void DebugOverlay::_drawLine(Vector2f a, Vector2f b, sf::Color color)
{
	sf::VertexArray array{sf::PrimitiveType::Lines, 2};

	array[0].position = {a.x, a.y};
	array[0].color = color;
	array[1].position = {b.x, b.y};
	array[1].color = color;
	game->screen->draw(array);
}

void DebugOverlay::_drawRect(Vector2f pos, Vector2f size, sf::Color outline, sf::Color fill)
{
	sf::RectangleShape shape;

	shape.setPosition({pos.x, pos.y});
	shape.setSize({size.x, size.y});
	shape.setFillColor(fill);
	shape.setOutlineColor(outline);
	shape.setOutlineThickness(1);
	game->screen->draw(shape);
}

void DebugOverlay::_drawCircle(Vector2f center, float radius, sf::Color outline, sf::Color fill)
{
	sf::CircleShape shape{radius, 48};

	shape.setOrigin({radius, radius});
	shape.setPosition({center.x, center.y});
	shape.setFillColor(fill);
	shape.setOutlineColor(outline);
	shape.setOutlineThickness(1);
	game->screen->draw(shape);
}

void DebugOverlay::_drawCross(Vector2f pos, float size, sf::Color color)
{
	this->_drawLine({pos.x - size, pos.y}, {pos.x + size, pos.y}, color);
	this->_drawLine({pos.x, pos.y - size}, {pos.x, pos.y + size}, color);
}

void DebugOverlay::_drawText(const std::string &str, Vector2f pos, sf::Color color, unsigned size)
{
	this->_text.setCharacterSize(size);
	this->_text.setString(str);
	this->_text.setPosition({pos.x, pos.y});
	this->_text.setFillColor(color);
	this->_text.setOutlineColor(withAlpha(sf::Color::Black, color.a));
	this->_text.setOutlineThickness(1);
	game->screen->draw(this->_text);
}

float DebugOverlay::_textWidth(const std::string &str, unsigned size)
{
	this->_text.setCharacterSize(size);
	this->_text.setString(str);
	return this->_text.getLocalBounds().size.x;
}

void DebugOverlay::renderWorld()
{
	this->_aliveValid = false;
#ifdef _DEBUG
	auto &opts = this->_manager.debugOptions;

	if (opts.renderFilter != RENDERFILTER_ALL && opts.showRenderOrderLine)
		this->_renderRenderOrder();
	if (opts.inspector)
		this->_renderInspectorWorld();
	if (opts.inspectorTags)
		this->_renderObjectTags();
	if (opts.projectiles)
		this->_renderProjectiles();
	if (opts.warheads)
		this->_renderWarheads();
#endif
}

void DebugOverlay::renderScreen()
{
	auto &opts = this->_manager.debugOptions;
	bool inspector = false;

#ifdef _DEBUG
	// The panel always shows something on the left: the selected object, or the controlled player
	inspector = opts.inspector || this->_manager.isDebugPanelOpen();
#endif
	if (!inspector && !opts.perfGraph) {
		this->_aliveValid = false;
		return;
	}

	sf::View old = game->screen->getView();
	Vector2u size = game->screen->getSize();

	game->screen->setView(sf::View{sf::FloatRect{
		{0, 0},
		{static_cast<float>(size.x), static_cast<float>(size.y)}
	}});
#ifdef _DEBUG
	if (inspector)
		this->_renderInspectorPanel();
#endif
	if (opts.perfGraph)
		this->_renderPerformance();
	game->screen->setView(old);
	this->_aliveValid = false;
}

#ifdef _DEBUG
void DebugOverlay::_renderRenderOrder()
{
	sf::View view = game->screen->getView();
	Vector2f center = toVec(view.getCenter());
	Vector2f size = toVec(view.getSize());
	float y = this->_manager._getCursorRenderKey();
	auto &opts = this->_manager.debugOptions;
	std::string label = "render key " + std::to_string(static_cast<int64_t>(y));

	switch (opts.renderFilter) {
	case RENDERFILTER_BELOW:
		label += " (showing objects below or at the line)";
		break;
	case RENDERFILTER_ABOVE:
		label += " (showing objects above or at the line)";
		break;
	case RENDERFILTER_AT:
		label += " (showing objects within +/-" + std::to_string(opts.renderFilterTolerance) + ")";
		break;
	default:
		break;
	}
	this->_drawLine({center.x - size.x / 2, y}, {center.x + size.x / 2, y}, PINK);
	this->_drawText(label, {center.x - size.x / 2 + 4, y - 14}, PINK, 12);
}

void DebugOverlay::_renderInspectorWorld()
{
	auto &opts = this->_manager.debugOptions;

	this->_ensureAlive();
	for (auto &player : this->_manager._players) {
		if (!player.isEnabled())
			continue;

		bool first = true;

		for (const GameObject *obj : player.uiState.selectedObjects) {
			if (!this->_alive.contains(obj))
				continue;

			Vector2f pos = Vector2f(obj->getPosition());
			Vector2f size = Vector2f(obj->getSize());
			sf::Color color = first ? CYAN : sf::Color{0x80, 0xA0, 0xFF};

			this->_drawRect(pos, size, color);
			this->_drawCross(obj->getMiddle(), 3, first ? sf::Color::Yellow : sf::Color::White);
			if (opts.inspectorBoxes) {
				auto &box = obj->getSelectBox();

				this->_drawRect(
					pos + Vector2f{static_cast<float>(box.position.x), static_cast<float>(box.position.y)},
					Vector2f{static_cast<float>(box.size.x), static_cast<float>(box.size.y)},
					sf::Color::Yellow
				);
			}
			if (opts.inspectorPath)
				obj->renderPathFindingPath();
			first = false;
		}
	}
}

//! Short tag above every object on screen (only while the debug panel is open)
void DebugOverlay::_renderObjectTags()
{
	auto &opts = this->_manager.debugOptions;
	sf::View view = game->screen->getView();
	Vector2f center = toVec(view.getCenter());
	Vector2f halfSize = toVec(view.getSize()) / 2.f;
	std::unordered_set<const GameObject *> tagged;

	// Selected objects of the enabled players already get their (longer) tag from the inspector
	if (opts.inspector && opts.inspectorTags)
		for (auto &player : this->_manager._players)
			if (player.isEnabled())
				tagged.insert(player.uiState.selectedObjects.begin(), player.uiState.selectedObjects.end());
	for (auto &obj : this->_manager._liveObjects) {
		if (obj->isDisposable() || tagged.contains(&*obj))
			continue;

		Vector2f pos = Vector2f(obj->getPosition());
		Vector2f size = Vector2f(obj->getSize());

		if (
			pos.x + size.x < center.x - halfSize.x || pos.x > center.x + halfSize.x ||
			pos.y + size.y < center.y - halfSize.y || pos.y > center.y + halfSize.y
		)
			continue;
		this->_drawText(
			obj->getId() + "#" + std::to_string(obj->index),
			{pos.x, pos.y - 12},
			sf::Color::White
		);
	}
}

void DebugOverlay::_renderProjectiles()
{
	this->_ensureAlive();
	for (auto &p : this->_manager._liveProjectiles) {
		if (p.isDisposable())
			continue;

		Vector2f pos = p.getPosition();
		float height = p.isArc() && p.getHeight() == 0 ? p.getArcHeight(pos) : p.getHeight();
		Vector2f visual = {pos.x, pos.y - height};
		Vector2f target = p.getTargetPosition();
		auto &trail = p.getTrail();
		std::string label = "dmg " + std::to_string(p.getDamage());

		// Where it has been
		if (!trail.empty()) {
			sf::VertexArray array{sf::PrimitiveType::LineStrip, trail.size() + 1};
			size_t i = 0;

			for (auto &point : trail) {
				array[i].position = {point.position.x, point.position.y - point.height};
				array[i].color = withAlpha(ORANGE, 40 + 215 * i / trail.size());
				i++;
			}
			array[i].position = {visual.x, visual.y};
			array[i].color = ORANGE;
			game->screen->draw(array);
		}

		// Where it is going
		if (p.isFalling()) {
			auto fall = p.simulateFall();

			if (!fall.empty()) {
				sf::VertexArray array{sf::PrimitiveType::LineStrip, fall.size() + 1};

				array[0].position = {visual.x, visual.y};
				array[0].color = sf::Color::Red;
				for (size_t i = 0; i < fall.size(); i++) {
					array[i + 1].position = {fall[i].position.x, fall[i].position.y - fall[i].height};
					array[i + 1].color = sf::Color::Red;
				}
				game->screen->draw(array);
				this->_drawCross(fall.back().position, 5, sf::Color::Red);
				this->_drawCircle(fall.back().position, 4, sf::Color::Red);
			}
			label += " falling";
		} else {
			if (p.isArc()) {
				constexpr unsigned SAMPLES = 24;
				sf::VertexArray array{sf::PrimitiveType::LineStrip, SAMPLES + 1};

				for (unsigned i = 0; i <= SAMPLES; i++) {
					Vector2f q = pos + (target - pos) * (static_cast<float>(i) / SAMPLES);

					array[i].position = {q.x, q.y - p.getArcHeight(q)};
					array[i].color = sf::Color::Yellow;
				}
				game->screen->draw(array);
				label += " arc";
			} else
				this->_drawLine(visual, target, sf::Color::Yellow);
			this->_drawCross(target, 4, sf::Color::Red);
			this->_drawCircle(target, 3, sf::Color::Red);
		}
		if (p.isHoming())
			label += " homing";
		if (p.isHitTravel())
			label += " hitTravel";

		// What it is going for
		const GameObject *obj = p.getTarget();

		if (obj && this->_alive.contains(obj)) {
			this->_drawRect(Vector2f(obj->getPosition()), Vector2f(obj->getSize()), sf::Color::Red);
			this->_drawLine(visual, obj->getMiddle(), withAlpha(sf::Color::Red, 140));
		}
		if (p.getRange())
			this->_drawCircle(p.getStartPosition(), p.getRange(), withAlpha(LIGHT_GRAY, 200), withAlpha(LIGHT_GRAY, 40));
		this->_drawCircle(p.getStartPosition(), 2, sf::Color::Green);
		this->_drawCircle(visual, 2, sf::Color::White, sf::Color::White);
		this->_drawText(label, {visual.x + 6, visual.y - 14}, sf::Color::White);
	}
}

void DebugOverlay::_renderWarheads()
{
	uint64_t tick = this->_manager._tick;

	for (auto &hit : this->_manager._warheadHits) {
		uint64_t age = tick - hit.lastTick;

		if (age >= DEBUG_WARHEAD_FADE_TICKS)
			continue;

		unsigned alpha = 255 - 255 * age / DEBUG_WARHEAD_FADE_TICKS;
		bool contact = age == 0;
		sf::Color color = damageTypeColor(hit.damageType);
		float radius = std::max(hit.radius, 3U);

		this->_drawCircle(hit.position, radius, withAlpha(color, alpha), contact ? withAlpha(color, 90) : sf::Color::Transparent);
		if (contact)
			this->_drawCircle(hit.position, radius + 2, sf::Color::White);
		this->_drawCross(hit.position, 4, withAlpha(sf::Color::White, alpha));
		for (auto &target : hit.targets) {
			this->_drawLine(hit.position, target.hitPoint, withAlpha(ORANGE, alpha));
			this->_drawCross(target.hitPoint, 3, withAlpha(ORANGE, alpha));
			this->_drawText(
				"#" + std::to_string(target.objectId) + " -" + std::to_string(target.damage) + " (x" + formatMs(target.multiplier) + ", d=" + formatMs(target.distance) + ")",
				{target.hitPoint.x + 5, target.hitPoint.y - 12},
				withAlpha(sf::Color::White, alpha)
			);
		}

		std::string header = "t=" + std::to_string(hit.tick) + " " + damageTypeName(hit.damageType) + " r=" + std::to_string(hit.radius) + " dmg=" + std::to_string(hit.baseDamage);

		if (hit.hits > 1)
			header += " x" + std::to_string(hit.hits);
		if (contact)
			header += "  CONTACT";
		this->_drawText(header, {hit.position.x - radius, hit.position.y - radius - 14}, withAlpha(sf::Color::White, alpha), 11);
	}
}

//! Lines of information about a player (debug tool)
static std::vector<std::pair<std::string, std::string>> playerDebugInfo(const Player &player, size_t controlled)
{
	std::vector<std::pair<std::string, std::string>> out;
	auto add = [&out](const std::string &name, const std::string &value) {
		out.emplace_back(name, value);
	};
	auto yesNo = [](bool b) -> std::string {
		return b ? "yes" : "no";
	};
	// Lists are wrapped on their item boundaries: the first row holds the name, the next ones are left blank
	auto addList = [&out](const std::string &name, std::vector<std::string> items, const std::string &separator) {
		constexpr size_t MAX_CHARS = 48;
		std::string line;
		bool first = true;

		if (items.empty()) {
			out.emplace_back(name, "none");
			return;
		}
		std::ranges::sort(items);
		for (auto &item : items) {
			if (!line.empty() && line.size() + separator.size() + item.size() > MAX_CHARS) {
				out.emplace_back(first ? name : "", line + separator.substr(0, separator.find_last_not_of(' ') + 1));
				first = false;
				line.clear();
			}
			line += (line.empty() ? "" : separator) + item;
		}
		out.emplace_back(first ? name : "", line);
	};
	auto addMap = [&addList](const std::string &name, const std::unordered_map<std::string, unsigned> &map) {
		std::vector<std::string> items;

		for (auto &[key, value] : map)
			items.push_back(key + "=" + std::to_string(value));
		addList(name, items, ", ");
	};
	auto color = player.getColor();
	auto &state = player.state;
	float p = 100 * (200.f + state.producedPower) / (state.consumedPower + 200.f);

	add("player", "#" + std::to_string(state.index) + (state.index == controlled ? " (controlled)" : ""));
	add("faction", player.getPrimaryFaction().getName() + " (#" + std::to_string(state.factionIndex) + ")");
	add("team", std::to_string(state.team));
	add("enabled / lost", yesNo(player.isEnabled()) + " / " + yesNo(player.lost()));
	add("color (hsl)", std::to_string(color.h) + ", " + std::to_string(color.s) + ", " + std::to_string(color.l));
	addMap("money", state.money);
	addMap("system money", state.systemMoney);
	addMap("total storage", state.totalStorage);
	add("power", std::to_string(state.consumedPower) + "/" + std::to_string(state.producedPower) + " " + std::to_string(p));
	add("radar", yesNo(state.hasRadar) + " (" + std::to_string(state.radars.size()) + " radar objects)");
	add("objects", std::to_string(state.myObjects.size()));
	add("selected", std::to_string(player.uiState.selectedObjects.size()));
	add("debug unlock", yesNo(player.debugUnlock));
	{
		std::vector<std::string> queues;

		for (auto &[id, queue] : state.queues)
			queues.push_back(id + ": " + std::to_string(queue.entries.size()) + (queue.busy ? " (busy)" : ""));
		addList("queues", queues, "; ");
	}
	{
		std::unordered_map<std::string, unsigned> elements;

		for (auto &[id, count] : state.elementsOwned)
			if (count)
				elements[id] = count;
		addMap("elements owned", elements);
	}
	for (auto &[queue, building] : state.primaryBuildings)
		add("primary " + queue, building ? building->getId() + "#" + std::to_string(building->index) : "none");
	return out;
}

void DebugOverlay::_drawInfoColumns(const std::vector<std::pair<std::string, std::string>> &info, Vector2f origin)
{
	constexpr unsigned FONT_SIZE = 12;
	constexpr float LINE_HEIGHT = 14;
	constexpr float NAME_WIDTH = 110;
	float screenHeight = game->screen->getSize().y;
	size_t perColumn = std::max<size_t>(1, (screenHeight - origin.y - 12) / LINE_HEIGHT);
	float x = origin.x;

	for (size_t begin = 0; begin < info.size(); begin += perColumn) {
		size_t end = std::min(info.size(), begin + perColumn);
		float valueWidth = 0;
		float y = origin.y;

		for (size_t i = begin; i < end; i++)
			valueWidth = std::max(valueWidth, this->_textWidth(info[i].second, FONT_SIZE));

		float width = NAME_WIDTH + valueWidth + 12;

		this->_drawRect({x - 4, origin.y - 4}, {width, (end - begin) * LINE_HEIGHT + 8}, sf::Color::Transparent, sf::Color{0, 0, 0, 170});
		for (size_t i = begin; i < end; i++) {
			this->_drawText(info[i].first, {x, y}, LIGHT_GRAY, FONT_SIZE);
			this->_drawText(info[i].second, {x + NAME_WIDTH, y}, sf::Color::White, FONT_SIZE);
			y += LINE_HEIGHT;
		}
		x += width + 8;
	}
}

void DebugOverlay::_renderInspectorPanel()
{
	this->_ensureAlive();

	const GameObject *target = nullptr;
	Vector2f origin = {8, 40};

	for (auto &player : this->_manager._players) {
		if (!player.isEnabled())
			continue;
		for (const GameObject *obj : player.uiState.selectedObjects)
			if (this->_alive.contains(obj)) {
				target = obj;
				break;
			}
		if (target)
			break;
	}
	if (!target) {
		// No object to describe: describe the player being controlled instead
		auto &players = this->_manager._players;
		size_t controlled = this->_manager.getControlledPlayer();

		if (controlled < players.size())
			this->_drawInfoColumns(playerDebugInfo(players[controlled], controlled), origin);
		return;
	}

	DebugInfoList info;

	info.reserve(80);
	target->getDebugInfo(info, this->_alive);
	this->_drawInfoColumns(info, origin);
}
#else
void DebugOverlay::_renderRenderOrder() {}
void DebugOverlay::_renderInspectorWorld() {}
void DebugOverlay::_renderObjectTags() {}
void DebugOverlay::_renderProjectiles() {}
void DebugOverlay::_renderWarheads() {}
void DebugOverlay::_renderInspectorPanel() {}
void DebugOverlay::_drawInfoColumns(const std::vector<std::pair<std::string, std::string>> &, Vector2f) {}
#endif

void DebugOverlay::_renderPerformance()
{
	auto &opts = this->_manager.debugOptions;
	Profiler &profiler = Profiler::get();
	Vector2u screen = game->screen->getSize();
	Vector2f size = {640, 240};
	Vector2f pos = {14, screen.y - size.y - 50.f};
	auto stats = profiler.getFrameStats(opts.perfFrames);
	char buffer[128];

	this->_drawRect({pos.x - 8, pos.y - 26}, {size.x + 16, size.y + 62}, sf::Color::Transparent, sf::Color{0, 0, 0, 180});
	snprintf(
		buffer, sizeof(buffer), "%.0f FPS   avg %.2f ms   worst %.2f ms   last %.2f ms",
		stats.averageMs > 0 ? 1000 / stats.averageMs : 0., stats.averageMs, stats.worstMs, stats.lastMs
	);

	std::string title = buffer;

	if (profiler.frozen)
		title += "   [FROZEN]";
	if (!Profiler::enabled)
		title += "   (profiler off: frame times only)";
#ifdef NO_PROFILER
	title += "   (compiled out)";
#endif
	this->_drawText(title, {pos.x, pos.y - 22}, profiler.frozen ? sf::Color::Yellow : sf::Color::White, 12);

	switch (opts.perfView) {
	case PERFVIEW_SECTIONS:
	case PERFVIEW_SUBTREE:
		this->_renderPerfBars(pos, size);
		break;
	case PERFVIEW_COUNTERS:
		this->_renderPerfCounters(pos, size);
		break;
	case PERFVIEW_TABLE:
		this->_renderPerfTable(pos, size);
		break;
	}
}

void DebugOverlay::_renderPerfBars(Vector2f pos, Vector2f size)
{
	auto &opts = this->_manager.debugOptions;
	Profiler &profiler = Profiler::get();
	size_t n = std::min<size_t>(opts.perfFrames, profiler.size());
	bool subtree = opts.perfView == PERFVIEW_SUBTREE;

	this->_drawRect(pos, size, DARK_GRAY, sf::Color{0x10, 0x10, 0x10, 200});
	if (n == 0)
		return;

	size_t first = profiler.size() - n;
	auto components = Profiler::splitPath(opts.perfSubtree);
	std::vector<int> roots(n);
	double maxMs = subtree ? 0.01 : 33.4;

	for (size_t i = 0; i < n; i++) {
		auto &frame = profiler.getFrame(first + i);

		roots[i] = subtree ? Profiler::findPath(frame, components) : 0;
		if (roots[i] >= 0)
			maxMs = std::max(maxMs, (subtree ? frame.nodes[roots[i]].ns : frame.frameNs) / 1e6);
	}
	maxMs *= 1.05;

	float scale = size.y / maxMs;
	float barWidth = size.x / std::max<unsigned>(opts.perfFrames, 1);
	float right = pos.x + size.x;
	float bottom = pos.y + size.y;
	BarBuilder bars;

	for (size_t i = 0; i < n; i++) {
		if (roots[i] < 0)
			continue;

		auto &frame = profiler.getFrame(first + i);
		auto &root = frame.nodes[roots[i]];
		float x = right - (n - i) * barWidth;
		float y = bottom;
		uint64_t used = 0;

		for (int c = root.firstChild; c != -1; c = frame.nodes[c].nextSibling) {
			float h = frame.nodes[c].ns / 1e6 * scale;

			bars.add(x, y, std::max(barWidth - (barWidth > 3 ? 1 : 0), 1.f), y - h, colorForName(frame.nodes[c].name));
			y -= h;
			used += frame.nodes[c].ns;
		}

		uint64_t total = subtree ? root.ns : frame.frameNs;

		if (total > used)
			bars.add(x, y, std::max(barWidth - (barWidth > 3 ? 1 : 0), 1.f), y - (total - used) / 1e6 * scale, subtree ? sf::Color{0x50, 0x50, 0x50} : sf::Color{0x90, 0x90, 0x90});
	}
	game->screen->draw(bars.array);

	// Reference lines
	for (double ref : {8.33, 16.67, 33.33}) {
		if (ref > maxMs)
			continue;

		float y = bottom - ref * scale;

		this->_drawLine({pos.x, y}, {right, y}, sf::Color{0xFF, 0xFF, 0xFF, 90});
		this->_drawText(formatMs(ref) + "ms", {pos.x + 2, y - 13}, sf::Color{0xFF, 0xFF, 0xFF, 140}, 10);
	}
	if (subtree)
		this->_drawText(formatMs(maxMs / 1.05) + "ms max  /" + opts.perfSubtree, {right - 220, pos.y + 2}, LIGHT_GRAY, 10);

	// Legend (latest frame)
	auto &latest = profiler.getFrame(profiler.size() - 1);

	if (roots[n - 1] >= 0) {
		float lx = pos.x;
		float ly = bottom + 4;

		for (int c = latest.nodes[roots[n - 1]].firstChild; c != -1; c = latest.nodes[c].nextSibling) {
			std::string label = std::string(latest.nodes[c].name) + " " + formatMs(latest.nodes[c].ns / 1e6);
			float w = this->_textWidth(label, 11) + 12;

			if (lx + w > right) {
				lx = pos.x;
				ly += 13;
			}
			this->_drawText(label, {lx, ly}, colorForName(latest.nodes[c].name), 11);
			lx += w;
		}
	}

	// Hover: tree of the hovered frame
	Vector2i mouse = game->screen->getMousePosition();

	if (mouse.x >= pos.x && mouse.x < right && mouse.y >= pos.y && mouse.y < bottom) {
		int index = std::floor((mouse.x - (right - n * barWidth)) / barWidth);

		if (index < 0 || static_cast<size_t>(index) >= n)
			return;

		auto &frame = profiler.getFrame(first + index);
		std::vector<std::string> lines;
		std::function<void (int)> visit = [&](int node) {
			for (int c = frame.nodes[node].firstChild; c != -1 && lines.size() < 40; c = frame.nodes[c].nextSibling) {
				lines.push_back(
					std::string((frame.nodes[c].depth - 1) * 2, ' ') + frame.nodes[c].name + "  " +
					formatMs(frame.nodes[c].ns / 1e6) + " ms  x" + std::to_string(frame.nodes[c].calls)
				);
				visit(c);
			}
		};

		lines.push_back("Frame #" + std::to_string(frame.serial) + "  " + formatMs(frame.frameNs / 1e6) + " ms");
		visit(0);
		this->_drawLine({right - (n - index) * barWidth + barWidth / 2, pos.y}, {right - (n - index) * barWidth + barWidth / 2, bottom}, sf::Color::White);

		float width = 0;

		for (auto &line : lines)
			width = std::max(width, this->_textWidth(line, 11));

		float height = lines.size() * 13.f;
		float tx = std::min<float>(mouse.x + 16, game->screen->getSize().x - width - 12);
		float ty = std::max(4.f, std::min<float>(mouse.y - height / 2, game->screen->getSize().y - height - 8));

		this->_drawRect({tx - 4, ty - 3}, {width + 8, height + 6}, LIGHT_GRAY, sf::Color{0, 0, 0, 220});
		for (size_t i = 0; i < lines.size(); i++)
			this->_drawText(lines[i], {tx, ty + i * 13.f}, sf::Color::White, 11);
	}
}

void DebugOverlay::_renderPerfCounters(Vector2f pos, Vector2f size)
{
	auto &opts = this->_manager.debugOptions;
	Profiler &profiler = Profiler::get();
	size_t n = std::min<size_t>(opts.perfFrames, profiler.size());

	this->_drawRect(pos, size, DARK_GRAY, sf::Color{0x10, 0x10, 0x10, 200});
	if (n == 0)
		return;

	size_t first = profiler.size() - n;
	float barWidth = size.x / std::max<unsigned>(opts.perfFrames, 1);

	for (unsigned c = 0; c < PROFCOUNTER_COUNT; c++) {
		uint64_t maxValue = 1;
		sf::Color color = hsv(c * 360 / PROFCOUNTER_COUNT, 0.7f, 1.f);
		sf::VertexArray array{sf::PrimitiveType::LineStrip, n};

		for (size_t i = 0; i < n; i++)
			maxValue = std::max(maxValue, profiler.getFrame(first + i).counters[c]);
		for (size_t i = 0; i < n; i++) {
			array[i].position = {
				pos.x + size.x - (n - i) * barWidth,
				pos.y + size.y - static_cast<float>(profiler.getFrame(first + i).counters[c]) / maxValue * (size.y - 4) - 2
			};
			array[i].color = color;
		}
		game->screen->draw(array);
		this->_drawText(
			std::string(getProfilerCounterName(static_cast<ProfilerCounter>(c))) + ": " +
			std::to_string(profiler.getFrame(profiler.size() - 1).counters[c]) + " (max " + std::to_string(maxValue) + ")",
			{pos.x + 4, pos.y + 2 + c * 13.f},
			color,
			11
		);
	}
}

void DebugOverlay::_renderPerfTable(Vector2f pos, Vector2f size)
{
	auto &opts = this->_manager.debugOptions;
	Profiler &profiler = Profiler::get();
	uint64_t serial = profiler.getSerial();

	this->_drawRect(pos, size, DARK_GRAY, sf::Color{0x10, 0x10, 0x10, 200});
	if (
		this->_statsSerial == UINT64_MAX ||
		this->_statsFrames != opts.perfFrames ||
		serial < this->_statsSerial ||
		serial - this->_statsSerial >= 10
	) {
		this->_stats = profiler.computeStats(opts.perfFrames);
		this->_statsSerial = serial;
		this->_statsFrames = opts.perfFrames;
	}

	std::vector<const Profiler::SectionStat *> rows;

	for (auto &stat : this->_stats)
		rows.push_back(&stat);
	switch (opts.perfSort) {
	case PERFSORT_AVERAGE:
		std::stable_sort(rows.begin(), rows.end(), [](auto *a, auto *b) { return a->averageMs > b->averageMs; });
		break;
	case PERFSORT_MAX:
		std::stable_sort(rows.begin(), rows.end(), [](auto *a, auto *b) { return a->maxMs > b->maxMs; });
		break;
	case PERFSORT_CALLS:
		std::stable_sort(rows.begin(), rows.end(), [](auto *a, auto *b) { return a->averageCalls > b->averageCalls; });
		break;
	case PERFSORT_NAME:
		break;
	}

	constexpr float LINE_HEIGHT = 13;
	size_t maxRows = size.y / LINE_HEIGHT - 1;
	float y = pos.y + 2;
	const float cols[] = {4, 330, 400, 470, 550};

	this->_drawText("Section", {pos.x + cols[0], y}, sf::Color::Yellow, 11);
	this->_drawText("avg ms", {pos.x + cols[1], y}, sf::Color::Yellow, 11);
	this->_drawText("max ms", {pos.x + cols[2], y}, sf::Color::Yellow, 11);
	this->_drawText("calls", {pos.x + cols[3], y}, sf::Color::Yellow, 11);
	y += LINE_HEIGHT;
	for (size_t i = 0; i < rows.size() && i < maxRows; i++) {
		auto &row = *rows[i];
		std::string name = opts.perfSort == PERFSORT_NAME ? std::string(row.depth * 2, ' ') + row.name : row.path;

		this->_drawText(name, {pos.x + cols[0], y}, colorForName(row.name.c_str()), 11);
		this->_drawText(formatMs(row.averageMs), {pos.x + cols[1], y}, sf::Color::White, 11);
		this->_drawText(formatMs(row.maxMs), {pos.x + cols[2], y}, sf::Color::White, 11);
		this->_drawText(formatMs(row.averageCalls), {pos.x + cols[3], y}, sf::Color::White, 11);
		y += LINE_HEIGHT;
	}
	if (rows.size() > maxRows)
		this->_drawText("... " + std::to_string(rows.size() - maxRows) + " more", {pos.x + cols[0], y}, LIGHT_GRAY, 11);
}
