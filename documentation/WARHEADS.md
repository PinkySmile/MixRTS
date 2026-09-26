# Overview

The `warhead.json` file defines the attributes, damage types, explosion radius, and armor effectiveness multipliers for warheads used by weapons.

The file is composed of a map, assigning a name to a Warhead object:

```jsonc
{
	"warhead.explosive": {
		"radius": 50,
		"dmg_type": "blast",
		"armor": {
			"armor.heavy": 0.5
		}
	}
}
```

# Fields

Every warhead definition must contain the following core properties:

```jsonc
{
	"radius": uint,         // The radius of the warhead's effect.
	"dmg_type": DamageType, // Specifies the damage type category of the warhead.
	"armor": map<float>?    // A dictionary mapping armor identifiers to floating-point damage multiplier values.
}
```

# Types

## DamageType

The damage type's only purpose is to change the death animations of objects dying from this warhead.

* **void**: Void damage type. Units dying to this disappear with no animation.
* **blast**: Explosion-based damage type.
* **bullet**: Ballistic or projectile-based damage type.
* **missile**: Missile-specific damage type.
* **sonic**: Sonic wave-based damage type.
* **fire**: Fire-based damage type.
* **electric**: Electric-based damage.
* **melee**: Close-combat damage type.

## ArmorMultipliers

A dictionary mapping armor identifiers to floating-point damage multiplier values.

```jsonc
{
	"armor.light": 1.0,  // Full damage against light armor.
	"armor.heavy": 0.5   // Reduced damage against heavy armor.
}
```
