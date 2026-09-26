# MixRTS

MixRTS is an open source Real Time Strategy (RTS) game written in C++ containing multiple faction from old RTS games
focusing on authenticity. The faction and system mechanics are replicated and added to the engine as needed.
Note that the goal of this project isn't to replace the original games. The factions will be slightly different
because of small engine differences. The goal is simply to bring the games together in the same game with as little
friction of entry as possible.

# Currently supported games/factions

- Dune 2000
   - ▶️ Maps (including a converter)
   - ▶️ Atreides
   - ▶️ Harkonnen
   - ▶️ Ordos
- StarCraft BroodWar
   - ❌ Maps
   - ❌ Terran
   - ❌ Zerg
   - ❌ Protoss
- Command & Conquer Red Alert
   - ❌ Maps
   - ❌ Soviets
   - ❌ Allied
- Supreme Commander 2
   - ❌ Maps
   - ❌ Aeon Illuminate
   - ❌ Cybran Nation
   - ❌ United Earth Federation
- Age of Empire 2
   - ❌ Maps
   - ❌ Armenians
   - ❌ Aztecs
   - ❌ Bengalis
   - ❌ Berbers
   - ❌ Bohemians
   - ❌ Britons
   - ❌ Bulgarians
   - ❌ Burgundians
   - ❌ Burmese
   - ❌ Byzantines
   - ❌ Celts
   - ❌ Chinese
   - ❌ Cumans
   - ❌ Danes
   - ❌ Dravidians
   - ❌ Ethiopians
   - ❌ Franks
   - ❌ Georgians
   - ❌ Goths
   - ❌ Gurjaras
   - ❌ Hindustanis
   - ❌ Huns
   - ❌ Inca
   - ❌ Italians
   - ❌ Japanese
   - ❌ Jurchens
   - ❌ Khitans
   - ❌ Khmer
   - ❌ Koreans
   - ❌ Lithuanians
   - ❌ Magyars
   - ❌ Malay
   - ❌ Malians
   - ❌ Mapuche
   - ❌ Maya
   - ❌ Mongols
   - ❌ Muisca
   - ❌ Persians
   - ❌ Poles
   - ❌ Portuguese
   - ❌ Romans
   - ❌ Saracens
   - ❌ Saxons
   - ❌ Shu
   - ❌ Sicilians
   - ❌ Slavs
   - ❌ Spanish
   - ❌ Tatars
   - ❌ Teutons
   - ❌ Tupi
   - ❌ Turks
   - ❌ Varangians
   - ❌ Vietnamese
   - ❌ Vikings
   - ❌ Wei
   - ❌ Wu
- Warcraft 3
   - ❌ Maps
   - ❌ Orcs
   - ❌ Humans
   - ❌ Night Elves
   - ❌ Undead
- Command & Conquer
   - ❌ Maps
   - ❌ Nod
   - ❌ GDI

# Design philosophy and game balance

