//
// Created by PinkySmile on 13/11/2026.
//

#include <algorithm>
#include <unordered_set>
#include "Player.hpp"
#include "StateContext.hpp"
#include "Objects/GameObject.hpp"
#include "Resources/Assert.hpp"

namespace {
	void saveUnsignedMap(StateWriter &out, const std::unordered_map<std::string, unsigned> &map)
	{
		std::vector<std::pair<std::string, unsigned>> entries(map.begin(), map.end());

		std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.first < b.first; });
		out.size(entries.size());
		for (const auto &entry : entries) {
			out.string(entry.first);
			out.u32(entry.second);
		}
	}

	void loadUnsignedMap(StateReader &in, std::unordered_map<std::string, unsigned> &dst)
	{
		size_t count = in.size();
		std::vector<std::pair<std::string, unsigned>> saved;
		std::unordered_set<std::string> keys;

		saved.reserve(static_cast<size_t>(count));
		// Argument evaluation order is unspecified, so the reads cannot share a call.
		for (uint64_t i = 0; i < count; i++) {
			std::string key = in.string();

			saved.emplace_back(std::move(key), in.u32());
		}
		for (const auto &entry : saved)
			keys.insert(entry.first);
		for (auto it = dst.begin(); it != dst.end(); ) {
			if (!keys.contains(it->first))
				it = dst.erase(it);
			else
				++it;
		}
		for (const auto &entry : saved)
			dst[entry.first] = entry.second;
	}

	void saveNestedMap(StateWriter &out, const std::unordered_map<std::string, std::unordered_map<std::string, unsigned>> &map)
	{
		std::vector<std::string> keys;

		keys.reserve(map.size());
		for (const auto &entry : map)
			keys.push_back(entry.first);
		std::ranges::sort(keys);
		out.size(keys.size());
		for (const std::string &key : keys) {
			out.string(key);
			saveUnsignedMap(out, map.at(key));
		}
	}

	void loadNestedMap(StateReader &in, std::unordered_map<std::string, std::unordered_map<std::string, unsigned>> &dst)
	{
		size_t count = in.size();
		std::vector<std::string> keys;

		keys.reserve(static_cast<size_t>(count));
		for (uint64_t i = 0; i < count; i++) {
			std::string key = in.string();

			loadUnsignedMap(in, dst[key]);
			keys.push_back(key);
		}
		for (auto it = dst.begin(); it != dst.end(); ) {
			if (std::ranges::find(keys, it->first) == keys.end())
				it = dst.erase(it);
			else
				++it;
		}
	}

	void saveObjects(StateSaveContext &ctx, const std::list<GameObject *> &objects)
	{
		ctx.out.size(objects.size());
		for (const GameObject *object : objects)
			ctx.obj(object);
	}

	void loadObjects(StateLoadContext &ctx, std::list<GameObject *> &objects)
	{
		uint64_t count = ctx.in.size();

		objects.clear();
		for (uint64_t i = 0; i < count; i++)
			objects.push_back(ctx.obj());
	}
}

void Player::saveState(StateSaveContext &ctx) const
{
	StateWriter &out = ctx.out;
	std::vector<std::string> keys;

	out.boolean(this->_lost);
	out.boolean(this->_won);
	out.boolean(this->debugUnlock);
	out.boolean(this->state.hasRadar);
	out.u32(this->state.consumedPower);
	out.u32(this->state.producedPower);
	saveUnsignedMap(out, this->state.money);
	saveUnsignedMap(out, this->state.systemMoney);
	saveUnsignedMap(out, this->state.totalStorage);
	saveUnsignedMap(out, this->state.elementsOwned);
	saveObjects(ctx, this->state.myObjects);
	saveObjects(ctx, this->state.radars);

	keys.clear();
	for (const auto &entry : this->state.queueBuildings)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	out.size(keys.size());
	for (const std::string &key : keys) {
		out.string(key);
		saveObjects(ctx, this->state.queueBuildings.at(key));
	}

	keys.clear();
	for (const auto &entry : this->state.primaryBuildings)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	out.size(keys.size());
	for (const std::string &key : keys) {
		out.string(key);
		ctx.obj(this->state.primaryBuildings.at(key));
	}

	keys.clear();
	for (const auto &entry : this->state.buildQueues)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	out.size(keys.size());
	for (const std::string &key : keys) {
		const PlayerQueueEntry *entry = this->state.buildQueues.at(key);

		out.string(key);
		out.string(entry && entry->elementId ? *entry->elementId : "");
	}

	keys.clear();
	for (const auto &entry : this->state.queues)
		keys.push_back(entry.first);
	std::ranges::sort(keys);
	out.size(keys.size());
	for (const std::string &key : keys) {
		const PlayerQueue &queue = this->state.queues.at(key);
		std::vector<std::string> entries;

		out.string(key);
		out.u32(queue.providers);
		out.u32(queue.timer);
		out.u32(queue.nb);
		out.boolean(queue.busy);
		out.boolean(queue.checkingMenu);
		saveUnsignedMap(out, queue.stock);
		saveUnsignedMap(out, queue.basket);
		saveNestedMap(out, queue.prices);
		for (const auto &entry : queue.entries)
			entries.push_back(entry.first);
		std::ranges::sort(entries);
		out.size(entries.size());
		for (const std::string &name : entries) {
			const PlayerQueueEntry &entry = queue.entries.at(name);

			out.string(name);
			saveUnsignedMap(out, entry.spent);
			out.f32(entry.buildTime);
			out.f32(entry.buildProgress);
			out.u32(entry.limit);
			out.boolean(entry.unlocked);
			out.boolean(entry.buildable);
			out.boolean(entry.paused);
		}
	}
}

