//
// Created by PinkySmile on 12/11/2026.
//

#include <cstring>
#include <fstream>
#include <stdexcept>
#include "Replay.hpp"
#include "Resources/SceneArgument.hpp"
#include "Resources/Assert.hpp"

namespace {

class BinaryWriter {
private:
	std::ofstream &_out;

public:
	explicit BinaryWriter(std::ofstream &out) : _out(out) {}

	void bytes(const void *data, size_t size)
	{
		this->_out.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
		if (!this->_out)
			throw std::runtime_error("Failed to write replay file");
	}

	void u8(uint8_t v) { this->bytes(&v, 1); }

	void u16(uint16_t v)
	{
		uint8_t b[2] = {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8)};
		this->bytes(b, 2);
	}

	void u32(uint32_t v)
	{
		uint8_t b[4] = {
			static_cast<uint8_t>(v),
			static_cast<uint8_t>(v >> 8),
			static_cast<uint8_t>(v >> 16),
			static_cast<uint8_t>(v >> 24)
		};
		this->bytes(b, 4);
	}

	void f32(float v)
	{
		uint32_t bits;
		static_assert(sizeof(float) == 4);
		std::memcpy(&bits, &v, 4);
		this->u32(bits);
	}

	void varint(uint64_t v)
	{
		while (v >= 0x80) {
			this->u8(static_cast<uint8_t>(v) | 0x80);
			v >>= 7;
		}
		this->u8(static_cast<uint8_t>(v));
	}

	void str(const std::string &s)
	{
		this->varint(s.size());
		this->bytes(s.data(), s.size());
	}

	void vec2f(Vector2f v)
	{
		this->f32(v.x);
		this->f32(v.y);
	}

	void vec2s(Vector2u v)
	{
		assert_msg(v.x <= 65535 && v.y <= 65535, "Replay coordinate does not fit in 16 bits");
		this->u16(static_cast<uint16_t>(v.x));
		this->u16(static_cast<uint16_t>(v.y));
	}

	void indices(const std::vector<unsigned> &values)
	{
		this->varint(values.size());
		for (unsigned value : values)
			this->u32(value);
	}
};

class BinaryReader {
private:
	std::ifstream &_in;

public:
	explicit BinaryReader(std::ifstream &in) : _in(in) {}

	void bytes(void *data, size_t size)
	{
		this->_in.read(static_cast<char *>(data), static_cast<std::streamsize>(size));
		if (this->_in.gcount() != static_cast<std::streamsize>(size))
			throw std::runtime_error("Truncated replay file");
	}

	uint8_t u8()
	{
		uint8_t v;
		this->bytes(&v, 1);
		return v;
	}

	uint16_t u16()
	{
		uint8_t b[2];
		this->bytes(b, 2);
		return static_cast<uint16_t>(b[0] | (b[1] << 8));
	}

