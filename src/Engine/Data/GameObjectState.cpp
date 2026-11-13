//
// Created by PinkySmile on 13/11/2026.
//

#include <variant>
#include "GameObject.hpp"
#include "StateContext.hpp"
#include "Player.hpp"
#include "Resources/Assert.hpp"

namespace {
	void saveAnims(StateWriter &out, const std::vector<ConditionalAnim> &anims)
	{
		out.size(anims.size());
		for (const ConditionalAnim &anim : anims)
			anim.saveState(out);
	}

	void loadAnims(StateReader &in, std::vector<ConditionalAnim> &anims)
	{
		size_t count = in.size();

		assert_exp(count == anims.size());
		for (ConditionalAnim &anim : anims)
			anim.loadState(in);
	}

	void saveOptAnim(StateWriter &out, const std::optional<ConditionalAnim> &anim)
	{
		out.boolean(anim.has_value());
		if (anim)
			anim->saveState(out);
	}

	void loadOptAnim(StateReader &in, std::optional<ConditionalAnim> &anim)
	{
		bool present = in.boolean();

		assert_exp(present == anim.has_value());
		if (present)
			anim->loadState(in);
	}

	void saveAnimGrid(StateWriter &out, const std::vector<std::vector<ConditionalAnim>> &anims)
	{
		out.size(anims.size());
		for (const auto &row : anims)
			saveAnims(out, row);
	}

	void loadAnimGrid(StateReader &in, std::vector<std::vector<ConditionalAnim>> &anims)
	{
		size_t count = in.size();

		assert_exp(count == anims.size());
		for (auto &row : anims)
			loadAnims(in, row);
	}

	void saveStringMap(StateWriter &out, const std::unordered_map<std::string, unsigned> &map)
	{
		std::vector<std::pair<std::string, unsigned>> entries(map.begin(), map.end());

		std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.first < b.first; });
		out.size(entries.size());
		for (const auto &entry : entries) {
			out.string(entry.first);
			out.u32(entry.second);
		}
	}

	std::unordered_map<std::string, unsigned> loadStringMap(StateReader &in)
	{
		size_t count = in.size();
		std::unordered_map<std::string, unsigned> map;

		for (uint64_t i = 0; i < count; i++) {
			std::string key = in.string();

			map.emplace(std::move(key), in.u32());
		}
		return map;
	}

	enum OrderId : uint8_t {
		ORDER_SPIN,
		ORDER_TURN,
		ORDER_MOVE,
		ORDER_DEPLOY,
		ORDER_SELF_DESTRUCT,
		ORDER_HARVEST,
		ORDER_HARVEST_CLOSE,
		ORDER_DUMP,
		ORDER_MOVE_CHECK,
		ORDER_ATTACK_UNIT,
		ORDER_ATTACK_POS,
		ORDER_FINISH,
		ORDER_PICKUP,
		ORDER_WAIT_PICKUP,
		ORDER_DIE,
		ORDER_DELIVERY,
		ORDER_EXEC_CB,
		ORDER_KEEP_MOVING,
		ORDER_KEEP_MOVING_DIE,
		ORDER_FREE_QUEUE,
	};

	enum MoveFailId : uint8_t {
		MOVE_FAIL_NONE,
		MOVE_FAIL_CLOSEST_DUMP,
	};
}

uint8_t GameObject::_moveFailId(void (GameObject::*fail)()) const
{
	if (fail == nullptr)
		return MOVE_FAIL_NONE;
	if (fail == &GameObject::_goToClosestResourceDump)
		return MOVE_FAIL_CLOSEST_DUMP;
	assert_not_reached_msg("MoveCheckOrder failure is not serializable");
}

void (GameObject::*GameObject::_moveFail(uint8_t id))()
{
	switch (id) {
	case MOVE_FAIL_NONE:
		return nullptr;
	case MOVE_FAIL_CLOSEST_DUMP:
		return &GameObject::_goToClosestResourceDump;
	default:
		assert_not_reached_msg("Unknown MoveCheckOrder failure id");
	}
}

