# Overview

The `object.json` file defines the attributes, health, build requirements, movement parameters, combat behavior, visual animations, and interactive properties for entities (units and buildings) used in the game.

The file is composed of a map, assigning a name to a GameObject:

```jsonc
{
	"object.tank": {
		"hp": 300,
		"cost": {
			"base:credits": 800
		},
		"size": {
			"x": 32,
			"y": 32
		},
		"armor": "armor.heavy",
		"tags": ["unit", "vehicle"],
		"requirements": [],
		"anims": [{"anim": "anim.tank_base", "layer": 1}]
	}
}

```

# Fields

Every object definition contains the following core properties:

```jsonc
{
	"hp": uint,                                       // The base health points of the object.
	"cost": map<uint>,                                // Resource costs required to build this object.
	"size": Vector2u,                                 // The footprint size of the object in pixels or world units.
	"armor": string,                                  // Identifier of the armor type assigned to this object.
	"tags": string[],                                 // Category tags defining object classifications and capabilities.
	"requirements": RequirementAndObject,             // Prerequisites required to construct or spawn this object.
	"anims": string[],                                // Default animation identifiers associated with this object.
	"speed": float?,                                  // Base movement speed of the object.
	"directions": uint?,                              // Number of rotational directions for rendering (must be divisible by 8).
	"turn_speed": float?,                             // Degrees the object can turn per frame (default: 360, which turns instantly).
	"sight": uint?,                                   // Vision range radius on the map.
	"altitude": int?,                                 // Vertical elevation offset for rendering and collision.
	"limit": uint?,                                   // Maximum instance count allowed per faction/player.
	"selectable": boolean?,                           // Whether the object can be selected by player input.
	"max_energy": uint?,                              // Maximum energy of the object (default: 0). Used by temporary stealths.
	"spawn_energy": uint?,                            // Energy the object has when it spawns (default: 0). Can't exceed max_energy.
	"energy_gain": uint?,                             // Energy gained every energy_gain_delay ticks, up to max_energy (default: 0). Nothing is gained while a temporary stealth is active.
	"energy_gain_delay": uint?,                       // Ticks between two energy gains (default: 0, every tick).
	"auto_attack": boolean?,                          // Whether this object automatically shoots at enemies in range when idle (default: true). Orders given by the player still work.
	"triggers_auto_attack": boolean?,                 // Whether idle enemy objects with a weapon in range automatically start shooting at this object (default: false). Meant for units and turrets.
	"sellable": boolean?,                             // Whether the object can be sold by the player.
	"repairable": boolean?,                           // Whether the object can be repaired when damaged.
	"can_crush": boolean?,                            // Whether this object can crush smaller crushable targets.
	"stackable": boolean?,                            // Whether multiple units of this type can stack on a single grid tile.
	"grid_bound": boolean?,                           // Whether the object's positioning is strictly bound to map tiles.
	"ignore_terrain": boolean?,                       // Whether the object bypasses standard terrain movement restrictions.
	"radar": boolean?,                                // Whether this object provides map radar surveillance.
	"fake": boolean?,                                 // Specifies if this object is a decoy or non-functional structural proxy.
	"proxy": string?,                                 // Target object identifier to act as a proxy for.
	"group": string?,                                 // Strategic group identifier for organizing units.
	"queue": string?,                                 // Identifier of the production queue required to build this object.
	"gives_queue": string?,                           // Production queue identifier unlocked or provided by this object.
	"requires_queue_building": boolean?,              // Requires a queue-providing building to exist for production.
	"requires_placement": boolean?,                   // Requires manual tile placement by the player upon completion.
	"build_speed": uint?,                             // Construction speed multiplier or rate factor.
	"build_max_distance": float?,                     // Maximum distance from existing structures allowed for placement.
	"power": int?,                                    // Power provided (positive) or consumed (negative) by this object.
	"lower_power_when_damaged": boolean?,             // Reduces power generation scaling with structural damage.
	"resource_storage": uint?,                        // Capacity for storing harvested raw resources.
	"storage": map<uint>?,                            // Map of specific stored item capacities or defaults.
	"harvest_time": uint?,                            // Time duration needed to perform a single harvesting tick.
	"harvest_delay": uint?,                           // Delay interval between consecutive harvesting actions.
	"accept_resources": string[]?,                    // Resource types this object can accept as a refinery or dump point.
	"pass_flags": string|uint?,                       // Passability flag bitmask or hex string defining walkable terrain.
	"show_after_build": uint?,                        // Frame threshold after construction before rendering main sprite.
	"idle_time": uint[]?,                             // Two-element array [min, max] defining random delay between idle animations.
	"tiles": uint[][]?,                               // 2D mask matrix defining structural tile solidity and placement masks.
	"skirt": RectObject?,                             // Rectangular tile boundary definition for structure concrete skirts.
	"point": Vector2f?,                               // Primary interaction or dock focal point offset.
	"exit_offset": Vector2f?,                         // Unit exit point offset relative to object location.
	"spawn_offset": Vector2f?,                        // Entity spawn point offset relative to object location.
	"select": RectObject?,                            // Custom selection box dimensions and offset.
	"decel": DecelObject?,                            // Deceleration physics configuration.
	"behavior": BehaviorObject?,                      // Default autonomous behavior order assigned to this object.
	"missile": boolean?,                              // When true, the object is a missile. It climbs until it is above the map, reappears above the position of an attack-position order, falls onto it, then dies and fires its barrels. Missiles are not solid, not selectable, ignore terrain and never attack on their own.
	"shoot_rounds": uint?,                            // Missile only. Number of volleys fired on arrival (default: 1). Each volley fires every barrel that is ready, in a single frame.
	"inaccuracy": float?,                             // Missile only. Each projectile is aimed at the target offset by a random integer in [-inaccuracy, inaccuracy] on both axes (default: 0).
	"deploy": DeployObject?,                          // Structure or unit conversion rules upon deployment.
	"self_destruct": SelfDestructObject?,             // Countdown, then fires a weapon at its own center. Mutually exclusive with "deploy".
	"stealth": StealthObject?,                        // Makes the object hidden from its enemies (stealth or camouflage, permanent or temporary).
	"reveal_stealth": float?,                         // Radius (in tiles) in which this object reveals enemy objects with a stealth of type "stealth" (infantry and turrets in Dune 2000).
	"detector": float?,                               // Radius (in tiles) in which this object reveals enemy objects with a stealth of type "camouflage".
	"sell": SellObject?,                              // Financial return and unit spawning parameters upon selling.
	"dump": DumpObject?,                              // Resource unloading animation and timing attributes.
	"death": DeathObject?,                            // Animation, sound, and particle behaviors triggered on destruction.
	"spawn": ExtraSpawnObject?,                       // Sub-entity spawning parameters during creation.
	"spawn_orders": string[]?,                        // Autonomous orders issued to newly spawned sub-entities.
	"spawn_params": SpawnParamsObject?,               // Spawning parameters pass-through data object.
	"barrels": BarrelObject[]?,                       // Weapon barrel attachments mounted on this object.
	"turrets": TurretObject[]?,                       // Independent rotating turret assemblies mounted on this object.
	"harvest_anim": HarvestAnimObject?,               // Directional harvesting animation configuration.
	"build_anim": ConditionalAnimObject?,             // Animation played while under construction.
	"crush_anim": ConditionalAnimObject?,             // Animation triggered when crushing targets.
	"producing_anim": ConditionalAnimObject?,         // Animation displayed while actively manufacturing items.
	"produced_anim": ConditionalAnimObject?,          // Animation played upon completing production.
	"anim_move": ConditionalAnimObject?,              // Movement animation definition.
	"anim_fire": ConditionalAnimObject?,              // Primary firing animation definition.
	"anim_crouch": ConditionalAnimObject?,            // Crouched state animation definition.
	"anim_crawl": ConditionalAnimObject?,             // Prone crawling movement animation definition.
	"anim_crawl_fire": ConditionalAnimObject?,        // Prone firing animation definition.
	"anim_idles": ConditionalAnimObject[]?,           // List of secondary idle animation variations.
	"muzzle": string?,                                // Particle effect identifier triggered at weapon muzzle on firing.
	"build": string?,                                 // Audio effect played when starting construction. (Must be an existing Sound)
	"crush_sound": string?,                           // Audio effect played when crushing targets. (Must be an existing Sound)
	"death_sound": string?,                           // Audio effect played upon destruction. (Must be an existing Sound)
	"select_sound": string?,                          // Audio effect played when selected. (Must be an existing Sound)
	"do_sound": string?                               // Audio effect played when executing a command. (Must be an existing Sound)
}
```

