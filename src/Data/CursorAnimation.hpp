//
// Created by PinkySmile on 28/03/2026.
//

#ifndef MIXRTS_CURSORANIMATION_HPP
#define MIXRTS_CURSORANIMATION_HPP


#include <sol/forward.hpp>
#include "Cursor.hpp"

class CursorAnimation {
private:
	unsigned _timer = 0;
	unsigned _currentAnimation = 0;
	std::vector<Cursor> _frames;

public:
	CursorAnimation(const std::filesystem::path &base, const nlohmann::json &json, const nlohmann::json &gen);
	void update();
	void render() const;

	static void pushUserType(sol::state &state);
};


#endif //MIXRTS_CURSORANIMATION_HPP