void GameObject::_saveOrder(StateSaveContext &ctx, const Order &order) const
{
	StateWriter &out = ctx.out;

	std::visit([&](const auto &value) {
		using T = std::decay_t<decltype(value)>;

		if constexpr (std::is_same_v<T, SpinOrder>) {
			out.u8(ORDER_SPIN);
			out.f32(value.angle);
		} else if constexpr (std::is_same_v<T, TurnOrder>) {
			out.u8(ORDER_TURN);
			out.f32(value.angle);
		} else if constexpr (std::is_same_v<T, MoveOrder>) {
			out.u8(ORDER_MOVE);
			out.vec2(value.destination);
			out.u8(static_cast<uint8_t>(value.reason));
			ctx.obj(value.ignore);
			out.boolean(value.force);
		} else if constexpr (std::is_same_v<T, DeployOrder>) {
			out.u8(ORDER_DEPLOY);
		} else if constexpr (std::is_same_v<T, SelfDestructOrder>) {
			out.u8(ORDER_SELF_DESTRUCT);
		} else if constexpr (std::is_same_v<T, HarvestOrder>) {
			out.u8(ORDER_HARVEST);
			out.vec2(value.destination);
			out.u8(static_cast<uint8_t>(value.reason));
			out.boolean(value.force);
		} else if constexpr (std::is_same_v<T, HarvestCloseOrder>) {
			out.u8(ORDER_HARVEST_CLOSE);
		} else if constexpr (std::is_same_v<T, DumpOrder>) {
			out.u8(ORDER_DUMP);
			ctx.obj(value.target);
		} else if constexpr (std::is_same_v<T, MoveCheckOrder>) {
			out.u8(ORDER_MOVE_CHECK);
			out.u8(this->_moveFailId(value.fail));
			out.vec2(value.destination);
		} else if constexpr (std::is_same_v<T, AttackUnitOrder>) {
			out.u8(ORDER_ATTACK_UNIT);
			ctx.obj(value.target);
			out.u32(value.maxShots);
			out.boolean(value.automatic);
			out.boolean(value.force);
		} else if constexpr (std::is_same_v<T, AttackPosOrder>) {
			out.u8(ORDER_ATTACK_POS);
			out.vec2(value.target);
			out.u32(value.maxShots);
			out.boolean(value.automatic);
			out.boolean(value.force);
		} else if constexpr (std::is_same_v<T, FinishMovementOrder>) {
			out.u8(ORDER_FINISH);
		} else if constexpr (std::is_same_v<T, PickupOrder>) {
			out.u8(ORDER_PICKUP);
			ctx.obj(value.target);
			out.vec2(value.destination);
		} else if constexpr (std::is_same_v<T, WaitPickupOrder>) {
			out.u8(ORDER_WAIT_PICKUP);
			out.size(value.targets.size());
			for (MoveReason reason : value.targets)
				out.u8(static_cast<uint8_t>(reason));
			out.f64(value.dist);
			out.boolean(value.other != nullptr);
			if (value.other)
				this->_saveOrder(ctx, *value.other);
		} else if constexpr (std::is_same_v<T, DieOrder>) {
			out.u8(ORDER_DIE);
		} else if constexpr (std::is_same_v<T, DeliveryOrder>) {
			out.u8(ORDER_DELIVERY);
			out.size(value.objects.size());
			for (const std::string &id : value.objects)
				out.string(id);
			out.string(value.queue);
			out.vec2(value.destination.pos);
			ctx.mapState(value.state);
		} else if constexpr (std::is_same_v<T, ExecCbOrder>) {
			out.u8(ORDER_EXEC_CB);
			assert_not_reached_msg("ExecCbOrder cannot be saved");
		} else if constexpr (std::is_same_v<T, KeepMovingOrder>) {
			out.u8(ORDER_KEEP_MOVING);
			out.u32(value.duration);
		} else if constexpr (std::is_same_v<T, KeepMovingAndDieOrder>) {
			out.u8(ORDER_KEEP_MOVING_DIE);
		} else if constexpr (std::is_same_v<T, FreeQueueOrder>) {
			out.u8(ORDER_FREE_QUEUE);
			out.string(value.queue);
		}
	}, order);
}

