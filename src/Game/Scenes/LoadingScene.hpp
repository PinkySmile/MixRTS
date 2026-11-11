//
// Created by PinkySmile on 03/12/22
//

#ifndef MIXRTS_LOADINGSCENE_HPP
#define MIXRTS_LOADINGSCENE_HPP


#include <mutex>
#include <thread>
#include <functional>
#include <TGUI/Widgets/Panel.hpp>
#include "Resources/IScene.hpp"
#include "Resources/SceneArgument.hpp"

class LoadingScene : public IScene {
private:
	bool _errored = false;
	mutable std::mutex _mutex;
	sf::String _status;
	IScene *_nextScene = nullptr;
	std::thread _thread;
	std::function<void (IScene *)> onLoadingFinished;
	std::function<void (LoadingScene *)> onUpdate;

public:
	LoadingScene(
		const std::function<IScene *(LoadingScene *me)> &fct,
		const std::function<void (IScene *)> &onLoadingFinished,
		const std::function<void (LoadingScene *)> &onUpdate = nullptr
	);
	~LoadingScene() override;
	void render() const override;
	void renderAbove() const override;
	void update() override;
	void setStatus(const std::string &status);
	void setStatus(const std::wstring &status);
	void consumeEvent(const sf::Event &event) override;
	[[nodiscard]] tgui::Panel::Ptr getPanel() const override;

	static LoadingScene *create(SceneArguments *);
};


#endif //MIXRTS_LOADINGSCENE_HPP
