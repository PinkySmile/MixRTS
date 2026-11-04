//
// Debug tools: dump of the internal values of a GameObject.
//

#include <cstdio>
#include <array>
#include <variant>
#include <type_traits>
#include "GameObject.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Weapon.hpp"
#include "Resources/Game.hpp"

namespace
{
	typedef std::unordered_set<const GameObject *> AliveSet;

	std::string fmt(double value, int decimals = 2)
	{
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
		return buffer;
	}

	template<typename T>
	std::string vec(const Vector2<T> &v)
	{
		if constexpr (std::is_floating_point_v<T>)
			return "(" + fmt(v.x) + ", " + fmt(v.y) + ")";
		else
			return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
	}

	std::string yesNo(bool value)
	{
		return value ? "yes" : "no";
	}

	const char *moveReasonToString(MoveReason reason)
	{
		switch (reason) {
		case MOVEREASON_MOVE_COMMAND:
			return "move_command";
		case MOVEREASON_FORCE_MOVE_COMMAND:
			return "force_move_command";
		case MOVEREASON_ATTACK_COMMAND:
			return "attack_command";
		case MOVEREASON_FORCE_ATTACK_COMMAND:
			return "force_attack_command";
		case MOVEREASON_HARVEST_COMMAND:
			return "harvest_command";
		case MOVEREASON_HARVEST:
			return "harvest";
		case MOVEREASON_DUMP_COMMAND:
			return "dump_command";
		case MOVEREASON_DUMP:
			return "dump";
		case MOVEREASON_AUTO_AGRO:
			return "auto_agro";
		case MOVEREASON_REPAIR:
			return "repair";
		case MOVEREASON_MOVE_ASIDE:
			return "move_aside";
		case MOVEREASON_MOVE_SPAWN:
			return "move_spawn";
		case MOVEREASON_FINISH_MOVE:
			return "finish_move";
		case MOVEREASON_DELIVERY:
			return "delivery";
		default:
			return "?";
		}
	}

	struct OrderPrinter {
		const AliveSet &alive;

		[[nodiscard]] std::string obj(const GameObject *o) const
		{
			if (!o)
				return "none";
			if (!this->alive.contains(o))
				return "<gone>";
			return o->getId() + "#" + std::to_string(o->index);
		}

		std::string operator()(const SpinOrder &o) const
		{
			return "Spin(angle=" + fmt(o.angle) + ")";
		}

		std::string operator()(const TurnOrder &o) const
		{
			return "Turn(angle=" + fmt(o.angle) + ")";
		}

		std::string operator()(const MoveOrder &o) const
		{
			return "Move(to=" + vec(o.destination) + ", reason=" + moveReasonToString(o.reason) + (o.force ? ", force" : "") + ", ignore=" + this->obj(o.ignore) + ")";
		}

		std::string operator()(const DeployOrder &) const
		{
			return "Deploy";
		}

		std::string operator()(const HarvestOrder &o) const
		{
			return "Harvest(tile=" + vec(o.destination) + ", reason=" + moveReasonToString(o.reason) + (o.force ? ", force" : "") + ")";
		}

		std::string operator()(const HarvestCloseOrder &) const
		{
			return "HarvestClose";
		}

		std::string operator()(const DumpOrder &o) const
		{
			return "Dump(target=" + this->obj(o.target) + ")";
		}

		std::string operator()(const MoveCheckOrder &o) const
		{
			return "MoveCheck(dest=" + vec(o.destination) + ")";
		}

		std::string operator()(const AttackUnitOrder &o) const
		{
			return "Attack(target=" + this->obj(o.target) + (o.automatic ? ", auto" : "") + (o.force ? ", force" : "") + ")";
		}

		std::string operator()(const AttackPosOrder &o) const
		{
			return "AttackPos(target=" + vec(o.target) + ")";
		}

		std::string operator()(const FinishMovementOrder &) const
		{
			return "FinishMovement";
		}

		std::string operator()(const PickupOrder &o) const
		{
			return "Pickup(target=" + this->obj(o.target) + ", dest=" + vec(o.destination) + ")";
		}

