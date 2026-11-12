# Replay File Format Specification

## 1. Overview

This document describes the binary structure of replay files, including their metadata, player information, input states, actions, and frame data.

A replay file consists of:

- A header containing replay metadata and game settings.
- A list of participating players.
- A sequence of frames containing player inputs and actions.

### 1.1. Serialization Conventions

The following conventions apply to the entire file format:

| Property | Specification |
| --- | --- |
| Endianness | Little-endian |
| Integer representation | Unsigned integers |
| Floating-point representation | 32-bit IEEE-754 floating-point |
| Variable-length integers | Unsigned LEB128 |
| String encoding | UTF-8 |
| String length | Number of bytes, not characters |
| Array length | Number of elements |

All fields are serialized sequentially, in the order specified by their respective structures (top to bottom).

There is no padding or alignment between fields.

### 1.2. Primitive Types

| Type | Size | Description |
| --- | --- | --- |
| `Uint8` | 1 byte | Unsigned 8-bit integer |
| `Uint16` | 2 bytes | Unsigned 16-bit integer |
| `Uint32` | 4 bytes | Unsigned 32-bit integer |
| `Uint128` | 16 bytes | Unsigned 128-bit integer |
| `Float32` | 4 bytes | 32-bit floating-point number |
| `Varint` | Variable | Unsigned LEB128 integer |

### 1.3. Notation

| Notation | Description |
| --- | --- |
| `[N] T` | N consecutive elements of type T |
| `Array<T>` | Variable-length array of elements of type T |
| `Optional<T>` | A field that may be omitted depending on a condition |

---

# 2. Primitive Structures

## 2.1. Color

Represents a color using the HSL (Hue, Saturation, Lightness) color model, matching the engine's `HSLColor`.

| Type | Description | Range |
| --- | --- | --- |
| `Uint16` | Hue (H) | 0–360 |
| `Uint16` | Saturation (S) | 0–1000 |
| `Uint16` | Lightness (L) | 0–1000 |

## 2.2. String

Represents a variable-length string.

| Type | Description |
| --- | --- |
| `Varint` | Length of the string in bytes |
| `[Length] Uint8` | String content |

The length prefix represents the number of bytes, not the number of Unicode characters.

The current implementation uses UTF-8 encoding. Other encodings are technically possible, provided that the reader and writer agree on the encoding.

## 2.3. Vector2f

Represents a two-dimensional vector using floating-point coordinates.

| Type | Description |
| --- | --- |
| `Float32` | X coordinate |
| `Float32` | Y coordinate |

## 2.4. Vector2fp16

Represents a normalized two-dimensional vector using two 16-bit unsigned integers.

| Type | Description |
| --- | --- |
| `Uint16` | X coordinate |
| `Uint16` | Y coordinate |

Each coordinate is normalized to the range \[0, 1\].

### Encoding

A normalized floating-point value is converted into its 16-bit representation using:

```cpp
float value;
uint16_t encoded = value * 65535;
```

Decoding uses:

```cpp
uint16_t encoded;
float value = encoded / 65535.f;
```

Consequently:

- `0` represents `0.0`.
- `65535` represents `1.0`.

## 2.5. Vector2s

Represents a two-dimensional vector using unsigned 16-bit integer coordinates.

| Type | Description |
| --- | --- |
| `Uint16` | X coordinate |
| `Uint16` | Y coordinate |

Negative coordinates are invalid and cannot be represented.

## 2.6. Array\<T\>

Represents a variable-length array of elements of type `T`.

| Type | Description |
| --- | --- |
| `Varint` | Number of elements (Size) |
| `[Size] T` | Array elements |

The size prefix uses unsigned LEB128 encoding and represents the number of elements, not the number of bytes.

---

# 3. Checksums

## 3.1. CRC32

CRC32 checksums are used to identify and validate game resources, including maps and factions.

The implementation follows the algorithm provided by:

https://github.com/aeldidi/crc32/blob/master/src/crc32.c

CRC32 values are serialized as `Uint32` using little-endian byte ordering.

---

# 4. Player Structures

## 4.1. PlayerInfo

Contains the metadata associated with a player.

| Type | Description |
| --- | --- |
| `String` | Player name |
| `String` | Faction identifier |
| `Uint32` | CRC32 checksum of the faction |
| `Vector2f` | Player spawn position |
| `Color` | Player color |
| `Uint8` | Player team identifier |

## 4.2. PlayerInput

Contains the player's input state for a given frame.

| Type | Description |
| --- | --- |
| `Vector2fp16` | Mouse position, relative to the screen |
| `Uint128` | Bitfield representing pressed keys and additional input state |

### 4.2.1. Input Bitfield

The 128-bit integer uses the following bit assignments:

| Bit(s) | Description |
| --- | --- |
| 0–100 | Keyboard keys, mapped to `sf::Keyboard::Key` |
| 101–105 | Mouse buttons, mapped to `sf::Mouse::Button` |
| 106–125 | Reserved / unused |
| 126 | Indicates whether the camera position field is present |
| 127 | Indicates whether the camera size field is present |

The bitfield is serialized as a little-endian 128-bit unsigned integer.