# Types

## RequirementAndObject

Array of requirements, and'ed together. It's type can be summarized as `(RequirementOrObject|string)[]`.
For each element, if it is a string, then it checks if this object group is owned.
If it is an array, see RequirementOrObject.
If that requirement **is not met**, then it stops and the whole block is considered **to not be met**.

The whole requirement block all considered complete if **all** of its component are met.

## RequirementOrObject

Array of requirements, or'ed together. It's type can be summarized as `(RequirementAndObject|string)[]`.
For each element, if it is a string, then it checks if this object group is owned.
If it is an array, see RequirementAndObject.
If that requirement **is met**, then it stops and the whole block is considered **to be met**.

The whole requirement block is considered complete if **any** of its component are met.

## DecelObject

Defines deceleration physics parameters for smooth stopping behavior.

```jsonc
{
	"dist": float, // Distance threshold where deceleration begins.
	"step": float, // Speed reduction step applied per frame tick.
	"min": float   // Minimum speed cap during deceleration.
}
```

## BehaviorObject

Specifies default autonomous tasks or order routines.

```jsonc
{
	"type": BehaviorType,    // Behavior type identifier.
	"angle": float?,         // Rotational angle parameters (mandatory when type is "spin").
	"for": string[]?,        // Filter list of move command reasons (mandatory when type is "pickup").
	"dist": float?,          // Minimum interaction distance offset for pickup behaviors.
	"extra": BehaviorObject? // Optional nested behavior order sequence.
}
```

