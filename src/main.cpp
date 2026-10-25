#include <iostream>
#include <cstdio>
#include <cfenv>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Data/Animation.hpp"
#include "Data/Projectile.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Effect.hpp"
#include "Data/Weapon.hpp"
#include "Data/Warhead.hpp"
#include "Resources/Game.hpp"
#include "Resources/Assert.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "UI/UI.hpp"
#include "Debug/Crash.hpp"
#include "Debug/Profiler.hpp"
#include "Utils.hpp"

#ifdef _WIN32
#include <windows.h>
#endif


tgui::Font *traceFont = nullptr;

struct Args {
	bool loadingOnly = false;
	bool preload = false;
	bool noLazyLoad = false;
	float speed = 1;
	std::string locale = "fr";
	std::string map = "mods/dune2000/maps/8PLAY1";
	std::string players = "d2k:atreides(214,375,919)A<;d2k:harkonnen(0,800,750)B;d2k:ordos(145,250,900)C;scbw:terran(239,1000,500)D;scbw:zerg(299,1000,500)E;scbw:protoss(158,1000,500)F";
};

void reloadViews(sf::RenderWindow &win, tgui::Gui &gui, Vector2u size, Vector2f offset)
{
	PROFILE_SCOPE("reload views");
	sf::View view{sf::FloatRect{
		offset,
		{
			static_cast<float>(size.x),
			static_cast<float>(size.y)
		}
	}};
	tgui::FloatRect viewGui{
		{0, 0},
		{
			size.x / guiScale,
			size.y / guiScale
		}
	};

	win.setView(view);
	gui.setAbsoluteView(viewGui);
}

void dispFPS(float frameTime)
{
	auto old = game->screen->getView();
	auto stats = Profiler::get().getFrameStats(60);
	char details[64] = "";

	if (stats.averageMs > 0)
		snprintf(details, sizeof(details), " (a %.1f / w%.1f ms)", stats.averageMs, stats.worstMs);

	auto str = sf::String(std::to_string(static_cast<unsigned>(round(1 / frameTime))) + "FPS" + details);
	Vector2u size = game->screen->getSize();
	sf::View view{sf::FloatRect{
		{0, 0},
		size.to<float>()
	}};

	game->screen->setView(view);
	game->screen->displayElement(str, {0, size.y - 35.f}, size.x, Screen::ALIGN_RIGHT);
	game->screen->setView(old);
}

int parseArgs(int argc, char **argv, Args &args)
{
	std::map<std::string, char> longToShort = {
		{"load-only", -1},
		{"no-lazy-load", -2},
		{"preload", 'P'},
		{"players", 'p'},
		{"speed", 's'},
		{"map", 'm'},
		{"locale", 'l'}
	};
	std::map<char, std::function<bool (int &i)>> cbs = {
		{-1, [&args](int &) {
			args.loadingOnly = true;
			return true;
		}},
		{-2, [&args](int &) {
			args.noLazyLoad = true;
			return true;
		}},
		{'P', [&args](int &) {
			args.preload = true;
			return true;
		}},
		{'s', [&argc, &argv, &args](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --speed\n");
				return false;
			}
			try {
				args.speed = std::stof(argv[i]);
			} catch (...) {
				fprintf(stderr, "Invalid number sent to --speed\n");
				return false;
			}
			return true;
		}},
		{'m', [&argc, &argv, &args](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --map\n");
				return false;
			}
			args.map = argv[i];
			return true;
		}},
		{'p', [&argc, &argv, &args](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --players\n");
				return false;
			}
			args.players = argv[i];
			return true;
		}},
		{'l', [&argc, &argv, &args](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --locale\n");
				return false;
			}
			args.locale = argv[i];
			return true;
		}}
	};

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == '-') {
				auto it = longToShort.find(&argv[i][2]);

				if (it == longToShort.end()) {
					fprintf(stderr, "Unknown format %s\n", &argv[i][2]);
					return EXIT_FAILURE;
				}
				if (!cbs[it->second](i))
					return EXIT_FAILURE;
			} else for (size_t j = 1; argv[i][j]; j++) {
				auto it = cbs.find(argv[i][j]);

				if (it == cbs.end()) {
					fprintf(stderr, "Unknown format %c\n", argv[i][j]);
					return EXIT_FAILURE;
				}
				if (!it->second(i))
					return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}

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
	sf::RenderWindow *window;
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
		window = new sf::RenderWindow{sf::VideoMode{sf::Vector2u{1000, 600}}, "Game crashed!"};
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

