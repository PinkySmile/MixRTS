//
// Created by PinkySmile on 11/02/23.
//

#ifndef MIXRTS_SCENEARGUMENT_HPP
#define MIXRTS_SCENEARGUMENT_HPP


#include <functional>
#include <memory>
#include <string>
#include "Utils.hpp"
#include "Data/GameManager.hpp"
#include "Data/Replay.hpp"

struct SceneArguments {
	std::function<void (const std::string &)> reportProgressA;
	std::function<void (const std::wstring &)> reportProgressW;

	virtual ~SceneArguments() = default;
};

struct LobbySettings {
	float speed;
	bool shortGame;
	bool randomSpawn;
};

struct PlayerArg {
	std::string faction;
	std::optional<Vector2f> spawnPoint;
	std::optional<unsigned> spawnPointIndex;
	HSLColor color;
	char team;
	bool control;
};

struct PlayerParams {
	std::string faction;
	Vector2f spawnPoint;
	HSLColor color;
	char team;
	bool control;
};

struct GameStartParams {
	unsigned seed;
	std::string map;
	std::vector<PlayerParams> players;
	LobbySettings settings;
};

struct TitleScreenArguments : public SceneArguments {
	std::optional<std::string> message;
};

struct InGameArguments : public SceneArguments {
	GameStartParams startParams;
	//! When set, the match is rebuilt from this file and paused on the saved tick.
	std::shared_ptr<const SaveStateFile> state;

	~InGameArguments() override;
};
struct ReplayArguments : public SceneArguments {
	ReplayData replay;
};
struct SpectatorArguments : public SceneArguments {
	class SpectatorConnection *connection;
};


#endif //MIXRTS_SCENEARGUMENT_HPP
