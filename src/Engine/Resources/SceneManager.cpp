//
// Created by PinkySmile on 11/02/23.
//

#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"
#include "SceneManager.hpp"
#include "UI/BaseUI.hpp"

void SceneManager::update()
{
	if (!this->_loading && !this->_nextScene.name.empty() && this->_currentScene != this->_nextScene.name)
		this->_applySwitchScene();

	std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};

	if (this->_scene)
		this->_scene->update();
	if (!this->_loading)
		this->_oldScene.reset();
}

void SceneManager::render() const
{
	std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};

	assert_exp(this->_scene);
	this->_scene->render();
}

void SceneManager::renderAbove() const
{
	{
		std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};

		assert_exp(this->_scene);
		this->_scene->renderAbove();
	}
	dispFPS(this->_clock.restart().asSeconds());
}

void SceneManager::consumeEvent(const sf::Event &event)
{
	std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};

	assert_exp(this->_scene);
	this->_scene->consumeEvent(event);
}

void SceneManager::switchScene(const std::string &name, SceneArguments *args, bool force)
{
	if (!force)
		assert_exp(name != this->_currentScene);
	assert_msg(this->_factory.contains(name), "Scene '" + name + "' doesn't exist");
	assert_exp(!this->_factory[name].needLoading || this->_factory.contains("loading"));
	assert_exp(!this->_factory[name].needLoading || !this->_loading);
	delete this->_nextScene.args;
	this->_nextScene.args = args;
	this->_nextScene.name = name;
	this->_loading = false;
	if (!this->_scene)
		this->_applySwitchScene();
}

SceneManager::SceneConstructor SceneManager::registerScene(
	const std::string &name,
	const std::function<IScene *(SceneArguments *)> &constructor,
	bool requireLoadingScene
)
{
	auto old = this->_factory[name];

	assert_exp(name != "loading" || !requireLoadingScene);
	this->_factory[name].callback = constructor;
	this->_factory[name].needLoading = requireLoadingScene;
	return old;
}

bool SceneManager::isLoading() const
{
	return this->_loading;
}

void SceneManager::cancelLoading()
{
	this->_loading = false;
}

std::pair<std::string, IScene *> SceneManager::getCurrentScene()
{
	return {
		this->_currentScene,
		&*this->_scene
	};
}

void SceneManager::_applySwitchScene()
{
	auto &factory = this->_factory[this->_nextScene.name];
	auto cb = [this](IScene *result){
		assert_exp(result);

		std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};

		tgui::Panel::Ptr oldPan = this->_scene ? this->_scene->getPanel() : nullptr;
		tgui::Panel::Ptr newPan = result->getPanel();

		if (oldPan)
			game->gui.remove(oldPan);
		if (newPan)
			game->gui.add(newPan);
		this->_oldScene = this->_scene;
		this->_scene.reset(result);
		this->_loading = false;
	};

	if (factory.needLoading) {
		LoadingArguments arg{factory.callback};

		this->_loading = true;
		arg.onLoadingFinished = cb;
		arg.args = this->_nextScene.args;

		std::unique_lock<std::recursive_mutex> guard{this->_sceneMutex};
		IScene *result = this->_factory["loading"].callback(&arg);
		assert_exp(result);
		tgui::Panel::Ptr oldPan = this->_scene ? this->_scene->getPanel() : nullptr;
		tgui::Panel::Ptr newPan = result->getPanel();

		if (oldPan)
			game->gui.remove(oldPan);
		if (newPan)
			game->gui.add(newPan);
		this->_scene.reset(result);
	} else
		cb(factory.callback(this->_nextScene.args));
	this->_currentScene = this->_nextScene.name;
	this->_nextScene.name.clear();
}