		std::string operator()(const WaitPickupOrder &o) const
		{
			std::string result = "WaitPickup(dist=" + fmt(o.dist) + ", then=";

			if (o.other)
				result += std::visit(*this, *o.other);
			else
				result += "none";
			return result + ")";
		}

		std::string operator()(const DieOrder &) const
		{
			return "Die";
		}

		std::string operator()(const DeliveryOrder &o) const
		{
			std::string result = "Delivery(queue=" + o.queue + ", objects=[";

			for (size_t i = 0; i < o.objects.size(); i++)
				result += (i ? ", " : "") + o.objects[i];
			return result + "])";
		}

		std::string operator()(const ExecCbOrder &) const
		{
			return "ExecCb";
		}
	};

	template<typename T, size_t N>
	std::string nonZero(const std::array<T, N> &array)
	{
		std::string result;

		for (size_t i = 0; i < N; i++) {
			if (array[i] == T{})
				continue;
			if (!result.empty())
				result += ", ";
			if constexpr (std::is_floating_point_v<T>)
				result += "[" + std::to_string(i) + "]=" + fmt(array[i]);
			else
				result += "[" + std::to_string(i) + "]=" + std::to_string(array[i]);
		}
		return result.empty() ? "all 0" : result;
	}
}

std::string GameObject::getDebugOrderSummary(const AliveSet &alive) const
{
	OrderPrinter printer{alive};

	if (!this->_orderQueue.empty()) {
		std::string result = std::visit(printer, this->_orderQueue.front());

		if (this->_orderQueue.size() > 1)
			result += " (+" + std::to_string(this->_orderQueue.size() - 1) + ")";
		return result;
	}
	if (this->_idleOrder)
		return "idle: " + std::visit(printer, *this->_idleOrder);
	return "idle";
}