void Player::loadState(StateLoadContext &ctx)
{
	StateReader &in = ctx.in;

	this->_lost = in.boolean();
	this->_won = in.boolean();
	this->debugUnlock = in.boolean();
	this->state.hasRadar = in.boolean();
	this->state.consumedPower = in.u32();
	this->state.producedPower = in.u32();
	loadUnsignedMap(in, this->state.money);
	loadUnsignedMap(in, this->state.systemMoney);
	loadUnsignedMap(in, this->state.totalStorage);
	loadUnsignedMap(in, this->state.elementsOwned);
	loadObjects(ctx, this->state.myObjects);
	loadObjects(ctx, this->state.radars);

	size_t buildings = in.size();
	std::unordered_set<std::string> buildingKeys;

	for (uint64_t i = 0; i < buildings; i++) {
		std::string key = in.string();

		loadObjects(ctx, this->state.queueBuildings[key]);
		buildingKeys.insert(key);
	}
	for (auto it = this->state.queueBuildings.begin(); it != this->state.queueBuildings.end(); ) {
		if (!buildingKeys.contains(it->first))
			it = this->state.queueBuildings.erase(it);
		else
			++it;
	}

	size_t primaries = in.size();
	std::unordered_set<std::string> primaryKeys;

	for (uint64_t i = 0; i < primaries; i++) {
		std::string key = in.string();

		this->state.primaryBuildings[key] = ctx.obj();
		primaryKeys.insert(key);
	}
	for (auto it = this->state.primaryBuildings.begin(); it != this->state.primaryBuildings.end(); ) {
		if (!primaryKeys.contains(it->first))
			it = this->state.primaryBuildings.erase(it);
		else
			++it;
	}

	size_t active = in.size();
	std::unordered_set<std::string> activeKeys;

	for (uint64_t i = 0; i < active; i++) {
		std::string queueId = in.string();
		std::string elementId = in.string();

		if (elementId.empty())
			this->state.buildQueues[queueId] = nullptr;
		else {
			assert_msg(this->state.queues.contains(queueId), queueId);
			PlayerQueue &queue = this->state.queues.at(queueId);

			assert_msg(queue.entries.contains(elementId), elementId);
			this->state.buildQueues[queueId] = &queue.entries.at(elementId);
		}
		activeKeys.insert(queueId);
	}
	for (auto it = this->state.buildQueues.begin(); it != this->state.buildQueues.end(); ) {
		if (!activeKeys.contains(it->first))
			it = this->state.buildQueues.erase(it);
		else
			++it;
	}

	size_t queues = in.size();

	assert_exp(queues == this->state.queues.size());
	for (uint64_t i = 0; i < queues; i++) {
		std::string key = in.string();

		assert_msg(this->state.queues.contains(key), key);

		PlayerQueue &queue = this->state.queues.at(key);

		queue.providers = in.u32();
		queue.timer = in.u32();
		queue.nb = in.u32();
		queue.busy = in.boolean();
		queue.checkingMenu = in.boolean();
		loadUnsignedMap(in, queue.stock);
		loadUnsignedMap(in, queue.basket);
		loadNestedMap(in, queue.prices);

		size_t entries = in.size();

		assert_exp(entries == queue.entries.size());
		for (uint64_t e = 0; e < entries; e++) {
			std::string name = in.string();

			assert_msg(queue.entries.contains(name), name);

			PlayerQueueEntry &entry = queue.entries.at(name);

			loadUnsignedMap(in, entry.spent);
			entry.buildTime = in.f32();
			entry.buildProgress = in.f32();
			entry.limit = in.u32();
			entry.unlocked = in.boolean();
			entry.buildable = in.boolean();
			entry.paused = in.boolean();
		}
	}
}
