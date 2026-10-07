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
include_directories(thirdparty src src/SFML)

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
	add_link_options(-fsanitize=address)
endif ()
if (TSAN)
	add_compile_options(-fsanitize=thread)
	add_link_options(-fsanitize=thread)
endif ()
if (MSAN)
	add_compile_options(-fsanitize=memory)
	add_link_options(-fsanitize=memory)
endif ()
if (UBSAN)
	add_compile_options(-fsanitize=undefined)
	add_link_options(-fsanitize=undefined)
endif ()

# When set, GoogleTest is not downloaded/built and the MixRTS-tests target is not
# built at all.
option(NO_TEST "Disable fetching GoogleTest and building the test target" OFF)
option(NO_TRACE "Disable fetching cpptrace" OFF)

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
	set(EXTRA_SOURCES src/Debug/CrashNone.cpp)
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
		set(EXTRA_SOURCES src/Debug/CrashWindows.cpp src/Debug/CrashHandlerMain.cpp)
	endif (NOT NO_TRACE)
else ()
	if (NOT NO_TRACE)
		set(EXTRA_SOURCES src/Debug/CrashUnix.cpp src/Debug/CrashHandlerMain.cpp)
	endif (NOT NO_TRACE)
endif()

# Game engine sources, shared between the MixRTS executable and the MixRTSTests
# target below. Kept in a single variable so both targets always compile the
# exact same engine code.
add_library(
	MixRTS_Core SHARED

	src/Data/Animation.cpp
	src/Data/Animation.hpp
	src/Data/ConditionalAnim.cpp
	src/Data/ConditionalAnim.hpp
	src/Data/CursorAnimation.cpp
	src/Data/CursorAnimation.hpp
	src/Data/Cursor.cpp
	src/Data/Cursor.hpp
	src/Data/Effect.cpp
	src/Data/Effect.hpp
	src/Data/Extra.cpp
	src/Data/Extra.hpp
	src/Data/Faction.cpp
	src/Data/Faction.hpp
	src/Data/FindPathAStar.cpp
	src/Data/FindPathCrashAndTurn.cpp
	src/Data/FindPath.hpp
	src/Data/GameManager.cpp
	src/Data/GameManager.hpp
	src/Data/GameObject.cpp
	src/Data/GameObject.hpp
	src/Data/GridMask.hpp
	src/Data/JsonLua.cpp
	src/Data/JsonLua.hpp
	src/Data/Logger.cpp
	src/Data/Logger.hpp
	src/Data/Map.cpp
	src/Data/Map.hpp
	src/Data/MapState.inl
	src/Data/Mod.cpp
	src/Data/Mod.hpp
	src/Data/ModManager.cpp
	src/Data/ModManager.hpp
	src/Data/MultiSound.cpp
	src/Data/MultiSound.hpp
	src/Data/Player.cpp
	src/Data/Player.hpp
	src/Data/Projectile.cpp
	src/Data/Projectile.hpp
	src/Data/Requirements/IReq.hpp
	src/Data/Requirements/ReqAndCombinator.cpp
	src/Data/Requirements/ReqAndCombinator.hpp
	src/Data/Requirements/ReqConst.cpp
	src/Data/Requirements/ReqConst.hpp
	src/Data/Requirements/Req.cpp
	src/Data/Requirements/Req.hpp
	src/Data/Requirements/ReqOrCombinator.cpp
	src/Data/Requirements/ReqOrCombinator.hpp
	src/Data/Upgrade.cpp
	src/Data/Upgrade.hpp
	src/Data/Warhead.cpp
	src/Data/Warhead.hpp
	src/Data/Weapon.cpp
	src/Data/Weapon.hpp
	src/forward.hpp
	src/Resources/Assert.cpp
	src/Resources/Assert.hpp
	src/Resources/Game.cpp
	src/Resources/Game.hpp
	src/Resources/IScene.hpp
	src/Resources/RandomWrapper.cpp
	src/Resources/RandomWrapper.hpp
	src/Resources/SceneArgument.hpp
	src/Resources/SceneManager.cpp
	src/Resources/SceneManager.hpp
	src/SFML/Color.cpp
	src/SFML/Color.hpp
	src/SFML/Screen.cpp
	src/SFML/Screen.hpp
	src/SFML/Sound.cpp
	src/SFML/Sound.hpp
	src/SFML/SoundManager.cpp
	src/SFML/SoundManager.hpp
	src/SFML/Sprite.cpp
	src/SFML/Sprite.hpp
	src/SFML/TextureManager.cpp
	src/SFML/TextureManager.hpp
	src/UI/LuaUI.cpp
	src/UI/LuaUI.hpp
	src/UI/TGUILua.cpp
	src/UI/TGUILua.hpp
	src/UI/UI.cpp
	src/UI/UI.hpp
	src/Utils.cpp
	src/Utils.hpp
)
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
	src/main.cpp
	src/Debug/Crash.hpp
	${EXTRA_SOURCES}
)
target_link_libraries(MixRTS MixRTS_Core)
add_custom_command(TARGET MixRTS POST_BUILD COMMAND cmake -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

set(MODS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mods")
file(GLOB MOD_PATHS RELATIVE "${MODS_DIR}" "${MODS_DIR}/*")
foreach(MOD_NAME ${MOD_PATHS})
	set(FULL_PATH "${MODS_DIR}/${MOD_NAME}")

	# Check if it is a directory and contains its own CMakeLists.txt
	if (IS_DIRECTORY "${FULL_PATH}" AND EXISTS "${FULL_PATH}/CMakeLists.txt")
		message(STATUS "Adding mod: ${MOD_NAME}")
		set(MOD_DEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/mods/${MOD_NAME}")
		add_subdirectory("mods/${MOD_NAME}")
		add_dependencies(MixRTS "${MOD_NAME}")
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

# Test target: same engine sources, but with the Screen/TextureManager/SoundManager
# classes swapped out for MockedScreen/MockedTextureManager/MockedSoundManager (see
# SCREEN_CLASS_IMPLEM/TEXTURE_MANAGER_CLASS_IMPLEM/SOUND_MANAGER_CLASS_IMPLEM usage
# in src/Resources/Game.hpp) so that tests can run fast and headless: without a
# real, visible window, without decoding real texture files, and without needing a
# real audio device. Entirely skipped when NO_TEST is set (this also means
# GoogleTest is never even fetched, see above).
if (NOT NO_TEST)
	add_executable(
		MixRTS-tests

		src/Mock/MockedScreen.cpp
		src/Mock/MockedScreen.hpp
		src/Mock/MockedSoundManager.cpp
		src/Mock/MockedSoundManager.hpp
		src/Mock/MockedTextureManager.cpp
		src/Mock/MockedTextureManager.hpp
		src/Mock/MockedUI.cpp
		src/Mock/MockedUI.hpp

		tests/GameFixture.hpp
		tests/GameFixture.cpp
		tests/DeployTests.cpp
		tests/CarryallTests.cpp
		tests/WeaponTests.cpp
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
		MixRTS_Core
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
endif ()
