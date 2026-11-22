//
// Created by PinkySmile on 09/11/2026.
//

#ifndef MIXRTS_BASEUI_HPP
#define MIXRTS_BASEUI_HPP


#include "Resources/Game.hpp"
#include "Screen.hpp"

void MYDLL_API reloadViews(sf::RenderWindow &win, tgui::Gui &gui, Vector2u size, Vector2f offset, float uiScale);
void MYDLL_API dispFPS(float frameTime);


#endif //MIXRTS_BASEUI_HPP
