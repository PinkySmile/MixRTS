//
// Created by PinkySmile on 13/11/2026.
//

#include <fstream>
#include <stdexcept>
#include "SaveState.hpp"
#include "StateStream.hpp"
#include "Utils.hpp"

namespace {
	void writeFile(const std::filesystem::path &path, const std::vector<uint8_t> &bytes)
	{
		std::ofstream stream(path, std::ios::binary);

		if (!stream)
			throw std::runtime_error("Could not write state file \"" + path.string() + "\"");
		stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!stream)
			throw std::runtime_error("Could not write state file \"" + path.string() + "\"");
	}

	std::vector<uint8_t> readFile(const std::filesystem::path &path)
	{
		std::ifstream stream(path, std::ios::binary);
		std::vector<uint8_t> bytes;

		if (!stream)
			throw std::runtime_error("Could not read state file \"" + path.string() + "\"");
		stream.seekg(0, std::ios::end);

		auto size = stream.tellg();

		if (size < 0)
			throw std::runtime_error("Could not read state file \"" + path.string() + "\"");
		bytes.resize(static_cast<size_t>(size));
		stream.seekg(0, std::ios::beg);
		if (size && !stream.read(reinterpret_cast<char *>(bytes.data()), size))
			throw std::runtime_error("Could not read state file \"" + path.string() + "\"");
		return bytes;
	}
}

SaveStateFile SaveStateFile::load(const std::filesystem::path &path)
{
	std::vector<uint8_t> bytes = readFile(path);
	StateReader in(bytes);
	SaveStateFile file;

	if (in.u32() != MAGIC)
		throw std::runtime_error("This file is not a MixRTS state");
	if (in.u32() != VERSION)
		throw std::runtime_error("This state was written by a different version of the game");
	file.start.seed = in.u32();
	file.start.map = in.string();
	file.start.settings.speed = in.f32();
	file.start.settings.shortGame = in.boolean();
	file.start.settings.randomSpawn = in.boolean();

	size_t players = in.size();

	file.start.players.reserve(static_cast<size_t>(players));
	file.factionCrcs.reserve(static_cast<size_t>(players));
	for (uint64_t i = 0; i < players; i++) {
		PlayerParams player;

		player.faction = in.string();
		file.factionCrcs.push_back(in.u32());
		player.spawnPoint = in.vec2<float>();
		player.color.h = in.u16();
		player.color.s = in.u16();
		player.color.l = in.u16();
		player.color.a = in.u8();
		player.team = static_cast<char>(in.u8());
		player.control = in.boolean();
		file.start.players.push_back(std::move(player));
	}
	file.mapCrc = in.u32();
	file.camera = in.vec2<float>();
	file.zoom = in.f32();
	file.snapshot.checksum = in.u64();

	uint64_t blobSize = in.u64();

	if (blobSize != in.remaining())
		throw std::runtime_error("This state file is truncated or has trailing bytes");
	file.snapshot.data.resize(static_cast<size_t>(blobSize));
	in.bytes(file.snapshot.data.data(), file.snapshot.data.size());
	if (GameManager::checksumState(file.snapshot.data.data(), file.snapshot.data.size()) != file.snapshot.checksum)
		throw std::runtime_error("This state file is corrupted");
	if (!file.snapshot.data.empty()) {
		StateReader blob(file.snapshot.data);

		file.snapshot.tick = blob.u8() == 1 ? blob.u64() : 0;
	}
	return file;
}

std::vector<uint8_t> SaveStateFile::encode() const
{
	std::vector<uint8_t> bytes;
	StateWriter out(bytes);

	if (this->factionCrcs.size() != this->start.players.size())
		throw std::runtime_error("State file is missing a faction checksum");
	out.u32(MAGIC);
	out.u32(VERSION);
	out.u32(this->start.seed);
	out.string(this->start.map);
	out.f32(this->start.settings.speed);
	out.boolean(this->start.settings.shortGame);
	out.boolean(this->start.settings.randomSpawn);
	out.size(this->start.players.size());
	for (size_t i = 0; i < this->start.players.size(); i++) {
		const PlayerParams &player = this->start.players[i];

		out.string(player.faction);
		out.u32(this->factionCrcs[i]);
		out.vec2(player.spawnPoint);
		out.u16(player.color.h);
		out.u16(player.color.s);
		out.u16(player.color.l);
		out.u8(player.color.a);
		out.u8(static_cast<uint8_t>(player.team));
		out.boolean(player.control);
	}
	out.u32(this->mapCrc);
	out.vec2(this->camera);
	out.f32(this->zoom);
	out.u64(this->snapshot.checksum);
	out.u64(this->snapshot.data.size());
	out.bytes(this->snapshot.data.data(), this->snapshot.data.size());
	return bytes;
}

void SaveStateFile::save(const std::filesystem::path &path) const
{
	writeFile(path, this->encode());
}
