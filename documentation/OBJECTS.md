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
	"turn_delay": uint?,                              // Interval delay between directional turns.
	"sight": uint?,                                   // Vision range radius on the map.
	"altitude": int?,                                 // Vertical elevation offset for rendering and collision.
	"limit": uint?,                                   // Maximum instance count allowed per faction/player.
	"selectable": boolean?,                           // Whether the object can be selected by player input.
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
	"deploy": DeployObject?,                          // Structure or unit conversion rules upon deployment.
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
	"direction": float?,                   // Required unit facing direction for deployment.
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
	"dir": uint,                         // Direction facing required for dumping resources.
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