	uint32_t u32()
	{
		uint8_t b[4];
		this->bytes(b, 4);
		return static_cast<uint32_t>(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
	}

	float f32()
	{
		uint32_t bits = this->u32();
		float v;
		std::memcpy(&v, &bits, 4);
		return v;
	}

	uint64_t varint()
	{
		uint64_t v = 0;
		int shift = 0;

		for (int i = 0; i < 10; i++) {
			uint8_t byte = this->u8();

			v |= static_cast<uint64_t>(byte & 0x7F) << shift;
			if ((byte & 0x80) == 0)
				return v;
			shift += 7;
		}
		throw std::runtime_error("Replay varint is too long");
	}

	std::string str()
	{
		uint64_t len = this->varint();

		if (len > 1'000'000)
			throw std::runtime_error("Replay string is too long");

		std::string s(static_cast<size_t>(len), '\0');
		if (len)
			this->bytes(s.data(), static_cast<size_t>(len));
		return s;
	}

	Vector2f vec2f()
	{
		float x = this->f32();
		float y = this->f32();
		return {x, y};
	}

	Vector2u vec2s()
	{
		return {this->u16(), this->u16()};
	}

	std::vector<unsigned> indices()
	{
		uint64_t len = this->varint();

		if (len > 100'000)
			throw std::runtime_error("Replay unit list is too long");

		std::vector<unsigned> values(static_cast<size_t>(len));
		for (auto &value : values)
			value = this->u32();
		return values;
	}

	void skipInput()
	{
		this->u16();
		this->u16();

		uint8_t bits[16];
		this->bytes(bits, 16);

		// Bit 126: camera position. Bit 127: camera size. Both are Vector2s.
		if ((bits[15] >> 6) & 1)
			this->u32();
		if ((bits[15] >> 7) & 1)
			this->u32();
	}
};

enum ActionId : uint8_t {
	ACT_START_BUILD = 0,
	ACT_PLACE_BUILD = 1,
	ACT_CANCEL_BUILD = 2,
	ACT_DEPLOY = 3,
	ACT_MOVE = 4,
	ACT_HARVEST = 5,
	ACT_DUMP = 6,
	ACT_SET_PRIMARY = 7,
	ACT_REPAIR = 8,
	ACT_SELL = 9,
	ACT_ATTACK = 10,
	ACT_STOP = 11,
	ACT_OPEN_QUEUE = 12,
	ACT_CLOSE_QUEUE = 13,
	ACT_SCREEN = 200,
	ACT_SETTINGS = 201,
	ACT_UI = 202,
};

void writeAction(BinaryWriter &out, const ReplayAction &action)
{
	std::visit([&out](auto &&arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, UI::StartBuildEvent>) {
			out.u8(ACT_START_BUILD);
			out.str(arg.id);
			out.str(arg.queue);
		} else if constexpr (std::is_same_v<T, UI::PlaceBuildEvent>) {
			out.u8(ACT_PLACE_BUILD);
			out.str(arg.id);
			out.str(arg.queue);
			out.vec2s(arg.position);
		} else if constexpr (std::is_same_v<T, UI::CancelBuildEvent>) {
			out.u8(ACT_CANCEL_BUILD);
			out.str(arg.id);
			out.str(arg.queue);
		} else if constexpr (std::is_same_v<T, UI::DeployUnitEvent>) {
			out.u8(ACT_DEPLOY);
			out.u32(arg.index);
		} else if constexpr (std::is_same_v<T, UI::MoveUnitEvent>) {
			out.u8(ACT_MOVE);
			out.indices(arg.indices);
			out.vec2s(arg.position);
		} else if constexpr (std::is_same_v<T, UI::HarvestUnitEvent>) {
			out.u8(ACT_HARVEST);
			out.indices(arg.indices);
			out.vec2s(arg.position);
		} else if constexpr (std::is_same_v<T, UI::DumpUnitEvent>) {
			out.u8(ACT_DUMP);
			out.indices(arg.indices);
			out.u32(arg.hole);
		} else if constexpr (std::is_same_v<T, UI::SetPrimaryEvent>) {
			out.u8(ACT_SET_PRIMARY);
			out.u32(arg.index);
		} else if constexpr (std::is_same_v<T, UI::RepairEvent>) {
			out.u8(ACT_REPAIR);
			out.u32(arg.index);
		} else if constexpr (std::is_same_v<T, UI::SellEvent>) {
			out.u8(ACT_SELL);
			out.u32(arg.index);
		} else if constexpr (std::is_same_v<T, UI::AttackUnitEvent>) {
			out.u8(ACT_ATTACK);
			out.indices(arg.indices);
			out.u32(arg.target);
		} else if constexpr (std::is_same_v<T, UI::StopEvent>) {
			out.u8(ACT_STOP);
			out.indices(arg.indices);
		} else if constexpr (std::is_same_v<T, UI::OpenQueueEvent>) {
			out.u8(ACT_OPEN_QUEUE);
			out.str(arg.queue);
		} else if constexpr (std::is_same_v<T, UI::CloseQueueEvent>) {
			out.u8(ACT_CLOSE_QUEUE);
			out.str(arg.queue);
		} else if constexpr (std::is_same_v<T, ScreenSizeChanged>) {
			out.u8(ACT_SCREEN);
			out.vec2s(arg.size);
		} else if constexpr (std::is_same_v<T, SettingsChanged>) {
			out.u8(ACT_SETTINGS);
			out.f32(arg.scale);
		} else if constexpr (std::is_same_v<T, UIChanged>) {
			out.u8(ACT_UI);
			out.u32(arg.index);
		} else
			static_assert(sizeof(T) == 0, "Unhandled replay action");
	}, action);
}

ReplayAction readAction(BinaryReader &in)
{
	switch (in.u8()) {
	case ACT_START_BUILD: {
		auto id = in.str();
		auto queue = in.str();
		return UI::StartBuildEvent{std::move(id), std::move(queue)};
	}
	case ACT_PLACE_BUILD: {
		auto id = in.str();
		auto queue = in.str();
		return UI::PlaceBuildEvent{std::move(id), std::move(queue), in.vec2s()};
	}
	case ACT_CANCEL_BUILD: {
		auto id = in.str();
		auto queue = in.str();
		return UI::CancelBuildEvent{std::move(id), std::move(queue)};
	}
	case ACT_DEPLOY:
		return UI::DeployUnitEvent{in.u32()};
	case ACT_MOVE: {
		auto ids = in.indices();
		return UI::MoveUnitEvent{std::move(ids), in.vec2s()};
	}
	case ACT_HARVEST: {
		auto ids = in.indices();
		return UI::HarvestUnitEvent{std::move(ids), in.vec2s()};
	}
	case ACT_DUMP: {
		auto ids = in.indices();
		return UI::DumpUnitEvent{std::move(ids), in.u32()};
	}
	case ACT_SET_PRIMARY:
		return UI::SetPrimaryEvent{in.u32()};
	case ACT_REPAIR:
		return UI::RepairEvent{in.u32()};
	case ACT_SELL:
		return UI::SellEvent{in.u32()};
	case ACT_ATTACK: {
		auto ids = in.indices();
		return UI::AttackUnitEvent{std::move(ids), in.u32()};
	}
	case ACT_STOP:
		return UI::StopEvent{in.indices()};
	case ACT_OPEN_QUEUE:
		return UI::OpenQueueEvent{in.str()};
	case ACT_CLOSE_QUEUE:
		return UI::CloseQueueEvent{in.str()};
	case ACT_SCREEN:
		return ScreenSizeChanged{in.vec2s()};
	case ACT_SETTINGS:
		return SettingsChanged{in.f32()};
	case ACT_UI:
		return UIChanged{in.u32()};
	default:
		throw std::runtime_error("Unknown replay action");
	}
}

}

