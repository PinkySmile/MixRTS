cmake_minimum_required(VERSION 4.1)
project(MixRTS)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(_USE_MATH_DEFINES)

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
	set(BUILD_STATIC ON)
	set(WEB_BUILD ON)
	# Dirty hack for emscripten
	set(SFML_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/SFML")
	set(FLAC_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/FLAC")
	set(Freetype_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/freetype")
	set(Ogg_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Ogg")
	set(Vorbis_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Vorbis")
	set(TGUI_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/TGUI")

	# Suppress warning in json library
	set(SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_DEBUG -g3 -gsource-map -Og)
	set(SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_RELEASE -O3)
	set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_DEBUG -g3 -gsource-map -sASSERTIONS=2 -sCHECK_NULL_WRITES=1 -sSTACK_OVERFLOW_CHECK=1 -sWEBSOCKET_DEBUG) # -sSAFE_HEAP=1
	set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_RELEASE -O3)
	set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_ASYNCIFY -sASYNCIFY=1) #-sASYNCIFY_IGNORE_INDIRECT=1)

	if (CMAKE_BUILD_TYPE STREQUAL Debug)
		set(SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS ${SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_DEBUG})
		set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS ${SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_DEBUG})
	else()
		set(SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS ${SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS_RELEASE})
		set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS ${SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_RELEASE})
	endif()

	set(SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS
		${SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS}
		#-pthread
		--use-port=freetype
		-fwasm-exceptions
	)
	set(SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS
		${SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS}

		${SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS_ASYNCIFY}

		#-pthread                            # Enable threading support

		-fwasm-exceptions
		-Wno-limited-postlink-optimizations # warning: running limited binaryen optimizations because DWARF info requested (or indirectly required)
		-Wno-pthreads-mem-growth            # warning: -pthread + ALLOW_MEMORY_GROWTH may run non-wasm code slowly, see https://github.com/WebAssembly/design/issues/1271

		-sNO_EXIT_RUNTIME=1
		-sEXPORTED_RUNTIME_METHODS=HEAPF32
		-sALLOW_MEMORY_GROWTH=1             # Grow the memory arrays at runtime
		-sFETCH=1                           # Enables `emscripten_fetch` API
		-sFORCE_FILESYSTEM=1                # Makes full filesystem support be included
		-sFULL_ES2=1                        # Forces support for all GLES2 features, not just the WebGL-friendly subset
		-sMAX_WEBGL_VERSION=1               # Specifies the highest WebGL version to target
		-sMIN_WEBGL_VERSION=1               # Specifies the lowest WebGL version to target
		-sSTACK_SIZE=8mb                    # Set the total stack size
		#-sUSE_PTHREADS=1                    # Enable threading support
		-sWASM=1                            # Compile code to WebAssembly

		-sGL_EXPLICIT_UNIFORM_LOCATION=1  # TODO P0:
		-sGL_EXPLICIT_UNIFORM_BINDING=1   # TODO P0:

		--bind
		--embed-file ${CMAKE_SOURCE_DIR}/mods/@/mods
		-sTOTAL_MEMORY=512mb

		--shell-file=${CMAKE_SOURCE_DIR}/shell.html
	)

	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	add_compile_options(${SOFGV_EMSCRIPTEN_TARGET_COMPILE_OPTIONS})
	add_link_options(${SOFGV_EMSCRIPTEN_TARGET_LINK_OPTIONS})
else ()
	set(WEB_BUILD OFF)
endif ()
if (BUILD_STATIC)
	set(TGUI_SHARED_LIBS OFF)
	set(SFML_STATIC_LIBRARIES ON)
else()
	set(TGUI_SHARED_LIBS ON)
	set(SFML_STATIC_LIBRARIES OFF)
endif()
find_package(TGUI)
find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Network)
include_directories(thirdparty src/Engine src/Engine/SFML)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-W -Wall -Wextra)
	endif ()
	add_compile_definitions(_DEBUG=1)
	option(DEBUG_CHECKS "Debug checks" ON)
else ()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if (WIN32)
			add_link_options(-mwindows)
		endif()
	endif ()
	add_compile_definitions(NDEBUG=1)
	option(DEBUG_CHECKS "Debug checks" OFF)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT NO_SHORTEN_PATHS)
	add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=/MixRTS")
	add_compile_options("-fmacro-prefix-map=${CMAKE_BINARY_DIR}/_deps/=/")
	add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=/MixRTS")
	add_compile_options("-ffile-prefix-map=${CMAKE_BINARY_DIR}/_deps/=/")
	add_compile_options("-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=/MixRTS")
	add_compile_options("-fdebug-prefix-map=${CMAKE_BINARY_DIR}/_deps/=/")
	add_link_options("-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=/MixRTS")
	add_link_options("-fdebug-prefix-map=${CMAKE_BINARY_DIR}/_deps/=/")