## BehaviorType

* **spin**: Spin in place.
* **pickup**: Wait and be ready for pickup orders.

## DeployObject

Defines conversion rules when deploying a unit into a structure.

```jsonc
{
	"direction": float?,                   // Facing required to deploy, in degrees clockwise from up (0 = north).
	"spawn_building": SpawnBuildingObject? // Properties of the building created upon deployment.
}
```

## SpawnBuildingObject

Defines the structure spawned upon unit deployment.

```jsonc
{
	"name": string,    // Target building identifier to construct. (Must be a valid Object)
	"queue": string,   // Production queue type associated with the created structure.
	"offset": Vector2f // Positional offset relative to the deploying unit.
}
```

## SelfDestructObject

Clicking the unit with the deploy cursor finishes its current movement step, then plays the weapon's `effect` on the unit's center for `delay` ticks. The animation starts over whenever it ends before the delay does. During that countdown the unit accepts no orders. When the delay elapses the effect is removed, the weapon fires at the unit's center (hitscan weapons only) and the unit dies.

A command given while the unit is still finishing its step cancels the self destruct.

```jsonc
{
	"delay": uint,    // Ticks the charge effect is shown before the weapon fires.
	"weapon": string, // Weapon fired at the unit's own center. Must be an existing hitscan Weapon.
	"cursor": string? // Cursor shown when hovering the unit to start it (default: "deploy").
}
```

## StealthObject

Makes the object invisible to its enemies, and impossible to target (including by the automatic attacks), until it is revealed.

There are 4 kinds of stealth, depending on `type` and `permanent`:

* **Permanent stealth** (`"stealth"`, permanent): always cloaked. Revealed by nearby enemy objects having `reveal_stealth`.
* **Temporary stealth** (`"stealth"`, not permanent): uses the object's energy (`max_energy`...). It can be activated using the deploy cursor when the object has at least `activation_threshold` energy. While active, `energy_cost.cost` energy is spent every `energy_cost.delay` ticks (and the object doesn't gain any), and the effect ends when there is no energy left. If `cancelable`, using the deploy cursor again ends it before that. Revealed by the same objects.
* **Permanent camouflage** (`"camouflage"`, permanent): always cloaked. Revealed by enemy objects having a `detector` in range.
* **Temporary camouflage** (`"camouflage"`, not permanent): same as temporary stealth but revealed by detectors.