void genSpawns(size_t size, std::vector<size_t> &array, const std::vector<Vector2u> &areas)
{
	assert_exp(areas.size() >= size);
	array.resize(size);
	array[0] = game->random() % areas.size();
	if (size == 1) return;

	double maxDist = -1.0;
	std::vector<size_t> bestSub;

	bestSub.resize(size - 1);
	// Recursive lambda to generate all combinations of (size - 1) indices from nb (excluding array[0])
	auto findBestCombination = [&](auto &self, size_t depth, size_t startIdx, std::vector<size_t> &currentSub) -> void {
		if (depth == size - 1) {
			// Find the minimum pairwise distance among all points: array[0] and all elements in currentSub
			double minDist = std::numeric_limits<double>::infinity();

			// Distance from array[0] to each chosen spawn
			for (size_t i = 0; i < size - 1; ++i) {
				double d = areas[array[0]].distance2(areas[currentSub[i]]);
				if (d < minDist) {
					minDist = d;
				}
			}

			// Distance between all pairs within the chosen spawns
			for (size_t i = 0; i < size - 1; ++i) {
				for (size_t j = i + 1; j < size - 1; ++j) {
					double d = areas[currentSub[i]].distance2(areas[currentSub[j]]);
					if (d < minDist) {
						minDist = d;
					}
				}
			}

			// We want to maximize the minimum distance found across the set
			if (minDist > maxDist) {
				maxDist = minDist;
				bestSub = currentSub;
			}
			return;
		}

		for (size_t j = startIdx; j < areas.size(); ++j) {
			if (j == array[0]) continue;
			currentSub[depth] = j;
			self(self, depth + 1, j + 1, currentSub);
		}
	};

	std::vector<size_t> currentSub{};

	currentSub.resize(size - 1);
	findBestCombination(findBestCombination, 0, 0, currentSub);
	// Copy the best combination into the remaining slots of the array
	for (size_t i = 0; i < size - 1; ++i)
		array[i + 1] = bestSub[i];
}

void loadMods()
{
	for (const auto &entry : std::filesystem::directory_iterator("mods"))
		if (entry.is_directory() && std::filesystem::exists(entry.path() / "mod.json"))
			game->modManager.loadMod(entry.path());
}