endif ()
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
if (ASAN)
	add_compile_options(-fsanitize=address)
	add_compile_options(-fsanitize=undefined)
	add_compile_options(-fsanitize=float-divide-by-zero)
	add_compile_options(-fsanitize-address-use-after-scope)
	add_link_options(-fsanitize=address)
	add_link_options(-fsanitize=undefined)
	add_link_options(-fsanitize=float-divide-by-zero)
	add_link_options(-fsanitize-address-use-after-scope)
endif ()
if (TSAN)
	add_compile_options(-fsanitize=thread)
	add_compile_options(-fsanitize=undefined)
	add_compile_options(-fsanitize=float-divide-by-zero)
	add_compile_options(-fsanitize-address-use-after-scope)
	add_link_options(-fsanitize=thread)
	add_link_options(-fsanitize=undefined)
	add_link_options(-fsanitize=float-divide-by-zero)
	add_link_options(-fsanitize-address-use-after-scope)
endif ()

# When set, GoogleTest is not downloaded/built and the MixRTS-tests target is not
# built at all.
option(NO_TEST "Disable fetching GoogleTest and building the test target" OFF)
option(NO_TRACE "Disable fetching cpptrace" OFF)
option(NO_PROFILER "Compile out the scoped profiler" OFF)
if (NO_PROFILER)
	add_compile_definitions(NO_PROFILER)
endif ()

if (NOT NO_TRACE)
	include(FetchContent)
	FetchContent_Declare(
		cpptrace
		GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
		GIT_TAG        v1.0.4
	)
	set(CPPTRACE_DISABLE_CXX_20_MODULES ON)
	if (WIN32)
		set(CPPTRACE_GET_SYMBOLS_WITH_LIBBACKTRACE ON CACHE BOOL "" FORCE)
	endif ()
	set(CPPTRACE_UNWIND_WITH_LIBUNWIND ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(cpptrace)
	set(EXTRA_LIBS cpptrace::cpptrace)
else (NOT NO_TRACE)
	add_compile_definitions(NO_TRACE)
	set(EXTRA_SOURCES src/Game/Debug/CrashNone.cpp)
endif (NOT NO_TRACE)

find_package(Lua 5.1 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
add_compile_definitions(LUA_VERSION_STRING="${LUA_VERSION_STRING}")
add_subdirectory(sol2)

if (NOT NO_TEST)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG        v1.15.2
	)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
	enable_testing()
endif ()


if (WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} ntdll dbghelp)
	if (NOT NO_TRACE)
		set(EXTRA_SOURCES src/Game/Debug/CrashWindows.cpp src/Game/Debug/CrashHandlerMain.cpp)
	endif (NOT NO_TRACE)
else ()
	if (NOT NO_TRACE)
		set(EXTRA_SOURCES src/Game/Debug/CrashUnix.cpp src/Game/Debug/CrashHandlerMain.cpp)
	endif (NOT NO_TRACE)
endif()

