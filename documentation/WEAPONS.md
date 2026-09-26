# Overview

The `weapon.json` file defines the attributes, damage properties, projectile behaviors, and special effects for weapons used by units and buildings in a mod.

The file is composed of a map, assigning a name to a Weapon object:

```jsonc
{
	"weapon.fire": {
		"type": "projectile",
		...
	},
	"weapon.gun": {
		"type": "hitscan",
		...
	}
}
```

# Fields

Every weapon definition must contain the following core properties:

```jsonc
{
	"type": WeaponType,                 // Specifies the weapon behavior type.
	"damage": uint,                     // The base damage dealt by the weapon.
	"range": uint,                      // The maximum range of the weapon.
	"warhead": string,                  // The identifier of the warhead associated with this weapon.
	"can_hit_tags": string[],           // Defines the category tags that this weapon is allowed to target.
	"air": boolean?,                    // Determines whether the weapon can target flying units.
	"ground": boolean?,                 // Determines whether the weapon can target ground units.
	"effect": string?,                  // Global effect triggered when firing.
	"hit_effect": string?,              // Effect triggered upon hitting a target.
	"sound": string?,                   // Sound effect played when the weapon is used.
	"inaccuracy_base": float?,          // The base inaccuracy value of the weapon.
	"inaccuracy_dist": float?,          // The scaling factor for inaccuracy based on distance.
	"send_forward": boolean?,           // Whether the command is passed forward.
	"take_control": TakeControlObject?, // Configuration for weapons that mind-control or convert targets.
	"projectile": ProjectileObject?     // Detailed configuration used if `type` is set to `"projectile"`.
}
```

# Types

## WeaponType

- **hitscan**: Instantaneous line-of-sight attack.
- **projectile**: Fires a moving projectile object.
- **enter**: An attack triggered upon entering.

## TakeControlObject

Used when a weapon's primary purpose is to temporarily or permanently seize control of enemy units.

```jsonc
{
	"targets": string[],   // The valid target tags or types for control.
	"base_time": uint,     // The base duration of the control effect.
	"time_loss_hit": uint  // Duration lost per hit taken.
}
```

## ProjectileObject

Configures the physics and rendering of traveling projectiles.

```jsonc
{
	"anim": ConditionalAnimObject, // Animation configuration for the projectile.
	"speed": float,                // Movement speed of the projectile.
	"range": uint?,                // Maximum travel distance for the projectile.
	"directions": uint?,           // Number of directional sprites (must be divisible by 8).
	"trail": string?,              // Effect identifier for a particle trail behind the projectile. Must be an existing Effect.
	"trail_interval": uint?,       // Frequency at which trail effects spawn.
	"homing": boolean?,            // Whether the projectile tracks its target.
	"arc": boolean?,               // Whether the projectile follows an arced trajectory. (By default false)
	"shadow": boolean?,            // Whether the projectile renders a ground shadow. (By default false)
	"falling": boolean?,           // Whether gravity pulls the projectile down. (By default false)
	"bounce": Pair<number>?,       // Defines bounce behavior parameters.
	"v": float?,                   // Initial velocity.
	"y": float?,                   // Starting vertical offset.
	"g": float?.                   // Gravity multiplier applied to the projectile.
}
```