ReplayData ReplayData::load(const std::filesystem::path &path)
{
	std::ifstream stream{path, std::ios::binary};

	if (!stream)
		throw std::runtime_error("Could not open replay file \"" + path.string() + "\"");

	BinaryReader in{stream};
	ReplayData data;

	if (in.u32() != MAGIC)
		throw std::runtime_error("Not a replay file");
	if (in.u8() != VERSION)
		throw std::runtime_error("Unsupported replay version");
	data.seed = in.u32();
	data.map = in.str();
	data.mapCrc = in.u32();
	data.speed = in.f32();
	data.flags = in.u8();

	uint8_t playerCount = in.u8();
	data.players.resize(playerCount);
	data.actions.resize(playerCount);
	for (auto &player : data.players) {
		player.name = in.str();
		player.faction = in.str();
		player.factionCrc = in.u32();
		player.spawn = in.vec2f();
		player.color.h = in.u16();
		player.color.s = in.u16();
		player.color.l = in.u16();
		player.team = in.u8();
	}
	data.frameCount = in.u32();

	bool hasInputs = data.flags & FLAG_INPUTS;
	for (uint32_t frame = 0; frame < data.frameCount; frame++) {
		for (size_t player = 0; player < playerCount; player++) {
			if (hasInputs)
				in.skipInput();

			uint8_t count = in.u8();
			for (uint8_t i = 0; i < count; i++)
				data.actions[player].push_back(RecordedAction{frame + 1, readAction(in)});
		}
	}
	return data;
}

