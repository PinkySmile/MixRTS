//
// Created by PinkySmile on 11/08/2026.
//

#include <TGUI/Widgets/Label.hpp>
#include "Crash.hpp"
#include "Resources/Game.hpp"
#include "UI/BaseUI.hpp"

#ifdef __linux__
#include <unistd.h>
#include <pwd.h>
#include <unistd.h>

static void acknowledgeCrash()
{
	char ack = 1;

	(void)!write(CRASH_ACK_FD, &ack, 1);
	close(CRASH_ACK_FD);
}
#else
static void acknowledgeCrash()
{
}
#endif

void cleanStackTrace(cpptrace::stacktrace &st)
{
	for (size_t i = 0; i < st.frames.size(); i++) {
		auto &sym = st.frames[i].symbol;

		if (sym == "register_frame_ctor" || sym.starts_with("do_try_catch") || sym.starts_with("try_catch"))
			st.frames.erase(st.frames.begin() + i--);
		else
			simplifySym(sym);
	}
}

std::string stacktraceToRichString(cpptrace::stacktrace st)
{
	std::stringstream ss;

	cleanStackTrace(st);
	st.print(ss, true);

	std::string s = ss.str();
	bool opened = false;
	std::string result;

	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == 27) {
			i++;
			if (s[i] != '[')
				continue;
			i++;
			if (s[i] == '3') {
				i++;
				if (opened)
					result += "</color>";
				result += "<color=#";
				opened = true;
				switch (s[i]) {
					case '1':
						result += "ff0000>";
						break;
					case '2':
						result += "00C000>";
						break;
					case '3':
						result += "C06000>";
						break;
					case '4':
						result += "6060ff>";
						break;
					case '5':
						result += "ff00ff>";
						break;
					case '6':
						result += "00ffff>";
						break;
					case '7':
						result += "ffffff>";
						break;
					case '8':
						result += "000000>";
						break;
					default:
						result += "ffffff>";
						break;
				}
			} else if (s[i] == '0') {
				if (opened)
					result += "</color>";
				opened = false;
			}
			i++;
		} else
			result.push_back(s[i]);
	}
	while (result.back() == '\n' || result.back() == '\r')
		result.pop_back();
	if (opened)
		result += "</color>";
	return result;
}

int crashHandlerMain()
{
	CrashPacket *packet = allocCrashPacket();
	time_t timer;
	char timebuffer[38];
	tm *tm_info;

	time(&timer);
	tm_info = localtime(&timer);
	strftime(timebuffer, sizeof(timebuffer), "crash-%Y-%m-%d-%H-%M-%S", tm_info);

	readCrashPacket(packet);
#ifdef __linux__
	std::filesystem::path home = getenv("HOME") ?: getpwuid(getuid())->pw_dir;
	const char *tmp = getenv("XDG_DATA_HOME");
	std::filesystem::path data = tmp ? std::filesystem::path(tmp) : home / ".local" / "share" / "mixrts";
#else
	std::filesystem::path data = ".";
#endif

	std::error_code err;
	std::filesystem::path path = data / timebuffer;

	std::filesystem::create_directory(path, err);

	try {
		extractCrashReplay(*packet, path);
	} catch (const std::exception &) {}
	try {
		extractCrashState(*packet, path);
	} catch (const std::exception &) {}
	acknowledgeCrash();

	sf::RenderWindow window{sf::VideoMode{sf::Vector2u{1000, 600}}, "Game crashed!"};
	tgui::Gui gui{window};
	tgui::Panel::Ptr panel = crashPacketPanel(*packet);

	std::filesystem::copy("latest.log", path / "log_file.log", err);
	std::ofstream stream{path / "stacktrace.txt"};

	stream << crashPacketToString(*packet);
	stream.close();
	panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

	auto crashPlaceLabel = tgui::Label::create(L"Crash information has been stored at " + std::filesystem::absolute(path).wstring());
	crashPlaceLabel->setPosition({10, "100% - h - 10"});
	crashPlaceLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold | tgui::TextStyle::Underlined);
	panel->setSize({"100%", "bottom.y - 10"});
	gui.add(crashPlaceLabel, "bottom");
	gui.add(panel);
	window.setFramerateLimit(30);
	while (window.isOpen()) {
		while (auto event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>())
				window.close();
			else if (auto resized = event->getIf<sf::Event::Resized>())
				reloadViews(window, gui, resized->size, {0, 0}, 1);
			gui.handleEvent(*event);
		}
		window.clear(sf::Color{0xC0, 0xC0, 0xC0});
		gui.draw();
		window.display();
	}
	return EXIT_SUCCESS;
}