void GameObject::_loadOrder(StateLoadContext &ctx, Order &order)
{
	StateReader &in = ctx.in;

	switch (in.u8()) {
	case ORDER_SPIN:
		order = SpinOrder{in.f32()};
		break;
	case ORDER_TURN:
		order = TurnOrder{in.f32()};
		break;
	case ORDER_MOVE: {
		MoveOrder value;

		value.destination = in.vec2<int>();
		value.reason = static_cast<MoveReason>(in.u8());
		value.ignore = ctx.obj();
		value.force = in.boolean();
		order = value;
		break;
	}
	case ORDER_DEPLOY:
		order = DeployOrder{};
		break;
	case ORDER_SELF_DESTRUCT:
		order = SelfDestructOrder{};
		break;
	case ORDER_HARVEST: {
		HarvestOrder value;

		value.destination = in.vec2<int>();
		value.reason = static_cast<MoveReason>(in.u8());
		value.force = in.boolean();
		order = value;
		break;
	}
	case ORDER_HARVEST_CLOSE:
		order = HarvestCloseOrder{};
		break;
	case ORDER_DUMP:
		order = DumpOrder{ctx.obj()};
		break;
	case ORDER_MOVE_CHECK: {
		MoveCheckOrder value;

		value.fail = this->_moveFail(in.u8());
		value.destination = in.vec2<int>();
		order = value;
		break;
	}
	case ORDER_ATTACK_UNIT: {
		AttackUnitOrder value;

		value.target = ctx.obj();
		value.maxShots = in.u32();
		value.automatic = in.boolean();
		value.force = in.boolean();
		order = value;
		break;
	}
	case ORDER_ATTACK_POS: {
		AttackPosOrder value;

		value.target = in.vec2<float>();
		value.maxShots = in.u32();
		value.automatic = in.boolean();
		value.force = in.boolean();
		order = value;
		break;
	}
	case ORDER_FINISH:
		order = FinishMovementOrder{};
		break;
	case ORDER_PICKUP: {
		PickupOrder value;

		value.target = ctx.obj();
		value.destination = in.vec2<float>();
		order = value;
		break;
	}
	case ORDER_WAIT_PICKUP: {
		WaitPickupOrder value;
		size_t count = in.size();

		value.targets.reserve(static_cast<size_t>(count));
		for (uint64_t i = 0; i < count; i++)
			value.targets.push_back(static_cast<MoveReason>(in.u8()));
		value.dist = in.f64();
		if (in.boolean()) {
			value.other = std::make_shared<Order>();
			this->_loadOrder(ctx, *value.other);
		}
		order = std::move(value);
		break;
	}
	case ORDER_DIE:
		order = DieOrder{};
		break;
	case ORDER_DELIVERY: {
		DeliveryOrder value;
		size_t count = in.size();

		value.objects.reserve(static_cast<size_t>(count));
		for (uint64_t i = 0; i < count; i++)
			value.objects.push_back(in.string());
		value.queue = in.string();
		value.destination.pos = in.vec2<float>();
		value.state = ctx.mapState();
		order = std::move(value);
		break;
	}
	case ORDER_EXEC_CB:
		assert_not_reached_msg("ExecCbOrder cannot be loaded");
	case ORDER_KEEP_MOVING:
		order = KeepMovingOrder{in.u32()};
		break;
	case ORDER_KEEP_MOVING_DIE:
		order = KeepMovingAndDieOrder{};
		break;
	case ORDER_FREE_QUEUE:
		order = FreeQueueOrder{in.string()};
		break;
	default:
		assert_not_reached_msg("Unknown order id");
	}
}