set(MIXRTS_CORE_SOURCES
	src/Engine/Data/Animation.cpp
	src/Engine/Data/Animation.hpp
	src/Engine/Data/ConditionalAnim.cpp
	src/Engine/Data/ConditionalAnim.hpp
	src/Engine/Data/CursorAnimation.cpp
	src/Engine/Data/CursorAnimation.hpp
	src/Engine/Data/Cursor.cpp
	src/Engine/Data/Cursor.hpp
	src/Engine/Data/Effect.cpp
	src/Engine/Data/Effect.hpp
	src/Engine/Data/Extra.cpp
	src/Engine/Data/Extra.hpp
	src/Engine/Data/Faction.cpp
	src/Engine/Data/Faction.hpp
	src/Engine/Data/FindPathAStar.cpp
	src/Engine/Data/FindPathCrashAndTurn.cpp
	src/Engine/Data/FindPath.hpp
	src/Engine/Data/GameManager.cpp
	src/Engine/Data/GameManager.hpp
	src/Engine/Data/GameManagerState.cpp
	src/Engine/Data/GameObjectState.cpp
	src/Engine/Data/PlayerStateSave.cpp
	src/Engine/Data/StateRegistry.cpp
	src/Engine/Data/StateRegistry.hpp
	src/Engine/Data/StateStream.hpp
	src/Engine/Data/StateContext.hpp
	src/Engine/Data/GameObject.cpp
	src/Engine/Data/GameObject.hpp
	src/Engine/Data/GameObjectDebug.cpp
	src/Engine/Data/GridMask.hpp
	src/Engine/Data/JsonLua.cpp
	src/Engine/Data/JsonLua.hpp
	src/Engine/Data/Logger.cpp
	src/Engine/Data/Logger.hpp
	src/Engine/Data/Map.cpp
	src/Engine/Data/Map.hpp
	src/Engine/Data/MapState.inl
	src/Engine/Data/Mod.cpp
	src/Engine/Data/Mod.hpp
	src/Engine/Data/ModManager.cpp
	src/Engine/Data/ModManager.hpp
	src/Engine/Data/MultiSound.cpp
	src/Engine/Data/MultiSound.hpp
	src/Engine/Data/Player.cpp
	src/Engine/Data/Player.hpp
	src/Engine/Data/Projectile.cpp
	src/Engine/Data/Projectile.hpp
	src/Engine/Data/Replay.cpp
	src/Engine/Data/Replay.hpp
	src/Engine/Data/SaveState.cpp
	src/Engine/Data/SaveState.hpp
	src/Engine/Data/Requirements/IReq.hpp
	src/Engine/Data/Requirements/ReqAndCombinator.cpp
	src/Engine/Data/Requirements/ReqAndCombinator.hpp
	src/Engine/Data/Requirements/ReqConst.cpp
	src/Engine/Data/Requirements/ReqConst.hpp
	src/Engine/Data/Requirements/Req.cpp
	src/Engine/Data/Requirements/Req.hpp
	src/Engine/Data/Requirements/ReqOrCombinator.cpp
	src/Engine/Data/Requirements/ReqOrCombinator.hpp
	src/Engine/Data/Upgrade.cpp
	src/Engine/Data/Upgrade.hpp
	src/Engine/Data/Warhead.cpp
	src/Engine/Data/Warhead.hpp
	src/Engine/Data/Weapon.cpp
	src/Engine/Data/Weapon.hpp
	src/Engine/Debug/DebugOptions.hpp
	src/Engine/Debug/DebugOverlay.cpp
	src/Engine/Debug/DebugOverlay.hpp
	src/Engine/Debug/DebugPanel.cpp
	src/Engine/Debug/DebugPanel.hpp
	src/Engine/Debug/Profiler.cpp
	src/Engine/Debug/Profiler.hpp
	src/Engine/forward.hpp
	src/Engine/Resources/Assert.cpp
	src/Engine/Resources/Assert.hpp
	src/Engine/Resources/Game.cpp
	src/Engine/Resources/Game.hpp
	src/Engine/Resources/IScene.hpp
	src/Engine/Resources/RandomWrapper.cpp
	src/Engine/Resources/RandomWrapper.hpp
	src/Engine/Resources/SceneArgument.hpp
	src/Engine/Resources/SceneManager.cpp
	src/Engine/Resources/SceneManager.hpp
	src/Engine/SFML/lib.cpp
	src/Engine/SFML/Color.cpp
	src/Engine/SFML/Color.hpp
	src/Engine/SFML/Screen.cpp
	src/Engine/SFML/Screen.hpp
	src/Engine/SFML/Sound.cpp
	src/Engine/SFML/Sound.hpp
	src/Engine/SFML/SoundManager.cpp
	src/Engine/SFML/SoundManager.hpp
	src/Engine/SFML/Sprite.cpp
	src/Engine/SFML/Sprite.hpp
	src/Engine/SFML/TextureManager.cpp
	src/Engine/SFML/TextureManager.hpp
	src/Engine/UI/LuaUI.cpp
	src/Engine/UI/LuaUI.hpp
	src/Engine/UI/TGUILua.cpp
	src/Engine/UI/TGUILua.hpp
	src/Engine/UI/UI.cpp
	src/Engine/UI/UI.hpp
	src/Engine/UI/BaseUI.cpp
	src/Engine/UI/BaseUI.hpp
	src/Engine/UI/CombinedUI.cpp
	src/Engine/UI/CombinedUI.hpp
	src/Engine/Utils.cpp
	src/Engine/Utils.hpp
	src/Engine/Data/SharedLibrary.hpp
	src/Engine/Data/ObjectSpawner.cpp
	src/Engine/Data/ObjectSpawner.hpp
)
add_library(MixRTS_Core SHARED ${MIXRTS_CORE_SOURCES})
target_link_libraries(
	MixRTS_Core
	SFML::Graphics
	SFML::Audio
	SFML::Network
	TGUI::TGUI
	sol2::sol2
	${LUA_LIBRARIES}
	${EXTRA_LIBS}
)
target_compile_definitions(MixRTS_Core PRIVATE MYDLL_EXPORTS)
if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_options(MixRTS_Core PRIVATE -Wl,--exclude-all-symbols)
endif ()