Bit numbering starts at the least significant bit (bit 0).

### 4.2.2. Conditional Camera Data

The following fields are conditionally serialized according to the corresponding flags in the input bitfield.

| Type | Presence condition | Description |
| --- | --- | --- |
| `Vector2s` | Bit 126 is set | Camera position (top-left corner) |
| `Vector2s` | Bit 127 is set | Camera size, in pixels |

If a flag is not set, its corresponding field is omitted.

---

# 5. Player Actions

Player actions represent commands issued by a player or changes to their game or interface state.

Each action consists of an action identifier followed by its associated payload.

## 5.1. Build Actions

### StartBuildEvent

Indicates that the player started constructing an object.

| Type | Description |
| --- | --- |
| `String` | Identifier of the object being constructed |
| `String` | Identifier of the queue in which construction was started |

### PlaceBuildEvent

Indicates that the player placed a constructed object.

| Type | Description |
| --- | --- |
| `String` | Identifier of the object being placed |
| `String` | Identifier of the queue containing the object |
| `Vector2s` | Placement position |

### CancelBuildEvent

Indicates that the player canceled an ongoing construction.

| Type | Description |
| --- | --- |
| `String` | Identifier of the canceled object |
| `String` | Identifier of the queue containing the object |

## 5.2. Unit Actions

### DeployUnitEvent

Commands an object to deploy.

| Type | Description |
| --- | --- |
| `Uint32` | Index of the object to deploy |

### MoveUnitEvent

Commands one or more units to move to a specified position.

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Indices of all units receiving the order |
| `Vector2s` | Destination position |

### HarvestUnitEvent

Commands one or more units to harvest resources at a specified position.

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Indices of all units receiving the order |
| `Vector2s` | Position at which harvesting should be attempted |

### DumpUnitEvent

Commands one or more units to transfer their resources to a specified object.

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Indices of all units receiving the order |
| `Uint32` | Index of the destination object |

### AttackUnitEvent

Commands one or more units to attack a specified object.

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Indices of all units receiving the order |
| `Uint32` | Index of the target object |

### StopEvent

Commands one or more units to cancel their current orders.

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Indices of all units receiving the order |

## 5.3. Object Management Actions

### SetPrimaryEvent

Promotes an object to become the primary building of its queue.

| Type | Description |
| --- | --- |
| `Uint32` | Index of the object to promote |

### RepairEvent

Indicates that the player started repairing an object.

| Type | Description |
| --- | --- |
| `Uint32` | Index of the object to repair |

### SellEvent

Indicates that the player sold an object.

| Type | Description |
| --- | --- |
| `Uint32` | Index of the object to sell |

## 5.4. Queue Actions

### OpenQueueEvent

Indicates that the player opened a queue menu.

| Type | Description |
| --- | --- |
| `String` | Identifier of the queue |

### CloseQueueEvent

Indicates that the player closed a queue menu.

| Type | Description |
| --- | --- |
| `String` | Identifier of the queue |

## 5.5. Interface and Settings Actions

### ScreenSizeChanged

Indicates that the player's screen resolution changed.

| Type | Description |
| --- | --- |
| `Vector2s` | New screen dimensions |

### SettingsChanged

Indicates that the player modified their interface settings.

| Type | Description |
| --- | --- |
| `Float32` | New UI scale |

### UIChanged

Indicates that the player switched to another interface.

| Type | Description |
| --- | --- |
| `Uint32` | Identifier of the newly displayed UI |

---

# 6. PlayerAction

Represents a single action performed by a player.

The structure begins with an action identifier, followed by the payload associated with that action.

| Type | Description |
| --- | --- |
| `Uint8` | Action identifier |
| Variable | Action-specific payload |

## 6.1. Action Identifiers

The action identifier determines how the remaining data must be interpreted.

| ID | Payload | Description |
| --- | --- | --- |
| 0 | `StartBuildEvent` | The player started constructing an object. |
| 1 | `PlaceBuildEvent` | The player placed an object. |
| 2 | `CancelBuildEvent` | The player canceled a construction. |
| 3 | `DeployUnitEvent` | The player commanded an object to deploy. |
| 4 | `MoveUnitEvent` | The player commanded units to move. |
| 5 | `HarvestUnitEvent` | The player commanded units to harvest resources. |
| 6 | `DumpUnitEvent` | The player commanded units to transfer their resources. |
| 7 | `SetPrimaryEvent` | The player promoted an object to primary status in its queue. |
| 8 | `RepairEvent` | The player initiated repairs on an object. |
| 9 | `SellEvent` | The player sold an object. |
| 10 | `AttackUnitEvent` | The player commanded units to attack an object. |
| 11 | `StopEvent` | The player commanded units to cancel their current orders. |
| 12 | `OpenQueueEvent` | The player opened a queue menu. |
| 13 | `CloseQueueEvent` | The player closed a queue menu. |
| 200 | `ScreenSizeChanged` | The player's screen resolution changed. |
| 201 | `SettingsChanged` | The player modified their settings. |
| 202 | `UIChanged` | The player changed the currently displayed interface. |

---

# 7. Frame Structures

## 7.1. PlayerFrame

