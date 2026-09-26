# Overview

The `effect.json` file defines the visual animations, rendering layers, movement speed, opacity, and associated sound effects for visual effects used in the game.

The file is composed of a map, assigning a name to an Effect object:

```jsonc
{
	"effect.explosion": {
		"anim": {
			"anim": "anim.explosion",
			"layer": 5
		},
		"sound": "sound.explosion"
	}
}
```

# Fields

Every effect definition must contain the following core properties:

```jsonc
{
	"anim": EffectAnimObject, // Animation configuration for the effect.
	"opacity": float?,        // The transparency level of the effect (defaults to 1).
	"sound": string?,         // Identifier of the sound effect played by the effect.
	"speed": Vector2f?        // Movement speed vector for the effect.
}
```

# Types

## EffectAnimObject

Defines the animation sequence and rendering properties for the effect.

```jsonc
{
	"anim": string,  // The identifier of the animation to play. Must be an existing Animation.
	"layer": int,    // The rendering layer order for the effect.
	"delay": uint?   // Frame delay interval between updates.
}
```
