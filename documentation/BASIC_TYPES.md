# boolean

A 2-state value. `true` or `false`.

# string

Characters between double quotation marks.

```jsonc
"Super string"
```

# int

Integer between -2147483648 and 2147483647 included.

# uint

Integer between 0 and 4294967295 included.

# float

A real number.

# double

A real number.

# pair<type>

A pair of 2 values of `type`.

```jsonc
[0, 1]
```

# type[]

Array of any number of `type`.

```jsonc
[0, 2, 3, 4, 89, 10, 0]
```

# map\<type\>

A generic map, from a string to `type`.

```jsonc
{
	"key1": type,
	"key2": type,
	"key3": type,
	"key4": type,
}
```

For example, a `map<uint>` would have all its values as `uint` values.

## type?

Defines an optional field. If the field is defined, it must be a `type`.

## type1|type2

Either `type1` or `type2`.

# ConditionalAnimObject

Every conditional animation definition supports the following core properties:The `ConditionalAnimObject` (or conditional animation object) defines an animation sequence that updates dynamically based on specific runtime conditions such as health thresholds or storage capacity values.

```jsonc
{
	"anim": string,                   // The identifier of the base animation to play. (Must be a valid Animation)
	"layer": int,                     // The rendering layer order for the animation.
	"delay": uint?,                   // Frame delay interval between updates.
	"when_hp_lower": float?,          // Condition triggered when the parent's health percentage is lower than this value.
	"when_hp_bigger": float?,         // Condition triggered when the parent's health percentage is greater than or equal to this value.
	"when_storage_bigger": float?,    // Condition triggered when storage percentage is greater than or equal to this value.
	"when_storage_is_in": pair<uint>? // Condition triggered when storage percentage falls within a specified range.
}
```

# Vector2X

Type for 2D vectors. The letter after defines the inner type.

```jsonc
{
	"x": type,
	"y": type,
}
```

Types:
* Vector2**i**: int
* Vector2**u**: uint
* Vector2**d**: double
* Vector2**f**: float

## RectObject

Defines rectangular coordinates.

```jsonc
{
	"x": uint, // X-coordinate of the rectangle's top-left corner.
	"y": uint, // Y-coordinate of the rectangle's top-left corner.
	"w": uint, // Width of the rectangle.
	"h": uint  // Height of the rectangle.
}
```

# EdgedVector

A special Vector2f that allows to select edges of the map.

```jsonc
{
	"x": float|Edge,
	"y": float|Edge,
}
```

If it is a float, then it is interpreted as an offset to the relevant point (for example, for the unit spawn point, it is an offset to the exit of the factory).
If it is a string, then it maps to the Edge enum and take the appropriate value.

# Edge

* **close_border**: The closest border for that axis.
* **far_border**: The furthest border for that axis.
* **min_border**: The lowest border for that axis. For x, left. For y, top.
* **max_border**: The height border for that axis. For x, right. For y, bottom.
