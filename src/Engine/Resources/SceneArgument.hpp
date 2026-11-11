//
// Created by PinkySmile on 11/02/23.
//

#ifndef MIXRTS_SCENEARGUMENT_HPP
#define MIXRTS_SCENEARGUMENT_HPP


#include <functional>
#include <string>
#include "Utils.hpp"
#include "Data/GameManager.hpp"

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
};
struct InGameArguments : public SceneArguments {
	GameStartParams startParams;
};
struct SpectatorArguments : public SceneArguments {
	class SpectatorConnection *connection;
};


#endif //MIXRTS_SCENEARGUMENT_HPP
