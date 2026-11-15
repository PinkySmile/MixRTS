//
// Created by PinkySmile on 04/10/2026.
//

#ifndef MIXRTS_LUAUI_HPP
#define MIXRTS_LUAUI_HPP


#include <sol/sol.hpp>
#include "UI.hpp"

class MYDLL_API LuaUI : public UI {
public:
	LuaUI(const Faction &faction, UIState *ostate, GameManager *manager, Player *owner, MapState *map, sol::state &state, const std::filesystem::path &path);
	~LuaUI() override = default;
	void setEnabled(bool enabled) override;

	void playEVA(EVASound sound, unsigned expireIn, unsigned priority, unsigned cooldown, const std::string &args) override;
	std::optional<Event> pollEvent() override;
	void renderAbove() override;
	void updateRender() override;

	void onCancel(Vector2i absPos) override;
	void tileClicked(Vector2i absPos) override;
	void mouseMoved(Vector2i absPos) override;
	void onKeyPressed(sf::Keyboard::Key key, bool ctrl, bool shift, bool alt, bool meta) override;
	void update() override;
	void render() override;

protected:
	void _displaySelectedObjectUI(const GameObject &object) override;

	sol::table _table;
	sol::environment _env;
};


#endif //MIXRTS_LUAUI_HPP
