//
// Created by PinkySmile on 22/04/2026.
//

#ifndef MIXRTS_MULTISOUND_HPP
#define MIXRTS_MULTISOUND_HPP


#include <vector>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include "Sound.hpp"

class MYDLL_API MultiSound {
private:
	std::vector<Sound> _sounds;

	void addObject(const Mod &base, const nlohmann::json &json);
public:
	void init(const Mod &base, const nlohmann::json &json);
	void play(Vector2f pos) const;
	void play() const;
};


#endif //MIXRTS_MULTISOUND_HPP