void GameObject::saveState(StateSaveContext &ctx) const
{
	StateWriter &out = ctx.out;
	uint8_t pickupBind = 0;
	int32_t deathAnim = -1;

	if (this->_pickupTargets) {
		if (!this->_orderQueue.empty()) {
			if (const auto *wait = std::get_if<WaitPickupOrder>(&this->_orderQueue.front()))
				if (this->_pickupTargets == &wait->targets)
					pickupBind = 1;
		}
		if (pickupBind == 0 && this->_idleOrder) {
			if (const auto *wait = std::get_if<WaitPickupOrder>(&*this->_idleOrder))
				if (this->_pickupTargets == &wait->targets)
					pickupBind = 2;
		}
	}
	if (this->_deathAnim) {
		if (this->_crushAnim && this->_deathAnim == &*this->_crushAnim)
			deathAnim = -2;
		else {
			for (size_t i = 0; i < this->_deathAnims.size(); i++)
				if (this->_deathAnim == &this->_deathAnims[i])
					deathAnim = static_cast<int32_t>(i);
			assert_msg(deathAnim >= 0, "Death animation pointer is not inside this object");
		}
	}

	ctx.mapState(this->map);
	out.vec2(this->_position);
	out.vec2(this->_gridPos);
	out.vec2(this->_occupiedTile);
	out.u8(static_cast<uint8_t>(this->_currentTilePosition));
	out.boolean(this->_tileReserved);
	out.boolean(this->_initGrid);
	out.boolean(this->_registered);
	out.u32(this->_hp);
	out.u32(this->_decayTarget);
	out.f32(this->_alpha);
	out.f32(this->animationState.health);
	out.f32(this->_facingAngle);
	out.i32(this->_altitude);
	out.u32(this->_spawnDelay);
	out.u32(this->_energy);
	out.u32(this->_energyGainTimer);
	out.u32(this->_stealthCostTimer);
	out.u32(this->_recloakTimer);
	out.u32(this->_stealthFrame);
	out.u32(this->_stealthFrameTimer);
	out.u8(static_cast<uint8_t>(this->_stealthState));
	out.boolean(this->_stealthActive);
	out.boolean(this->_stealthFlickerUncloak);
	out.boolean(this->_stealthReverse);
	out.boolean(this->_recloakPending);
	out.u32(this->_asideCooldown);
	out.u32(this->_autoAcquireTimer);
	out.u32(this->_decayTimer);
	out.u32(this->_idleTimer);
	out.u32(this->_currentIdle);
	out.u32(this->_repairTimer);
	out.u32(this->_sellTimer);
	out.u32(this->_crouchTimer);
	out.vec2(this->_cachedDest);
	out.f64(this->_cachedDist);
	out.f64(this->_pickupDistMin);
	out.u32(this->_takeControlTime);
	out.u32(this->_timeDecOnAtk);
	ctx.player(this->owner);
	ctx.player(this->_originalOwner);
	ctx.obj(this->_pickup);
	ctx.obj(this->_lastResourceDump);
	out.i32(deathAnim);
	out.u8(pickupBind);
	out.boolean(this->_animFinished);
	out.boolean(this->_built);
	out.boolean(this->_disposable);
	out.boolean(this->_dumpStart);
	out.boolean(this->_dumpStop);
	out.boolean(this->_dumping);
	out.boolean(this->_firstEvent);
	out.boolean(this->_firstIEvent);
	out.boolean(this->_firstREvent);
	out.boolean(this->_harvesting);
	out.boolean(this->_isPrimary);
	out.boolean(this->_moving);
	out.boolean(this->_pathCached);
	out.boolean(this->_producing);
	out.boolean(this->_produced);
	out.boolean(this->_repairing);
	out.boolean(this->_shooting);
	out.boolean(this->_sold);
	out.boolean(this->_solid);
	for (unsigned value : this->_gpTimerU)
		out.u32(value);
	for (int value : this->_gpTimerI)
		out.i32(value);
	for (float value : this->_gpTimerF)
		out.f32(value);
	out.size(this->_pathCache.size());
	for (const Vector2i &point : this->_pathCache)
		out.vec2(point);
	out.span(this->_nearRevealers);
	out.size(this->_revealedToTeams.size());
	for (size_t team : this->_revealedToTeams)
		out.u32(static_cast<uint32_t>(team));
	out.size(this->_lots.size());
	for (const ResourceLot &lot : this->_lots)
		saveStringMap(out, lot.value);
	out.size(this->_orderQueue.size());
	for (const Order &order : this->_orderQueue)
		this->_saveOrder(ctx, order);
	out.boolean(this->_idleOrder.has_value());
	if (this->_idleOrder)
		this->_saveOrder(ctx, *this->_idleOrder);
	for (const auto &side : this->_connections) {
		out.size(side.size());
		for (const GameObject *object : side)
			ctx.obj(object);
	}
	out.size(this->_barrels.size());
	for (const ObjectBarrel &barrel : this->_barrels)
		out.u32(barrel.shootTimer);
	out.size(this->_turrets.size());
	for (const ObjectTurret &turret : this->_turrets) {
		out.f32(turret.angle);
		out.f32(turret.targetAngle);
		out.u32(turret.shotTimer);
		out.u32(turret.recoilTimer);
		out.boolean(turret.fast);
		saveAnims(out, turret.anims);
	}
	saveAnimGrid(out, this->_anims);
	saveAnims(out, this->_moveAnim);
	saveAnims(out, this->_fireAnim);
	saveAnims(out, this->_crouchAnim);
	saveAnims(out, this->_crawlAnim);
	saveAnims(out, this->_crawlFireAnim);
	saveAnims(out, this->_idleAnims);
	saveAnims(out, this->_harvestAnims);
	saveOptAnim(out, this->_sellAnim);
	saveOptAnim(out, this->_repairAnim);
	saveOptAnim(out, this->_buildAnim);
	saveOptAnim(out, this->_producingAnim);
	saveOptAnim(out, this->_producedAnim);
	out.boolean(this->_crushAnim.has_value());
	if (this->_crushAnim)
		saveOptAnim(out, this->_crushAnim->anim);
	out.size(this->_deathAnims.size());
	for (const ObjectDeathParams &death : this->_deathAnims)
		saveOptAnim(out, death.anim);
	out.boolean(this->_dump.has_value());
	if (this->_dump) {
		this->_dump->during->saveState(out);
		this->_dump->start->saveState(out);
		this->_dump->stop->saveState(out);
	}
}

