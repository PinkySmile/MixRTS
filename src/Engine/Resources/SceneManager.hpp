//
// Created by PinkySmile on 11/02/23.
//

#ifndef MIXRTS_SCENEMANAGER_HPP
#define MIXRTS_SCENEMANAGER_HPP


#include <functional>
#include <map>
#include "IScene.hpp"
#include "SceneArgument.hpp"

struct LoadingArguments : public SceneArguments {
	std::function<void (IScene *)> onLoadingFinished;
	const std::function<IScene *(SceneArguments *args)> &fct;
	SceneArguments *args;
	IScene *loadingScene = nullptr;

	LoadingArguments(const std::function<IScene *(SceneArguments *args)> &fct) : fct(fct) {};
	~LoadingArguments() override = default;
};

class SceneManager {
private:
	struct SceneInitializer {
		std::string name;
		SceneArguments *args = nullptr;

		~SceneInitializer() { delete this->args; }
	};
	struct SceneConstructor {
		std::function<IScene *(SceneArguments *args)> callback;
		bool needLoading;
	};

	bool _loading = false;
	mutable sf::Clock _clock;
	std::string _currentScene;
	SceneInitializer _nextScene;
	mutable std::recursive_mutex _sceneMutex;
	std::shared_ptr<IScene> _scene;
	std::map<std::string, SceneConstructor> _factory;

	void _applySwitchScene();

public:
	void update();
	void render() const;
	void renderAbove() const;
	void consumeEvent(const sf::Event &event);
	void switchScene(const std::string &name, SceneArguments *args = nullptr, bool force = false);
	SceneConstructor registerScene(
		const std::string &name,
		const std::function<IScene *(SceneArguments *args)> &constructor,
		bool requireLoadingScene
	);
	std::pair<std::string, IScene *> getCurrentScene();
	bool isLoading();
};


#endif //MIXRTS_SCENEMANAGER_HPP
