//
// Created by PinkySmile on 09/11/2026.
//

#include <iostream>
#include <fstream>
#include "UI/BaseUI.hpp"
#include "Resources/Game.hpp"
#include "Crash.hpp"
#include "Utils.hpp"

static tgui::Font *traceFont = nullptr;

tgui::Panel::Ptr generateCrashPanel(Stacktrace st, const std::exception &e)
{
	tgui::Panel::Ptr panel = tgui::Panel::create({"100%", "100%"});
	tgui::Panel::Ptr out = tgui::Panel::create();
	std::string exc = getLastExceptionName();

	cleanStackTrace(st);
	if (exc == "_AssertionFailedException")
		exc = "AssertionFailed";
	std::string whatStr = e.what();
	simplifySym(whatStr);

	if (!traceFont)
		traceFont = new tgui::Font("assets/NotoSansMono-Regular.ttf");
	auto title = tgui::Label::create("Game crashed !");
	auto exception = tgui::RichTextLabel::create("Exception: <color=#ff0000>" + exc + "</color>");
	auto what = tgui::Label::create(whatStr);
	std::stringstream ss;

	title->setTextSize(40);
	title->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
	title->setPosition({10, 10});
	title->setSize({"&.w - 20", 60});
	title->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
	panel->add(title, "title");

	exception->setPosition({10, "title.y + title.h + 10"});
	exception->setSize({"&.w - 20", 20});
	panel->add(exception, "signal");

	what->setPosition({10, "signal.y + signal.h + 10"});
	what->setAutoSize(true);
	panel->add(what, "faultAddr");
	what->setSize({tgui::Layout{"max(&.w - 20, " + std::to_string(what->getSize().x) + ")"}, what->getSize().y});

	std::string richString = stacktraceToRichString(st);

	auto stackTrace = tgui::RichTextLabel::create(richString);
	stackTrace->setAutoSize(true);
	stackTrace->getRenderer()->setTextColor(tgui::Color::White);
	stackTrace->getRenderer()->setFont(*traceFont);

	auto spanel = tgui::ScrollablePanel::create();
	spanel->setPosition({10, "faultAddr.y + faultAddr.h + 10"});
	spanel->setSize({"&.w - 20", "&.h - y"});
	spanel->getRenderer()->setBackgroundColor(tgui::Color{0x40, 0x40, 0x40});
	spanel->add(stackTrace);
	panel->add(spanel, "last");


	time_t timer;
	char timebuffer[31];
	tm *tm_info;

	time(&timer);
	tm_info = localtime(&timer);
	strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d-%H-%M-%S", tm_info);

	std::string folder = "crash-";
	std::error_code err;
	folder += timebuffer;
	std::filesystem::path path = folder;
	auto crashPlaceLabel = tgui::Label::create(L"Crash information has been stored at " + std::filesystem::absolute(folder).wstring());

	std::filesystem::create_directory(path, err);
	std::filesystem::copy("latest.log", path / "log_file.log", err);
	std::ofstream stream{path / "stacktrace.txt"};

	stream << "Exception: " << exc << std::endl;
	stream << whatStr << std::endl << std::endl;
	st.print(stream, false);
	stream.close();

	std::cerr << "Exception: " << exc << std::endl;
	std::cerr << whatStr << std::endl << std::endl;
	st.print(std::cerr, false);

	crashPlaceLabel->setPosition({10, "100% - h - 10"});
	crashPlaceLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold | tgui::TextStyle::Underlined);
	panel->setSize({"100%", "bottom.y - 10"});

	out->add(crashPlaceLabel, "bottom");
	out->add(panel);
	return out;
}

void handleCrash(Stacktrace st, const std::exception &e)
{
	Screen *window;
	tgui::Gui *gui;
	tgui::ChildWindow::Ptr out;
	sf::Texture *background = nullptr;
	sf::Sprite *sprite = nullptr;

	guiScale = 1;
	if (game) {
		std::stringstream ss;

		st.print(ss, false);
		game->logger.fatal(e.what());
		game->logger.fatal(ss.str());
		window = &*game->screen;
		background = new sf::Texture(window->getSize());
		background->update(*window);
		sprite = new sf::Sprite{*background};
		gui = &game->gui;
		gui->removeAllWidgets();

		auto pan = tgui::Panel::create({"100%", "100%"});

		pan->getRenderer()->setBackgroundColor({0, 0, 0, 175});
		pan->setUserData(false);
		gui->add(pan);

		out = tgui::ChildWindow::create();
		out->setSize(1000, 600);
		out->setPosition("(&.w - w) / 2", "(&.h - h) / 2");
		gui->add(out);
		out->setFocused(true);
		out->setTitleButtons(0);
	} else {
		window = new Screen{"Game crashed!"};
		window->setSize({1000, 600});
		gui = new tgui::Gui{*window};
	}

	reloadViews(*window, *gui, window->getSize(), {0, 0});
	window->setMouseCursorGrabbed(false);
	window->setMouseCursorVisible(true);

	auto panel = generateCrashPanel(st, e);

	if (out)
		out->add(panel);
	else
		gui->add(panel);

	window->setFramerateLimit(30);
	while (window->isOpen()) {
		while (auto event = window->pollEvent()) {
			if (event->is<sf::Event::Closed>())
				window->close();
			else if (auto resized = event->getIf<sf::Event::Resized>())
				reloadViews(*window, *gui, resized->size, {0, 0});
			gui->handleEvent(*event);
		}
		window->clear(sf::Color{0xC0, 0xC0, 0xC0});
		if (sprite) {
			sprite->setScale({
				static_cast<float>(window->getSize().x) / background->getSize().x,
				static_cast<float>(window->getSize().y) / background->getSize().y
			});
			window->draw(*sprite);
		}
		gui->draw();
		window->display();
	}
	delete traceFont;
	if (!game) {
		delete window;
		delete gui;
	}
	delete sprite;
	delete background;
}