The goal of MixRTS is to provide a faithful experience for players coming from X game.
It is made as a way to get friends together on a game even if they have very different RTS game tastes.
Now, because of they all are in the same engine, some compromises has to be made:
1. Some factions have 0 counterplay to some other factions. For example only StarCraft (that I'm aware of) has the concept
   of "detector" units or Starcraft maps always expect air drops to be possible, and some part of the maps aren't accessible without some.
   In these cases, I'd rather add units, or give properties to some existing ones to help them fight back
2. Some small details are hard to replicate, like specifics of path finding or move command or things like that.
   I'd rather add new factions than fixing small details.

Since I want to keep an authentic experience, I, when possible, keep mirrors unaffected.
Because of that, I won't change intra-game balance (as in, balancing between factions from within the same game), but there are few levers
to keep inter-game balance manageable:
- Multiplier on HP and damage
- Armor multipliers

# Game picking

Games planned are in the [Currently supported games/factions section](#Currently supported games/factions).
If your favorite game/faction isn't in the list, feel free to suggest it. If you want to add it yourself, see [Contributing](#Contributing).

Note that I will refuse any game that is still currently being patched.

# Download

You can find the latest release in the Tags tab, or at [this link](https://github.com/PinkySmile/releases/lastest).

# Compilation

## Requirements

- git
- cmake >=4.1
- Some extra requirement depending on the platform, explained bellow

## Repository initialization

These steps are to be run once  on all platforms.
It downloads the repository and all the mods.
The following steps assumes these have been ran.
```sh
git clone https://github.com/PinkySmile/MixRTS
cd MixRTS
git submodule init
git submodule update
```

## On Linux for Linux

### Requirements

- g++ >=13 or clang++ >=21
- SFML >=3.0 (Tested with 3.0.2 and 3.1.0)
- TGUI >=1.9 (Tested with 1.9 and 1.11)

### Preparation (only necessary once)
```sh
cmake -B build-lin-deb -S . -DCMAKE_BUILD_TYPE=Debug -DNO_TEST=ON
cmake -B build-lin-rel -S . -DCMAKE_BUILD_TYPE=Release -DNO_TEST=ON
```

### Build
```sh
cmake --build build-lin-deb --target MixRTS -- -j $(nproc)
cmake --build build-lin-rel --target MixRTS -- -j $(nproc)
```

The first command compiles in debug mode, the second in release mode.
The -j argument defines how many cores to use for the compilation.
In this example, it takes all the cores. Adjust to your needs.

## On Linux for Windows

### Requirements

- MinGW-g++ >=13
- SFML >=3.0 (Tested with 3.0.2 and 3.1.0)
- TGUI >=1.9 (Tested with 1.9 and 1.11)

### Preparation (only necessary once)
```sh
echo '# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# where is the target environment located
set(CMAKE_SYSROOT /usr/x86_64-w64-mingw32/)
set(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32/)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(WIN32 ON)
set(PKG_CONFIG_EXECUTABLE x86_64-w64-mingw32-pkg-config CACHE PATH "pkg-config")' > mingw-cross-toolchain.cmake

cmake -B build-win-deb -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=mingw-cross-toolchain.cmake.cmake -DNO_TEST=ON
cmake -B build-win-rel -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=mingw-cross-toolchain.cmake.cmake -DNO_TEST=ON
```

### Build
```sh
cmake --build build-win-deb --target MixRTS -- -j $(nproc)
cmake --build build-win-rel --target MixRTS -- -j $(nproc)
```

The first command compiles in debug mode, the second in release mode.
The -j argument defines how many cores to use for the compilation.
In this example, it takes all the cores. Adjust to your needs.

## On Windows for Windows

TODO

# Reporting a bug or issue

Before reporting an issue, search the Discord #known-issues to see if it's a known problem to avoid duplicate.
Playing and reporting issues already helps a lot! Report issues in the Discord channel #ask-help or in the github issues tab.
Try to be as detailed as possible and attach screenshots/videos, replays if applicable and any other information you may find relevent.

Keep balancing discussion in #balancing-discussion.

# Contributing

You can contribute anything. Factions/Mods have multiple chunks that can be done individually:
- Art (Sprites and metadata to center them and properly animate them)
- Music (Sound file and metadata such as the name, and the loop points)
- Sound effect
- Game data (Weapon damage, range, unit/building cost, ....)
- Maps (converter script, and converted maps)
- UI

In all cases, it's recommended to see other mods and check how things are done there.
If you need to do something that has already been done in another mod, it helps a lot to see how it's done there, and in
general tinker around with something that already works rather than starting from something that doesn't.

Doing the first 4 don't require coding knowledge.
To add them in, it requires knowing how to properly format a JSON file, and read the error messages from the engine.
See [Common Errors]() to know how to read error messages, and how to fix the most common ones.
See [Test Art]() to see how to test adding art if you don't want to properly add the units.
See [Introduction to Adding Units]() for a tutorial on how to add some units/buildings.
See [Assets Documentation]() for a more in depth documentation of the formats of each file.

To add maps, you'll need to write a converter script. See [Map Format]() for a documentation on the map binary format.

Custom UI for factions is needed to properly integrate a game. It requires installing the TGUI GUI-Builder tool.
UIs are implemented as a Lua script. See [Lua Documentation]() and other.

You can join the discord server, and ask in the #ask-help-dev channel for help if needed.
