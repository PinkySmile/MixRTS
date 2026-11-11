//
// Created by PinkySmile on 18/09/2021.
//

#include <filesystem>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <TGUI/Texture.hpp>
#include "Game.hpp"
#include "Assert.hpp"
#include "Data/Logger.hpp"
#include "UI/UI.hpp"
#include "UI/LuaUI.hpp"
#include "UI/TGUILua.hpp"
#include "Data/Warhead.hpp"
#include "Data/Effect.hpp"
#include "Data/Upgrade.hpp"
#include "Data/Weapon.hpp"
#include "Data/Animation.hpp"
#include "Data/ConditionalAnim.hpp"
#include "Data/Extra.hpp"
#include "Data/Faction.hpp"
#include "Data/GameObject.hpp"
#include "Data/Player.hpp"
#include "Data/GameManager.hpp"
#include "Data/Projectile.hpp"
#include "Data/Mod.hpp"
#include "Data/CursorAnimation.hpp"
#include "SFML/Sound.hpp"

MYDLL_API Game *game = nullptr;
MYDLL_API float guiScale = 1.5f;
#ifdef _DEBUG
MYDLL_API bool DEBUG_MODE = false;
#endif

void Game::pushUserType(sol::state &state)
{
	sol::usertype<Game> type = state.new_usertype<Game>("Game");

	//type["locale"] = &Game::locale;
	type["paused"] = &Game::paused;
	// Minimal immediate-mode drawing API for UI scripts (e.g. drawing a
	// health bar). Deliberately NOT real methods on Game (no "self" of type
	// Game is taken), so they're called as game.render_rect(...) /
	// game.render_sprite(...) rather than game:render_rect(...).
	type["render_rect"] = [](
		Vector2f position, Vector2f size, Color fillColor,
		sol::optional<Color> outlineColor, sol::optional<float> outlineThickness
	) {
		sf::RectangleShape rect;

		rect.setPosition(position);
		rect.setSize(size);
		rect.setFillColor(fillColor);
		if (outlineColor)
			rect.setOutlineColor(*outlineColor);
		if (outlineThickness)
			rect.setOutlineThickness(*outlineThickness);
		::game->screen->draw(rect);
	};
	type["render_sprite"] = [](
		const tgui::Texture &texture, Vector2f position, sol::optional<Color> tint
	) {
		auto data = texture.getData();

		if (!data || !data->backendTexture)
			return;

		auto &backendTexture = *data->backendTexture;
		sf::Image image{backendTexture.getSize(), backendTexture.getPixels()};
		sf::Texture sfTexture;

		if (!sfTexture.loadFromImage(image))
			return;

		sf::Sprite sprite{sfTexture};

		sprite.setPosition(position);
		if (tint)
			sprite.setColor(*tint);
		::game->screen->draw(sprite);
	};
	// Lists the direct (non-recursive) contents of a directory. Returns an
	// array of {name = <filename stem, no extension>, path = <full path>}.
	// Used by UI scripts to enumerate things like per-faction cameo images,
	// mirroring what the engine itself does with std::filesystem::directory_iterator
	// (which has no Lua binding of its own).
	type["list_directory"] = [&state](const std::string &path) {
		sol::table entries = state.create_table();
		int i = 1;

		if (!std::filesystem::exists(path))
			return entries;
		for (auto &entry : std::filesystem::directory_iterator(path)) {
			entries[i++] = state.create_table_with(
				"name", entry.path().filename().stem().string(),
				"path", entry.path().string()
			);
		}
		return entries;
	};
}

void Game::init()
{
	// TODO: Remove dangerous libraries/functions such as os.execute or arbitrary file write functions
	this->lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::debug,
		sol::lib::bit32,
		sol::lib::io,
		sol::lib::utf8
	);
	// Order doesn't matter for sol2 usertype registration (each new_usertype
	// call is independent), but types are registered roughly leaf-first for
	// readability.
	Warhead::pushUserType(this->lua);
	Effect::pushUserType(this->lua);
	Upgrade::pushUserType(this->lua);
	Weapon::pushUserType(this->lua);
	Animation::pushUserType(this->lua);
	ConditionalAnim::pushUserType(this->lua);
	Position::pushUserType(this->lua);
	ExtraSpawn::pushUserType(this->lua);
	Mod::pushUserType(this->lua);
	Faction::pushUserType(this->lua);
	GameObject::pushUserType(this->lua);
	Projectile::pushUserType(this->lua);
	Player::pushUserType(this->lua);
	GameManager::pushUserType(this->lua);
	CursorAnimation::pushUserType(this->lua);
	Sound::pushUserType(this->lua);
	UI::pushUserType(this->lua);
	pushTGUIUserTypes(this->lua);
	Game::pushUserType(this->lua);
	this->screen->setFramerateLimit(50);
	this->screen->setFont(this->font);
	assert_exp(!::game);
	::game = this;
	this->lua["game"] = this;

#ifdef _WIN32
	ShowWindow(this->screen->getNativeHandle(), SW_MAXIMIZE);
#endif
}

#ifdef MOCK
Game::Game(const std::filesystem::path &fontPath, MockMode mode, const std::filesystem::path &video, Vector2u size, const std::filesystem::path &loggerPath) :
	gameRandom(random()),
	logger(loggerPath),
	font(fontPath),
	screen(new ScreenImpl("MixRTS", mode, video, size)),
	gui(this->screen->getRenderTarget()),
	textureMgr(mode),
	soundMgr(mode, video)
{
	this->init();
}
#else
Game::Game(const std::filesystem::path &fontPath, const std::filesystem::path &loggerPath) :
	gameRandom(random()),
	logger(loggerPath),
	font(fontPath),
	screen(new ScreenImpl("MixRTS")),
	gui(this->screen->getRenderTarget())
{
	this->init();
}
#endif