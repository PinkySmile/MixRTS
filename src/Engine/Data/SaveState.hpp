//
// Created by PinkySmile on 13/11/2026.
//

#ifndef MIXRTS_SAVESTATE_HPP
#define MIXRTS_SAVESTATE_HPP


#include <cstdint>
#include <filesystem>
#include <vector>
#include "Data/Vector.hpp"
#include "Resources/SceneArgument.hpp"

//! A .state file: the match that was running, the saving player's camera, and one simulation blob.
struct MYDLL_API SaveStateFile {
	static constexpr uint32_t MAGIC = 0x45544153;
	static constexpr uint32_t VERSION = 1;

	GameStartParams start;
	//! One faction checksum per player, in the same order as start.players.
	std::vector<uint32_t> factionCrcs;
	uint32_t mapCrc = 0;
	Vector2f camera;
	float zoom = 1;
	GameManager::GameStateSnapshot snapshot;

	static SaveStateFile load(const std::filesystem::path &path);
	void save(const std::filesystem::path &path) const;
};


#endif //MIXRTS_SAVESTATE_HPP