Distances are measured in tiles between the centers of the objects. A half tile of tolerance is added: a radius of 1 covers the 8 tiles around the revealer.
Only operational revealers count (alive, built and powered when they require power).

Revealing works differently depending on the kind:

* Permanent stealths (with a `recloak_delay`): an enemy revealer **coming** in range uncloaks the object for everyone. After `recloak_delay` ticks spent completely visible, it tries to cloak again, even if the revealer is still close. Being revealed again meanwhile does not postpone that moment.
* Firing: a cloaked object first uncloaks completely, and only then shoots. Each shot restarts the `recloak_delay`, so it stays visible for as long as it keeps shooting, and tries to cloak again `recloak_delay` ticks after the last shot.
* Temporary stealths: nearby revealers make the object visible to their team for as long as they are around. When it wants to fire, the effect ends (and the energy is emptied), the object uncloaks completely and then shoots.
* Objects that are hidden from the controlled player don't appear on the minimap.
* If `recloak_delay` is `null`, firing doesn't uncloak anything and permanent stealths stay visible for as long as a revealer is around.

An object that is damaged (health ratio under `damaged_threshold`) may fail to cloak: each attempt succeeds with a probability of `recloak_chance`. When it fails, the object *flickers*: it plays the cloak and uncloak animations over and over, and rolls again after each loop. While flickering, it is not considered cloaked (it can be targeted). It succeeds if repaired over the threshold. This applies to both temporary and permanent stealths. With a chance of 0, a damaged object can never cloak again.

The cloak animation plays the mask forward, the uncloak plays it backward. Both change the sprite while it is in progress.

```jsonc
{
	"type": string?,              // "stealth" (default) or "camouflage".
	"permanent": boolean?,        // Whether the object is always cloaked (default: true).
	"cancelable": boolean?,       // Temporary only: whether the player can end the effect by hand. If not, it lasts until the energy is depleted (default: true).
	"activation_threshold": uint?, // Temporary only: energy the object needs to be able to activate the effect (default: 0).
	"energy_cost": EnergyCostObject?, // Temporary only: energy spent while the effect is active (default: free, which requires "cancelable").
	"recloak_delay": uint|null?,  // Ticks between being revealed and trying to cloak again. null: Firing doesn't uncloak the object (default: null).
	"damaged_threshold": float|null?, // Health ratio (0 to 1) under which cloaking can fail (default: null, never fails).
	"recloak_chance": float?,     // Chance (0 to 1) to succeed to cloak while under the threshold (default: 1).
	"cloak_sound": string?,       // Audio effect played every time the object starts to cloak. (Must be an existing Sound)
	"uncloak_sound": string?,     // Audio effect played every time the object starts to uncloak. (Must be an existing Sound)
	"anim": StealthAnimObject?,   // Dissolve mask: frame 0 keeps the whole sprite, the last one hides it. Without it, there is no transition.
	"ally_display": StealthAllyDisplayObject?, // How the owner and its allies see the object while cloaked (default: shadow, black with 50% opacity).
	"enemy_display": string?      // How the enemies see the object while cloaked: "invisible" (default) or "lens" (not implemented yet, behaves like "invisible").
}
```

## EnergyCostObject

```jsonc
{
	"cost": uint,   // Energy removed every time.
	"delay": uint?  // Ticks between two payments, the first one being this long after the activation (default: 0, every tick).
}
```

## StealthAnimObject

```jsonc
{
	"anim": string,  // Animation containing the frames of the mask. (Must be an existing Animation)
	"delay": uint?   // Ticks added to every frame (default: 0).
}
```

## StealthAllyDisplayObject

```jsonc
{
	"type": string,         // "shadow": the sprite is tinted with "color". "transparent": the sprite is drawn with "opacity".
	"color": ColorObject?,  // Shadow only: Object with the fields "r", "g", "b" and "a" (0 to 255). Default: {"r": 0, "g": 0, "b": 0, "a": 128}
	"opacity": float?       // Transparent only: 0 (invisible) to 1 (opaque). Default: 0.5
}
```

## SellObject

Configures parameters applied when selling a building or unit.

```jsonc
{
	"anim": ConditionalAnimObject,   // Animation sequence played while selling.
	"units": map<uint>,              // Map of unit identifiers and quantities spawned upon sale.
	"refund": map<uint>              // Map of resource types and values refunded upon sale.
}
```

