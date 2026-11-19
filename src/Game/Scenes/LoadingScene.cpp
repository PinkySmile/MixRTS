//
// Created by PinkySmile on 03/12/22
//

#include "LoadingScene.hpp"

#include <SFML/Window/Context.hpp>
#include "TitleScreen.hpp"
#include "Resources/Assert.hpp"
#include "Resources/Game.hpp"

namespace {
// The map render texture leaves its private GL context current on this thread.
// GLX keeps that binding after the thread exits, and the next glXMakeContextCurrent
// then fails with BadAccess. A temporary context takes over on this thread and
// releases it again in its destructor.
void releaseGlContextOnThisThread()
{
	sf::Context release;
}
}

LoadingScene::LoadingScene(
	const std::function<IScene *(LoadingScene *)> &fct,
	const std::function<void (IScene *)> &onLoadingFinished,
	const std::function<void (LoadingScene *)> &onUpdate
) :
	onLoadingFinished(onLoadingFinished),
	onUpdate(onUpdate)
{
	this->_thread = std::thread{[this, fctCopy = std::make_unique<std::function<IScene *(LoadingScene *)>>(fct)]{
		try {
			auto val = (*fctCopy)(this);

			this->setStatus("Cleaning up...");
			this->_nextScene = val;
		} catch (std::exception &e) {
			std::stringstream stack;

			if (auto *ce = dynamic_cast<const cpptrace::exception *>(&e))
				ce->trace().print(stack);
			else
				cpptrace::from_current_exception().print(stack);

			this->_mutex.lock();
			this->_status = e.what();
			this->_errored = true;
			this->_mutex.unlock();
			game->logger.error(e.what());
			game->logger.error(stack.str());
		}
		releaseGlContextOnThisThread();
	}};
#ifndef __EMSCRIPTEN__
	pthread_setname_np(this->_thread.native_handle(), "Scene loading thread");
#endif
}

LoadingScene::~LoadingScene()
{
	game->logger.debug("~LoadingScene()");
	if (this->_thread.joinable())
		this->_thread.join();
}

void LoadingScene::render() const
{
	ViewPort view{{{0, 0}, {1680, 960}}};

	game->screen->setView(view);
	game->screen->clear({0xA0, 0xA0, 0xA0});
	game->screen->borderColor(2, Color::Black);
	game->screen->fillColor();
	game->screen->textSize(50);
	game->screen->displayElement("Loading...", {0, 440}, 1680, Screen::ALIGN_CENTER);
	this->_mutex.lock();
	game->screen->displayElement(this->_status, {0, 490}, 1680, Screen::ALIGN_CENTER);
	this->_mutex.unlock();
	game->screen->borderColor();
	game->screen->textSize(30);
}

void LoadingScene::renderAbove() const
{
}

void LoadingScene::update()
{
	if (this->onUpdate)
		this->onUpdate(this);
	if (this->_errored) {
		auto args = new TitleScreenArguments();

		args->message = this->_status;
		game->scene.switchScene("title_screen", args);
	}
	if (this->_nextScene) {
		this->onLoadingFinished(this->_nextScene);
		this->_nextScene = nullptr;
	}
}

void LoadingScene::consumeEvent(const sf::Event &event)
{
}

tgui::Panel::Ptr LoadingScene::getPanel() const
{
	return nullptr;
}

void LoadingScene::setStatus(const std::wstring &status)
{
	this->_mutex.lock();
	this->_status = status;
	this->_mutex.unlock();
}

void LoadingScene::setStatus(const std::string &status)
{
	this->_mutex.lock();
	this->_status = status;
	this->_mutex.unlock();
}

LoadingScene *LoadingScene::create(SceneArguments *args)
{
	checked_cast(arg, LoadingArguments, args);

	// We copy the arguments because we will run everything in a thread.
	auto &fct = arg->fct;
	auto realArgs = arg->args;

	return new LoadingScene([&fct, realArgs](LoadingScene *me){
		if (realArgs) {
			realArgs->reportProgressA = [me](const std::string &t) {
				me->setStatus(t);
			};
			realArgs->reportProgressW = [me](const std::wstring &t) {
				me->setStatus(t);
			};
		}
		return fct(realArgs);
	}, arg->onLoadingFinished);
}