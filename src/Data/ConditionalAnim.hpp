//
// Created by PinkySmile on 10/01/2026.
//

#ifndef MIXRTS_CONDITIONALANIM_HPP
#define MIXRTS_CONDITIONALANIM_HPP


#include <functional>
#include <nlohmann/json_fwd.hpp>
#include "../forward.hpp"
#include "Color.hpp"
#include "Utils.hpp"

class ConditionalAnim {
private:
	const GameObject *_parent;
	std::vector<Animation> _anim;
	std::function<bool(const ConditionalAnim &This)> _cond;
	unsigned _index = 0;
	unsigned _delay = 0;
	unsigned _totalDelay = 0;
	int _layer = 0;

	void _init(const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j, const std::string &anim);
public:
	ConditionalAnim(const GameObject *parent, const ConditionalAnim &other);
	ConditionalAnim(const Mod &base, const GameObject *parent, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j);
	ConditionalAnim(const Mod &base, const GameObject *parent, const std::unordered_map<std::string, std::vector<Animation>> &animList, const nlohmann::json &j, unsigned index);
	bool operator()() const;
	bool update();
	void reset();
	Animation &getAnim();
	const Animation &getAnim() const;
	const std::vector<Animation> &getAnims() const;
	int getLayer() const;
	size_t size() const;
	unsigned current() const;
	void setColor(const HSLColor &color);
};


#endif //MIXRTS_CONDITIONALANIM_HPP