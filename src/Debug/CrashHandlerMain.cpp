//
// Created by PinkySmile on 11/08/2026.
//

#include <TGUI/Widgets/Label.hpp>
#include "Crash.hpp"
#include "Resources/Game.hpp"

void replace(std::string &str, const std::string &from, const std::string &to) {
	while (true) {
		size_t start_pos = str.find(from);

		if (start_pos == std::string::npos)
			return;
		str.replace(start_pos, from.length(), to);
	}
}

void simplifySym(std::string &sym)
{
	replace(sym, "_AssertionFailedException", "AssertionFailedException");
	replace(sym, "nlohmann::json_abi_v3_12_0::basic_json<std::map, std::vector, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool, long, unsigned long, double, std::allocator, nlohmann::json_abi_v3_12_0::adl_serializer, std::vector<unsigned char, std::allocator<unsigned char> >, void>", "nlohmann::json");
	replace(sym, "nlohmann::json_abi_v3_12_0::basic_json<std::map, std::vector, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, bool, long long, unsigned long long, double, std::allocator, nlohmann::json_abi_v3_12_0::adl_serializer, std::vector<unsigned char, std::allocator<unsigned char> >, void>", "nlohmann::json");
	replace(sym, "nlohmann::json_abi_v3_12_0", "nlohmann");
	replace(sym, "std::filesystem::__cxx11::path", "std::filesystem::path");
	replace(sym, "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >", "std::string");
	replace(sym, "std::__cxx11::basic_string<char>", "std::string");

	size_t pos = 0;
	while (true) {
		pos = sym.find("std::vector<", pos);
		if (pos == std::string::npos)
			break;
		pos += 12;
		auto startPos = pos;

		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;

		auto type = sym.substr(startPos, pos - startPos);
		auto sPos = pos;

		unsigned nb = 0;
		while (pos < sym.size() && (nb != 0 || sym[pos] != '>')) {
			nb -= sym[pos] == '>';
			nb += sym[pos] == '<';
			pos++;
		}
		if (pos >= sym.size())
			break;

		auto target = sym.substr(sPos, pos - sPos);
		if (target == ", std::allocator<" + type + "> ")
			sym.replace(startPos, pos - startPos - 1, type);
	}

	pos = 0;
	while (true) {
		pos = sym.find("std::unordered_map<", pos);
		if (pos == std::string::npos)
			break;
		pos += 19;
		auto startPos = pos;

		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;
		auto type1 = sym.substr(startPos, pos - startPos);
		pos += 2;
		auto sPos = pos;
		while (pos < sym.size() && sym[pos] != ',')
			pos++;
		if (pos >= sym.size())
			break;
		auto type2 = sym.substr(sPos, pos - sPos);

		sPos = pos;
		unsigned nb = 0;
		while (pos < sym.size() && (nb != 0 || sym[pos] != '>')) {
			nb -= sym[pos] == '>';
			nb += sym[pos] == '<';
			pos++;
		}
		if (pos >= sym.size())
			break;

		auto target = sym.substr(sPos, pos - sPos);
		auto t = ", std::hash<" + type1 + " >, std::equal_to<" + type1 + " >, std::allocator<std::pair<" + type1 + " const, " + type2 + "> > ";
		if (target == t)
			sym.replace(startPos, pos - startPos - 1, type1 + ", " + type2);
	}
}

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

static void reloadViews(sf::RenderWindow &win, tgui::Gui &gui, Vector2u size)
{
	sf::View view{sf::FloatRect{
		{0, 0},
		{
			static_cast<float>(size.x),
			static_cast<float>(size.y)
		}
	}};
	tgui::FloatRect viewGui{
		{0, 0},
		{
			static_cast<float>(size.x),
			static_cast<float>(size.y)
		}
	};

	win.setView(view);
	gui.setAbsoluteView(viewGui);
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
	char timebuffer[31];
	tm *tm_info;

	time(&timer);
	tm_info = localtime(&timer);
	strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d-%H-%M-%S", tm_info);

	readCrashPacket(packet);

	sf::RenderWindow window{sf::VideoMode{sf::Vector2u{1000, 600}}, "Game crashed!"};
	tgui::Gui gui{window};
	tgui::Panel::Ptr panel = crashPacketPanel(*packet);
	std::string folder = "crash-";
	std::error_code err;
	folder += timebuffer;
	std::filesystem::path path = folder;

	std::filesystem::create_directory(path, err);
	std::filesystem::copy("latest.log", path / "log_file.log", err);
	std::ofstream stream{path / "stacktrace.txt"};

	stream << crashPacketToString(*packet);
	stream.close();
	panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

	auto crashPlaceLabel = tgui::Label::create(L"Crash information has been stored at " + std::filesystem::absolute(folder).wstring());
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
				reloadViews(window, gui, resized->size);
			gui.handleEvent(*event);
		}
		window.clear(sf::Color{0xC0, 0xC0, 0xC0});
		gui.draw();
		window.display();
	}
	return EXIT_SUCCESS;
}