Contains the input state and actions of a single player during a frame.

| Type | Description |
| --- | --- |
| `Optional<PlayerInput>` | Player input state |
| `Uint8` | Number of actions (`a_cnt`) |
| `[a_cnt] PlayerAction` | Actions performed during this frame |

### 7.1.1. Conditional Input Data

The `PlayerInput` structure is only present when the **Player Inputs Recorded** flag is enabled in the replay header.

If this flag is disabled, the entire `PlayerInput` structure is omitted.

## 7.2. FrameData

Contains the frame data for all players.

| Type | Description |
| --- | --- |
| `[p_cnt] PlayerFrame` | One `PlayerFrame` for each player |

The number of player frames is determined by `p_cnt`, which is defined in the replay header.

### 7.2.1. Frame Timing

Each `FrameData` represents exactly one game simulation tick.

Consequently, a replay containing `f_cnt` frames represents exactly `f_cnt` simulation ticks.

The actual playback duration can be computed with the formula `f_cnt / (50 * gameSpeed)`. The `gameSpeed` 

---

# 8. Replay File Structure

## 8.1. Replay Header and Data

The replay file begins with metadata describing the replay, followed by the player information and frame sequence.

| Type | Description |
| --- | --- |
| `Uint32` | Magic number (`0x7D4E82AF`) |
| `Uint8` | Replay format version (`0x01`) |
| `Uint32` | Random seed used by the game |
| `String` | Map identifier or name |
| `Uint32` | CRC32 checksum of the map |
| `Float32` | Game speed |
| `Uint8` | Replay flags |
| `Uint8` | Number of players (`p_cnt`) |
| `[p_cnt] PlayerInfo` | Player metadata |
| `Uint32` | Number of frames (`f_cnt`) |
| `[f_cnt] FrameData` | Recorded frame data |

### 8.1.1. Magic Number

The magic number identifies the file format.

Its value is:

text

```text
0x7D4E82AF
```

Since the file uses little-endian byte ordering, its serialized representation is:

text

```text
AF 82 4E 7D
```

## 8.2. Replay Flags

The replay flags field is a single `Uint8` bitfield.

| Bit | Description |
| --- | --- |
| 0 | Short game |
| 1 | Random spawn |
| 2–6 | Reserved / unused |
| 7 | Player inputs recorded |

### Flag Descriptions

- **Short game (Bit 0):** Indicates whether the replay was recorded using the short-game setting.
- **Random spawn (Bit 1):** Indicates whether player spawn positions were randomized.
- **Player inputs recorded (Bit 7):** Indicates whether player input data is included in each `PlayerFrame`.

When bit 7 is set, each `PlayerFrame` contains a `PlayerInput` structure before its action count.

When bit 7 is not set, this structure is omitted.

## 8.3. Overall File Layout

The complete replay file follows this structure:

text

```text
Replay
│
├── Magic (0x7D4E82AF)
├── Format Version (1)
├── Random Seed
├── Map
├── Map CRC32
├── Game Speed
├── Replay Flags
│
├── Player Count (p_cnt)
├── Players
│   └── [p_cnt] PlayerInfo
│
├── Frame Count (f_cnt)
└── Frames
    └── [f_cnt] FrameData
        │
        └── [p_cnt] PlayerFrame
            │
            ├── [Optional] PlayerInput
            │   ├── Mouse Position
            │   ├── Input Bitfield
            │   ├── [Optional] Camera Position
            │   └── [Optional] Camera Size
            │
            ├── Action Count (a_cnt)
            └── [a_cnt] PlayerAction
                │
                ├── Action ID
                └── Action-Specific Payload
```

---

# 9. Implementation Notes

The following rules should be observed when implementing a replay reader or writer.

1. **Endianness:** All multibyte values use little-endian byte ordering, including integers, floating-point values, and the 128-bit input bitfield.
2. **Variable-length integers:** All `Varint` values use unsigned LEB128 encoding. This applies to string lengths and array sizes.
3. **String lengths:** String lengths are expressed in bytes, not Unicode characters. UTF-8 is used by the current implementation.
4. **Array lengths:** Array sizes represent the number of elements, not the total number of bytes occupied by the array.
5. **Conditional fields:** Optional fields must only be read when their corresponding flags indicate their presence.
6. **Action payloads:** The action identifier determines the structure and size of the remaining action data.
7. **Frame organization:** Every `FrameData` contains exactly one `PlayerFrame` per player, in the same order as the players listed in the replay header.
8. **Frame timing:** Every `FrameData` represents exactly one simulation tick.
9. **Object indices:** Every object receives a unique index when it is spawned by the game engine. Actions referencing objects use these indices.
10. **Coordinates:** `Vector2s` coordinates are unsigned and must never be negative.
11. **Normalized coordinates:** `Vector2fp16` values are decoded by dividing their unsigned integer representation by `65535.f`.
12. **Checksums:** Map and faction CRC32 values use the specified CRC32 implementation.
13. **Reserved bits:** Reserved bits should not be interpreted as meaningful data by readers of the current format.
14. **Version compatibility:** The replay format version should be checked before parsing the file, as future versions may introduce changes to the binary layout.
