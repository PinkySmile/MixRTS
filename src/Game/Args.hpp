//
// Created by PinkySmile on 10/11/2026.
//

#ifndef MIXRTS_ARGS_HPP
#define MIXRTS_ARGS_HPP


#include <string>
#include <vector>
#include "Utils.hpp"
#include "Resources/SceneArgument.hpp"

struct Args {
	bool loadingOnly = false;
	bool preload = false;
	bool noLazyLoad = false;
	float speed = 1;
	std::string locale = "fr";
	std::string replay;
	std::string map = "d2k:maps/8PLAY1";
	std::vector<PlayerArg> players = {
		{"d2k:atreides",  {}, {}, {214, 375,  919}, 0, true},
		{"d2k:harkonnen", {}, {}, {  0, 800,  750}, 1, false},
		{"d2k:ordos",     {}, {}, {145, 250,  900}, 2, false},
		{"scbw:terran",   {}, {}, {239, 1000, 500}, 3, false},
		{"scbw:zerg",     {}, {}, {299, 1000, 500}, 4, false},
		{"scbw:protoss",  {}, {}, {158, 1000, 500}, 5, false},
	};

	bool parse(int argc, char **argv);
};


#endif //MIXRTS_ARGS_HPP
