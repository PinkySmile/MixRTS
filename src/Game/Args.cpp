//
// Created by PinkySmile on 10/11/2026.
//

#include "Args.hpp"
#include "Resources/Assert.hpp"

static std::vector<PlayerArg> parsePlayers(const char *players)
{
	std::vector<PlayerArg> playersParams;
	std::vector<std::string> p = split(players, ";");
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

		char letter = pl[posColE + 1];
		char *teamId = strchr(teamLetter, letter);

		assert_exp(teamId);
		// TODO: Allow to select spawn from CLI maybe?
		playersParams.push_back(PlayerArg{factionName, {}, {}, color, *teamId, false});
	}
	if (!playersParams.empty())
		playersParams[controllingIndex].control = true;
	return playersParams;
}

bool Args::parse(int argc, char **argv)
{
	std::map<std::string, char> longToShort = {
		{"load-only", -1},
		{"no-lazy-load", -2},
		{"preload", 'P'},
		{"players", 'p'},
		{"replay", 'r'},
		{"speed", 's'},
		{"map", 'm'},
		{"locale", 'l'}
	};
	std::map<char, std::function<bool (int &i)>> cbs = {
		{-1, [this](int &) {
			this->loadingOnly = true;
			return true;
		}},
		{-2, [this](int &) {
			this->noLazyLoad = true;
			return true;
		}},
		{'P', [this](int &) {
			this->preload = true;
			return true;
		}},
		{'s', [&argc, &argv, this](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --speed\n");
				return false;
			}
			try {
				this->speed = std::stof(argv[i]);
			} catch (...) {
				fprintf(stderr, "Invalid number sent to --speed\n");
				return false;
			}
			return true;
		}},
		{'m', [&argc, &argv, this](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --map\n");
				return false;
			}
			this->map = argv[i];
			return true;
		}},
		{'r', [&argc, &argv, this](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --replay\n");
				return false;
			}
			this->replay = argv[i];
			return true;
		}},
		{'p', [&argc, &argv, this](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --players\n");
				return false;
			}
			this->players = parsePlayers(argv[i]);
			return true;
		}},
		{'l', [&argc, &argv, this](int &i) {
			i++;
			if (i == argc) {
				fprintf(stderr, "Required param to --locale\n");
				return false;
			}
			this->locale = argv[i];
			return true;
		}}
	};

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == '-') {
				auto it = longToShort.find(&argv[i][2]);

				if (it == longToShort.end()) {
					fprintf(stderr, "Unknown format %s\n", &argv[i][2]);
					return false;
				}
				if (!cbs[it->second](i))
					return false;
			} else for (size_t j = 1; argv[i][j]; j++) {
				auto it = cbs.find(argv[i][j]);

				if (it == cbs.end()) {
					fprintf(stderr, "Unknown format %c\n", argv[i][j]);
					return false;
				}
				if (!it->second(i))
					return false;
			}
		}
	}
	return true;
}