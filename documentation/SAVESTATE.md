# State File Format Specification

## 1. Overview

A `.state` file stores one match exactly as it was between two simulation ticks. Loading it from the title screen rebuilds that match and pauses on the saved tick. The same blob, without the file header, is what rollback and sync checksums use.

A state file consists of:

- A header describing the match that produced the blob (seed, map, players, camera).
- The simulation blob.
- A checksum of that blob.

The camera is not part of the simulation. It is restored only when a file is loaded.

### 1.1. Serialization Conventions

The following conventions apply to the entire file, including the blob:

| Property | Specification |
| --- | --- |
| Endianness | Little-endian |
| Integer representation | Unsigned integers, unless a field is explicitly signed |
| Floating-point representation | IEEE-754 |
| Variable-length integers | Unsigned LEB128 |
| String encoding | UTF-8 |
| String length | Number of bytes, not characters |
| Array length | Number of elements |

All fields are serialized sequentially. There is no padding or alignment between fields. Pointers are never written.

### 1.2. Primitive Types

| Type | Size | Description |
| --- | --- | --- |
| `Uint8` | 1 byte | Unsigned 8-bit integer |
| `Uint16` | 2 bytes | Unsigned 16-bit integer |
| `Uint32` | 4 bytes | Unsigned 32-bit integer |
| `Uint64` | 8 bytes | Unsigned 64-bit integer |
| `Int32` | 4 bytes | Signed 32-bit integer |
| `Float32` | 4 bytes | 32-bit floating-point number |
| `Float64` | 8 bytes | 64-bit floating-point number |
| `Bool` | 1 byte | `0` or `1` |
| `Varint` | Variable | Unsigned LEB128 integer |

### 1.3. Notation

| Notation | Description |
| --- | --- |
| `[N] T` | N consecutive elements of type T |
| `Array<T>` | A `Varint` count followed by that many elements of type T |
| `String` | A `Varint` byte count followed by that many bytes |

---

# 2. File

| Type | Description |
| --- | --- |
| `Uint32` | Magic, `0x45544153` |
| `Uint32` | File version. The only version is `1` |
| `Uint32` | Match seed |
| `String` | Map path, relative to the working directory |
| `Float32` | Game speed |
| `Bool` | Short game |
| `Bool` | Random spawn |
| `Array<Player>` | Players, in slot order |
| `Uint32` | Map checksum |
| `Float32` | Camera top-left X |
| `Float32` | Camera top-left Y |
| `Float32` | Camera zoom |
| `Uint64` | Checksum of the blob (`hash64`, seed 0) |
| `Uint64` | Blob size, in bytes |
| `[Size] Uint8` | Simulation blob |

The checksum is checked before the blob is applied. The map checksum and each faction checksum are checked against the installed files. A mismatch rejects the file.

## 2.1. Player

| Type | Description |
| --- | --- |
| `String` | Faction id |
| `Uint32` | Faction checksum |
| `Float32` | Spawn X, in pixels |
| `Float32` | Spawn Y, in pixels |
| `Uint16` | Color hue |
| `Uint16` | Color saturation |
| `Uint16` | Color lightness |
| `Uint8` | Color alpha |
| `Uint8` | Team |
| `Bool` | Controlled locally when the state was saved |

---

# 3. Simulation blob

The first byte of the blob is the blob version. Version `1` is the only version. Everything after it is the simulation at a tick boundary, after object cleanup.

Occupancy grids are not stored. They are rebuilt from each object's footprint after the blob is applied. `unordered_map` entries are written sorted by key.

References use these ids:

| Reference | Encoding |
| --- | --- |
| Nothing | `Uint32` `0xFFFFFFFF` |
| Live object | `Uint32` `GameObject::index` |
| Player | `Uint32` player slot |
| Map state | `Uint32` index in the match's map-state list. The loaded map's own grid uses the size of that list |
| Template | `Uint32` id from the state registry. `0` is null |

Registry ids are assigned once, when the match is created: faction order, then sorted template name, separately for objects, weapons, effects, sounds, upgrades and queues. They stay valid as long as the faction checksums in the file header match.

## 3.1. Globals

| Type | Description |
| --- | --- |
| `Uint8` | Blob version (`1`) |
| `Uint64` | Tick |
| `Uint32` | Next object index |
| `Bool` | Match finished |
| `Uint32` | Screen shake time |
| `Uint32` | Screen shake power |
| `Uint32` | Screen shake interval |
| `Bool` | Screen shake freezes time |
| `Uint32` | Screen shake timer |
| `Uint32` | RNG seed |
| `Uint32` | RNG invoke count |

The RNG is restored after every object has been created, so copies made during the load do not change the sequence.

## 3.2. Terrain

| Type | Description |
| --- | --- |
| `Array<Uint32>` | Words of the loaded map grid |
| `Varint` | Number of per-faction map states |
| `Array<Uint32>` | Words of each map state, in order |

Only words that differ from the current map are copied. A changed tile is redrawn. Unchanged tiles keep the frame they already show, because the random variant of a tile is chosen outside the simulation RNG.

## 3.3. Objects

Objects are stored in live-object order.

| Type | Description |
| --- | --- |
| `Varint` | Object count |
| `ObjectHeader` | One header per object |
| `ObjectBody` | One body per object, in the same order |

### ObjectHeader

| Type | Description |
| --- | --- |
| `Uint32` | Object index |
| `Uint8` | Kind. `0` is a faction object, `1` is an object spawner |
| `Uint32` | Owner player |
| `Uint32` | Template id. Present only when the kind is `0` |

An object that still exists is overwritten in place. A missing object is copied from its template. An object that is not in the blob is destroyed. A spawner's body then stores the spawned object name (`String`) and its initial orders (`Array<Order>`).

An order is a `Uint8` id followed by its payload. Ids `0` to `19` follow the order variant. `ExecCbOrder` (`16`) is rejected: the simulation uses `FreeQueueOrder` (`19`) instead. A `MoveCheckOrder` failure callback is a `Uint8`: `0` for none, `1` for the resource-dump redirect.

## 3.4. Players

| Type | Description |
| --- | --- |
| `Varint` | Player count. Must match the match |
| `PlayerBody` | One body per player, in slot order |

A player body stores the lost and won flags, the debug unlock flag, radar, power, money, storage, owned elements, object lists, build queues and every production queue. Queue and entry keys must already exist. Values are overwritten in place so the iteration order of those maps does not change. The player's enabled flag and interface state are not stored. The local selection is kept across an in-place restore and dropped when the object no longer exists.

## 3.5. Projectiles

| Type | Description |
| --- | --- |
| `Varint` | Projectile count |
| `Projectile` | One record per projectile |

Each record is the weapon template id, the parent object, then the projectile's mutable fields: target, owner, position, timers, movement and one playback record per animation. The projectile is rebuilt from its weapon without drawing a random number, and the saved fields replace the defaults.

## 3.6. Effects

| Type | Description |
| --- | --- |
| `Uint8` | Layer count. Always `5` |
| `Array<Effect>` | One array per layer |

Each effect is its template id, the player whose color it uses (`0xFFFFFFFF` for none), then its playback: position, speed, timers, frame, repeat count, opacity and layer. The effect is copied from the template and the playback is overwritten. Its sound is not played again.