void GameObject::loadState(StateLoadContext &ctx)
{
	StateReader &in = ctx.in;

	this->map = ctx.mapState();
	this->_position = in.vec2<float>();
	this->_gridPos = in.vec2<int>();
	this->_occupiedTile = in.vec2<int>();
	this->_currentTilePosition = static_cast<PFFreeMask>(in.u8());
	this->_tileReserved = in.boolean();
	this->_initGrid = in.boolean();
	this->_registered = in.boolean();
	this->_hp = in.u32();
	this->_decayTarget = in.u32();
	this->_alpha = in.f32();
	this->animationState.health = in.f32();
	this->_facingAngle = in.f32();
	this->_altitude = in.i32();
	this->_spawnDelay = in.u32();
	this->_energy = in.u32();
	this->_energyGainTimer = in.u32();
	this->_stealthCostTimer = in.u32();
	this->_recloakTimer = in.u32();
	this->_stealthFrame = in.u32();
	this->_stealthFrameTimer = in.u32();
	this->_stealthState = static_cast<StealthState>(in.u8());
	this->_stealthActive = in.boolean();
	this->_stealthFlickerUncloak = in.boolean();
	this->_stealthReverse = in.boolean();
	this->_recloakPending = in.boolean();
	this->_asideCooldown = in.u32();
	this->_autoAcquireTimer = in.u32();
	this->_decayTimer = in.u32();
	this->_idleTimer = in.u32();
	this->_currentIdle = in.u32();
	this->_repairTimer = in.u32();
	this->_sellTimer = in.u32();
	this->_crouchTimer = in.u32();
	this->_cachedDest = in.vec2<int>();
	this->_cachedDist = in.f64();
	this->_pickupDistMin = in.f64();
	this->_takeControlTime = in.u32();
	this->_timeDecOnAtk = in.u32();
	this->owner = ctx.player();
	this->_originalOwner = ctx.player();
	this->_pickup = ctx.obj();
	this->_lastResourceDump = ctx.obj();

	int32_t deathAnim = in.i32();

	if (deathAnim == -1)
		this->_deathAnim = nullptr;
	else if (deathAnim == -2) {
		assert_exp(this->_crushAnim);
		this->_deathAnim = &*this->_crushAnim;
	} else {
		assert_msg(deathAnim >= 0 && static_cast<size_t>(deathAnim) < this->_deathAnims.size(), "Death animation index is out of range");
		this->_deathAnim = &this->_deathAnims[deathAnim];
	}

	uint8_t pickupBind = in.u8();

	this->_animFinished = in.boolean();
	this->_built = in.boolean();
	this->_disposable = in.boolean();
	this->_dumpStart = in.boolean();
	this->_dumpStop = in.boolean();
	this->_dumping = in.boolean();
	this->_firstEvent = in.boolean();
	this->_firstIEvent = in.boolean();
	this->_firstREvent = in.boolean();
	this->_harvesting = in.boolean();
	this->_isPrimary = in.boolean();
	this->_moving = in.boolean();
	this->_pathCached = in.boolean();
	this->_producing = in.boolean();
	this->_produced = in.boolean();
	this->_repairing = in.boolean();
	this->_shooting = in.boolean();
	this->_sold = in.boolean();
	this->_solid = in.boolean();
	for (unsigned &value : this->_gpTimerU)
		value = in.u32();
	for (int &value : this->_gpTimerI)
		value = in.i32();
	for (float &value : this->_gpTimerF)
		value = in.f32();

	size_t pathCount = in.size();

	this->_pathCache.resize(static_cast<size_t>(pathCount));
	for (Vector2i &point : this->_pathCache)
		point = in.vec2<int>();
	in.span(this->_nearRevealers);

	size_t teams = in.size();

	this->_revealedToTeams.clear();
	for (uint64_t i = 0; i < teams; i++)
		this->_revealedToTeams.insert(in.u32());

	size_t lots = in.size();

	this->_lots.clear();
	for (uint64_t i = 0; i < lots; i++)
		this->_lots.push_back(ResourceLot{loadStringMap(in)});

	size_t orders = in.size();

	this->_orderQueue.clear();
	for (uint64_t i = 0; i < orders; i++) {
		Order order;

		this->_loadOrder(ctx, order);
		this->_orderQueue.push_back(std::move(order));
	}
	if (in.boolean()) {
		Order order;

		this->_loadOrder(ctx, order);
		this->_idleOrder = std::move(order);
	} else
		this->_idleOrder.reset();
	this->_pickupTargets = nullptr;
	if (pickupBind == 1) {
		assert_exp(!this->_orderQueue.empty());
		this->_pickupTargets = &std::get<WaitPickupOrder>(this->_orderQueue.front()).targets;
	} else if (pickupBind == 2) {
		assert_exp(this->_idleOrder);
		this->_pickupTargets = &std::get<WaitPickupOrder>(*this->_idleOrder).targets;
	} else
		assert_exp(pickupBind == 0);
	for (auto &side : this->_connections) {
		size_t count = in.size();

		side.clear();
		side.reserve(static_cast<size_t>(count));
		for (uint64_t i = 0; i < count; i++)
			side.push_back(ctx.obj());
	}
	this->_recomputeConnectedMask();

	size_t barrels = in.size();

	assert_exp(barrels == this->_barrels.size());
	for (ObjectBarrel &barrel : this->_barrels)
		barrel.shootTimer = in.u32();

	size_t turrets = in.size();

	assert_exp(turrets == this->_turrets.size());
	for (ObjectTurret &turret : this->_turrets) {
		turret.angle = in.f32();
		turret.targetAngle = in.f32();
		turret.shotTimer = in.u32();
		turret.recoilTimer = in.u32();
		turret.fast = in.boolean();
		loadAnims(in, turret.anims);
	}
	loadAnimGrid(in, this->_anims);
	loadAnims(in, this->_moveAnim);
	loadAnims(in, this->_fireAnim);
	loadAnims(in, this->_crouchAnim);
	loadAnims(in, this->_crawlAnim);
	loadAnims(in, this->_crawlFireAnim);
	loadAnims(in, this->_idleAnims);
	loadAnims(in, this->_harvestAnims);
	loadOptAnim(in, this->_sellAnim);
	loadOptAnim(in, this->_repairAnim);
	loadOptAnim(in, this->_buildAnim);
	loadOptAnim(in, this->_producingAnim);
	loadOptAnim(in, this->_producedAnim);
	assert_exp(in.boolean() == this->_crushAnim.has_value());
	if (this->_crushAnim)
		loadOptAnim(in, this->_crushAnim->anim);

	size_t deaths = in.size();

	assert_exp(deaths == this->_deathAnims.size());
	for (ObjectDeathParams &death : this->_deathAnims)
		loadOptAnim(in, death.anim);
	assert_exp(in.boolean() == this->_dump.has_value());
	if (this->_dump) {
		this->_dump->during->loadState(in);
		this->_dump->start->loadState(in);
		this->_dump->stop->loadState(in);
	}
	if (this->owner)
		this->setColor(this->owner->getColor());
}
