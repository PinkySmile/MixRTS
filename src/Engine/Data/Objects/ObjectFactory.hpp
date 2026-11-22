//
// Created by PinkySmile on 18/11/2026.
//

#ifndef MIXRTS_OBJECTFACTORY_HPP
#define MIXRTS_OBJECTFACTORY_HPP


#include <unordered_map>
#include <string>
#include <nlohmann/json_fwd.hpp>
#include "forward.hpp"

typedef GameObject *(*ObjectCreationCb)(
	const Faction &base,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
);

template<typename T, typename ...Args, Args... args>
GameObject *ObjectFactory_createBase(
	const Faction &base,
	const std::string &id,
	const std::unordered_map<std::string, std::vector<Animation>> &animList,
	const std::unordered_map<std::string, MultiSound> &sounds,
	const std::unordered_map<std::string, Effect> &effects,
	const std::unordered_map<std::string, Weapon> &weapons,
	const nlohmann::json &json
)
{
	return new T(base, id, animList, sounds, effects, weapons, json, args...);
}

class MYDLL_API ObjectFactory {
private:
	Faction &_faction;
	std::unordered_map<std::string, ObjectCreationCb> _creators;

public:
	ObjectFactory(Faction &faction);

	template<typename T, typename ...Args, Args... args>
	void push(const std::string &type)
	{
		this->push(type, &ObjectFactory_createBase<T, args...>);
	}

	void push(const std::string &type, ObjectCreationCb cb);
	GameObject *create(
		const Faction &base,
		const std::string &id,
		const std::unordered_map<std::string, std::vector<Animation>> &animList,
		const std::unordered_map<std::string, MultiSound> &sounds,
		const std::unordered_map<std::string, Effect> &effects,
		const std::unordered_map<std::string, Weapon> &weapons,
		const nlohmann::json &json
	) const;
};


#endif //MIXRTS_OBJECTFACTORY_HPP