## DumpObject

Configures resource unloading timings and animation sequences.

```jsonc
{
	"dir": float,                        // Facing required to dump, in degrees clockwise from up (0 = north).
	"delay": uint,                       // Processing delay per resource dumping tick.
	"delay_after": uint,                 // Post-dumping delay before returning to operation.
	"during": ConditionalAnimObject,     // Animation displayed continuously during dumping.
	"start_anim": ConditionalAnimObject, // Animation played when resource dumping begins.
	"stop_anim": ConditionalAnimObject   // Animation played when resource dumping concludes.
}
```

## DeathObject

Defines destruction parameters and weapon-type specific death animations.

```jsonc
{
	"generic": DeathAnimObject?,  // Default fallback death animation configuration.
	"bullet": DeathAnimObject?,   // Death animation configuration when killed by bullet damage.
	"missile": DeathAnimObject?,  // Death animation configuration when killed by missile damage.
	"blast": DeathAnimObject?,    // Death animation configuration when killed by blast damage.
	"sonic": DeathAnimObject?,    // Death animation configuration when killed by sonic damage.
	"fire": DeathAnimObject?,     // Death animation configuration when killed by fire damage.
	"electric": DeathAnimObject?, // Death animation configuration when killed by electric damage.
	"melee": DeathAnimObject?     // Death animation configuration when killed by melee damage.
}
```

## DeathAnimObject

Defines animation and particle properties for destruction states.

```jsonc
{
	"anim": ConditionalAnimObject?, // Conditional animation played upon death.
	"effect": string?,              // Visual particle effect identifier spawned on death. (Must be a valid Effect)
	"stayTime": uint?,              // Frame duration the carcass remains visible before fading.
	"fadeTime": uint?,              // Frame duration over which the carcass fades away.
	"count": uint?,                 // Number of visual effects spawned upon destruction.
	"variance": Vector2f?,          // Positional variance range for particle explosion spawning.
	"perTile": boolean?             // Spawns destruction effects individually per occupied map tile.
}
```

## HarvestAnimObject

Defines directional harvesting animation attributes.

```jsonc
{
	"dirs": uint,                 // Number of directional variants provided (must divide into base directions).
	"anim": ConditionalAnimObject // Animation definition used during harvesting.
}
```

## BarrelObject

Configures attached weapon barrels mounted on the object.

```jsonc
{
	"weapons": ObjectWeaponObject[], // List of weapons fitted to this barrel assembly.
	"priority": int?                 // Priority sorting order for weapon selection logic.
}
```

## ObjectWeaponObject

Maps specific weapons to a barrel attachment.

```jsonc
{
	"weapon": string,  // Identifier of the weapon assigned. (Must be an existing Weapon)
	"cooldown": uint,  // Frame cooldown interval between consecutive shots.
	"priority": int?   // Selection priority compared to alternate weapons.
}
```

## TurretObject

Configures rotating turret assemblies mounted on the object.

```jsonc
{
	"fixed": boolean?,             // Prevents independent rotation if set to true.
	"turn_speed": float?,          // Degrees the turret can turn per frame (default: 360, which turns instantly). Divided by 8 while it is not tracking a target.
	"anims": ConditionalAnimObject // Directional animation sets used to render the turret.
}
```

## SpawnParamsObject

How the unit should spawn on screen. See EdgedVector for the x and y parameters.

```jsonc
{
	"type": SpawnType,
	"x": float|Edge,
	"y": float|Edge
}
```

## ExtraSpawnObject

Extra units to spawn along with this object when it is created.

```jsonc
{
	"unit": string,           // Unit to spawn. (Must be a valid Object)
	"pos": EdgedVector,       // Position to spawn to the unit to.
	"invincible": boolean?,   // Is this unit invincible until it despawns.
	"dest": EdgedVector?,     // Destination point. If defined, the unit will spawn at pos, then travel to dest.
	"out": EdgedVector?,      // Despawn point. If defined, this unit will spawn at pos, then travel to dest (if defined), then travel to out, and despawn.
	"carry": ExtraSpawnObject // Object to carry to dest.
}
```