int main(int argc, char **argv)
{
	if (argc != 0 && strcmp(argv[0], "crash_handler") == 0)
		return crashHandlerMain();
	initCrashHandler();

	Args args;
	int ret = parseArgs(argc, argv, args);

	if (ret != 0)
		return ret;
	TRY {
		new Game("assets/NotoSans-Regular.ttf");
	#ifdef _WIN32
		ShowWindow(game->screen->getNativeHandle(), SW_MAXIMIZE);
	#endif
		loadMods();
		game->screen->setMouseCursorVisible(false);
		game->locale = args.locale;

		std::filesystem::directory_iterator d2kMusics{"mods/dune2000/music_game"};
		std::filesystem::directory_iterator scbwMusics{"mods/starcraft_broodwar/music_game"};
		std::vector<std::filesystem::path> musics;
		musics.insert(musics.end(), begin(d2kMusics), end(d2kMusics));
		musics.insert(musics.end(), begin(scbwMusics), end(scbwMusics));
		sf::Music music{musics[game->random() % musics.size()]};

		std::vector<Faction> allFactions;
		std::vector<std::string> loadedMods;
		std::vector<std::string> loadedFactions;
		std::unordered_map<std::string, Faction> factions;
		std::vector<std::tuple<unsigned, unsigned, HSLColor>> playersParams;
		std::vector<std::string> p = split(args.players, ";");
		unsigned controllingIndex = 0;
		char teamLetter[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

		for (size_t i = 0; i < p.size(); i++) {
			auto &pl = p[i];
			size_t posMod = pl.find(':');
			size_t posColS = pl.find('(');
			size_t posColE = pl.find(')');
			assert_exp(posMod != std::string::npos);
			assert_exp(posColS != std::string::npos);
			assert_exp(posColE != std::string::npos);

			std::string modName = pl.substr(0, posMod);
			std::string factionName = pl.substr(0, posColS);
			auto it = std::ranges::find(loadedFactions, modName);
			unsigned factionIndex;
			std::vector<std::string> colorComponents = split(pl.substr(posColS + 1, posColE - posColS), ",");
			auto color = HSLColor(
				std::stoul(colorComponents[0]),
				std::stoul(colorComponents[1]),
				std::stoul(colorComponents[2])
			);

			assert_exp(colorComponents.size() == 3);
			assert_exp(posColE + 1 <= pl.size());
			if (posColE + 2 != pl.size())
				controllingIndex = i;
			if (std::ranges::find(loadedMods, modName) == loadedMods.end()) {
				game->modManager.get(modName).loadFactions("factions.json", factions);
				loadedMods.push_back(modName);
			}
			if (it == loadedFactions.end()) {
				factionIndex = loadedFactions.size();
				loadedFactions.push_back(factionName);
				assert_msg(factions.contains(factionName), factionName);
				allFactions.push_back(factions.at(factionName));
			} else
				factionIndex = it - loadedFactions.begin();

			char letter = pl[posColE + 1];
			char *teamId = strchr(teamLetter, letter);

			assert_exp(teamId);
			playersParams.emplace_back(teamId - teamLetter, factionIndex, color);
		}

		game->game = std::make_unique<GameManager>(allFactions, 6, GameManager::LobbySettings{true});
		std::vector<size_t> spawn;
		auto &players = game->game->getPlayers();
		auto &map = game->game->getLoadedMap();
		size_t i = 0;
		sf::Clock clock;
		sf::Text text{game->font};
		float timer = 0;

		game->game->loadMap(game->modManager.get("d2k"), args.map);
		genSpawns(playersParams.size(), spawn, map.grid.startAreas);
		for (auto [team, faction, color] : playersParams) {
			game->game->addPlayer(faction, color, spawn[i], team);
			players[i].setEnabled(i == controllingIndex);
			i++;
		}

		if (args.noLazyLoad)
			game->game->preloadAllSprites();
		else if (args.preload)
			game->game->preloadSprites();

		music.play();
		music.setLooping(true);
		reloadViews(*game->screen, game->gui, game->screen->getSize(), {0, -20 * guiScale});
		if (args.loadingOnly)
			game->screen->close();
		while (game->screen->isOpen()) {
			Profiler::get().beginFrame();
			TRY {
				{
					PROFILE_SCOPE("events");
					while (auto event = game->screen->pollEvent()) {
						if (event->is<sf::Event::Closed>())
							game->screen->close();
						else if (auto resized = event->getIf<sf::Event::Resized>())
							reloadViews(*game->screen, game->gui, resized->size, {0, -20 * guiScale});
						game->game->consumeEvent(*event);
						game->game->consumeGeneralEvent(*event);
						{
							PROFILE_SCOPE("gui");
							game->gui.handleEvent(*event);
						}
					}
				}

				timer += args.speed;
				while (timer >= 1) {
					game->game->update();
					timer -= 1;
				}
			} CPPTRACE_CATCH(const std::exception &e) {
				tgui::Panel::Ptr panel;
				auto win = tgui::ChildWindow::create();

				win->setSize(1000, 600);
				win->setPosition("max((&.w - w) / 2, 0)", "max((&.h - h) / 2, 0)");
			#ifdef NO_TRACE
				panel = generateCrashPanel({}, e);
			#else
				if (auto *ce = dynamic_cast<const cpptrace::exception *>(&e))
					panel = generateCrashPanel(ce->trace(), *ce);
				else
					panel = generateCrashPanel(cpptrace::from_current_exception(), e);
			#endif
				panel->setSize("100%", "100%");
				win->add(panel);
				game->game->setPaused();
				game->gui.add(win);
				timer = 0;
			}
			game->game->updateRender();
			game->screen->clear(sf::Color::Black);
			game->game->render();

			{
				PROFILE_SCOPE("gui_draw");
				game->gui.draw();
			}
			game->game->renderAbove();
			dispFPS(clock.restart().asSeconds());
			{
				// Includes waiting for the vertical synchronization
				PROFILE_SCOPE("display");
				game->screen->display();
			}
			game->screen->setMouseCursorGrabbed(!game->paused);

			if (game->game->finished)
				game->screen->close();
			Profiler::get().endFrame();
		}
		return 0;
	} CPPTRACE_CATCH(const std::exception &e) {
	#ifdef NO_TRACE
		handleCrash({}, e);
	#else
		if (auto *ce = dynamic_cast<const cpptrace::exception *>(&e))
			handleCrash(ce->trace(), *ce);
		else
			handleCrash(cpptrace::from_current_exception(), e);
		delete game;
	#endif
	}
}
