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
	std::string map;
	std::vector<PlayerArg> players;

	bool parse(int argc, char **argv);
};


#endif //MIXRTS_ARGS_HPP
