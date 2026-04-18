cmake_minimum_required(VERSION 4.1)
project(MixRTS)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(_USE_MATH_DEFINES)

set(TGUI_SHARED_LIBS ON)
set(SFML_STATIC_LIBRARIES OFF)
find_package(TGUI)
find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Network)
include_directories(thirdparty src src/SFML)

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

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-W -Wall -Wextra)
	endif ()
	add_compile_definitions(_DEBUG=1)
else ()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if (WIN32)
			add_link_options(-mwindows)
		endif()
		add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/src=src" "-ffile-prefix-map=${CMAKE_SOURCE_DIR}/src=src")
	endif ()
	add_compile_definitions(NDEBUG=1)
endif ()

add_executable(
	MixRTS
	src/Data/Animation.cpp
	src/Data/Animation.hpp
	src/Data/Building.cpp
	src/Data/Building.hpp
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
)

target_link_libraries(
	MixRTS
	SFML::Graphics
	SFML::Audio
	SFML::Network
	TGUI::TGUI
)