void ReplayData::save(const std::filesystem::path &path) const
{
	std::ofstream stream{path, std::ios::binary};

	if (!stream)
		throw std::runtime_error("Could not write replay file \"" + path.string() + "\"");
	assert_exp(this->actions.size() == this->players.size());

	BinaryWriter out{stream};
	std::vector<std::vector<std::vector<ReplayAction>>> frames(this->frameCount, std::vector<std::vector<ReplayAction>>(this->players.size()));

	for (size_t player = 0; player < this->actions.size(); player++) {
		for (const auto &recorded : this->actions[player]) {
			assert_msg(recorded.frame >= 1 && recorded.frame <= this->frameCount, "Replay action is outside the recorded frames");
			frames[recorded.frame - 1][player].push_back(recorded.action);
		}
	}

	out.u32(MAGIC);
	out.u8(VERSION);
	out.u32(this->seed);
	out.str(this->map);
	out.u32(this->mapCrc);
	out.f32(this->speed);
	out.u8(this->flags & ~FLAG_INPUTS);
	out.u8(static_cast<uint8_t>(this->players.size()));
	for (const auto &player : this->players) {
		out.str(player.name);
		out.str(player.faction);
		out.u32(player.factionCrc);
		out.vec2f(player.spawn);
		out.u16(player.color.h);
		out.u16(player.color.s);
		out.u16(player.color.l);
		out.u8(player.team);
	}
	out.u32(this->frameCount);
	for (const auto &frame : frames) {
		for (const auto &playerFrame : frame) {
			assert_msg(playerFrame.size() <= 255, "Too many actions in one replay frame");
			out.u8(static_cast<uint8_t>(playerFrame.size()));
			for (const auto &action : playerFrame)
				writeAction(out, action);
		}
	}
}

GameStartParams ReplayData::toGameStartParams() const
{
	GameStartParams params;

	params.seed = this->seed;
	params.map = this->map;
	params.settings.speed = this->speed;
	params.settings.shortGame = this->flags & FLAG_SHORT_GAME;
	params.settings.randomSpawn = this->flags & FLAG_RANDOM_SPAWN;
	for (size_t i = 0; i < this->players.size(); i++) {
		const auto &player = this->players[i];

		params.players.push_back(PlayerParams{
			player.faction,
			player.spawn,
			player.color,
			static_cast<char>(player.team),
			i == 0
		});
	}
	return params;
}

ReplayData ReplayData::fromGame(
	const GameStartParams &params,
	uint32_t mapCrc,
	const std::vector<uint32_t> &factionCrcs,
	uint32_t frameCount,
	std::vector<std::vector<RecordedAction>> actions
)
{
	assert_exp(factionCrcs.size() == params.players.size());
	assert_exp(actions.size() == params.players.size());

	ReplayData data;

	data.seed = params.seed;
	data.map = params.map;
	data.mapCrc = mapCrc;
	data.speed = params.settings.speed;
	data.flags = 0;
	if (params.settings.shortGame)
		data.flags |= FLAG_SHORT_GAME;
	if (params.settings.randomSpawn)
		data.flags |= FLAG_RANDOM_SPAWN;
	data.frameCount = frameCount;
	data.actions = std::move(actions);
	for (size_t i = 0; i < params.players.size(); i++) {
		const auto &player = params.players[i];

		data.players.push_back(ReplayPlayerInfo{
			"Player " + std::to_string(i + 1),
			player.faction,
			factionCrcs[i],
			player.spawnPoint,
			player.color,
			static_cast<uint8_t>(player.team)
		});
	}
	return data;
}
