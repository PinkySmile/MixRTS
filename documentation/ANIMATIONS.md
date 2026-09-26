# Overview

The `animation.json` defines the sprite assets, texture bounds, offset coordinates, blending modes, and color/palette manipulation rules for rendering animations in the game.

The file or object configuration maps properties to configure how an individual animation frame or sprite is loaded and displayed.

# Fields

Every animation definition contains the following core properties:

```jsonc
{
	"sprite": string,           // The filename of the sprite to load, relative to the base path.
	"offset": Vector2f,         // The 2D offset position for rendering the sprite.
	"bounds": RectObject,       // The rectangular bounds within the texture. Using a 0 width and/or height means "the entire image" for that dimension.
	"blending": BlendingMode?,  // The blending mode applied during rendering. (By default "alpha")
	"coloring": ColoringObject? // Configuration for palette or color modifications.
}
```

# Types

## BlendingMode

Determines how the animation colors are blended onto the screen.

* **alpha**: Standard alpha transparency blending.
* **add**: Additive blending.
* **sub**: Subtractive blending.

## ColoringObject

Configures how the sprite's colors or palettes are dynamically adjusted at runtime.

```jsonc
{
	"type": ColoringMethod,  // The coloring method type. (By default "none")
	"indices": uint[]?,      // Array of palette indices to replace. Mandatory only when using type "palette".
	"mask": path?,           // Path to the color mask. Mandatory only when using type "mask".
	"hue_range": pair<uint>? // Range of hues to replace. Mandatory only when using type "hue".
}
```

## ColoringMethod

Determines how the sprite is player-colored.

* **none**: No coloration
* **palette**: Replace a set of colors in the embedded palette of the image.
* **hue**: Replaces colors based on their hue values.
* **mask**: Applies a colored mask on top of the image.
