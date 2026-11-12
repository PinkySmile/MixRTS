//
// Created by PinkySmile on 12/11/2026.
//

#ifndef MIXRTS_REPLAYINGAME_HPP
#define MIXRTS_REPLAYINGAME_HPP


#include "InGame.hpp"

class ReplayInGame : public InGame {
private:
	ReplayData _replay;

public:
	ReplayInGame(ReplayData replay, std::function<void (const std::string &)> &cb);
	void update() override;

	static ReplayInGame *create(SceneArguments *);
};


#endif //MIXRTS_REPLAYINGAME_HPP