add_executable(
	MixRTS
	src/Game/main.cpp
	src/Game/Debug/Crash.cpp
	src/Game/Debug/Crash.hpp
	src/Game/Scenes/TitleScreen.cpp
	src/Game/Scenes/TitleScreen.hpp
	src/Game/Scenes/LoadingScene.cpp
	src/Game/Scenes/LoadingScene.hpp
	src/Game/Args.cpp
	src/Game/Args.hpp
	src/Game/Scenes/InGame.cpp
	src/Game/Scenes/InGame.hpp
	src/Game/Scenes/ReplayInGame.cpp
	src/Game/Scenes/ReplayInGame.hpp
	${EXTRA_SOURCES}
)
target_link_libraries(MixRTS MixRTS_Core)
add_custom_command(TARGET MixRTS POST_BUILD COMMAND cmake -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

if (NOT NO_TEST)
	# The engine again, but built with MOCK defined (the mocked Screen,
	# TextureManager, SoundManager and UI replace the real ones, see
	# src/Resources/Game.hpp). The mocks live in this library (and not in the
	# test executable) because Game itself needs them.
	add_library(
		MixRTS_Core_Mock SHARED

		${MIXRTS_CORE_SOURCES}

		src/Engine/Mock/MockedScreen.cpp
		src/Engine/Mock/MockedScreen.hpp
		src/Engine/Mock/MockedSoundManager.cpp
		src/Engine/Mock/MockedSoundManager.hpp
		src/Engine/Mock/MockedTextureManager.cpp
		src/Engine/Mock/MockedTextureManager.hpp
		src/Engine/Mock/MockedUI.cpp
		src/Engine/Mock/MockedUI.hpp
	)
	target_link_libraries(
		MixRTS_Core_Mock
		SFML::Graphics
		SFML::Audio
		SFML::Network
		TGUI::TGUI
		sol2::sol2
		${LUA_LIBRARIES}
		${EXTRA_LIBS}
	)
	target_compile_definitions(MixRTS_Core_Mock PRIVATE MYDLL_EXPORTS MOCK)
	if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_options(MixRTS_Core_Mock PRIVATE -Wl,--exclude-all-symbols)
	endif ()

	add_executable(
		MixRTS-tests

		tests/GameFixture.hpp
		tests/GameFixture.cpp
		tests/SaveStateTests.cpp
		tests/DeployTests.cpp
		tests/CarryallTests.cpp
		tests/DebugToolsTests.cpp
		tests/MovementTests.cpp
		tests/TestHelpers.hpp
		tests/WeaponTests.cpp
		tests/AutoAttackTests.cpp
		tests/StealthTests.cpp
		tests/main.cpp
	)
	target_compile_definitions(MixRTS-tests PRIVATE MOCK)
	target_include_directories(MixRTS-tests PRIVATE tests)

	# Deliberately NOT using GTest::gtest_main: tests/main.cpp provides its own
	# main() so that --mode/--video/--size can be parsed from the command line
	# and forwarded to GameFixture::configure() before RUN_ALL_TESTS() (see
	# tests/main.cpp for the exact flags).
	target_link_libraries(
		MixRTS-tests
		MixRTS_Core_Mock
		GTest::gtest
	)

	if (WIN32)
		add_custom_command(
			TARGET MixRTS-tests POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:cpptrace::cpptrace>
			$<TARGET_FILE_DIR:MixRTS-tests>
		)
	endif()

	# Deliberately NOT using gtest_discover_tests(): it needs to actually run the
	# freshly built test executable at build time to enumerate its tests, which
	# is unnecessarily fragile (e.g. when cross-compiling for Windows, the
	# resulting .exe would have to run - through Wine - as part of the Linux
	# build). A single ctest entry running the whole gtest binary is enough.
	# Tests load mods/fonts using paths relative to the repo root, so make sure
	# ctest runs the binary from there regardless of the build directory.
	add_test(NAME MixRTS-tests COMMAND MixRTS-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	add_custom_command(TARGET MixRTS-tests POST_BUILD COMMAND cmake -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/tests_env/assets")
endif ()

set(MODS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mods")
file(GLOB MOD_PATHS RELATIVE "${MODS_DIR}" "${MODS_DIR}/*")
foreach(MOD_NAME ${MOD_PATHS})
	set(FULL_PATH "${MODS_DIR}/${MOD_NAME}")

	# Check if it is a directory and contains its own CMakeLists.txt
	if (IS_DIRECTORY "${FULL_PATH}" AND EXISTS "${FULL_PATH}/CMakeLists.txt")
		message(STATUS "Adding mod: ${MOD_NAME}")
		set(MOD_DEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/mods/${MOD_NAME}")
		set(TEST_MOD_DEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/tests_env/mods/${MOD_NAME}")
		add_subdirectory("mods/${MOD_NAME}")
		add_dependencies(MixRTS "${MOD_NAME}")
		if (NOT NO_TEST)
			add_dependencies(MixRTS-tests "${MOD_NAME}_test")
		endif ()
	endif()
endforeach()

if (WIN32)
	add_custom_command(
		TARGET MixRTS POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:cpptrace::cpptrace>
		$<TARGET_FILE_DIR:MixRTS>
	)
endif()