void GameObject::getDebugInfo(DebugInfoList &out, const AliveSet &alive) const
{
	OrderPrinter printer{alive};
	auto add = [&out](const std::string &name, const std::string &value) {
		out.emplace_back(name, value);
	};
	auto gridH = this->manager ? this->manager->getLoadedMap().pathFindingGrid.gridSize.y : 0;

	// Identity
	add("id", this->_id);
	add("index", std::to_string(this->index));
	add("owner", this->owner ? std::to_string(this->owner->state.index) : "none");
	if (this->_originalOwner != this->owner)
		add("original owner", this->_originalOwner ? std::to_string(this->_originalOwner->state.index) : "none");
	add("group / armor", this->_group + " / " + this->_armor);
	add("health", std::to_string(this->_hp) + " / " + std::to_string(this->_maxHp));
	{
		std::string tags;

		for (auto &tag : this->_tags)
			tags += (tags.empty() ? "" : ", ") + tag;
		add("tags", tags.empty() ? "none" : tags);
	}

	// Position
	add("position", vec(this->_position));
	add("size", vec(this->_size));
	add("grid pos", vec(this->_gridPos));
	add("occupied tile", vec(this->_occupiedTile) + (this->_tileReserved ? " (reserved)" : ""));
	add("altitude", std::to_string(this->_altitude));
	add("render key", std::to_string(static_cast<int64_t>(this->getBottom()) + static_cast<int64_t>(this->_altitude) * gridH));
	add("hitbox middle", vec(this->getMiddle()));

	// Movement
	add("direction", fmt(this->getFacingAngle() * 180 / M_PI, 1) + " deg (sprite " + std::to_string(this->getDirection()) + " / " + std::to_string(this->getDirections()) + ")");
	add("speed", fmt(this->_speed));
	add("alpha", fmt(this->_alpha));
	add("pass flags", std::to_string(this->_passFlags));
	add("bound to grid", yesNo(this->_boundToGrid));
	add("path", this->_pathCached ? std::to_string(this->_pathCache.size()) + " steps, dest " + vec(this->_cachedDest) : "none");

	// Flags
	{
		std::string flags;
		auto flag = [&flags](bool value, const char *name) {
			if (value)
				flags += (flags.empty() ? "" : ", ") + std::string(name);
		};

		flag(this->_moving, "moving");
		flag(this->_shooting, "shooting");
		flag(this->_harvesting, "harvesting");
		flag(this->_dumping, "dumping");
		flag(this->_dumpStart, "dumpStart");
		flag(this->_dumpStop, "dumpStop");
		flag(this->_repairing, "repairing");
		flag(this->_built, "built");
		flag(this->_sold, "sold");
		flag(this->_producing, "producing");
		flag(this->_produced, "produced");
		flag(this->_isPrimary, "primary");
		flag(this->_solid, "solid");
		flag(this->_stackable, "stackable");
		flag(this->_wall, "wall");
		flag(this->_fake, "fake");
		flag(this->_central, "central");
		flag(this->_canCrush, "canCrush");
		flag(this->_selectable, "selectable");
		flag(this->_registered, "registered");
		flag(this->_animFinished, "animFinished");
		flag(this->_disposable, "disposable");
		add("flags", flags.empty() ? "none" : flags);
	}

	// Timers
	add("gpTimerU", nonZero(this->_gpTimerU));
	add("gpTimerI", nonZero(this->_gpTimerI));
	add("gpTimerF", nonZero(this->_gpTimerF));
	add("idle timer", std::to_string(this->_idleTimer) + " (idle anim " + std::to_string(this->_currentIdle) + ")");
	add("repair timer", std::to_string(this->_repairTimer) + " / " + std::to_string(this->_repairDelay));
	add("sell timer", std::to_string(this->_sellTimer));
	add("crouch timer", std::to_string(this->_crouchTimer));
	add("harvest timer", std::to_string(this->_harvestTime) + " / " + std::to_string(this->_harvestDelay));
	add("decay timer", std::to_string(this->_decayTimer) + " (target " + std::to_string(this->_decayTarget) + ")");
	add("turn speed", fmt(this->_turnSpeed) + " deg/frame");
	add("aside cooldown", std::to_string(this->_asideCooldown));
	if (this->_takeControlTime)
		add("take control", std::to_string(this->_takeControlTime));

	// State
	add("resources", std::to_string(this->_resourceStorage));
	{
		std::string storage;

		for (auto &[key, value] : this->_storage)
			storage += (storage.empty() ? "" : ", ") + key + "=" + std::to_string(value);
		add("storage", storage.empty() ? "empty" : storage);
	}
	add("resource lots", std::to_string(this->_lots.size()));
	add("pickup", this->_pickup && alive.contains(this->_pickup) ? this->_pickup->getId() + "#" + std::to_string(this->_pickup->index) : "none");
	add("last dump", this->_lastResourceDump && alive.contains(this->_lastResourceDump) ? this->_lastResourceDump->getId() + "#" + std::to_string(this->_lastResourceDump->index) : "none");
	for (size_t i = 0; i < this->_barrels.size(); i++) {
		auto &barrel = this->_barrels[i];
		std::string cooldowns;

		for (auto &weapon : barrel.weapons)
			cooldowns += (cooldowns.empty() ? "" : ", ") + std::to_string(weapon.cooldown);
		add("barrel " + std::to_string(i), "timer " + std::to_string(barrel.shootTimer) + ", cooldowns [" + cooldowns + "]");
	}
	for (size_t i = 0; i < this->_turrets.size(); i++) {
		auto &turret = this->_turrets[i];

		add(
			"turret " + std::to_string(i),
			"angle " + fmt(turret.angle * 180 / M_PI, 1) + " -> " + fmt(turret.targetAngle * 180 / M_PI, 1) +
			" deg, shot " + std::to_string(turret.shotTimer) +
			", speed " + fmt(turret.turnSpeed) +
			", recoil " + std::to_string(turret.recoilTimer) + "/" + std::to_string(turret.recoilMax)
		);
	}

	// Orders
	add("orders", std::to_string(this->_orderQueue.size()));
	for (size_t i = 0; i < this->_orderQueue.size(); i++)
		add(i == 0 ? "  current" : "  #" + std::to_string(i), std::visit(printer, this->_orderQueue[i]));
	add("idle order", this->_idleOrder ? std::visit(printer, *this->_idleOrder) : "none");
}
