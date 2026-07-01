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
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/src=src" "-ffile-prefix-map=${CMAKE_SOURCE_DIR}/src=src")
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

add_executable(
	MixRTS
	src/Data/Animation.cpp
	src/Data/Animation.hpp
	src/Data/ConditionalAnim.cpp
	src/Data/ConditionalAnim.hpp
	src/Data/CursorAnimation.cpp
	src/Data/CursorAnimation.hpp
	src/Data/Cursor.cpp
	src/Data/Cursor.hpp
	src/Data/Logger.cpp
	src/Data/Logger.hpp
	src/Data/Mod.cpp
	src/Data/Mod.hpp
	src/Data/ModManager.cpp
	src/Data/ModManager.hpp
	src/main.cpp
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
	src/Utils.cpp
	src/Utils.hpp
	src/forward.hpp
	src/D2K/D2KUI.cpp
	src/D2K/D2KUI.hpp
	src/UI.hpp
	src/Data/Faction.cpp
	src/Data/Faction.hpp
	src/D2K/D2KCameo.cpp
	src/D2K/D2KCameo.hpp
	src/UI.cpp
	src/Data/Player.cpp
	src/Data/Player.hpp
	src/Data/GameManager.cpp
	src/Data/GameManager.hpp
	src/Data/Requirements/ReqAndCombinator.cpp
	src/Data/Requirements/ReqAndCombinator.hpp
	src/Data/Requirements/ReqOrCombinator.cpp
	src/Data/Requirements/ReqOrCombinator.hpp
	src/Data/Requirements/Req.cpp
	src/Data/Requirements/Req.hpp
	src/Data/Requirements/IReq.hpp
	src/Data/Upgrade.cpp
	src/Data/Upgrade.hpp
	src/Data/Requirements/ReqConst.cpp
	src/Data/Requirements/ReqConst.hpp
	src/Data/GameObject.cpp
	src/Data/GameObject.hpp
	src/Data/MultiSound.cpp
	src/Data/MultiSound.hpp
	src/Data/FindPathAStar.cpp
	src/Data/FindPathCrashAndTurn.cpp
	src/Data/FindPath.hpp
	src/Data/MapState.inl
	src/Data/GridMask.hpp
	src/Data/Map.cpp
	src/Data/Map.hpp
)

target_link_libraries(
	MixRTS
	SFML::Graphics
	SFML::Audio
	SFML::Network
	TGUI::TGUI
)